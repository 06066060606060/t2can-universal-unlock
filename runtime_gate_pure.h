#pragma once
#include <stdint.h>

// Host-testable AP state validity helpers. AP/NOA/manual state remains latched
// until the active topology receives a newer DAS_status frame or the relevant
// CAN recovery boundary invalidates that authorization.
static inline bool dasStateApActivePure(bool valid, uint8_t state4) {
  if (!valid) return false;
  return state4 == 3 || state4 == 4 || state4 == 5 || state4 == 6;
}

static inline bool dasStateNoaPure(bool valid, uint8_t state4) {
  return valid && state4 == 5;
}

static inline bool dasStateAutosteerPure(bool valid, uint8_t state4) {
  return valid && state4 == 3;
}

static inline bool dasStateManualPure(bool valid, uint8_t state4) {
  if (!valid) return false;
  return state4 == 0 || state4 == 1 || state4 == 8 || state4 == 9 || state4 == 14;
}

enum NagSkipReasonPure : uint8_t {
  NAG_SKIP_NONE = 0,
  NAG_SKIP_DISABLED = 1,
  NAG_SKIP_BOOT_DELAY = 2,
  NAG_SKIP_WARMUP = 3,
  NAG_SKIP_SELF_FRAME = 4,
  NAG_SKIP_HANDS_ON = 5,
  NAG_SKIP_AP_INVALID = 6,
  NAG_SKIP_AP_INACTIVE = 7,
  NAG_SKIP_DECISION = 8,
  NAG_SKIP_STOPPED = 9,
  NAG_SKIP_CADENCE = 10,
  NAG_SKIP_SPEED_STALE = 11
};

static inline NagSkipReasonPure nagEligibilityReasonPure(
    bool enabled, bool bootDelayPassed, bool canSeen, bool isOurs,
    uint8_t handsOnState, bool apStateValid, bool apActive) {
  if (!enabled) return NAG_SKIP_DISABLED;
  if (!bootDelayPassed) return NAG_SKIP_BOOT_DELAY;
  if (!canSeen) return NAG_SKIP_WARMUP;
  if (isOurs) return NAG_SKIP_SELF_FRAME;
  if (handsOnState > 1) return NAG_SKIP_HANDS_ON;
  if (!apStateValid) return NAG_SKIP_AP_INVALID;
  if (!apActive) return NAG_SKIP_AP_INACTIVE;
  return NAG_SKIP_NONE;
}

// Mode D is deliberately Party-CAN-local and portable.
// Its runtime gate uses only conditions available from the local 0x370 stream
// and local runtime state: enabled, warmup, self-echo rejection and OEM
// handsOnLevel <= 1. It does not depend on AP/ALC/steering context; the optional
// 0 km/h pause is applied separately from Party 0x257.
static inline NagSkipReasonPure nagPortableEligibilityReasonPure(
    bool enabled, bool bootDelayPassed, bool canSeen, bool isOurs,
    uint8_t handsOnState) {
  if (!enabled) return NAG_SKIP_DISABLED;
  if (!bootDelayPassed) return NAG_SKIP_BOOT_DELAY;
  if (!canSeen) return NAG_SKIP_WARMUP;
  if (isOurs) return NAG_SKIP_SELF_FRAME;
  if (handsOnState > 1) return NAG_SKIP_HANDS_ON;
  return NAG_SKIP_NONE;
}

static inline NagSkipReasonPure nagModeDPortableEligibilityReasonPure(
    bool enabled, bool bootDelayPassed, bool canSeen, bool isOurs,
    uint8_t handsOnState) {
  return nagPortableEligibilityReasonPure(enabled, bootDelayPassed, canSeen, isOurs, handsOnState);
}

// Portable D/E/F and event-based Mode H force HO=1 on injected frames.
// Match the exact recent transmitted 0x370 payload because Mode F/H can use
// dynamic torque values and must not rely on the legacy A/B/C torque-table
// heuristic.
static inline bool nagPortableRecentTxEchoPure(bool portableMode, bool lastTxValid,
                                                uint32_t ageMs, const uint8_t *rx,
                                                const uint8_t *tx, uint8_t dlc) {
  if (!portableMode || !lastTxValid || !rx || !tx || dlc != 8 || ageMs > 20u) return false;
  for (uint8_t i = 0; i < 8; i++) {
    if (rx[i] != tx[i]) return false;
  }
  return true;
}

