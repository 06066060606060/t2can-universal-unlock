// c++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter
//     tests/board_can_test.cpp -o /tmp/board_can_test && /tmp/board_can_test
#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <iostream>

enum { INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, LOW = 0, HIGH = 1 };
static int modes[40], levels[40], upBits, downBits;
static std::vector<int> driven;
static bool glitch;
static unsigned reads;
static void pinMode(int pin, int mode) { modes[pin] = mode; if (mode == OUTPUT) driven.push_back(pin); }
static void digitalWrite(int pin, int level) { levels[pin] = level; }
static int digitalRead(int pin) {
  const int bits = modes[pin] == INPUT_PULLUP ? upBits : downBits;
  return ((bits >> (pin - 15)) & 1) ^ (glitch && ++reads == 7);
}
static void delayMicroseconds(unsigned) {}
using TickType_t = uint32_t;
static uint32_t ticks;
static uint32_t millis() { return ticks; }
static uint32_t xTaskGetTickCount() { return ticks; }
static void vTaskDelay(uint32_t n) { ticks += n; }
constexpr int pdTRUE = 1, pdPASS = 1;
constexpr uint32_t portMAX_DELAY = UINT32_MAX;
using SemaphoreHandle_t = bool *;
static bool mutexHeld;
static SemaphoreHandle_t xSemaphoreCreateMutex() { return &mutexHeld; }
static int xSemaphoreTake(bool *m, TickType_t wait) {
  if (*m) { assert(!wait); return 0; }
  *m = true; return pdTRUE;
}
static void xSemaphoreGive(bool *m) { assert(*m); *m = false; }
struct Queue { size_t capacity, itemSize; std::deque<std::vector<uint8_t>> items; };
using QueueHandle_t = Queue *;
static QueueHandle_t xQueueCreate(size_t n, size_t size) { return new Queue{n, size, {}}; }
static int xQueueSend(Queue *q, const void *value, TickType_t) {
  if (q->items.size() == q->capacity) return 0;
  const auto p = static_cast<const uint8_t *>(value);
  q->items.emplace_back(p, p + q->itemSize); return pdTRUE;
}
static int xQueueReceive(Queue *q, void *value, TickType_t) {
  if (q->items.empty()) return 0;
  memcpy(value, q->items.front().data(), q->itemSize); q->items.pop_front(); return pdTRUE;
}
static void xQueueReset(Queue *q) { q->items.clear(); }
static unsigned uxQueueMessagesWaiting(Queue *q) { return q->items.size(); }
using TaskHandle_t = void *;
using BaseType_t = int;
constexpr int pdFALSE = 0, FALLING = 2;
#define ARDUINO_ISR_ATTR
static unsigned txWakes, irqWakes, isrYields, irqAttachments;
static void (*irqHandler)();
static int digitalPinToInterrupt(int pin) { return pin; }
static void attachInterrupt(int pin, void (*fn)(), int edge) {
  assert(pin == 5 && edge == FALLING); irqHandler = fn; irqAttachments++;
}
static void xTaskNotifyGive(TaskHandle_t task) { assert(task); txWakes++; }
static void vTaskNotifyGiveFromISR(TaskHandle_t task, BaseType_t *woken) {
  assert(task); irqWakes++; *woken = pdTRUE;
}
static void portYIELD_FROM_ISR() { isrYields++; }
static uint32_t ulTaskNotifyTake(int, TickType_t) { return 0; }
static int xTaskCreatePinnedToCore(void (*)(void *), const char *, int, void *, int, void **task, int) {
  *task = &ticks; return pdPASS;
}
class Preferences {
 public:
  inline static std::map<std::string, uint8_t> values;
  bool begin(const char *, bool) { return true; }
  void end() {}
  uint8_t getUChar(const char *k, uint8_t d) { return values.count(k) ? values[k] : d; }
  bool getBool(const char *k, bool d) { return getUChar(k, d); }
  size_t putUChar(const char *k, uint8_t v) { values[k] = v; return 1; }
  size_t putBool(const char *k, bool v) { return putUChar(k, v); }
};
using esp_err_t = int;
enum { ESP_OK, ESP_FAIL, ESP_ERR_INVALID_STATE, ESP_ERR_NO_MEM, ESP_ERR_INVALID_ARG, ESP_ERR_TIMEOUT };
enum { TWAI_STATE_STOPPED, TWAI_STATE_RUNNING, TWAI_STATE_BUS_OFF, TWAI_STATE_RECOVERING };
enum { TWAI_ALERT_RX_QUEUE_FULL = 1, TWAI_ALERT_BUS_OFF = 2, TWAI_ALERT_RX_DATA = 4,
       TWAI_ALERT_TX_SUCCESS = 8, TWAI_ALERT_TX_FAILED = 16, TWAI_ALERT_TX_IDLE = 32 };
