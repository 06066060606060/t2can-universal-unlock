// TMR Unified - three MCP2515 buses for all vehicle models
//
// CAN A / J2 BODY    -> 0x249 RX/TX + 0x3C2 stalkless RX/TX
// CAN B / J3 CHASSIS -> Advanced EAP, Summon and TLSSC
// CAN C / J4 PARTY   -> optional Nag Killer module

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <Update.h>
#include "index_html.h"

#define FW_VERSION "TMR-LITE-v1.0.0"

// T-2CAN board specific
#include "pin_config.h"
#include <mcp2515.h>
#include <SPI.h>

// J4 PARTY transport.  This is a direct MCP2515 implementation; there is no
// ESP32 native CAN driver, queue or controller anywhere in this firmware.
struct PartyFrame {
  uint32_t identifier = 0;
  uint8_t data_length_code = 0;
  // Compatibility field: MCP2515 itself uses identifier and payload only.
  uint32_t flags = 0;
  uint8_t data[8] = {};
};

enum PartyState : uint8_t {
  PARTY_STATE_STOPPED = 0,
  PARTY_STATE_RUNNING = 1,
  PARTY_STATE_BUS_OFF = 2
};

struct PartyStatus {
  PartyState state = PARTY_STATE_STOPPED;
  uint32_t msgs_to_tx = 0;
  uint32_t msgs_to_rx = 0;
};

static esp_err_t partyGetStatus(PartyStatus *out);
static esp_err_t partyReceive(PartyFrame *out, TickType_t waitTicks);
static esp_err_t partyTransmit(const PartyFrame *in, TickType_t waitTicks);
static esp_err_t chassisGetStatus(PartyStatus *out);
static esp_err_t chassisReceive(PartyFrame *out, TickType_t waitTicks);
static esp_err_t chassisTransmit(const PartyFrame *in, TickType_t waitTicks);
static inline esp_err_t partyClearTransmitQueue() { return ESP_OK; }

// The known-good Nag Killer logic, compiled directly into this sketch and
// bound exclusively to J4 PARTY.
class NagParty {
 public:
  void begin() {
    prefs_.begin("nag_party", false);
    // Match the standalone Nag Killer: a fresh configuration starts enabled.
    // An explicit dashboard choice remains persistent.
    enabled_ = prefs_.isKey("en") ? prefs_.getBool("en", true) : true;
    mode_ = prefs_.getUChar("mode", 0);
    targetId_ = prefs_.getUShort("id", 0x370);
    hoRatePct_ = prefs_.getUChar("ho", 100);
    burstMs_ = prefs_.getUShort("bms", 1000);
    pauseMs_ = prefs_.getUShort("pms", 1500);
    prefs_.end();
    clamp();
    startedMs_ = millis();
    warmupFrames_ = 0;
  }

  // J4 alone owns the Nag path. After a J4 controller reset, reproduce the
  // standalone sketch's short CAN observation period before resuming echoes.
  void partyTransportReset() { startedMs_ = millis(); warmupFrames_ = 0; }

  void process(const PartyFrame &src) {
    canFrames_++;
    warmupFrames_++;
    lastFrameMs_ = millis();
    if (src.identifier != targetId_ || src.data_length_code < 8) return;
    rxFrames_++;
    const uint8_t handsOn = (src.data[4] >> 6) & 0x03;
    const uint16_t torque = ((src.data[2] & 0x0F) << 8) | src.data[3];
    realHo_ = handsOn;
    realTorque_ = torque * 0.01f - 20.5f;
    // Preserve the original, working eligibility rule: only stock HandsOn
    // states 0/1 may be echoed. Frames already produced by this injector are
    // recognised by their torque signature and never echoed again.
    // J4 needs a brief observation period after boot/reset, but the original
    // 15 s / 1000-frame delay was unnecessarily long on this three-MCP board.
    const bool warmupReady = (uint32_t)(millis() - startedMs_) >= 3000 &&
                             warmupFrames_ >= 100;
    if (!enabled_ || !warmupReady || handsOn > 1 || isOurFrame(handsOn, torque)) return;

    uint8_t b2, b3;
    bool setHandsOn;
    if (!decide(b2, b3, setHandsOn)) return;
    PartyFrame out = src;
    out.data[2] = (out.data[2] & 0xF0) | (b2 & 0x0F);
    out.data[3] = b3;
    // Match the known-good injector: a positive sample raises HandsOn bit 6;
    // a negative sample leaves the stock field untouched rather than clearing
    // it. This preserves the native HandsOn state encoding.
    out.data[4] = setHandsOn ? (uint8_t)(src.data[4] | 0x40) : src.data[4];
    out.data[6] = (out.data[6] & 0xF0) | ((out.data[6] + 1) & 0x0F);
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 7; ++i) sum += out.data[i];
    out.data[7] = (uint8_t)((sum + 0x73) & 0xFF);
    const uint32_t started = micros();
    if (partyTransmit(&out, pdMS_TO_TICKS(2)) == ESP_OK) {
      txOk_++; echoCount_++; lastInjectedHo_ = setHandsOn ? 1 : 0;
      lastInjectedNm_ = ((((uint16_t)(b2 & 0x0F) << 8) | b3) * 0.01f) - 20.5f;
    } else txFail_++;
    latencyUs_ = micros() - started;
  }

  String configJson() const {
    String s = "{\"enabled\":" + String(enabled_ ? "true" : "false");
    s += ",\"mode\":" + String(mode_) + ",\"targetId\":" + String(targetId_);
    s += ",\"hoRatePct\":" + String(hoRatePct_) + ",\"burstMs\":" + String(burstMs_);
    return s + ",\"pauseMs\":" + String(pauseMs_) + "}";
  }
  String statsJson() const {
    const uint32_t now = millis();
    String s = "{\"enabled\":" + String(enabled_ ? "true" : "false");
    s += ",\"mode\":" + String(mode_) + ",\"rx\":" + String(rxFrames_);
    s += ",\"canFrames\":" + String(canFrames_) + ",\"echo\":" + String(echoCount_);
    s += ",\"txOk\":" + String(txOk_) + ",\"txFail\":" + String(txFail_);
    s += ",\"latUs\":" + String(latencyUs_) + ",\"ho\":" + String(realHo_);
    s += ",\"torque\":" + String(realTorque_, 2) + ",\"injHo\":" + String(lastInjectedHo_);
    s += ",\"injNm\":" + String(lastInjectedNm_, 2);
    s += ",\"warmupFrames\":" + String(warmupFrames_);
    s += ",\"warmupReady\":" + String(((uint32_t)(now - startedMs_) >= 3000 && warmupFrames_ >= 100) ? "true" : "false");
    return s + ",\"canAgeMs\":" + String(lastFrameMs_ ? now - lastFrameMs_ : 999999UL) + "}";
  }
  void setMode(uint8_t mode) { mode_ = mode > 2 ? 0 : mode; save(); }
  void update(bool enabled, uint16_t target, uint8_t rate, uint16_t burst, uint16_t pause) {
    enabled_ = enabled; targetId_ = target; hoRatePct_ = rate; burstMs_ = burst; pauseMs_ = pause; clamp(); save();
  }
  void resetStats() { rxFrames_ = canFrames_ = echoCount_ = txOk_ = txFail_ = latencyUs_ = 0; }

 private:
  Preferences prefs_;
  bool enabled_ = true;
  uint8_t mode_ = 0, hoRatePct_ = 100, cycleIndex_ = 0, previousB3_ = 0xA7;
  uint16_t hoSequence_ = 0, targetId_ = 0x370, burstMs_ = 1000, pauseMs_ = 1500;
  uint32_t rxFrames_ = 0, canFrames_ = 0, warmupFrames_ = 0, echoCount_ = 0, txOk_ = 0, txFail_ = 0, latencyUs_ = 0, lastFrameMs_ = 0, lastChangeMs_ = 0, startedMs_ = 0;
  uint8_t realHo_ = 0, lastInjectedHo_ = 0;
  float realTorque_ = 0, lastInjectedNm_ = 0;
  static const uint8_t torqueB2[4];
  static const uint8_t torqueB3[4];
  void clamp() { hoRatePct_ = hoRatePct_ > 100 ? 100 : hoRatePct_; burstMs_ = constrain(burstMs_, 50, 10000); pauseMs_ = pauseMs_ > 10000 ? 10000 : pauseMs_; }
  void save() { prefs_.begin("nag_party", false); prefs_.putBool("en", enabled_); prefs_.putUChar("mode", mode_); prefs_.putUShort("id", targetId_); prefs_.putUChar("ho", hoRatePct_); prefs_.putUShort("bms", burstMs_); prefs_.putUShort("pms", pauseMs_); prefs_.end(); }
  bool isOurFrame(uint8_t handsOn, uint16_t torque) const {
    if (handsOn != 1) return false;
    if (mode_ == 2) return torque == (uint16_t)(0x0800 | previousB3_);
    if (mode_ == 0) return torque == 0x08B6;
    for (uint8_t i = 0; i < 4; ++i) if (torque == (uint16_t)(((uint16_t)torqueB2[i] << 8) | torqueB3[i])) return true;
    return false;
  }
  bool decide(uint8_t &b2, uint8_t &b3, bool &setHandsOn) {
    const uint32_t now = millis();
    // Standalone Mode A: fixed +1.80 Nm torque. It must not cycle through
    // Mode B's four torque values.
    if (mode_ == 0) { b2 = 0x08; b3 = 0xB6; setHandsOn = ((hoSequence_ * 100u) / 65536u) < hoRatePct_; hoSequence_ = (uint16_t)(hoSequence_ * 1103u + 12345u); return true; }
    const uint32_t total = (uint32_t)burstMs_ + pauseMs_;
    if (total && ((uint32_t)(now - startedMs_) % total) >= burstMs_) return false;
    if (mode_ == 1) {
      if (now - lastChangeMs_ >= 200) { cycleIndex_++; lastChangeMs_ = now; }
      const uint8_t i = cycleIndex_ & 3; b2 = torqueB2[i]; b3 = torqueB3[i]; setHandsOn = true; return true;
    }
    const int low = max(0x98, (int)previousB3_ - 15), high = min(0xB6, (int)previousB3_ + 15);
    if (now - lastChangeMs_ >= 200) { previousB3_ = random(low, high + 1); lastChangeMs_ = now; }
    b2 = 0x08; b3 = previousB3_; setHandsOn = true; return true;
  }
};

const uint8_t NagParty::torqueB2[4] = {0x08, 0x08, 0x07, 0x07};
const uint8_t NagParty::torqueB3[4] = {0xB6, 0x98, 0x6C, 0x4E};

static unsigned long bootTime = 0;
static unsigned long canInitTime = 0;
static volatile bool mcpReady = false;
static volatile uint32_t canAnyFrames = 0;
static volatile unsigned long lastCanFrameMs = 0;
static volatile uint32_t canBeat = 0;
static volatile uint32_t canRxBeat = 0;
static volatile uint32_t webBeat = 0;
static volatile uint32_t runtimeStatsResetCount = 0;
static volatile uint32_t runtimeStatsLastResetMs = 0;
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
static Preferences prefs;

// ═══════════════════════════════════════════════════════════════
// BOOT / FIRST-CAN TIMING CAPTURE (rev.15)
//
// Passive diagnostics only. These timestamps never participate in feature
// gating, CAN recovery decisions, or TX/injection. Capture starts
// automatically on every ESP32 boot and can be exported as CSV from:
//   /api/system/boot-capture.csv
// All timestamps are milliseconds from setup() entry (bootTime).
// ═══════════════════════════════════════════════════════════════

static constexpr uint32_t BOOT_CAPTURE_UNSET = 0xFFFFFFFFUL;
static portMUX_TYPE bootCaptureMux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t bootCapCanInitDoneMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapCanTasksStartedMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapWifiReadyMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstCanAMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstCanBMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirst399Ms = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstParty24AMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstVh249Ms = BOOT_CAPTURE_UNSET;

struct BootHardReinitEvent {
  uint32_t startMs;
  uint32_t endMs;
  uint8_t reason;
  int8_t success; // -1=in progress, 0=failed, 1=success
};

static constexpr uint8_t BOOT_CAPTURE_HARD_MAX = 8;
static BootHardReinitEvent bootCapHard[BOOT_CAPTURE_HARD_MAX] = {};
static volatile uint8_t bootCapHardCount = 0;
static volatile uint32_t bootCapHardDropped = 0;

static inline uint32_t bootCaptureNowMs() {
  return (uint32_t)(millis() - bootTime);
}

static void bootCaptureMarkOnce(volatile uint32_t *slot) {
  // Fast path after the first event: no critical section on normal CAN traffic.
  if (*slot != BOOT_CAPTURE_UNSET) return;
  const uint32_t t = bootCaptureNowMs();
  portENTER_CRITICAL(&bootCaptureMux);
  if (*slot == BOOT_CAPTURE_UNSET) *slot = t;
  portEXIT_CRITICAL(&bootCaptureMux);
}

static void bootCaptureObservePartyFrame(uint16_t id, uint8_t dlc, const uint8_t *data) {
  bootCaptureMarkOnce(&bootCapFirstCanAMs);

  if (id == 0x399 && dlc >= 1)
    bootCaptureMarkOnce(&bootCapFirst399Ms);

  if (id == 0x24A && dlc >= 8)
    bootCaptureMarkOnce(&bootCapFirstParty24AMs);

  if (id == 0x249 && dlc >= 4)
    bootCaptureMarkOnce(&bootCapFirstVh249Ms);

}

static void bootCaptureObserveVhFrame(uint32_t id, uint8_t dlc) {
  bootCaptureMarkOnce(&bootCapFirstCanBMs);
}

static int8_t bootCaptureHardStart(uint8_t reason) {
  const uint32_t t = bootCaptureNowMs();
  int8_t idx = -1;
  portENTER_CRITICAL(&bootCaptureMux);
  if (bootCapHardCount < BOOT_CAPTURE_HARD_MAX) {
    idx = (int8_t)bootCapHardCount++;
    bootCapHard[idx].startMs = t;
    bootCapHard[idx].endMs = BOOT_CAPTURE_UNSET;
    bootCapHard[idx].reason = reason;
    bootCapHard[idx].success = -1;
  } else {
    bootCapHardDropped++;
  }
  portEXIT_CRITICAL(&bootCaptureMux);
  return idx;
}

static void bootCaptureHardFinish(int8_t idx, bool success) {
  if (idx < 0 || idx >= (int8_t)BOOT_CAPTURE_HARD_MAX) return;
  const uint32_t t = bootCaptureNowMs();
  portENTER_CRITICAL(&bootCaptureMux);
  bootCapHard[(uint8_t)idx].endMs = t;
  bootCapHard[(uint8_t)idx].success = success ? 1 : 0;
  portEXIT_CRITICAL(&bootCaptureMux);
}

static const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL_RESET";
    case ESP_RST_SW:        return "SOFTWARE_RESET";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

// ═══════════════════════════════════════════════════════════════
// MCP2515 GLOBALS
// ═══════════════════════════════════════════════════════════════

static constexpr CAN_CLOCK MCP_CLOCK = MCP_16MHZ;

static constexpr uint32_t MCP_SPI_HZ = 10000000;

static constexpr uint8_t MCP_RX_BUDGET = 32;

static MCP2515 Can_A(MCP2515_BODY_CS, MCP_SPI_HZ, &SPI);
static MCP2515 Can_Chassis(MCP2515_CHASSIS_CS, MCP_SPI_HZ, &SPI);
MCP2515 Can_Party(MCP2515_PARTY_CS, MCP_SPI_HZ, &SPI);
static volatile bool partyReady = false;
static volatile bool chassisMcpReady = false;
static volatile uint32_t chassisRxCount = 0;
static volatile uint32_t partyRxCount = 0;
static volatile uint32_t lastCanChassisFrameMs = 0;
static volatile uint32_t lastCanPartyFrameMs = 0;
static NagParty nagParty;
static volatile uint8_t  mcpState = 0;      // 0=OK, 1=WARN, 2=BUS-OFF
static volatile uint32_t mcpTxOk = 0;
static volatile uint32_t mcpTxFail = 0;
static volatile uint8_t  mcpTxFailConsecutive = 0;
static volatile uint32_t mcpRxCount = 0;
static unsigned long lastMcpStatusMs = 0;
static unsigned long lastMcpRecoverMs = 0;
static esp_err_t partyGetStatus(PartyStatus *out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  out->state = partyReady ? PARTY_STATE_RUNNING : PARTY_STATE_STOPPED;
  out->msgs_to_tx = 0; // MCP2515 has direct hardware TX buffers only.
  out->msgs_to_rx = 0;
  return ESP_OK;
}

static esp_err_t partyReceive(PartyFrame *out, TickType_t waitTicks) {
  if (!out || !partyReady) return ESP_ERR_INVALID_STATE;
  struct can_frame frame = {};
  if (Can_Party.readMessage(&frame) != MCP2515::ERROR_OK) {
    if (waitTicks) vTaskDelay(waitTicks);
    if (Can_Party.readMessage(&frame) != MCP2515::ERROR_OK) return ESP_ERR_TIMEOUT;
  }
  out->identifier = frame.can_id & 0x7FFU;
  out->data_length_code = frame.can_dlc;
  memcpy(out->data, frame.data, out->data_length_code);
  return ESP_OK;
}

static esp_err_t partyTransmit(const PartyFrame *in, TickType_t) {
  if (!in || !partyReady || in->data_length_code > 8) return ESP_ERR_INVALID_STATE;
  struct can_frame frame = {};
  frame.can_id = in->identifier;
  frame.can_dlc = in->data_length_code;
  memcpy(frame.data, in->data, frame.can_dlc);
  return Can_Party.sendMessage(&frame) == MCP2515::ERROR_OK ? ESP_OK : ESP_ERR_TIMEOUT;
}

