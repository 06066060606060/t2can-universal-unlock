#pragma once

// VEHICLE FEATURE LOGIC / R79 / SUMMON / ALC / VH RECORDER
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// R79 policy + bit18 LAB
// 0x3FD mux1 bit19 (UI_applyEceR79) is now a fixed policy value of 0.
// 0x3FD mux1 bit47 (UI_hardCoreSummon) is now a fixed policy value of 1.
// bit18 remains an independent live-test candidate for UI_applyEceR79SmartSummonOnly.
// Injection is allowed only for a valid latched AP ACTIVE state, confirmed Summon, or fresh Park.
// Park is a fixed production gate source; it is not a LAB option. Manual driving fails closed.
// // ═══════════════════════════════════════════════════════════════
static constexpr uint8_t R79LAB_STOCK = 0;
static constexpr uint8_t R79LAB_FORCE_0 = 1;
static constexpr uint8_t R79LAB_FORCE_1 = 2;
static constexpr uint8_t R79LAB_APPLY_ECE_BIT = 19;
static constexpr bool R79_POLICY_APPLY_ECE_VALUE = false;   // bit19 = FORCE 0
static constexpr bool R79_POLICY_HARD_CORE_VALUE = true;    // bit47 = FORCE 1
static constexpr uint8_t R79LAB_SMART_SUMMON_CANDIDATE_BIT = 18;
static constexpr uint8_t R79LAB_HARD_CORE_SUMMON_BIT = 47;
static constexpr uint16_t R79LAB_DEFAULT_PERIOD_MS = 500;
static constexpr uint8_t R79LAB_GATE_BLOCKED = 0;
static constexpr uint8_t R79LAB_GATE_AP = 1;
static constexpr uint8_t R79LAB_GATE_SUMMON = 2;
static constexpr uint8_t R79LAB_GATE_PARK = 3;
static constexpr uint8_t R79LAB_TX_NONE = 0;
static constexpr uint8_t R79LAB_TX_IMMEDIATE = 1;
static constexpr uint8_t R79LAB_TX_PERIODIC = 2;
static portMUX_TYPE r79LabMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t r79LabSmartMode = R79LAB_STOCK;
static volatile uint16_t r79LabPeriodMs = R79LAB_DEFAULT_PERIOD_MS;
static volatile uint32_t r79LabLastAttemptMs = 0;
static volatile uint32_t r79LabLastTxMs = 0;
static volatile uint32_t r79LabNoTemplateSkip = 0;
static volatile uint32_t r79LabQueueSkip = 0;
static volatile uint32_t r79LabBit47Changes = 0;
static volatile uint32_t r79LabBit18Rx0 = 0;
static volatile uint32_t r79LabBit18Rx1 = 0;
static volatile uint32_t r79LabBit19Rx0 = 0;
static volatile uint32_t r79LabBit19Rx1 = 0;
static volatile uint32_t r79LabBit47Rx0 = 0;
static volatile uint32_t r79LabBit47Rx1 = 0;
static volatile uint32_t r79LabImmediateTxOk = 0;
static volatile uint32_t r79LabImmediateTxFail = 0;
static volatile uint32_t r79LabPeriodicTxOk = 0;
static volatile uint32_t r79LabPeriodicTxFail = 0;
static volatile uint8_t r79LabLastTxKind = R79LAB_TX_NONE;
static volatile bool r79LabLastTxValid = false;
static volatile uint8_t r79LabLastGateReason = R79LAB_GATE_BLOCKED;
static volatile bool r79LabStockValid = false;
static volatile uint8_t r79LabStockApply = 0;
static volatile uint8_t r79LabStockSmart = 0;
static volatile uint8_t r79LabStockHardCore = 0;
static volatile uint8_t r79LabEffectiveApply = 0;
static volatile uint8_t r79LabEffectiveSmart = 0;
static volatile uint8_t r79LabEffectiveHardCore = 0;
static volatile uint32_t r79LabLast3fdMs = 0;
static volatile uint32_t r79Lab3fdRx = 0;
static volatile uint32_t r79LabBit18Changes = 0;
static volatile uint32_t r79LabBit19Changes = 0;
static volatile uint32_t r79LabTxOk = 0;
static volatile uint32_t r79LabTxFail = 0;
static volatile uint32_t r79LabGateBlocked = 0;
static volatile uint32_t r79LabAppliedFrames = 0;
static uint8_t r79LabLastStockRaw[8] = {0};
static uint8_t r79LabLastEffectiveRaw[8] = {0};
static volatile bool r79Lab7ffSeen = false;
static volatile uint8_t r79Lab7ffBus = T2CAN_BUS_VH;
static volatile uint8_t r79Lab7ffPage = 0xFF;
static volatile uint32_t r79Lab7ffRx = 0;
static volatile uint32_t r79Lab7ffLastMs = 0;
static uint8_t r79Lab7ffRaw[8] = {0};

// Arduino .ino auto-prototype generation can place callers ahead of later
// definitions. Keep the R79 gate/immediate path explicit and deterministic.
static uint8_t r79LabGateReason(uint32_t now);
static void r79LabImmediateFromStock(const uint8_t *data, uint8_t dlc, uint32_t now);


// ═══════════════════════════════════════════════════════════════
// SUMMON MONITOR / VH OVERLAYS (CAN B - TWAI)
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




static bool r79LabApGateOpen(uint32_t now);

static bool r79LabPeriodValid(uint16_t ms) {
  return ms == 20 || ms == 100 || ms == 250 || ms == 500 || ms == 1000;
}

static const char* r79LabGateReasonName(uint8_t reason) {
  switch (reason) {
    case R79LAB_GATE_AP: return "AP";
    case R79LAB_GATE_SUMMON: return "SUMMON";
    case R79LAB_GATE_PARK: return "PARK";
    default: return "BLOCKED";
  }
}

static void r79LabObserve3fdMux1(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 8) return;
  const uint32_t now = (uint32_t)millis();
  const uint8_t smart = getBit(data, R79LAB_SMART_SUMMON_CANDIDATE_BIT) ? 1 : 0;
  const uint8_t apply = getBit(data, R79LAB_APPLY_ECE_BIT) ? 1 : 0;
  const uint8_t hard = getBit(data, R79LAB_HARD_CORE_SUMMON_BIT) ? 1 : 0;
  portENTER_CRITICAL(&r79LabMux);
  if (r79LabStockValid) {
    if (smart != r79LabStockSmart) { r79LabBit18Changes++; }
    if (apply != r79LabStockApply) { r79LabBit19Changes++; }
    if (hard != r79LabStockHardCore) { r79LabBit47Changes++; }
  }
  if (smart) r79LabBit18Rx1++; else r79LabBit18Rx0++;
  if (apply) r79LabBit19Rx1++; else r79LabBit19Rx0++;
  if (hard)  r79LabBit47Rx1++; else r79LabBit47Rx0++;
  r79LabStockValid = true;
  r79LabStockSmart = smart;
  r79LabStockApply = apply;
  r79LabStockHardCore = hard;
  r79LabLast3fdMs = now;
  r79Lab3fdRx++;
  memcpy(r79LabLastStockRaw, data, 8);
  // Do NOT overwrite LAST TX here. Stock RX and our last injected frame are
  // deliberately independent so the LAB UI cannot flicker STOCK -> STOCK
  // between successful reassertions.
  portEXIT_CRITICAL(&r79LabMux);

  // Event-driven reassertion: while an AP/Summon/Park gate is open, every fresh
  // stock mux1 frame is immediately copied, fixed bit19/47 policy is applied,
  // optional bit18 LAB override is applied, and the frame is transmitted once. The periodic
  // scheduler then fills the interval until the next stock frame.
  r79LabImmediateFromStock(data, dlc, now);
}

static bool r79LabApplySelectedBits(uint8_t *data) {
  if (!data) return false;
  uint8_t smartMode;
  portENTER_CRITICAL(&r79LabMux);
  smartMode = r79LabSmartMode;
  portEXIT_CRITICAL(&r79LabMux);

  // Fixed production R79 policy, validated by vehicle testing.
  setBit(data, R79LAB_APPLY_ECE_BIT, R79_POLICY_APPLY_ECE_VALUE);
  setBit(data, R79LAB_HARD_CORE_SUMMON_BIT, R79_POLICY_HARD_CORE_VALUE);

  // bit18 remains experimental and is the only user-selectable R79 bit.
  if (smartMode != R79LAB_STOCK) {
    setBit(data, R79LAB_SMART_SUMMON_CANDIDATE_BIT, smartMode == R79LAB_FORCE_1);
  }
  return true;
}

