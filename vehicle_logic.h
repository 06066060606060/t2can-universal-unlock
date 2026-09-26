#pragma once

// VEHICLE FEATURE LOGIC / R79 / SUMMON / ALC / VH RECORDER
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// R79 POLICY / SUMMON UNLOCK
// v3.6 beta 2: R79 is default-on after a real stock 0x3FD mux1 template
// is captured. Injection is suspended only for positively confirmed manual
// Drive/Reverse. AP, Summon, Park, Neutral and unknown/stale state remain ON.
// bit19 is fixed 0, bit47 fixed 1; bit18 remains an independent LAB option.
// ═══════════════════════════════════════════════════════════════
static constexpr uint8_t R79LAB_APPLY_ECE_BIT = 19;
static constexpr uint8_t R79LAB_SMART_SUMMON_CANDIDATE_BIT = 18;
static constexpr uint8_t R79LAB_HARD_CORE_SUMMON_BIT = 47;

static constexpr uint8_t R79LAB_TX_NONE = 0;
static constexpr uint8_t R79LAB_TX_IMMEDIATE = 1;
static constexpr uint8_t R79LAB_TX_PERIODIC = 2;
static constexpr uint8_t R79LAB_TX_RETRY = 3;

enum R79RuntimeTxState : uint8_t {
  R79_TX_STATE_ACTIVE = 0,
  R79_TX_STATE_SUSPENDED = 1,
  R79_TX_STATE_WAIT_TEMPLATE = 2,
  R79_TX_STATE_CAN_OFFLINE = 3,
  R79_TX_STATE_ADMIN_HOLD = 4
};

static constexpr uint8_t R79LAB_BLOCK_NONE = 0;
static constexpr uint8_t R79LAB_BLOCK_MANUAL = 1;
static constexpr uint8_t R79LAB_BLOCK_TEMPLATE = 2;
static constexpr uint8_t R79LAB_BLOCK_CAN_OFFLINE = 3;
static constexpr uint8_t R79LAB_BLOCK_ADMIN_HOLD = 4;
static constexpr uint8_t R79LAB_BLOCK_TX = 5;

struct R79RuntimeStatus {
  uint8_t state;
  R79TxDecisionPure decision;
  bool gearValid;
  uint8_t gearRaw;
  bool dasValid;
  uint8_t dasState4;
  bool summonSessionActive;
  bool remoteStartupEvidence;
  bool manualLatchActive;
};

static portMUX_TYPE r79LabMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t r79Bit18Policy = R79_BIT18_DEFAULT_PURE;
static volatile uint32_t r79LabLastAttemptMs = 0;
static volatile uint32_t r79LabLastPeriodicRequestMs = 0;
static volatile uint32_t r79LabLastPeriodicTxMs = 0;
static volatile uint32_t r79LabLastTxMs = 0;
static volatile uint32_t r79LabNoTemplateSkip = 0;
static volatile uint32_t r79LabManualSuspendSkip = 0;
static volatile uint32_t r79LabCanOfflineSkip = 0;
static volatile uint32_t r79LabAdminHoldSkip = 0;
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
static volatile bool r79RetryPending = false;
static volatile uint8_t r79RetryIndex = 0;
static volatile uint8_t r79RetryOriginKind = R79LAB_TX_NONE;
static volatile uint32_t r79RetryDueMs = 0;
static volatile uint32_t r79RetryScheduled = 0;
static volatile uint32_t r79RetryTxOk = 0;
static volatile uint32_t r79RetryTxFail = 0;
static volatile uint32_t r79RetryExhausted = 0;
static volatile uint32_t r79LastRetryMs = 0;
static volatile uint32_t r79EmergencyQueueFlushCount = 0;
static volatile uint32_t r79FlushTriggeredRetryOk = 0;
static volatile uint32_t r79FlushTriggeredRetryFail = 0;
static R79FixedQuietStatePure r79FixedQuietState = {};
static volatile uint32_t r79QuietArmCount = 0;
static volatile uint32_t r79QuietCancelCount = 0;
static volatile uint32_t r79QuietDueCount = 0;
static volatile uint32_t r79QuietFireCount = 0;
static volatile uint32_t r79QuietGuardSkip = 0;
static volatile uint32_t r79QuietDisallowedSkip = 0;
static volatile uint32_t r79PeriodicRetryCancelledByStock = 0;
static volatile uint32_t r79PeriodicSlotDue = 0;
static volatile uint32_t r79PeriodicSlotFire = 0;
static volatile uint32_t r79PeriodicSlotGuard = 0;
static volatile uint8_t r79LabLastTxKind = R79LAB_TX_NONE;
static volatile bool r79LabLastTxValid = false;
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
static volatile uint32_t r79LabAppliedFrames = 0;
// v3.6d2 fast reactive echo telemetry. Latency is measured from the point the
// CAN-B task dequeues the received 0x3FD mux1 frame from TWAI to the instant
// the replacement TX request is submitted. It is intentionally not labeled as
// physical wire RX->TX latency because the classic ESP32 TWAI API exposes no
// hardware RX timestamp.
static volatile uint32_t r79FastEchoAttempts = 0;
static volatile uint32_t r79FastEchoTxOk = 0;
static volatile uint32_t r79FastEchoTxFail = 0;
static volatile uint32_t r79FastEchoBlocked = 0;
static volatile uint32_t r79FastEchoLatencyLastUs = 0;
static volatile uint32_t r79FastEchoLatencyMinUs = 0;
static volatile uint32_t r79FastEchoLatencyMaxUs = 0;
static uint64_t r79FastEchoLatencyTotalUs = 0;
static volatile uint32_t r79FastEchoLatencySamples = 0;
static volatile uint32_t r79FastEchoLt100Us = 0;
static volatile uint32_t r79FastEchoLt250Us = 0;
static volatile uint32_t r79FastEchoLt1000Us = 0;
static volatile uint32_t r79FastEchoGe1000Us = 0;
// d3 best-effort wire-completion telemetry. A sample is armed only when the
// TWAI TX pipeline was observed idle immediately before the fast R79 enqueue.
// It is then accepted only if no later CAN-B application enqueue occurred
// before a TX_SUCCESS alert is observed. The timestamp is therefore a
// software-observed upper bound, not a hardware wire timestamp.
static volatile bool r79FastSuccessPending = false;
static volatile uint32_t r79FastSuccessAcceptedSerial = 0;
static int64_t r79FastSuccessRxDequeueUs = 0;
static int64_t r79FastSuccessRequestUs = 0;
static volatile uint32_t r79FastSuccessSamples = 0;
static volatile uint32_t r79FastSuccessAmbiguous = 0;
static volatile uint32_t r79FastSuccessLastRxUs = 0;
static volatile uint32_t r79FastSuccessMinRxUs = 0;
static volatile uint32_t r79FastSuccessMaxRxUs = 0;
static uint64_t r79FastSuccessTotalRxUs = 0;
static volatile uint32_t r79FastSuccessLastReqUs = 0;
static volatile uint32_t r79FastSuccessMinReqUs = 0;
static volatile uint32_t r79FastSuccessMaxReqUs = 0;
static uint64_t r79FastSuccessTotalReqUs = 0;
static volatile uint8_t r79LabLastBlockReason = R79LAB_BLOCK_NONE;
static volatile uint32_t r79LabLastBlockMs = 0;
static uint8_t r79LabLastStockRaw[8] = {0};
static uint8_t r79LabLastEffectiveRaw[8] = {0};
static volatile bool r79Lab7ffSeen = false;
static volatile uint8_t r79Lab7ffBus = T2CAN_BUS_VH;
static volatile uint8_t r79Lab7ffPage = 0xFF;
static volatile uint32_t r79Lab7ffRx = 0;
static volatile uint32_t r79Lab7ffLastMs = 0;
static uint8_t r79Lab7ffRaw[8] = {0};

static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now);
static void r79LabRecordSuppressedState(const R79RuntimeStatus &status, uint32_t now);
static bool r79LabApplySelectedBits(uint8_t *data);
static void r79LabRecordTxResult(bool ok, const twai_message_t &out, uint8_t txKind);
static bool r79LabTransmitShadow(const uint8_t *stock, uint32_t now, uint8_t txKind);
static bool r79FixedFastEcho(const twai_message_t &src, int64_t rxDequeueUs);
static void r79FixedObserveStock(uint8_t mux, uint32_t nowMs);
static void r79FixedTick();
static bool r79CopyLatestStock(uint8_t out[8]);
static void r79LabRetryTick();
static bool r79LabTransmitOnce(const uint8_t *stock, uint32_t now,
                               uint8_t txKind, esp_err_t *errOut);

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





static const char* r79RuntimeStateName(uint8_t state) {
  switch (state) {
    case R79_TX_STATE_ACTIVE: return "ACTIVE";
    case R79_TX_STATE_SUSPENDED: return "SUSPENDED";
    case R79_TX_STATE_WAIT_TEMPLATE: return "WAIT TEMPLATE";
    case R79_TX_STATE_CAN_OFFLINE: return "CAN OFFLINE";
    case R79_TX_STATE_ADMIN_HOLD: return "ADMIN HOLD";
    default: return "UNKNOWN";
  }
}

static const char* r79GearName(uint8_t raw) {
  switch (raw) {
    case TESLA_GEAR_P: return "P";
    case TESLA_GEAR_R: return "R";
    case TESLA_GEAR_N: return "N";
    case TESLA_GEAR_D: return "D";
    default: return "UNKNOWN";
  }
}