// Advanced EAP / Summon / TLSSC run on J3 CHASSIS.  J4 remains dedicated to
// the Nag Killer stream, so never route feature injections through Can_Party.
static esp_err_t chassisGetStatus(PartyStatus *out) {
  if (!out) return ESP_ERR_INVALID_ARG;
  out->state = chassisMcpReady ? PARTY_STATE_RUNNING : PARTY_STATE_STOPPED;
  out->msgs_to_tx = 0;
  out->msgs_to_rx = 0;
  return ESP_OK;
}

static esp_err_t chassisReceive(PartyFrame *out, TickType_t waitTicks) {
  if (!out || !chassisMcpReady) return ESP_ERR_INVALID_STATE;
  struct can_frame frame = {};
  if (Can_Chassis.readMessage(&frame) != MCP2515::ERROR_OK) {
    if (waitTicks) vTaskDelay(waitTicks);
    if (Can_Chassis.readMessage(&frame) != MCP2515::ERROR_OK) return ESP_ERR_TIMEOUT;
  }
  out->identifier = frame.can_id & 0x7FFU;
  out->data_length_code = frame.can_dlc;
  memcpy(out->data, frame.data, out->data_length_code);
  return ESP_OK;
}

static esp_err_t chassisTransmit(const PartyFrame *in, TickType_t) {
  if (!in || !chassisMcpReady || in->data_length_code > 8) return ESP_ERR_INVALID_STATE;
  struct can_frame frame = {};
  frame.can_id = in->identifier;
  frame.can_dlc = in->data_length_code;
  memcpy(frame.data, in->data, frame.can_dlc);
  return Can_Chassis.sendMessage(&frame) == MCP2515::ERROR_OK ? ESP_OK : ESP_ERR_TIMEOUT;
}


// ═══════════════════════════════════════════════════════════════
// CAN RECOVERY SUPERVISOR (ported from V2.14 recovery architecture)
//
// IMPORTANT: this block does NOT participate in Summon / TLSSC / Advanced EAP /
// Auto Blinker gating. Original V2.3 feature logic remains authoritative.
// It only monitors CAN controller/task liveness and recreates the CAN
// subsystem when acquisition or wake recovery fails.
// ═══════════════════════════════════════════════════════════════

enum CanSupervisorCommand : uint8_t {
  CAN_SUP_NONE = 0,
  CAN_SUP_HARD_ACQUIRE = 1,
  CAN_SUP_HARD_STALE = 2,
  CAN_SUP_HARD_MANUAL = 3
};

static portMUX_TYPE canRecoveryMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t canSupervisorCommand = CAN_SUP_NONE;
static volatile bool canSubsystemBusy = false;
static volatile bool canTasksStopping = false;
static volatile bool canTaskMcpQuiesced = false;
static volatile bool canTaskPartyQuiesced = false;
static volatile bool canTaskChassisQuiesced = false;
static TaskHandle_t canTaskMcpHandle = nullptr;
static TaskHandle_t canTaskPartyHandle = nullptr;
static TaskHandle_t canTaskChassisHandle = nullptr;
static TaskHandle_t canSupervisorHandle = nullptr;

static volatile uint32_t canTaskMcpHeartbeatMs = 0;
static volatile uint32_t canTaskPartyHeartbeatMs = 0;
static volatile uint32_t canTaskChassisHeartbeatMs = 0;
static volatile uint32_t lastCanAFrameMs = 0;
static volatile uint32_t lastCanBFrameMs = 0;
static volatile uint32_t canHardReinitCount = 0;
static volatile uint32_t canHardReinitFailCount = 0;
static volatile uint8_t  canLastHardReinitReason = CAN_SUP_NONE;
static volatile uint32_t canRecoverySleepCount = 0;
static volatile uint32_t canRecoveryWakeCount = 0;

static bool mcpSpiStarted = false;
static bool recoveryEverBothActive = false;
static bool recoverySleeping = false;
static uint32_t recoveryWakeAcquireStartMs = 0;
static uint32_t recoveryOneBusStaleStartMs = 0;
static uint32_t recoveryLastHardRequestMs = 0;
static uint32_t recoveryLastBothActiveMs = 0;
static uint8_t recoveryColdRetryCount = 0;
static bool recoveryColdRetriesExhausted = false;

// A Tesla bus can legitimately have short quiet windows. Keep status display
// and recovery separate: a real MCP2515 bus-off is still recovered immediately
// by the per-controller checks, while traffic silence now needs sustained
// evidence before it can restart all three controllers.
static constexpr uint32_t CAN_TRAFFIC_ONLINE_MS = 8000;
static constexpr uint32_t RECOVERY_BUS_FRESH_MS = 6000;
static constexpr uint32_t RECOVERY_SLEEP_QUIET_MS = 15000;
static constexpr uint32_t RECOVERY_WAKE_ACQUIRE_MS = 15000;
static constexpr uint32_t RECOVERY_ONE_BUS_STALE_MS = 20000;
// rev.14: fast cold acquisition; CAN RX starts as soon as controllers are ready.
// Keep task-heartbeat startup grace separate so fast acquisition does not make
// the task watchdog unnecessarily aggressive.
static constexpr uint32_t RECOVERY_COLD_FIRST_ACQUIRE_MS = 8000;
static constexpr uint32_t RECOVERY_TASK_START_GRACE_MS = 8000;
static constexpr uint32_t RECOVERY_HARD_COOLDOWN_MS = 30000;
static constexpr uint32_t RECOVERY_COLD_RETRY_INTERVAL_MS = 30000;
static constexpr uint8_t  RECOVERY_COLD_MAX_RETRIES = 3;
static constexpr uint32_t RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS = 3000;
static constexpr uint32_t RECOVERY_TASK_STOP_SETTLE_MS = 50;
static constexpr uint32_t RECOVERY_PARTY_WAIT_MS = 1800;

static void requestCanSubsystemRestart(uint8_t reason);

// ═══════════════════════════════════════════════════════════════
// SUMMON UNLOCK (CAN B - J3 CHASSIS)
// ═══════════════════════════════════════════════════════════════

static inline uint8_t readMuxID(const uint8_t *data) {
    return data[0] & 0x07;
}
static inline bool getBit(const uint8_t *data, int bit) {
    return (data[bit / 8] >> (bit % 8)) & 0x01;
}
static inline void setBit(uint8_t *data, int bit, bool val) {
    uint8_t mask = (uint8_t)(1U << (bit % 8));
    if (val) data[bit / 8] |=  mask;
    else     data[bit / 8] &= ~mask;
}
static inline uint8_t readDIGear(const uint8_t *data) {
    return (data[2] >> 5) & 0x07;
}
static inline uint8_t readVehicleGear(const uint8_t *data) {
    return (data[2] >> 5) & 0x07;
}
static inline int gearState(uint8_t gear) {
    if (gear == 1)             return  1;
    if (gear == 2 || gear == 3 || gear == 4) return 0;
    return -1;
}
static inline uint8_t readDASStatus(const uint8_t *data) {
    return data[0] & 0x07;
}
// all models NOA discriminator. Keep the legacy 3-bit AP decoder above
// unchanged for NAG and the existing AP gate; Auto Blinker alone uses the
// full low nibble so ACTIVE_NAV (5) can be gated independently.
static inline uint8_t readDASState4(const uint8_t *data) {
    return data[0] & 0x0F;
}

static volatile bool forceMode = false;
static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool summonEnabled = true;
static volatile bool tlsscEnabled  = false;   // "Enable TLSSC" - off by default
static volatile bool tlsscRestoreEnabled = false; // 0x331 DAS_autopilotConfig restore
// Latest real 0x3FD mux1 frame, used as the template for AP/ALC snooze.
static volatile bool seen3fdMux1 = false;
static uint8_t realRaw3fdMux1[8] = {0};
static volatile uint32_t snoozeTxOk = 0;
static volatile uint32_t snoozeTxFail = 0;
static volatile uint32_t summonPeriodicTxOk = 0;
static volatile uint32_t summonPeriodicTxFail = 0;
static volatile uint32_t lastSummonPeriodicTxMs = 0;
#define SUMMON_PERIODIC_TX_MS 500																							  
static volatile bool gateAPActive  = false;
static volatile bool gateNOAActive = false;   // raw DAS_autopilotState == ACTIVE_NAV (5)
static volatile uint8_t dasAutopilotState4 = 0xFF; // low nibble of Party-CAN 0x399 byte0
static volatile uint32_t lastDASStatusMillis = 0;  // last valid Party-CAN 0x399 RX
static constexpr uint32_t NOA_STATUS_FRESH_MS = 2000; // dashboard freshness indicator

// Auto Blinker uses the last observed 0x399 DAS state. It is enabled only in
// states 3 (Autosteer), 4 (restricted Autosteer) and 5 (NOA). Once a state has
// been observed, a stale 0x399 must not cancel an active NOA request.
static portMUX_TYPE blinkAMux = portMUX_INITIALIZER_UNLOCKED;

static bool autoBlinkerGateOpen(uint32_t now, uint32_t* ageOut = nullptr) {
  uint8_t dasState;
  uint32_t last;
  portENTER_CRITICAL(&stateMux);
  dasState = dasAutopilotState4;
  last = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  uint32_t age = (last == 0) ? UINT32_MAX : (uint32_t)(now - last);
  if (ageOut) *ageOut = age;
  if (last == 0) return false;

  return dasState == 3 || dasState == 4 || dasState == 5;
}
static volatile bool gateParked    = true;
static volatile bool gateSummoning = false;
static volatile bool sprSeen  = false;
static volatile bool lastAca  = false;
#define PARKED_TIMEOUT_MS  5000
static volatile uint32_t last280Millis = 0;

// Summon TX priority is intentionally separate from the Original feature gate.
// It never writes gateParked/gateSummoning/lastAca/sprSeen/forceMode.
// NORMAL        : driving / no fresh confirmed Park. No priority shedding/flush.
// PARK_STANDBY  : fresh Park confirmed. Reserve queue headroom for a Summon start.
// SUMMON_FULL   : Summon session confirmed. Summon owns J3 CHASSIS TX priority.
enum SummonPriorityState : uint8_t {
  SUMMON_PRIORITY_NORMAL = 0,
  SUMMON_PRIORITY_PARK_STANDBY = 1,
  SUMMON_PRIORITY_FULL = 2
};
static volatile uint8_t summonPriorityState = SUMMON_PRIORITY_NORMAL;
static volatile int8_t priorityGear280State = -1;
static volatile int8_t priorityGear390State = -1;
static volatile uint32_t priorityGear280Ms = 0;
static volatile uint32_t priorityGear390Ms = 0;
static volatile uint32_t summonPriorityStateSinceMs = 0;
static volatile uint32_t summonPriorityTransitions = 0;
static volatile uint32_t summonPriorityFullEnterCount = 0;
static volatile uint32_t summonPriorityFullExitCount = 0;
static volatile uint32_t summonPriorityFullInactiveSinceMs = 0;
static constexpr uint32_t SUMMON_PRIORITY_PARK_FRESH_MS = 3000;
static constexpr uint32_t SUMMON_PRIORITY_FULL_EXIT_GRACE_MS = 1500;

static volatile uint32_t sumRxMux1   = 0;
static volatile uint32_t sumTxOk     = 0;
static volatile uint32_t sumTxFail   = 0;
static volatile uint32_t r79RxMux1   = 0;
static volatile uint32_t r79RxMux0   = 0;
static volatile uint32_t r79TxOk     = 0;
static volatile uint32_t r79TxFail   = 0;
static volatile uint32_t sumRx280    = 0;
static volatile uint32_t sumRx390    = 0;
static volatile uint32_t sumRx921    = 0;
static volatile uint32_t sumRx1016   = 0;
static char gateBlockReason[48] = "boot";
// ═══════════════════════════════════════════════════════════════
// ADVANCED EAP — UNIVERSAL ROUTING
//   CAN A / J2 BODY    : 0x249 or 0x3C2 RX/TX + 0x102 RX (Body CAN)
//   CAN B / J3 CHASSIS: DAS 0x247/0x24A/0x399/0x3F8/0x3FD and Summon traffic
// ═══════════════════════════════════════════════════════════════

#define LEFTSTALK_ID      0x249
#define VCLEFT_SWITCH_ID  0x3C2
#define VISUAL_DEBUG_ID   0x24A
#define DRIVER_ASSIST_ID  0x3F8

#define STALK_IDLE    0
#define STALK_UP_1    2
#define STALK_DOWN_1  6  // stock capture: left soft stalk

#define BLINKA_TX_PERIOD_MS         20
#define BLINKA_PULSE_MS             350
#define BLINKA_TRIGGER_DELAY_DEFAULT_MS     3000
#define BLINKA_AP_ENTRY_DELAY_DEFAULT_MS    3000
#define BLINKA_LANE_CHANGE_DELAY_DEFAULT_MS 20000
#define BLINKA_SOURCE_FRESH_MS       500
#define BLINKA_STALKLESS_PRESS_MS    300
#define BLINKA_STALKLESS_RELEASE_MS  200
#define BLINKA_PHYSICAL_DISPLAY_MS  1500

#define BLINKA_TRANSPORT_NONE        0
#define BLINKA_TRANSPORT_249         1
#define BLINKA_TRANSPORT_3C2         2

// Selected Auto Blinker turn-signal transport. Stalk (0x249) is the default.
static volatile uint8_t blinkATransportMode = BLINKA_TRANSPORT_249;

#define VCLEFT_SWITCH_MUX1           1
#define VCLEFT_SWITCH_SNA            0
#define VCLEFT_SWITCH_OFF            1
#define VCLEFT_SWITCH_ON             2

// Real SCCM diagnostics and rolling-counter alignment.
static volatile uint32_t rx249 = 0;
static volatile uint8_t realCounter = 0;
static volatile uint8_t realTurn = 0;
static volatile uint8_t realCksum = 0;
static volatile bool cksumSelfTest = true;
static volatile bool seen249 = false;
static volatile uint32_t last249Ms = 0;
static volatile uint8_t blinkACounter = 0;
static volatile uint8_t realDlc = 0;
static uint8_t realRaw249[8] = {0};


// Auto blinker state.
static volatile bool blinkAEnabled = true;

static volatile uint8_t activeTurn = STALK_IDLE;
static volatile uint8_t lastReqDir = 0;
// rev.06: restore the v1.0b-style separation between delayed trigger timing
// and the active SCCM one-shot pulse. 0x24A state changes may cancel a
// pending trigger, but must not truncate an already-started 350 ms pulse.
static volatile uint8_t oneShotTurn = STALK_IDLE;
static volatile uint32_t oneShotUntil = 0;
static volatile uint32_t oneShotReleaseAt = 0;
static volatile uint8_t blinkATransport = BLINKA_TRANSPORT_NONE;
static volatile uint8_t autoPendingDir = 0;
static volatile uint32_t blinkATriggerDelayMs = BLINKA_TRIGGER_DELAY_DEFAULT_MS;
static volatile uint32_t blinkAApEntryDelayMs = BLINKA_AP_ENTRY_DELAY_DEFAULT_MS;
static volatile uint32_t blinkALaneChangeDelayMs = BLINKA_LANE_CHANGE_DELAY_DEFAULT_MS;
static volatile uint32_t autoApReadyAt = 0;
static volatile uint32_t autoLaneChangeReadyAt = 0;
static volatile bool autoGateWasOpen = false;
static volatile uint32_t autoFireAt = 0;
static volatile bool autoArmed = false;
static volatile uint32_t blkATxOk = 0;
static volatile uint32_t blkATxFail = 0;

// Entering a permitted AP/NOA state starts the first configurable delay.
// Completing a detected lane change starts the independent re-arm delay.
static void updateAutoBlinkerGate(uint32_t now) {
  const bool gateOpen = autoBlinkerGateOpen(now);
  portENTER_CRITICAL(&blinkAMux);
  if (gateOpen && !autoGateWasOpen) {
    autoApReadyAt = now + blinkAApEntryDelayMs;
  } else if (!gateOpen) {
    autoApReadyAt = 0;
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
    lastReqDir = 0;
  }
  autoGateWasOpen = gateOpen;
  portEXIT_CRITICAL(&blinkAMux);
}

static bool autoBlinkerReady(uint32_t now) {
  if (!autoBlinkerGateOpen(now)) return false;
  uint32_t apReadyAt, laneReadyAt;
  portENTER_CRITICAL(&blinkAMux);
  apReadyAt = autoApReadyAt;
  laneReadyAt = autoLaneChangeReadyAt;
  portEXIT_CRITICAL(&blinkAMux);
  return (apReadyAt == 0 || (int32_t)(now - apReadyAt) >= 0) &&
         (laneReadyAt == 0 || (int32_t)(now - laneReadyAt) >= 0);
}

static void noteAutoBlinkerLaneChangeComplete(uint32_t now) {
  portENTER_CRITICAL(&blinkAMux);
  autoLaneChangeReadyAt = now + blinkALaneChangeDelayMs;
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  lastReqDir = 0;
  portEXIT_CRITICAL(&blinkAMux);
}

// Highland / stalkless diagnostics and native-cadence injection.
// Verified in a passive capture on a 2024 Shanghai Model 3 Highland:
//   0x3C2 mux1, left button  = bits 30..31 (byte 3 bits 6..7)
//   0x3C2 mux1, right button = bits 44..45 (byte 5 bits 4..5)
//   2 = SWITCH_ON, 1 = SWITCH_OFF, 0 = SWITCH_SNA/idle.
static volatile bool seen3C2 = false;
static volatile uint32_t rx3C2 = 0;
static volatile uint32_t last3C2Mux1Ms = 0;
static volatile uint8_t real3C2LeftButton = VCLEFT_SWITCH_SNA;
static volatile uint8_t real3C2RightButton = VCLEFT_SWITCH_SNA;
static volatile uint8_t stalklessLastPhysicalTurn = STALK_IDLE;
static volatile uint32_t stalklessLastPhysicalPressMs = 0;
static volatile uint32_t stalklessPhysicalPressCount = 0;
static volatile uint32_t stalklessTxOk = 0;
static volatile uint32_t stalklessTxFail = 0;
static volatile uint32_t stalklessManualOverride = 0;

