#pragma once

#include <stdint.h>

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
