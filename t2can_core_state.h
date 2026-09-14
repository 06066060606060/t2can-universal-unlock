#pragma once

// COMMON TYPES / CORE STATE
// Kept in the same translation unit to preserve proven runtime behavior.

struct NagConfig;
struct NagContext;

// Declared before the first sketch function so Arduino's generated prototypes
// can reference the recovery-barrier type.
static constexpr uint8_t CAN_TX_FRESH_PARTY = 0x01;
static constexpr uint8_t CAN_TX_FRESH_VH = 0x02;
static constexpr uint8_t CAN_TX_FRESH_BOTH = CAN_TX_FRESH_PARTY | CAN_TX_FRESH_VH;

struct CanTxBarrierState {
  uint32_t epoch;
  uint8_t freshMask;
};

// BEGIN HOST-TESTABLE LOGIC
// Pure NAG helper kept free of Arduino/ESP-IDF dependencies for host regression tests.
static inline uint16_t nagModeCNextRawPure(uint16_t previousRaw, uint16_t &seed) {
  static constexpr uint16_t kMin = 0x898; // +1.50 Nm
  static constexpr uint16_t kMax = 0x8B6; // +1.80 Nm
  static constexpr uint16_t kStep = 15;   // 0.15 Nm / 200 ms maximum
  if (previousRaw < kMin) previousRaw = kMin;
  if (previousRaw > kMax) previousRaw = kMax;
  const uint16_t low = previousRaw > (uint16_t)(kMin + kStep) ? (uint16_t)(previousRaw - kStep) : kMin;
  const uint16_t high = previousRaw < (uint16_t)(kMax - kStep) ? (uint16_t)(previousRaw + kStep) : kMax;
  seed = (uint16_t)(seed * 1103u + 12345u);
  const uint16_t span = (uint16_t)(high - low + 1u);
  return (uint16_t)(low + (seed % span));
}

static inline void canTxBarrierInvalidatePure(CanTxBarrierState &state) {
  state.epoch++;
  if (state.epoch == 0) state.epoch = 1;
  state.freshMask = 0;
}

static inline void canTxBarrierMarkFreshPure(CanTxBarrierState &state, uint8_t busBit) {
  state.freshMask = (uint8_t)(state.freshMask | busBit);
}

static inline bool canTxBarrierAllowsMaskedPure(
    const CanTxBarrierState &state, uint32_t expectedEpoch, uint8_t requiredFreshMask) {
  return summonTxBarrierAllowsPure(
      state.epoch, state.freshMask, expectedEpoch, requiredFreshMask);
}

static inline bool canTxBarrierAllowsPure(
    const CanTxBarrierState &state, uint32_t expectedEpoch) {
  return canTxBarrierAllowsMaskedPure(state, expectedEpoch, CAN_TX_FRESH_BOTH);
}

static inline void canTxBarrierInvalidatePreservePure(
    CanTxBarrierState &state, uint8_t preservedFreshMask) {
  state.epoch++;
  if (state.epoch == 0) state.epoch = 1;
  state.freshMask = (uint8_t)(preservedFreshMask & CAN_TX_FRESH_BOTH);
}

static inline bool manualDasStatePure(uint8_t state4) {
  return state4 == 0 || state4 == 1 || state4 == 8 || state4 == 9 || state4 == 14;
}

struct RoadContext238Pure {
  bool valid;
  uint8_t roadClass;
  bool gpsRoadMatch;
  bool navRouteActive;
  bool controlledAccess;
  bool nextBranchLeftOffRamp;
  bool nextBranchRightOffRamp;
};

static inline RoadContext238Pure decodeRoadContext238Pure(const uint8_t *data, uint8_t dlc) {
  RoadContext238Pure r = {};
  if (!data || dlc < 5) return r;
  r.valid = true;
  r.roadClass = (uint8_t)((data[0] >> 3) & 0x07u);       // bits 3..5
  r.gpsRoadMatch = ((data[3] >> 4) & 0x01u) != 0;       // bit 28
  r.navRouteActive = ((data[3] >> 5) & 0x01u) != 0;     // bit 29
  r.controlledAccess = ((data[4] >> 5) & 0x01u) != 0;   // bit 37
  r.nextBranchLeftOffRamp = ((data[4] >> 6) & 0x01u) != 0;  // bit 38
  r.nextBranchRightOffRamp = ((data[4] >> 7) & 0x01u) != 0; // bit 39
  return r;
}

struct TlsscHighwayHysteresisPure {
  uint8_t positiveCount;
  uint8_t negativeCount;
  bool confirmed;
};

static inline bool tlsscHighwayUpdatePure(TlsscHighwayHysteresisPure &state,
                                          bool trustedMapContext,
                                          bool controlledAccess) {
  if (!trustedMapContext) {
    state.positiveCount = 0;
    state.negativeCount = 0;
    state.confirmed = false;
    return false;
  }

  if (controlledAccess) {
    state.negativeCount = 0;
    if (state.positiveCount < 2) state.positiveCount++;
    if (state.positiveCount >= 2) state.confirmed = true;
  } else {
    state.positiveCount = 0;
    if (state.negativeCount < 2) state.negativeCount++;
    if (state.negativeCount >= 2) state.confirmed = false;
  }
  return state.confirmed;
}
// END HOST-TESTABLE LOGIC

static unsigned long bootTime = 0;
static unsigned long canInitTime = 0;
static volatile bool twaiReady = false;
static volatile bool mcpReady = false;
static volatile unsigned long lastCanFrameMs = 0;
static volatile uint32_t canBeat = 0;
static volatile uint32_t canARxCount = 0;
static volatile uint32_t canBRxCount = 0;
static volatile uint32_t webBeat = 0;
static volatile uint32_t runtimeStatsResetCount = 0;
static volatile uint32_t runtimeStatsLastResetMs = 0;
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
static Preferences prefs;
// v3.3 top-level feature switches loaded before CAN/BLE runtime starts.
static volatile bool labMenuEnabled = false;
static volatile bool bannedCar = false;
static volatile bool tlsscRestoreEnabled = false;
static volatile bool doorOpenCancelEnabled = false;
static SemaphoreHandle_t canTxBarrierMutex = nullptr;
static CanTxBarrierState canTxBarrierState = {1, 0};

enum CanRxBus : uint8_t {
  CAN_RX_BUS_PARTY = 0,
  CAN_RX_BUS_VH = 1
};

// CAN A and CAN B run on separate FreeRTOS tasks. GCC atomic builtins prevent
// lost read-modify-write updates while retaining lock-free telemetry reads.
static inline void canRxObserve(uint8_t bus, uint32_t frameNow) {
  if (bus == CAN_RX_BUS_PARTY)
    __atomic_add_fetch(&canARxCount, 1U, __ATOMIC_RELAXED);
  else
    __atomic_add_fetch(&canBRxCount, 1U, __ATOMIC_RELAXED);
  __atomic_store_n(&lastCanFrameMs, (unsigned long)frameNow, __ATOMIC_RELAXED);
}

static inline uint32_t canRxTotal() {
  return __atomic_load_n(&canARxCount, __ATOMIC_RELAXED) +
         __atomic_load_n(&canBRxCount, __ATOMIC_RELAXED);
}



// Web server instance is shared by the web API module and a few feature status helpers.
static WebServer server(80);