// J3 CHASSIS telemetry used by the auto blinker (0x24A).
static volatile uint8_t visualBehaviorType = 0;
static volatile uint32_t visualDebugRxCount = 0;
static volatile uint32_t visualDebugLastMs = 0;

// 0x3F8 is passive RX only in rev.07.
// These observed stock values are retained for diagnostics; T-2CAN no longer
// modifies or retransmits UI_ulcSpeedConfig / UI_ulcBlindSpotConfig.
static volatile uint8_t uiUlcBlindSpotConfig = 0;
static volatile uint8_t uiUlcSpeedConfig = 0;

// Lane Change / ULC blind-spot configuration.
// 0 = STANDARD, 1 = AGGRESSIVE, 2 = MAD_MAX.
// Injection is active only while DAS state is 3 (AP) or 5 (NOA).
static volatile uint8_t ulcBlindSpotInjectConfig = 0;
static volatile bool laneChangeButtonPressed = false;
static volatile uint32_t laneChangeButtonRx = 0;

// Latest stock DAS_autopilotDebug (0x247) template and lane-change state on
// J3 CHASSIS. DAS_arbiterBehavior is bits 2..4, little-endian.
static volatile bool seen247 = false;
static uint8_t realRaw247[8] = {0};
static volatile uint32_t laneChangeCancelCount = 0;
static volatile uint8_t dasArbiterBehavior = 0;
static volatile bool dasLaneChangeInProgress = false;
static volatile uint32_t dasLaneChangeCompleteCount = 0;

// J3 CHASSIS load-shedding / queue telemetry.
// rev.05: priority policy is state-scoped so normal/AP driving cannot trigger
// Summon queue flushing or aggressive Summon load shedding.
static constexpr uint16_t PARTY_TX_QUEUE_LEN = 16;
static constexpr uint16_t PARTY_STANDBY_NON_SUMMON_QUEUE_LIMIT = 12;
static constexpr uint16_t PARTY_FULL_NON_SUMMON_QUEUE_LIMIT = 6;
static constexpr uint8_t  PARTY_RX_DRAIN_BUDGET = 64;

static volatile uint32_t partyTxQueueNow = 0;
static volatile uint32_t partyTxQueueMax = 0;
static volatile uint32_t partyRxQueueNow = 0;
static volatile uint32_t partyRxQueueMax = 0;
static volatile uint32_t partyNonSummonShed = 0;
static volatile uint32_t partyStandbyShed = 0;
static volatile uint32_t partyFullShed = 0;
static volatile uint32_t partySummonQueueFlush = 0;
static volatile uint32_t partySummonRetryOk = 0;
static volatile uint32_t partySummonRetryFail = 0;
static volatile uint32_t partySummonTxNormal = 0;
static volatile uint32_t partySummonTxStandby = 0;
static volatile uint32_t partySummonTxFull = 0;

static const char* summonPriorityStateName(uint8_t state) {
  switch (state) {
    case SUMMON_PRIORITY_PARK_STANDBY: return "PARK_STANDBY";
    case SUMMON_PRIORITY_FULL:         return "SUMMON_FULL";
    default:                           return "NORMAL";
  }
}

// Keep the browser independent from ESP-IDF enum ordering.
static const char* partyStateName(int state) {
  switch (state) {
    case PARTY_STATE_STOPPED:    return "STOPPED";
    case PARTY_STATE_RUNNING:    return "RUNNING";
    case PARTY_STATE_BUS_OFF:    return "BUS OFF";
    default:                    return "UNKNOWN";
  }
}

// stateMux must already be held when calling this helper.
// The most recently received *fresh* valid gear source wins. This prevents
// Original V2.3's legacy 280-stale => gateParked=true fallback from enabling
// the new priority layer while the vehicle is actually driving.
static bool summonPriorityFreshParkedLocked(uint32_t now) {
  const bool fresh280 = priorityGear280State >= 0 && priorityGear280Ms != 0 &&
                        (uint32_t)(now - priorityGear280Ms) <= SUMMON_PRIORITY_PARK_FRESH_MS;
  const bool fresh390 = priorityGear390State >= 0 && priorityGear390Ms != 0 &&
                        (uint32_t)(now - priorityGear390Ms) <= SUMMON_PRIORITY_PARK_FRESH_MS;
  if (!fresh280 && !fresh390) return false;

  if (fresh280 && (!fresh390 || (int32_t)(priorityGear280Ms - priorityGear390Ms) >= 0))
    return priorityGear280State == 1;
  return priorityGear390State == 1;
}

// stateMux must already be held when calling this helper.
// FULL can only be entered from a fresh confirmed Park context. Once FULL has
// started, it is allowed to remain FULL while the vehicle physically moves
// under Summon. A short exit grace prevents a single ACA/SPR state dropout from
// tearing down Summon priority in the middle of an otherwise active session.
static void recomputeSummonPriorityStateLocked(uint32_t now) {
  const bool freshParked = summonPriorityFreshParkedLocked(now);
  const uint8_t oldState = summonPriorityState;
  uint8_t nextState = oldState;

  if (oldState == SUMMON_PRIORITY_FULL) {
    if (gateSummoning) {
      summonPriorityFullInactiveSinceMs = 0;
    } else {
      if (summonPriorityFullInactiveSinceMs == 0)
        summonPriorityFullInactiveSinceMs = now;
      if ((uint32_t)(now - summonPriorityFullInactiveSinceMs) >= SUMMON_PRIORITY_FULL_EXIT_GRACE_MS)
        nextState = freshParked ? SUMMON_PRIORITY_PARK_STANDBY : SUMMON_PRIORITY_NORMAL;
    }
  } else {
    summonPriorityFullInactiveSinceMs = 0;
    if (gateSummoning && (oldState == SUMMON_PRIORITY_PARK_STANDBY || freshParked))
      nextState = SUMMON_PRIORITY_FULL;
    else
      nextState = freshParked ? SUMMON_PRIORITY_PARK_STANDBY : SUMMON_PRIORITY_NORMAL;
  }

  if (nextState != oldState) {
    summonPriorityState = nextState;
    summonPriorityStateSinceMs = now;
    summonPriorityTransitions++;
    if (nextState == SUMMON_PRIORITY_FULL) {
      summonPriorityFullEnterCount++;
      summonPriorityFullInactiveSinceMs = 0;
    }
    if (oldState == SUMMON_PRIORITY_FULL) {
      summonPriorityFullExitCount++;
      summonPriorityFullInactiveSinceMs = 0;
    }
  }
}

static void refreshSummonPriorityState() {
  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&stateMux);
  recomputeSummonPriorityStateLocked(now);
  portEXIT_CRITICAL(&stateMux);
}

static uint8_t getSummonPriorityState() {
  uint8_t state;
  portENTER_CRITICAL(&stateMux);
  state = summonPriorityState;
  portEXIT_CRITICAL(&stateMux);
  return state;
}

static bool chassisReadStatus(PartyStatus *out = nullptr) {
  PartyStatus st = {};
  if (chassisGetStatus(&st) != ESP_OK) return false;
  partyTxQueueNow = st.msgs_to_tx;
  partyRxQueueNow = st.msgs_to_rx;
  if (st.msgs_to_tx > partyTxQueueMax) partyTxQueueMax = st.msgs_to_tx;
  if (st.msgs_to_rx > partyRxQueueMax) partyRxQueueMax = st.msgs_to_rx;
  if (out) *out = st;
  return true;
}

// Lower-priority features never block J3 CHASSIS RX. Queue reservation is scoped:
// - NORMAL: no Summon-specific shedding.
// - PARK_STANDBY: mild reservation (4 slots) for a clean Summon start.
// - SUMMON_FULL: aggressive reservation for continuous Summon injection.
static bool chassisNonSummonAdmissionOpen() {
  const uint8_t priorityState = getSummonPriorityState();

  // During normal driving there is no Summon-specific admission policy at all.
  // The following chassisTransmit(..., 0) remains non-blocking and is allowed to
  // succeed/fail directly without an extra status query on every injected frame.
  if (priorityState == SUMMON_PRIORITY_NORMAL) return true;

  PartyStatus st = {};
  if (!chassisReadStatus(&st)) return false;

  const uint16_t limit = (priorityState == SUMMON_PRIORITY_FULL)
                       ? PARTY_FULL_NON_SUMMON_QUEUE_LIMIT
                       : PARTY_STANDBY_NON_SUMMON_QUEUE_LIMIT;

  if (st.msgs_to_tx >= limit) {
    partyNonSummonShed++;
    if (priorityState == SUMMON_PRIORITY_PARK_STANDBY) partyStandbyShed++;
    if (priorityState == SUMMON_PRIORITY_FULL) partyFullShed++;
    return false;
  }
  return true;
}

// Summon TX transport is state-scoped and non-blocking on the J3 CHASSIS RX task.
// NORMAL: no destructive priority behavior.
// PARK_STANDBY: queue headroom is reserved by non-Summon admission control.
// SUMMON_FULL: stale pending T-2CAN TX may be flushed to protect the newest
//              Summon mux1 injection. Queue clear is NEVER used outside FULL.
static esp_err_t chassisTransmitSummonPriority(const PartyFrame *msg) {
  const uint8_t priorityState = getSummonPriorityState();

  if (priorityState == SUMMON_PRIORITY_NORMAL) {
    const esp_err_t err = chassisTransmit(msg, 0);
    if (err == ESP_OK) partySummonTxNormal++;
    return err;
  }

  if (priorityState == SUMMON_PRIORITY_PARK_STANDBY) {
    const esp_err_t err = chassisTransmit(msg, 0);
    if (err == ESP_OK) partySummonTxStandby++;
    return err;
  }

  // SUMMON_FULL only: keep stale pending injections from delaying the newest
  // unlock frame. The currently transmitting hardware frame is not cleared.
  PartyStatus st = {};
  if (chassisReadStatus(&st) && st.msgs_to_tx >= (PARTY_TX_QUEUE_LEN - 2)) {
    if (partyClearTransmitQueue() == ESP_OK) partySummonQueueFlush++;
  }

  esp_err_t err = chassisTransmit(msg, 0);
  if (err == ESP_OK) {
    partySummonTxFull++;
    return ESP_OK;
  }

  // Only queue saturation gets a destructive retry. Driver/bus state errors
  // are left to the existing recovery supervisor.
  if (err != ESP_ERR_TIMEOUT) {
    partySummonRetryFail++;
    return err;
  }

  if (partyClearTransmitQueue() == ESP_OK) partySummonQueueFlush++;
  err = chassisTransmit(msg, 0);
  if (err == ESP_OK) {
    partySummonRetryOk++;
    partySummonTxFull++;
  } else {
    partySummonRetryFail++;
  }
  return err;
}

static inline uint32_t readBitsLE(const uint8_t *data, int startBit, int len) {
  uint32_t val = 0;
  for (int i = 0; i < len; i++) {
    int totalBit = startBit + i;
    int byteIdx = totalBit / 8;
    int bitIdx = totalBit % 8;
    if ((data[byteIdx] >> bitIdx) & 0x01) val |= (1UL << i);
  }
  return val;
}


// rev.09 all models SCCM_leftStalk (0x249) checksum model.
//
// Validated against the user's stock all models passive capture:
//   - 256 / 256 captured frames matched.
//   - DLC = 4
//   - byte 1 low nibble = rolling counter
//   - byte 2 low nibble = turn value
//   - RIGHT soft = 2, LEFT soft = 6
//
// CRC input excludes the counter nibble itself and preserves the rest of the
// live stock payload:
//   { byte1 & 0xF0, byte2, byte3, 0x00 }
// CRC-8 polynomial: 0x2F, initial value 0x00, MSB-first.
// The result is XORed with the counter-specific data-ID byte below.
static const uint8_t CKSUM_CTR[16] = {
  0x9B, 0xE8, 0x2A, 0xD3, 0xD3, 0x83, 0x4C, 0x5E,
  0x3F, 0x5E, 0xE2, 0x28, 0x3A, 0x13, 0xAF, 0xCE
};

static inline uint8_t sccm249Crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x2F)
                         : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

static inline uint8_t leftStalkChecksum(const uint8_t frame[4], uint8_t counter) {
  const uint8_t crcInput[4] = {
    (uint8_t)(frame[1] & 0xF0),
    frame[2],
    frame[3],
    0x00
  };
  return (uint8_t)(sccm249Crc8(crcInput, 4) ^ CKSUM_CTR[counter & 0x0F]);
}

static inline uint8_t dirToTurn(uint8_t dir) {
  if (dir == 1) return STALK_DOWN_1;
  if (dir == 2) return STALK_UP_1;
  return STALK_IDLE;
}

// Read the real 0x249 frame on CAN A and align the injected counter.
static void handle249OnCanA(const uint8_t *data, uint8_t dlc) {
  const uint8_t safeDlc = dlc > 8 ? 8 : dlc;
  portENTER_CRITICAL(&blinkAMux);
  realDlc = safeDlc;
  memset(realRaw249, 0, sizeof(realRaw249));
  if (safeDlc) memcpy(realRaw249, data, safeDlc);

  portEXIT_CRITICAL(&blinkAMux);

  if (dlc < 3) return;

  const uint8_t cnt = data[1] & 0x0F;
  const uint8_t turn = data[2] & 0x0F;
  const uint8_t ck = data[0];

  // rev.09: the validated all models checksum requires all four stock bytes.
  // A short/universal frame is still counted/observed, but cannot pass self-test.
  const bool checksumComparable = dlc >= 4;
  const uint8_t predicted = checksumComparable ? leftStalkChecksum(data, cnt) : 0;

  portENTER_CRITICAL(&blinkAMux);
  rx249++;
  realCounter = cnt;
  realTurn = turn;
  realCksum = ck;
  cksumSelfTest = checksumComparable && (predicted == ck);
  seen249 = true;
  last249Ms = (uint32_t)millis();
  if (activeTurn == STALK_IDLE) blinkACounter = cnt;
  portEXIT_CRITICAL(&blinkAMux);
}

// Send SCCM_turnIndicatorStalkStatus on CAN A.
//
// The newest real stock frame is used as the template so byte1 upper bits,
// byte2 upper bits and byte3 remain exactly as the vehicle produced them.
// Only the rolling counter and requested turn nibble are changed, then the
// validated full-payload CRC is recalculated.
static void sendStalkFrameCanA(uint8_t turn) {
  uint8_t cnt;
  uint8_t stockTemplate[4] = {0};
  bool haveStockTemplate = false;

  portENTER_CRITICAL(&blinkAMux);
  cnt = (blinkACounter + 1) & 0x0F;
  blinkACounter = cnt;
  haveStockTemplate = seen249 && realDlc >= 4;
  if (haveStockTemplate) memcpy(stockTemplate, realRaw249, sizeof(stockTemplate));
  portEXIT_CRITICAL(&blinkAMux);

  if (!haveStockTemplate) {
    portENTER_CRITICAL(&blinkAMux);
    blkATxFail++;
    portEXIT_CRITICAL(&blinkAMux);
    return;
  }

  struct can_frame out = {};
  out.can_id = LEFTSTALK_ID;
  out.can_dlc = 4;
  memcpy(out.data, stockTemplate, sizeof(stockTemplate));

  out.data[1] = (uint8_t)((out.data[1] & 0xF0) | (cnt & 0x0F));
  out.data[2] = (uint8_t)((out.data[2] & 0xF0) | (turn & 0x0F));
  out.data[0] = leftStalkChecksum(out.data, cnt);

  MCP2515::ERROR err = Can_A.sendMessage(&out);
  portENTER_CRITICAL(&blinkAMux);
  if (err == MCP2515::ERROR_OK) {
    blkATxOk++;
    mcpTxOk++;
    mcpTxFailConsecutive = 0;
  } else {
    blkATxFail++;
    mcpTxFail++;
    if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++;
  }
  portEXIT_CRITICAL(&blinkAMux);
}

static inline uint8_t vcleftMux(const uint8_t data[8]) {
  return data[0] & 0x03u;
}

static inline uint8_t vcleftLeftButton(const uint8_t data[8]) {
  return (data[3] >> 6) & 0x03u;
}

static inline uint8_t vcleftRightButton(const uint8_t data[8]) {
  return (data[5] >> 4) & 0x03u;
}

static inline void vcleftSetLeftButton(uint8_t data[8], uint8_t state) {
  data[3] = (uint8_t)((data[3] & 0x3Fu) | ((state & 0x03u) << 6));
}

static inline void vcleftSetRightButton(uint8_t data[8], uint8_t state) {
  data[5] = (uint8_t)((data[5] & 0xCFu) | ((state & 0x03u) << 4));
}

