#include <cstdint>
#include <cassert>
#include <cstdio>
#include <initializer_list>

using TaskHandle_t = void *;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
static uint32_t clockUs = 0;
static TaskHandle_t task = reinterpret_cast<void *>(1);
static unsigned sleeps = 0, yields = 0;
static uint32_t micros() { return clockUs; }
static TaskHandle_t xTaskGetCurrentTaskHandle() { return task; }
static void vTaskDelay(int ticks) { clockUs += ticks * 1000; ++sleeps; }
static void taskYIELD() { ++yields; }
struct can_frame { uint32_t can_id = 0; uint8_t can_dlc = 8; };
struct MCP2515 {
  enum ERROR { ERROR_OK, ERROR_FAIL, ERROR_ALLTXBUSY, ERROR_FAILTX, ERROR_NOMSG };
  ERROR readResult = ERROR_OK, sendResult = ERROR_OK;
  uint8_t status = 0;
  ERROR readMessage(can_frame *) { clockUs += 20; return readResult; }
  ERROR sendMessage(const can_frame *) { clockUs += 30; return sendResult; }
  uint8_t getStatus() { return status; }
};
#include "can_performance.h"

int main() {
  MCP2515 controller;
  can_frame frame;
  uint32_t receivedUs;
  assert(canPerfRead(controller, CAN_PERF_BODY, &frame, &receivedUs) == MCP2515::ERROR_OK);
  clockUs += 10;
  {
    CanPerfRxScope scope(CAN_PERF_BODY, receivedUs);
    assert(canPerfSend(controller, CAN_PERF_BODY, &frame) == MCP2515::ERROR_OK);
    // Other task TX and other bus TX are not attributed to this RX frame.
    task = reinterpret_cast<void *>(2);
    canPerfSend(controller, CAN_PERF_BODY, &frame);
    task = reinterpret_cast<void *>(1);
    canPerfSend(controller, CAN_PERF_PARTY, &frame);
  }
  canPerfSend(controller, CAN_PERF_BODY, &frame); // periodic, outside RX scope
  controller.sendResult = MCP2515::ERROR_ALLTXBUSY;
  canPerfSend(controller, CAN_PERF_BODY, &frame);
  controller.sendResult = MCP2515::ERROR_FAILTX;
  canPerfSend(controller, CAN_PERF_BODY, &frame);
  controller.readResult = MCP2515::ERROR_NOMSG;
  canPerfRead(controller, CAN_PERF_BODY, &frame);
  controller.readResult = MCP2515::ERROR_FAIL;
  canPerfRead(controller, CAN_PERF_BODY, &frame);
  controller.status = 0x54;
  canPerfSampleTx(controller, CAN_PERF_BODY);
  canPerfObserveOverflow(CAN_PERF_BODY, 0x80);
  const auto snapshot = canPerfSnapshot(CAN_PERF_BODY);
  assert(snapshot.rxFrames == 1 && snapshot.rxErrors == 1 && snapshot.polls == 3);
  assert(snapshot.rxRead.count == 1 && snapshot.rxRead.maxUs == 20);
  assert(snapshot.rxDispatch.maxUs == 10 && snapshot.rxProcess.maxUs == 90);
  assert(snapshot.txAttempts == 5 && snapshot.txQueued == 3 && snapshot.txBusy == 1 && snapshot.txErrors == 1);
  assert(snapshot.rxToEnqueue.count == 1 && snapshot.rxToEnqueue.maxUs == 40);
  assert(snapshot.txPendingLast == 3 && snapshot.txFullSamples == 1);
  assert(snapshot.rxOverflowEvents == 1 && snapshot.rx1OverflowEvents == 1);
  assert(canPerfSnapshot(CAN_PERF_PARTY).rxToEnqueue.count == 0);

  // RAII unwinds the context on early return, including invalid-frame paths.
  auto earlyReturn = [&]() { CanPerfRxScope scope(CAN_PERF_BODY, clockUs); return; };
  earlyReturn();
  assert(!canPerfRxContexts[CAN_PERF_BODY].active);
  canPerfRxContexts[CAN_PERF_BODY] = {task, clockUs, true};
  canPerfClearRxContexts();
  assert(!canPerfRxContexts[CAN_PERF_BODY].active);
  { CanPerfRxScope scope(CAN_PERF_BODY, clockUs); canPerfReset(); assert(canPerfRxContexts[CAN_PERF_BODY].active); }
  assert(canPerfSnapshot(CAN_PERF_BODY).txAttempts == 0);

  // Raw PARTY 0x370 reception must count even without a NAG/profile decision.
  canPerfReset();
  controller.readResult = MCP2515::ERROR_OK;
  frame.can_id = 0x370;
  canPerfRead(controller, CAN_PERF_PARTY, &frame);
  canPerfRead(controller, CAN_PERF_BODY, &frame);
  for (uint32_t id : {0x80000370u, 0x40000370u, 0x399u}) {
    frame.can_id = id;
    canPerfRead(controller, CAN_PERF_PARTY, &frame);
  }
  frame.can_id = 0x370;
  frame.can_dlc = 0;
  canPerfRead(controller, CAN_PERF_PARTY, &frame);
  frame.can_dlc = 8;
  controller.readResult = MCP2515::ERROR_NOMSG;
  canPerfRead(controller, CAN_PERF_PARTY, &frame);
  assert(canPerfSnapshot(CAN_PERF_PARTY).rx370 == 1);
  assert(canPerfSnapshot(CAN_PERF_BODY).rx370 == 0);
  controller.sendResult = MCP2515::ERROR_OK;
  canPerfSend(controller, CAN_PERF_PARTY, &frame);
  canPerfSend(controller, CAN_PERF_CHASSIS, &frame);
  controller.sendResult = MCP2515::ERROR_ALLTXBUSY;
  canPerfSend(controller, CAN_PERF_PARTY, &frame);
  controller.sendResult = MCP2515::ERROR_FAILTX;
  canPerfSend(controller, CAN_PERF_PARTY, &frame);
  canPerfSend(controller, CAN_PERF_PARTY, nullptr);
  assert(canPerfSnapshot(CAN_PERF_PARTY).tx370 == 1);
  assert(canPerfSnapshot(CAN_PERF_PARTY).tx370Errors == 2);
  assert(canPerfSnapshot(CAN_PERF_CHASSIS).tx370 == 0);
  for (auto bus : {CAN_PERF_BODY, CAN_PERF_CHASSIS, CAN_PERF_PARTY}) {
    canPerfObserveRecovery(bus, true);
    canPerfObserveRecovery(bus, false);
    const auto counters = canPerfSnapshot(bus);
    assert(counters.recoveryAttempts == 2 && counters.recoveries == 1 && counters.recoveryErrors == 1);
  }
  canPerfReset();
  assert(canPerfSnapshot(CAN_PERF_PARTY).rx370 == 0);
  assert(canPerfSnapshot(CAN_PERF_PARTY).tx370 == 0);
  assert(canPerfSnapshot(CAN_PERF_PARTY).recoveries == 0);
  assert(canPerfSnapshot(CAN_PERF_BODY).recoveryAttempts == 0);
  uint32_t burst = clockUs;
  canRxFinishBurst(true, burst);
  assert(yields == 1 && sleeps == 0);
  clockUs += 2000;
  canRxFinishBurst(true, burst);
  assert(sleeps == 1 && burst == clockUs);
  canRxFinishBurst(false, burst);
  assert(sleeps == 2);
  std::puts("PASS: software timing, TX saturation/error separation, per-task RX attribution, reset and watchdog pauses");
}