static const char* r79LabBlockReasonName(uint8_t reason) {
  switch (reason) {
    case R79LAB_BLOCK_MANUAL: return "MANUAL_DRIVING";
    case R79LAB_BLOCK_TEMPLATE: return "NO_TEMPLATE";
    case R79LAB_BLOCK_CAN_OFFLINE: return "CAN_OFFLINE";
    case R79LAB_BLOCK_ADMIN_HOLD: return "ADMIN_HOLD";
    case R79LAB_BLOCK_TX: return "TX_ERROR";
    default: return "NONE";
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

  // Fast Echo runs before capture/decoder work. This observer updates only the
  // accepted stock template and production counters afterward.
}

static bool r79LabApplySelectedBits(uint8_t *data) {
  if (!data) return false;
  uint8_t bit18Policy;
  portENTER_CRITICAL(&r79LabMux);
  bit18Policy = r79Bit18Policy;
  portEXIT_CRITICAL(&r79LabMux);
  r79FixedApplyBitsPure(data, bit18Policy);
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
    if (gear == TESLA_GEAR_P) return 1;
    if (gear == TESLA_GEAR_R || gear == TESLA_GEAR_N || gear == TESLA_GEAR_D) return 0;
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

static volatile bool tlsscEnabled  = false;   // "Enable TLSSC" - off by default
static volatile bool tlsscHighwayGateEnabled = false; // experimental 0x238 controlled-access gate
static volatile bool tlsscBlockInNoa = false; // keep TLSSC available in AUTOSTEER but clear it during NOA
// Runtime ownership of the TLSSC overlay. Once this firmware has actually
// transmitted bit38/39=1, every transition out of the active TLSSC policy is
// followed by one explicit bit38/39=0 frame before returning to pure STOCK.
static volatile bool tlsscInjectedActive = false;
static volatile bool tlsscClearPending = false;
static volatile uint32_t tlsscClearTxOk = 0;
static volatile uint32_t tlsscClearTxFail = 0;

static volatile bool gateAPActive  = false;
static volatile bool gateNOAActive = false;   // raw DAS_autopilotState == ACTIVE_NAV (5)
static volatile uint8_t dasAutopilotState4 = 0xFF; // low nibble of Party-CAN 0x399 byte0
static volatile bool dasAutopilotStateValid = false; // latched until CAN A recovery / next valid 0x399
static volatile uint32_t lastDASStatusMillis = 0;  // diagnostic age only; not a functional AP timeout

// AP right-scroll pulse on CAN B 0x3C2 mux1. Model YL uses VH; Standard
// profiles use Chassis. Configuration is NVS-backed; scheduler state is not.
static portMUX_TYPE apRightScrollMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool apRightScrollEnabled = false;
static volatile uint16_t apRightScrollIntervalSeconds = AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE;
static volatile uint8_t apRightScrollVisualRepeatSeconds = AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE;
static ApRightScrollStatePure apRightScrollState = {};
static volatile uint32_t apRightScrollMux1Rx = 0;
static volatile uint32_t apRightScrollPhysicalDeferrals = 0;
static volatile uint32_t apRightScrollTxUpOk = 0;
static volatile uint32_t apRightScrollTxDownOk = 0;
static volatile uint32_t apRightScrollTxFail = 0;
static volatile uint32_t apRightScrollLastTxMs = 0;
static volatile uint8_t apRightScrollLastTxValue = 0;

static void apRightScrollRuntimeReset(bool resetCounters) {
  portENTER_CRITICAL(&apRightScrollMux);
  apRightScrollState = {};
  if (resetCounters) {
    apRightScrollMux1Rx = 0;
    apRightScrollPhysicalDeferrals = 0;
    apRightScrollTxUpOk = 0;
    apRightScrollTxDownOk = 0;
    apRightScrollTxFail = 0;
    apRightScrollLastTxMs = 0;
    apRightScrollLastTxValue = 0;
  }
  portEXIT_CRITICAL(&apRightScrollMux);
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
static volatile bool gateParked    = true;
static volatile bool gateSummoning = false;
static volatile bool sprSeen  = false;
static volatile bool lastAca  = false;
static volatile bool acaValid = false;
static volatile uint32_t lastAcaMillis = 0;
static volatile bool sprValid = false;
static volatile uint32_t lastSprMillis = 0;
static volatile uint8_t lastSprRaw = 0;
static volatile uint32_t last280Millis = 0;
static constexpr uint32_t PARKED_TIMEOUT_MS = 5000;
static volatile uint8_t summonGearSource = SUMMON_GEAR_NONE;
static volatile uint32_t summonGearObservedMs = 0;
// Confirmed gear is RAM-only and scoped to the current MCU boot/profile. A real
// decoded P or D/R/N updates it; source silence and local CAN recovery do not.
// This preserves remote wake compatibility without ever letting timeout turn a
// confirmed D/R/N back into PARK.
static SummonConfirmedGearLatchPure summonConfirmedGearLatch = {};
static volatile uint32_t summonConfirmedGearTransitions = 0;

// Fresh source observations remain explicit. These windows are not R79
// authorization gates. They are used only to avoid declaring a Summon startup
// transition as manual driving before the sticky ACA+SPR session is confirmed.
static constexpr uint32_t SUMMON_GEAR_FRESH_MS = 3000;
static constexpr uint32_t SUMMON_ACA_FRESH_MS = 750;
static constexpr uint32_t SUMMON_SPR_FRESH_MS = 2500;

// Raw gear observations used by both Summon diagnostics and the R79 manual
// suppression latch. 0x118 is primary; 0x186 is profile-gated fallback.
static volatile int8_t gear118State = -1;
static volatile int8_t gear186State = -1;
static volatile uint8_t gear118Raw = TESLA_GEAR_INVALID;
static volatile uint8_t gear186Raw = TESLA_GEAR_INVALID;
static volatile uint32_t gear118Ms = 0;
static volatile uint32_t gear186Ms = 0;

// v3.6b2: once manual D/R is positively confirmed, keep R79 suspended across
// transient unknown DAS/gear observations. AP, confirmed Summon, P/N, profile
// reset or relevant CAN recovery clears this latch.
static R79ManualSuppressionPure r79ManualSuppression = {};

static volatile uint32_t sumTxOk     = 0;
static volatile uint32_t sumTxFail   = 0;
static volatile uint32_t sumRx280    = 0;
static volatile uint32_t sumRx390    = 0;
static volatile uint32_t sumRx921    = 0;
static volatile uint32_t sumRx1016   = 0;
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
#define UI_CHASSIS_CONTROL_ID 0x293
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
#define BLINKA_S3XY_STOCK_TIMEOUT_MS 250
#define BLINKA_AUTO_DELAY_DEFAULT_MS 2000
#define BLINKA_RETRY_PERIOD_MS      250
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
static uint8_t realRaw249[8] = {0};
static volatile bool doorButtonPressed = false;


// 0x238 UI_driverAssistMapData road-context telemetry (VH/Chassis CAN).
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

// Auto blinker state.
static volatile bool blinkAEnabled = false;
static volatile uint8_t activeTurn = STALK_IDLE;
static volatile uint8_t lastReqDir = 0;
// Keep delayed trigger timing separate from pulse lifetime
// and the active SCCM one-shot pulse. 0x24A state changes may cancel a
// pending trigger, but must not truncate an already-started 350 ms pulse.
static volatile uint8_t oneShotTurn = STALK_IDLE;
// True only for an explicit S3XY Left/Right Blinker command. Auto Blinker
// one-shots leave this false so their NOA/ALC/Confirm-Free policy remains intact.
static volatile bool oneShotDirect = false;
static volatile uint32_t oneShotUntil = 0;
static volatile uint32_t oneShotReleaseAt = 0;
static volatile uint8_t oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
static volatile uint8_t blinkerTxMode = BLINKER_TX_MODE_SINGLE_PURE;
static BlinkerTxRequestStatePure blinkerTxRequestState = {};
static volatile uint32_t blinkerTxRequestCount = 0;
static volatile uint32_t blinkerTxBlockedCount = 0;
static volatile uint8_t blinkerTxLastDir = 0;
static volatile uint8_t blinkerTxLastSource = BLINKER_TX_SOURCE_NONE_PURE;
static volatile uint8_t blinkerTxLastResult = 0;  // 0=NONE, 1=PENDING, 2=OK, 3=FAIL, 4=BLOCKED
static volatile uint32_t s3xyBlinkerEventToken = 0;
static volatile uint32_t autoBlinkerSessionToken = 0;
static volatile uint8_t autoPendingDir = 0;
static volatile uint32_t blinkADelayMs = BLINKA_AUTO_DELAY_DEFAULT_MS;
static volatile uint8_t blinkANoaStabilizationSeconds =
    BLINKA_NOA_STABILIZE_DEFAULT_S_PURE;
static volatile uint8_t blinkACancelPauseSeconds =
    BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE;
static AutoBlinkerTimingStatePure autoBlinkerTimingState = {};
static volatile uint32_t autoFireAt = 0;
static volatile uint32_t autoRetryAt = 0;
static volatile uint32_t autoRequestLastSeenMs = 0;
static volatile bool autoArmed = false;
static volatile uint32_t autoRetryCount = 0;
static volatile uint32_t blkATxOk = 0;
static volatile uint32_t blkATxFail = 0;

// CAN B telemetry used by the auto blinker.
static volatile uint8_t visualBehaviorType = 0;
static volatile uint32_t visualDebugRxCount = 0;
static volatile uint32_t visualDebugLastMs = 0;

// Auto Blinker request-session tracking. A fresh 0x24A LEFT/RIGHT request is
// latched even when its lane is temporarily ineligible. NOA loss, an opposite
// request, CAN reset, or a sustained request absence ends the session; ALC
// blocking merely defers FIRE and is retried at a bounded cadence.
static constexpr uint32_t BLINKA_REQUEST_FRESH_MS = 2000;

static bool autoBlinkerPlannerGateOpen(uint32_t now) {
  if (!autoBlinkerNOAGateOpen(now)) return false;
  bool ready, paused;
  portENTER_CRITICAL(&blinkAMux);
  ready = autoBlinkerNoaReadyPure(autoBlinkerTimingState, now);
  const bool pauseWasActive = autoBlinkerTimingState.cancelPauseActive;
  paused = autoBlinkerPauseActivePure(autoBlinkerTimingState, now);
  if (pauseWasActive && !paused) {
    // A manual release or natural expiry begins a fresh planner request
    // session instead of leaving the cancelled direction latched forever.
    lastReqDir = 0;
    autoRequestLastSeenMs = 0;
  }
  portEXIT_CRITICAL(&blinkAMux);
  return ready && !paused;
}

// Caller must hold blinkAMux. This clears only the not-yet-fired request
// session; it deliberately does not touch oneShotTurn or the request-history
// latch used to prevent duplicate pulses for one continuous planner request.
static inline void autoBlinkerClearPendingLocked() {
  autoArmed = false;
  autoPendingDir = 0;
  autoFireAt = 0;
  autoRetryAt = 0;
}

// Caller holds blinkAMux. Zero is reserved for callers that deliberately do
// not request token-based duplicate suppression.
static inline uint32_t blinkerNextEventTokenLocked(volatile uint32_t &token) {
  token++;
  if (token == 0) token++;
  return token;
}

static inline void blinkerTxClearPendingSourceLocked(uint8_t source) {
  if (blinkerTxRequestState.pendingSource != source) return;
  blinkerTxRequestState.pendingDir = 0;
  blinkerTxRequestState.pendingSource = BLINKER_TX_SOURCE_NONE_PURE;
  blinkerTxRequestState.requestedMs = 0;
}

static inline void blinkerTxRecordLocked(uint8_t dir, uint8_t source,
                                         uint8_t result) {
  blinkerTxLastDir = dir;
  blinkerTxLastSource = source;
  blinkerTxLastResult = result;
}

static bool autoBlinkerALCAllowsDirection(uint8_t reqDir, uint32_t now, uint8_t *alcOut = nullptr) {
  uint8_t alc;
  bool valid;
  portENTER_CRITICAL(&stateMux);
  alc = dasAutoLaneChangeState;
  valid = dasAutoLaneChangeStateValid;
  portEXIT_CRITICAL(&stateMux);

  if (alcOut) *alcOut = alc;
  if (!valid) return false;

  bool roadValid, navRoute, leftOffRamp, rightOffRamp;
  uint32_t roadLast;
  portENTER_CRITICAL(&roadContextMux);
  roadValid = roadContextValid;
  navRoute = roadContextNavRouteActive;
  leftOffRamp = roadContextLeftOffRamp;
  rightOffRamp = roadContextRightOffRamp;
  roadLast = roadContextLastRxMs;
  portEXIT_CRITICAL(&roadContextMux);
  const bool roadFresh = roadValid && roadLast != 0 &&
                         (uint32_t)(now - roadLast) <= ROAD_CONTEXT_FRESH_MS;

  return autoBlinkerAlcAllowsDirectionPure(reqDir, alc, roadFresh, navRoute,
                                           leftOffRamp, rightOffRamp);
}

// Returns the fresh LEFT(1)/RIGHT(2) planner request without applying ALC
// eligibility. Used to ARM, detect an explicit opposite request, and preserve
// a pending request across temporary 0x24A IN_LANE/stale gaps.
static uint8_t autoBlinkerCurrentRequestDir(
    uint32_t now, uint32_t *requestLastRxOut = nullptr) {
  if (!activeProfileAdvancedEapSupported()) return 0;
  bool en;
  uint8_t behavior;
  uint32_t visualLast;
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  behavior = visualBehaviorType;
  visualLast = visualDebugLastMs;
  portEXIT_CRITICAL(&blinkAMux);

  if (requestLastRxOut) *requestLastRxOut = visualLast;
  if (!en || !autoBlinkerPlannerGateOpen(now)) return 0;
  if (visualLast == 0 || (uint32_t)(now - visualLast) > BLINKA_REQUEST_FRESH_MS) return 0;
  if (behavior == 2) return 1;
  if (behavior == 3) return 2;
  return 0;
}

// Manual NOA-cancel context gate shared by S3XY and the driver-door button.
// The cancel path intentionally does NOT inspect DAS_behaviorType. A fresh
// 0x24A frame is retained only as a fail-closed CAN-context freshness check.
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

static constexpr uint8_t AUTO_BLINKER_CANCEL_SOURCE_S3XY = 1u;
static constexpr uint8_t AUTO_BLINKER_CANCEL_SOURCE_DOOR = 2u;

static AutoBlinkerCancelActionPure handleAutoBlinkerCancelToggle(
    uint8_t source, bool edgeTriggered) {
  if (!edgeTriggered) return AUTO_BLINKER_CANCEL_REJECTED_PURE;
  const uint32_t now = (uint32_t)millis();
  const bool noaOpen = autoBlinkerNOAGateOpen(now);
  uint32_t visualLast;
  uint8_t pauseSeconds;
  portENTER_CRITICAL(&blinkAMux);
  visualLast = visualDebugLastMs;
  pauseSeconds = blinkACancelPauseSeconds;
  portEXIT_CRITICAL(&blinkAMux);
  const uint32_t visualAge = visualLast == 0
      ? UINT32_MAX : (uint32_t)(now - visualLast);

  // Hotfix: manual cancel must not depend on DAS_behaviorType. At the first
  // Tesla lane-change notification the planner can still report IN_LANE, so
  // requiring LEFT/RIGHT here drops a legitimate user cancel. Keep the
  // existing NOA and 0x24A freshness gates, but treat behaviorType as irrelevant.
  const bool eligible = noaOpen && visualLast != 0 &&
      visualAge <= ULC_REQUEST_FRESH_MS;

  AutoBlinkerCancelActionPure action;
  portENTER_CRITICAL(&blinkAMux);
  action = autoBlinkerCancelTogglePure(
      autoBlinkerTimingState, now, eligible, pauseSeconds);
  if (action == AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE) {
    lastReqDir = 0;
    autoRequestLastSeenMs = 0;
  } else if (action == AUTO_BLINKER_CANCEL_START_PAUSE_PURE) {
    autoBlinkerClearPendingLocked();
    blinkerTxClearPendingSourceLocked(BLINKER_TX_SOURCE_AUTO_PURE);
    oneShotTurn = STALK_IDLE;
    oneShotDirect = false;
    oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
    activeTurn = STALK_IDLE;
    // Do not synthesize or retain a planner direction from the manual cancel.
    lastReqDir = 0;
    autoRequestLastSeenMs = 0;
  }
  portEXIT_CRITICAL(&blinkAMux);

  if (action == AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE) {
    // Do not send another UI_ulcSnooze when the same cancel control is used
    // to resume Auto Blinker.
    ulcSnoozeFinishRequest();
    ulcSnoozeSetResult("RESUMED: Auto Blinker", 0, true);
    if (source == AUTO_BLINKER_CANCEL_SOURCE_S3XY)
      s3xyLogPush(S3XY_LOG_INFO, "button action -> Auto Blinker resumed");
    return action;
  }
  if (action == AUTO_BLINKER_CANCEL_REJECTED_PURE) {
    const char *result = !noaOpen ? "BLOCKED: NOA state invalid"
        : "BLOCKED: 0x24A stale";
    ulcSnoozeSetResult(result, 0, false);
    if (source == AUTO_BLINKER_CANCEL_SOURCE_S3XY)
      s3xyLogPush(S3XY_LOG_INFO, result);
    return action;
  }

  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozePending = true;
  ulcSnoozeExpireMs = now + ULC_SNOOZE_REQUEST_TIMEOUT_MS;
  portEXIT_CRITICAL(&ulcSnoozeMux);

  const bool door = source == AUTO_BLINKER_CANCEL_SOURCE_DOOR;
  ulcSnoozeSetResult(door ? "ARMED: door cancel" : "ARMED: cancel", 0, true);
  if (source == AUTO_BLINKER_CANCEL_SOURCE_S3XY)
    s3xyLogPush(S3XY_LOG_INFO,
        "button action -> ULC snooze + Auto Blinker pause");
  return action;
}

static void handleS3xySingleAction() {
  (void)handleAutoBlinkerCancelToggle(
      AUTO_BLINKER_CANCEL_SOURCE_S3XY, true);
}

// 0x3F8 UI_driverAssistControl stock telemetry + controlled LAB overlay.
// The original frame is never blocked.  When an override is selected, the newest
// stock 0x3F8 is copied, only the selected field(s) are changed, and one overlay
// frame is transmitted.  If every field is STOCK, 0x3F8 remains RX-only.
//
// Existing bit56 ALC / ACC overlay TX remains fail-closed to AUTOSTEER.
// ULC Blind Spot / Off-Highway and Confirm-Free are production settings.
// Auto Lane Change 0x293 remains a LAB-only experiment.
static portMUX_TYPE lab3f8Mux = portMUX_INITIALIZER_UNLOCKED;
static constexpr uint8_t LAB3F8_STOCK = 0xFF;
static constexpr uint8_t LAB3F8_ALC_STOCK = 0;
static constexpr uint8_t LAB3F8_ALC_FORCE_OFF = 1;
static constexpr uint8_t LAB3F8_ALC_FORCE_ON = 2;

static volatile uint8_t uiUlcBlindSpotConfig = 0;       // stock bits 52-53
static volatile bool    uiUlcOffHighway = false;        // stock bit 15
static volatile bool    uiAlcOffHighwayEnable = false;  // stock bit 56
static volatile uint32_t uiDriverAssistLastRxMs = 0;

// Persistent production selections. 0xFF means STOCK for raw-valued controls.
static volatile uint8_t lab3f8AlcMode = LAB3F8_ALC_STOCK;
static volatile uint8_t lab3f8UlcBlindMode = LAB3F8_STOCK;
static volatile uint8_t lab3f8UlcOffHighwayMode = LAB3F8_STOCK;
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
static volatile bool lab3f8LastTxBit56 = false;

// AP-active 0x3F8 ULC policy counters/state.
static volatile uint32_t ulcOffHighwayTxOk = 0;
static volatile uint32_t ulcOffHighwayTxFail = 0;
static volatile uint32_t ulcOffHighwayGateBlocked = 0;
static volatile bool ulcOffHighwayLastTxValid = false;
static volatile uint8_t ulcOffHighwayLastTxRaw = 0xFF;
static volatile uint32_t ulcOffHighwayLastTxMs = 0;

// Production CAN-B UI_ulcStalkConfirm feature. Default timing remains
// AP-active-only; PRE-AP is the explicit alternate policy.
static volatile bool ulcNoConfirmEnabled = false;
static volatile bool uiUlcStalkConfirm = true;
static volatile uint8_t ulcNoConfirmTimingMode = ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE;
static volatile uint32_t ulcNoConfirmTxOk = 0;
static volatile uint32_t ulcNoConfirmTxFail = 0;
static volatile uint32_t ulcNoConfirmTxBOk = 0;
static volatile uint32_t ulcNoConfirmTxBFail = 0;
static volatile uint32_t ulcNoConfirmGateBlocked = 0;
static volatile uint32_t ulcNoConfirmGateBlockedB = 0;
static volatile bool ulcNoConfirmLastTxValid = false;
static volatile bool ulcNoConfirmLastTxBit1 = true;
static volatile uint32_t ulcNoConfirmLastTxMs = 0;

// CAN-B stock/final monitor for the single production 0x3F8 compositor.
static volatile bool lab3f8CanBValid = false;
static volatile uint32_t lab3f8CanBRx = 0;
static volatile uint32_t lab3f8CanBLastMs = 0;
static volatile uint32_t lab3f8CanBPeriodMs = 0;
static volatile uint8_t lab3f8CanBDlc = 0;
static uint8_t lab3f8CanBData[8] = {};

// 0x293 UI_chassisControl research. Observe both physical CAN buses because
// Tesla routes/mirrors this frame differently across YL and Standard 3/Y.
// When enabled, each stock frame is overlaid back onto the same bus only while
// AP is active. The original frame is never blocked.
static portMUX_TYPE autoLc293Mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool uiAutoLaneChangeEnabled = false;
// b22: independent 0x293 TX target. BOTH preserves b21 behavior.
static volatile uint8_t uiAutoLaneChangeTargetBus = UI_AUTO_LC_BUS_BOTH_PURE;
static volatile bool uiAutoLaneChangeStockAValid = false;
static volatile bool uiAutoLaneChangeStockBValid = false;
static volatile uint8_t uiAutoLaneChangeStockARaw = 0xFF;
static volatile uint8_t uiAutoLaneChangeStockBRaw = 0xFF;
static volatile uint32_t uiAutoLaneChangeStockAMs = 0;
static volatile uint32_t uiAutoLaneChangeStockBMs = 0;
static volatile uint32_t uiAutoLaneChangeRxA = 0;
static volatile uint32_t uiAutoLaneChangeRxB = 0;
static volatile uint32_t uiAutoLaneChangeTxAOk = 0;
static volatile uint32_t uiAutoLaneChangeTxAFail = 0;
static volatile uint32_t uiAutoLaneChangeTxBOk = 0;
static volatile uint32_t uiAutoLaneChangeTxBFail = 0;
static volatile uint32_t uiAutoLaneChangeGateBlockedA = 0;
static volatile uint32_t uiAutoLaneChangeGateBlockedB = 0;
static volatile bool uiAutoLaneChangeLastTxValid = false;
static volatile uint8_t uiAutoLaneChangeLastTxBus = 0; // 1=A, 2=B
static volatile uint8_t uiAutoLaneChangeLastTxRaw = 0xFF;
static volatile uint32_t uiAutoLaneChangeLastTxMs = 0;


static const char *pedalMapName(uint8_t v) {
  switch (v) { case 0: return "CHILL"; case 1: return "SPORT"; case 2: return "PERFORMANCE"; default: return "UNKNOWN"; }
}
static const char *alcStateName(uint8_t v) {
  switch (v) {
    case 0: return "DISABLED";
    case 1: return "NO LANES";
    case 2: return "SONICS INVALID";
    case 3: return "TP FOLLOW";
    case 4: return "EXITING HIGHWAY";
    case 5: return "VEHICLE SPEED";
    case 6: return "AVAILABLE LEFT";
    case 7: return "AVAILABLE RIGHT";
    case 8: return "AVAILABLE BOTH";
    case 9: return "IN PROGRESS LEFT";
    case 10: return "IN PROGRESS RIGHT";
    case 11: return "WAIT SIDE OBST LEFT";
    case 12: return "WAIT SIDE OBST RIGHT";
    case 13: return "WAIT FWD OBST LEFT";
    case 14: return "WAIT FWD OBST RIGHT";
    case 15: return "SIDE OBSTACLE LEFT";
    case 16: return "SIDE OBSTACLE RIGHT";
    case 17: return "POOR VIEW RANGE";
    case 18: return "LC HEALTH BAD";
    case 19: return "BLINKER OFF";
    case 20: return "OTHER ABORT";
    case 21: return "SOLID LANE";
    case 22: return "TTC LEFT";
    case 23: return "TTC+USS LEFT";
    case 24: return "TTC RIGHT";
    case 25: return "TTC+USS RIGHT";
    case 26: return "LANE TYPE LEFT";
    case 27: return "LANE TYPE RIGHT";
    case 28: return "WAIT HANDS ON";
    case 29: return "TIMEOUT";
    case 30: return "MISSION PLAN INVALID";
    case 31: return "SNA";
    default: return "UNKNOWN";
  }
}

// CAN B load-shedding / queue telemetry. v3.6d2 reserves a small amount of
// headroom in fresh real PARK and promotes R79 to absolute application-level
// priority as soon as remote Summon startup evidence becomes fresh.
static constexpr uint16_t TWAI_TX_QUEUE_LEN = 16;
static constexpr uint16_t TWAI_PARK_NON_R79_QUEUE_LIMIT = 14;
static constexpr uint16_t TWAI_SUMMON_NON_R79_QUEUE_LIMIT = 6;
static constexpr uint8_t  TWAI_RX_DRAIN_BUDGET = 64;
static constexpr uint32_t TWAI_QUEUE_TELEMETRY_PERIOD_MS = 50;

static volatile uint32_t twaiTxQueueNow = 0;
static volatile uint32_t twaiTxQueueMax = 0;
static volatile uint32_t twaiRxQueueNow = 0;
static volatile uint32_t twaiRxQueueMax = 0;

static volatile uint32_t twaiNonSummonShed = 0;
static volatile uint32_t twaiParkSoftShed = 0;
static volatile uint32_t twaiReadyShed = 0;
static volatile uint32_t twaiActiveShed = 0;

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

static SummonRoutePure activeSummonRoute() {
  return summonRoutePure(activeVehicleProfile, activeVehicleTopology);
}

// stateMux must already be held when calling this helper. 0x118 is primary;
// 0x186 is a profile-gated fallback only after 0x118 becomes invalid/stale.
static SummonGearDecisionPure summonFreshGearDecisionLocked(uint32_t now) {
  const SummonRoutePure route = activeSummonRoute();
  const SummonGearObservationPure gear118 = {
      gear118State, gear118Ms, gear118State >= 0};
  const SummonGearObservationPure gear186 = {
      gear186State, gear186Ms, gear186State >= 0};
  return summonFreshGearPure(now, gear118, gear186,
                             route.valid && route.allow186Fallback,
                             SUMMON_GEAR_FRESH_MS);
}

// stateMux must already be held. A fresh decoded 0x118 is primary; Standard
// 3/Y may use its validated 0x186 fallback only when 0x118 is stale.
static bool r79FreshGearRawLocked(uint32_t now, uint8_t &rawOut) {
  const SummonGearDecisionPure gear = summonFreshGearDecisionLocked(now);
  if (!gear.valid) return false;
  if (gear.source == SUMMON_GEAR_118) rawOut = gear118Raw;
  else if (gear.source == SUMMON_GEAR_186) rawOut = gear186Raw;
  else return false;
  return rawOut == TESLA_GEAR_P || rawOut == TESLA_GEAR_R ||
         rawOut == TESLA_GEAR_N || rawOut == TESLA_GEAR_D;
}

// Fresh one-sided ACA/SPR evidence is used only to avoid entering manual
// suppression during the first moments of a remote Summon startup. It is not a
// confirmed Summon session and therefore never labels the dashboard SUMMON.
static bool r79RemoteStartupEvidenceLocked(uint32_t now) {
  const bool acaEvidence = acaValid && lastAca && lastAcaMillis != 0 &&
      summonAgeFreshPure(now, lastAcaMillis, SUMMON_ACA_FRESH_MS);
  const bool sprEvidence = sprValid && lastSprRaw != 0 && lastSprMillis != 0 &&
      summonAgeFreshPure(now, lastSprMillis, SUMMON_SPR_FRESH_MS);
  return acaEvidence || sprEvidence;
}

// stateMux must already be held. Priority never uses the v2.6 compatibility
// gateParked fallback; PARK_STANDBY requires a fresh decoded real gear source.
static uint8_t summonPriorityStateLocked(uint32_t now) {
  uint8_t gearRaw = TESLA_GEAR_INVALID;
  const bool gearValid = r79FreshGearRawLocked(now, gearRaw);
  return summonTxPriorityStatePure(
      gearValid, gearRaw, r79RemoteStartupEvidenceLocked(now), gateSummoning);
}

static uint8_t summonPriorityStateSnapshot(uint32_t now) {
  uint8_t state;
  portENTER_CRITICAL(&stateMux);
  state = summonPriorityStateLocked(now);
  portEXIT_CRITICAL(&stateMux);
  return state;
}

static const char* summonPriorityStateName(uint8_t state) {
  switch (state) {
    case SUMMON_PRIORITY_PARK_STANDBY: return "PARK_STANDBY";
    case SUMMON_PRIORITY_READY: return "SUMMON_READY";
    case SUMMON_PRIORITY_ACTIVE: return "SUMMON_ACTIVE";
    default: return "NORMAL";
  }
}

static void updateR79ManualSuppressionLocked(uint32_t now) {
  uint8_t gearRaw = TESLA_GEAR_INVALID;
  const bool gearValid = r79FreshGearRawLocked(now, gearRaw);
  const bool dasValid = dasAutopilotStateValid;
  const bool apActive = dasStateApActivePure(dasValid, dasAutopilotState4);
  const bool manualState = dasStateManualPure(dasValid, dasAutopilotState4);
  const bool remoteStartupEvidence = r79RemoteStartupEvidenceLocked(now);
  r79ManualSuppressionUpdatePure(
      r79ManualSuppression,
      gearValid, gearRaw,
      dasValid, apActive, manualState,
      gateSummoning, remoteStartupEvidence);
}

// Recompute the live Summon monitor state and R79 manual-suppression latch.
// v3.6d2 derives CAN-B transport priority separately from fresh real gear plus
// ACA/SPR evidence; the V2.6 compatibility PARK fallback is monitor/gate-only.
// stateMux must already be held.
static void refreshSummonDerivedStateLocked(uint32_t now) {
  SummonV26CompatStatePure compat = {
      gateParked, gateSummoning, lastAca, sprSeen, last280Millis};
  summonV26CompatTickPure(compat, now, PARKED_TIMEOUT_MS);
  gateParked = compat.parked;
  gateSummoning = compat.summoning;
  lastAca = compat.acaActive;
  sprSeen = compat.sprSeen;
  last280Millis = compat.last118Ms;

  const SummonGearDecisionPure gear = summonFreshGearDecisionLocked(now);
  summonGearSource = gear.valid ? gear.source : SUMMON_GEAR_NONE;
  summonGearObservedMs = gear.valid ? gear.observedMs : 0;
  if (summonGearLatchApplyDecisionPure(summonConfirmedGearLatch, gear))
    summonConfirmedGearTransitions++;

  updateR79ManualSuppressionLocked(now);
}

static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now) {
  R79RuntimeStatus out = {};
  out.state = R79_TX_STATE_ACTIVE;
  out.gearRaw = TESLA_GEAR_INVALID;

  bool stockValid = false;
  portENTER_CRITICAL(&r79LabMux);
  stockValid = r79LabStockValid;
  portEXIT_CRITICAL(&r79LabMux);

  bool apActive = false;
  R79ManualSuppressionPure manual = {};
  portENTER_CRITICAL(&stateMux);
  out.gearValid = r79FreshGearRawLocked(now, out.gearRaw);
  out.dasValid = dasAutopilotStateValid;
  out.dasState4 = dasAutopilotState4;
  out.summonSessionActive = gateSummoning;
  out.remoteStartupEvidence = r79RemoteStartupEvidenceLocked(now);
  manual = r79ManualSuppression;
  out.manualLatchActive = manual.active;
  apActive = dasStateApActivePure(out.dasValid, out.dasState4);
  portEXIT_CRITICAL(&stateMux);

  out.decision = r79TxDecisionPure(apActive, out.summonSessionActive, manual);

  if (canTxAdministrativeHold) out.state = R79_TX_STATE_ADMIN_HOLD;
  else if (!twaiReady) out.state = R79_TX_STATE_CAN_OFFLINE;
  else if (!stockValid) out.state = R79_TX_STATE_WAIT_TEMPLATE;
  else if (!out.decision.txEnabled) out.state = R79_TX_STATE_SUSPENDED;
  else out.state = R79_TX_STATE_ACTIVE;
  return out;
}

static void refreshSummonState() {
  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&stateMux);
  refreshSummonDerivedStateLocked(now);
  portEXIT_CRITICAL(&stateMux);
}

static bool summonLoadSheddingActive() {
  const uint8_t priority = summonPriorityStateSnapshot((uint32_t)millis());
  return priority == SUMMON_PRIORITY_READY || priority == SUMMON_PRIORITY_ACTIVE;
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

// Non-R79 CAN-B traffic is admitted by explicit transport priority. NORMAL is
// unchanged. Fresh real PARK softly reserves two of the 16 TWAI queue slots.
// READY and ACTIVE preserve the stronger historical six-frame ceiling so R79
// can be enqueued before and throughout remote motion.
static bool twaiNonSummonAdmissionOpen() {
  const uint8_t priority = summonPriorityStateSnapshot((uint32_t)millis());
  if (priority == SUMMON_PRIORITY_NORMAL) return true;

  twai_status_info_t st = {};
  if (!twaiReadQueueStatus(&st)) return false;
  if (summonPriorityNonR79AdmissionPure(
          priority, st.msgs_to_tx, TWAI_PARK_NON_R79_QUEUE_LIMIT,
          TWAI_SUMMON_NON_R79_QUEUE_LIMIT))
    return true;

  twaiNonSummonShed++;
  if (priority == SUMMON_PRIORITY_PARK_STANDBY) twaiParkSoftShed++;
  else if (priority == SUMMON_PRIORITY_READY) twaiReadyShed++;
  else if (priority == SUMMON_PRIORITY_ACTIVE) twaiActiveShed++;
  return false;
}

// 0x334 UI_powertrainControl overlay shared by LAB/S3XY PedalMap and AP Drive
// Profile. YL routes 0x334 on VH/CAN B; Standard Model 3/Y Body+Chassis routes
// the Body 0x334 copy on CAN A. Party+Chassis has no supported 0x334 path.
static portMUX_TYPE pedalMapMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool pedalMapStockValid = false;
static volatile uint8_t pedalMapStockRaw = 0xFF;
static volatile uint32_t pedalMapStockMs = 0;
static uint8_t pedalMapStockData[8] = {};
static volatile bool pedalMapStockDataValid = false;
static PedalMapSessionPure pedalMapSession = pedalMapSessionInitialPure();
static volatile uint32_t pedalMapTxOk = 0, pedalMapTxFail = 0, pedalMapBlocked = 0, pedalMapStockAccepted = 0;
static volatile uint32_t pedalMapImmediateTxOk = 0, pedalMapImmediateTxFail = 0;
static volatile uint32_t pedalMapFollowTxOk = 0, pedalMapFollowTxFail = 0;
static constexpr uint32_t PEDAL_MAP_STOCK_FRESH_MS = 1500;

// AP Drive Profile overlays UI_powertrainControl 0x334 while AUTOSTEER/NOA is
// active. YL/VH STANDARD=20 and REDUCED=10 are real-car validated. Standard
// Model 3/Y Body+Chassis uses the same supported byte[2] mapping. The production
// selector exposes STANDARD=20, REDUCED=10, and MINIMAL=1.
static volatile bool apDriveProfileEnabled = false;
static volatile uint8_t apDriveProfileRegenRaw = AP_DRIVE_REGEN_REDUCED_RAW;
static volatile uint32_t apDriveProfileTxOk = 0;
static volatile uint32_t apDriveProfileTxFail = 0;

static bool apDriveProfileGateOpen() {
  if (!activeProfileApDriveProfileSupported()) return false;
  bool enabled, valid;
  uint8_t state4;
  portENTER_CRITICAL(&stateMux);
  enabled = apDriveProfileEnabled;
  valid = dasAutopilotStateValid;
  state4 = dasAutopilotState4;
  portEXIT_CRITICAL(&stateMux);
  // AUTOSTEER nominal/restricted + NOA only. FSD state 6 is intentionally not
  // included because the requested profile is for AUTOSTEER/NOA.
  return enabled && valid && (state4 == 3 || state4 == 4 || state4 == 5);
}

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

// Mode changes get one best-effort immediate 0x334 TX from the newest real
// stock template, then the existing stock-follow overlay remains authoritative.
// The immediate path never bypasses CAN recovery, AP ownership or Summon gates.
static bool pedalMapTransmitImmediateFromCache(uint8_t targetRaw) {
  if (!activeProfilePedalMapSupported() || !pedalMapRawSelectablePure(targetRaw)) return false;
  const uint32_t now = (uint32_t)millis();
  uint8_t stockData[8] = {};
  bool valid; uint32_t stockMs;
  portENTER_CRITICAL(&pedalMapMux);
  valid = pedalMapStockDataValid && pedalMapStockValid;
  stockMs = pedalMapStockMs;
  if (valid) memcpy(stockData, pedalMapStockData, sizeof(stockData));
  portEXIT_CRITICAL(&pedalMapMux);

  if (!valid || stockMs == 0 || (uint32_t)(now - stockMs) > PEDAL_MAP_STOCK_FRESH_MS ||
      apDriveProfileGateOpen() || summonLoadSheddingActive() ||
      (targetRaw != PEDAL_MAP_RAW_STOCK && !manualDrivingGateOpen(now))) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return false;
  }

  if (targetRaw != PEDAL_MAP_RAW_STOCK) writeBitsLE(stockData, 5, 2, targetRaw);
  uint8_t ctr = (uint8_t)readBitsLE(stockData, 52, 4);
  ctr = (uint8_t)((ctr + 1) & 0x0F);
  writeBitsLE(stockData, 52, 4, ctr);
  stockData[7] = 0;
  stockData[7] = ui334Checksum(stockData);
  const uint32_t txEpoch = canTxEpochSnapshot();

  bool ok = false;
  if (activeProfileIsYl()) {
    if (!twaiNonSummonAdmissionOpen()) {
      portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
      return false;
    }
    twai_message_t out = {};
    out.identifier = UI_POWERTRAIN_ID;
    out.data_length_code = 8;
    out.flags = 0;
    memcpy(out.data, stockData, sizeof(stockData));
    ok = canTxTwaiTransmit(&out, txEpoch) == ESP_OK;
  } else if (activeCanAIsBody()) {
    struct can_frame out = {};
    out.can_id = UI_POWERTRAIN_ID;
    out.can_dlc = 8;
    memcpy(out.data, stockData, sizeof(stockData));
    MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
    const bool attempted = canTxMcpSend(&out, txEpoch, mcpErr, nullptr);
    ok = attempted && mcpErr == MCP2515::ERROR_OK;
    portENTER_CRITICAL(&pedalMapMux);
    if (ok) { mcpTxOk++; mcpTxFailConsecutive = 0; }
    else { mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
    portEXIT_CRITICAL(&pedalMapMux);
  } else {
    return false;
  }

  portENTER_CRITICAL(&pedalMapMux);
  if (ok) { pedalMapTxOk++; pedalMapImmediateTxOk++; }
  else { pedalMapTxFail++; pedalMapImmediateTxFail++; }
  portEXIT_CRITICAL(&pedalMapMux);
  return ok;
}

static bool requestPedalMapMode(uint8_t targetRaw, const char *sourceLabel) {
  if (!pedalMapRawSelectablePure(targetRaw)) return false;

  // STOCK is always an immediate volatile-session release. It deliberately
  // bypasses the manual/AP gate because releasing an override is always safe.
  if (targetRaw == PEDAL_MAP_RAW_STOCK) {
    bool wasActive = false;
    portENTER_CRITICAL(&pedalMapMux);
    wasActive = pedalMapSession.active;
    pedalMapSessionClearPure(pedalMapSession);
    portEXIT_CRITICAL(&pedalMapMux);
    if (wasActive) {
      String m = String(sourceLabel ? sourceLabel : "PedalMap") + " target: STOCK";
      s3xyLogPush(S3XY_LOG_INFO, m.c_str());
    }
    (void)pedalMapTransmitImmediateFromCache(PEDAL_MAP_RAW_STOCK);
    return true;
  }

  if (!activeProfilePedalMapSupported()) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    s3xyLogPush(S3XY_LOG_INFO, "Acceleration mode unavailable for current vehicle profile");
    return false;
  }

  const uint32_t now = (uint32_t)millis();
  const uint32_t txEpoch = canTxEpochSnapshot();
  bool valid; uint8_t stock; uint32_t age;
  portENTER_CRITICAL(&pedalMapMux);
  valid = pedalMapStockValid;
  stock = pedalMapStockRaw;
  age = pedalMapStockMs ? now - pedalMapStockMs : UINT32_MAX;
  portEXIT_CRITICAL(&pedalMapMux);

  if (!valid || stock > PEDAL_MAP_RAW_PERFORMANCE || age > PEDAL_MAP_STOCK_FRESH_MS ||
      !manualDrivingGateOpen(now) || summonLoadSheddingActive()) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    s3xyLogPush(S3XY_LOG_INFO, "Acceleration mode blocked: 0x334 stale/AP/Summon");
    return false;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return false;
  }
  if (!canTxBarrierAllowsPure(canTxBarrierState, txEpoch)) {
    xSemaphoreGive(canTxBarrierMutex);
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    return false;
  }

  bool accepted;
  portENTER_CRITICAL(&pedalMapMux);
  accepted = pedalMapSessionSetTargetPure(pedalMapSession, stock, targetRaw);
  portEXIT_CRITICAL(&pedalMapMux);
  xSemaphoreGive(canTxBarrierMutex);
  if (!accepted) return false;

  (void)pedalMapTransmitImmediateFromCache(targetRaw);
  String m = String(sourceLabel ? sourceLabel : "PedalMap") + " target: " + pedalMapName(targetRaw);
  s3xyLogPush(S3XY_LOG_INFO, m.c_str());
  return true;
}

