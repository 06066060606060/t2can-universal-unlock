#pragma once
#include <stdint.h>

// r79 fixed policy pure
enum R79Bit18PolicyPure : uint8_t {
  R79_BIT18_STOCK_PURE = 0,
  R79_BIT18_FORCE_0_PURE = 1
};

static constexpr uint8_t R79_BIT18_DEFAULT_PURE = R79_BIT18_STOCK_PURE;
static constexpr uint16_t R79_FIXED_FAST_WAIT_MS_PURE = 2u;
static constexpr uint16_t R79_FIXED_QUIET_DELAY_MS_PURE = 150u;
static constexpr uint16_t R79_FIXED_QUIET_HARD_END_MS_PURE = 340u;

enum R79Mode1WaitModePure : uint8_t {
  R79_MODE1_FAST_ECHO_PURE = 0,
  R79_MODE1_2MS_WAIT_PURE = 1
};

static constexpr uint8_t R79_MODE1_WAIT_DEFAULT_PURE =
    R79_MODE1_2MS_WAIT_PURE;

static inline uint8_t r79Mode1WaitModeSanitizePure(uint8_t mode) {
  return mode == R79_MODE1_FAST_ECHO_PURE || mode == R79_MODE1_2MS_WAIT_PURE
      ? mode : R79_MODE1_WAIT_DEFAULT_PURE;
}

static inline uint16_t r79Mode1TxWaitMsPure(uint8_t mode) {
  return r79Mode1WaitModeSanitizePure(mode) == R79_MODE1_FAST_ECHO_PURE
      ? 0u : R79_FIXED_FAST_WAIT_MS_PURE;
}

static inline bool r79Mode1DisableCancelsRetryPure(
    bool retryPending, bool retryIsPeriodic) {
  return retryPending && retryIsPeriodic;
}

static inline uint16_t r79FixedQuietDelaySanitizePure(uint16_t delayMs) {
  return delayMs <= R79_FIXED_QUIET_HARD_END_MS_PURE
      ? delayMs : R79_FIXED_QUIET_DELAY_MS_PURE;
}

static inline bool r79DelayTextParsePure(
    const char *text, uint16_t maxValue, uint16_t &out) {
  if (!text || !*text) return false;
  uint32_t value = 0u;
  for (const char *p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    value = value * 10u + (uint32_t)(*p - '0');
    if (value > maxValue) return false;
  }
  out = (uint16_t)value;
  return true;
}

static inline uint8_t r79Bit18PolicySanitizePure(uint8_t policy) {
  return policy == R79_BIT18_STOCK_PURE || policy == R79_BIT18_FORCE_0_PURE
      ? policy : R79_BIT18_DEFAULT_PURE;
}

static inline void r79FixedApplyBitsPure(
    uint8_t data[8], uint8_t policy, bool preserveBit47 = false) {
  if (!data) return;
  if (r79Bit18PolicySanitizePure(policy) == R79_BIT18_FORCE_0_PURE)
    data[2] = (uint8_t)(data[2] & (uint8_t)~(1u << 2));
  data[2] = (uint8_t)(data[2] & (uint8_t)~(1u << 3));
  if (!preserveBit47)
    data[5] = (uint8_t)(data[5] | (uint8_t)(1u << 7));
}

struct R79FixedQuietStatePure {
  bool pending;
  bool stockSeen;
  uint16_t delayMs;
  uint32_t cycleAnchorMs;
  uint32_t dueMs;
  uint32_t lastStockMs;
};

struct R79FixedQuietObserveResultPure {
  bool armed;
  bool cancelled;
};

enum R79FixedQuietActionPure : uint8_t {
  R79_FIXED_QUIET_WAIT_PURE = 0,
  R79_FIXED_QUIET_FIRE_PURE = 1,
  R79_FIXED_QUIET_GUARD_SKIP_PURE = 2,
  R79_FIXED_QUIET_DISALLOWED_SKIP_PURE = 3
};