// Observe the real stalkless steering-wheel switch frame and, only during an
// already-armed Auto Blinker one-shot, echo the live mux1 payload with the
// requested turn button changed. 0x3C2 has no rolling counter or checksum;
// using each newly received stock frame preserves every unrelated switch bit
// and keeps the injection at the native mux1 cadence (~10 Hz on the captured
// Highland) instead of generating a free-running synthetic stream.
static void handle3C2OnCanA(const struct can_frame &incoming) {
  if (incoming.can_dlc < 8 || vcleftMux(incoming.data) != VCLEFT_SWITCH_MUX1) return;

  const uint32_t now = (uint32_t)millis();
  const uint8_t left = vcleftLeftButton(incoming.data);
  const uint8_t right = vcleftRightButton(incoming.data);
  bool inject = false;
  bool firstStalklessFrame = false;
  uint8_t turn = STALK_IDLE;
  uint8_t switchState = VCLEFT_SWITCH_SNA;

  portENTER_CRITICAL(&blinkAMux);
  firstStalklessFrame = !seen3C2;
  const uint8_t previousLeft = real3C2LeftButton;
  const uint8_t previousRight = real3C2RightButton;
  seen3C2 = true;
  rx3C2++;
  last3C2Mux1Ms = now;
  real3C2LeftButton = left;
  real3C2RightButton = right;

  // Latch rising edges long enough for the 500 ms dashboard polling cycle.
  // The physical ON phase measured on the Highland lasts only ~300 ms and
  // could otherwise begin and end entirely between two HTTP responses.
  if (left == VCLEFT_SWITCH_ON && previousLeft != VCLEFT_SWITCH_ON) {
    stalklessLastPhysicalTurn = STALK_DOWN_1;
    stalklessLastPhysicalPressMs = now;
    stalklessPhysicalPressCount++;
  }
  if (right == VCLEFT_SWITCH_ON && previousRight != VCLEFT_SWITCH_ON) {
    stalklessLastPhysicalTurn = STALK_UP_1;
    stalklessLastPhysicalPressMs = now;
    stalklessPhysicalPressCount++;
  }

  // A real steering-wheel press always wins over an automatic request.
  // The frame read here is the stock frame, before our echo is generated.
  if (blinkATransportMode == BLINKA_TRANSPORT_3C2 &&
      (left == VCLEFT_SWITCH_ON || right == VCLEFT_SWITCH_ON) &&
      (autoArmed ||
       (blinkATransport == BLINKA_TRANSPORT_3C2 && oneShotTurn != STALK_IDLE))) {
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
    blinkATransport = BLINKA_TRANSPORT_NONE;
    oneShotTurn = STALK_IDLE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
    activeTurn = STALK_IDLE;
    stalklessManualOverride++;
  } else if (blinkATransport == BLINKA_TRANSPORT_3C2 &&
             oneShotTurn != STALK_IDLE &&
             (int32_t)(oneShotUntil - now) > 0) {
    inject = true;
    turn = oneShotTurn;
    switchState = ((int32_t)(oneShotReleaseAt - now) > 0)
                    ? VCLEFT_SWITCH_ON : VCLEFT_SWITCH_OFF;
  }
  portEXIT_CRITICAL(&blinkAMux);

  if (firstStalklessFrame) {
    Serial.println("[CAN A] stalkless 0x3C2 mux1 detected");
  }
  if (!inject) return;

  struct can_frame out = incoming;
  if (turn == STALK_DOWN_1) {
    vcleftSetLeftButton(out.data, switchState);
  } else if (turn == STALK_UP_1) {
    vcleftSetRightButton(out.data, switchState);
  } else {
    return;
  }

  const MCP2515::ERROR err = Can_A.sendMessage(&out);
  portENTER_CRITICAL(&blinkAMux);
  if (err == MCP2515::ERROR_OK) {
    blkATxOk++;
    stalklessTxOk++;
    mcpTxOk++;
    mcpTxFailConsecutive = 0;
  } else {
    blkATxFail++;
    stalklessTxFail++;
    mcpTxFail++;
    if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++;
  }
  portEXIT_CRITICAL(&blinkAMux);
}

// Arm a delayed trigger when behaviorType becomes LEFT/RIGHT.
static void evaluateAutoBlinker() {
  bool en;
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  portEXIT_CRITICAL(&blinkAMux);

  const uint32_t now = (uint32_t)millis();
  const bool gateOpen = autoBlinkerReady(now);

  uint8_t behavior = visualBehaviorType;
  uint8_t reqDir = 0;

  // Auto Blinker is fail-closed. It can arm only after the fresh 0x399
  // gate (states 3, 4 or 5), AP-entry delay and lane-change delay are ready.
  if (en && gateOpen) {
    if (behavior == 2) reqDir = 1;
    else if (behavior == 3) reqDir = 2;
  }

  portENTER_CRITICAL(&blinkAMux);

  if (reqDir != 0 && reqDir != lastReqDir && !autoArmed) {
    autoPendingDir = reqDir;
    autoFireAt = now + blinkATriggerDelayMs;
    autoArmed = true;
  }

  if (reqDir == 0) {
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
  }

  lastReqDir = reqDir;
  portEXIT_CRITICAL(&blinkAMux);
}

// Generate the one-shot 350 ms J3 CHASSIS pulse.
// rev.06 follows the Advanced EAP v1.0b state model:
//   autoFireAt   = delayed-trigger deadline only
//   oneShotUntil = active-pulse deadline only
// This prevents a later 0x24A behavior change from shortening or extending
// an SCCM pulse that has already started.
static void blinkATxTick() {
  static uint32_t lastTxMs = 0;
  uint32_t now = millis();
  uint8_t turn = STALK_IDLE;
  uint8_t transport = BLINKA_TRANSPORT_NONE;
  // Also observes a stale 0x399 timeout, so a later AP/NOA re-entry starts
  // a fresh activation delay even when the state value itself is unchanged.
  updateAutoBlinkerGate(now);
  const bool gateOpen = autoBlinkerReady(now);

  portENTER_CRITICAL(&blinkAMux);

  // Final guard: a request is never allowed to fire unless the fresh 0x399
  // state, AP-entry delay and lane-change delay are all satisfied. A pulse
  // that already started is allowed to finish its existing 350 ms window.
  if (!gateOpen && autoArmed) {
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
    lastReqDir = 0;
  }

  // 1) Delayed trigger reached its deadline -> start an independent pulse.
  if (gateOpen && autoArmed && (int32_t)(now - autoFireAt) >= 0) {
    oneShotTurn = dirToTurn(autoPendingDir);

    const bool fresh249 = seen249 && realDlc >= 4 && cksumSelfTest &&
                          last249Ms != 0 &&
                          (uint32_t)(now - last249Ms) <= BLINKA_SOURCE_FRESH_MS;
    const bool fresh3C2 = seen3C2 && last3C2Mux1Ms != 0 &&
                          (uint32_t)(now - last3C2Mux1Ms) <= BLINKA_SOURCE_FRESH_MS;

    if (oneShotTurn == STALK_IDLE) {
      blinkATransport = BLINKA_TRANSPORT_NONE;
      oneShotUntil = 0;
      oneShotReleaseAt = 0;
    } else if (blinkATransportMode == BLINKA_TRANSPORT_249 && fresh249) {
      // Stalk mode: use the real 0x249 SCCM stalk frame as the template.
      blinkATransport = BLINKA_TRANSPORT_249;
      oneShotReleaseAt = 0;
      oneShotUntil = now + BLINKA_PULSE_MS;
    } else if (blinkATransportMode == BLINKA_TRANSPORT_3C2 && fresh3C2) {
      // Stalkless mode: hold SWITCH_ON for the measured gesture, then
      // SWITCH_OFF, using each real 0x3C2 mux1 frame as the native template.
      blinkATransport = BLINKA_TRANSPORT_3C2;
      oneShotReleaseAt = now + BLINKA_STALKLESS_PRESS_MS;
      oneShotUntil = oneShotReleaseAt + BLINKA_STALKLESS_RELEASE_MS;
    } else {
      // Fail closed when neither a valid stalk template nor a fresh stalkless
      // switch frame is available. Count the failed request once, not at 50 Hz.
      blinkATransport = BLINKA_TRANSPORT_NONE;
      oneShotTurn = STALK_IDLE;
      oneShotUntil = 0;
      oneShotReleaseAt = 0;
      blkATxFail++;
    }
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
  }

  // 2) Once started, the pulse lifetime is independent of autoFireAt and
  //    subsequent behaviorType changes.
  if (oneShotTurn != STALK_IDLE && (int32_t)(oneShotUntil - now) > 0) {
    turn = oneShotTurn;
    transport = blinkATransport;
  } else {
    oneShotTurn = STALK_IDLE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
    blinkATransport = BLINKA_TRANSPORT_NONE;
  }

  activeTurn = turn;
  portEXIT_CRITICAL(&blinkAMux);

  if (turn == STALK_IDLE) return;
  // 0x3C2 is emitted from canTaskMcp only when a real mux1 frame arrives.
  if (transport != BLINKA_TRANSPORT_249) return;
  if (now - lastTxMs < BLINKA_TX_PERIOD_MS) return;
  lastTxMs = now;
  sendStalkFrameCanA(turn);
}

// rev.07: 0x3F8 ULC configuration injection removed.
// handle1016() remains RX-only for SPR detection and passive stock telemetry.


static inline bool isDASActive(uint8_t status) {
  bool fm = forceMode;

  switch (status) {
    // ON : 3,4,5,6
    case 3:
    case 4:
    case 5:
    case 6:
      fm = true;
      break;

    // OFF : 0,1,8,9,14
    case 0:
    case 1:
    case 8:
    case 9:
    case 14:
      fm = false;
      break;

    default:
      fm = false;
      break;
  }


  portENTER_CRITICAL(&stateMux);
  forceMode = fm;
  portEXIT_CRITICAL(&stateMux);


  return status == 3 || status == 4 || status == 5 || status == 6;
}


static inline bool summonInjectionGateOpen() {
    return gateParked || gateSummoning;
}

static void recomputeSummoning() {
    gateSummoning = lastAca && sprSeen;
}

static void clearSummonOnPark() {
    gateSummoning = false;
    sprSeen       = false;
}

static void clearSummonOnParkIfAcaInactive(uint8_t gear) {
    if (gear == 1 && !lastAca)
        clearSummonOnPark();
}

static void handle280(const uint8_t *data) {
    sumRx280++;
    const uint32_t now = (uint32_t)millis();
    last280Millis = now;
    uint8_t gear = readDIGear(data);
    int     gs   = gearState(gear);
    portENTER_CRITICAL(&stateMux);
    if (gs == 1)  gateParked = true;
    if (gs == 0)  gateParked = false;
    if (gs >= 0) {
        priorityGear280State = (int8_t)gs;
        priorityGear280Ms = now;
    }
    bool aca = (data[6] & 0x04) != 0;
    if (lastAca && !aca)
        sprSeen = false;
    lastAca = aca;
    recomputeSummoning();
    clearSummonOnParkIfAcaInactive(gear);
    recomputeSummonPriorityStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
}

static void handle390(const uint8_t *data) {
    sumRx390++;
    const uint32_t now = (uint32_t)millis();
    uint8_t gear = readVehicleGear(data);
    int     gs   = gearState(gear);
    if (gs < 0) return;
    portENTER_CRITICAL(&stateMux);
    priorityGear390State = (int8_t)gs;
    priorityGear390Ms = now;
    uint32_t age = now - last280Millis;
    if (last280Millis == 0 || age > PARKED_TIMEOUT_MS) {
        gateParked = (gs == 1);
        clearSummonOnParkIfAcaInactive(gear);
    }
    recomputeSummonPriorityStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
}

static void handle921(const uint8_t *data) {
    sumRx921++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t dasState4 = readDASState4(data);
    bool ap = isDASActive(readDASStatus(data));
    bool noa = (dasState4 == 5); // ACTIVE_NAV = Navigate on Autopilot
    portENTER_CRITICAL(&stateMux);
    gateAPActive = ap;
    gateNOAActive = noa;
    dasAutopilotState4 = dasState4;
    lastDASStatusMillis = now;
    portEXIT_CRITICAL(&stateMux);

    updateAutoBlinkerGate(now);
}

static void handle1016(const uint8_t *data, uint8_t dlc) {
    if (dlc < 4) return;
    sumRx1016++;
    const uint32_t now = (uint32_t)millis();
    uint8_t spr = (data[3] >> 4) & 0x0F;

    if (dlc >= 7) {
        // UI_ulcSpeedConfig: bits 50-51.
        // UI_ulcBlindSpotConfig: bits 52-53.
        uiUlcSpeedConfig = (data[6] >> 2) & 0x03;
        uiUlcBlindSpotConfig = (data[6] >> 4) & 0x03;
    }

    bool apNoa = false;
    uint8_t injectCfg = 0;
    portENTER_CRITICAL(&stateMux);
    const uint8_t dasState = dasAutopilotState4;
    apNoa = (dasState == 3 || dasState == 5) &&
            lastDASStatusMillis != 0 &&
            (uint32_t)(now - lastDASStatusMillis) <= NOA_STATUS_FRESH_MS;
    portEXIT_CRITICAL(&stateMux);

    portENTER_CRITICAL(&blinkAMux);
    injectCfg = ulcBlindSpotInjectConfig;
    portEXIT_CRITICAL(&blinkAMux);

    // Inject only in AP / NOA. Keep all unrelated bits from the stock frame.
    if (dlc >= 7 && apNoa) {
        const uint8_t stockCfg = (data[6] >> 4) & 0x03;
        if (stockCfg != injectCfg) {
            PartyFrame out = {};
            out.identifier = DRIVER_ASSIST_ID;
            out.data_length_code = dlc;
            out.flags = 0;
            for (uint8_t i = 0; i < 8; i++) out.data[i] = data[i];

            out.data[6] = (uint8_t)((out.data[6] & ~(0x03u << 4)) |
                                   ((injectCfg & 0x03u) << 4));

            if (chassisNonSummonAdmissionOpen()) {
                esp_err_t err = chassisTransmit(&out, 0);
                if (err == ESP_OK) sumTxOk++;
                else               sumTxFail++;
            } else {
                sumTxFail++;
            }
        }
    }

    portENTER_CRITICAL(&stateMux);
    if (spr != 0)
        sprSeen = true;
    recomputeSummoning();
    recomputeSummonPriorityStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
}

// During SUMMONING only, resend the latest real 0x3FD mux1 frame every 1 s.
// Preserve the latest received payload and force bit 19=0, bit 47=1.
static void summonPeriodicTick() {
    bool summoning;
    portENTER_CRITICAL(&stateMux);
    summoning = gateSummoning;
    portEXIT_CRITICAL(&stateMux);

    if (!summoning)
        return;

    const uint32_t now = (uint32_t)millis();
    if (lastSummonPeriodicTxMs != 0 &&
        (uint32_t)(now - lastSummonPeriodicTxMs) < SUMMON_PERIODIC_TX_MS)
        return;

    uint8_t dat[8];
    bool haveTemplate;
    portENTER_CRITICAL(&blinkAMux);
    haveTemplate = seen3fdMux1;
    if (haveTemplate)
        memcpy(dat, realRaw3fdMux1, 8);
    portEXIT_CRITICAL(&blinkAMux);

    if (!haveTemplate)
        return;

    setBit(dat, 19, false);
    setBit(dat, 47, true);

    PartyFrame out = {};
    out.identifier = 0x3FD;
    out.data_length_code = 8;
    out.flags = 0;
    memcpy(out.data, dat, 8);

    esp_err_t err = chassisTransmitSummonPriority(&out);
    if (err == ESP_OK) {
        summonPeriodicTxOk++;
        lastSummonPeriodicTxMs = now;
    } else {
        summonPeriodicTxFail++;
    }
}						
static void injectSummon(const PartyFrame &src) {
     bool en, gate, fmode, ap;
    portENTER_CRITICAL(&stateMux);
    en   = summonEnabled;
    gate = summonInjectionGateOpen();
    fmode = forceMode;
    // Use the 0x399 state directly as well as the legacy AP gate. This keeps
    // the R79 correction tied to actual AP states 3/4/5 even if the legacy
    // three-bit status decoder changes in a future capture.
    ap = gateAPActive || dasAutopilotState4 == 3 || dasAutopilotState4 == 4 || dasAutopilotState4 == 5;
     if (!gate && !fmode) {
        if (!gateAPActive  && !gateParked && !gateSummoning)
            strncpy(gateBlockReason, "AP-,Park-,Summon-", sizeof(gateBlockReason));
    }
    portEXIT_CRITICAL(&stateMux);
    const bool unlockAllowed = (en && gate) || fmode;

    PartyFrame out;
    out.identifier       = src.identifier;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];
    sumRxMux1++;
    if (ap) r79RxMux1++;

    // R79 (bit 19) follows AP state from 0x399 and is independent of Park /
    // Summon. The EU Unlock bit 47 remains strictly behind the Summon gate.
    const bool needR79Patch = ap && getBit(out.data, 19);
    const bool needSummonUnlock = unlockAllowed &&
                                  (getBit(out.data, 19) || !getBit(out.data, 47));
    if (!needR79Patch && !needSummonUnlock) return;

    if (needR79Patch) setBit(out.data, 19, false);
    if (needSummonUnlock) {
      setBit(out.data, 19, false);
      setBit(out.data, 47, true);
    }

    // Only the actual EU Unlock uses the Summon priority/gate transport.
    // An AP-active R79 correction is sent on J3 without granting bit 47.
    esp_err_t err = needSummonUnlock ? chassisTransmitSummonPriority(&out)
                                     : chassisTransmit(&out, 0);
    const bool patchedR79 = needR79Patch || (needSummonUnlock && getBit(src.data, 19));
    if (err == ESP_OK) {
      sumTxOk++;
      if (patchedR79) r79TxOk++;
    } else {
      sumTxFail++;
      if (patchedR79) r79TxFail++;
    }
}