static inline bool nagModeDRecentTxEchoPure(bool modeD, bool lastTxValid,
                                             uint32_t ageMs, const uint8_t *rx,
                                             const uint8_t *tx, uint8_t dlc) {
  return nagPortableRecentTxEchoPure(modeD, lastTxValid, ageMs, rx, tx, dlc);
}

static inline uint32_t nagGapMaxUpdatePure(uint32_t previousTxMs, uint32_t nowMs,
                                           uint32_t currentMaxMs) {
  if (previousTxMs == 0) return currentMaxMs;
  const uint32_t gap = (uint32_t)(nowMs - previousTxMs);
  return gap > currentMaxMs ? gap : currentMaxMs;
}

// Tesla Party CAN 0x257 DI_vehicleSpeed: Intel bit 12, length 12,
// factor 0.08 km/h, offset -40 km/h. Raw 4095 is SNA.
static inline bool nagDecodePartySpeedRawPure(const uint8_t *data, uint8_t dlc, uint16_t &rawOut) {
  if (!data || dlc < 3) return false;
  const uint16_t raw = (uint16_t)(((uint16_t)(data[1] >> 4) & 0x0Fu) | ((uint16_t)data[2] << 4));
  if (raw == 0x0FFFu) return false;
  rawOut = raw;
  return true;
}

static inline int32_t nagPartySpeedKphX100Pure(uint16_t raw) {
  // (raw * 0.08 - 40.0) * 100 = raw * 8 - 4000.
  return (int32_t)raw * 8 - 4000;
}

static inline bool nagPauseAtZeroBlocksPure(bool optionEnabled, bool speedValid,
                                             bool speedFresh, uint16_t raw) {
  return optionEnabled && speedValid && speedFresh && raw == 500u;
}

// Mode E: host-testable 0x370 cadence learner. It intentionally learns from
// each runtime session instead of persisting period/counter stride in NVS.
// One unstable OEM interval resets the learner and the current frame becomes
// the first sample of a new training window.
struct NagCadenceStatePure {
  bool haveLast;
  bool haveStep;
  bool stable;
  uint8_t cleanFrames;
  uint8_t lastCounter;
  uint8_t counterStep;
  uint32_t lastRxUs;
  uint32_t periodUs;
  uint32_t jitterUs;
  uint32_t intervalCount;
};

// Broad sanity bounds keep the learner portable across vehicles with different
// 0x370 rates (for example ~10 ms and ~40 ms captures). Stability itself is
// learned relative to the observed period rather than hard-coded to one car.
static constexpr uint32_t NAG_CADENCE_MIN_PERIOD_US = 5000u;
static constexpr uint32_t NAG_CADENCE_MAX_PERIOD_US = 100000u;
static constexpr uint32_t NAG_CADENCE_JITTER_FLOOR_US = 2000u;
static constexpr uint8_t NAG_CADENCE_JITTER_DIVISOR = 8u;
static constexpr uint8_t NAG_CADENCE_STABLE_FRAMES = 8u;

static inline void nagCadenceResetPure(NagCadenceStatePure &s) {
  s.haveLast = false;
  s.haveStep = false;
  s.stable = false;
  s.cleanFrames = 0;
  s.lastCounter = 0;
  s.counterStep = 0;
  s.lastRxUs = 0;
  s.periodUs = 0;
  s.jitterUs = 0;
  s.intervalCount = 0;
}

static inline void nagCadenceStartPure(NagCadenceStatePure &s, uint32_t nowUs, uint8_t counter) {
  s.haveLast = true;
  s.haveStep = false;
  s.stable = false;
  s.cleanFrames = 1;
  s.lastCounter = (uint8_t)(counter & 0x0Fu);
  s.counterStep = 0;
  s.lastRxUs = nowUs;
  s.periodUs = 0;
  s.jitterUs = 0;
  s.intervalCount = 0;
}

