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
  const uint32_t started = (uint32_t)micros();
  const MCP2515::ERROR result = controller.readMessage(frame);
  const uint32_t ended = (uint32_t)micros();
  if (readUs) *readUs = ended;
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
  canPerfPollObservePure(s, started);
  if (result == MCP2515::ERROR_OK) {
    ++s.rxFrames;
    canPerfTimingObservePure(s.rxRead, ended - started);
  } else if (result != MCP2515::ERROR_NOMSG) ++s.rxErrors;
  portEXIT_CRITICAL(&canPerfMux);
  return result;
}

// A frame context is attributed only to TX by the task decoding this bus.
// Web/BLE TX on another task must not inherit its RX timestamp.
class CanPerfRxScope {
public:
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
  CanPerfRxScope(const CanPerfRxScope &) = delete;
  CanPerfRxScope &operator=(const CanPerfRxScope &) = delete;
private:
  CanPerfBus bus_;
  uint32_t started_;
};

static MCP2515::ERROR canPerfSend(MCP2515 &controller, CanPerfBus bus,
                                 const can_frame *frame) {
  const uint32_t started = (uint32_t)micros();
  const MCP2515::ERROR result = controller.sendMessage(frame);
  const uint32_t ended = (uint32_t)micros();
  const TaskHandle_t owner = xTaskGetCurrentTaskHandle();
  portENTER_CRITICAL(&canPerfMux);
  CanPerfBusPure &s = canPerfBuses[bus];
  ++s.txAttempts;
  canPerfTimingObservePure(s.txCall, ended - started);
  if (result == MCP2515::ERROR_OK) {
    ++s.txQueued;
    const CanPerfRxContext &rx = canPerfRxContexts[bus];
    if (rx.active && rx.owner == owner)
      canPerfTimingObservePure(s.rxToEnqueue, ended - rx.readUs);
  } else if (result == MCP2515::ERROR_ALLTXBUSY) ++s.txBusy;
  else ++s.txErrors;
  portEXIT_CRITICAL(&canPerfMux);
  return result;
}

static void canPerfObserveOverflow(CanPerfBus bus, uint8_t flags) {
  portENTER_CRITICAL(&canPerfMux);
  canPerfOverflowObservePure(canPerfBuses[bus], flags);
  portEXIT_CRITICAL(&canPerfMux);
}

static void canPerfSampleTx(MCP2515 &controller, CanPerfBus bus) {
  const uint8_t status = controller.getStatus();
  portENTER_CRITICAL(&canPerfMux);
  canPerfStatusObservePure(canPerfBuses[bus], status);
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

static void canRxFinishBurst(bool received, uint32_t &burstStartedUs) {
  const uint32_t now = (uint32_t)micros();
  if (canRxShouldSleepPure(received, burstStartedUs, now)) {
    // Allow idle/watchdog and lower-priority tasks to run periodically.
    vTaskDelay(1);
    burstStartedUs = (uint32_t)micros();
  } else {
    // Equal-priority RX peer on core 1 gets the next cooperative slice.
    taskYIELD();
  }
}