static void requestPedalMapToggleFromButton() {
  bool valid; uint8_t stock; PedalMapSessionPure session;
  portENTER_CRITICAL(&pedalMapMux);
  valid = pedalMapStockValid;
  stock = pedalMapStockRaw;
  session = pedalMapSession;
  portEXIT_CRITICAL(&pedalMapMux);
  if (!valid) {
    portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
    s3xyLogPush(S3XY_LOG_INFO, "Acceleration toggle blocked: no 0x334 stock frame");
    return;
  }
  const uint8_t next = pedalMapToggleTargetPure(session, stock); // CHILL <-> SPORT; PERFORMANCE -> CHILL
  requestPedalMapMode(next, "Acceleration toggle");
}

static void requestPedalMapPerformanceFromButton() {
  requestPedalMapMode(PEDAL_MAP_RAW_PERFORMANCE, "Performance mode");
}

static void pedalMapClearSessionOnParkTransition(int previousGearState, int currentGearState) {
  if (!pedalMapEnteredParkPure(previousGearState, currentGearState)) return;
  bool wasActive = false;
  portENTER_CRITICAL(&pedalMapMux);
  wasActive = pedalMapSession.active;
  pedalMapSessionOnGearTransitionPure(pedalMapSession, previousGearState, currentGearState);
  portEXIT_CRITICAL(&pedalMapMux);
  if (wasActive) s3xyLogPush(S3XY_LOG_INFO, "PedalMap session cleared: entered PARK -> STOCK");
}