// ── TLSSC : 0x3FD mux0 bit38/39 ──
// rev.16: TLSSC has its own AP-active gate. It no longer shares or bypasses
// the Parked/Summoning gate used by Summon/EU Unlock.
static void injectTLSSC(const PartyFrame &src) {
    bool en, ap;
    portENTER_CRITICAL(&stateMux);
    en = tlsscEnabled;
    ap = gateAPActive;
    portEXIT_CRITICAL(&stateMux);

    // AP patch: UI_applyEceR79 (0x3FD bit 19) must be forced to 0
    // whenever AP is active, independently of the TLSSC setting.
    // TLSSC bits 38/39 remain controlled separately by tlsscEnabled.
    if (!ap)
        return;

    r79RxMux0++;

    PartyFrame out;
    out.identifier       = src.identifier;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];

    const bool needEcePatch = getBit(out.data, 19); // force 0 in AP
    const bool needTlsscPatch = en && (!getBit(out.data, 38) || !getBit(out.data, 39));

    // Nothing to change: do not echo a duplicate frame.
    if (!needEcePatch && !needTlsscPatch)
        return;

    if (needEcePatch) {
        setBit(out.data, 19, false);  // UI_applyEceR79 = 0
    }

    if (en) {
        setBit(out.data, 38, true);   // UI_fsdStopsControlEnabled = 1
        setBit(out.data, 39, true);   // UI_fsdContinueOnGreenWithCIPV = 1
    }

    // AP/TLSSC patch is lower priority than Summon and must never block J3 RX.
    if (!chassisNonSummonAdmissionOpen()) {
      sumTxFail++;
      if (needEcePatch) r79TxFail++;
      return;
    }
    esp_err_t err = chassisTransmit(&out, 0);
    if (err == ESP_OK) {
      sumTxOk++;
      if (needEcePatch) r79TxOk++;
    } else {
      sumTxFail++;
      if (needEcePatch) r79TxFail++;
    }
}

// ── TLSSC Restore : 0x331 (DAS_autopilotConfig) ──
// DAS_autopilot & DAS_autopilotBase -> SELF_DRIVING (3).
// Force byte 0 low six bits to 0x1B while preserving the top two bits.
static void doInjectTlsscRestore(const PartyFrame &src) {
    bool en;
    portENTER_CRITICAL(&stateMux);
    en = tlsscRestoreEnabled;
    portEXIT_CRITICAL(&stateMux);
    if (!en || src.data_length_code < 1) return;

    // Do not echo a frame that is already patched.
    if ((src.data[0] & 0x3F) == 0x1B) return;

    PartyFrame out = {};
    out.identifier       = 0x331;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];

    out.data[0] = (uint8_t)((out.data[0] & 0xC0) | 0x1B);

    esp_err_t err = chassisTransmit(&out, pdMS_TO_TICKS(2));
    if (err == ESP_OK) sumTxOk++;
    else               sumTxFail++;
}

static void summonCfgLoad() {
    prefs.begin("summon", false);
    summonEnabled = prefs.getBool("en", true);
    tlsscEnabled  = prefs.getBool("tlssc", false);
    tlsscRestoreEnabled = prefs.getBool("tlrst", false);
    blinkAEnabled = prefs.getBool("blkA", true);
    blinkATransportMode = (uint8_t)constrain((int)prefs.getUInt("blkATrans", BLINKA_TRANSPORT_249), BLINKA_TRANSPORT_249, BLINKA_TRANSPORT_3C2);
    ulcBlindSpotInjectConfig = (uint8_t)constrain((int)prefs.getUInt("ulcBspot", 0), 0, 2);

    // Three independent timing settings: the original delay before a turn
    // request, the AP/NOA-entry delay, and the post-lane-change re-arm delay.
    blinkATriggerDelayMs = constrain((uint32_t)prefs.getUInt("blkTrigD", BLINKA_TRIGGER_DELAY_DEFAULT_MS), (uint32_t)0, (uint32_t)30000);
    blinkAApEntryDelayMs = constrain((uint32_t)prefs.getUInt("blkApEnt", BLINKA_AP_ENTRY_DELAY_DEFAULT_MS), (uint32_t)0, (uint32_t)30000);
    blinkALaneChangeDelayMs = constrain((uint32_t)prefs.getUInt("blkLaneD", BLINKA_LANE_CHANGE_DELAY_DEFAULT_MS), (uint32_t)0, (uint32_t)60000);

    // Compatibility cleanup: remove retired configuration keys from older revisions.
    prefs.remove("blkAMode");
    prefs.remove("blkADly");
    prefs.remove("blkDly17");
    prefs.remove("ulcbs");
    prefs.remove("ulcsp");
    prefs.end();
}

static void summonCfgSave() {
    prefs.begin("summon", false);
    prefs.putBool("en", summonEnabled);
    prefs.putBool("tlssc", tlsscEnabled);
    prefs.putBool("tlrst", tlsscRestoreEnabled);
    prefs.putBool("blkA", blinkAEnabled);
    prefs.putUInt("blkATrans", blinkATransportMode);
    prefs.putUInt("blkTrigD", blinkATriggerDelayMs);
    prefs.putUInt("blkApEnt", blinkAApEntryDelayMs);
    prefs.putUInt("blkLaneD", blinkALaneChangeDelayMs);
    prefs.putUInt("ulcBspot", ulcBlindSpotInjectConfig);
    prefs.end();
}

// ═══════════════════════════════════════════════════════════════
// OTA UPDATE
// ═══════════════════════════════════════════════════════════════

static volatile bool     otaInProgress = false;
static volatile bool     otaSuccess    = false;
static volatile bool     otaError      = false;
static volatile uint32_t otaBytes      = 0;
static volatile uint32_t otaTotal      = 0;
static char              otaErrMsg[64] = "";

// ═══════════════════════════════════════════════════════════════
// WEB SERVER
// ═══════════════════════════════════════════════════════════════

extern const char INDEX_HTML[] PROGMEM;
static WebServer server(80);

static String summonStatsToJson() {
    bool en, tlssc, tlsscRestore, ap, parked, summon, aca, spr, fmode, priorityFreshParked;
    uint8_t priorityState;
    uint32_t prioritySince, priorityTransitions, priorityFullEnter, priorityFullExit, priorityFullInactiveSince;
    uint32_t rmx, tok, tfail, r280, r390, r921, r1016, r79m1, r79m0, r79ok, r79fail;
    portENTER_CRITICAL(&stateMux);
    en     = summonEnabled;
    tlssc  = tlsscEnabled;
    tlsscRestore = tlsscRestoreEnabled;
    ap     = gateAPActive;
    parked = gateParked;
    summon = gateSummoning;
    aca    = lastAca;
    spr    = sprSeen;
    fmode  = forceMode;
    priorityState = summonPriorityState;
    priorityFreshParked = summonPriorityFreshParkedLocked((uint32_t)millis());
    prioritySince = summonPriorityStateSinceMs;
    priorityTransitions = summonPriorityTransitions;
    priorityFullEnter = summonPriorityFullEnterCount;
    priorityFullExit = summonPriorityFullExitCount;
    priorityFullInactiveSince = summonPriorityFullInactiveSinceMs;
    rmx    = sumRxMux1;
    tok    = sumTxOk;
    tfail  = sumTxFail;
    r280   = sumRx280;
    r390   = sumRx390;
    r921   = sumRx921;
    r1016  = sumRx1016;
    r79m1  = r79RxMux1;
    r79m0  = r79RxMux0;
    r79ok  = r79TxOk;
    r79fail = r79TxFail;
    portEXIT_CRITICAL(&stateMux);
    bool gate = parked || summon;
    PartyStatus st = {};
    const bool partyStatusOk = (chassisGetStatus(&st) == ESP_OK);
    String s = "{";
    s += "\"enabled\":"  + String(en     ? "true" : "false");
    s += ",\"tlssc\":"   + String(tlssc  ? "true" : "false");
    s += ",\"tlsscRestore\":" + String(tlsscRestore ? "true" : "false");
    s += ",\"gate\":"    + String(gate   ? "true" : "false");
    s += ",\"ap\":"      + String(ap     ? "true" : "false");
    s += ",\"parked\":"  + String(parked ? "true" : "false");
    s += ",\"summon\":"  + String(summon ? "true" : "false");
    s += ",\"aca\":"     + String(aca    ? "true" : "false");
    s += ",\"spr\":"     + String(spr    ? "true" : "false");
    s += ",\"forceMode\":"+ String(fmode ? "true" : "false");
    s += ",\"priorityState\":" + String((int)priorityState);
    s += ",\"priorityStateName\":\"" + String(summonPriorityStateName(priorityState)) + "\"";
    s += ",\"priorityFreshParked\":" + String(priorityFreshParked ? "true" : "false");
    s += ",\"priorityStateSinceMs\":" + String((unsigned long)prioritySince);
    s += ",\"priorityTransitions\":" + String((unsigned long)priorityTransitions);
    s += ",\"priorityFullEnter\":" + String((unsigned long)priorityFullEnter);
    s += ",\"priorityFullExit\":" + String((unsigned long)priorityFullExit);
    s += ",\"priorityExitGraceActive\":" + String(priorityFullInactiveSince != 0 ? "true" : "false");
    s += ",\"txQueueNow\":" + String((unsigned long)partyTxQueueNow);
    s += ",\"txQueueMax\":" + String((unsigned long)partyTxQueueMax);
    s += ",\"nonSummonShed\":" + String((unsigned long)partyNonSummonShed);
    s += ",\"standbyShed\":" + String((unsigned long)partyStandbyShed);
    s += ",\"fullShed\":" + String((unsigned long)partyFullShed);
    s += ",\"summonQueueFlush\":" + String((unsigned long)partySummonQueueFlush);
    s += ",\"summonTxNormal\":" + String((unsigned long)partySummonTxNormal);
    s += ",\"summonTxStandby\":" + String((unsigned long)partySummonTxStandby);
    s += ",\"summonTxFull\":" + String((unsigned long)partySummonTxFull);
    s += ",\"rxMux1\":"  + String(rmx);
    s += ",\"txOk\":"    + String(tok);
    s += ",\"txFail\":"  + String(tfail);
    s += ",\"rx280\":"   + String(r280);
    s += ",\"rx390\":"   + String(r390);
    s += ",\"rx921\":"   + String(r921);
    s += ",\"rx1016\":"  + String(r1016);
    s += ",\"r79RxMux1\":" + String(r79m1);
    s += ",\"r79RxMux0\":" + String(r79m0);
    s += ",\"r79TxOk\":" + String(r79ok);
    s += ",\"r79TxFail\":" + String(r79fail);
    s += ",\"canState\":" + String(partyStatusOk ? (int)st.state : -1);
    s += ",\"canStateName\":\"" + String(partyStatusOk ? partyStateName(st.state) : "UNAVAILABLE") + "\"";
    s += ",\"uptimeS\":"  + String((millis() - bootTime) / 1000);
    s += "}";
    return s;
}


static String blinkAStatsToJson() {
  bool en, ap, noaRaw, noaEffective, noaFresh, fmode;
  uint8_t dasState4;
  uint8_t curTurn, pending, transport, transportMode, leftButton, rightButton, physicalTurn;
  uint32_t triggerDelayMs, apEntryDelayMs, laneChangeDelayMs, apReadyAt, laneReadyAt, remain, txOk, txFail, r249, r3C2;
  uint32_t last3C2, physicalPressMs, physicalPressCount;
  uint32_t sTxOk, sTxFail, manualOverride;
  bool armed, seen, selfTest, seenStalkless;
  uint8_t rCnt, rTurn, rCk, rDlc;
  uint8_t raw249[8] = {0};
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  curTurn = activeTurn;
  transport = blinkATransport;
  transportMode = blinkATransportMode;
  pending = autoPendingDir;
  triggerDelayMs = blinkATriggerDelayMs;
  apEntryDelayMs = blinkAApEntryDelayMs;
  laneChangeDelayMs = blinkALaneChangeDelayMs;
  apReadyAt = autoApReadyAt;
  laneReadyAt = autoLaneChangeReadyAt;
  armed = autoArmed;
  uint32_t now = millis();
  remain = (autoArmed && (int32_t)(autoFireAt - now) > 0) ? (autoFireAt - now) : 0;
  txOk = blkATxOk;
  txFail = blkATxFail;
  r249 = rx249;
  r3C2 = rx3C2;
  last3C2 = last3C2Mux1Ms;
  leftButton = real3C2LeftButton;
  rightButton = real3C2RightButton;
  physicalTurn = stalklessLastPhysicalTurn;
  physicalPressMs = stalklessLastPhysicalPressMs;
  physicalPressCount = stalklessPhysicalPressCount;
  seenStalkless = seen3C2;
  sTxOk = stalklessTxOk;
  sTxFail = stalklessTxFail;
  manualOverride = stalklessManualOverride;
  rCnt = realCounter;
  rTurn = realTurn;
  rCk = realCksum;
  seen = seen249;
  selfTest = cksumSelfTest;
  rDlc = realDlc;
  memcpy(raw249, realRaw249, sizeof(raw249));
  portEXIT_CRITICAL(&blinkAMux);
  uint32_t noaLastMs;
  portENTER_CRITICAL(&stateMux);
  ap = gateAPActive;
  noaRaw = gateNOAActive;
  dasState4 = dasAutopilotState4;
  noaLastMs = lastDASStatusMillis;
  fmode = forceMode;
  portEXIT_CRITICAL(&stateMux);

  const uint32_t noaNow = (uint32_t)millis();
  const uint32_t stalklessAgeMs = (last3C2 == 0) ? UINT32_MAX : (uint32_t)(noaNow - last3C2);
  const uint32_t physicalPressAgeMs = (physicalPressMs == 0) ? UINT32_MAX : (uint32_t)(noaNow - physicalPressMs);
  const bool physicalPressActive = physicalTurn != STALK_IDLE &&
                                   physicalPressMs != 0 &&
                                   physicalPressAgeMs <= BLINKA_PHYSICAL_DISPLAY_MS;
  const uint32_t noaAgeMs = (noaLastMs == 0) ? UINT32_MAX : (uint32_t)(noaNow - noaLastMs);
  noaFresh = (noaLastMs != 0 && noaAgeMs <= NOA_STATUS_FRESH_MS);
  noaEffective = noaRaw && noaFresh;
  const bool blinkStateOpen = autoBlinkerGateOpen(noaNow);
  const bool blinkGateOpen = autoBlinkerReady(noaNow);
  const uint32_t apEntryRemainMs = (apReadyAt != 0 && (int32_t)(apReadyAt - noaNow) > 0) ? (apReadyAt - noaNow) : 0;
  const uint32_t laneChangeRemainMs = (laneReadyAt != 0 && (int32_t)(laneReadyAt - noaNow) > 0) ? (laneReadyAt - noaNow) : 0;

  String s = "{";
  s += "\"enabled\":" + String(en ? "true" : "false");
  s += ",\"apActive\":" + String(ap ? "true" : "false");
  s += ",\"noaActive\":" + String(noaEffective ? "true" : "false");
  s += ",\"noaRawActive\":" + String(noaRaw ? "true" : "false");
  s += ",\"noaFresh\":" + String(noaFresh ? "true" : "false");
  s += ",\"noaAgeMs\":" + String((noaAgeMs == UINT32_MAX) ? 999999UL : (unsigned long)noaAgeMs);
  s += ",\"noaFreshLimitMs\":" + String((unsigned long)NOA_STATUS_FRESH_MS);
  s += ",\"gateOpen\":" + String(blinkGateOpen ? "true" : "false");
  s += ",\"stateGateOpen\":" + String(blinkStateOpen ? "true" : "false");
  s += ",\"dasState\":" + String((int)dasState4);
  s += ",\"forceMode\":" + String(fmode ? "true" : "false");
  s += ",\"behaviorType\":" + String((int)visualBehaviorType);
  s += ",\"ulcBlindSpotInjectConfig\":" + String((int)ulcBlindSpotInjectConfig);
  s += ",\"laneChangeButtonPressed\":" + String(laneChangeButtonPressed ? "true" : "false");
  s += ",\"laneChangeButtonRx\":" + String((unsigned long)laneChangeButtonRx);
  s += ",\"laneChangeCancelCount\":" + String((unsigned long)laneChangeCancelCount);
  s += ",\"activeTurn\":" + String(curTurn);
  s += ",\"transportMode\":" + String((int)transportMode);
  s += ",\"transportName\":\"" + String(transport == BLINKA_TRANSPORT_249 ? "0x249_STALK" :
                                                transport == BLINKA_TRANSPORT_3C2 ? "0x3C2_STALKLESS" : "IDLE") + "\"";
  s += ",\"triggerDelayMs\":" + String(triggerDelayMs);
  s += ",\"apEntryDelayMs\":" + String(apEntryDelayMs);
  s += ",\"laneChangeDelayMs\":" + String(laneChangeDelayMs);
  s += ",\"apEntryRemainMs\":" + String(apEntryRemainMs);
  s += ",\"laneChangeRemainMs\":" + String(laneChangeRemainMs);
  s += ",\"autoArmed\":" + String(armed ? "true" : "false");
  s += ",\"autoPending\":" + String(pending);
  s += ",\"autoRemainMs\":" + String(remain);
  s += ",\"txOk\":" + String(txOk);
  s += ",\"txFail\":" + String(txFail);
  s += ",\"rx249\":" + String(r249);
  s += ",\"seen249\":" + String(seen ? "true" : "false");
  s += ",\"rx3C2\":" + String(r3C2);
  s += ",\"seen3C2\":" + String(seenStalkless ? "true" : "false");
  s += ",\"stalklessAgeMs\":" + String((stalklessAgeMs == UINT32_MAX) ? 999999UL : (unsigned long)stalklessAgeMs);
  s += ",\"stalklessLeftButton\":" + String((int)leftButton);
  s += ",\"stalklessRightButton\":" + String((int)rightButton);
  s += ",\"physicalPressActive\":" + String(physicalPressActive ? "true" : "false");
  s += ",\"physicalTurn\":" + String((int)physicalTurn);
  s += ",\"physicalTurnName\":\"" + String(physicalTurn == STALK_DOWN_1 ? "LEFT" :
                                                   physicalTurn == STALK_UP_1 ? "RIGHT" : "IDLE") + "\"";
  s += ",\"physicalPressAgeMs\":" + String((physicalPressAgeMs == UINT32_MAX) ? 999999UL : (unsigned long)physicalPressAgeMs);
  s += ",\"physicalPressCount\":" + String((unsigned long)physicalPressCount);
  s += ",\"stalklessTxOk\":" + String((unsigned long)sTxOk);
  s += ",\"stalklessTxFail\":" + String((unsigned long)sTxFail);
  s += ",\"stalklessManualOverride\":" + String((unsigned long)manualOverride);
  s += ",\"realCounter\":" + String(rCnt);
  s += ",\"realTurn\":" + String(rTurn);
  s += ",\"realCksum\":" + String(rCk);
  s += ",\"realDlc\":" + String(rDlc);
  String rawHex;
  rawHex.reserve(24);
  for (uint8_t i = 0; i < rDlc; i++) {
    if (i) rawHex += " ";
    if (raw249[i] < 0x10) rawHex += "0";
    rawHex += String(raw249[i], HEX);
  }
  rawHex.toUpperCase();
  s += ",\"realRaw\":\"" + rawHex + "\"";
  s += ",\"cksumSelfTest\":" + String(selfTest ? "true" : "false");
  s += ",\"canBState\":" + String((int)partyReady);
  s += ",\"uptimeS\":" + String((millis() - bootTime) / 1000);
  s += "}";
  return s;
}

