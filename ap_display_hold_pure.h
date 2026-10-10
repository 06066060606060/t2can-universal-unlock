#pragma once
#include <stdint.h>

static constexpr uint32_t AP_DISPLAY_HARD_INIT_GRACE_MS_PURE = 10000u;

struct ApDisplayHoldPure {
  bool lastValid;
  uint8_t lastState;
  uint32_t hardInitHoldUntilMs;
};

struct ApDisplayValuePure {
  bool valid;
  uint8_t state;
};

static inline void apDisplayObserveValidPure(ApDisplayHoldPure &hold,
                                             uint8_t state) {
  hold.lastValid = true;
  hold.lastState = state;
  hold.hardInitHoldUntilMs = 0u;
}

static inline void apDisplayBeginHardInitPure(ApDisplayHoldPure &hold,
                                               uint32_t nowMs) {
  if (!hold.lastValid) return;
  hold.hardInitHoldUntilMs = nowMs + AP_DISPLAY_HARD_INIT_GRACE_MS_PURE;
  if (hold.hardInitHoldUntilMs == 0u) hold.hardInitHoldUntilMs = 1u;
}

static inline bool apDisplayDeadlinePendingPure(uint32_t nowMs,
                                                uint32_t deadlineMs) {
  return deadlineMs != 0u && (int32_t)(deadlineMs - nowMs) >= 0;
}

static inline ApDisplayValuePure apDisplayResolvePure(
    const ApDisplayHoldPure &hold, bool liveValid, uint8_t liveState,
    bool hardInitBusy, uint32_t nowMs) {
  if (liveValid) return {true, liveState};
  if (hold.lastValid &&
      (hardInitBusy ||
       apDisplayDeadlinePendingPure(nowMs, hold.hardInitHoldUntilMs))) {
    return {true, hold.lastState};
  }
  return {false, 0xFFu};
}