struct twai_status_info_t {
  int state; uint32_t tx_error_counter, rx_error_counter, tx_failed_count,
      rx_overrun_count, rx_missed_count, msgs_to_rx, msgs_to_tx;
};
struct twai_message_t { bool extd, rtr, self, ss; uint32_t identifier; uint8_t data_length_code, data[8]; };
struct twai_general_config_t { unsigned rx_queue_len = 256, tx_queue_len = 16; };
struct twai_timing_config_t {};
struct twai_filter_config_t {};
static std::vector<std::string> nativeCalls;
#define NATIVE(name, args) static esp_err_t name args { nativeCalls.push_back(#name); return ESP_OK; }
NATIVE(twai_driver_install, (const twai_general_config_t *, const twai_timing_config_t *, const twai_filter_config_t *))
NATIVE(twai_driver_uninstall, ()) NATIVE(twai_start, ()) NATIVE(twai_stop, ())
NATIVE(twai_initiate_recovery, ()) NATIVE(twai_clear_transmit_queue, ())
NATIVE(twai_transmit, (const twai_message_t *, TickType_t))
NATIVE(twai_receive, (twai_message_t *, TickType_t))
NATIVE(twai_get_status_info, (twai_status_info_t *))
NATIVE(twai_reconfigure_alerts, (uint32_t, uint32_t *))
NATIVE(twai_read_alerts, (uint32_t *, TickType_t))
constexpr uint32_t CAN_EFF_FLAG = 0x80000000, CAN_RTR_FLAG = 0x40000000,
                   CAN_EFF_MASK = 0x1fffffff, CAN_SFF_MASK = 0x7ff;
struct can_frame { uint32_t can_id; uint8_t can_dlc, data[8]; };
enum CAN_SPEED { CAN_500KBPS }; enum CAN_CLOCK { MCP_16MHZ };
static int SPI;
class MCP2515 {
 public:
  enum ERROR { ERROR_OK, ERROR_FAIL, ERROR_ALLTXBUSY, ERROR_NOMSG };
  enum { EFLG_RX0OVR = 0x40, EFLG_RX1OVR = 0x80, EFLG_TXBO = 0x20, TXB0 = 0 };
  inline static std::map<int, MCP2515 *> chips;
  int cs, resets = 0;
  uint8_t errors = 0, status = 0;
  bool failInit = false;
  bool errInterrupt = false, merrInterrupt = false;
  std::deque<can_frame> rx;
  std::vector<can_frame> sent;
  MCP2515(int pin, int, int *) : cs(pin) { chips[cs] = this; }
  ERROR reset() { assert(mutexHeld); resets++; status = errors = 0; rx.clear(); return failInit ? ERROR_FAIL : ERROR_OK; }
  ERROR setBitrate(CAN_SPEED, CAN_CLOCK) { assert(mutexHeld); return failInit ? ERROR_FAIL : ERROR_OK; }
  ERROR setNormalMode() { assert(mutexHeld); return failInit ? ERROR_FAIL : ERROR_OK; }
  ERROR readMessage(can_frame *f) {
    assert(mutexHeld); if (rx.empty()) return ERROR_NOMSG;
    *f = rx.front(); rx.pop_front(); return ERROR_OK;
  }
  ERROR sendMessage(const can_frame *f) { assert(mutexHeld); sent.push_back(*f); status = 4; return ERROR_OK; }
  ERROR sendMessage(int, const can_frame *f) { return sendMessage(f); }
  uint8_t getErrorFlags() { assert(mutexHeld); return errors; }
  uint8_t errorCountTX() { return errors ? 128 : 0; }
  uint8_t errorCountRX() { return 0; }
  uint8_t getStatus() { assert(mutexHeld); return status; }
  void clearRXnOVRFlags() { errors &= 0x3f; }
  void clearRXnOVR() { clearRXnOVRFlags(); }
  void clearTXInterrupts() { status &= ~8; }
  void clearERRIF() { assert(mutexHeld); errInterrupt = false; }
  void clearMERR() { assert(mutexHeld); merrInterrupt = false; }
};