static String dasTelemetryStatsToJson() {
  uint32_t now = millis();
  String s = "{";
  s += "\"behaviorType\":" + String((int)visualBehaviorType);
  s += ",\"ulcBlindSpotConfig\":" + String((int)uiUlcBlindSpotConfig);
  s += ",\"ulcSpeedConfig\":" + String((int)uiUlcSpeedConfig);
  s += ",\"visualDebugRx\":" + String((unsigned long)visualDebugRxCount);
  s += ",\"visualDebugStaleMs\":" + String(visualDebugLastMs == 0 ? 999999UL : (now - visualDebugLastMs));
  s += "}";
  return s;
}

static String systemStatsToJson() {
  String s = "{";
  s += "\"fwVersion\":\"" + String(FW_VERSION) + "\"";
  s += ",\"freeHeap\":"      + String(ESP.getFreeHeap());
  s += ",\"uptimeS\":"      + String((millis() - bootTime) / 1000);
  s += ",\"mcpReady\":"     + String(mcpReady  ? "true" : "false");
  s += ",\"chassisReady\":" + String(chassisMcpReady ? "true" : "false");
  s += ",\"partyReady\":"   + String(partyReady ? "true" : "false");
const uint32_t busNow = (uint32_t)millis();
  const bool mcpTrafficOnline = mcpReady && lastCanAFrameMs != 0 &&
      (uint32_t)(busNow - lastCanAFrameMs) <= CAN_TRAFFIC_ONLINE_MS;
  const bool legacyPartyTrafficOnline = lastCanBFrameMs != 0 &&
      (uint32_t)(busNow - lastCanBFrameMs) <= CAN_TRAFFIC_ONLINE_MS;
  s += ",\"mcpTrafficOnline\":" + String(mcpTrafficOnline ? "true" : "false");
  s += ",\"legacyPartyTrafficOnline\":" + String(legacyPartyTrafficOnline ? "true" : "false");																											  
  const bool chassisTrafficOnline = chassisMcpReady && lastCanChassisFrameMs != 0 &&
      (uint32_t)(busNow - lastCanChassisFrameMs) <= CAN_TRAFFIC_ONLINE_MS;
  const bool partyTrafficOnline = partyReady && lastCanPartyFrameMs != 0 &&
      (uint32_t)(busNow - lastCanPartyFrameMs) <= CAN_TRAFFIC_ONLINE_MS;
  s += ",\"chassisTrafficOnline\":" + String(chassisTrafficOnline ? "true" : "false");
  s += ",\"partyTrafficOnline\":" + String(partyTrafficOnline ? "true" : "false");
  s += ",\"trafficOnlineLimitMs\":" + String(CAN_TRAFFIC_ONLINE_MS);
  s += ",\"bodyAgeMs\":" + String(lastCanAFrameMs ? (uint32_t)(busNow - lastCanAFrameMs) : 999999UL);
  s += ",\"chassisAgeMs\":" + String(lastCanChassisFrameMs ? (uint32_t)(busNow - lastCanChassisFrameMs) : 999999UL);
  s += ",\"partyAgeMs\":" + String(lastCanPartyFrameMs ? (uint32_t)(busNow - lastCanPartyFrameMs) : 999999UL);
  s += ",\"chassisRx\":" + String((unsigned long)chassisRxCount);
  s += ",\"partyRx\":" + String((unsigned long)partyRxCount);
  s += ",\"rtcBootCount\":" + String((unsigned long)rtcBootCount);
  s += ",\"runtimeStatsResetCount\":" + String((unsigned long)runtimeStatsResetCount);
  s += ",\"runtimeStatsLastResetMs\":" + String((unsigned long)runtimeStatsLastResetMs);
  s += ",\"canHardReinit\":" + String((unsigned long)canHardReinitCount);
  s += ",\"canHardReinitFail\":" + String((unsigned long)canHardReinitFailCount);
  s += ",\"canLastHardReason\":" + String((int)canLastHardReinitReason);
  s += ",\"canRecoverySleeping\":" + String(recoverySleeping ? "true" : "false");
  s += ",\"partyTxQueueNow\":" + String((unsigned long)partyTxQueueNow);
  s += ",\"partyTxQueueMax\":" + String((unsigned long)partyTxQueueMax);
  s += ",\"partyRxQueueNow\":" + String((unsigned long)partyRxQueueNow);
  s += ",\"partyRxQueueMax\":" + String((unsigned long)partyRxQueueMax);
  s += ",\"partyNonSummonShed\":" + String((unsigned long)partyNonSummonShed);
  s += ",\"partyStandbyShed\":" + String((unsigned long)partyStandbyShed);
  s += ",\"partyFullShed\":" + String((unsigned long)partyFullShed);
  s += ",\"summonPriorityState\":" + String((int)getSummonPriorityState());
  s += ",\"summonPriorityStateName\":\"" + String(summonPriorityStateName(getSummonPriorityState())) + "\"";
  s += ",\"partySummonTxNormal\":" + String((unsigned long)partySummonTxNormal);
  s += ",\"partySummonTxStandby\":" + String((unsigned long)partySummonTxStandby);
  s += ",\"partySummonTxFull\":" + String((unsigned long)partySummonTxFull);
  s += ",\"partySummonQueueFlush\":" + String((unsigned long)partySummonQueueFlush);
  s += ",\"partySummonRetryOk\":" + String((unsigned long)partySummonRetryOk);
  s += ",\"partySummonRetryFail\":" + String((unsigned long)partySummonRetryFail);
  s += ",\"otaInProgress\":" + String(otaInProgress ? "true" : "false");
  s += ",\"otaSuccess\":"    + String(otaSuccess    ? "true" : "false");
  s += ",\"otaError\":"      + String(otaError      ? "true" : "false");
  s += ",\"otaErrMsg\":\""   + String(otaErrMsg) + "\"";
  s += ",\"otaBytes\":"      + String(otaBytes);
  s += ",\"otaTotal\":"      + String(otaTotal);
  s += "}";
  return s;
}

// ─── Boot timing capture export ─────────────────────────────

static const char* bootCaptureHardReasonName(uint8_t reason) {
  switch (reason) {
    case CAN_SUP_HARD_ACQUIRE: return "ACQUIRE";
    case CAN_SUP_HARD_STALE:   return "STALE";
    case CAN_SUP_HARD_MANUAL:  return "MANUAL";
    default:                   return "UNKNOWN";
  }
}

static void bootCaptureAppendEvent(String &out, const char *event, uint32_t t, const String &detail = String()) {
  out += event;
  out += ",";
  if (t == BOOT_CAPTURE_UNSET) out += "-1";
  else out += String((unsigned long)t);
  out += ",\"";
  out += detail;
  out += "\"\n";
}

static String bootCaptureToCsv() {
  uint32_t canInitDone, canTasks, wifiReady, firstA, firstB;
  uint32_t first370, first370Torque, first399, first24A, first249;
  uint16_t first370Raw, first370TorqueRaw;
  uint8_t hardCount;
  uint32_t hardDropped;
  BootHardReinitEvent hard[BOOT_CAPTURE_HARD_MAX];

  portENTER_CRITICAL(&bootCaptureMux);
  canInitDone = bootCapCanInitDoneMs;
  canTasks = bootCapCanTasksStartedMs;
  wifiReady = bootCapWifiReadyMs;
  firstA = bootCapFirstCanAMs;
  firstB = bootCapFirstCanBMs;
  first399 = bootCapFirst399Ms;
  first24A = bootCapFirstParty24AMs;
  first249 = bootCapFirstVh249Ms;
  hardCount = bootCapHardCount;
  hardDropped = bootCapHardDropped;
  for (uint8_t i = 0; i < hardCount && i < BOOT_CAPTURE_HARD_MAX; i++) hard[i] = bootCapHard[i];
  portEXIT_CRITICAL(&bootCaptureMux);

  String out;
  out.reserve(2200);
  out = "event,time_ms,detail\n";
  bootCaptureAppendEvent(out, "BOOT_SETUP_START", 0, String(FW_VERSION));
  bootCaptureAppendEvent(out, "CAN_INIT_DONE", canInitDone);
  bootCaptureAppendEvent(out, "CAN_RX_TASKS_STARTED", canTasks);
  bootCaptureAppendEvent(out, "WIFI_AP_READY", wifiReady);
  bootCaptureAppendEvent(out, "FIRST_CAN_A_ANY", firstA, "Party/MCP2515");
  bootCaptureAppendEvent(out, "FIRST_CAN_B_ANY", firstB, "VH/PARTY");

  bootCaptureAppendEvent(out, "FIRST_PARTY_0x399", first399, "DAS/AP state");
  bootCaptureAppendEvent(out, "FIRST_PARTY_0x24A_DLC8", first24A, "DAS visual debug / Auto Blinker source");
  bootCaptureAppendEvent(out, "FIRST_CAN_A_0x249_DLC4", first249, "SCCM stalk status");

  for (uint8_t i = 0; i < hardCount && i < BOOT_CAPTURE_HARD_MAX; i++) {
    String startName = "HARD_REINIT_" + String((unsigned)(i + 1)) + "_START";
    String endName = "HARD_REINIT_" + String((unsigned)(i + 1)) + "_END";
    String detail = "reason=" + String(bootCaptureHardReasonName(hard[i].reason));
    bootCaptureAppendEvent(out, startName.c_str(), hard[i].startMs, detail);
    String endDetail = detail + ";success=" + String(hard[i].success == 1 ? "1" : hard[i].success == 0 ? "0" : "in_progress");
    bootCaptureAppendEvent(out, endName.c_str(), hard[i].endMs, endDetail);
  }

  bootCaptureAppendEvent(out, "EXPORT", bootCaptureNowMs(),
    "hard_reinit_events=" + String((unsigned)hardCount) +
    ";hard_reinit_dropped=" + String((unsigned long)hardDropped) +
    ";mcp_rx_count=" + String((unsigned long)mcpRxCount) +
    ";vh_rx_count=" + String((unsigned long)canRxBeat));
  return out;
}

static void httpBootCaptureCsv() {
  server.sendHeader("Content-Disposition", "attachment; filename=boot_capture.csv");
  server.send(200, "text/csv", bootCaptureToCsv());
}

// ─── OTA update ─────────────────────────────────────────────

static void httpOtaUpload() {
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        otaInProgress = true;
        otaSuccess    = false;
        otaError      = false;
        otaBytes      = 0;
        otaErrMsg[0]  = '\0';
        Serial.printf("[OTA] Start: %s\n", up.filename.c_str());

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] begin() failed: %s\n", otaErrMsg);
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (!otaError && Update.write(up.buf, up.currentSize) != up.currentSize) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] write() failed: %s\n", otaErrMsg);
        }
        otaBytes += up.currentSize;
    } else if (up.status == UPLOAD_FILE_END) {
        if (!otaError && Update.end(true)) {
            otaSuccess = true;
            otaTotal   = otaBytes;
            Serial.printf("[OTA] Success: %u bytes\n", up.totalSize);
        } else if (!otaError) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] end() failed: %s\n", otaErrMsg);
        }
        otaInProgress = false;
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        otaInProgress = false;
        otaError      = true;
        strncpy(otaErrMsg, "aborted", sizeof(otaErrMsg) - 1);
        Serial.println("[OTA] Aborted");
    }
}

static void httpOtaFinish() {
    bool ok = otaSuccess && !otaError;
    String resp = String("{\"ok\":") + (ok ? "true" : "false") +
                  ",\"error\":\"" + String(otaErrMsg) + "\"}";
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", resp);
    if (ok) {
        delay(700);
        ESP.restart();
    }
}

static void httpSystemStats() { server.send(200, "application/json", systemStatsToJson()); }

// J4 PARTY / New.ino controls.  Keeping these endpoints separate prevents a
// Nag configuration change from touching the Advanced-EAP settings namespace.
static void httpNagConfig() { server.send(200, "application/json", nagParty.configJson()); }
static void httpNagStats()  { server.send(200, "application/json", nagParty.statsJson()); }
static void httpNagMode() {
  nagParty.setMode((uint8_t)constrain(server.arg("mode").toInt(), 0, 2));
  server.send(200, "application/json", nagParty.configJson());
}
static void httpNagUpdate() {
  const bool enabled = server.hasArg("enabled") && server.arg("enabled") == "1";
  const uint16_t target = (uint16_t)constrain((int)strtol(server.arg("targetId").c_str(), nullptr, 0), 1, 0x7FF);
  const uint8_t rate = (uint8_t)constrain(server.arg("hoRatePct").toInt(), 0, 100);
  const uint16_t burst = (uint16_t)constrain(server.arg("burstMs").toInt(), 50, 10000);
  const uint16_t pause = (uint16_t)constrain(server.arg("pauseMs").toInt(), 0, 10000);
  nagParty.update(enabled, target, rate, burst, pause);
  server.send(200, "application/json", nagParty.configJson());
}
static void httpNagReset() {
  nagParty.resetStats();
  server.send(200, "application/json", "{\"ok\":true}");
}


static void httpCanHardReinit() {
  requestCanSubsystemRestart(CAN_SUP_HARD_MANUAL);
  server.send(202, "application/json", "{\"ok\":true,\"action\":\"hard-can-reinit-requested\"}");
}

static void httpRebootT2Can() {
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"rebooting\"}");
  delay(250);
  ESP.restart();
}