static bool pedalMapObserveAndPrepare(const uint8_t *srcData, uint8_t dlc, uint8_t outData[8],
                                     uint32_t &txEpoch, bool requireTwaiAdmission,
                                     bool &apDriveProfileOut) {
  apDriveProfileOut = false;
  if (!srcData || dlc != 8 || !outData) return false;
  const uint32_t now=(uint32_t)millis();
  const uint8_t stock=(uint8_t)readBitsLE(srcData,5,2);
  const bool apDriveProfile = apDriveProfileGateOpen();
  bool active; uint8_t target;
  portENTER_CRITICAL(&pedalMapMux);
  pedalMapStockValid=true; pedalMapStockRaw=stock; pedalMapStockMs=now;
  memcpy(pedalMapStockData, srcData, 8);
  pedalMapStockDataValid=true;
  pedalMapSessionObserveStockPure(pedalMapSession, stock);
  active=pedalMapSession.active; target=pedalMapSession.targetRaw;
  portEXIT_CRITICAL(&pedalMapMux);

  if (apDriveProfile) {
    uint8_t apRegen;
    portENTER_CRITICAL(&stateMux);
    apRegen = apDriveProfileRegenRaw;
    portEXIT_CRITICAL(&stateMux);
    if (!apDriveRegenRawSelectablePure(apRegen)) apRegen = AP_DRIVE_REGEN_REDUCED_RAW;
    // AP profile owns outgoing 0x334: CHILL/Comfort raw 0 plus the selected
    // regen byte[2] target on the supported 0x334 route.
    if (stock == 0 && srcData[2] == apRegen) return false;
    txEpoch=canTxEpochSnapshot();
    if (requireTwaiAdmission && !twaiNonSummonAdmissionOpen()) {
      portENTER_CRITICAL(&pedalMapMux); pedalMapBlocked++; portEXIT_CRITICAL(&pedalMapMux);
      return false;
    }
    memcpy(outData, srcData, 8);
    writeBitsLE(outData,5,2,0);
    outData[2]=apRegen;
    uint8_t ctr=(uint8_t)readBitsLE(outData,52,4);
    ctr=(uint8_t)((ctr+1)&0x0F);
    writeBitsLE(outData,52,4,ctr);
    outData[7]=0;
    outData[7]=ui334Checksum(outData);
    apDriveProfileOut = true;
    return true;
  }

  if (!active || stock==target) return false;

  txEpoch=canTxEpochSnapshot();
  // Re-read volatile session ownership after the epoch snapshot. AP Drive
  // Profile may temporarily supersede the output, but does not erase session.
  portENTER_CRITICAL(&pedalMapMux);
  active=pedalMapSession.active; target=pedalMapSession.targetRaw;
  portEXIT_CRITICAL(&pedalMapMux);
  if (!active || stock==target) return false;
  if (!manualDrivingGateOpen(now) || summonLoadSheddingActive() ||
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
  bool apDriveProfileTx = false;
  if (!pedalMapObserveAndPrepare(src.data, src.data_length_code, outData, txEpoch, true, apDriveProfileTx)) return;
  twai_message_t out=src;
  memcpy(out.data,outData,8);
  const esp_err_t err=canTxTwaiTransmit(&out,txEpoch);
  portENTER_CRITICAL(&pedalMapMux);
  if(err==ESP_OK) {
    pedalMapTxOk++;
    pedalMapFollowTxOk++;
    if (apDriveProfileTx) apDriveProfileTxOk++;
  } else {
    pedalMapTxFail++;
    pedalMapFollowTxFail++;
    if (apDriveProfileTx) apDriveProfileTxFail++;
  }
  portEXIT_CRITICAL(&pedalMapMux);
}

// Standard Model 3/Y Body+Chassis: 0x334 is on Body / CAN A (MCP2515).
static void handlePedalMap334OnCanA(const struct can_frame &src) {
  if ((src.can_id & 0xC0000000UL) != 0 || src.can_dlc != 8) return;
  uint8_t outData[8] = {};
  uint32_t txEpoch = 0;
  bool apDriveProfileTx = false;
  if (!pedalMapObserveAndPrepare(src.data, src.can_dlc, outData, txEpoch, false, apDriveProfileTx)) return;
  struct can_frame out = {};
  out.can_id = UI_POWERTRAIN_ID;
  out.can_dlc = 8;
  memcpy(out.data,outData,8);
  MCP2515::ERROR mcpErr=MCP2515::ERROR_FAIL;
  const bool attempted=canTxMcpSend(&out,txEpoch,mcpErr,nullptr);
  portENTER_CRITICAL(&pedalMapMux);
  if(attempted && mcpErr==MCP2515::ERROR_OK) {
    pedalMapTxOk++;
    pedalMapFollowTxOk++;
    if (apDriveProfileTx) apDriveProfileTxOk++;
    mcpTxOk++; mcpTxFailConsecutive=0;
  } else {
    pedalMapTxFail++;
    pedalMapFollowTxFail++;
    if (apDriveProfileTx) apDriveProfileTxFail++;
    mcpTxFail++; if(mcpTxFailConsecutive<255)mcpTxFailConsecutive++;
  }
  portEXIT_CRITICAL(&pedalMapMux);
}

static String pedalMapStatsJson(){
  const uint32_t now=(uint32_t)millis();
  bool valid,active;
  uint8_t stock,target,origin;
  uint32_t ms,ok,fail,blocked,accepted,apOk,apFail,imOk,imFail,followOk,followFail;
  portENTER_CRITICAL(&pedalMapMux);
  valid=pedalMapStockValid; active=pedalMapSession.active; stock=pedalMapStockRaw;
  target=pedalMapSession.targetRaw; origin=pedalMapSession.originRaw; ms=pedalMapStockMs;
  ok=pedalMapTxOk; fail=pedalMapTxFail; blocked=pedalMapBlocked; accepted=pedalMapStockAccepted;
  apOk=apDriveProfileTxOk; apFail=apDriveProfileTxFail;
  imOk=pedalMapImmediateTxOk; imFail=pedalMapImmediateTxFail;
  followOk=pedalMapFollowTxOk; followFail=pedalMapFollowTxFail;
  portEXIT_CRITICAL(&pedalMapMux);
  bool apEnabled, apActive; uint8_t apRegen;
  portENTER_CRITICAL(&stateMux);
  apEnabled=apDriveProfileEnabled; apRegen=apDriveProfileRegenRaw;
  portEXIT_CRITICAL(&stateMux);
  apActive=apDriveProfileGateOpen();

  String j;
  j.reserve(720);
  JsonWriterArduino jw(j);
  jw.boolean("valid", valid);
  jw.u32("stockRaw", stock);
  jw.string("stockName", valid ? pedalMapName(stock) : "NO DATA");
  jw.u32("ageMs", ms ? now - ms : 999999UL);
  jw.boolean("overrideActive", active);
  jw.u32("targetRaw", target);
  jw.string("targetName", active ? pedalMapName(target) : "STOCK");
  jw.u32("originRaw", origin);
  jw.u32("txOk", ok);
  jw.u32("txFail", fail);
  jw.u32("immediateTxOk", imOk);
  jw.u32("immediateTxFail", imFail);
  jw.u32("followTxOk", followOk);
  jw.u32("followTxFail", followFail);
  jw.u32("blocked", blocked);
  jw.u32("stockAccepted", accepted);
  jw.boolean("apDriveProfileEnabled", apEnabled);
  jw.boolean("apDriveProfileActive", apActive);
  jw.u32("apDriveProfileRegenRaw", apRegen);
  jw.string("apDriveProfileRegenName", apDriveRegenNamePure(apRegen));
  jw.u32("apDriveProfileTxOk", apOk);
  jw.u32("apDriveProfileTxFail", apFail);
  jw.finish();
  return j;
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
static inline uint8_t leftStalkChecksum(const uint8_t frame[4], uint8_t counter) { return leftStalkChecksumPure(frame, counter); }

static inline uint8_t dirToTurn(uint8_t dir) {
  if (dir == 1) return STALK_DOWN_1;
  if (dir == 2) return STALK_UP_1;
  return STALK_IDLE;
}

static inline uint8_t turnToDir(uint8_t turn) {
  if (turn == STALK_DOWN_1) return 1;
  if (turn == STALK_UP_1) return 2;
  return 0;
}

// Caller holds blinkAMux. Legacy mode is the only path that creates a timed
// pulse; Single mode remains stock-synchronized and has no resend latch.
static bool blinkerLegacyStartLocked(uint8_t dir, uint8_t source, uint32_t now) {
  const uint8_t turn = dirToTurn(dir);
  if (turn == STALK_IDLE ||
      (source != BLINKER_TX_SOURCE_AUTO_PURE &&
       source != BLINKER_TX_SOURCE_S3XY_PURE)) {
    return false;
  }
  oneShotTurn = turn;
  oneShotSource = source;
  oneShotDirect = source == BLINKER_TX_SOURCE_S3XY_PURE;
  if (activeTurnSignalVariant == TURN_SIGNAL_STALKLESS) {
    oneShotReleaseAt = now + BLINKA_STALKLESS_PRESS_MS;
    oneShotUntil = oneShotReleaseAt + BLINKA_STALKLESS_RELEASE_MS;
  } else {
    oneShotReleaseAt = 0;
    oneShotUntil = now + BLINKA_PULSE_MS;
  }
  activeTurn = turn;
  return true;
}

static void blinkerTxCancelLegacyLocked() {
  oneShotTurn = STALK_IDLE;
  oneShotDirect = false;
  oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  activeTurn = STALK_IDLE;
}

static bool blinkerTxSetMode(uint8_t requested) {
  portENTER_CRITICAL(&blinkAMux);
  const BlinkerTxModeTransitionPure transition =
      blinkerTxModeTransitionPure(blinkerTxMode, requested, labMenuEnabled);
  if (!transition.valid) {
    portEXIT_CRITICAL(&blinkAMux);
    return false;
  }
  if (transition.cancelLegacy) blinkerTxCancelLegacyLocked();
  blinkerTxMode = transition.effectiveMode;
  portEXIT_CRITICAL(&blinkAMux);
  return true;
}

static bool requestBlinkerTx(uint8_t dir, uint8_t source,
                             uint32_t eventToken, uint32_t now) {
  portENTER_CRITICAL(&blinkAMux);
  const uint8_t effective =
      blinkerTxEffectiveModePure(blinkerTxMode, labMenuEnabled);
  const bool accepted = effective == BLINKER_TX_MODE_SINGLE_PURE
      ? blinkerTxArmPure(blinkerTxRequestState, dir, source, eventToken, now)
      : blinkerLegacyStartLocked(dir, source, now);
  if (accepted) {
    blinkerTxRequestCount++;
    blinkerTxRecordLocked(dir, source, 1);
  } else {
    blinkerTxBlockedCount++;
    blinkerTxRecordLocked(dir, source, 4);
  }
  portEXIT_CRITICAL(&blinkAMux);
  return accepted;
}

// Read a real 0x249, align the injected counter, and atomically consume at
// most one pending Single-mode request. The caller transmits only after the
// stock frame has completed, so neither source enters the legacy 20 ms loop.
static BlinkerTxConsumeResultPure observe249AndTakeBlinkerRequest(
    const uint8_t *data, uint8_t dlc) {
  BlinkerTxConsumeResultPure consumed = {};
  const uint8_t safeDlc = dlc > 8 ? 8 : dlc;
  portENTER_CRITICAL(&blinkAMux);
  realDlc = safeDlc;
  memset(realRaw249, 0, sizeof(realRaw249));
  if (safeDlc) memcpy(realRaw249, data, safeDlc);

  portEXIT_CRITICAL(&blinkAMux);

  if (dlc < 3) return consumed;

  const uint8_t cnt = data[1] & 0x0F;
  const uint8_t turn = data[2] & 0x0F;
  const uint8_t ck = data[0];

  // The validated Model YL checksum requires all four stock bytes.
  // A short/non-YL frame is still counted/observed, but cannot pass self-test.
  const bool checksumComparable = dlc >= 4;
  const uint8_t predicted = checksumComparable ? leftStalkChecksum(data, cnt) : 0;
  const uint32_t now = (uint32_t)millis();

  portENTER_CRITICAL(&blinkAMux);
  rx249++;
  realCounter = cnt;
  realTurn = turn;
  realCksum = ck;
  cksumSelfTest = checksumComparable && (predicted == ck);
  seen249 = true;
  if (activeTurn == STALK_IDLE) blinkACounter = cnt;
  const uint8_t pendingDir = blinkerTxRequestState.pendingDir;
  const uint8_t pendingSource = blinkerTxRequestState.pendingSource;
  consumed = blinkerTxConsumeStockPure(
      blinkerTxRequestState, now, turn == STALK_IDLE,
      BLINKA_S3XY_STOCK_TIMEOUT_MS);
  if (pendingDir != 0 && !consumed.transmit) {
    blinkerTxBlockedCount++;
    blinkerTxRecordLocked(pendingDir, pendingSource, 4);
  }
  portEXIT_CRITICAL(&blinkAMux);
  return consumed;
}

// Read the real 0x249 frame on CAN B and send one stock-synchronized overlay
// only when a fresh shared request was consumed above.
static void handle249OnCanB(const uint8_t *data, uint8_t dlc) {
  const BlinkerTxConsumeResultPure request =
      observe249AndTakeBlinkerRequest(data, dlc);
  if (request.transmit) {
    sendStalkFrameCanB(dirToTurn(request.dir), canTxEpochSnapshot(),
                       request.source == BLINKER_TX_SOURCE_S3XY_PURE);
  }
}

// Send SCCM_turnIndicatorStalkStatus on CAN B.
//
// Do not fabricate a 0x249 payload from zeros. The newest real
// Model YL stock frame is used as the template so byte1 upper bits, byte2
// upper bits and byte3 remain exactly as the vehicle produced them.
// Only the rolling counter and requested turn nibble are changed, then the
// validated full-payload CRC is recalculated.
static void sendStalkFrameCanB(uint8_t turn, uint32_t txEpoch, bool directUser) {
  if (!directUser && ulcNoConfirmEnabledSnapshot()) return;
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
    blinkerTxBlockedCount++;
    blinkerTxRecordLocked(
        turnToDir(turn), directUser ? BLINKER_TX_SOURCE_S3XY_PURE
                                    : BLINKER_TX_SOURCE_AUTO_PURE, 4);
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
  const uint8_t traceSource = directUser
      ? CAN_TX_TRACE_SOURCE_S3XY_BUTTON
      : CAN_TX_TRACE_SOURCE_AUTO_BLINKER;
  if (twaiNonSummonAdmissionOpen())
    err = canTxTwaiTransmitTagged(&out, txEpoch, traceSource);
  portENTER_CRITICAL(&blinkAMux);
  if (err == ESP_OK) blkATxOk++;
  else blkATxFail++;
  blinkerTxRecordLocked(
      turnToDir(turn), directUser ? BLINKER_TX_SOURCE_S3XY_PURE
                                  : BLINKER_TX_SOURCE_AUTO_PURE,
      err == ESP_OK ? 2 : 3);
  portEXIT_CRITICAL(&blinkAMux);
}

static void handle249OnCanA(const uint8_t *data, uint8_t dlc) {
  const BlinkerTxConsumeResultPure request =
      observe249AndTakeBlinkerRequest(data, dlc);
  if (request.transmit) {
    sendStalkFrameCanA(dirToTurn(request.dir), canTxEpochSnapshot(),
                       request.source == BLINKER_TX_SOURCE_S3XY_PURE);
  }
}

static void sendStalkFrameCanA(uint8_t turn, uint32_t txEpoch, bool directUser) {
  if (!directUser && ulcNoConfirmEnabledSnapshot()) return;
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
    blinkerTxBlockedCount++;
    blinkerTxRecordLocked(
        turnToDir(turn), directUser ? BLINKER_TX_SOURCE_S3XY_PURE
                                    : BLINKER_TX_SOURCE_AUTO_PURE, 4);
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
  MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
  const uint8_t traceSource = directUser
      ? CAN_TX_TRACE_SOURCE_S3XY_BUTTON
      : CAN_TX_TRACE_SOURCE_AUTO_BLINKER;
  const bool attempted = canTxMcpSendTagged(
      &out, txEpoch, traceSource, mcpErr, nullptr);
  portENTER_CRITICAL(&blinkAMux);
  if (attempted && mcpErr == MCP2515::ERROR_OK) { blkATxOk++; mcpTxOk++; mcpTxFailConsecutive = 0; }
  else { blkATxFail++; mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
  blinkerTxRecordLocked(
      turnToDir(turn), directUser ? BLINKER_TX_SOURCE_S3XY_PURE
                                  : BLINKER_TX_SOURCE_AUTO_PURE,
      attempted && mcpErr == MCP2515::ERROR_OK ? 2 : 3);
  portEXIT_CRITICAL(&blinkAMux);
}


static void handle3C2OnCanA(const struct can_frame &incoming) {
  if (incoming.can_dlc < 8 || vcleftMuxPure(incoming.data) != VCLEFT_SWITCH_MUX1) return;
  const uint32_t now = (uint32_t)millis();
  const uint8_t left = vcleftLeftButtonPure(incoming.data);
  const uint8_t right = vcleftRightButtonPure(incoming.data);
  bool inject = false;
  bool directUser = false;
  uint8_t turn = STALK_IDLE;
  uint8_t switchState = VCLEFT_SWITCH_SNA;

  portENTER_CRITICAL(&blinkAMux);
  // A physical steering-wheel button press always wins over an injected pulse.
  if ((left == VCLEFT_SWITCH_ON || right == VCLEFT_SWITCH_ON) && oneShotTurn != STALK_IDLE) {
    autoBlinkerClearPendingLocked();
    oneShotTurn = STALK_IDLE; oneShotDirect = false;
    oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
    oneShotUntil = 0; oneShotReleaseAt = 0; activeTurn = STALK_IDLE;
  } else if (activeTurnSignalVariant == TURN_SIGNAL_STALKLESS &&
             oneShotTurn != STALK_IDLE && (int32_t)(oneShotUntil - now) > 0) {
    inject = true;
    directUser = oneShotDirect;
    turn = oneShotTurn;
    switchState = ((int32_t)(oneShotReleaseAt - now) > 0) ? VCLEFT_SWITCH_ON : VCLEFT_SWITCH_OFF;
  }
  portEXIT_CRITICAL(&blinkAMux);
  if (!inject) return;

  // Confirm-Free conflicts only with the planner-driven Auto Blinker path.
  // Explicit S3XY direct turn commands remain available.
  if (!directUser && ulcNoConfirmEnabledSnapshot()) return;

  struct can_frame out = incoming;
  if (turn == STALK_DOWN_1) vcleftSetLeftButtonPure(out.data, switchState);
  else if (turn == STALK_UP_1) vcleftSetRightButtonPure(out.data, switchState);
  else return;
  const uint32_t txEpoch = canTxEpochSnapshot();
  MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
  const bool attempted = canTxMcpSend(&out, txEpoch, mcpErr, nullptr);
  portENTER_CRITICAL(&blinkAMux);
  if (attempted && mcpErr == MCP2515::ERROR_OK) { blkATxOk++; mcpTxOk++; mcpTxFailConsecutive = 0; }
  else { blkATxFail++; mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
  portEXIT_CRITICAL(&blinkAMux);
}

static void handle3C2OnCanBRightScroll(const twai_message_t &incoming) {
  if (!activeProfileApRightScrollSupported() || incoming.data_length_code < 8 ||
      apRightScrollMuxPure(incoming.data) != AP_RIGHT_SCROLL_MUX1_PURE) return;

  const uint32_t now = (uint32_t)millis();
  bool apActive;
  portENTER_CRITICAL(&stateMux);
  apActive = dasStateApActivePure(dasAutopilotStateValid, dasAutopilotState4);
  portEXIT_CRITICAL(&stateMux);

  uint32_t visualWarningEpoch;
  bool visualWarningActive;
  portENTER_CRITICAL(&nagCtxMux);
  visualWarningEpoch = nagCtx.visualWarningEpoch;
  visualWarningActive = nagCtx.visualWarningActive;
  portEXIT_CRITICAL(&nagCtxMux);

  bool enabled;
  uint16_t intervalSeconds;
  uint8_t warningRepeatSeconds;
  ApRightScrollActionPure action;
  const uint8_t physicalValue = apRightScrollValuePure(incoming.data);
  portENTER_CRITICAL(&apRightScrollMux);
  enabled = apRightScrollEnabled;
  intervalSeconds = apRightScrollIntervalSeconds;
  warningRepeatSeconds = apRightScrollVisualRepeatSeconds;
  const bool gateOpen = enabled && apActive && twaiReady &&
                        !canTxAdministrativeHold;
  apRightScrollMux1Rx++;
  if (physicalValue != 0U && gateOpen && apRightScrollState.gateWasOpen)
    apRightScrollPhysicalDeferrals++;
  action = apRightScrollStepPure(apRightScrollState, now, gateOpen,
                                 intervalSeconds, warningRepeatSeconds,
                                 visualWarningEpoch, visualWarningActive,
                                 true, physicalValue);
  portEXIT_CRITICAL(&apRightScrollMux);
  if (action == AP_RIGHT_SCROLL_ACTION_NONE_PURE) return;

  twai_message_t out = incoming;
  const uint8_t value = action == AP_RIGHT_SCROLL_ACTION_UP_PURE
      ? AP_RIGHT_SCROLL_UP_PURE : AP_RIGHT_SCROLL_DOWN_PURE;
  apRightScrollSetValuePure(out.data, value);
  const uint32_t txEpoch = canTxEpochSnapshot();
  const esp_err_t err = canTxTwaiTransmitWithMask(&out, txEpoch, CAN_TX_FRESH_VH);
  researchCaptureObserveTxVh((uint16_t)out.identifier, out.data_length_code,
                             out.data, err == ESP_OK);

  portENTER_CRITICAL(&apRightScrollMux);
  apRightScrollTxResultPure(apRightScrollState, now, action, err == ESP_OK);
  if (err == ESP_OK) {
    if (action == AP_RIGHT_SCROLL_ACTION_UP_PURE) apRightScrollTxUpOk++;
    else apRightScrollTxDownOk++;
    apRightScrollLastTxMs = now;
    apRightScrollLastTxValue = value;
  } else {
    apRightScrollTxFail++;
  }
  portEXIT_CRITICAL(&apRightScrollMux);
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
  portEXIT_CRITICAL(&blinkAMux);
  (void)handleAutoBlinkerCancelToggle(
      AUTO_BLINKER_CANCEL_SOURCE_DOOR,
      enabled && pressed && !previous);
}


// Request-session Auto Blinker. A fresh LEFT/RIGHT planner request arms the
// configured delay even when the requested lane is temporarily unavailable.
// After the delay, blocked ALC eligibility keeps the request pending and the
// TX task retries at BLINKA_RETRY_PERIOD_MS. One continuous planner request can
// fire at most once; an opposite request restarts the session, while NOA loss
// or a sustained request absence ends it.
static void evaluateAutoBlinker() {
  if (ulcNoConfirmEnabledSnapshot()) {
    portENTER_CRITICAL(&blinkAMux);
    autoBlinkerClearPendingLocked();
    blinkerTxClearPendingSourceLocked(BLINKER_TX_SOURCE_AUTO_PURE);
    // Confirm-Free disables only planner-driven Auto Blinker activity. Preserve
    // an explicit S3XY direct turn-signal pulse already in progress.
    if (!oneShotDirect) {
      oneShotTurn = STALK_IDLE; oneShotUntil = 0; oneShotReleaseAt = 0;
      oneShotSource = BLINKER_TX_SOURCE_NONE_PURE; activeTurn = STALK_IDLE;
    }
    portEXIT_CRITICAL(&blinkAMux);
    return;
  }
  const uint32_t now = (uint32_t)millis();
  const bool noaOpen = autoBlinkerPlannerGateOpen(now);
  uint32_t requestFrameMs = 0;
  const uint8_t currentReqDir = autoBlinkerCurrentRequestDir(now, &requestFrameMs);
  const bool currentAlcAllowed = currentReqDir != 0 &&
      autoBlinkerALCAllowsDirection(currentReqDir, now);

  portENTER_CRITICAL(&blinkAMux);
  if (!blinkAEnabled || !noaOpen) {
    autoBlinkerClearPendingLocked();
    blinkerTxClearPendingSourceLocked(BLINKER_TX_SOURCE_AUTO_PURE);
    lastReqDir = 0;
    autoRequestLastSeenMs = 0;
    portEXIT_CRITICAL(&blinkAMux);
    return;
  }

  const bool pulseActive = oneShotTurn != STALK_IDLE &&
      (int32_t)(oneShotUntil - now) > 0;

  if (currentReqDir != 0) {
    if (autoArmed && autoPendingDir != currentReqDir) {
      // Explicit opposite-direction planner request: replace the old session
      // and apply the full configured delay to the new direction.
      autoPendingDir = currentReqDir;
      blinkerNextEventTokenLocked(autoBlinkerSessionToken);
      autoFireAt = now + blinkADelayMs;
      autoRetryAt = autoFireAt;
      autoRequestLastSeenMs = requestFrameMs;
      lastReqDir = currentReqDir;
    } else if (autoArmed) {
      autoRequestLastSeenMs = requestFrameMs;
      lastReqDir = currentReqDir;
      // 0x399 and 0x24A updates call this function directly. If the delay has
      // elapsed and the lane just became available, wake the next TX tick
      // immediately instead of waiting for the fallback retry deadline.
      if (currentAlcAllowed && (int32_t)(now - autoFireAt) >= 0)
        autoRetryAt = now;
    } else if (autoBlinkerShouldStartSessionPure(
                   currentReqDir, lastReqDir, false, pulseActive)) {
      autoPendingDir = currentReqDir;
      blinkerNextEventTokenLocked(autoBlinkerSessionToken);
      autoFireAt = now + blinkADelayMs;
      autoRetryAt = autoFireAt;
      autoRequestLastSeenMs = requestFrameMs;
      autoArmed = true;
      lastReqDir = currentReqDir;
    } else {
      // Same request after a successful pulse: keep the session-history latch
      // fresh so the request cannot fire twice until it genuinely ends.
      autoRequestLastSeenMs = requestFrameMs;
      lastReqDir = currentReqDir;
    }
  } else {
    const bool requestRecent = autoRequestLastSeenMs != 0 &&
        (uint32_t)(now - autoRequestLastSeenMs) <= BLINKA_REQUEST_FRESH_MS;
    if (!requestRecent) {
      autoBlinkerClearPendingLocked();
      lastReqDir = 0;
      autoRequestLastSeenMs = 0;
    }
  }
  portEXIT_CRITICAL(&blinkAMux);
}

// Generate one independent turn-signal pulse. A request that has reached its
// delay but remains ALC-blocked is retained and checked every 250 ms. This is
// a pre-fire retry only: after one pulse has started, the firmware does not
// repeatedly toggle the stalk because no verified lamp-state signal is used.
static void blinkATxTick() {
  static uint32_t lastTxMs = 0;
  const uint32_t now = (uint32_t)millis();

  bool armed;
  uint8_t pendingSnapshot;
  uint32_t fireAtSnapshot, retryAtSnapshot, requestLastSeenSnapshot;
  uint32_t eventTokenSnapshot;
  bool pulsePending;
  bool pulseDirect;
  portENTER_CRITICAL(&blinkAMux);
  armed = autoArmed;
  pendingSnapshot = autoPendingDir;
  fireAtSnapshot = autoFireAt;
  retryAtSnapshot = autoRetryAt;
  requestLastSeenSnapshot = autoRequestLastSeenMs;
  eventTokenSnapshot = autoBlinkerSessionToken;
  pulsePending = oneShotTurn != STALK_IDLE;
  pulseDirect = oneShotDirect;
  portEXIT_CRITICAL(&blinkAMux);

  // Auto Blinker retains its original policy gates. A direct S3XY turn-signal
  // pulse bypasses those planner/profile gates but still uses the validated
  // route/template and normal CAN transport admission below.
  if (armed && (ulcNoConfirmEnabledSnapshot() || !activeProfileAdvancedEapSupported())) {
    portENTER_CRITICAL(&blinkAMux);
    autoBlinkerClearPendingLocked();
    armed = false;
    portEXIT_CRITICAL(&blinkAMux);
  }
  if (pulsePending && !pulseDirect &&
      (ulcNoConfirmEnabledSnapshot() || !activeProfileAdvancedEapSupported())) {
    portENTER_CRITICAL(&blinkAMux);
    oneShotTurn = STALK_IDLE;
    oneShotDirect = false;
    oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
    activeTurn = STALK_IDLE;
    pulsePending = false;
    portEXIT_CRITICAL(&blinkAMux);
  }
  if (!armed && !pulsePending) return;

  const uint32_t txEpoch = canTxEpochSnapshot();
  uint8_t turn = STALK_IDLE;
  bool directUser = false;

  bool fireSingleOrLegacyRequest = false;
  uint8_t fireRequestDir = 0;
  if (armed) {
    const bool noaOpen = autoBlinkerPlannerGateOpen(now);
    const uint8_t currentReqDir = autoBlinkerCurrentRequestDir(now);
    const bool requestSeenRecently = requestLastSeenSnapshot != 0 &&
        (uint32_t)(now - requestLastSeenSnapshot) <= BLINKA_REQUEST_FRESH_MS;
    const bool delayElapsed = (int32_t)(now - fireAtSnapshot) >= 0;
    const bool retryDue = retryAtSnapshot == 0 ||
        (int32_t)(now - retryAtSnapshot) >= 0;
    const bool alcAllowed = pendingSnapshot != 0 &&
        autoBlinkerALCAllowsDirection(pendingSnapshot, now);
    const AutoBlinkerSessionDecisionPure decision = autoBlinkerSessionDecisionPure(
        noaOpen, pendingSnapshot, currentReqDir, requestSeenRecently,
        delayElapsed, alcAllowed, retryDue);

    portENTER_CRITICAL(&blinkAMux);
    const bool sameSession = autoArmed && autoPendingDir == pendingSnapshot &&
        autoFireAt == fireAtSnapshot && autoRetryAt == retryAtSnapshot &&
        autoRequestLastSeenMs == requestLastSeenSnapshot &&
        autoBlinkerSessionToken == eventTokenSnapshot;
    if (sameSession && decision.cancel) {
      autoBlinkerClearPendingLocked();
      lastReqDir = 0;
      autoRequestLastSeenMs = 0;
    } else if (sameSession && decision.retry) {
      autoRetryAt = now + BLINKA_RETRY_PERIOD_MS;
      autoRetryCount++;
    } else if (sameSession && decision.fire) {
      fireRequestDir = autoPendingDir;
      fireSingleOrLegacyRequest = fireRequestDir != 0;
      autoBlinkerClearPendingLocked();
    }
    portEXIT_CRITICAL(&blinkAMux);
  }

  if (fireSingleOrLegacyRequest) {
    requestBlinkerTx(fireRequestDir, BLINKER_TX_SOURCE_AUTO_PURE,
                     eventTokenSnapshot, now);
  }

  // Once started, preserve the established pulse behavior: later planner or
  // ALC changes do not truncate a pulse. Direct/auto origin is carried with it.
  portENTER_CRITICAL(&blinkAMux);
  if (oneShotTurn != STALK_IDLE && (int32_t)(oneShotUntil - now) > 0) {
    turn = oneShotTurn;
    directUser = oneShotDirect;
  } else {
    oneShotTurn = STALK_IDLE;
    oneShotDirect = false;
    oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
    oneShotUntil = 0;
    oneShotReleaseAt = 0;
  }
  activeTurn = turn;
  portEXIT_CRITICAL(&blinkAMux);

  if (turn == STALK_IDLE) return;
  if (activeTurnSignalVariant == TURN_SIGNAL_STALKLESS) return; // 0x3C2 echoes on each live mux1 RX
  if ((uint32_t)(now - lastTxMs) < BLINKA_TX_PERIOD_MS) return;
  lastTxMs = now;
  if (activeProfileIsYl()) sendStalkFrameCanB(turn, txEpoch, directUser);
  else if (activeCanAIsBody()) sendStalkFrameCanA(turn, txEpoch, directUser);
}

// Legacy 0x3F8 ULC injection remains removed; LAB provides only the
// explicit LAB overlay below, gated to fresh AUTOSTEER state 3.


static inline bool isDASActive(uint8_t status) {
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


static void handle280(const uint8_t *data) {
    if (!data) return;
    sumRx280++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t gear = readVehicleGear(data);
    const int gs = gearState(gear);
    const bool aca = (data[6] & 0x04) != 0;
    int prevGs;
    portENTER_CRITICAL(&stateMux);
    prevGs = gear118State;
    if (gs >= 0) {
        gear118State = (int8_t)gs;
        gear118Raw = gear;
        gear118Ms = now;
    }

    SummonV26CompatStatePure compat = {
        gateParked, gateSummoning, lastAca, sprSeen, last280Millis};
    summonV26CompatApply118Pure(compat, (int8_t)gs, aca, now);
    gateParked = compat.parked;
    gateSummoning = compat.summoning;
    lastAca = compat.acaActive;
    sprSeen = compat.sprSeen;
    last280Millis = compat.last118Ms;

    acaValid = true;
    lastAcaMillis = now;
    refreshSummonDerivedStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
    pedalMapClearSessionOnParkTransition(prevGs, gs);
}

static void handle390(const uint8_t *data) {
    if (!data) return;
    sumRx390++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t gear = readVehicleGear(data);
    const int gs = gearState(gear);
    if (gs < 0) return;

    int prevGs;
    portENTER_CRITICAL(&stateMux);
    prevGs = gear186State;
    gear186State = (int8_t)gs;
    gear186Raw = gear;
    gear186Ms = now;

    SummonV26CompatStatePure compat = {
        gateParked, gateSummoning, lastAca, sprSeen, last280Millis};
    const SummonRoutePure route = activeSummonRoute();
    summonV26CompatApply186Pure(
        compat, (int8_t)gs, now, PARKED_TIMEOUT_MS,
        route.valid && route.allow186Fallback);
    gateParked = compat.parked;
    gateSummoning = compat.summoning;
    lastAca = compat.acaActive;
    sprSeen = compat.sprSeen;
    last280Millis = compat.last118Ms;

    refreshSummonDerivedStateLocked(now);
    portEXIT_CRITICAL(&stateMux);
    pedalMapClearSessionOnParkTransition(prevGs, gs);
}

static void handle921(const uint8_t *data, uint8_t dlc) {
    sumRx921++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t dasState4 = readDASState4(data);
    const uint8_t handsOnState = data[0] & 0x0Fu;
    const bool alcValid = (dlc >= 7);
    const uint8_t alcState = alcValid ? readDASAutoLaneChangeState(data) : 0xFF;
    bool ap = isDASActive(readDASStatus(data));
    bool noa = (dasState4 == 5); // ACTIVE_NAV = Navigate on Autopilot
    bool wasAp, wasNoa, wasScrollAp;
    const bool scrollAp = dasStateApActivePure(true, dasState4);
    portENTER_CRITICAL(&stateMux);
    wasAp = gateAPActive;
    wasNoa = gateNOAActive;
    wasScrollAp = dasStateApActivePure(dasAutopilotStateValid, dasAutopilotState4);
    gateAPActive = ap;
    gateNOAActive = noa;
    dasAutopilotState4 = dasState4;
    dasAutopilotStateValid = true;
    dasAutoLaneChangeState = alcState;
    dasAutoLaneChangeStateValid = alcValid;
    lastDASStatusMillis = now;
    refreshSummonDerivedStateLocked(now);
    portEXIT_CRITICAL(&stateMux);

    // Chassis 0x399 carries the same low-nibble hands-on state used by the
    // Party observer. Sharing one epoch prevents 3 -> 4 -> 5 from retriggering.
    nagObserveHandsOnState(handsOnState, now);

    portENTER_CRITICAL(&blinkAMux);
    autoBlinkerObserveNoaPure(
        autoBlinkerTimingState, now, true, noa,
        blinkANoaStabilizationSeconds);
    portEXIT_CRITICAL(&blinkAMux);

    if (wasScrollAp && !scrollAp) apRightScrollRuntimeReset(false);

    // Keep NAG continuity diagnostics scoped to the current AP session.
    nagDiagApTransition(ap, wasAp, now);
    // Mode B should begin with a fresh burst when AP transitions OFF -> ON.
    if (ap && !wasAp) nagModeBPhaseStartMs = now;
    // Mode H never resumes an event across an AP boundary. Both engagement
    // and disengagement invalidate the current descriptor and session seed.
    nagHumanRuntimeApTransition(ap, wasAp);

    // A delayed Auto Blinker request must not survive a NOA ->
    // Autosteer/OFF transition. AP/NOA validity is latched until CAN A recovery;
    // do not truncate a pulse already in progress.
    if (wasNoa && !noa) {
      portENTER_CRITICAL(&blinkAMux);
      autoBlinkerClearPendingLocked();
      lastReqDir = 0;
      autoRequestLastSeenMs = 0;
      portEXIT_CRITICAL(&blinkAMux);
    }

    // ALC availability lives in this same 0x399 frame. Re-evaluate immediately
    // so a pending Auto Blinker request can fire as soon as the requested lane
    // becomes available, without waiting for another 0x24A frame.
    evaluateAutoBlinker();
}

static void handle1016(const uint8_t *data, uint8_t dlc) {
    if (!data || dlc < 4) return;
    sumRx1016++;
    const uint32_t now = (uint32_t)millis();
    const uint8_t spr = (data[3] >> 4) & 0x0F;
    if (dlc >= 8) {
        // UI_ulcOffHighway: bit 15.
        // UI_ulcBlindSpotConfig: bits 52-53.
        // UI_alcOffHighwayEnable: bit 56.
        uiUlcOffHighway = getBit(data, 15);
        uiUlcBlindSpotConfig = (uint8_t)readBitsLE(data, 52, 2);
        uiAlcOffHighwayEnable = getBit(data, 56);
        uiDriverAssistLastRxMs = now;
    }

    portENTER_CRITICAL(&stateMux);
    lastSprRaw = spr;
    sprValid = true;
    lastSprMillis = now;

    SummonV26CompatStatePure compat = {
        gateParked, gateSummoning, lastAca, sprSeen, last280Millis};
    summonV26CompatApplySprPure(compat, spr);
    gateParked = compat.parked;
    gateSummoning = compat.summoning;
    lastAca = compat.acaActive;
    sprSeen = compat.sprSeen;
    last280Millis = compat.last118Ms;

    refreshSummonDerivedStateLocked(now);
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

static bool ulcNoConfirmGateOpen();

static void lab3f8ObserveCanB(const twai_message_t &src) {
    if (src.identifier != DRIVER_ASSIST_ID || src.data_length_code < 1) return;
    const uint32_t now = (uint32_t)millis();
    portENTER_CRITICAL(&lab3f8Mux);
    if (lab3f8CanBLastMs) lab3f8CanBPeriodMs = (uint32_t)(now - lab3f8CanBLastMs);
    lab3f8CanBLastMs = now;
    lab3f8CanBRx++;
    lab3f8CanBValid = true;
    lab3f8CanBDlc = src.data_length_code > 8 ? 8 : src.data_length_code;
    memset(lab3f8CanBData, 0, sizeof(lab3f8CanBData));
    memcpy(lab3f8CanBData, src.data, lab3f8CanBDlc);
    portEXIT_CRITICAL(&lab3f8Mux);
}

static bool ulcNoConfirmEnabledSnapshot() {
    bool enabled;
    portENTER_CRITICAL(&lab3f8Mux);
    enabled = ulcNoConfirmEnabled;
    portEXIT_CRITICAL(&lab3f8Mux);
    return activeProfileUlcNoConfirmSupported() && enabled;
}

static bool ulcNoConfirmGateOpen() {
    bool enabled;
    uint8_t timingMode;
    uint8_t state4;
    bool valid;
    portENTER_CRITICAL(&lab3f8Mux);
    enabled = ulcNoConfirmEnabled;
    timingMode = ulcNoConfirmTimingMode;
    portEXIT_CRITICAL(&lab3f8Mux);
    portENTER_CRITICAL(&stateMux);
    state4 = dasAutopilotState4;
    valid = dasAutopilotStateValid;
    portEXIT_CRITICAL(&stateMux);
    return ulcNoConfirmGateOpenWithTimingPure(
        activeProfileUlcNoConfirmSupported(), enabled, timingMode, valid, state4);
}

static bool uiAutoLaneChangeGateOpen() {
    bool enabled;
    bool valid;
    uint8_t state4;
    portENTER_CRITICAL(&autoLc293Mux);
    enabled = uiAutoLaneChangeEnabled;
    portEXIT_CRITICAL(&autoLc293Mux);
    portENTER_CRITICAL(&stateMux);
    valid = dasAutopilotStateValid;
    state4 = dasAutopilotState4;
    portEXIT_CRITICAL(&stateMux);
    return uiAutoLaneChangeGateOpenPure(labMenuEnabled, enabled, valid, state4);
}

static void uiAutoLaneChangeObserveAndInjectCanA(const struct can_frame &src) {
    if ((src.can_id & 0xC0000000UL) != 0 || src.can_dlc < 8) return;
    const uint32_t now = (uint32_t)millis();
    const uint8_t raw = uiAutoLaneChangeReadRawPure(src.data, src.can_dlc);
    bool enabled;
    uint8_t targetBus;
    portENTER_CRITICAL(&autoLc293Mux);
    uiAutoLaneChangeStockAValid = raw <= UI_AUTO_LANE_CHANGE_SNA_PURE;
    uiAutoLaneChangeStockARaw = raw;
    uiAutoLaneChangeStockAMs = now;
    uiAutoLaneChangeRxA++;
    enabled = uiAutoLaneChangeEnabled;
    targetBus = uiAutoLaneChangeTargetBus;
    portEXIT_CRITICAL(&autoLc293Mux);
    if (!labMenuEnabled || !enabled) return;
    if (!uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_A_PURE)) return;
    if (!uiAutoLaneChangeGateOpen()) {
        portENTER_CRITICAL(&autoLc293Mux); uiAutoLaneChangeGateBlockedA++; portEXIT_CRITICAL(&autoLc293Mux);
        return;
    }
    if (raw == UI_AUTO_LANE_CHANGE_ON_PURE) return;

    struct can_frame out = {};
    out.can_id = UI_CHASSIS_CONTROL_ID;
    out.can_dlc = 8;
    memcpy(out.data, src.data, 8);
    if (!uiAutoLaneChangeFinalizePure(out.data, out.can_dlc)) return;
    const uint32_t txEpoch = canTxEpochSnapshot();
    MCP2515::ERROR mcpErr = MCP2515::ERROR_FAIL;
    const bool attempted = canTxMcpSend(&out, txEpoch, mcpErr, nullptr);
    const bool ok = attempted && mcpErr == MCP2515::ERROR_OK;
    if (attempted) {
      if (ok) { mcpTxOk++; mcpTxFailConsecutive = 0; }
      else { mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
    }
    portENTER_CRITICAL(&autoLc293Mux);
    if (ok) uiAutoLaneChangeTxAOk++; else uiAutoLaneChangeTxAFail++;
    if (attempted) {
      uiAutoLaneChangeLastTxValid = true;
      uiAutoLaneChangeLastTxBus = 1;
      uiAutoLaneChangeLastTxRaw = UI_AUTO_LANE_CHANGE_ON_PURE;
      uiAutoLaneChangeLastTxMs = now;
    }
    portEXIT_CRITICAL(&autoLc293Mux);
}

static void uiAutoLaneChangeObserveAndInjectCanB(const twai_message_t &src) {
    if (src.extd || src.rtr || src.data_length_code < 8) return;
    const uint32_t now = (uint32_t)millis();
    const uint8_t raw = uiAutoLaneChangeReadRawPure(src.data, src.data_length_code);
    bool enabled;
    uint8_t targetBus;
    portENTER_CRITICAL(&autoLc293Mux);
    uiAutoLaneChangeStockBValid = raw <= UI_AUTO_LANE_CHANGE_SNA_PURE;
    uiAutoLaneChangeStockBRaw = raw;
    uiAutoLaneChangeStockBMs = now;
    uiAutoLaneChangeRxB++;
    enabled = uiAutoLaneChangeEnabled;
    targetBus = uiAutoLaneChangeTargetBus;
    portEXIT_CRITICAL(&autoLc293Mux);
    if (!labMenuEnabled || !enabled) return;
    if (!uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_B_PURE)) return;
    if (!uiAutoLaneChangeGateOpen()) {
        portENTER_CRITICAL(&autoLc293Mux); uiAutoLaneChangeGateBlockedB++; portEXIT_CRITICAL(&autoLc293Mux);
        return;
    }
    if (raw == UI_AUTO_LANE_CHANGE_ON_PURE) return;
    if (!twaiNonSummonAdmissionOpen()) {
        portENTER_CRITICAL(&autoLc293Mux); uiAutoLaneChangeTxBFail++; portEXIT_CRITICAL(&autoLc293Mux);
        return;
    }

    twai_message_t out = src;
    if (!uiAutoLaneChangeFinalizePure(out.data, out.data_length_code)) return;
    const uint32_t txEpoch = canTxEpochSnapshot();
    const esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    researchCaptureObserveTxVh((uint16_t)out.identifier, out.data_length_code, out.data, err == ESP_OK);
    portENTER_CRITICAL(&autoLc293Mux);
    if (err == ESP_OK) uiAutoLaneChangeTxBOk++; else uiAutoLaneChangeTxBFail++;
    uiAutoLaneChangeLastTxValid = true;
    uiAutoLaneChangeLastTxBus = 2;
    uiAutoLaneChangeLastTxRaw = UI_AUTO_LANE_CHANGE_ON_PURE;
    uiAutoLaneChangeLastTxMs = now;
    portEXIT_CRITICAL(&autoLc293Mux);
}

static bool lab3f8ApPolicyGateOpen(bool selectionActive) {
    uint8_t state4;
    bool valid;
    portENTER_CRITICAL(&stateMux);
    state4 = dasAutopilotState4;
    valid = dasAutopilotStateValid;
    portEXIT_CRITICAL(&stateMux);
    return ulcPolicyApGateOpenPure(selectionActive, valid, state4);
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
    uint8_t alc, blind, ulcOff; bool noConfirm, autoLc293;
    portENTER_CRITICAL(&lab3f8Mux);
    alc = lab3f8AlcMode;
    blind = lab3f8UlcBlindMode;
    ulcOff = lab3f8UlcOffHighwayMode;
    noConfirm = ulcNoConfirmEnabled;
    portEXIT_CRITICAL(&lab3f8Mux);
    portENTER_CRITICAL(&autoLc293Mux);
    autoLc293 = uiAutoLaneChangeEnabled;
    portEXIT_CRITICAL(&autoLc293Mux);
    return alc == LAB3F8_ALC_FORCE_ON || blind != LAB3F8_STOCK ||
           ulcOff != LAB3F8_STOCK || noConfirm || autoLc293;
}

static void injectDriverAssistControl(const twai_message_t &src) {
    lab3f8ObserveCanB(src);
    if (src.data_length_code < 8) return;

    UlcCompositeSelectionPure selected = {};
    uint8_t confirmTiming;
    portENTER_CRITICAL(&lab3f8Mux);
    uiUlcStalkConfirm = getBit(src.data, 1);
    selected.alcOffHighwayEnabled =
        lab3f8AlcMode == LAB3F8_ALC_FORCE_ON;
    selected.blindSpotMode = lab3f8UlcBlindMode;
    selected.ulcOffHighwayMode = lab3f8UlcOffHighwayMode;
    selected.confirmFreeEnabled = ulcNoConfirmEnabled;
    confirmTiming = ulcNoConfirmTimingMode;
    portEXIT_CRITICAL(&lab3f8Mux);

    const bool blindSelected =
        selected.blindSpotMode != ULC_COMPOSITE_STOCK_PURE;
    const bool offHighwaySelected =
        selected.ulcOffHighwayMode != ULC_COMPOSITE_STOCK_PURE;
    if (!selected.alcOffHighwayEnabled && !blindSelected &&
        !offHighwaySelected && !selected.confirmFreeEnabled) return;

    bool dasValid;
    uint8_t dasState;
    portENTER_CRITICAL(&stateMux);
    dasValid = dasAutopilotStateValid;
    dasState = dasAutopilotState4;
    portEXIT_CRITICAL(&stateMux);
    const bool ulcSelected = blindSelected || offHighwaySelected;
    const UlcCompositeGatesPure gates = {
        dasStateAutosteerPure(dasValid, dasState),
        ulcPolicyApGateOpenPure(ulcSelected, dasValid, dasState),
        ulcNoConfirmGateOpenWithTimingPure(
            activeProfileUlcNoConfirmSupported(),
            selected.confirmFreeEnabled, confirmTiming, dasValid, dasState)};

    if (selected.alcOffHighwayEnabled && !gates.alcAutosteerOpen) {
        portENTER_CRITICAL(&lab3f8Mux);
        lab3f8GateBlocked++;
        portEXIT_CRITICAL(&lab3f8Mux);
    }
    if (ulcSelected && !gates.ulcApOpen) {
        portENTER_CRITICAL(&lab3f8Mux);
        if (blindSelected) lab3f8GateBlocked++;
        if (offHighwaySelected) ulcOffHighwayGateBlocked++;
        portEXIT_CRITICAL(&lab3f8Mux);
    }
    if (selected.confirmFreeEnabled && !gates.confirmFreeOpen) {
        portENTER_CRITICAL(&lab3f8Mux);
        ulcNoConfirmGateBlocked++;
        ulcNoConfirmGateBlockedB++;
        portEXIT_CRITICAL(&lab3f8Mux);
    }

    twai_message_t out = src;
    out.flags = 0;
    const uint8_t stockBlindBefore =
        (uint8_t)readBitsLE(out.data, 52, 2);
    const UlcCompositeResultPure result =
        ulcCompose3f8Pure(out.data, out.data_length_code, selected, gates);
    if (!result.changed) return;

    if (!twaiNonSummonAdmissionOpen()) {
        portENTER_CRITICAL(&lab3f8Mux);
        lab3f8TxFail++;
        if (result.ulcOffHighwayChanged) ulcOffHighwayTxFail++;
        if (result.confirmFreeChanged) {
          ulcNoConfirmTxFail++;
          ulcNoConfirmTxBFail++;
        }
        portEXIT_CRITICAL(&lab3f8Mux);
        return;
    }

    const uint32_t now = (uint32_t)millis();
    const uint32_t txEpoch = canTxEpochSnapshot();
    const esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    researchCaptureObserveTxVh((uint16_t)out.identifier, out.data_length_code, out.data, err == ESP_OK);
    const uint8_t actualTxBlind = (uint8_t)readBitsLE(out.data, 52, 2);
    portENTER_CRITICAL(&lab3f8Mux);
    lab3f8LastTxValid = true;
    lab3f8LastTxBlind = actualTxBlind;
    lab3f8LastTxStockBlind = stockBlindBefore;
    lab3f8LastTxSelectedBlind = selected.blindSpotMode;
    lab3f8LastTxBlindChanged = result.blindSpotChanged;
    lab3f8LastTxResult = (err == ESP_OK) ? 1 : 2;
    lab3f8LastTxMs = now;
    lab3f8LastTxBit56 = getBit(out.data, 56);
    if (err == ESP_OK) lab3f8TxOk++;
    else               lab3f8TxFail++;
    if (result.ulcOffHighwayChanged) {
      ulcOffHighwayLastTxValid = true;
      ulcOffHighwayLastTxRaw = uiUlcOffHighwayReadRawPure(out.data, out.data_length_code);
      ulcOffHighwayLastTxMs = now;
      if (err == ESP_OK) ulcOffHighwayTxOk++; else ulcOffHighwayTxFail++;
    }
    if (result.confirmFreeChanged) {
      ulcNoConfirmLastTxValid = true;
      ulcNoConfirmLastTxBit1 = getBit(out.data, 1);
      ulcNoConfirmLastTxMs = now;
      if (err == ESP_OK) { ulcNoConfirmTxOk++; ulcNoConfirmTxBOk++; }
      else { ulcNoConfirmTxFail++; ulcNoConfirmTxBFail++; }
    }
    portEXIT_CRITICAL(&lab3f8Mux);
}

static void injectUlcSnooze3fdMux1(const twai_message_t &src) {
    const uint32_t now = (uint32_t)millis();
    if (!ulcSnoozeRequestActive(now) || !autoBlinkerNOAGateOpen(now)) return;

    twai_message_t out = src;
    out.flags = 0;
    if (getBit(out.data, 36)) {
      ulcSnoozeFinishRequest();
      return;
    }
    setBit(out.data, 36, true); // UI_ulcSnooze

    if (!twaiNonSummonAdmissionOpen()) {
      portENTER_CRITICAL(&ulcSnoozeMux);
      ulcSnoozeTxFail++;
      portEXIT_CRITICAL(&ulcSnoozeMux);
      sumTxFail++;
      return;
    }

    const uint32_t txEpoch = canTxEpochSnapshot();
    const esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    if (err == ESP_OK) sumTxOk++;
    else               sumTxFail++;

    portENTER_CRITICAL(&ulcSnoozeMux);
    if (err == ESP_OK) ulcSnoozeTxOk++;
    else               ulcSnoozeTxFail++;
    portEXIT_CRITICAL(&ulcSnoozeMux);
    if (err == ESP_OK) ulcSnoozeFinishRequest();
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
// TLSSC has its own AP-active gate. It is independent of the confirmed Summon
// session monitor and the v3.6 R79 default-on/manual-suspend policy.
static uint8_t r79BlockReasonForRuntimeState(uint8_t state) {
  switch (state) {
    case R79_TX_STATE_SUSPENDED: return R79LAB_BLOCK_MANUAL;
    case R79_TX_STATE_WAIT_TEMPLATE: return R79LAB_BLOCK_TEMPLATE;
    case R79_TX_STATE_CAN_OFFLINE: return R79LAB_BLOCK_CAN_OFFLINE;
    case R79_TX_STATE_ADMIN_HOLD: return R79LAB_BLOCK_ADMIN_HOLD;
    default: return R79LAB_BLOCK_NONE;
  }
}

static void r79LabRecordSuppressedState(const R79RuntimeStatus &status, uint32_t now) {
  portENTER_CRITICAL(&r79LabMux);
  r79LabLastBlockReason = r79BlockReasonForRuntimeState(status.state);
  r79LabLastBlockMs = now;
  if (status.state == R79_TX_STATE_SUSPENDED) r79LabManualSuspendSkip++;
  else if (status.state == R79_TX_STATE_WAIT_TEMPLATE) r79LabNoTemplateSkip++;
  else if (status.state == R79_TX_STATE_CAN_OFFLINE) r79LabCanOfflineSkip++;
  else if (status.state == R79_TX_STATE_ADMIN_HOLD) r79LabAdminHoldSkip++;
  portEXIT_CRITICAL(&r79LabMux);
}

static esp_err_t r79LabDirectTwaiTransmit(const twai_message_t *msg) {
  if (!msg) return ESP_ERR_INVALID_ARG;
  if (canTxAdministrativeHold || !twaiReady) {
    canBTraceRecordTx(msg, ESP_ERR_INVALID_STATE);
    return ESP_ERR_INVALID_STATE;
  }
  const esp_err_t err = twai_transmit(
      msg, pdMS_TO_TICKS(R79_FIXED_FAST_WAIT_MS_PURE));
  canBTraceRecordTx(msg, err);
  return err;
}

static void r79RetryCancel() {
  portENTER_CRITICAL(&r79LabMux);
  r79RetryPending = false;
  r79RetryIndex = 0;
  r79RetryOriginKind = R79LAB_TX_NONE;
  r79RetryDueMs = 0;
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79RetrySchedule(uint8_t originKind, uint32_t now) {
  const uint16_t delayMs = r79RetryDelayMsPure(0);
  if (!delayMs) return;
  portENTER_CRITICAL(&r79LabMux);
  r79RetryPending = true;
  r79RetryIndex = 0;
  r79RetryOriginKind = originKind;
  r79RetryDueMs = now + delayMs;
  r79RetryScheduled++;
  portEXIT_CRITICAL(&r79LabMux);
}

// Minimal authorization gate for the d2 receive-synchronized fast path. This
// deliberately preserves the existing v3.6 fail-open policy: unknown/stale
// gear or DAS does not suppress R79. Only the already-established manual D/R
// latch blocks TX, while AP or confirmed Summon wins exactly as before.
static uint8_t r79FastReactiveBlockReason() {
  if (canTxAdministrativeHold) return R79LAB_BLOCK_ADMIN_HOLD;
  if (!twaiReady) return R79LAB_BLOCK_CAN_OFFLINE;

  bool apActive = false;
  bool summonConfirmed = false;
  R79ManualSuppressionPure manual = {};
  portENTER_CRITICAL(&stateMux);
  apActive = dasStateApActivePure(dasAutopilotStateValid, dasAutopilotState4);
  summonConfirmed = gateSummoning;
  manual = r79ManualSuppression;
  portEXIT_CRITICAL(&stateMux);

  const R79TxDecisionPure decision = r79TxDecisionPure(apActive, summonConfirmed, manual);
  return decision.txEnabled ? R79LAB_BLOCK_NONE : R79LAB_BLOCK_MANUAL;
}

static void r79RecordTransportBlocked(uint8_t reason, uint32_t now, bool countFast) {
  portENTER_CRITICAL(&r79LabMux);
  if (countFast) r79FastEchoBlocked++;
  r79LabLastBlockReason = reason;
  r79LabLastBlockMs = now;
  if (reason == R79LAB_BLOCK_MANUAL) r79LabManualSuspendSkip++;
  else if (reason == R79LAB_BLOCK_CAN_OFFLINE) r79LabCanOfflineSkip++;
  else if (reason == R79LAB_BLOCK_ADMIN_HOLD) r79LabAdminHoldSkip++;
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79FastReactiveRecordBlocked(uint8_t reason, uint32_t now) {
  r79RecordTransportBlocked(reason, now, true);
}

static void r79FastReactiveRecordLatency(uint32_t latencyUs) {
  portENTER_CRITICAL(&r79LabMux);
  r79FastEchoLatencyLastUs = latencyUs;
  if (r79FastEchoLatencySamples == 0 || latencyUs < r79FastEchoLatencyMinUs)
    r79FastEchoLatencyMinUs = latencyUs;
  if (latencyUs > r79FastEchoLatencyMaxUs) r79FastEchoLatencyMaxUs = latencyUs;
  r79FastEchoLatencyTotalUs += latencyUs;
  r79FastEchoLatencySamples++;
  if (latencyUs < 100U) r79FastEchoLt100Us++;
  else if (latencyUs < 250U) r79FastEchoLt250Us++;
  else if (latencyUs < 1000U) r79FastEchoLt1000Us++;
  else r79FastEchoGe1000Us++;
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79FastReactiveRecordTxState(esp_err_t err, uint32_t now) {
  portENTER_CRITICAL(&r79LabMux);
  if (err == ESP_OK) {
    r79LabLastTxMs = now;
    r79LabLastBlockReason = R79LAB_BLOCK_NONE;
    r79LabLastBlockMs = 0;
  } else {
    if (canTxAdministrativeHold) {
      r79LabLastBlockReason = R79LAB_BLOCK_ADMIN_HOLD;
      r79LabAdminHoldSkip++;
    } else if (!twaiReady) {
      r79LabLastBlockReason = R79LAB_BLOCK_CAN_OFFLINE;
      r79LabCanOfflineSkip++;
    } else {
      r79LabLastBlockReason = R79LAB_BLOCK_TX;
    }
    r79LabLastBlockMs = now;
  }
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79FastReactiveArmSuccessProbe(int64_t rxDequeueUs, int64_t requestUs,
                                             uint32_t acceptedSerialBefore,
                                             uint32_t acceptedSerial,
                                             bool pipelineWasIdle) {
  portENTER_CRITICAL(&r79LabMux);
  if (r79FastSuccessPending) r79FastSuccessAmbiguous++;
  // The probe is valid only when the application TX pipeline was observed idle
  // and this Fast Echo was exactly the next accepted application enqueue. This
  // closes the tiny cross-task race between the pre-enqueue idle snapshot and
  // twai_transmit().
  if (!pipelineWasIdle || acceptedSerial == 0 ||
      acceptedSerial != (uint32_t)(acceptedSerialBefore + 1U)) {
    r79FastSuccessAmbiguous++;
    r79FastSuccessPending = false;
    r79FastSuccessAcceptedSerial = 0;
  } else {
    r79FastSuccessPending = true;
    r79FastSuccessAcceptedSerial = acceptedSerial;
    r79FastSuccessRxDequeueUs = rxDequeueUs;
    r79FastSuccessRequestUs = requestUs;
  }
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79FastReactiveObserveTxSuccessAlert(int64_t observedUs,
                                                 uint32_t acceptedSerialNow) {
  bool pending = false;
  uint32_t expectedSerial = 0;
  int64_t rxUs = 0, reqUs = 0;
  portENTER_CRITICAL(&r79LabMux);
  pending = r79FastSuccessPending;
  expectedSerial = r79FastSuccessAcceptedSerial;
  rxUs = r79FastSuccessRxDequeueUs;
  reqUs = r79FastSuccessRequestUs;
  if (pending) {
    r79FastSuccessPending = false;
    r79FastSuccessAcceptedSerial = 0;
  }
  portEXIT_CRITICAL(&r79LabMux);
  if (!pending) return;

  if (expectedSerial == 0 || acceptedSerialNow != expectedSerial ||
      observedUs < rxUs || observedUs < reqUs) {
    portENTER_CRITICAL(&r79LabMux);
    r79FastSuccessAmbiguous++;
    portEXIT_CRITICAL(&r79LabMux);
    return;
  }

  const uint64_t rxDelta64 = (uint64_t)(observedUs - rxUs);
  const uint64_t reqDelta64 = (uint64_t)(observedUs - reqUs);
  const uint32_t rxDelta = rxDelta64 > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (uint32_t)rxDelta64;
  const uint32_t reqDelta = reqDelta64 > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (uint32_t)reqDelta64;
  portENTER_CRITICAL(&r79LabMux);
  r79FastSuccessLastRxUs = rxDelta;
  if (r79FastSuccessSamples == 0 || rxDelta < r79FastSuccessMinRxUs) r79FastSuccessMinRxUs = rxDelta;
  if (rxDelta > r79FastSuccessMaxRxUs) r79FastSuccessMaxRxUs = rxDelta;
  r79FastSuccessTotalRxUs += rxDelta;
  r79FastSuccessLastReqUs = reqDelta;
  if (r79FastSuccessSamples == 0 || reqDelta < r79FastSuccessMinReqUs) r79FastSuccessMinReqUs = reqDelta;
  if (reqDelta > r79FastSuccessMaxReqUs) r79FastSuccessMaxReqUs = reqDelta;
  r79FastSuccessTotalReqUs += reqDelta;
  r79FastSuccessSamples++;
  portEXIT_CRITICAL(&r79LabMux);
}

// Receive-synchronized R79 echo. Called as the first special-case operation
// after TWAI dequeues a standard 0x3FD mux1 frame. The production path has one
// fixed bounded 2 ms queue-admission wait; payload authorization, emergency
// flush and bounded retry semantics remain unchanged.
static bool r79CopyLatestStock(uint8_t out[8]) {
  bool valid;
  portENTER_CRITICAL(&r79LabMux);
  valid = r79LabStockValid;
  if (valid) memcpy(out, r79LabLastStockRaw, 8);
  portEXIT_CRITICAL(&r79LabMux);
  return valid;
}

static bool r79LabTransmitOnce(const uint8_t *stock, uint32_t now,
                               uint8_t txKind, esp_err_t *errOut = nullptr) {
  if (errOut) *errOut = ESP_ERR_INVALID_ARG;
  if (!stock) return false;

  const R79RuntimeStatus status = r79RuntimeStatusSnapshot(now);
  if (status.state != R79_TX_STATE_ACTIVE) {
    r79LabRecordSuppressedState(status, now);
    if (errOut) *errOut = ESP_ERR_INVALID_STATE;
    return false;
  }

  twai_message_t out = {};
  out.identifier = 0x3FD;
  out.data_length_code = 8;
  out.flags = 0;
  memcpy(out.data, stock, 8);
  if (readMuxID(out.data) != 1 || !r79LabApplySelectedBits(out.data)) return false;

  portENTER_CRITICAL(&r79LabMux);
  r79LabAppliedFrames++;
  portEXIT_CRITICAL(&r79LabMux);

  const esp_err_t err = r79LabDirectTwaiTransmit(&out);
  if (errOut) *errOut = err;
  r79LabRecordTxResult(err == ESP_OK, out, txKind);

  portENTER_CRITICAL(&r79LabMux);
  if (err == ESP_OK) {
    r79LabLastTxMs = now;
    r79LabLastBlockReason = R79LAB_BLOCK_NONE;
    r79LabLastBlockMs = 0;
  } else {
    if (canTxAdministrativeHold) {
      r79LabLastBlockReason = R79LAB_BLOCK_ADMIN_HOLD;
      r79LabAdminHoldSkip++;
    } else if (!twaiReady) {
      r79LabLastBlockReason = R79LAB_BLOCK_CAN_OFFLINE;
      r79LabCanOfflineSkip++;
    } else {
      r79LabLastBlockReason = R79LAB_BLOCK_TX;
    }
    r79LabLastBlockMs = now;
  }
  portEXIT_CRITICAL(&r79LabMux);
  return err == ESP_OK;
}

// Production request path: one normal R79 attempt. In READY/ACTIVE only, an
// enqueue timeout may destructively clear the pending TWAI TX queue and retry
// the newest R79 template once immediately. If it still fails, schedule the
// bounded +5/+15/+30 ms recovery sequence. Queue flush is never used in NORMAL
// or PARK_STANDBY.
static bool r79LabTransmitShadow(const uint8_t *stock, uint32_t now,
                                 uint8_t txKind) {
  esp_err_t err = ESP_ERR_INVALID_ARG;
  if (r79LabTransmitOnce(stock, now, txKind, &err)) {
    r79RetryCancel();
    return true;
  }

  uint8_t priority = summonPriorityStateSnapshot(now);
  if (!summonPriorityAllowsR79FlushPure(priority)) return false;

  if (err == ESP_ERR_TIMEOUT && !canTxAdministrativeHold && twaiReady) {
    portENTER_CRITICAL(&r79LabMux);
    r79EmergencyQueueFlushCount++;
    portEXIT_CRITICAL(&r79LabMux);

    if (twai_clear_transmit_queue() == ESP_OK) {
      uint8_t latest[8] = {};
      if (!r79CopyLatestStock(latest)) memcpy(latest, stock, 8);
      esp_err_t flushRetryErr = ESP_ERR_INVALID_ARG;
      const bool flushRetryOk = r79LabTransmitOnce(
          latest, (uint32_t)millis(), R79LAB_TX_RETRY, &flushRetryErr);
      portENTER_CRITICAL(&r79LabMux);
      if (flushRetryOk) r79FlushTriggeredRetryOk++;
      else r79FlushTriggeredRetryFail++;
      portEXIT_CRITICAL(&r79LabMux);
      if (flushRetryOk) {
        r79RetryCancel();
        return true;
      }
      err = flushRetryErr;
    }
  }

  const uint32_t retryNow = (uint32_t)millis();
  priority = summonPriorityStateSnapshot(retryNow);
  if (summonPriorityAllowsR79FlushPure(priority) &&
      r79RuntimeStatusSnapshot(retryNow).state == R79_TX_STATE_ACTIVE) {
    r79RetrySchedule(txKind, retryNow);
  }
  return false;
}

static void r79LabRetryTick() {
  const uint32_t now = (uint32_t)millis();
  bool pending;
  uint8_t retryIndex, originKind;
  uint32_t dueMs;
  portENTER_CRITICAL(&r79LabMux);
  pending = r79RetryPending;
  retryIndex = r79RetryIndex;
  originKind = r79RetryOriginKind;
  dueMs = r79RetryDueMs;
  portEXIT_CRITICAL(&r79LabMux);
  if (!pending || (int32_t)(now - dueMs) < 0) return;

  const uint8_t priority = summonPriorityStateSnapshot(now);
  const R79RuntimeStatus runtime = r79RuntimeStatusSnapshot(now);
  if (!summonPriorityAllowsR79FlushPure(priority) ||
      runtime.state != R79_TX_STATE_ACTIVE) {
    r79RetryCancel();
    return;
  }

  if (originKind == R79LAB_TX_PERIODIC) {
    bool safe = false;
    portENTER_CRITICAL(&r79LabMux);
    safe = r79FixedQuietRetrySafePure(r79FixedQuietState, now);
    if (!safe) r79QuietGuardSkip++;
    if (!safe) {
      r79RetryPending = false;
      r79RetryIndex = 0;
      r79RetryOriginKind = R79LAB_TX_NONE;
      r79RetryDueMs = 0;
    }
    portEXIT_CRITICAL(&r79LabMux);
    if (!safe) return;
  }

  uint8_t stock[8] = {};
  if (!r79CopyLatestStock(stock)) {
    r79RetryCancel();
    return;
  }

  esp_err_t err = ESP_ERR_INVALID_ARG;
  portENTER_CRITICAL(&r79LabMux);
  r79LastRetryMs = now;
  r79LabLastAttemptMs = now;
  portEXIT_CRITICAL(&r79LabMux);
  const bool ok = r79LabTransmitOnce(stock, now, R79LAB_TX_RETRY, &err);

  if (ok) {
    portENTER_CRITICAL(&r79LabMux);
    r79RetryTxOk++;
    if (originKind == R79LAB_TX_PERIODIC) r79LabLastPeriodicTxMs = now;
    portEXIT_CRITICAL(&r79LabMux);
    r79RetryCancel();
    return;
  }

  const uint8_t nextIndex = (uint8_t)(retryIndex + 1U);
  portENTER_CRITICAL(&r79LabMux);
  r79RetryTxFail++;
  if (nextIndex >= 3U) {
    r79RetryExhausted++;
    r79RetryPending = false;
    r79RetryIndex = 0;
    r79RetryOriginKind = R79LAB_TX_NONE;
    r79RetryDueMs = 0;
  } else {
    const uint16_t delayMs = r79RetryDelayMsPure(nextIndex);
    r79RetryIndex = nextIndex;
    r79RetryDueMs = now + delayMs;
    r79RetryScheduled++;
  }
  portEXIT_CRITICAL(&r79LabMux);
}

// v3.7 production transport. There is no transport, wait-mode or scheduler
// selector in this path: accepted stock MUX1 is echoed with a bounded 2 ms
// enqueue wait, and accepted stock MUX2 owns one quiet-window slot at +150 ms.
static bool r79FixedFastEcho(const twai_message_t &src, int64_t rxDequeueUs) {
  if (src.data_length_code < 8 || readMuxID(src.data) != 1u) return false;
  const uint32_t now = (uint32_t)millis();
  const uint8_t blockReason = r79FastReactiveBlockReason();
  if (blockReason != R79LAB_BLOCK_NONE) {
    r79FastReactiveRecordBlocked(blockReason, now);
    return false;
  }

  uint8_t bit18Policy;
  portENTER_CRITICAL(&r79LabMux);
  bit18Policy = r79Bit18Policy;
  portEXIT_CRITICAL(&r79LabMux);

  twai_message_t out = {};
  out.identifier = 0x3FD;
  out.data_length_code = 8;
  out.flags = 0;
  memcpy(out.data, src.data, 8);
  r79FixedApplyBitsPure(out.data, bit18Policy);

  const bool pipelineWasIdle = canBTxPipelineIdleSnapshot();
  const uint32_t acceptedSerialBefore = canBTxAcceptedSerialSnapshot();
  const int64_t txRequestUs = esp_timer_get_time();
  const uint32_t latencyUs = txRequestUs > rxDequeueUs
      ? (uint32_t)(((uint64_t)(txRequestUs - rxDequeueUs) > 0xFFFFFFFFULL)
          ? 0xFFFFFFFFULL : (uint64_t)(txRequestUs - rxDequeueUs))
      : 0u;
  const esp_err_t err = twai_transmit(&out, pdMS_TO_TICKS(R79_FIXED_FAST_WAIT_MS_PURE));
  const uint32_t acceptedSerial = canBTraceRecordTx(&out, err);
  if (err == ESP_OK) {
    r79FastReactiveArmSuccessProbe(rxDequeueUs, txRequestUs,
        acceptedSerialBefore, acceptedSerial, pipelineWasIdle);
  }

  portENTER_CRITICAL(&r79LabMux);
  r79FastEchoAttempts++;
  r79LabAppliedFrames++;
  r79LabLastAttemptMs = now;
  if (err == ESP_OK) r79FastEchoTxOk++; else r79FastEchoTxFail++;
  portEXIT_CRITICAL(&r79LabMux);
  r79FastReactiveRecordLatency(latencyUs);
  r79LabRecordTxResult(err == ESP_OK, out, R79LAB_TX_IMMEDIATE);
  r79FastReactiveRecordTxState(err, now);
  if (err == ESP_OK) {
    r79RetryCancel();
    return true;
  }

  uint8_t priority = summonPriorityStateSnapshot(now);
  esp_err_t recoveryErr = err;
  if (err == ESP_ERR_TIMEOUT && summonPriorityAllowsR79FlushPure(priority) &&
      !canTxAdministrativeHold && twaiReady) {
    portENTER_CRITICAL(&r79LabMux);
    r79EmergencyQueueFlushCount++;
    portEXIT_CRITICAL(&r79LabMux);
    if (twai_clear_transmit_queue() == ESP_OK &&
        r79FastReactiveBlockReason() == R79LAB_BLOCK_NONE) {
      const esp_err_t retryErr = twai_transmit(
          &out, pdMS_TO_TICKS(R79_FIXED_FAST_WAIT_MS_PURE));
      canBTraceRecordTx(&out, retryErr);
      portENTER_CRITICAL(&r79LabMux);
      r79LabAppliedFrames++;
      if (retryErr == ESP_OK) r79FlushTriggeredRetryOk++;
      else r79FlushTriggeredRetryFail++;
      portEXIT_CRITICAL(&r79LabMux);
      r79LabRecordTxResult(retryErr == ESP_OK, out, R79LAB_TX_RETRY);
      r79FastReactiveRecordTxState(retryErr, (uint32_t)millis());
      if (retryErr == ESP_OK) {
        r79RetryCancel();
        return true;
      }
      recoveryErr = retryErr;
    }
  }

  const uint32_t retryNow = (uint32_t)millis();
  priority = summonPriorityStateSnapshot(retryNow);
  if (summonPriorityAllowsR79FlushPure(priority) &&
      recoveryErr != ESP_ERR_INVALID_STATE &&
      r79FastReactiveBlockReason() == R79LAB_BLOCK_NONE) {
    r79RetrySchedule(R79LAB_TX_IMMEDIATE, retryNow);
  }
  return false;
}

static void r79FixedObserveStock(uint8_t mux, uint32_t nowMs) {
  portENTER_CRITICAL(&r79LabMux);
  const R79FixedQuietObserveResultPure result =
      r79FixedQuietObserveStockPure(r79FixedQuietState, mux, nowMs);
  if (result.armed) r79QuietArmCount++;
  if (result.cancelled) r79QuietCancelCount++;
  if (r79RetryPending && r79RetryOriginKind == R79LAB_TX_PERIODIC) {
    r79RetryPending = false;
    r79RetryIndex = 0;
    r79RetryOriginKind = R79LAB_TX_NONE;
    r79RetryDueMs = 0;
    r79PeriodicRetryCancelledByStock++;
  }
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79FixedTick() {
  r79LabRetryTick();
  const uint32_t now = (uint32_t)millis();
  const R79RuntimeStatus runtime = r79RuntimeStatusSnapshot(now);
  uint8_t stock[8] = {};
  bool haveStock = false;
  R79FixedQuietActionPure action;

  portENTER_CRITICAL(&r79LabMux);
  action = r79FixedQuietStepPure(
      r79FixedQuietState, now, runtime.state == R79_TX_STATE_ACTIVE);
  if (action != R79_FIXED_QUIET_WAIT_PURE) {
    r79QuietDueCount++;
    r79PeriodicSlotDue++;
  }
  if (action == R79_FIXED_QUIET_GUARD_SKIP_PURE) {
    r79QuietGuardSkip++;
    r79PeriodicSlotGuard++;
  } else if (action == R79_FIXED_QUIET_DISALLOWED_SKIP_PURE) {
    r79QuietDisallowedSkip++;
  } else if (action == R79_FIXED_QUIET_FIRE_PURE) {
    haveStock = r79LabStockValid;
    if (haveStock) memcpy(stock, r79LabLastStockRaw, sizeof(stock));
    r79LabLastAttemptMs = now;
    r79LabLastPeriodicRequestMs = now;
    r79QuietFireCount++;
    r79PeriodicSlotFire++;
  }
  portEXIT_CRITICAL(&r79LabMux);

  if (action != R79_FIXED_QUIET_FIRE_PURE) return;
  if (!haveStock) {
    r79LabRecordSuppressedState(runtime, now);
    return;
  }
  if (r79LabTransmitShadow(stock, now, R79LAB_TX_PERIODIC)) {
    portENTER_CRITICAL(&r79LabMux);
    r79LabLastPeriodicTxMs = (uint32_t)millis();
    portEXIT_CRITICAL(&r79LabMux);
  }
}

static bool setTlsscEnabled(bool enabled, bool persist) {
  portENTER_CRITICAL(&stateMux);
  const bool wasEnabled = tlsscEnabled;
  tlsscEnabled = enabled;
  if (!enabled && wasEnabled && tlsscInjectedActive) tlsscClearPending = true;
  if (enabled) tlsscClearPending = false;
  portEXIT_CRITICAL(&stateMux);
  if (persist) summonCfgSave();
  return true;
}

static bool setAutoBlinkerEnabled(bool enabled, bool persist) {
  if (enabled && !activeProfileAdvancedEapSupported()) return false;
  portENTER_CRITICAL(&blinkAMux);
  blinkAEnabled = enabled;
  if (!enabled) {
    autoBlinkerClearPendingLocked();
    blinkerTxClearPendingSourceLocked(BLINKER_TX_SOURCE_AUTO_PURE);
    // Auto Blinker enable is independent from an in-flight direct S3XY turn
    // command. Only planner-driven pulses are cancelled here.
    if (!oneShotDirect) {
      oneShotTurn = STALK_IDLE;
      oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
      oneShotUntil = 0;
      oneShotReleaseAt = 0;
      activeTurn = STALK_IDLE;
    }
    lastReqDir = 0;
    autoRequestLastSeenMs = 0;
  }
  portEXIT_CRITICAL(&blinkAMux);
  if (enabled) evaluateAutoBlinker();
  if (persist) summonCfgSave();
  return true;
}

static void toggleTlsscEnabledFromButton() {
  bool enabled;
  portENTER_CRITICAL(&stateMux); enabled = tlsscEnabled; portEXIT_CRITICAL(&stateMux);
  setTlsscEnabled(!enabled, true);
  s3xyLogPush(S3XY_LOG_INFO, enabled ? "TLSSC -> OFF" : "TLSSC -> ON");
}

static void toggleAutoBlinkerEnabledFromButton() {
  bool enabled;
  portENTER_CRITICAL(&blinkAMux); enabled = blinkAEnabled; portEXIT_CRITICAL(&blinkAMux);
  if (!setAutoBlinkerEnabled(!enabled, true)) {
    s3xyLogPush(S3XY_LOG_INFO, "Auto Blinker toggle unavailable for current profile");
    return;
  }
  s3xyLogPush(S3XY_LOG_INFO, enabled ? "Auto Blinker -> OFF" : "Auto Blinker -> ON");
}

// S3XY direct turn-signal action. This is deliberately independent from Auto
// Blinker enable/NOA/ALC state. Stalkless vehicles retain their RX-following
// 0x3C2 press/release cycle; stalk vehicles consume the request once on the
// next idle stock 0x249 instead of generating a 20 ms timed burst.
static bool requestTurnSignalPulseFromButton(uint8_t dir) {
  if (dir != 1 && dir != 2) return false;
  if (activeTurnSignalVariant == TURN_SIGNAL_UNSET) return false;

  const uint32_t now = (uint32_t)millis();
  uint32_t eventToken;

  portENTER_CRITICAL(&blinkAMux);
  // A direct user command supersedes an unfired planner-driven Auto Blinker
  // request, but does not change the persistent Auto Blinker enable setting.
  autoBlinkerClearPendingLocked();
  blinkerTxClearPendingSourceLocked(BLINKER_TX_SOURCE_AUTO_PURE);
  lastReqDir = 0;
  autoRequestLastSeenMs = 0;
  eventToken = blinkerNextEventTokenLocked(s3xyBlinkerEventToken);
  portEXIT_CRITICAL(&blinkAMux);
  return requestBlinkerTx(dir, BLINKER_TX_SOURCE_S3XY_PURE,
                          eventToken, now);
}

static void injectTLSSC(const twai_message_t &src) {
    const uint32_t now = (uint32_t)millis();
    const uint32_t txEpoch = canTxEpochSnapshot();
    bool en, ap, noa, blockNoa, injected, clearPending;
    portENTER_CRITICAL(&stateMux);
    en = tlsscEnabled;
    ap = gateAPActive;
    noa = gateNOAActive;
    blockNoa = tlsscBlockInNoa;
    injected = tlsscInjectedActive;
    clearPending = tlsscClearPending;
    portEXIT_CRITICAL(&stateMux);

    const bool highwayBlocked = tlsscHighwayGateBlocked(now);
    if (highwayBlocked) {
      portENTER_CRITICAL(&roadContextMux);
      tlsscHighwayGateBlockedCount++;
      portEXIT_CRITICAL(&roadContextMux);
    }
    const bool desiredActive = en && ap && !highwayBlocked && !(blockNoa && noa);

    // Once this firmware has asserted TLSSC, every transition out of the
    // active policy sends exactly one explicit OFF overlay before returning to
    // pure STOCK. This covers user OFF, AP exit, highway gate and NOA block.
    if (!desiredActive) {
      if (injected && !clearPending) {
        portENTER_CRITICAL(&stateMux);
        tlsscClearPending = true;
        clearPending = true;
        portEXIT_CRITICAL(&stateMux);
      }
      if (!clearPending) return;

      twai_message_t out = src;
      out.flags = 0;
      setBit(out.data, 38, false);
      setBit(out.data, 39, false);
      if (!twaiNonSummonAdmissionOpen()) {
        sumTxFail++;
        return;
      }
      const esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
      portENTER_CRITICAL(&stateMux);
      if (err == ESP_OK) {
        tlsscInjectedActive = false;
        tlsscClearPending = false;
        tlsscClearTxOk++;
      } else {
        tlsscClearPending = true;
        tlsscClearTxFail++;
      }
      portEXIT_CRITICAL(&stateMux);
      if (err == ESP_OK) sumTxOk++;
      else               sumTxFail++;
      return;
    }

    // Policy is active again before a pending clear was transmitted: cancel
    // the stale clear and keep/assert the ON overlay.
    if (clearPending) {
      portENTER_CRITICAL(&stateMux);
      tlsscClearPending = false;
      portEXIT_CRITICAL(&stateMux);
    }

    twai_message_t out = src;
    out.flags = 0;
    // If both bits are already ON in stock, do not create new ownership. If we
    // previously asserted TLSSC, however, preserve that ownership until the
    // requested one-shot OFF transition is delivered.
    if (getBit(out.data, 38) && getBit(out.data, 39)) return;

    setBit(out.data, 38, true);   // UI_fsdStopsControlEnabled = 1
    setBit(out.data, 39, true);   // UI_fsdContinueOnGreenWithCIPV = 1

    // TLSSC is lower priority than Summon and must never block CAN B RX.
    if (!twaiNonSummonAdmissionOpen()) {
      sumTxFail++;
      return;
    }
    const esp_err_t err = canTxTwaiTransmit(&out, txEpoch);
    if (err == ESP_OK) {
      sumTxOk++;
      portENTER_CRITICAL(&stateMux);
      tlsscInjectedActive = true;
      portEXIT_CRITICAL(&stateMux);
    } else {
      sumTxFail++;
    }
}

// ═══════════════════════════════════════════════════════════════
// NVS schema migration
// Keep obsolete-key cleanup one-shot instead of repeating remove() calls on every boot.
// Schema 1 retires legacy Summon/R79 keys and canonicalizes S3XY 3D49 identity as raw bN bytes.
// Schema 2 separates production ULC policy from the LAB-only 0x293 experiment.
// Schema 3 promotes the fixed R79 bit18 policy and timing values to production.
// ═══════════════════════════════════════════════════════════════
static constexpr uint16_t NVS_SCHEMA_ULC = 2;
static constexpr uint16_t NVS_SCHEMA_CURRENT = 3;
static uint16_t nvsSchemaVersion = 0;
static uint16_t nvsSchemaBootVersion = 0;

static void nvsSchemaRead() {
  Preferences p;
  uint16_t version = 0;
  if (p.begin("t2meta", true)) {
    version = p.getUShort("schema", 0);
    p.end();
  }
  nvsSchemaVersion = version;
  nvsSchemaBootVersion = version;
}

static void nvsSchemaFinalize() {
  if (nvsSchemaBootVersion >= 1) return;

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
    T2CAN_SERIAL_PRINTLN("NVS schema migration incomplete; will retry next boot");
    return;
  }

  bool markerReady = nvsSchemaVersion >= 1;
  if (!markerReady && p.begin("t2meta", false)) {
    const size_t written = p.putUShort("schema", 1);
    p.end();
    markerReady = written > 0;
    if (markerReady) nvsSchemaVersion = 1;
  }
  if (markerReady) {
    Preferences cleanup;
    if (cleanup.begin("summon", false)) {
      cleanup.remove("blkDly17");
      cleanup.end();
    }
    T2CAN_SERIAL_PRINTLN("NVS schema 1 cleanup complete");
    return;
  }
  T2CAN_SERIAL_PRINTLN("NVS schema marker write failed; will retry next boot");
}

static void featureCfgLoad() {
    prefs.begin("features", false);
    labMenuEnabled = prefs.getBool("lab", false);
    bannedCar = prefs.getBool("banned", false);
    doorOpenCancelEnabled = prefs.getBool("doorCancel", false);
    tlsscRestoreEnabled = prefs.getBool("tlRestore", false);
    const bool persistedApDriveProfile = prefs.getBool("apDrive", false);
    const uint8_t persistedApRegen = prefs.getUChar("apRegen", AP_DRIVE_REGEN_REDUCED_RAW);
    const bool persistedRightScroll = prefs.getBool("rsEnabled", false);
    const uint16_t persistedRightScrollInterval = prefs.getUShort("rsInterval", AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE);
    const uint8_t persistedRightScrollWarning = prefs.getUChar("rsWarnS", AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE);
    apDriveProfileEnabled = activeProfileApDriveProfileSupported() && persistedApDriveProfile;
    apDriveProfileRegenRaw = apDriveRegenRawSelectablePure(persistedApRegen) ? persistedApRegen : AP_DRIVE_REGEN_REDUCED_RAW;
    if (!activeProfileApDriveProfileSupported() && persistedApDriveProfile) prefs.putBool("apDrive", false);
    if (!apDriveRegenRawSelectablePure(persistedApRegen)) prefs.putUChar("apRegen", AP_DRIVE_REGEN_REDUCED_RAW);
    apRightScrollEnabled = activeProfileApRightScrollSupported() && persistedRightScroll;
    apRightScrollIntervalSeconds = apRightScrollIntervalSanitizePure(persistedRightScrollInterval);
    apRightScrollVisualRepeatSeconds = apRightScrollWarningRepeatSanitizePure(persistedRightScrollWarning);
    if (!activeProfileApRightScrollSupported() && persistedRightScroll) prefs.putBool("rsEnabled", false);
    if (!apRightScrollIntervalValidPure(persistedRightScrollInterval))
      prefs.putUShort("rsInterval", AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE);
    if (!apRightScrollWarningRepeatValidPure(persistedRightScrollWarning))
      prefs.putUChar("rsWarnS", AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE);
    if (!activeProfileBodyControlsSupported() && !activeProfileIsYl() && doorOpenCancelEnabled) {
      // Door-open cancel consumes Body CAN on Standard 3/Y. A topology change
      // to Party+Chassis must not retain an enabled-but-unreachable feature.
      doorOpenCancelEnabled = false;
      prefs.putBool("doorCancel", false);
    }
    if (!activeProfileBannedCarSupported()) {
      // Model Y L does not expose Banned Car in v3.2 hotfix. Fail closed if an older
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
    prefs.putBool("apDrive", activeProfileApDriveProfileSupported() ? apDriveProfileEnabled : false);
    prefs.putUChar("apRegen", apDriveRegenRawSelectablePure(apDriveProfileRegenRaw) ? apDriveProfileRegenRaw : AP_DRIVE_REGEN_REDUCED_RAW);
    prefs.putBool("rsEnabled", activeProfileApRightScrollSupported() ? apRightScrollEnabled : false);
    prefs.putUShort("rsInterval", apRightScrollIntervalSanitizePure(apRightScrollIntervalSeconds));
    prefs.putUChar("rsWarnS", apRightScrollWarningRepeatSanitizePure(apRightScrollVisualRepeatSeconds));
    prefs.end();
}

static void summonCfgLoad() {
    prefs.begin("summon", false);
    tlsscEnabled  = prefs.getBool("tlssc", false);
    tlsscHighwayGateEnabled = prefs.getBool("tlHwy", false);
    tlsscBlockInNoa = prefs.getBool("tlNoa", false);
    blinkAEnabled = prefs.getBool("blkA", false);
    tlsscInjectedActive = false;
    tlsscClearPending = false;

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
    blinkANoaStabilizationSeconds =
        autoBlinkerNoaStabilizationSecondsSanitizePure(
            prefs.getUChar("noaStabS", BLINKA_NOA_STABILIZE_DEFAULT_S_PURE));
    blinkACancelPauseSeconds = autoBlinkerCancelPauseSecondsSanitizePure(
        prefs.getUChar("cancelPauseS", BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE));
    prefs.end();
}

static void summonCfgSave() {
    prefs.begin("summon", false);
    prefs.putBool("tlssc", tlsscEnabled);
    prefs.putBool("tlHwy", tlsscHighwayGateEnabled);
    prefs.putBool("tlNoa", tlsscBlockInNoa);
    prefs.putBool("blkA", blinkAEnabled);
    prefs.putUInt("blkADly", blinkADelayMs);
    prefs.putUChar("noaStabS", autoBlinkerNoaStabilizationSecondsSanitizePure(
        blinkANoaStabilizationSeconds));
    prefs.putUChar("cancelPauseS", autoBlinkerCancelPauseSecondsSanitizePure(
        blinkACancelPauseSeconds));
    prefs.end();
}

static bool featureConfigWriteSchema3Values(
    const FeatureConfigV37Pure &config) {
  Preferences p;
  if (!p.begin("r79", false)) return false;
  bool ok = p.putUChar("bit18", r79Bit18PolicySanitizePure(
      config.r79Bit18Mode)) > 0;
  p.end();
  if (!ok || !p.begin("summon", false)) return false;
  ok = p.putUChar("noaStabS", autoBlinkerNoaStabilizationSecondsSanitizePure(
           config.noaStabilizationSeconds)) > 0 &&
       p.putUChar("cancelPauseS", autoBlinkerCancelPauseSecondsSanitizePure(
           config.cancelPauseSeconds)) > 0;
  p.end();
  if (!ok || !p.begin("features", false)) return false;
  ok = p.putUChar("rsWarnS", apRightScrollWarningRepeatSanitizePure(
      config.rightScrollWarningSeconds)) > 0;
  p.end();
  return ok;
}

static bool featureConfigSchema3ReadBackMatches(
    const FeatureConfigV37Pure &expected) {
  Preferences p;
  if (!p.begin("r79", true)) return false;
  const bool r79Ok = p.getUChar("bit18", 0xFFu) ==
      r79Bit18PolicySanitizePure(expected.r79Bit18Mode);
  p.end();
  if (!r79Ok || !p.begin("summon", true)) return false;
  const bool summonOk =
      p.getUChar("noaStabS", 0u) ==
          autoBlinkerNoaStabilizationSecondsSanitizePure(
              expected.noaStabilizationSeconds) &&
      p.getUChar("cancelPauseS", 0u) ==
          autoBlinkerCancelPauseSecondsSanitizePure(
              expected.cancelPauseSeconds);
  p.end();
  if (!summonOk || !p.begin("features", true)) return false;
  const bool featuresOk = p.getUChar("rsWarnS", 0u) ==
      apRightScrollWarningRepeatSanitizePure(
          expected.rightScrollWarningSeconds);
  p.end();
  return featuresOk;
}

static void featureConfigCleanupRetiredR79Keys() {
  Preferences p;
  if (!p.begin("r79lab", false)) return;
  p.remove("smart18");
  p.remove("strategy");
  p.remove("period");
  p.remove("refresh");
  p.remove("sched");
  p.remove("shots");
  p.remove("postTx");
  p.remove("qAnchor");
  p.remove("post2En");
  p.remove("post2Off");
  p.remove("preEn");
  p.remove("preOff");
  p.remove("d2FastEcho");
  p.remove("d4Period");
  p.remove("d6Quiet");
  p.end();
}

static bool featureConfigMigrateToSchema3() {
  if (nvsSchemaVersion >= NVS_SCHEMA_CURRENT) {
    // A reset immediately after the durable marker may have interrupted only
    // cleanup. It is safe and intentional to retry cleanup on every such boot.
    featureConfigCleanupRetiredR79Keys();
    return true;
  }

  Preferences p;
  uint8_t legacyR79Bit18 = R79_BIT18_DEFAULT_PURE;
  if (!p.begin("r79lab", false)) return false;
  legacyR79Bit18 = p.getUChar("smart18", R79_BIT18_DEFAULT_PURE);
  p.end();

  FeatureConfigV37Pure config =
      featureConfigV37FromLegacyPure(legacyR79Bit18);
  if (!p.begin("summon", false)) return false;
  config.noaStabilizationSeconds =
      autoBlinkerNoaStabilizationSecondsSanitizePure(
          p.getUChar("noaStabS", BLINKA_NOA_STABILIZE_DEFAULT_S_PURE));
  config.cancelPauseSeconds = autoBlinkerCancelPauseSecondsSanitizePure(
      p.getUChar("cancelPauseS", BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE));
  p.end();
  if (!p.begin("features", false)) return false;
  config.rightScrollWarningSeconds =
      apRightScrollWarningRepeatSanitizePure(
          p.getUChar("rsWarnS", AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE));
  p.end();

  if (!featureConfigWriteSchema3Values(config) ||
      !featureConfigSchema3ReadBackMatches(config)) {
    T2CAN_SERIAL_PRINTLN("NVS schema 3 value verification failed; will retry");
    return false;
  }

  if (!p.begin("t2meta", false)) return false;
  const size_t markerWritten = p.putUShort("schema", NVS_SCHEMA_CURRENT);
  p.end();
  if (markerWritten == 0 || !p.begin("t2meta", true)) return false;
  const uint16_t durableMarker = p.getUShort("schema", 0);
  p.end();
  if (durableMarker != NVS_SCHEMA_CURRENT) return false;

  nvsSchemaVersion = NVS_SCHEMA_CURRENT;
  featureConfigCleanupRetiredR79Keys();
  return true;
}


static bool featureConfigWriteSchema2Values(
    const FeatureConfigMigrationPure &config) {
  Preferences p;
  bool ok = p.begin("ulc", false);
  if (ok) {
    ok = p.putBool("alcOff", config.ulc.alcOffHighwayEnabled) > 0 &&
         p.putUChar("blind", config.ulc.blindSpotMode) > 0 &&
         p.putUChar("ulcOff", config.ulc.ulcOffHighwayMode) > 0 &&
         p.putBool("confirm", config.ulc.confirmFreeEnabled) > 0 &&
         p.putUChar("timing", config.ulc.confirmFreeTiming) > 0;
    p.end();
  }
  if (!ok || !p.begin("alc293lab", false)) return false;
  ok = p.putBool("enabled", config.autoLaneChange.enabled) > 0 &&
       p.putUChar("bus", config.autoLaneChange.bus) > 0;
  p.end();
  return ok;
}

static bool featureConfigSchema2ReadBackMatches(
    const FeatureConfigMigrationPure &expected) {
  Preferences p;
  if (!p.begin("ulc", true)) return false;
  const bool ulcOk =
      p.getBool("alcOff", false) == expected.ulc.alcOffHighwayEnabled &&
      p.getUChar("blind", 0) == expected.ulc.blindSpotMode &&
      p.getUChar("ulcOff", 0) == expected.ulc.ulcOffHighwayMode &&
      p.getBool("confirm", false) == expected.ulc.confirmFreeEnabled &&
      p.getUChar("timing", 0) == expected.ulc.confirmFreeTiming;
  p.end();
  if (!ulcOk || !p.begin("alc293lab", true)) return false;
  const bool alcOk =
      p.getBool("enabled", false) == expected.autoLaneChange.enabled &&
      p.getUChar("bus", 0) == expected.autoLaneChange.bus;
  p.end();
  return alcOk;
}

static bool featureConfigMigrateToSchema2() {
  if (nvsSchemaVersion >= NVS_SCHEMA_ULC) return true;

  LegacyLab3f8ConfigPure legacy = {};
  Preferences p;
  // Opening read-write also creates the legacy namespace on a fresh install,
  // allowing an all-default migration instead of treating absence as failure.
  if (!p.begin("lab3f8", false)) return false;
  legacy.alcMode = p.getUChar("alc", LAB3F8_ALC_STOCK);
  legacy.blindMode = p.getUChar("ulcbs", LAB3F8_STOCK);
  legacy.ulcOffHighwayMode = p.getUChar("ulcoff", LAB3F8_STOCK);
  legacy.confirmFreeEnabled = p.getBool("ulcNoConf", false);
  legacy.confirmFreeTiming = p.getUChar(
      "ulcTiming", ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE);
  legacy.autoLaneChange293Enabled = p.getBool("autoLc293", false);
  legacy.autoLaneChange293Bus = p.getUChar(
      "autoLcBus", UI_AUTO_LC_BUS_BOTH_PURE);
  p.end();

  const FeatureConfigMigrationPure migrated = migrateLegacyLab3f8Pure(legacy);
  if (!featureConfigWriteSchema2Values(migrated) ||
      !featureConfigSchema2ReadBackMatches(migrated)) {
    T2CAN_SERIAL_PRINTLN("NVS schema 2 value verification failed; will retry");
    return false;
  }
  if (!p.begin("t2meta", false)) return false;
  const size_t markerWritten = p.putUShort("schema", NVS_SCHEMA_ULC);
  p.end();
  if (markerWritten == 0) return false;
  nvsSchemaVersion = NVS_SCHEMA_ULC;

  // Retire the former mixed-domain namespace only after the new values and
  // schema marker are durable. Obsolete experiment keys are cleanup-only.
  if (p.begin("lab3f8", false)) {
    p.remove("alc");
    p.remove("ulcbs");
    p.remove("ulcoff");
    p.remove("ulcNoConf");
    p.remove("ulcTiming");
    p.remove("autoLc293");
    p.remove("autoLcBus");
    p.remove("accfd");
    p.remove("ulcspd");
    p.remove("ulcBus");
    p.end();
  }
  return true;
}

static FeatureConfigMigrationPure featureConfigRuntimeSnapshot() {
  FeatureConfigMigrationPure config = {};
  portENTER_CRITICAL(&lab3f8Mux);
  config.ulc.alcOffHighwayEnabled =
      lab3f8AlcMode == LAB3F8_ALC_FORCE_ON;
  config.ulc.blindSpotMode = lab3f8UlcBlindMode;
  config.ulc.ulcOffHighwayMode = lab3f8UlcOffHighwayMode;
  config.ulc.confirmFreeEnabled = ulcNoConfirmEnabled;
  config.ulc.confirmFreeTiming = ulcNoConfirmTimingMode;
  portEXIT_CRITICAL(&lab3f8Mux);
  portENTER_CRITICAL(&autoLc293Mux);
  config.autoLaneChange.enabled = uiAutoLaneChangeEnabled;
  config.autoLaneChange.bus = uiAutoLaneChangeTargetBus;
  portEXIT_CRITICAL(&autoLc293Mux);
  return config;
}

static bool ulcCfgSave() {
  const FeatureConfigMigrationPure config = featureConfigRuntimeSnapshot();
  Preferences p;
  if (!p.begin("ulc", false)) return false;
  const bool ok =
      p.putBool("alcOff", config.ulc.alcOffHighwayEnabled) > 0 &&
      p.putUChar("blind", config.ulc.blindSpotMode) > 0 &&
      p.putUChar("ulcOff", config.ulc.ulcOffHighwayMode) > 0 &&
      p.putBool("confirm", config.ulc.confirmFreeEnabled) > 0 &&
      p.putUChar("timing", config.ulc.confirmFreeTiming) > 0;
  p.end();
  return ok;
}

static bool autoLaneChangeLabCfgSave() {
  const FeatureConfigMigrationPure config = featureConfigRuntimeSnapshot();
  Preferences p;
  if (!p.begin("alc293lab", false)) return false;
  const bool ok = p.putBool("enabled", config.autoLaneChange.enabled) > 0 &&
                  p.putUChar("bus", config.autoLaneChange.bus) > 0;
  p.end();
  return ok;
}

static bool ulcCfgLoadAndMigrate() {
  const bool migrated = featureConfigMigrateToSchema2();
  Preferences p;
  bool alcOff = false, confirm = false;
  uint8_t blind = LAB3F8_STOCK, ulcOff = LAB3F8_STOCK;
  uint8_t timing = ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE;
  if (migrated && p.begin("ulc", true)) {
    alcOff = p.getBool("alcOff", false);
    blind = p.getUChar("blind", LAB3F8_STOCK);
    ulcOff = p.getUChar("ulcOff", LAB3F8_STOCK);
    confirm = p.getBool("confirm", false);
    timing = p.getUChar("timing", ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE);
    p.end();
  }
  if (blind != LAB3F8_STOCK && blind > 2) blind = LAB3F8_STOCK;
  if (ulcOff != LAB3F8_STOCK && ulcOff > 1) ulcOff = LAB3F8_STOCK;
  if (!ulcNoConfirmTimingValidPure(timing))
    timing = ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE;
  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8AlcMode = alcOff ? LAB3F8_ALC_FORCE_ON : LAB3F8_ALC_STOCK;
  lab3f8UlcBlindMode = blind;
  lab3f8UlcOffHighwayMode = ulcOff;
  ulcNoConfirmEnabled = activeProfileUlcNoConfirmSupported() && confirm;
  ulcNoConfirmTimingMode = timing;
  portEXIT_CRITICAL(&lab3f8Mux);
  return migrated;
}

static bool autoLaneChangeLabCfgLoadAndMigrate() {
  const bool migrated = featureConfigMigrateToSchema2();
  Preferences p;
  bool enabled = false;
  uint8_t bus = UI_AUTO_LC_BUS_BOTH_PURE;
  if (migrated && p.begin("alc293lab", true)) {
    enabled = p.getBool("enabled", false);
    bus = p.getUChar("bus", UI_AUTO_LC_BUS_BOTH_PURE);
    p.end();
  }
  if (!uiAutoLaneChangeBusTargetValidPure(bus))
    bus = UI_AUTO_LC_BUS_BOTH_PURE;
  portENTER_CRITICAL(&autoLc293Mux);
  uiAutoLaneChangeEnabled = enabled;
  uiAutoLaneChangeTargetBus = bus;
  portEXIT_CRITICAL(&autoLc293Mux);
  return migrated;
}


static void r79CfgLoad() {
  prefs.begin("r79", false);
  const uint8_t stored = prefs.getUChar("bit18", R79_BIT18_DEFAULT_PURE);
  prefs.end();
  portENTER_CRITICAL(&r79LabMux);
  r79Bit18Policy = r79Bit18PolicySanitizePure(stored);
  r79FixedQuietState = {};
  portEXIT_CRITICAL(&r79LabMux);
}

static void r79CfgSave() {
  uint8_t bit18Policy;
  portENTER_CRITICAL(&r79LabMux);
  bit18Policy = r79Bit18Policy;
  portEXIT_CRITICAL(&r79LabMux);
  prefs.begin("r79", false);
  prefs.putUChar("bit18", r79Bit18PolicySanitizePure(bit18Policy));
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