static void r79LabRecordTxResult(bool ok, const twai_message_t &out, uint8_t txKind) {
  portENTER_CRITICAL(&r79LabMux);
  if (ok) r79LabTxOk++; else r79LabTxFail++;
  if (txKind == R79LAB_TX_IMMEDIATE) {
    if (ok) r79LabImmediateTxOk++; else r79LabImmediateTxFail++;
  } else if (txKind == R79LAB_TX_PERIODIC) {
    if (ok) r79LabPeriodicTxOk++; else r79LabPeriodicTxFail++;
  }
  if (ok && out.data_length_code >= 8) {
    r79LabLastTxValid = true;
    r79LabLastTxKind = txKind;
    r79LabEffectiveSmart = getBit(out.data, R79LAB_SMART_SUMMON_CANDIDATE_BIT) ? 1 : 0;
    r79LabEffectiveApply = getBit(out.data, R79LAB_APPLY_ECE_BIT) ? 1 : 0;
    r79LabEffectiveHardCore = getBit(out.data, R79LAB_HARD_CORE_SUMMON_BIT) ? 1 : 0;
    memcpy(r79LabLastEffectiveRaw, out.data, 8);
  }
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79LabObserve7ff(uint8_t bus, uint8_t dlc, const uint8_t *data) {
  if (!data || dlc < 1) return;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  portENTER_CRITICAL(&r79LabMux);
  r79Lab7ffSeen = true;
  r79Lab7ffBus = bus;
  r79Lab7ffPage = data[0];
  r79Lab7ffRx++;
  r79Lab7ffLastMs = (uint32_t)millis();
  memset(r79Lab7ffRaw, 0, sizeof(r79Lab7ffRaw));
  memcpy(r79Lab7ffRaw, data, n);
  portEXIT_CRITICAL(&r79LabMux);
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
// Model YL NOA discriminator. Keep the legacy 3-bit AP decoder above
// unchanged for NAG and the existing AP gate; Auto Blinker alone uses the
// full low nibble so ACTIVE_NAV (5) can be gated independently.
static inline uint8_t readDASState4(const uint8_t *data) {
    return data[0] & 0x0F;
}
// Public Tesla DBCs map DAS_autoLaneChangeState to little-endian bit 46, len 5
// in DAS_status (0x399 / decimal 921). For an 8-byte frame this is:
//   data[5] bits 6..7 -> state bits 0..1
//   data[6] bits 0..2 -> state bits 2..4
static inline uint8_t readDASAutoLaneChangeState(const uint8_t *data) {
    return das399ReadAlcPure(data);
}

static volatile bool forceMode = false;
static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool tlsscEnabled  = false;   // "Enable TLSSC" - off by default
static volatile bool tlsscHighwayGateEnabled = false; // experimental 0x238 controlled-access gate
static volatile bool gateAPActive  = false;
static volatile bool gateNOAActive = false;   // raw DAS_autopilotState == ACTIVE_NAV (5)
static volatile uint8_t dasAutopilotState4 = 0xFF; // low nibble of Party-CAN 0x399 byte0
static volatile bool dasAutopilotStateValid = false; // latched until CAN A recovery / next valid 0x399
static volatile uint8_t dasAutoLaneChangeState = 0xFF; // 0x399 bit46..50, 6=L, 7=R, 8=BOTH
static volatile uint8_t dasAutoLaneChangePrevState = 0xFF;
static volatile uint32_t dasAutoLaneChangeLastChangeMs = 0;
static volatile uint32_t dasAutoLaneChangeChangeCount = 0;
static volatile bool dasAutoLaneChangeStateValid = false;
static volatile uint32_t lastDASStatusMillis = 0;  // diagnostic age only; not a functional AP timeout

static bool r79LabApGateOpen(uint32_t now) {
  (void)now;
  uint8_t state4;
  bool valid;
  portENTER_CRITICAL(&stateMux);
  state4 = dasAutopilotState4;
  valid = dasAutopilotStateValid;
  portEXIT_CRITICAL(&stateMux);
  return dasStateApActivePure(valid, state4);
}

// AP/NOA is a latched state, not a periodic command. Keep the last valid Party
// 0x399 state until CAN A itself crosses a recovery boundary. The timestamp is
// retained for diagnostics only; transient planner requests keep their own
// freshness gates.
static bool autoBlinkerNOAGateOpen(uint32_t now, uint32_t* ageOut = nullptr) {
  bool valid;
  uint8_t state4;
  uint32_t last;
  portENTER_CRITICAL(&stateMux);
  valid = dasAutopilotStateValid;
  state4 = dasAutopilotState4;
  last = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  const uint32_t age = (last == 0) ? UINT32_MAX : (uint32_t)(now - last);
  if (ageOut) *ageOut = age;
  return dasStateNoaPure(valid, state4);
}
static volatile bool gateParked    = false;
static volatile bool gateSummoning = false;
static volatile bool sprSeen  = false;
static volatile bool lastAca  = false;
#define PARKED_TIMEOUT_MS  5000
static volatile uint32_t last280Millis = 0;

// Brief, conditional continuity grace for an already-open Summon gate.
// This does NOT create a Summon session. It only keeps the effective injection
// gate open for a short period after a confirmed gateSummoning session loses ACA.
// Repeated ACA-low frames never extend the deadline.
static constexpr uint32_t SUMMON_GATE_DROPOUT_GRACE_MS = 300;
static volatile uint32_t summonGateGraceUntilMs = 0;
static volatile uint32_t summonGateGraceEnterCount = 0;
static volatile uint32_t summonGateGraceRecoverCount = 0;
static volatile uint32_t summonGateGraceExpireCount = 0;

// Summon Monitor is always active. It observes state/capture/priority only;
// R79 bit18/bit19/bit47 remain exclusively controlled and persisted by R79 LAB.

// Summon TX priority is intentionally separate from the Original feature gate.
// It never writes gateParked/gateSummoning/lastAca/sprSeen/forceMode.
// NORMAL        : driving / no fresh confirmed Park. No priority shedding/flush.
// PARK_STANDBY  : fresh Park confirmed. Reserve queue headroom for a Summon start.
// SUMMON_FULL   : Summon session confirmed. Summon owns CAN B TX priority.
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
static volatile uint32_t sumRx280    = 0;
static volatile uint32_t sumRx390    = 0;
static volatile uint32_t sumRx921    = 0;
static volatile uint32_t sumRx1016   = 0;
static char gateBlockReason[48] = "boot";
// ═══════════════════════════════════════════════════════════════
// ADVANCED EAP — MODEL YL SPLIT-BUS ROUTING
//   CAN A / MCP2515 / Party CAN : RX DAS_visualDebug.behaviorType (0x24A, DLC 8)
//   CAN B / TWAI / VH CAN      : RX/TX SCCM_turnIndicatorStalkStatus (0x249, DLC 4)
//   CAN B / TWAI / VH CAN      : RX-only UI_driverAssistControl (0x3F8) for SPR / passive telemetry
//
// Model YL also carries numeric ID 0x24A on VH CAN with DLC 4. That is a
// different bus-local frame and MUST NOT be used as DAS_visualDebug.
// ═══════════════════════════════════════════════════════════════

#define LEFTSTALK_ID      0x249
#define VISUAL_DEBUG_ID   0x24A
#define DRIVER_ASSIST_ID  0x3F8
#define UI_POWERTRAIN_ID   0x334
#define VCLEFT_SWITCH_ID   0x3C2
#define DOOR_SWITCH_ID     0x102
#define VCLEFT_SWITCH_MUX1 1
#define VCLEFT_SWITCH_SNA  0
#define VCLEFT_SWITCH_OFF  1
#define VCLEFT_SWITCH_ON   2

#define STALK_IDLE    0
#define STALK_UP_1    2
#define STALK_DOWN_1  6  // Model YL stock capture: left soft stalk

#define BLINKA_TX_PERIOD_MS         20
#define BLINKA_PULSE_MS             350
#define BLINKA_AUTO_DELAY_DEFAULT_MS 2000
#define BLINKA_STALKLESS_PRESS_MS 300
#define BLINKA_STALKLESS_RELEASE_MS 200

static portMUX_TYPE blinkAMux = portMUX_INITIALIZER_UNLOCKED;

// Real SCCM diagnostics and rolling-counter alignment.
static volatile uint32_t rx249 = 0;
static volatile uint8_t realCounter = 0;
static volatile uint8_t realTurn = 0;
static volatile uint8_t realCksum = 0;
static volatile bool cksumSelfTest = true;
static volatile bool seen249 = false;
static volatile uint8_t blinkACounter = 0;
static volatile uint8_t realDlc = 0;
static volatile uint32_t last249Ms = 0;
static uint8_t realRaw249[8] = {0};
static volatile bool seen3C2 = false;
static volatile uint32_t rx3C2 = 0;
static volatile uint32_t last3C2Mux1Ms = 0;
static volatile uint8_t real3C2LeftButton = VCLEFT_SWITCH_SNA;
static volatile uint8_t real3C2RightButton = VCLEFT_SWITCH_SNA;
static volatile uint32_t stalklessTxOk = 0;
static volatile uint32_t stalklessTxFail = 0;
static volatile bool doorButtonPressed = false;
static volatile uint32_t doorButtonRx = 0;
static volatile uint32_t doorCancelAccepted = 0;
static volatile uint32_t doorCancelBlocked = 0;


// Auto blinker state.
static volatile bool blinkAEnabled = false;
static volatile uint8_t activeTurn = STALK_IDLE;
static volatile uint8_t lastReqDir = 0;
// Keep delayed trigger timing separate from pulse lifetime
// and the active SCCM one-shot pulse. 0x24A state changes may cancel a
// pending trigger, but must not truncate an already-started 350 ms pulse.
static volatile uint8_t oneShotTurn = STALK_IDLE;
static volatile uint32_t oneShotUntil = 0;
static volatile uint32_t oneShotReleaseAt = 0;
static volatile uint8_t autoPendingDir = 0;
static volatile uint32_t blinkADelayMs = BLINKA_AUTO_DELAY_DEFAULT_MS;
static volatile uint32_t autoFireAt = 0;
static volatile bool autoArmed = false;
static volatile uint32_t blkATxOk = 0;
static volatile uint32_t blkATxFail = 0;

// CAN B telemetry used by the auto blinker.
static volatile uint8_t visualBehaviorType = 0;
static volatile uint32_t visualDebugRxCount = 0;
static volatile uint32_t visualDebugLastMs = 0;

// Auto Blinker requires the current 0x24A planner request to remain fresh
// through both ARM and FIRE. 0x399 ALC availability is also direction-gated:
//   LEFT  -> ALC_AVAILABLE_ONLY_L (6) or ALC_AVAILABLE_BOTH (8)
//   RIGHT -> ALC_AVAILABLE_ONLY_R (7) or ALC_AVAILABLE_BOTH (8)
// ALC_IN_PROGRESS / WAITING / BLOCKED / UNAVAILABLE states are fail-closed.
static constexpr uint32_t BLINKA_REQUEST_FRESH_MS = 2000;

static bool autoBlinkerALCAllowsDirection(uint8_t reqDir, uint8_t *alcOut = nullptr) {
  uint8_t alc;
  bool valid;
  portENTER_CRITICAL(&stateMux);
  alc = dasAutoLaneChangeState;
  valid = dasAutoLaneChangeStateValid;
  portEXIT_CRITICAL(&stateMux);

  if (alcOut) *alcOut = alc;
  if (!valid) return false;
  if (reqDir == 1) return (alc == 6 || alc == 8); // LEFT
  if (reqDir == 2) return (alc == 7 || alc == 8); // RIGHT
  return false;
}

// Returns the currently eligible LEFT(1)/RIGHT(2) Auto Blinker request.
// This is the single source of truth for both ARM and delayed FIRE.
static uint8_t autoBlinkerEligibleRequestDir(uint32_t now, uint8_t *alcOut = nullptr) {
  if (!activeProfileAdvancedEapSupported()) return 0;
  bool en;
  uint8_t behavior;
  uint32_t visualLast;
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  behavior = visualBehaviorType;
  visualLast = visualDebugLastMs;
  portEXIT_CRITICAL(&blinkAMux);

  if (!en) return 0;
  if (!autoBlinkerNOAGateOpen(now)) return 0;
  if (visualLast == 0 || (uint32_t)(now - visualLast) > BLINKA_REQUEST_FRESH_MS) return 0;

  uint8_t reqDir = 0;
  if (behavior == 2) reqDir = 1;      // LEFT
  else if (behavior == 3) reqDir = 2; // RIGHT
  if (reqDir == 0) return 0;

  return autoBlinkerALCAllowsDirection(reqDir, alcOut) ? reqDir : 0;
}

// S3XY single-click NOA-cancel gate. A retained/stale 0x24A behaviorType must
// never authorize a UI_ulcSnooze transmission.
static constexpr uint32_t ULC_REQUEST_FRESH_MS = 2000;

static void ulcSnoozeSetResult(const char *text, uint8_t dir, bool accepted) {
  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozeLastActionMs = (uint32_t)millis();
  ulcSnoozeLastDir = dir;
  strncpy(ulcSnoozeLastResult, text ? text : "unknown", sizeof(ulcSnoozeLastResult) - 1);
  ulcSnoozeLastResult[sizeof(ulcSnoozeLastResult) - 1] = '\0';
  if (accepted) ulcSnoozeAccepted++;
  else          ulcSnoozeBlocked++;
  portEXIT_CRITICAL(&ulcSnoozeMux);
}

static bool ulcSnoozeRequestActive(uint32_t now) {
  bool pending;
  uint32_t expire;
  portENTER_CRITICAL(&ulcSnoozeMux);
  pending = ulcSnoozePending;
  expire = ulcSnoozeExpireMs;
  if (pending && (int32_t)(now - expire) >= 0) {
    ulcSnoozePending = false;
    ulcSnoozeExpireMs = 0;
    pending = false;
  }
  portEXIT_CRITICAL(&ulcSnoozeMux);
  return pending;
}

static void ulcSnoozeFinishRequest() {
  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozePending = false;
  ulcSnoozeExpireMs = 0;
  portEXIT_CRITICAL(&ulcSnoozeMux);
}

static void handleS3xySingleAction() {
  const uint32_t now = (uint32_t)millis();

  if (!autoBlinkerNOAGateOpen(now)) {
    ulcSnoozeSetResult("BLOCKED: NOA state invalid", 0, false);
    s3xyLogPush(S3XY_LOG_INFO, "button action blocked: NOA state invalid");
    return;
  }

  const uint8_t behavior = visualBehaviorType;
  const uint32_t visualLast = visualDebugLastMs;
  const uint32_t visualAge = (visualLast == 0) ? UINT32_MAX : (uint32_t)(now - visualLast);
  uint8_t dir = 0;
  if (behavior == 2) dir = 1;
  else if (behavior == 3) dir = 2;

  if (visualLast == 0 || visualAge > ULC_REQUEST_FRESH_MS) {
    ulcSnoozeSetResult("BLOCKED: 0x24A stale", dir, false);
    s3xyLogPush(S3XY_LOG_INFO, "button action blocked: 0x24A stale");
    return;
  }
  if (dir == 0) {
    ulcSnoozeSetResult("BLOCKED: no lane-change request", 0, false);
    s3xyLogPush(S3XY_LOG_INFO, "button action blocked: no lane-change request");
    return;
  }

  // Cancel our own Auto Blinker state at the same time as the Tesla snooze
  // request. lastReqDir remains equal to the current proposal direction so a
  // retained 0x24A request cannot immediately re-arm. behavior=0 resets it.
  portENTER_CRITICAL(&blinkAMux);
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  oneShotTurn = STALK_IDLE;
  oneShotUntil = 0;
  activeTurn = STALK_IDLE;
  lastReqDir = dir;
  portEXIT_CRITICAL(&blinkAMux);

  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozePending = true;
  ulcSnoozeExpireMs = now + ULC_SNOOZE_REQUEST_TIMEOUT_MS;
  portEXIT_CRITICAL(&ulcSnoozeMux);

  ulcSnoozeSetResult(dir == 1 ? "ARMED: LEFT cancel" : "ARMED: RIGHT cancel", dir, true);
  s3xyLogPush(S3XY_LOG_INFO, dir == 1 ? "button action -> ULC snooze LEFT" : "button action -> ULC snooze RIGHT");
}

// 0x3F8 UI_driverAssistControl stock telemetry + controlled LAB overlay.
// The original frame is never blocked.  When an override is selected, the newest
// stock 0x3F8 is copied, only the selected field(s) are changed, and one overlay
// frame is transmitted.  If every field is STOCK, 0x3F8 remains RX-only.
//
// Overlay TX is fail-closed and is permitted only
// while the freshest Model YL DAS state is exactly AUTOSTEER (state 3).
// AUTOSTEER_RESTRICTED(4), NOA(5), FSD(6), AP OFF/manual driving and Park never TX.
static portMUX_TYPE lab3f8Mux = portMUX_INITIALIZER_UNLOCKED;
static constexpr uint8_t LAB3F8_STOCK = 0xFF;
static constexpr uint8_t LAB3F8_ALC_STOCK = 0;
static constexpr uint8_t LAB3F8_ALC_FORCE_OFF = 1;
static constexpr uint8_t LAB3F8_ALC_FORCE_ON = 2;
static constexpr uint32_t LAB399_LANE_FRESH_MS = 1000;

static volatile uint8_t uiUlcBlindSpotConfig = 0;       // stock bits 52-53
static volatile uint8_t uiUlcSpeedConfig = 0;           // stock bits 50-51
static volatile bool    uiAlcOffHighwayEnable = false;  // stock bit 56
static volatile uint8_t uiAccFollowDistanceRaw = 0;     // stock bits 45-47 (0..7)
static volatile uint32_t uiDriverAssistLastRxMs = 0;

// Persistent LAB selections. Blind/ACC use 0xFF for STOCK.
static volatile uint8_t lab3f8AlcMode = LAB3F8_ALC_STOCK;
static volatile uint8_t lab3f8UlcBlindMode = LAB3F8_STOCK;
static volatile uint8_t lab3f8AccFollowRaw = LAB3F8_STOCK;
static volatile uint32_t lab3f8TxOk = 0;
static volatile uint32_t lab3f8TxFail = 0;
static volatile uint32_t lab3f8GateBlocked = 0;
static volatile bool lab3f8LastTxValid = false;
static volatile uint8_t lab3f8LastTxBlind = 0;
static volatile uint8_t lab3f8LastTxStockBlind = 0;
static volatile uint8_t lab3f8LastTxSelectedBlind = LAB3F8_STOCK;
static volatile bool lab3f8LastTxBlindChanged = false;
static volatile uint8_t lab3f8LastTxResult = 0; // 0=NONE, 1=QUEUED, 2=FAILED
static volatile uint32_t lab3f8LastTxMs = 0;

// 0x238 UI_driverAssistMapData road-context telemetry (VH CAN).
// Public Tesla DBC layout is used as a reference interpretation and is monitored
// independently from the existing ALC geometry investigation.
static portMUX_TYPE roadContextMux = portMUX_INITIALIZER_UNLOCKED;
static constexpr uint32_t ROAD_CONTEXT_FRESH_MS = 2000;
static volatile bool roadContextValid = false;
static volatile uint8_t roadContextRoadClass = 0;
static volatile bool roadContextGpsRoadMatch = false;
static volatile bool roadContextNavRouteActive = false;
static volatile bool roadContextControlledAccess = false;
static volatile bool roadContextLeftOffRamp = false;
static volatile bool roadContextRightOffRamp = false;
static volatile uint32_t roadContextLastRxMs = 0;
static TlsscHighwayHysteresisPure tlsscHighwayHysteresis = {};
static volatile uint32_t tlsscHighwayGateBlockedCount = 0;
static volatile uint32_t tlsscHighwayTransitions = 0;

static const char *pedalMapName(uint8_t v) {
  switch (v) { case 0: return "CHILL"; case 1: return "SPORT"; case 2: return "PERFORMANCE"; default: return "UNKNOWN"; }
}
static const char *alcStateName(uint8_t v) {
  switch (v) {
    case 6: return "AVAILABLE LEFT";
    case 7: return "AVAILABLE RIGHT";
    case 8: return "AVAILABLE BOTH";
    case 21: return "SOLID LANE";
    case 22: return "TTC LEFT";
    case 23: return "TTC+USS LEFT";
    case 24: return "TTC RIGHT";
    case 25: return "TTC+USS RIGHT";
    case 26: return "LANE TYPE LEFT";
    case 27: return "LANE TYPE RIGHT";
    default: return "OTHER/UNAVAILABLE";
  }
}

// CAN B load-shedding / queue telemetry.
// Priority policy is state-scoped so normal/AP driving cannot trigger
// Summon queue flushing or aggressive Summon load shedding.
static constexpr uint16_t TWAI_TX_QUEUE_LEN = 16;
static constexpr uint16_t TWAI_STANDBY_NON_SUMMON_QUEUE_LIMIT = 12;
static constexpr uint16_t TWAI_FULL_NON_SUMMON_QUEUE_LIMIT = 6;
static constexpr uint8_t  TWAI_RX_DRAIN_BUDGET = 64;
static constexpr uint32_t TWAI_QUEUE_TELEMETRY_PERIOD_MS = 50;

static volatile uint32_t twaiTxQueueNow = 0;
static volatile uint32_t twaiTxQueueMax = 0;
static volatile uint32_t twaiRxQueueNow = 0;
static volatile uint32_t twaiRxQueueMax = 0;
static volatile uint32_t twaiNonSummonShed = 0;
static volatile uint32_t twaiStandbyShed = 0;
static volatile uint32_t twaiFullShed = 0;
static volatile uint32_t twaiSummonQueueFlush = 0;
static volatile uint32_t twaiSummonRetryOk = 0;
static volatile uint32_t twaiSummonRetryFail = 0;
static volatile uint32_t twaiSummonTxNormal = 0;
static volatile uint32_t twaiSummonTxStandby = 0;
static volatile uint32_t twaiSummonTxFull = 0;

static const char* summonPriorityStateName(uint8_t state) {
  switch (state) {
    case SUMMON_PRIORITY_PARK_STANDBY: return "PARK_STANDBY";
    case SUMMON_PRIORITY_FULL:         return "SUMMON_FULL";
    default:                           return "NORMAL";
  }
}

// Keep the browser independent from ESP-IDF enum ordering.
static const char* twaiStateName(int state) {
  switch (state) {
    case TWAI_STATE_STOPPED:    return "STOPPED";
    case TWAI_STATE_RUNNING:    return "RUNNING";
    case TWAI_STATE_BUS_OFF:    return "BUS OFF";
    case TWAI_STATE_RECOVERING: return "RECOVERING";
    default:                    return "UNKNOWN";
  }
}

// stateMux must already be held when calling this helper.
// The most recently received *fresh* valid gear source wins. This prevents
// Prevent the legacy 280-stale => gateParked=true fallback from enabling
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
  const bool sessionActive = gateSummoning;
  const uint8_t oldState = summonPriorityState;
  uint8_t nextState = oldState;

  if (oldState == SUMMON_PRIORITY_FULL) {
    if (sessionActive) {
      summonPriorityFullInactiveSinceMs = 0;
    } else {
      if (summonPriorityFullInactiveSinceMs == 0)
        summonPriorityFullInactiveSinceMs = now;
      if ((uint32_t)(now - summonPriorityFullInactiveSinceMs) >= SUMMON_PRIORITY_FULL_EXIT_GRACE_MS)
        nextState = freshParked ? SUMMON_PRIORITY_PARK_STANDBY : SUMMON_PRIORITY_NORMAL;
    }
  } else {
    summonPriorityFullInactiveSinceMs = 0;
    if (sessionActive && (oldState == SUMMON_PRIORITY_PARK_STANDBY || freshParked))
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

static bool twaiReadQueueStatus(twai_status_info_t *out = nullptr) {
  twai_status_info_t st = {};
  if (twai_get_status_info(&st) != ESP_OK) return false;
  twaiTxQueueNow = st.msgs_to_tx;
  twaiRxQueueNow = st.msgs_to_rx;
  if (st.msgs_to_tx > twaiTxQueueMax) twaiTxQueueMax = st.msgs_to_tx;
  if (st.msgs_to_rx > twaiRxQueueMax) twaiRxQueueMax = st.msgs_to_rx;
  if (out) *out = st;
  return true;
}

// Lower-priority features never block CAN B RX. Queue reservation is scoped:
// - NORMAL: no Summon-specific shedding.
// - PARK_STANDBY: mild reservation (4 slots) for a clean Summon start.
// - SUMMON_FULL: aggressive reservation for Summon-scoped TX.
static bool twaiNonSummonAdmissionOpen() {
  const uint8_t priorityState = getSummonPriorityState();

  // During normal driving there is no Summon-specific admission policy at all.
  // The following guarded driver enqueue remains non-blocking and is allowed to
  // succeed/fail directly without an extra status query on every injected frame.
  if (priorityState == SUMMON_PRIORITY_NORMAL) return true;

  twai_status_info_t st = {};
  if (!twaiReadQueueStatus(&st)) return false;

  const uint16_t limit = (priorityState == SUMMON_PRIORITY_FULL)
                       ? TWAI_FULL_NON_SUMMON_QUEUE_LIMIT
                       : TWAI_STANDBY_NON_SUMMON_QUEUE_LIMIT;

  if (st.msgs_to_tx >= limit) {
    twaiNonSummonShed++;
    if (priorityState == SUMMON_PRIORITY_PARK_STANDBY) twaiStandbyShed++;
    if (priorityState == SUMMON_PRIORITY_FULL) twaiFullShed++;
    return false;
  }
  return true;
}

// S3XY acceleration-mode experiment: VH 0x334 UI_powertrainControl overlay.
// Live YL validation confirmed DLC8 VH frames track the Tesla UI pedal map.
static portMUX_TYPE pedalMapMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool pedalMapStockValid = false;
static volatile uint8_t pedalMapStockRaw = 0xFF;
static volatile uint32_t pedalMapStockMs = 0;
static volatile bool pedalMapOverrideActive = false;
static volatile uint8_t pedalMapTargetRaw = 0xFF;
static volatile uint8_t pedalMapOriginRaw = 0xFF;
static volatile uint32_t pedalMapTxOk = 0, pedalMapTxFail = 0, pedalMapBlocked = 0, pedalMapStockAccepted = 0;
static constexpr uint32_t PEDAL_MAP_STOCK_FRESH_MS = 1500;

// Unknown after CAN A recovery is not confirmed manual mode. Manual-only pedal
// map TX therefore requires a valid latched Party 0x399 state that explicitly
// reports manual driving; no arbitrary age timeout is applied.
static bool manualDrivingGateOpen(uint32_t now) {
  (void)now;
  uint8_t state4;
  bool valid;
  portENTER_CRITICAL(&stateMux);
  state4 = dasAutopilotState4;
  valid = dasAutopilotStateValid;
  portEXIT_CRITICAL(&stateMux);
  return dasStateManualPure(valid, state4);
}

static uint8_t ui334Checksum(const uint8_t *data) {
  uint8_t sum = (uint8_t)(0x34 + 0x03); // low byte 0x34 + high-ID nibble 0x3
  for (uint8_t i=0;i<7;i++) sum = (uint8_t)(sum + data[i]);
  return sum;
}

static void requestPedalMapToggleFromButton() {
  if (!activeProfilePedalMapSupported()) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    s3xyLogPush(S3XY_LOG_INFO, "Acceleration toggle unavailable for current vehicle profile");
    return;
  }
  const uint32_t now=(uint32_t)millis();
  const uint32_t txEpoch=canTxEpochSnapshot();
  bool valid, active; uint8_t stock,target; uint32_t age;
  portENTER_CRITICAL(&pedalMapMux);
  valid=pedalMapStockValid; stock=pedalMapStockRaw; active=pedalMapOverrideActive; target=pedalMapTargetRaw; age=pedalMapStockMs?now-pedalMapStockMs:UINT32_MAX;
  portEXIT_CRITICAL(&pedalMapMux);
  if (!valid || age>PEDAL_MAP_STOCK_FRESH_MS || !manualDrivingGateOpen(now) ||
      getSummonPriorityState()==SUMMON_PRIORITY_FULL) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    s3xyLogPush(S3XY_LOG_INFO,"Acceleration toggle blocked: 0x334 stale/AP/Summon"); return;
  }
  const uint8_t current=active?target:stock;
  const uint8_t next=(current==0)?1:0; // CHILL <-> SPORT; PERFORMANCE -> CHILL
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return;
  }
  if (!canTxBarrierAllowsPure(canTxBarrierState, txEpoch)) {
    xSemaphoreGive(canTxBarrierMutex);
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return;
  }
  portENTER_CRITICAL(&pedalMapMux);
  pedalMapOverrideActive=true; pedalMapOriginRaw=stock; pedalMapTargetRaw=next;
  portEXIT_CRITICAL(&pedalMapMux);
  xSemaphoreGive(canTxBarrierMutex);
  s3xyLogPush(S3XY_LOG_INFO,next==1?"Acceleration target: SPORT":"Acceleration target: CHILL");
}

