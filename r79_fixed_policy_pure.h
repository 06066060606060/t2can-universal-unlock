#pragma once

#include <stdint.h>

enum R79Bit18PolicyPure : uint8_t {
  R79_BIT18_STOCK_PURE = 0,
  R79_BIT18_FORCE_0_PURE = 1
};

static constexpr uint8_t R79_BIT18_DEFAULT_PURE = R79_BIT18_FORCE_0_PURE;
static constexpr uint16_t R79_FIXED_FAST_WAIT_MS_PURE = 2u;
static constexpr uint16_t R79_FIXED_QUIET_DELAY_MS_PURE = 150u;
static constexpr uint16_t R79_FIXED_QUIET_HARD_END_MS_PURE = 340u;

static inline uint8_t r79Bit18PolicySanitizePure(uint8_t policy) {
  return policy == R79_BIT18_STOCK_PURE || policy == R79_BIT18_FORCE_0_PURE
      ? policy : R79_BIT18_DEFAULT_PURE;
}

static inline void r79FixedApplyBitsPure(uint8_t data[8], uint8_t policy) {
  if (!data) return;
  if (r79Bit18PolicySanitizePure(policy) == R79_BIT18_FORCE_0_PURE)
    data[2] = (uint8_t)(data[2] & (uint8_t)~(1u << 2));
  data[2] = (uint8_t)(data[2] & (uint8_t)~(1u << 3));
  data[5] = (uint8_t)(data[5] | (uint8_t)(1u << 7));
}

struct R79FixedQuietStatePure {
  bool pending;
  bool stockSeen;
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

static inline R79FixedQuietObserveResultPure r79FixedQuietObserveStockPure(
    R79FixedQuietStatePure &state, uint8_t mux, uint32_t nowMs) {
  R79FixedQuietObserveResultPure out = {};
  state.stockSeen = true;
  state.lastStockMs = nowMs;

  if (mux == 2u) {
    state.pending = true;
    state.cycleAnchorMs = nowMs;
    state.dueMs = nowMs + R79_FIXED_QUIET_DELAY_MS_PURE;
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

static inline bool r79FixedQuietSafeNowPure(
    const R79FixedQuietStatePure &state, uint32_t nowMs) {
  if (!state.stockSeen) return false;
  const uint32_t cycleAge = (uint32_t)(nowMs - state.cycleAnchorMs);
  const uint32_t stockAge = (uint32_t)(nowMs - state.lastStockMs);
  return cycleAge >= R79_FIXED_QUIET_DELAY_MS_PURE &&
         cycleAge <= R79_FIXED_QUIET_HARD_END_MS_PURE &&
         stockAge >= R79_FIXED_QUIET_DELAY_MS_PURE;
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