static void httpRoot()   { server.send_P(200, "text/html", INDEX_HTML); }
static void httpSummonStats()  { server.send(200, "application/json", summonStatsToJson()); }
static void httpSummonEnable() {
    portENTER_CRITICAL(&stateMux); summonEnabled = true;  portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonDisable() {
    portENTER_CRITICAL(&stateMux); summonEnabled = false; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonTlsscEnable() {
    portENTER_CRITICAL(&stateMux); tlsscEnabled = true;  portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonTlsscDisable() {
    portENTER_CRITICAL(&stateMux); tlsscEnabled = false; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpTlsscRestoreEnable() {
    portENTER_CRITICAL(&stateMux); tlsscRestoreEnabled = true; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpTlsscRestoreDisable() {
    portENTER_CRITICAL(&stateMux); tlsscRestoreEnabled = false; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}

static void httpSummonForceMode() {
    portENTER_CRITICAL(&stateMux);
    forceMode = !forceMode;
    portEXIT_CRITICAL(&stateMux);
    evaluateAutoBlinker();
    server.send(200, "application/json", summonStatsToJson());
}


static void httpDasTelemetryStats() {
  server.send(200, "application/json", dasTelemetryStatsToJson());
}

static void httpBlinkAStats() {
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkAEnable() {
  portENTER_CRITICAL(&blinkAMux);
  blinkAEnabled = true;
  portEXIT_CRITICAL(&blinkAMux);
  evaluateAutoBlinker();
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADisable() {
  portENTER_CRITICAL(&blinkAMux);
  blinkAEnabled = false;
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  oneShotTurn = STALK_IDLE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  blinkATransport = BLINKA_TRANSPORT_NONE;
  activeTurn = STALK_IDLE;
  lastReqDir = 0;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkATransport() {
  int v = server.hasArg("transport") ? server.arg("transport").toInt() : BLINKA_TRANSPORT_249;
  v = constrain(v, BLINKA_TRANSPORT_249, BLINKA_TRANSPORT_3C2);

  portENTER_CRITICAL(&blinkAMux);
  blinkATransportMode = (uint8_t)v;

  // Changing the transport cancels any pending/active automatic pulse so the
  // next request starts cleanly on the newly selected CAN frame type.
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  lastReqDir = 0;
  oneShotTurn = STALK_IDLE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  blinkATransport = BLINKA_TRANSPORT_NONE;
  activeTurn = STALK_IDLE;
  portEXIT_CRITICAL(&blinkAMux);

  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADelays() {
  int triggerMs = server.hasArg("triggerMs") ? server.arg("triggerMs").toInt() : (int)BLINKA_TRIGGER_DELAY_DEFAULT_MS;
  int apMs = server.hasArg("apMs") ? server.arg("apMs").toInt() : (int)BLINKA_AP_ENTRY_DELAY_DEFAULT_MS;
  int laneMs = server.hasArg("laneMs") ? server.arg("laneMs").toInt() : (int)BLINKA_LANE_CHANGE_DELAY_DEFAULT_MS;
  triggerMs = constrain(triggerMs, 0, 30000);
  apMs = constrain(apMs, 0, 30000);
  laneMs = constrain(laneMs, 0, 60000);
  portENTER_CRITICAL(&blinkAMux);
  blinkATriggerDelayMs = (uint32_t)triggerMs;
  blinkAApEntryDelayMs = (uint32_t)apMs;
  blinkALaneChangeDelayMs = (uint32_t)laneMs;
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  lastReqDir = 0;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpLaneChangeBlindspot() {
  int v = server.hasArg("cfg") ? server.arg("cfg").toInt() : 0;
  v = constrain(v, 0, 2);
  portENTER_CRITICAL(&blinkAMux);
  ulcBlindSpotInjectConfig = (uint8_t)v;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADelay() {
  int v = server.hasArg("ms") ? server.arg("ms").toInt() : (int)BLINKA_TRIGGER_DELAY_DEFAULT_MS;
  v = constrain(v, 0, 30000);
  portENTER_CRITICAL(&blinkAMux);
  blinkATriggerDelayMs = (uint32_t)v;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

// Reset only diagnostic/session counters. No NVS/configuration, live feature state,
// CAN liveness timestamps, or mcpRxCount warmup state is modified.
static void resetRuntimeStats() {
  mcpTxOk = 0;
  mcpTxFail = 0;
  chassisRxCount = 0;
  partyRxCount = 0;
  nagParty.resetStats();

  portENTER_CRITICAL(&stateMux);
  sumRxMux1 = 0;
  sumTxOk = 0;
  sumTxFail = 0;
  summonPeriodicTxOk = 0;
  summonPeriodicTxFail = 0;
  lastSummonPeriodicTxMs = 0;						 
  sumRx280 = 0;
  sumRx390 = 0;
  sumRx921 = 0;
  sumRx1016 = 0;
  r79RxMux1 = 0;
  r79RxMux0 = 0;
  r79TxOk = 0;
  r79TxFail = 0;
  summonPriorityTransitions = 0;
  summonPriorityFullEnterCount = 0;
  summonPriorityFullExitCount = 0;
  portEXIT_CRITICAL(&stateMux);

  portENTER_CRITICAL(&blinkAMux);
  rx249 = 0;
  rx3C2 = 0;
  blkATxOk = 0;
  blkATxFail = 0;
  stalklessLastPhysicalTurn = STALK_IDLE;
  stalklessLastPhysicalPressMs = 0;
  stalklessPhysicalPressCount = 0;
  stalklessTxOk = 0;
  stalklessTxFail = 0;
  stalklessManualOverride = 0;
  portEXIT_CRITICAL(&blinkAMux);
  visualDebugRxCount = 0;

  chassisReadStatus();
  partyTxQueueMax = partyTxQueueNow;
  partyRxQueueMax = partyRxQueueNow;
  partyNonSummonShed = 0;
  partyStandbyShed = 0;
  partyFullShed = 0;
  partySummonQueueFlush = 0;
  partySummonRetryOk = 0;
  partySummonRetryFail = 0;
  partySummonTxNormal = 0;
  partySummonTxStandby = 0;
  partySummonTxFull = 0;

  canHardReinitCount = 0;
  canHardReinitFailCount = 0;
  canRecoverySleepCount = 0;
  canRecoveryWakeCount = 0;

  runtimeStatsResetCount++;
  runtimeStatsLastResetMs = (uint32_t)millis();
}

static void httpResetRuntimeStats() {
  resetRuntimeStats();
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"runtime-stats-reset\"}");
}

static void webTask(void *arg) {
  Serial.println("WiFi: Starting AP...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  char ssid[24];
  snprintf(ssid, sizeof(ssid), "TMR-%02X%02X", mac[4], mac[5]);
  while (!WiFi.softAP(ssid, "12345678")) {
    Serial.println("WiFi: Failed to start AP, retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  IPAddress ip = WiFi.softAPIP();
  bootCaptureMarkOnce(&bootCapWifiReadyMs);
  Serial.printf("AP: SSID=%s IP=%s\n", ssid, ip.toString().c_str());

  server.on("/",                  HTTP_GET,  httpRoot);
  server.on("/api/summon/stats",  HTTP_GET,  httpSummonStats);
  server.on("/api/summon/enable", HTTP_POST, httpSummonEnable);
  server.on("/api/summon/disable",HTTP_POST, httpSummonDisable);
  server.on("/api/summon/tlssc-enable",  HTTP_POST, httpSummonTlsscEnable);
  server.on("/api/summon/tlssc-disable", HTTP_POST, httpSummonTlsscDisable);
  server.on("/api/tlssc-restore/enable",  HTTP_POST, httpTlsscRestoreEnable);
  server.on("/api/tlssc-restore/disable", HTTP_POST, httpTlsscRestoreDisable);
  server.on("/api/summon/forcemode", HTTP_POST, httpSummonForceMode);
  server.on("/api/blinkA/stats", HTTP_GET, httpBlinkAStats);
  server.on("/api/blinkA/enable", HTTP_POST, httpBlinkAEnable);
  server.on("/api/blinkA/disable", HTTP_POST, httpBlinkADisable);
  server.on("/api/blinkA/delay", HTTP_POST, httpBlinkADelay);
  server.on("/api/blinkA/delays", HTTP_POST, httpBlinkADelays);
  server.on("/api/laneChange/blindspot", HTTP_POST, httpLaneChangeBlindspot);
   server.on("/api/blinkA/transport", HTTP_POST, httpBlinkATransport);
  server.on("/api/das/stats", HTTP_GET, httpDasTelemetryStats);
  server.on("/api/nag/config", HTTP_GET, httpNagConfig);
  server.on("/api/nag/stats", HTTP_GET, httpNagStats);
  server.on("/api/nag/mode", HTTP_POST, httpNagMode);
  server.on("/api/nag/update", HTTP_POST, httpNagUpdate);
  server.on("/api/nag/reset", HTTP_POST, httpNagReset);
  server.on("/api/system/stats",  HTTP_GET,  httpSystemStats);
  server.on("/api/system/boot-capture.csv", HTTP_GET, httpBootCaptureCsv);
  server.on("/api/system/reset-stats", HTTP_POST, httpResetRuntimeStats);
  server.on("/api/system/reinit-can", HTTP_POST, httpCanHardReinit);
  server.on("/api/system/reboot", HTTP_POST, httpRebootT2Can);
  server.on("/update", HTTP_POST, httpOtaFinish, httpOtaUpload);
  server.begin();

  for (;;) {
    server.handleClient();
    webBeat++;
    vTaskDelay(1);
  }
}

// ═══════════════════════════════════════════════════════════════
// CAN TASKS
// ═══════════════════════════════════════════════════════════════

// Reinitialize the MCP2515 cleanly (reset + bitrate + normal mode).
// Always use the same MCP_CLOCK constant.
static void mcpReinit() {
  Can_A.reset();
  delay(2); // conservative margin beyond MCP2515 128-cycle oscillator startup
  Can_A.setBitrate(CAN_500KBPS, MCP_CLOCK);
  Can_A.setNormalMode();
  mcpTxFailConsecutive = 0;
}

static void partyMcpReinit() {
  partyReady = false;
  Can_Party.reset();
  delay(2);
  const MCP2515::ERROR bitrate = Can_Party.setBitrate(CAN_500KBPS, MCP_CLOCK);
  const MCP2515::ERROR mode = bitrate == MCP2515::ERROR_OK ? Can_Party.setNormalMode() : bitrate;
  partyReady = bitrate == MCP2515::ERROR_OK && mode == MCP2515::ERROR_OK;
  nagParty.partyTransportReset();
  Serial.printf("[J4 PARTY] MCP2515 recovery %s\n", partyReady ? "complete" : "failed");
}

static void handle247OnChassis(const uint8_t *data, uint8_t dlc) {
  if (dlc < 1) return;
  // DBC: SG_ DAS_arbiterBehavior : 2|3@1+ (1,0) [0|4].
  const uint8_t behavior = (data[0] >> 2) & 0x07u;
  bool laneChangeComplete = false;
  portENTER_CRITICAL(&blinkAMux);
  if (dlc >= 8) memcpy(realRaw247, data, 8);
  seen247 = true;
  dasArbiterBehavior = behavior;
  if (behavior == 2 || behavior == 3) { // LANE_CHANGE_LEFT / RIGHT
    dasLaneChangeInProgress = true;
  } else if (behavior == 1 && dasLaneChangeInProgress) { // IN_LANE after a change
    dasLaneChangeInProgress = false;
    dasLaneChangeCompleteCount++;
    laneChangeComplete = true;
  }
  portEXIT_CRITICAL(&blinkAMux);

  if (laneChangeComplete) {
    noteAutoBlinkerLaneChangeComplete((uint32_t)millis());
  }
}

static void sendLaneChangeSoftAbortingLeft247() {
  uint8_t dat[8] = {0};

  portENTER_CRITICAL(&blinkAMux);
  if (seen247) memcpy(dat, realRaw247, 8);
  portEXIT_CRITICAL(&blinkAMux);

  // DAS_alcInternalState: 12|4@1+, raw 10 (0xA) = SOFT_ABORTING_LEFT.
  // Bits 12..15 are the high nibble of byte 1. Preserve all other bits.
  dat[1] = (uint8_t)((dat[1] & 0x0Fu) | 0xA0u);

  PartyFrame out = {};
  out.identifier = 0x247;
  out.data_length_code = 8;
  memcpy(out.data, dat, 8);

  if (chassisTransmit(&out, 0) == ESP_OK) {
    sumTxOk++;
    portENTER_CRITICAL(&blinkAMux);
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
    lastReqDir = 0;
    oneShotTurn = STALK_IDLE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
    blinkATransport = BLINKA_TRANSPORT_NONE;
    activeTurn = STALK_IDLE;
    laneChangeCancelCount++;
    portEXIT_CRITICAL(&blinkAMux);
  } else {
    sumTxFail++;
  }
}

static void sendLaneChangeSnooze3fd() {
  bool ap;
  uint8_t dat[8] = {0};
  bool haveTemplate;

  portENTER_CRITICAL(&stateMux);
  ap = gateAPActive;
  portEXIT_CRITICAL(&stateMux);

  if (!ap)
    return;

  portENTER_CRITICAL(&blinkAMux);
  haveTemplate = seen3fdMux1;
  if (haveTemplate) memcpy(dat, realRaw3fdMux1, 8);
  portEXIT_CRITICAL(&blinkAMux);

  if (!haveTemplate)
    return;

  // 0x3FD mux1: UI_ulcSnooze = bit 36 = 1.
  setBit(dat, 36, true);

  PartyFrame out = {};
  out.identifier = 0x3FD;
  out.data_length_code = 8;
  out.flags = 0;
  memcpy(out.data, dat, 8);

  if (!chassisNonSummonAdmissionOpen()) {
    snoozeTxFail++;
    return;
  }

  esp_err_t err = chassisTransmit(&out, 0);
  if (err == ESP_OK) {
    snoozeTxOk++;
  } else {
    snoozeTxFail++;
  }
}

static void handle102LaneChangeCancel(const uint8_t *data, uint8_t dlc) {
  if (dlc < 4) return;

  // VCLEFT_frontIntSwitchPressed: bit 31, little-endian numbering.
  const bool pressed = ((data[31 / 8] >> (31 % 8)) & 0x01u) != 0;

  bool previous;
  portENTER_CRITICAL(&blinkAMux);
  previous = laneChangeButtonPressed;
  laneChangeButtonPressed = pressed;
  if (pressed) laneChangeButtonRx++;
  portEXIT_CRITICAL(&blinkAMux);

  // Only the rising edge during an active DAS lane-change behavior
  // triggers 0x3FD mux1 UI_ulcSnooze = 1.
  if (!pressed || previous) return;

  const uint8_t behavior = visualBehaviorType;
  if (behavior != 1 && behavior != 2 && behavior != 3) return; // LEFT / RIGHT lane change

  sendLaneChangeSnooze3fd();
}

static void canTaskMcp(void* arg) {
  Serial.println("[CAN A] MCP2515 task started");
  for (;;) {
    canTaskMcpHeartbeatMs = (uint32_t)millis();
    if (canTasksStopping) {
      canTaskMcpQuiesced = true;
      while (canTasksStopping) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskMcpQuiesced = false;
      continue;
    }
    // ── BOUNDED READ LOOP ──
    // Never drain more than MCP_RX_BUDGET frames without yielding the
    // task. If an RX buffer gets stuck (uncleared overflow -> same
    // frame repeated in a loop), the task still exits: no more
    // infinite loop -> no freeze / watchdog.
    struct can_frame rxf;
    uint8_t budget = MCP_RX_BUDGET;
    while (budget-- && Can_A.readMessage(&rxf) == MCP2515::ERROR_OK) {
      lastCanAFrameMs = (uint32_t)millis();
      mcpRxCount++;
      bootCaptureObservePartyFrame((uint16_t)(rxf.can_id & 0x7FF), rxf.can_dlc, rxf.data);
      if (((rxf.can_id & 0x7FF) == LEFTSTALK_ID) && rxf.can_dlc >= 3) {
        handle249OnCanA(rxf.data, rxf.can_dlc);
      }
      if (((rxf.can_id & 0x7FF) == VCLEFT_SWITCH_ID) && rxf.can_dlc >= 8) {
        handle3C2OnCanA(rxf);
      }
      if ((rxf.can_id & 0x7FF) == 0x102 && rxf.can_dlc >= 4) {
        handle102LaneChangeCancel(rxf.data, rxf.can_dlc);
      }
    }

    // ── STATUS CHECK / RECOVERY (1 Hz) ──
    unsigned long now = millis();
    if (now - lastMcpStatusMs >= 1000) {
      lastMcpStatusMs = now;

      // Read the REAL MCP2515 error flags (EFLG register).
      uint8_t eflg = Can_A.getErrorFlags();

      // 1) RX overflow: MUST be cleared, otherwise the controller stops
      //    receiving in this buffer and CAN A reception appears frozen.
      if (eflg & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR)) {
        Can_A.clearRXnOVR();
        Serial.println("[CAN A] RX overflow flags cleared");
      }

      // 2) REAL bus-off via EFLG_TXBO (not only TX failures).
      uint8_t consecutive = mcpTxFailConsecutive;
      bool busOff = (eflg & MCP2515::EFLG_TXBO) || (consecutive > 5);

      if (busOff) {
        mcpState = 2; // BUS-OFF
        if (now - lastMcpRecoverMs > 3000) {
          lastMcpRecoverMs = now;
          Serial.printf("[CAN A] MCP2515 bus-off (eflg=0x%02X txFailSeq=%u), reset...\n",
                        eflg, consecutive);
          mcpReinit();
        }
      } else if (consecutive > 0 || (eflg & (MCP2515::EFLG_TXWAR | MCP2515::EFLG_RXWAR))) {
        mcpState = 1; // Warning
      } else {
        mcpState = 0; // OK
      }
    }

    vTaskDelay(1);
  }
}

static void canTaskChassis(void* arg) {
  Serial.println("[J3 CHASSIS] MCP2515 feature task started");
  unsigned long lastChassisStatusMs = 0;
  for (;;) {
    canTaskChassisHeartbeatMs = (uint32_t)millis();
    if (canTasksStopping) {
      canTaskChassisQuiesced = true;
      while (canTasksStopping) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskChassisQuiesced = false;
      continue;
    }

    PartyFrame f;
    uint8_t rxBudget = 0;
    while (rxBudget < PARTY_RX_DRAIN_BUDGET &&
           chassisReceive(&f, pdMS_TO_TICKS(2)) == ESP_OK) {
      rxBudget++;
      canTaskChassisHeartbeatMs = (uint32_t)millis();
      chassisRxCount++;
      lastCanChassisFrameMs = (uint32_t)millis();
      // CAN B's recovery/feature path is physically J3 CHASSIS.
      lastCanBFrameMs = lastCanChassisFrameMs;
      canAnyFrames++;
      canRxBeat++;
      lastCanFrameMs = millis();
      bootCaptureObserveVhFrame(f.identifier, f.data_length_code);

      // Keep the latest real 0x3FD mux1 frame as a template for AP/ALC snooze.
      if (f.identifier == 0x3FD && f.data_length_code >= 8 && readMuxID(f.data) == 1) {
        portENTER_CRITICAL(&blinkAMux);
        memcpy(realRaw3fdMux1, f.data, 8);
        seen3fdMux1 = true;
        portEXIT_CRITICAL(&blinkAMux);
      }

      switch (f.identifier) {
        case 0x247: // DAS_autopilotDebug / DAS_arbiterBehavior on J3.
          handle247OnChassis(f.data, f.data_length_code);
          break;
        case VISUAL_DEBUG_ID:
          if (f.data_length_code >= 8) {
            visualBehaviorType = (uint8_t)readBitsLE(f.data, 56, 2);
            visualDebugRxCount++;
            visualDebugLastMs = millis();
            evaluateAutoBlinker();
          }
          break;
        case 280:
          if (f.data_length_code >= 7) handle280(f.data);
          break;
        case 390:
          if (f.data_length_code >= 8) handle390(f.data);
          break;
        case 921: // 0x399 DAS state: confirmed on J3 CHASSIS.
          if (f.data_length_code >= 1) handle921(f.data);
          break;
        case DRIVER_ASSIST_ID:
          handle1016(f.data, f.data_length_code);
          break;
        case 0x331:
          doInjectTlsscRestore(f);
          break;
        case 1021: // 0x3FD: confirmed on J3 CHASSIS.
          if (f.data_length_code >= 8) {
            uint8_t mux = readMuxID(f.data);
            if (mux == 1)      injectSummon(f);
            else if (mux == 0) injectTLSSC(f);
          }
          break;
        default:
          break;
      }
    }

    summonPeriodicTick();
    refreshSummonPriorityState();
    chassisReadStatus();
    blinkATxTick();

    const unsigned long now = millis();
    if (now - lastChassisStatusMs >= 1000) {
      lastChassisStatusMs = now;
      if (chassisMcpReady && (Can_Chassis.getErrorFlags() & MCP2515::EFLG_TXBO)) {
        Serial.println("[J3 CHASSIS] MCP2515 bus-off -> reset");
        Can_Chassis.reset();
        delay(2);
        const MCP2515::ERROR bitrate = Can_Chassis.setBitrate(CAN_500KBPS, MCP_CLOCK);
        const MCP2515::ERROR mode = bitrate == MCP2515::ERROR_OK ? Can_Chassis.setNormalMode() : bitrate;
        chassisMcpReady = bitrate == MCP2515::ERROR_OK && mode == MCP2515::ERROR_OK;
      }
    }

    uint32_t nowMs = (uint32_t)millis();
    portENTER_CRITICAL(&stateMux);
    bool can280Stale = (last280Millis > 0) && (nowMs - last280Millis > PARKED_TIMEOUT_MS);
    if (can280Stale) gateParked = true;
    portEXIT_CRITICAL(&stateMux);
    vTaskDelay(1);
  }
}

static void canTaskParty(void* arg) {
  Serial.println("[J4 PARTY] MCP2515 Nag Killer task started");
  unsigned long lastPartyStatusMs = 0;
  unsigned long lastNoCanWarn = 0;

  for (;;) {
    canTaskPartyHeartbeatMs = (uint32_t)millis();
    if (canTasksStopping) {
      canTaskPartyQuiesced = true;
      while (canTasksStopping) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskPartyQuiesced = false;
      continue;
    }
    PartyFrame f;
    uint8_t rxBudget = 0;
    while (rxBudget < PARTY_RX_DRAIN_BUDGET &&
           partyReceive(&f, pdMS_TO_TICKS(2)) == ESP_OK) {
      rxBudget++;
      // Keep the supervisor heartbeat alive even under sustained J4 traffic.
      canTaskPartyHeartbeatMs = (uint32_t)millis();
      lastCanPartyFrameMs = (uint32_t)millis();
      partyRxCount++;
      canAnyFrames++;
      canRxBeat++;
      lastCanFrameMs = millis();
      // New.ino's configurable torque echo is attached only to J4 PARTY.
      nagParty.process(f);
      bootCaptureObserveVhFrame(f.identifier, f.data_length_code);
    }

    // J4 MCP2515 health check: EFLG_TXBO is the physical bus-off indicator.
    const unsigned long now = millis();
    if (now - lastPartyStatusMs >= 1000) {
      lastPartyStatusMs = now;
      if (partyReady && (Can_Party.getErrorFlags() & MCP2515::EFLG_TXBO)) {
        Serial.println("[J4 PARTY] MCP2515 bus-off -> reset");
        partyMcpReinit();
      }
    }

    // No-CAN warning (shared counter)
    if ((millis() - bootTime) > 20000 && canAnyFrames == 0) {
      if (millis() - lastNoCanWarn > 5000) {
        Serial.println("No CAN frames yet on either bus, staying alive.");
        lastNoCanWarn = millis();
      }
    }

    // Summon watchdog: if CAN 280 silent > PARKED_TIMEOUT_MS
    uint32_t nowMs = (uint32_t)millis();
    portENTER_CRITICAL(&stateMux);
    bool can280Stale = (last280Millis > 0) && (nowMs - last280Millis > PARKED_TIMEOUT_MS);
    if (can280Stale) gateParked = true;
    portEXIT_CRITICAL(&stateMux);

    vTaskDelay(1);
  }
}


// ═══════════════════════════════════════════════════════════════
// RECOVERY-ONLY CAN SUBSYSTEM SUPERVISOR
// ═══════════════════════════════════════════════════════════════

static inline bool recoveryFresh(uint32_t now, uint32_t ts, uint32_t timeoutMs) {
  return ts != 0 && (uint32_t)(now - ts) <= timeoutMs;
}

static void requestCanSubsystemRestart(uint8_t reason) {
  portENTER_CRITICAL(&canRecoveryMux);
  if (reason > canSupervisorCommand) canSupervisorCommand = reason;
  portEXIT_CRITICAL(&canRecoveryMux);
}

static bool initMcp2515(MCP2515 &controller, uint8_t cs, uint8_t stby, const char *name) {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  // TMR transceivers use an active-high standby input: LOW enables CAN.
  pinMode(stby, OUTPUT);
  digitalWrite(stby, LOW);
  controller.reset();
  delay(2);
  const MCP2515::ERROR rateErr = controller.setBitrate(CAN_500KBPS, MCP_CLOCK);
  const MCP2515::ERROR modeErr = rateErr == MCP2515::ERROR_OK ? controller.setNormalMode() : rateErr;
  const bool ok = rateErr == MCP2515::ERROR_OK && modeErr == MCP2515::ERROR_OK;
  if (!ok) Serial.printf("[%s] MCP2515 init failed: bitrate=%d mode=%d\n", name, (int)rateErr, (int)modeErr);
  return ok;
}

static bool recoveryMcpColdInit() {
  mcpReady = false;
  chassisMcpReady = false;
  partyReady = false;
  if (mcpSpiStarted) {
    SPI.end();
    mcpSpiStarted = false;
    delay(20);
  }

  SPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, MCP2515_BODY_CS);
  mcpSpiStarted = true;
  delay(20);

  mcpReady = initMcp2515(Can_A, MCP2515_BODY_CS, MCP2515_BODY_STBY, "J2 BODY");
  chassisMcpReady = initMcp2515(Can_Chassis, MCP2515_CHASSIS_CS, MCP2515_CHASSIS_STBY, "J3 CHASSIS");
  partyReady = initMcp2515(Can_Party, MCP2515_PARTY_CS, MCP2515_PARTY_STBY, "J4 PARTY");
  nagParty.partyTransportReset();
  const bool ok = mcpReady && chassisMcpReady && partyReady;
  if (mcpReady) {
    mcpTxFailConsecutive = 0;
    mcpState = 0;
  } else {
    mcpState = 2;
  }
  return ok;
}

static bool recoveryPartyFullReinit() {
  // Kept as a named call site for the existing recovery supervisor.  The
  // physical work is performed by recoveryMcpColdInit() for all three buses.
  return partyReady;
}

static void recoveryStopCanTasks() {
  canTasksStopping = true;
  canTaskMcpQuiesced = false;
  canTaskPartyQuiesced = false;
  canTaskChassisQuiesced = false;

  uint32_t start = (uint32_t)millis();
  while ((!canTaskMcpQuiesced || !canTaskPartyQuiesced || !canTaskChassisQuiesced) &&
         (uint32_t)((uint32_t)millis() - start) < 300) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  TaskHandle_t a = canTaskMcpHandle;
  TaskHandle_t b = canTaskPartyHandle;
  TaskHandle_t c = canTaskChassisHandle;
  canTaskMcpHandle = nullptr;
  canTaskPartyHandle = nullptr;
  canTaskChassisHandle = nullptr;
  if (a) vTaskDelete(a);
  if (b) vTaskDelete(b);
  if (c) vTaskDelete(c);

  canTasksStopping = false;
  canTaskMcpQuiesced = false;
  canTaskPartyQuiesced = false;
  canTaskChassisQuiesced = false;
  canTaskMcpHeartbeatMs = 0;
  canTaskPartyHeartbeatMs = 0;
  canTaskChassisHeartbeatMs = 0;
  vTaskDelay(pdMS_TO_TICKS(RECOVERY_TASK_STOP_SETTLE_MS));
}

static bool recoveryStartCanTasks() {
  BaseType_t a = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, &canTaskMcpHandle, 1);
  if (a != pdPASS) {
    canTaskMcpHandle = nullptr;
    return false;
  }
  BaseType_t b = xTaskCreatePinnedToCore(canTaskChassis, "canChassis", 4096, nullptr, 4, &canTaskChassisHandle, 1);
  if (b != pdPASS) {
    vTaskDelete(canTaskMcpHandle);
    canTaskMcpHandle = nullptr;
    canTaskChassisHandle = nullptr;
    return false;
  }
  BaseType_t c = xTaskCreatePinnedToCore(canTaskParty, "canParty", 8192, nullptr, 4, &canTaskPartyHandle, 1);
  if (c != pdPASS) {
    vTaskDelete(canTaskMcpHandle);
    vTaskDelete(canTaskChassisHandle);
    canTaskMcpHandle = nullptr;
    canTaskChassisHandle = nullptr;
    canTaskPartyHandle = nullptr;
    return false;
  }
  return true;
}

static bool recoveryHardReinitialize(uint8_t reason) {
  if (canSubsystemBusy) return false;
  canSubsystemBusy = true;
  const int8_t bootCapHardIdx = bootCaptureHardStart(reason);
  canLastHardReinitReason = reason;
  canHardReinitCount++;
  Serial.printf("[CAN SUP] hard CAN reinitialize #%lu reason=%u\n",
                (unsigned long)canHardReinitCount, (unsigned)reason);

  recoveryStopCanTasks();
  // Fail closed immediately during CAN recovery; a previously latched state=5
  // must never authorize a new Auto Blinker request after bus interruption.
  portENTER_CRITICAL(&stateMux);
  lastDASStatusMillis = 0;
  portEXIT_CRITICAL(&stateMux);
  portENTER_CRITICAL(&blinkAMux);
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  oneShotTurn = STALK_IDLE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  blinkATransport = BLINKA_TRANSPORT_NONE;
  activeTurn = STALK_IDLE;
  lastReqDir = 0;
  portEXIT_CRITICAL(&blinkAMux);
  bool aOk = recoveryMcpColdInit();
  bool bOk = recoveryPartyFullReinit();
  bool tasksOk = aOk && bOk && recoveryStartCanTasks();

  lastCanAFrameMs = 0;
  lastCanBFrameMs = 0;
  canInitTime = millis();
  recoveryOneBusStaleStartMs = 0;
  recoveryWakeAcquireStartMs = (reason == CAN_SUP_HARD_ACQUIRE || recoveryEverBothActive)
                                 ? (uint32_t)millis() : 0;
  canSubsystemBusy = false;

  if (!(aOk && bOk && tasksOk)) {
    bootCaptureHardFinish(bootCapHardIdx, false);
    canHardReinitFailCount++;
    Serial.printf("[CAN SUP] hard CAN reinitialize FAILED A=%u B=%u tasks=%u\n",
                  aOk ? 1 : 0, bOk ? 1 : 0, tasksOk ? 1 : 0);
    return false;
  }
  bootCaptureHardFinish(bootCapHardIdx, true);
  Serial.println("[CAN SUP] hard CAN reinitialize complete");
  return true;
}

static void canSupervisorTask(void* arg) {
  Serial.println("[CAN SUP] recovery-only supervisor started");
  for (;;) {
    uint32_t now = (uint32_t)millis();

    if (!canSubsystemBusy) {
      // Independent task heartbeat: still advances while the vehicle is asleep.
      // Therefore silence on the CAN wires is not confused with a wedged task.
      bool graceDone = (uint32_t)(now - canInitTime) >= RECOVERY_TASK_START_GRACE_MS;
      bool aTaskDead = canTaskMcpHandle && graceDone &&
                       (canTaskMcpHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskMcpHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      bool bTaskDead = canTaskPartyHandle && graceDone &&
                       (canTaskPartyHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskPartyHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      bool cTaskDead = canTaskChassisHandle && graceDone &&
                       (canTaskChassisHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskChassisHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      if (aTaskDead || bTaskDead || cTaskDead) {
        Serial.printf("[CAN SUP] task heartbeat stale J2=%u J4=%u J3=%u\n",
                      aTaskDead ? 1 : 0, bTaskDead ? 1 : 0, cTaskDead ? 1 : 0);
        requestCanSubsystemRestart(CAN_SUP_HARD_STALE);
      }

      const bool aFresh = recoveryFresh(now, lastCanAFrameMs, RECOVERY_BUS_FRESH_MS);
      const bool bFresh = recoveryFresh(now, lastCanBFrameMs, RECOVERY_BUS_FRESH_MS);
      const bool anyFresh = aFresh || bFresh;

      // Traffic silence is normal on these vehicle networks and must never
      // reset J4 PARTY. Each MCP2515 already has its own real bus-off recovery
      // in its RX task. The global restart remains available for a dead task
      // above and the explicit dashboard action only.
      if (aFresh && bFresh) {
        if (!recoveryEverBothActive || recoverySleeping) Serial.println("[CAN SUP] J2+J3 active");
        recoveryEverBothActive = true;
        recoverySleeping = false;
        recoveryLastBothActiveMs = now;
      } else if (!anyFresh && recoveryEverBothActive && !recoverySleeping &&
                 recoveryLastBothActiveMs != 0 &&
                 (uint32_t)(now - recoveryLastBothActiveMs) >= RECOVERY_SLEEP_QUIET_MS) {
        recoverySleeping = true;
        canRecoverySleepCount++;
        Serial.printf("[CAN SUP] vehicle CAN quiet #%lu -> passive wait\n",
                      (unsigned long)canRecoverySleepCount);
      } else if (anyFresh && recoverySleeping) {
        recoverySleeping = false;
        canRecoveryWakeCount++;
        Serial.printf("[CAN SUP] vehicle CAN wake #%lu\n", (unsigned long)canRecoveryWakeCount);
      }
    }

    uint8_t cmd = CAN_SUP_NONE;
    portENTER_CRITICAL(&canRecoveryMux);
    cmd = canSupervisorCommand;
    canSupervisorCommand = CAN_SUP_NONE;
    portEXIT_CRITICAL(&canRecoveryMux);

    if (cmd != CAN_SUP_NONE && !canSubsystemBusy) {
      if (!recoveryHardReinitialize(cmd)) {
        Serial.println("[CAN SUP] subsystem recovery failed -> reboot T-2CAN");
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ═══════════════════════════════════════════════════════════════
// SETUP / LOOP
// ═══════════════════════════════════════════════════════════════

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  delay(100); // rev.14: serial settle only; CAN startup is not held here

  rtcBootCount++;
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.printf("\n=== TMR BOOT ===\n");
  Serial.printf("Reset reason: %d (%s)\n", reset_reason, resetReasonName(reset_reason));
  Serial.printf("RTC boot count: %lu\n", (unsigned long)rtcBootCount);
  if (reset_reason == ESP_RST_BROWNOUT) {
    Serial.println("WARNING: Brownout detected!");
  }
  Serial.printf("IDF version: %s\n", esp_get_idf_version());

  // NVS init
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("NVS: Corrupted, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    Serial.printf("NVS: Init failed %d\n", err);
  }

  // Load configs
  summonCfgLoad();

  Serial.printf("Summon enabled=%s\n", summonEnabled ? "true" : "false");
  Serial.printf("TLSSC enabled=%s (0x3FD mux0 bit38)\n", tlsscEnabled ? "true" : "false");
  Serial.printf("TLSSC Restore enabled=%s (0x331 DAS_autopilotConfig)\n", tlsscRestoreEnabled ? "true" : "false");

  // rev.14: board power-on is treated as the wake signal for RX.
  // Existing per-feature validity gates still control every injection/TX path.
  Serial.println("Driver-wake power detected. Starting CAN init immediately...");

  Serial.println("Initializing J2 BODY, J3 CHASSIS and J4 PARTY MCP2515 controllers...");
  if (!recoveryMcpColdInit()) {
    Serial.println("MCP2515 initialization failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }
  nagParty.begin();

  canInitTime = millis();
  bootCaptureMarkOnce(&bootCapCanInitDoneMs);

  // Start CAN tasks immediately after both controllers are ready/running.
  BaseType_t retMcp = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, &canTaskMcpHandle, 1);
  if (retMcp != pdPASS) {
    Serial.printf("CAN A task creation failed: %d\n", retMcp);
    delay(3000);
    ESP.restart();
  }

  BaseType_t retChassis = xTaskCreatePinnedToCore(canTaskChassis, "canChassis", 4096, nullptr, 4, &canTaskChassisHandle, 1);
  if (retChassis != pdPASS) {
    Serial.printf("J3 CHASSIS task creation failed: %d\n", retChassis);
    delay(3000);
    ESP.restart();
  }

  BaseType_t retParty = xTaskCreatePinnedToCore(canTaskParty, "canParty", 8192, nullptr, 4, &canTaskPartyHandle, 1);
  if (retParty != pdPASS) {
    Serial.printf("CAN B task creation failed: %d\n", retParty);
    delay(3000);
    ESP.restart();
  }
  bootCaptureMarkOnce(&bootCapCanTasksStartedMs);

  BaseType_t retSup = xTaskCreatePinnedToCore(canSupervisorTask, "canSup", 6144, nullptr, 3, &canSupervisorHandle, 0);
  if (retSup != pdPASS) {
    Serial.printf("CAN supervisor task creation failed: %d\n", retSup);
    delay(3000);
    ESP.restart();
  }

  Serial.printf("[BOOT] CAN RX tasks started at %lu ms\n", (unsigned long)(millis() - bootTime));

  // Start Wi-Fi/web after the CAN receive path is live.
  BaseType_t retWeb = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);
  if (retWeb != pdPASS) {
    Serial.printf("Web task creation failed: %d\n", retWeb);
    delay(3000);
    ESP.restart();
  }

  Serial.println("BOOT OK");
}

void loop() {
  static unsigned long lastBeatLog = 0;
  static uint32_t loopBeat = 0;
  loopBeat++;
  unsigned long now = millis();

  if (now - lastBeatLog >= 5000) {
    lastBeatLog = now;
    unsigned long canAgeMs = (lastCanFrameMs == 0) ? 999999 : (now - lastCanFrameMs);
    Serial.printf(
      "[BEAT] uptime=%lu loop=%lu canBeat=%lu canRxBeat=%lu webBeat=%lu canFrames=%lu canAgeMs=%lu mcpTxOk=%lu mcpTxFail=%lu sumTxOk=%lu sumTxFail=%lu heap=%u\n",
      now / 1000,
      (unsigned long)loopBeat,
      (unsigned long)canBeat,
      (unsigned long)canRxBeat,
      (unsigned long)webBeat,
      (unsigned long)canAnyFrames,
      canAgeMs,
      (unsigned long)mcpTxOk,
      (unsigned long)mcpTxFail,
      (unsigned long)sumTxOk,
      (unsigned long)sumTxFail,
      ESP.getFreeHeap()
    );
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}