static bool pedalMapObserveAndPrepare(const uint8_t *srcData, uint8_t dlc, uint8_t outData[8],
                                     uint32_t &txEpoch, bool requireTwaiAdmission) {
  if (!srcData || dlc != 8 || !outData) return false;
  const uint32_t now=(uint32_t)millis();
  const uint8_t stock=(uint8_t)readBitsLE(srcData,5,2);
  bool active; uint8_t target,origin;
  portENTER_CRITICAL(&pedalMapMux);
  pedalMapStockValid=true; pedalMapStockRaw=stock; pedalMapStockMs=now;
  active=pedalMapOverrideActive; target=pedalMapTargetRaw; origin=pedalMapOriginRaw;
  // If Tesla UI itself adopts our target, return to pure STOCK operation.
  if (active && stock==target) { pedalMapOverrideActive=false; pedalMapStockAccepted++; active=false; }
  // If the user changes the UI to a third value, respect the UI and release our override.
  else if (active && stock!=origin && stock!=target) { pedalMapOverrideActive=false; active=false; }
  portEXIT_CRITICAL(&pedalMapMux);
  if (!active || stock==target) return false;

  txEpoch=canTxEpochSnapshot();
  // The override is recovery-invalidated state. Re-read after the epoch snapshot.
  portENTER_CRITICAL(&pedalMapMux);
  active=pedalMapOverrideActive; target=pedalMapTargetRaw;
  portEXIT_CRITICAL(&pedalMapMux);
  if (!active || stock==target) return false;
  if (!manualDrivingGateOpen(now) || getSummonPriorityState()!=SUMMON_PRIORITY_NORMAL ||
      (requireTwaiAdmission && !twaiNonSummonAdmissionOpen())) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return false;
  }

  memcpy(outData, srcData, 8);
  writeBitsLE(outData,5,2,target);
  uint8_t ctr=(uint8_t)readBitsLE(outData,52,4);
  ctr=(uint8_t)((ctr+1)&0x0F);
  writeBitsLE(outData,52,4,ctr);
  outData[7]=0;
  outData[7]=ui334Checksum(outData);
  return true;
}

