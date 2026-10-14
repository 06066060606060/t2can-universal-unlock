#pragma once
#include "can_performance_pure.h"
#include "can_service_policy_pure.h"

enum CanPerfBus : uint8_t { CAN_PERF_BODY, CAN_PERF_CHASSIS, CAN_PERF_PARTY };
static portMUX_TYPE canPerfMux = portMUX_INITIALIZER_UNLOCKED;
static CanPerfBusPure canPerfBuses[3] = {};
struct CanPerfRxContext {
  TaskHandle_t owner;
  uint32_t readUs;
  bool active;
};
static CanPerfRxContext canPerfRxContexts[3] = {};

// Latencies start at the software read, never at physical CAN arrival.
static MCP2515::ERROR canPerfRead(MCP2515 &controller, CanPerfBus bus,
                                 can_frame *frame, uint32_t *readUs = nullptr) {
#if TMR_CAN_DETAILED_STATS
  const uint32_t started = (uint32_t)micros();
#endif
  MCP2515::RXBn buffer = MCP2515::RXB0;
#if TMR_CAN_DETAILED_STATS
  uint8_t status = 0;
  const MCP2515::ERROR result = controller.readMessage(frame, &buffer, &status);
  const uint32_t ended = (uint32_t)micros();
  if (readUs) *readUs = ended;
#else
  const MCP2515::ERROR result = controller.readMessage(frame, &buffer);
  // This optional output is used only by CanPerfRxScope, which is a no-op
  // in the essential build. Do not pay for a timestamp that no one consumes.
  if (readUs) *readUs = 0;
#endif
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
#if TMR_CAN_DETAILED_STATS
  canPerfPollObservePure(s, started);
  // The driver's RX poll already read TXREQ. No extra SPI read is needed.
  canPerfStatusObservePure(s, status);
#else
  ++s.polls;
#endif
  if (result == MCP2515::ERROR_OK) {
    ++s.rxFrames;
    if (buffer == MCP2515::RXB0) ++s.rx0Frames;
    else ++s.rx1Frames;
    if (bus == CAN_PERF_PARTY && frame && frame->can_id == 0x370u && frame->can_dlc > 0)
      ++s.rx370;
#if TMR_CAN_DETAILED_STATS
    canPerfTimingObservePure(s.rxRead, ended - started);
#endif
  } else if (result != MCP2515::ERROR_NOMSG) ++s.rxErrors;
  portEXIT_CRITICAL(&canPerfMux);
  return result;
}

// A frame context is attributed only to TX by the task decoding this bus.
// Web/BLE TX on another task must not inherit its RX timestamp.
class CanPerfRxScope {
public:
#if TMR_CAN_DETAILED_STATS
  CanPerfRxScope(CanPerfBus bus, uint32_t readUs) : bus_(bus), started_((uint32_t)micros()) {
    const TaskHandle_t owner = xTaskGetCurrentTaskHandle();
    portENTER_CRITICAL(&canPerfMux);
    canPerfTimingObservePure(canPerfBuses[bus].rxDispatch, started_ - readUs);
    canPerfRxContexts[bus] = {owner, readUs, true};
    portEXIT_CRITICAL(&canPerfMux);
  }
  ~CanPerfRxScope() {
    const uint32_t ended = (uint32_t)micros();
    portENTER_CRITICAL(&canPerfMux);
    canPerfTimingObservePure(canPerfBuses[bus_].rxProcess, ended - started_);
    canPerfRxContexts[bus_].active = false;
    portEXIT_CRITICAL(&canPerfMux);
  }
#else
  CanPerfRxScope(CanPerfBus, uint32_t) {}
#endif
  CanPerfRxScope(const CanPerfRxScope &) = delete;
  CanPerfRxScope &operator=(const CanPerfRxScope &) = delete;
private:
#if TMR_CAN_DETAILED_STATS
  CanPerfBus bus_;
  uint32_t started_;
#endif
};

