#pragma once

#include <stdint.h>

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