#define ARDUINO 10800
#include "../board.h"
#include "../vehicle_profile.h"
#include "../summon_state_pure.h"
#include "../board_can.h"

int main() {
  // Exhaust every stable signature, plus an unstable TMR strap. Unknown
  // hardware must not drive any of the conflicting controller pins.
  for (upBits = 0; upBits < 8; ++upBits) for (downBits = 0; downBits < 8; ++downBits) {
    driven.clear(); boardDetect();
    assert(board == (downBits ? BOARD_UNKNOWN : upBits == 0 ? BOARD_TMR : upBits == 7 ? BOARD_T2CAN : BOARD_UNKNOWN));
    assert(driven.size() == (board == BOARD_TMR ? 6u : 0u));
    for (int pin = 15; pin <= 17; ++pin) assert(modes[pin] == INPUT);
  }
  upBits = downBits = 0; glitch = true; reads = 0; driven.clear(); boardDetect();
  assert(board == BOARD_UNKNOWN && driven.empty()); glitch = false;
  boardDetect(); assert(board == BOARD_TMR); assert(boardCanBegin());
  // Every standard model gains all three roles; invalid combinations and
  // Highland's missing turn type still fail closed. Persist and reload.
  for (int profile = 2; profile <= 5; ++profile) {
    assert(vehicleProfileSave(profile, 4, 1)); assert(vehicleProfileLoadFromNvs());
    assert(activeCanAIsBody() && activeCanBIsChassis() && !activeCanAIsParty());
    assert(activeProfileNagSupported() && activeProfileAdvancedEapSupported() && activeProfilePedalMapSupported());
    assert(activeProfileNagGateDependsOnCanB());
    const auto route = summonRoutePure(profile, 4);
    assert(route.valid && route.dasBusMask == SUMMON_BUS_B && route.transportBusMask == SUMMON_BUS_B);
    assert(!vehicleProfileSave(profile, 2, 1));
  }
  assert(!vehicleProfileSave(4, 4, 0)); assert(!vehicleProfileSave(1, 4, 1));
  BoardCanA a;
  assert(a.reset() == MCP2515::ERROR_OK);
  assert(a.setBitrate(CAN_500KBPS, MCP_16MHZ) == MCP2515::ERROR_OK);
  assert(a.setNormalMode() == MCP2515::ERROR_OK);
  assert(levels[18] == LOW && levels[21] == LOW && levels[14] == HIGH);
  auto &body = *MCP2515::chips.at(10), &party = *MCP2515::chips.at(8);
  can_frame frame{0x399, 8, {1}}, received{}; bool isParty;
  body.rx.push_back(frame); frame.data[0] = 2; party.rx.push_back(frame);
  assert(a.readMessage(&received, &isParty) == MCP2515::ERROR_OK && !isParty && received.data[0] == 1);
  assert(a.readMessage(&received, &isParty) == MCP2515::ERROR_OK && isParty && received.data[0] == 2);
  a.sendMessage(&frame, false); a.sendMessage(&frame, true);
  assert(body.sent.size() == 1 && party.sent.size() == 1);
  party.errors = MCP2515::EFLG_TXBO; assert(a.getErrorFlags() & MCP2515::EFLG_TXBO);
  a.reset(); assert(!party.errors && body.resets == 2 && party.resets == 2);
  party.failInit = true; assert(a.setNormalMode() != MCP2515::ERROR_OK);
  assert(levels[18] == HIGH && levels[21] == HIGH); party.failInit = false;
  twai_general_config_t g; twai_timing_config_t t; twai_filter_config_t f;
  assert(twai_driver_install(&g, &t, &f) == ESP_OK && twai_start() == ESP_OK);
  assert(irqAttachments == 1 && modes[5] == INPUT_PULLUP);
  irqHandler(); assert(irqWakes == 1 && isrYields == 1);
  assert(levels[14] == LOW && nativeCalls.empty());
  twai_reconfigure_alerts(UINT32_MAX, nullptr);
  twai_message_t msg{}; msg.identifier = 0x399; msg.data_length_code = 8;
  assert(twai_transmit(&msg, 0) == ESP_OK); boardCanPollOnce();
  assert(txWakes == 1);
  uint32_t alerts; twai_read_alerts(&alerts, 0); assert(!(alerts & TWAI_ALERT_TX_SUCCESS));
  auto &chassis = *MCP2515::chips.at(9);
  chassis.status = 8; boardCanPollOnce(); twai_read_alerts(&alerts, 0);
  assert((alerts & (TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_IDLE)) == (TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_IDLE));
  for (int i = 0; i < 3; ++i) { msg.data[0] = i; assert(twai_transmit(&msg, 0) == ESP_OK); }
  for (int i = 0; i < 3; ++i) {
    boardCanPollOnce(); assert(chassis.sent.back().data[0] == i);
    boardCanPollOnce(); assert(chassis.sent.size() == size_t(i + 2)); // Pending TX owns the mailbox.
    chassis.status = 8;
  }
  boardCanPollOnce();
  chassis.rx.push_back(frame); chassis.errors = MCP2515::EFLG_RX0OVR;
  chassis.errInterrupt = chassis.merrInterrupt = true;
  boardCanPollOnce(); assert(twai_receive(&msg, 0) == ESP_OK && msg.identifier == 0x399 && msg.data[0] == 2);
  assert(!chassis.errInterrupt && !chassis.merrInterrupt);
  twai_status_info_t status{}; twai_get_status_info(&status); assert(status.rx_overrun_count == 1);
  for (int i = 0; i < 16; ++i) assert(twai_transmit(&msg, 0) == ESP_OK);
  assert(twai_transmit(&msg, 0) == ESP_ERR_TIMEOUT);
  twai_clear_transmit_queue(); twai_get_status_info(&status); assert(!status.msgs_to_tx);
  assert(twai_transmit(&msg, 0) == ESP_OK); boardCanPollOnce(); ticks += 300; boardCanPollOnce();
  twai_get_status_info(&status); assert(status.state == TWAI_STATE_BUS_OFF && levels[14] == HIGH);
  assert(twai_transmit(&msg, 0) == ESP_ERR_INVALID_STATE);
  assert(twai_initiate_recovery() == ESP_OK && twai_start() == ESP_OK);
  assert(!uxQueueMessagesWaiting(boardTx) && !boardTxPending && levels[14] == LOW);
  chassis.errors = MCP2515::EFLG_TXBO; boardCanPollOnce();
  twai_get_status_info(&status); assert(status.state == TWAI_STATE_BUS_OFF);
  twai_driver_uninstall(); assert(!boardInstalled && levels[14] == HIGH);
  // YL only enables Party + VH. The unused Body transceiver stays in standby.
  assert(vehicleProfileSave(1, 1, 1)); assert(vehicleProfileLoadFromNvs());
  a.reset(); a.setBitrate(CAN_500KBPS, MCP_16MHZ); a.setNormalMode();
  assert(levels[18] == HIGH && levels[21] == LOW && !boardTripleCan());
  // All native TWAI entrypoints delegate on T-2CAN; topology and NVS guards
  // prevent a TMR three-bus profile from running on that board.
  board = BOARD_T2CAN;
  const unsigned attachedBeforeNative = irqAttachments, wakesBeforeNative = txWakes;
  assert(!vehicleProfileSave(3, 4, 1));
  assert(vehicleProfileSave(3, 2, 1) && vehicleProfileLoadFromNvs());
  assert(!activeProfileNagSupported() && activeProfileAdvancedEapSupported());
  assert(vehicleProfileSave(3, 3, 0) && vehicleProfileLoadFromNvs());
  assert(activeProfileNagSupported() && !activeProfileAdvancedEapSupported());
  twai_driver_install(&g, &t, &f); twai_start(); twai_stop(); twai_driver_uninstall();
  twai_initiate_recovery(); twai_transmit(&msg, 0); twai_receive(&msg, 0);
  twai_get_status_info(&status); twai_clear_transmit_queue();
  twai_reconfigure_alerts(0, nullptr); twai_read_alerts(&alerts, 0);
  assert(nativeCalls.size() == 11);
  assert(irqAttachments == attachedBeforeNative && txWakes == wakesBeforeNative);
  delete boardRx; delete boardTx;
  std::cout << "PASS: straps, profiles/NVS, three-bus routing, FIFO/alerts, overflow, recovery, T-2CAN delegation\n";
}