static MCP2515::ERROR canPerfSend(MCP2515 &controller, CanPerfBus bus,
                                 const can_frame *frame) {
#if TMR_CAN_DETAILED_STATS
  const uint32_t started = (uint32_t)micros();
#endif
  const MCP2515::ERROR result = controller.sendMessage(frame);
#if TMR_CAN_DETAILED_STATS
  const uint32_t ended = (uint32_t)micros();
  const TaskHandle_t owner = xTaskGetCurrentTaskHandle();
#endif
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
  ++s.txAttempts;
#if TMR_CAN_DETAILED_STATS
  canPerfTimingObservePure(s.txCall, ended - started);
#endif
  if (result == MCP2515::ERROR_OK) {
    ++s.txQueued;
    if (bus == CAN_PERF_PARTY && frame && frame->can_id == 0x370u && frame->can_dlc > 0)
      ++s.tx370;
#if TMR_CAN_DETAILED_STATS
    const CanPerfRxContext &rx = canPerfRxContexts[bus];
    if (rx.active && rx.owner == owner)
      canPerfTimingObservePure(s.rxToEnqueue, ended - rx.readUs);
#endif
  } else if (result == MCP2515::ERROR_ALLTXBUSY) ++s.txBusy;
  else ++s.txErrors;
  if (result != MCP2515::ERROR_OK && bus == CAN_PERF_PARTY &&
      frame && frame->can_id == 0x370u && frame->can_dlc > 0)
    ++s.tx370Errors;
  portEXIT_CRITICAL(&canPerfMux);
  return result;
}

static void canPerfObserveOverflow(CanPerfBus bus, uint8_t flags) {
  portENTER_CRITICAL(&canPerfMux);
  canPerfOverflowObservePure(canPerfBuses[bus], flags);
  portEXIT_CRITICAL(&canPerfMux);
}

// Only runtime recovery paths call this; initial boot setup is not a recovery.
static void canPerfObserveRecovery(CanPerfBus bus, bool ok) {
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
  ++s.recoveryAttempts;
  if (ok) ++s.recoveries;
  else ++s.recoveryErrors;
  portEXIT_CRITICAL(&canPerfMux);
}

// Each RX owner reads its own controller, at most once per second. No RX
// interrupt or overflow flag is cleared by this readback.
static void canPerfSampleRxConfig(MCP2515 &controller, CanPerfBus bus) {
  // Each entry is owned by its bus's single RX task. Statistics reset does
  // not change this schedule: configuration is republished within one second.
  // Avoid acquiring the statistics lock on every empty/active receive pass.
  static uint32_t sampledAtMs[3] = {};
  static bool sampled[3] = {};
  const uint32_t nowMs = (uint32_t)millis();
  if (sampled[bus] && (uint32_t)(nowMs - sampledAtMs[bus]) < 1000u) return;
  sampled[bus] = true;
  sampledAtMs[bus] = nowMs;
  uint8_t rx0, rx1;
  controller.getRxBufferConfiguration(rx0, rx1);
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
  s.rx0Ctrl = rx0;
  s.rx1Ctrl = rx1;
  s.rxConfigLastUs = (uint32_t)micros();
  ++s.rxConfigSamples;
  portEXIT_CRITICAL(&canPerfMux);
}

static CanPerfBusPure canPerfSnapshot(CanPerfBus bus) {
  portENTER_CRITICAL(&canPerfMux);
  const CanPerfBusPure result = canPerfBuses[bus];
  portEXIT_CRITICAL(&canPerfMux);
  return result;
}

static void canPerfReset() {
  portENTER_CRITICAL(&canPerfMux);
  for (uint8_t i = 0; i < 3; ++i) canPerfBuses[i] = {};
  // Keep in-flight frame contexts: resetting statistics is not cancellation.
  portEXIT_CRITICAL(&canPerfMux);
}

static void canPerfClearRxContexts() {
  portENTER_CRITICAL(&canPerfMux);
  for (uint8_t i = 0; i < 3; ++i) canPerfRxContexts[i] = {};
  portEXIT_CRITICAL(&canPerfMux);
}

static void canRxFinishBurst(bool received, CanRxPauseStatePure &state) {
  const uint32_t now = (uint32_t)micros();
  if (canRxShouldSleepPure(received, state, now)) {
    // Allow idle/watchdog and lower-priority tasks to run periodically.
    vTaskDelay(1);
    state.burstStartedUs = (uint32_t)micros();
  } else {
    // Equal-priority RX peer on core 1 gets the next cooperative slice.
    taskYIELD();
  }
}