// Model Y L: 0x334 is on VH / CAN B (TWAI).
static void handlePedalMap334OnCanB(const twai_message_t &src) {
  if (src.extd || src.rtr || src.data_length_code != 8) return;
  uint8_t outData[8] = {};
  uint32_t txEpoch = 0;
  if (!pedalMapObserveAndPrepare(src.data, src.data_length_code, outData, txEpoch, true)) return;
  twai_message_t out=src;
  memcpy(out.data,outData,8);
  const esp_err_t err=canTxTwaiTransmit(&out,txEpoch);
  portENTER_CRITICAL(&pedalMapMux);
  if(err==ESP_OK)pedalMapTxOk++; else pedalMapTxFail++;
  portEXIT_CRITICAL(&pedalMapMux);
}

// Standard Model 3/Y Body+Chassis: 0x334 is on Body / CAN A (MCP2515).
static void handlePedalMap334OnCanA(const struct can_frame &src) {
  if ((src.can_id & 0xC0000000UL) != 0 || src.can_dlc != 8) return;
  uint8_t outData[8] = {};
  uint32_t txEpoch = 0;
  if (!pedalMapObserveAndPrepare(src.data, src.can_dlc, outData, txEpoch, false)) return;
  struct can_frame out = {};
  out.can_id = UI_POWERTRAIN_ID;
  out.can_dlc = 8;
  memcpy(out.data,outData,8);
  MCP2515::ERROR mcpErr=MCP2515::ERROR_FAIL;
  const bool attempted=canTxMcpSend(&out,txEpoch,mcpErr,nullptr);
  portENTER_CRITICAL(&pedalMapMux);
  if(attempted && mcpErr==MCP2515::ERROR_OK) {
    pedalMapTxOk++; mcpTxOk++; mcpTxFailConsecutive=0;
  } else {
    pedalMapTxFail++; mcpTxFail++; if(mcpTxFailConsecutive<255)mcpTxFailConsecutive++;
  }
  portEXIT_CRITICAL(&pedalMapMux);
}

static String pedalMapStatsJson(){
  const uint32_t now=(uint32_t)millis(); bool valid,active; uint8_t stock,target,origin; uint32_t ms,ok,fail,blocked,accepted;
  portENTER_CRITICAL(&pedalMapMux); valid=pedalMapStockValid; active=pedalMapOverrideActive; stock=pedalMapStockRaw; target=pedalMapTargetRaw; origin=pedalMapOriginRaw; ms=pedalMapStockMs; ok=pedalMapTxOk; fail=pedalMapTxFail; blocked=pedalMapBlocked; accepted=pedalMapStockAccepted; portEXIT_CRITICAL(&pedalMapMux);
  String j="{\"valid\":"+String(valid?"true":"false")+",\"stockRaw\":"+String((unsigned)stock)+",\"stockName\":\""+String(valid?pedalMapName(stock):"NO DATA")+"\",\"ageMs\":"+String((unsigned long)(ms?now-ms:999999UL))+",\"overrideActive\":"+String(active?"true":"false")+",\"targetRaw\":"+String((unsigned)target)+",\"targetName\":\""+String(active?pedalMapName(target):"STOCK")+"\",\"originRaw\":"+String((unsigned)origin)+",\"txOk\":"+String((unsigned long)ok)+",\"txFail\":"+String((unsigned long)fail)+",\"blocked\":"+String((unsigned long)blocked)+",\"stockAccepted\":"+String((unsigned long)accepted)+"}"; return j;
}