static inline bool r79FixedTimeReachedPure(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static inline bool r79FixedDeadlineExpiredPure(
    uint32_t nowMs, uint32_t deadlineMs) {
  return (int32_t)(nowMs - deadlineMs) > 0;
}

static inline uint16_t r79FixedDeadlineWaitBudgetPure(
    uint32_t nowMs, uint32_t deadlineMs, uint16_t requestedWaitMs) {
  if (r79FixedDeadlineExpiredPure(nowMs, deadlineMs)) return 0u;
  const uint32_t remainingMs = (uint32_t)(deadlineMs - nowMs);
  return remainingMs < requestedWaitMs
      ? (uint16_t)remainingMs : requestedWaitMs;
}

static inline uint32_t r79FixedQuietHardDeadlinePure(
    const R79FixedQuietStatePure &state) {
  return state.cycleAnchorMs + R79_FIXED_QUIET_HARD_END_MS_PURE;
}

static inline R79FixedQuietObserveResultPure r79FixedQuietObserveStockPure(
    R79FixedQuietStatePure &state, uint8_t mux, uint32_t nowMs,
    bool reinjectEnabled, uint16_t delayMs) {
  R79FixedQuietObserveResultPure out = {};
  state.stockSeen = true;
  state.lastStockMs = nowMs;

  if (!reinjectEnabled) {
    if (state.pending) {
      state.pending = false;
      out.cancelled = true;
    }
    return out;
  }

  if (mux == 2u) {
    state.delayMs = r79FixedQuietDelaySanitizePure(delayMs);
    state.pending = true;
    state.cycleAnchorMs = nowMs;
    state.dueMs = nowMs + state.delayMs;
    out.armed = true;
    return out;
  }

  // MUX0/MUX1 marks the following stock cycle. A slot belonging to the
  // previous MUX2 must never leak into that cycle.
  if (state.pending && (mux == 0u || mux == 1u)) {
    state.pending = false;
    out.cancelled = true;
  }
  return out;
}

static inline R79FixedQuietObserveResultPure r79FixedQuietObserveStockPure(
    R79FixedQuietStatePure &state, uint8_t mux, uint32_t nowMs,
    uint16_t delayMs) {
  return r79FixedQuietObserveStockPure(
      state, mux, nowMs, true, delayMs);
}

static inline R79FixedQuietObserveResultPure r79FixedQuietObserveStockPure(
    R79FixedQuietStatePure &state, uint8_t mux, uint32_t nowMs) {
  return r79FixedQuietObserveStockPure(
      state, mux, nowMs, true, R79_FIXED_QUIET_DELAY_MS_PURE);
}

static inline bool r79FixedQuietSafeNowPure(
    const R79FixedQuietStatePure &state, uint32_t nowMs) {
  if (!state.stockSeen) return false;
  const uint32_t cycleAge = (uint32_t)(nowMs - state.cycleAnchorMs);
  const uint32_t stockAge = (uint32_t)(nowMs - state.lastStockMs);
  return cycleAge >= state.delayMs &&
         cycleAge <= R79_FIXED_QUIET_HARD_END_MS_PURE &&
         stockAge >= state.delayMs;
}

static inline bool r79FixedQuietRetrySafePure(
    const R79FixedQuietStatePure &state, uint32_t nowMs) {
  return r79FixedQuietSafeNowPure(state, nowMs);
}

static inline R79FixedQuietActionPure r79FixedQuietStepPure(
    R79FixedQuietStatePure &state, uint32_t nowMs, bool allowed) {
  if (!state.pending || !r79FixedTimeReachedPure(nowMs, state.dueMs))
    return R79_FIXED_QUIET_WAIT_PURE;

  // Consume before returning so every MUX2 cycle yields at most one action.
  state.pending = false;
  if (!allowed) return R79_FIXED_QUIET_DISALLOWED_SKIP_PURE;
  if (!r79FixedQuietSafeNowPure(state, nowMs))
    return R79_FIXED_QUIET_GUARD_SKIP_PURE;
  return R79_FIXED_QUIET_FIRE_PURE;
}

// r79 mode2 pure
enum R79ModePure : uint8_t {
  R79_MODE_1_PURE = 1,
  R79_MODE_2_PURE = 2
};

static constexpr uint8_t R79_MODE_DEFAULT_PURE = R79_MODE_1_PURE;
static constexpr uint16_t R79_MODE2_TX_WAIT_MS_PURE = 0u;
static constexpr uint16_t R79_MODE2_DELAY_DEFAULT_MS_PURE = 150u;
static constexpr uint16_t R79_MODE2_DELAY_MAX_MS_PURE = 2000u;

static inline uint8_t r79ModeSanitizePure(uint8_t mode) {
  return mode == R79_MODE_1_PURE || mode == R79_MODE_2_PURE
      ? mode : R79_MODE_DEFAULT_PURE;
}

static inline uint16_t r79Mode2DelaySanitizePure(uint16_t delayMs) {
  return delayMs <= R79_MODE2_DELAY_MAX_MS_PURE
      ? delayMs : R79_MODE2_DELAY_DEFAULT_MS_PURE;
}

// V14-style R79 MUX1 payload policy used by Mode 2. Bit18 is intentionally
// absent from this mask, so its stock value is preserved. No V13/V14 vehicle
// classification participates in the decision. The caller may explicitly select
// stock bit47 preservation for a supported Legacy V12/V13 HW3 profile.
static inline bool r79Mode2ApplyBitsPure(
    uint8_t data[8], bool preserveBit47 = false) {
  if (!data) return false;
  const uint8_t before2 = data[2];
  const uint8_t before5 = data[5];
  data[2] = (uint8_t)(data[2] & (uint8_t)~(1u << 3));  // bit19 = 0
  if (!preserveBit47)
    data[5] = (uint8_t)(data[5] | (uint8_t)(1u << 7)); // bit47 = 1
  return data[2] != before2 || data[5] != before5;
}

struct R79Mode2DelayedStatePure {
  bool pending;
  uint32_t dueMs;
};

struct R79Mode2ObserveResultPure {
  bool armed;
  bool cancelled;
};

enum R79Mode2DelayedActionPure : uint8_t {
  R79_MODE2_DELAY_WAIT_PURE = 0,
  R79_MODE2_DELAY_FIRE_PURE = 1,
  R79_MODE2_DELAY_DISALLOWED_PURE = 2
};

static inline bool r79Mode2TimeReachedPure(uint32_t nowMs, uint32_t dueMs) {
  return (int32_t)(nowMs - dueMs) >= 0;
}

static inline R79Mode2ObserveResultPure r79Mode2ObserveStockPure(
    R79Mode2DelayedStatePure &state, uint8_t mux, uint32_t nowMs,
    bool reinjectEnabled, uint16_t delayMs) {
  R79Mode2ObserveResultPure result = {};
  if (mux == 2u && reinjectEnabled) {
    state.pending = true;
    state.dueMs = nowMs + r79Mode2DelaySanitizePure(delayMs);
    result.armed = true;
    return result;
  }

  if (state.pending && (mux == 0u || mux == 1u || mux == 2u)) {
    state.pending = false;
    result.cancelled = true;
  }
  return result;
}

static inline R79Mode2DelayedActionPure r79Mode2DelayedStepPure(
    R79Mode2DelayedStatePure &state, uint32_t nowMs, bool allowed) {
  if (!state.pending || !r79Mode2TimeReachedPure(nowMs, state.dueMs))
    return R79_MODE2_DELAY_WAIT_PURE;
  state.pending = false;
  return allowed ? R79_MODE2_DELAY_FIRE_PURE
                 : R79_MODE2_DELAY_DISALLOWED_PURE;
}

// r79 ap gate pure
enum R79ApGateModePure : uint8_t {
  R79_AP_GATE_BLOCK_PURE = 0,
  R79_AP_GATE_DELAY_PURE = 1
};

enum R79ApGateReasonPure : uint8_t {
  R79_AP_GATE_BYPASS_PURE = 0,
  R79_AP_GATE_NON_AP_PURE = 1,
  R79_AP_GATE_BLOCKED_PURE = 2,
  R79_AP_GATE_WAITING_PURE = 3,
  R79_AP_GATE_READY_PURE = 4,
  R79_AP_GATE_UNKNOWN_PURE = 5
};

struct R79ApGateConfigPure {
  bool enabled;
  uint8_t mode;
  uint8_t delaySeconds;
  bool allowManualDriving = false;
};

struct R79ApGateSessionPure {
  bool active;
  uint32_t startedMs;
  bool released;
};

struct R79ApGateDecisionPure {
  bool allowed;
  uint8_t reason;
  uint32_t remainingMs;
};

static inline bool r79ApGateConfigValidPure(const R79ApGateConfigPure &config) {
  return config.mode <= R79_AP_GATE_DELAY_PURE &&
         config.delaySeconds >= 2u && config.delaySeconds <= 10u;
}

// Caller supplies validated, fresh DAS state (AP states 3..6 all map to true).
// Reset the session on configuration/recovery and observe invalid before accepting
// a new frame after a freshness gap. No timestamp value is used as a sentinel.
static inline void r79ApGateObservePure(
    R79ApGateSessionPure &session, const R79ApGateConfigPure &config,
    uint32_t nowMs, bool dasFreshValid, bool apActive) {
  if (!config.enabled || !r79ApGateConfigValidPure(config) ||
      !dasFreshValid || !apActive) {
    session = {};
    return;
  }
  if (!session.active) {
    session.active = true;
    session.startedMs = nowMs;
    session.released = false;
  }
  if (config.mode == R79_AP_GATE_DELAY_PURE &&
      (uint32_t)(nowMs - session.startedMs) >=
          (uint32_t)config.delaySeconds * 1000u) {
    session.released = true;
  }
}

// A supplementary permission only: the caller must AND it with legacy R79
// authorization. Snapshot evaluation never mutates the CAN-owned session.
static inline R79ApGateDecisionPure r79ApGateDecisionPure(
    const R79ApGateConfigPure &config, const R79ApGateSessionPure &session,
    uint32_t nowMs, bool dasFreshValid, bool apActive) {
  if (!config.enabled) return {true, R79_AP_GATE_BYPASS_PURE, 0u};
  if (!r79ApGateConfigValidPure(config) || !dasFreshValid)
    return {false, R79_AP_GATE_UNKNOWN_PURE, 0u};
  if (!apActive) return {true, R79_AP_GATE_NON_AP_PURE, 0u};
  if (config.mode == R79_AP_GATE_BLOCK_PURE)
    return {false, R79_AP_GATE_BLOCKED_PURE, 0u};
  if (!session.active) return {false, R79_AP_GATE_UNKNOWN_PURE, 0u};
  const uint32_t delayMs = (uint32_t)config.delaySeconds * 1000u;
  const uint32_t elapsedMs = (uint32_t)(nowMs - session.startedMs);
  if (session.released || elapsedMs >= delayMs)
    return {true, R79_AP_GATE_READY_PURE, 0u};
  return {false, R79_AP_GATE_WAITING_PURE, delayMs - elapsedMs};
}