static inline bool nagCadenceObservePure(NagCadenceStatePure &s, uint32_t nowUs, uint8_t counter) {
  counter &= 0x0Fu;
  if (!s.haveLast) {
    nagCadenceStartPure(s, nowUs, counter);
    return false;
  }

  const uint32_t interval = (uint32_t)(nowUs - s.lastRxUs);
  const uint8_t step = (uint8_t)((counter - s.lastCounter) & 0x0Fu);
  if (interval < NAG_CADENCE_MIN_PERIOD_US || interval > NAG_CADENCE_MAX_PERIOD_US ||
      step == 0u || (s.haveStep && step != s.counterStep)) {
    nagCadenceStartPure(s, nowUs, counter);
    return false;
  }

  const uint32_t priorPeriod = s.periodUs == 0u ? interval : s.periodUs;
  const uint32_t jitter = interval > priorPeriod ? interval - priorPeriod : priorPeriod - interval;
  uint32_t jitterLimit = priorPeriod / NAG_CADENCE_JITTER_DIVISOR;
  if (jitterLimit < NAG_CADENCE_JITTER_FLOOR_US) jitterLimit = NAG_CADENCE_JITTER_FLOOR_US;
  if (jitter > jitterLimit) {
    nagCadenceStartPure(s, nowUs, counter);
    return false;
  }

  if (!s.haveStep) {
    s.counterStep = step;
    s.haveStep = true;
  }
  s.periodUs = s.intervalCount == 0u
      ? interval
      : (uint32_t)(((uint64_t)s.periodUs * s.intervalCount + interval) / (s.intervalCount + 1u));
  s.intervalCount++;
  if (jitter > s.jitterUs) s.jitterUs = jitter;
  s.lastCounter = counter;
  s.lastRxUs = nowUs;
  if (s.cleanFrames < 255u) s.cleanFrames++;
  s.stable = s.cleanFrames >= NAG_CADENCE_STABLE_FRAMES;
  return s.stable;
}

static inline uint8_t nagCadenceExpectedCounterPure(const NagCadenceStatePure &s) {
  if (!s.haveLast || !s.haveStep) return 0u;
  return (uint8_t)((s.lastCounter + s.counterStep) & 0x0Fu);
}

// Mode F: same-direction human-like hold/walk inside each 1 s burst, then
// alternate direction on the next burst. Raw 0x802 is 0 Nm for Tesla's
// (raw * 0.01 - 20.5) encoding. This helper is Party-only and AP-independent.
static inline bool nagModeFFaithfulRawPure(uint32_t sinceBootMs, uint16_t burstMs,
                                           uint16_t pauseMs, uint16_t &rawOut) {
  uint32_t cycleMs = (uint32_t)burstMs + (uint32_t)pauseMs;
  if (cycleMs == 0u) cycleMs = 1u;
  const uint32_t cycleIndex = sinceBootMs / cycleMs;
  const uint32_t phase = sinceBootMs % cycleMs;
  if (phase >= burstMs) return false;

  static const uint8_t magRaw[5] = {150u, 160u, 170u, 180u, 170u};
  const uint8_t idx = (uint8_t)((phase / 200u) % 5u);
  const uint16_t center = 2050u; // 20.5 / 0.01
  rawOut = (cycleIndex & 1u)
      ? (uint16_t)(center - magRaw[idx])
      : (uint16_t)(center + magRaw[idx]);
  return true;
}


enum McpTxResultReason : uint8_t {
  MCP_TX_OK = 0,
  MCP_TX_INVALID_MSG = 1,
  MCP_TX_MUTEX_BUSY = 2,
  MCP_TX_MCP_NOT_READY = 3,
  MCP_TX_EPOCH_MISMATCH = 4,
  MCP_TX_FRESH_MASK = 5,
  MCP_TX_SEND_ERROR = 6
};

static inline McpTxResultReason mcpTxResultReasonPure(
    bool msgValid, bool mutexAcquired, bool mcpReady, uint32_t currentEpoch,
    uint8_t freshMask, uint32_t expectedEpoch, bool sendOk) {
  if (!msgValid) return MCP_TX_INVALID_MSG;
  if (!mutexAcquired) return MCP_TX_MUTEX_BUSY;
  if (!mcpReady) return MCP_TX_MCP_NOT_READY;
  if (currentEpoch != expectedEpoch) return MCP_TX_EPOCH_MISMATCH;
  if (freshMask != 0x03u) return MCP_TX_FRESH_MASK;
  if (!sendOk) return MCP_TX_SEND_ERROR;
  return MCP_TX_OK;
}

// R79 bit18 is experimental and remains LAB-only. Fixed production bit19/47
// policy is intentionally independent of the LAB menu.
static inline bool r79SmartOverrideActivePure(bool labMenuEnabled, uint8_t smartMode) {
  return labMenuEnabled && smartMode != 0u;
}