// Summon TX transport is state-scoped and non-blocking on the CAN B RX task.
// NORMAL: no destructive priority behavior.
// PARK_STANDBY: queue headroom is reserved by non-Summon admission control.
// SUMMON_FULL: stale pending T-2CAN TX may be flushed to protect the newest
//              Summon mux1 injection. Queue clear is NEVER used outside FULL.
static esp_err_t twaiTransmitSummonPriority(const twai_message_t *msg, uint32_t txEpoch) {
  const uint8_t priorityState = getSummonPriorityState();

  if (priorityState == SUMMON_PRIORITY_NORMAL) {
    const esp_err_t err = canTxTwaiTransmit(msg, txEpoch);
    if (err == ESP_OK) twaiSummonTxNormal++;
    return err;
  }

  if (priorityState == SUMMON_PRIORITY_PARK_STANDBY) {
    const esp_err_t err = canTxTwaiTransmit(msg, txEpoch);
    if (err == ESP_OK) twaiSummonTxStandby++;
    return err;
  }

  // SUMMON_FULL only: keep stale pending injections from delaying the newest
  // unlock frame. The currently transmitting hardware frame is not cleared.
  twai_status_info_t st = {};
  if (twaiReadQueueStatus(&st) && st.msgs_to_tx >= (TWAI_TX_QUEUE_LEN - 2)) {
    if (canTxTwaiClearQueue(txEpoch) == ESP_OK) twaiSummonQueueFlush++;
  }

  esp_err_t err = canTxTwaiTransmit(msg, txEpoch);
  if (err == ESP_OK) {
    twaiSummonTxFull++;
    return ESP_OK;
  }

  // Only queue saturation gets a destructive retry. Driver/bus state errors
  // are left to the existing recovery supervisor.
  if (err != ESP_ERR_TIMEOUT) {
    twaiSummonRetryFail++;
    return err;
  }

  if (canTxTwaiClearQueue(txEpoch) == ESP_OK) twaiSummonQueueFlush++;
  err = canTxTwaiTransmit(msg, txEpoch);
  if (err == ESP_OK) {
    twaiSummonRetryOk++;
    twaiSummonTxFull++;
  } else {
    twaiSummonRetryFail++;
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

static inline void writeBitsLE(uint8_t *data, int startBit, int len, uint32_t value) {
  for (int i = 0; i < len; i++) {
    setBit(data, startBit + i, ((value >> i) & 0x01U) != 0);
  }
}


// SCCM_leftStalk (0x249) checksum model shared by YL and Standard 3/Y stalk vehicles.
static inline uint8_t sccm249Crc8(const uint8_t *data, uint8_t len) { return sccm249Crc8Pure(data, len); }
static inline uint8_t leftStalkChecksum(const uint8_t frame[4], uint8_t counter) { return leftStalkChecksumPure(frame, counter); }

static inline uint8_t dirToTurn(uint8_t dir) {
  if (dir == 1) return STALK_DOWN_1;
  if (dir == 2) return STALK_UP_1;
  return STALK_IDLE;
}

// Read the real 0x249 frame on CAN B and align the injected counter.
static void handle249OnCanB(const uint8_t *data, uint8_t dlc) {
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

  // The validated Model YL checksum requires all four stock bytes.
  // A short/non-YL frame is still counted/observed, but cannot pass self-test.
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

// Send SCCM_turnIndicatorStalkStatus on CAN B.
//
// Do not fabricate a 0x249 payload from zeros. The newest real
// Model YL stock frame is used as the template so byte1 upper bits, byte2
// upper bits and byte3 remain exactly as the vehicle produced them.
// Only the rolling counter and requested turn nibble are changed, then the
// validated full-payload CRC is recalculated.
static void sendStalkFrameCanB(uint8_t turn, uint32_t txEpoch) {
  uint8_t cnt;
  uint8_t stockTemplate[4] = {0};
  bool haveStockTemplate = false;

  portENTER_CRITICAL(&blinkAMux);
  cnt = (blinkACounter + 1) & 0x0F;
  blinkACounter = cnt;
  haveStockTemplate = seen249 && realDlc >= 4;
  if (haveStockTemplate) memcpy(stockTemplate, realRaw249, sizeof(stockTemplate));
  portEXIT_CRITICAL(&blinkAMux);

  // Do not inject a guessed SCCM frame before a real Model YL 0x249 template
  // has been observed on the bus.
  if (!haveStockTemplate) {
    portENTER_CRITICAL(&blinkAMux);
    blkATxFail++;
    portEXIT_CRITICAL(&blinkAMux);
    return;
  }

  twai_message_t out = {};
  out.identifier = LEFTSTALK_ID;
  out.data_length_code = 4;
  out.flags = 0;
  memcpy(out.data, stockTemplate, sizeof(stockTemplate));

  out.data[1] = (uint8_t)((out.data[1] & 0xF0) | (cnt & 0x0F));
  out.data[2] = (uint8_t)((out.data[2] & 0xF0) | (turn & 0x0F));
  out.data[0] = leftStalkChecksum(out.data, cnt);

  // Auto Blinker is lower priority than Summon. Never block CAN B RX.
  esp_err_t err = ESP_ERR_TIMEOUT;
  if (twaiNonSummonAdmissionOpen()) err = canTxTwaiTransmit(&out, txEpoch);
  portENTER_CRITICAL(&blinkAMux);
  if (err == ESP_OK) blkATxOk++;
  else blkATxFail++;
  portEXIT_CRITICAL(&blinkAMux);
}

static void handle249OnCanA(const uint8_t *data, uint8_t dlc) {
  handle249OnCanB(data, dlc); // identical codec/state; only physical bus differs
}

static void sendStalkFrameCanA(uint8_t turn, uint32_t txEpoch) {
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
    portENTER_CRITICAL(&blinkAMux); blkATxFail++; portEXIT_CRITICAL(&blinkAMux);
    return;
  }

  struct can_frame out = {};
  out.can_id = LEFTSTALK_ID;
  out.can_dlc = 4;
  memcpy(out.data, stockTemplate, sizeof(stockTemplate));
  out.data[1] = (uint8_t)((out.data[1] & 0xF0) | (cnt & 0x0F));
  out.data[2] = (uint8_t)((out.data[2] & 0xF0) | (turn & 0x0F));
  out.data[0] = leftStalkChecksum(out.data, cnt);
  MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
  const bool attempted = canTxMcpSend(&out, txEpoch, mcpErr, nullptr);
  portENTER_CRITICAL(&blinkAMux);
  if (attempted && mcpErr == MCP2515::ERROR_OK) { blkATxOk++; mcpTxOk++; mcpTxFailConsecutive = 0; }
  else { blkATxFail++; mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
  portEXIT_CRITICAL(&blinkAMux);
}


static void handle3C2OnCanA(const struct can_frame &incoming) {
  if (incoming.can_dlc < 8 || vcleftMuxPure(incoming.data) != VCLEFT_SWITCH_MUX1) return;
  const uint32_t now = (uint32_t)millis();
  const uint8_t left = vcleftLeftButtonPure(incoming.data);
  const uint8_t right = vcleftRightButtonPure(incoming.data);
  bool inject = false;
  uint8_t turn = STALK_IDLE;
  uint8_t switchState = VCLEFT_SWITCH_SNA;

  portENTER_CRITICAL(&blinkAMux);
  seen3C2 = true;
  rx3C2++;
  last3C2Mux1Ms = now;
  real3C2LeftButton = left;
  real3C2RightButton = right;
  // A physical steering-wheel button press wins over an automatic pulse.
  if ((left == VCLEFT_SWITCH_ON || right == VCLEFT_SWITCH_ON) && oneShotTurn != STALK_IDLE) {
    autoArmed = false; autoPendingDir = 0; autoFireAt = 0;
    oneShotTurn = STALK_IDLE; oneShotUntil = 0; oneShotReleaseAt = 0; activeTurn = STALK_IDLE;
  } else if (activeTurnSignalVariant == TURN_SIGNAL_STALKLESS &&
             oneShotTurn != STALK_IDLE && (int32_t)(oneShotUntil - now) > 0) {
    inject = true;
    turn = oneShotTurn;
    switchState = ((int32_t)(oneShotReleaseAt - now) > 0) ? VCLEFT_SWITCH_ON : VCLEFT_SWITCH_OFF;
  }
  portEXIT_CRITICAL(&blinkAMux);
  if (!inject) return;

  struct can_frame out = incoming;
  if (turn == STALK_DOWN_1) vcleftSetLeftButtonPure(out.data, switchState);
  else if (turn == STALK_UP_1) vcleftSetRightButtonPure(out.data, switchState);
  else return;
  const uint32_t txEpoch = canTxEpochSnapshot();
  MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
  const bool attempted = canTxMcpSend(&out, txEpoch, mcpErr, nullptr);
  portENTER_CRITICAL(&blinkAMux);
  if (attempted && mcpErr == MCP2515::ERROR_OK) { blkATxOk++; stalklessTxOk++; mcpTxOk++; mcpTxFailConsecutive = 0; }
  else { blkATxFail++; stalklessTxFail++; mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
  portEXIT_CRITICAL(&blinkAMux);
}

static void handle102LaneChangeCancel(const uint8_t *data, uint8_t dlc) {
  if (!activeProfileBodyControlsSupported() && !activeProfileIsYl()) return;
  const bool pressed = doorOpenButtonPressedPure(data, dlc);
  bool previous;
  bool enabled;
  portENTER_CRITICAL(&blinkAMux);
  previous = doorButtonPressed;
  doorButtonPressed = pressed;
  enabled = doorOpenCancelEnabled;
  if (pressed) doorButtonRx++;
  portEXIT_CRITICAL(&blinkAMux);
  if (!enabled || !pressed || previous) return;

  const uint32_t now = (uint32_t)millis();
  if (!autoBlinkerNOAGateOpen(now) || visualDebugLastMs == 0 ||
      (uint32_t)(now - visualDebugLastMs) > ULC_REQUEST_FRESH_MS ||
      (visualBehaviorType != 2 && visualBehaviorType != 3)) {
    portENTER_CRITICAL(&blinkAMux); doorCancelBlocked++; portEXIT_CRITICAL(&blinkAMux);
    return;
  }

  const uint8_t dir = visualBehaviorType == 2 ? 1 : 2;
  portENTER_CRITICAL(&blinkAMux);
  autoArmed = false; autoPendingDir = 0; autoFireAt = 0;
  oneShotTurn = STALK_IDLE; oneShotUntil = 0; oneShotReleaseAt = 0;
  activeTurn = STALK_IDLE; lastReqDir = dir; doorCancelAccepted++;
  portEXIT_CRITICAL(&blinkAMux);
  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozePending = true;
  ulcSnoozeExpireMs = now + ULC_SNOOZE_REQUEST_TIMEOUT_MS;
  portEXIT_CRITICAL(&ulcSnoozeMux);
  ulcSnoozeSetResult(dir == 1 ? "ARMED: LEFT door cancel" : "ARMED: RIGHT door cancel", dir, true);
}


// Arm a delayed trigger when behaviorType becomes LEFT/RIGHT.
static void evaluateAutoBlinker() {
  const uint32_t now = (uint32_t)millis();
  const uint8_t reqDir = autoBlinkerEligibleRequestDir(now);

  portENTER_CRITICAL(&blinkAMux);

  if (reqDir != 0 && reqDir != lastReqDir && !autoArmed) {
    autoPendingDir = reqDir;
    autoFireAt = now + blinkADelayMs;
    autoArmed = true;
  }

  // Any loss of valid NOA state, 0x24A freshness, direction availability,
  // or request direction cancels only the pending delayed trigger.
  // A pulse that has already started keeps its original 350 ms lifetime.
  if (reqDir == 0) {
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
  }

  lastReqDir = reqDir;
  portEXIT_CRITICAL(&blinkAMux);
}

// Generate the one-shot 350 ms CAN B pulse.
// Advanced EAP state model:
//   autoFireAt   = delayed-trigger deadline only
//   oneShotUntil = active-pulse deadline only
// This prevents a later 0x24A behavior change from shortening or extending
// an SCCM pulse that has already started.
static void blinkATxTick() {
  if (!activeProfileAdvancedEapSupported()) return;
  static uint32_t lastTxMs = 0;
  const uint32_t now = millis();
  bool workPending;
  portENTER_CRITICAL(&blinkAMux);
  workPending = autoArmed || oneShotTurn != STALK_IDLE;
  portEXIT_CRITICAL(&blinkAMux);
  if (!workPending) return;
  const uint32_t txEpoch = canTxEpochSnapshot();
  uint8_t turn = STALK_IDLE;

  // Re-evaluate the full gate immediately before a delayed request is allowed
  // to fire. This prevents a request armed while ALC was available from
  // transmitting after the lane becomes blocked/unavailable during the delay.
  const uint8_t eligibleDirNow = autoBlinkerEligibleRequestDir(now);

  portENTER_CRITICAL(&blinkAMux);

  if (autoArmed && eligibleDirNow != autoPendingDir) {
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
    lastReqDir = eligibleDirNow;
  }

  // Delayed trigger reached its deadline -> start an independent pulse only
  // if the exact LEFT/RIGHT request is still eligible now.
  if (autoArmed &&
      eligibleDirNow != 0 &&
      eligibleDirNow == autoPendingDir &&
      (int32_t)(now - autoFireAt) >= 0) {
    oneShotTurn = dirToTurn(autoPendingDir);
    if (oneShotTurn != STALK_IDLE && activeTurnSignalVariant == TURN_SIGNAL_STALKLESS) {
      oneShotReleaseAt = now + BLINKA_STALKLESS_PRESS_MS;
      oneShotUntil = oneShotReleaseAt + BLINKA_STALKLESS_RELEASE_MS;
    } else {
      oneShotReleaseAt = 0;
      oneShotUntil = (oneShotTurn != STALK_IDLE) ? (now + BLINKA_PULSE_MS) : 0;
    }
    autoArmed = false;
    autoPendingDir = 0;
    autoFireAt = 0;
  }

  // Once started, preserve the established pulse behavior: the pulse lifetime is
  // independent of later planner/state changes and may finish its 350 ms window.
  if (oneShotTurn != STALK_IDLE && (int32_t)(oneShotUntil - now) > 0) {
    turn = oneShotTurn;
  } else {
    oneShotTurn = STALK_IDLE;
    oneShotUntil = 0;
  }

  activeTurn = turn;
  portEXIT_CRITICAL(&blinkAMux);

  if (turn == STALK_IDLE) return;
  if (activeTurnSignalVariant == TURN_SIGNAL_STALKLESS) return; // 0x3C2 echoes on each live mux1 RX
  if (now - lastTxMs < BLINKA_TX_PERIOD_MS) return;
  lastTxMs = now;
  if (activeProfileIsYl()) sendStalkFrameCanB(turn, txEpoch);
  else if (activeCanAIsBody()) sendStalkFrameCanA(turn, txEpoch);
}

// Legacy 0x3F8 ULC injection remains removed; LAB provides only the
// explicit LAB overlay below, gated to fresh AUTOSTEER state 3.


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


static void nagApGateSnapshot(bool &validOut, bool &activeOut) {
    portENTER_CRITICAL(&stateMux);
    validOut = dasAutopilotStateValid;
    activeOut = gateAPActive;
    portEXIT_CRITICAL(&stateMux);
}

// NAG transport gate: inject only while the active topology has a valid
// latched DAS state saying AP is active. YL sources that state from Party CAN;
// Standard Party+Chassis sources it from Chassis CAN B. No AP age timeout is used.
static bool nagApInjectionGateOpen() {
    bool ap, valid;
    portENTER_CRITICAL(&stateMux);
    ap = gateAPActive;
    valid = dasAutopilotStateValid;
    portEXIT_CRITICAL(&stateMux);
    return valid && ap;
}

static inline bool summonGateGraceActiveLocked(uint32_t now) {
    return summonGateGraceUntilMs != 0 && (int32_t)(summonGateGraceUntilMs - now) > 0;
}

static inline void expireSummonGateGraceLocked(uint32_t now) {
    if (summonGateGraceUntilMs != 0 && !summonGateGraceActiveLocked(now)) {
        summonGateGraceUntilMs = 0;
        summonGateGraceExpireCount++;
    }
}

static inline void recoverSummonGateGraceLocked() {
    if (gateSummoning && summonGateGraceUntilMs != 0) {
        summonGateGraceUntilMs = 0;
        summonGateGraceRecoverCount++;
    }
}

static inline bool summonInjectionGateOpen() {
    const uint32_t now = (uint32_t)millis();
    expireSummonGateGraceLocked(now);
    return gateParked || gateSummoning || summonGateGraceActiveLocked(now);
}

static void recomputeSummoning() {
    gateSummoning = lastAca && sprSeen;
    recoverSummonGateGraceLocked();
}

static void clearSummonOnPark() {
    gateSummoning = false;
    sprSeen       = false;
    summonGateGraceUntilMs = 0;
}

static void clearSummonOnParkIfAcaInactive(uint8_t gear) {
    if (gear == 1 && !lastAca)
        clearSummonOnPark();
}

static void handle280(const uint8_t *data) {
    sumRx280++;
    const uint32_t now = (uint32_t)millis();
    last280Millis = now;
    uint8_t gear = readVehicleGear(data);
    int     gs   = gearState(gear);
    portENTER_CRITICAL(&stateMux);
    if (gs == 1)  gateParked = true;
    if (gs == 0)  gateParked = false;
    if (gs >= 0) {
        priorityGear280State = (int8_t)gs;
        priorityGear280Ms = now;
    }
    bool aca = (data[6] & 0x04) != 0;
    if (lastAca && !aca) {
        // Only bridge a dropout that happened after Summon was already confirmed.
        // Do not arm/extend grace from Park or from an already-inactive session.
        if (gateSummoning && !gateParked && summonGateGraceUntilMs == 0) {
            summonGateGraceUntilMs = now + SUMMON_GATE_DROPOUT_GRACE_MS;
            summonGateGraceEnterCount++;
        }
        sprSeen = false;
    }
    lastAca = aca;
    recomputeSummoning();
    expireSummonGateGraceLocked(now);
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

static void handle921(const uint8_t *data, uint8_t dlc) {
    sumRx921++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t dasState4 = readDASState4(data);
    const bool alcValid = (dlc >= 7);
    const uint8_t alcState = alcValid ? readDASAutoLaneChangeState(data) : 0xFF;
    bool ap = isDASActive(readDASStatus(data));
    bool noa = (dasState4 == 5); // ACTIVE_NAV = Navigate on Autopilot
    bool wasAp, wasNoa;
    portENTER_CRITICAL(&stateMux);
    wasAp = gateAPActive;
    wasNoa = gateNOAActive;
    gateAPActive = ap;
    gateNOAActive = noa;
    dasAutopilotState4 = dasState4;
    dasAutopilotStateValid = true;
    if (alcValid && (!dasAutoLaneChangeStateValid || alcState != dasAutoLaneChangeState)) {
      dasAutoLaneChangePrevState = dasAutoLaneChangeState;
      dasAutoLaneChangeLastChangeMs = now;
      dasAutoLaneChangeChangeCount++;
    }
    dasAutoLaneChangeState = alcState;
    dasAutoLaneChangeStateValid = alcValid;
    lastDASStatusMillis = now;
    recomputeSummonPriorityStateLocked(now);
    portEXIT_CRITICAL(&stateMux);

    // Keep NAG continuity diagnostics scoped to the current AP session.
    nagDiagApTransition(ap, wasAp, now);
    // Mode B should begin with a fresh burst when AP transitions OFF -> ON.
    if (ap && !wasAp) nagModeBPhaseStartMs = now;

    // A delayed Auto Blinker request must not survive a NOA ->
    // Autosteer/OFF transition. AP/NOA validity is latched until CAN A recovery;
    // do not truncate a pulse already in progress.
    if (wasNoa && !noa) {
      portENTER_CRITICAL(&blinkAMux);
      autoArmed = false;
      autoPendingDir = 0;
      autoFireAt = 0;
      lastReqDir = 0;
      portEXIT_CRITICAL(&blinkAMux);
    }

    // ALC availability lives in this same 0x399 frame. Re-evaluate immediately
    // so a pending Auto Blinker request is cancelled as soon as the requested
    // direction becomes unavailable, without waiting for another 0x24A frame.
    evaluateAutoBlinker();
}

static void handle1016(const uint8_t *data, uint8_t dlc) {
    if (dlc < 4) return;
    sumRx1016++;
    const uint32_t now = (uint32_t)millis();
    uint8_t spr = (data[3] >> 4) & 0x0F;
    if (dlc >= 8) {
        // UI_accFollowDistanceSetting: bits 45-47.
        // UI_ulcSpeedConfig: bits 50-51.
        // UI_ulcBlindSpotConfig: bits 52-53.
        // UI_alcOffHighwayEnable: bit 56.
        uiAccFollowDistanceRaw = (uint8_t)readBitsLE(data, 45, 3);
        uiUlcSpeedConfig = (uint8_t)readBitsLE(data, 50, 2);
        uiUlcBlindSpotConfig = (uint8_t)readBitsLE(data, 52, 2);
        uiAlcOffHighwayEnable = getBit(data, 56);
        uiDriverAssistLastRxMs = now;
    }
    portENTER_CRITICAL(&stateMux);
    if (spr != 0)
        sprSeen = true;
    recomputeSummoning();
    recomputeSummonPriorityStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
}

static void handleRoadContext238(const uint8_t *data, uint8_t dlc) {
    const RoadContext238Pure decoded = decodeRoadContext238Pure(data, dlc);
    if (!decoded.valid) return;
    const uint32_t now = (uint32_t)millis();

    portENTER_CRITICAL(&roadContextMux);
    const bool wasConfirmed = tlsscHighwayHysteresis.confirmed;
    roadContextValid = true;
    roadContextRoadClass = decoded.roadClass;
    roadContextGpsRoadMatch = decoded.gpsRoadMatch;
    roadContextNavRouteActive = decoded.navRouteActive;
    roadContextControlledAccess = decoded.controlledAccess;
    roadContextLeftOffRamp = decoded.nextBranchLeftOffRamp;
    roadContextRightOffRamp = decoded.nextBranchRightOffRamp;
    roadContextLastRxMs = now;
    tlsscHighwayUpdatePure(tlsscHighwayHysteresis,
                           decoded.gpsRoadMatch,
                           decoded.controlledAccess);
    if (tlsscHighwayHysteresis.confirmed != wasConfirmed) tlsscHighwayTransitions++;
    portEXIT_CRITICAL(&roadContextMux);
}

static bool tlsscHighwayGateBlocked(uint32_t now) {
    bool enabled;
    portENTER_CRITICAL(&stateMux);
    enabled = tlsscHighwayGateEnabled;
    portEXIT_CRITICAL(&stateMux);
    if (!enabled) return false;

    bool valid, gpsMatch, confirmed;
    uint32_t last;
    portENTER_CRITICAL(&roadContextMux);
    valid = roadContextValid;
    gpsMatch = roadContextGpsRoadMatch;
    confirmed = tlsscHighwayHysteresis.confirmed;
    last = roadContextLastRxMs;
    portEXIT_CRITICAL(&roadContextMux);

    if (!valid || !gpsMatch || last == 0) return false;
    if ((uint32_t)(now - last) > ROAD_CONTEXT_FRESH_MS) return false;
    return confirmed;
}

static bool lab3f8AutosteerGateOpen(uint32_t now) {
    (void)now;
    uint8_t state4;
    bool valid;
    portENTER_CRITICAL(&stateMux);
    state4 = dasAutopilotState4;
    valid = dasAutopilotStateValid;
    portEXIT_CRITICAL(&stateMux);
    return dasStateAutosteerPure(valid, state4);
}

static bool lab3f8AnyOverrideSelected() {
    uint8_t alc, blind, acc;
    portENTER_CRITICAL(&lab3f8Mux);
    alc = lab3f8AlcMode;
    blind = lab3f8UlcBlindMode;
    acc = lab3f8AccFollowRaw;
    portEXIT_CRITICAL(&lab3f8Mux);
    return alc != LAB3F8_ALC_STOCK || blind != LAB3F8_STOCK || acc != LAB3F8_STOCK;
}

static void injectDriverAssistControl(const twai_message_t &src) {
    if (src.data_length_code < 8) return;

    uint8_t alc, blind, acc;
    portENTER_CRITICAL(&lab3f8Mux);
    alc = lab3f8AlcMode;
    blind = lab3f8UlcBlindMode;
    acc = lab3f8AccFollowRaw;
    portEXIT_CRITICAL(&lab3f8Mux);

    // Off-Highway ALC is a production BETA and remains independent from the
    // LAB master. ULC Blind Spot / ACC Follow Distance are research-only and
    // must become pure STOCK while LAB is disabled without erasing their saved
    // selections.
    if (!labMenuEnabled) {
      blind = LAB3F8_STOCK;
      acc = LAB3F8_STOCK;
    }

    // All STOCK = guaranteed RX-only.  No 0x3F8 TX at all.
    if (alc == LAB3F8_ALC_STOCK && blind == LAB3F8_STOCK && acc == LAB3F8_STOCK)
        return;

    const uint32_t txEpoch = canTxEpochSnapshot();
    const uint32_t now = (uint32_t)millis();
    if (!lab3f8AutosteerGateOpen(now)) {
        portENTER_CRITICAL(&lab3f8Mux);
        lab3f8GateBlocked++;
        portEXIT_CRITICAL(&lab3f8Mux);
        return;
    }

    twai_message_t out;
    out.identifier       = src.identifier;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];

    bool changed = false;
    bool blindChanged = false;
    const uint8_t stockBlindBefore = (uint8_t)readBitsLE(out.data, 52, 2);

    if (alc == LAB3F8_ALC_FORCE_OFF) {
        if (getBit(out.data, 56)) {
            setBit(out.data, 56, false);
            changed = true;
        }
    } else if (alc == LAB3F8_ALC_FORCE_ON) {
        if (!getBit(out.data, 56)) {
            setBit(out.data, 56, true);
            changed = true;
        }
    }

    if (blind != LAB3F8_STOCK && blind <= 2) {
        const uint8_t stockBlind = (uint8_t)readBitsLE(out.data, 52, 2);
        if (stockBlind != blind) {
            writeBitsLE(out.data, 52, 2, blind);
            blindChanged = true;
            changed = true;
        }
    }

    // Dashboard exposes follow-distance labels 1..7 as raw values 0..6.
    // Raw 7 is SNA and is never selectable as a forced value.
    if (acc != LAB3F8_STOCK && acc <= 6) {
        const uint8_t stockAcc = (uint8_t)readBitsLE(out.data, 45, 3);
        if (stockAcc != acc) {
            writeBitsLE(out.data, 45, 3, acc);
            changed = true;
        }
    }

    // FORCE value already equals stock -> do not create a duplicate frame.
    if (!changed) return;

    if (!twaiNonSummonAdmissionOpen()) {
        portENTER_CRITICAL(&lab3f8Mux);
        lab3f8TxFail++;
        portEXIT_CRITICAL(&lab3f8Mux);
        return;
    }

    esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    researchCaptureObserveTxVh((uint16_t)out.identifier, out.data_length_code, out.data, err == ESP_OK);
    const uint8_t actualTxBlind = (uint8_t)readBitsLE(out.data, 52, 2);
    portENTER_CRITICAL(&lab3f8Mux);
    lab3f8LastTxValid = true;
    lab3f8LastTxBlind = actualTxBlind;
    lab3f8LastTxStockBlind = stockBlindBefore;
    lab3f8LastTxSelectedBlind = blind;
    lab3f8LastTxBlindChanged = blindChanged;
    lab3f8LastTxResult = (err == ESP_OK) ? 1 : 2;
    lab3f8LastTxMs = now;
    if (err == ESP_OK) lab3f8TxOk++;
    else               lab3f8TxFail++;
    portEXIT_CRITICAL(&lab3f8Mux);
}

static void injectSummon(const twai_message_t &src) {
    const uint32_t now = (uint32_t)millis();
    const uint32_t txEpoch = canTxEpochSnapshot();
    bool gate = false, fmode = false, summonTransportWanted = false;

    portENTER_CRITICAL(&stateMux);
    gate = summonInjectionGateOpen();
    fmode = forceMode;
    // Monitoring and transport selection are always active. This does not own
    // or mutate R79 bit18/bit19/bit47; R79 LAB is the sole owner of those bits.
    summonTransportWanted = fmode || gate;
    if (!gate && !fmode) {
      if (!gateAPActive && !gateParked && !gateSummoning)
        strncpy(gateBlockReason, "AP-,Park-,Summon-", sizeof(gateBlockReason));
    }
    portEXIT_CRITICAL(&stateMux);

    const bool snoozeWanted = ulcSnoozeRequestActive(now) && autoBlinkerNOAGateOpen(now);
    if (!summonTransportWanted && !snoozeWanted) return;

    twai_message_t out = src;
    out.flags = 0;
    bool changed = false;
    bool snoozeApplied = false;

    // R79 LAB split: Summon Monitor does not own bit18/bit19/bit47.
    // Keep legacy Summon state/capture/priority telemetry, but all R79/HardCore
    // injection is emitted only by r79LabPeriodicTick().
    if (summonTransportWanted) sumRxMux1++;

    // UI_ulcSnooze = 0x3FD mux1 bit36. Independent from Summon and R79 LAB.
    if (snoozeWanted) {
      if (!getBit(out.data, 36)) {
        setBit(out.data, 36, true);
        changed = true;
        snoozeApplied = true;
      } else {
        ulcSnoozeFinishRequest();
      }
    }

    if (!changed) return;

    esp_err_t err = ESP_ERR_TIMEOUT;
    if (summonTransportWanted) err = twaiTransmitSummonPriority(&out, txEpoch);
    else if (twaiNonSummonAdmissionOpen()) err = canTxTwaiTransmit(&out, txEpoch);

    if (err == ESP_OK) sumTxOk++;
    else               sumTxFail++;

    if (snoozeApplied) {
      portENTER_CRITICAL(&ulcSnoozeMux);
      if (err == ESP_OK) ulcSnoozeTxOk++;
      else               ulcSnoozeTxFail++;
      portEXIT_CRITICAL(&ulcSnoozeMux);
      if (err == ESP_OK) ulcSnoozeFinishRequest();
    }
}

// ── TLSSC Restore : 0x331 DAS_autopilotConfig (Standard 3/Y only) ──
// Reference firmware semantics verified for Standard 3/Y: preserve byte0 bits
// 7..6 and force the low six bits to 0x1B. Model Y L remains unsupported until
// its 0x331 field semantics are independently verified.
static void doInjectTlsscRestore(const twai_message_t &src) {
    if (!activeProfileTlsscRestoreSupported()) return;
    if (!bannedCar || !tlsscRestoreEnabled) return;
    if (src.extd || src.rtr || src.data_length_code < 1) return;

    const uint8_t patched0 = (uint8_t)((src.data[0] & 0xC0U) | 0x1BU);
    if (patched0 == src.data[0]) return;
    if (!twaiNonSummonAdmissionOpen()) return;

    twai_message_t out = src;
    out.flags = 0;
    out.data[0] = patched0;
    const uint32_t txEpoch = canTxEpochSnapshot();
    (void)canTxTwaiTransmit(&out, txEpoch);
}

// ── TLSSC : 0x3FD mux0 bit38/39 ──
// TLSSC has its own AP-active gate. It does not share or bypass
// the Parked/Summoning gate used by Summon Monitor.
static uint8_t r79LabGateReason(uint32_t now) {
  // Summon takes precedence over AP for transport selection. Tesla can expose
  // brief AP-state transients around a Summon session; a confirmed Summon must
  // still use the priority TX path instead of being downgraded to normal AP TX.
  bool summonActive = false;
  bool freshParked = false;
  portENTER_CRITICAL(&stateMux);
  // Fail closed for Summon: require the priority layer to have entered FULL from
  // a fresh Park context, plus current ACA+SPR activity or its short dropout grace.
  summonActive = (summonPriorityState == SUMMON_PRIORITY_FULL) &&
                 (gateSummoning || summonGateGraceActiveLocked(now));
  freshParked = summonPriorityFreshParkedLocked(now);
  portEXIT_CRITICAL(&stateMux);

  if (summonActive) return R79LAB_GATE_SUMMON;
  if (r79LabApGateOpen(now)) return R79LAB_GATE_AP;

  if (freshParked) return R79LAB_GATE_PARK;
  return R79LAB_GATE_BLOCKED;
}

static bool r79LabTransmitShadow(const uint8_t *stock, uint32_t now,
                                 uint8_t gateReason, uint8_t txKind, uint32_t txEpoch) {
  if (!stock || gateReason == R79LAB_GATE_BLOCKED) return false;

  // During Summon, use the existing priority transport so the reassertion is not
  // starved by non-Summon VH traffic. AP and Park traffic stays disposable,
  // non-blocking, and refuses to pile into a busy queue.
  if (gateReason != R79LAB_GATE_SUMMON &&
      twaiTxQueueNow >= TWAI_STANDBY_NON_SUMMON_QUEUE_LIMIT) {
    portENTER_CRITICAL(&r79LabMux); r79LabQueueSkip++; portEXIT_CRITICAL(&r79LabMux);
    return false;
  }

  twai_message_t out = {};
  out.identifier = 0x3FD;
  out.data_length_code = 8;
  out.flags = 0;
  memcpy(out.data, stock, 8);
  if (readMuxID(out.data) != 1 || !r79LabApplySelectedBits(out.data)) return false;

  // Deliberately transmit even when FORCE equals stock. This is a refresh /
  // reassertion experiment, not only an edge-triggered mutation.
  portENTER_CRITICAL(&r79LabMux); r79LabAppliedFrames++; portEXIT_CRITICAL(&r79LabMux);
  const esp_err_t err = (gateReason == R79LAB_GATE_SUMMON)
                          ? twaiTransmitSummonPriority(&out, txEpoch)
                          : canTxTwaiTransmit(&out, txEpoch);
  r79LabRecordTxResult(err == ESP_OK, out, txKind);
  if (err == ESP_OK) {
    portENTER_CRITICAL(&r79LabMux); r79LabLastTxMs = now; portEXIT_CRITICAL(&r79LabMux);
  }
  return err == ESP_OK;
}

static void r79LabImmediateFromStock(const uint8_t *data, uint8_t dlc, uint32_t now) {
  if (!labMenuEnabled || !data || dlc < 8 || readMuxID(data) != 1) return;
  const uint32_t txEpoch = canTxEpochSnapshot();

  const uint8_t gateReason = r79LabGateReason(now);
  portENTER_CRITICAL(&r79LabMux);
  r79LabLastGateReason = gateReason;
  portEXIT_CRITICAL(&r79LabMux);
  if (gateReason == R79LAB_GATE_BLOCKED) {
    portENTER_CRITICAL(&r79LabMux); r79LabGateBlocked++; portEXIT_CRITICAL(&r79LabMux);
    return;
  }

  // Immediate stock-RX injection is intentionally independent from the periodic
  // refresh phase. A fresh stock mux1 updates the shadow/template and is injected
  // immediately, but it does not delay or reset the selected periodic clock.
  r79LabTransmitShadow(data, now, gateReason, R79LAB_TX_IMMEDIATE, txEpoch);
}

static void r79LabPeriodicTick() {
  if (!labMenuEnabled) return;
  const uint32_t now = (uint32_t)millis();
  uint16_t period;
  uint32_t lastAttempt;
  portENTER_CRITICAL(&r79LabMux);
  period = r79LabPeriodMs;
  lastAttempt = r79LabLastAttemptMs;
  portEXIT_CRITICAL(&r79LabMux);
  if (!r79LabPeriodValid(period)) period = R79LAB_DEFAULT_PERIOD_MS;
  if ((uint32_t)(now - lastAttempt) < period) return;

  // Attempt at most once per selected period even if the gate/queue/TX rejects it.
  portENTER_CRITICAL(&r79LabMux);
  r79LabLastAttemptMs = now;
  portEXIT_CRITICAL(&r79LabMux);

  const uint32_t txEpoch = canTxEpochSnapshot();
  const uint8_t gateReason = r79LabGateReason(now);
  portENTER_CRITICAL(&r79LabMux);
  r79LabLastGateReason = gateReason;
  portEXIT_CRITICAL(&r79LabMux);
  if (gateReason == R79LAB_GATE_BLOCKED) {
    portENTER_CRITICAL(&r79LabMux); r79LabGateBlocked++; portEXIT_CRITICAL(&r79LabMux);
    return;
  }

  uint8_t stock[8];
  bool valid;
  portENTER_CRITICAL(&r79LabMux);
  valid = r79LabStockValid;
  memcpy(stock, r79LabLastStockRaw, sizeof(stock));
  portEXIT_CRITICAL(&r79LabMux);
  if (!valid) {
    // Never synthesize mux1 from constants. Periodic refresh starts only after
    // this CAN epoch has observed at least one real stock mux1 template. Once
    // captured, the latest template remains valid until CAN state/reinit clears it.
    portENTER_CRITICAL(&r79LabMux); r79LabNoTemplateSkip++; portEXIT_CRITICAL(&r79LabMux);
    return;
  }

  r79LabTransmitShadow(stock, now, gateReason, R79LAB_TX_PERIODIC, txEpoch);
}

static void injectTLSSC(const twai_message_t &src) {
    const uint32_t now = (uint32_t)millis();
    if (!tlsscEnabled) return;
    const uint32_t txEpoch = canTxEpochSnapshot();
    bool en, ap;
    portENTER_CRITICAL(&stateMux);
    en = tlsscEnabled;
    ap = gateAPActive;
    portEXIT_CRITICAL(&stateMux);

    if (!en || !ap)
        return;

    if (tlsscHighwayGateBlocked(now)) {
      portENTER_CRITICAL(&roadContextMux);
      tlsscHighwayGateBlockedCount++;
      portEXIT_CRITICAL(&roadContextMux);
      return;
    }

    twai_message_t out;
    out.identifier       = src.identifier;
    out.data_length_code = src.data_length_code;
    out.flags            = 0;
    for (int i = 0; i < 8; i++) out.data[i] = src.data[i];
    // Avoid a duplicate if the incoming mux0 already contains both values.
    if (getBit(out.data, 38) && getBit(out.data, 39)) return;

    setBit(out.data, 38, true);   // UI_fsdStopsControlEnabled = 1
    setBit(out.data, 39, true);   // UI_fsdContinueOnGreenWithCIPV = 1

    // TLSSC is lower priority than Summon and must never block CAN B RX.
    if (!twaiNonSummonAdmissionOpen()) {
      sumTxFail++;
      return;
    }
    esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    if (err == ESP_OK) sumTxOk++;
    else               sumTxFail++;
}

// ═══════════════════════════════════════════════════════════════
// NVS schema migration
// Keep obsolete-key cleanup one-shot instead of repeating remove() calls on every boot.
// Schema 1 retires legacy Summon/R79 keys and canonicalizes S3XY 3D49 identity as raw bN bytes.
// ═══════════════════════════════════════════════════════════════
static constexpr uint16_t NVS_SCHEMA_CURRENT = 1;
static uint16_t nvsSchemaVersion = 0;

static void nvsSchemaRead() {
  Preferences p;
  uint16_t version = 0;
  if (p.begin("t2meta", true)) {
    version = p.getUShort("schema", 0);
    p.end();
  }
  nvsSchemaVersion = version;
}

static void nvsSchemaFinalize() {
  if (nvsSchemaVersion >= NVS_SCHEMA_CURRENT) return;

  bool ok = true;
  Preferences p;
  if (p.begin("summon", false)) {
    p.remove("en");
    p.remove("logic");
    p.remove("tlrst");
    p.remove("ulcbs");
    p.remove("ulcsp");
    // Keep blkDly17 until the new schema marker is committed. If the marker
    // write fails, the legacy delay migration remains idempotent on next boot.
    p.end();
  } else {
    ok = false;
  }

  if (p.begin("r79lab", false)) {
    p.remove("parkInj");
    p.remove("apply");
    p.remove("hard47");
    p.end();
  } else {
    ok = false;
  }

  // s3xy/addr is a legacy single-device target. s3xyAutoLoadConfig() runs
  // before this finalizer and migrates it into the registry when needed.
  if (p.begin("s3xy", false)) {
    if (s3xyRegisteredCount() > 0) p.remove("addr");
    p.end();
  } else {
    ok = false;
  }

  // Raw bN is the canonical persisted 3D49 identity. Remove the legacy iN
  // string only after the raw bytes can be read back from NVS successfully.
  if (p.begin("s3xyreg", false)) {
    for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
      String sb = "b" + String(i);
      String si = "i" + String(i);
      const size_t rawLen = p.getBytesLength(sb.c_str());
      if (rawLen == 0 || rawLen > S3XY_PEER_ID_MAX) continue;
      uint8_t raw[S3XY_PEER_ID_MAX] = {};
      const size_t got = p.getBytes(sb.c_str(), raw, sizeof(raw));
      if (got == rawLen) p.remove(si.c_str());
    }
    p.end();
  } else {
    ok = false;
  }

  if (!ok) {
    Serial.println("NVS schema migration incomplete; will retry next boot");
    return;
  }

  if (p.begin("t2meta", false)) {
    const size_t written = p.putUShort("schema", NVS_SCHEMA_CURRENT);
    p.end();
    if (written > 0) {
      nvsSchemaVersion = NVS_SCHEMA_CURRENT;
      Preferences cleanup;
      if (cleanup.begin("summon", false)) {
        cleanup.remove("blkDly17");
        cleanup.end();
      }
      Serial.printf("NVS schema migrated to %u\n", (unsigned)NVS_SCHEMA_CURRENT);
      return;
    }
  }
  Serial.println("NVS schema marker write failed; will retry next boot");
}

static void featureCfgLoad() {
    prefs.begin("features", false);
    labMenuEnabled = prefs.getBool("lab", false);
    bannedCar = prefs.getBool("banned", false);
    doorOpenCancelEnabled = prefs.getBool("doorCancel", false);
    tlsscRestoreEnabled = prefs.getBool("tlRestore", false);
    if (!activeProfileBodyControlsSupported() && !activeProfileIsYl() && doorOpenCancelEnabled) {
      // Door-open cancel consumes Body CAN on Standard 3/Y. A topology change
      // to Party+Chassis must not retain an enabled-but-unreachable feature.
      doorOpenCancelEnabled = false;
      prefs.putBool("doorCancel", false);
    }
    if (!activeProfileBannedCarSupported()) {
      // Model Y L does not expose Banned Car in v3.1 hotfix. Fail closed if an older
      // profile/settings combination left either flag persisted. Avoid writing
      // unchanged false values on every YL boot.
      const bool persistedBanned = bannedCar;
      const bool persistedRestore = tlsscRestoreEnabled;
      bannedCar = false;
      tlsscRestoreEnabled = false;
      if (persistedBanned) prefs.putBool("banned", false);
      if (persistedRestore) prefs.putBool("tlRestore", false);
    } else if (!bannedCar && tlsscRestoreEnabled) {
      // Persisted-state invariant: Restore may never remain ON while Banned Car is OFF.
      tlsscRestoreEnabled = false;
      prefs.putBool("tlRestore", false);
    }
    if (!activeProfileTlsscRestoreSupported() && tlsscRestoreEnabled) {
      tlsscRestoreEnabled = false;
      prefs.putBool("tlRestore", false);
    }
    prefs.end();
}

static void featureCfgSave() {
    prefs.begin("features", false);
    prefs.putBool("lab", labMenuEnabled);
    prefs.putBool("banned", activeProfileBannedCarSupported() ? bannedCar : false);
    prefs.putBool("doorCancel", (activeProfileBodyControlsSupported() || activeProfileIsYl()) ? doorOpenCancelEnabled : false);
    prefs.putBool("tlRestore", (activeProfileBannedCarSupported() && bannedCar && activeProfileTlsscRestoreSupported()) ? tlsscRestoreEnabled : false);
    prefs.end();
}

static void summonCfgLoad() {
    prefs.begin("summon", false);
    tlsscEnabled  = prefs.getBool("tlssc", false);
    tlsscHighwayGateEnabled = prefs.getBool("tlHwy", false);
    blinkAEnabled = prefs.getBool("blkA", false);

    // Pre-schema builds used blkDly17 as a one-time migration marker. Preserve
    // that behavior exactly once when upgrading directly from those versions.
    if (nvsSchemaVersion < 1) {
      const bool delayMigratedRev17 = prefs.getBool("blkDly17", false);
      if (!delayMigratedRev17) {
        blinkADelayMs = BLINKA_AUTO_DELAY_DEFAULT_MS;
        prefs.putUInt("blkADly", blinkADelayMs);
      } else {
        blinkADelayMs = prefs.getUInt("blkADly", BLINKA_AUTO_DELAY_DEFAULT_MS);
      }
    } else {
      blinkADelayMs = prefs.getUInt("blkADly", BLINKA_AUTO_DELAY_DEFAULT_MS);
    }
    blinkADelayMs = constrain((uint32_t)blinkADelayMs, (uint32_t)0, (uint32_t)30000);
    prefs.end();
}

static void summonCfgSave() {
    prefs.begin("summon", false);
    prefs.putBool("tlssc", tlsscEnabled);
    prefs.putBool("tlHwy", tlsscHighwayGateEnabled);
    prefs.putBool("blkA", blinkAEnabled);
    prefs.putUInt("blkADly", blinkADelayMs);
    prefs.end();
}


static void lab3f8CfgLoad() {
    prefs.begin("lab3f8", false);
    uint8_t alc = prefs.getUChar("alc", LAB3F8_ALC_STOCK);
    uint8_t blind = prefs.getUChar("ulcbs", LAB3F8_STOCK);
    uint8_t acc = prefs.getUChar("accfd", LAB3F8_STOCK);

    // v2.7b11 stored FUSED LEFT TEST as alc=3 even though it acted on 0x3F8.
    // Normalize that obsolete selection to STOCK, but DO NOT auto-enable the
    // new direct Party 0x399 experiment. It must be explicitly enabled by the
    // user after updating because its TX semantics are materially different.
    if (alc == 3) {
      alc = LAB3F8_ALC_STOCK;
      prefs.putUChar("alc", alc);
    }

    // v2.8a2 promotes Off-Highway ALC to a production BETA with only ON/OFF.
    // The former FORCE OFF selection is no longer a user mode: OFF means pure STOCK.
    if (alc == LAB3F8_ALC_FORCE_OFF) {
      alc = LAB3F8_ALC_STOCK;
      prefs.putUChar("alc", alc);
    }
    prefs.end();

    if (alc > LAB3F8_ALC_FORCE_ON) alc = LAB3F8_ALC_STOCK;
    if (blind != LAB3F8_STOCK && blind > 2) blind = LAB3F8_STOCK;
    if (acc != LAB3F8_STOCK && acc > 6) acc = LAB3F8_STOCK;

    portENTER_CRITICAL(&lab3f8Mux);
    lab3f8AlcMode = alc;
    lab3f8UlcBlindMode = blind;
    lab3f8AccFollowRaw = acc;
    portEXIT_CRITICAL(&lab3f8Mux);
}

static void lab3f8CfgSave() {
    uint8_t alc, blind, acc;
    portENTER_CRITICAL(&lab3f8Mux);
    alc = lab3f8AlcMode;
    blind = lab3f8UlcBlindMode;
    acc = lab3f8AccFollowRaw;
    portEXIT_CRITICAL(&lab3f8Mux);

    prefs.begin("lab3f8", false);
    prefs.putUChar("alc", alc);
    prefs.putUChar("ulcbs", blind);
    prefs.putUChar("accfd", acc);
    prefs.end();
}


static void r79LabCfgLoad() {
    prefs.begin("r79lab", false);
    uint8_t smart = prefs.getUChar("smart18", R79LAB_STOCK);
    uint16_t period = prefs.getUShort("period", R79LAB_DEFAULT_PERIOD_MS);
    prefs.end();
    if (smart > R79LAB_FORCE_1) smart = R79LAB_STOCK;
    if (!r79LabPeriodValid(period)) period = R79LAB_DEFAULT_PERIOD_MS;
    portENTER_CRITICAL(&r79LabMux);
    r79LabSmartMode = smart;
    r79LabPeriodMs = period;
    portEXIT_CRITICAL(&r79LabMux);
}

static void r79LabCfgSave() {
    uint8_t smart;
    uint16_t period;
    portENTER_CRITICAL(&r79LabMux);
    smart = r79LabSmartMode;
    period = r79LabPeriodMs;
    portEXIT_CRITICAL(&r79LabMux);
    prefs.begin("r79lab", false);
    prefs.putUChar("smart18", smart);
    prefs.putUShort("period", period);
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
