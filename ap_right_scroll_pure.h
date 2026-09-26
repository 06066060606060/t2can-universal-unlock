#pragma once

#include <stdint.h>

static constexpr uint16_t AP_RIGHT_SCROLL_MIN_INTERVAL_S_PURE = 1;
static constexpr uint16_t AP_RIGHT_SCROLL_MAX_INTERVAL_S_PURE = 600;
static constexpr uint16_t AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE = 30;
static constexpr uint8_t AP_RIGHT_SCROLL_MIN_WARNING_REPEAT_S_PURE = 1;
static constexpr uint8_t AP_RIGHT_SCROLL_MAX_WARNING_REPEAT_S_PURE = 5;
static constexpr uint8_t AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE = 2;
static constexpr uint8_t AP_RIGHT_SCROLL_MUX1_PURE = 1;
static constexpr uint8_t AP_RIGHT_SCROLL_UP_PURE = 0x01;
static constexpr uint8_t AP_RIGHT_SCROLL_DOWN_PURE = 0x3F;

enum ApRightScrollActionPure : uint8_t {
  AP_RIGHT_SCROLL_ACTION_NONE_PURE = 0,
  AP_RIGHT_SCROLL_ACTION_UP_PURE = 1,
  AP_RIGHT_SCROLL_ACTION_DOWN_PURE = 2,
};

struct ApRightScrollStatePure {
  bool gateWasOpen;
  bool downPending;
  bool downFromWarning;
  bool attemptedFromWarning;
  bool warningActive;
  bool warningDueArmed;
  uint32_t nextDueMs;
  uint32_t warningDueMs;
  uint32_t warningEpochSeen;
  uint32_t regularIntervalMs;
  uint32_t warningRepeatMs;
};

static inline bool apRightScrollIntervalValidPure(uint16_t seconds) {
  return seconds >= AP_RIGHT_SCROLL_MIN_INTERVAL_S_PURE &&
         seconds <= AP_RIGHT_SCROLL_MAX_INTERVAL_S_PURE;
}

static inline uint16_t apRightScrollIntervalSanitizePure(uint16_t seconds) {
  return apRightScrollIntervalValidPure(seconds)
      ? seconds : AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE;
}

static inline bool apRightScrollWarningRepeatValidPure(uint8_t seconds) {
  return seconds >= AP_RIGHT_SCROLL_MIN_WARNING_REPEAT_S_PURE &&
         seconds <= AP_RIGHT_SCROLL_MAX_WARNING_REPEAT_S_PURE;
}

static inline uint8_t apRightScrollWarningRepeatSanitizePure(uint8_t seconds) {
  return apRightScrollWarningRepeatValidPure(seconds)
      ? seconds : AP_RIGHT_SCROLL_DEFAULT_WARNING_REPEAT_S_PURE;
}

static inline bool apRightScrollDeadlineReachedPure(uint32_t nowMs,
                                                     uint32_t deadlineMs) {
  return (int32_t)(nowMs - deadlineMs) >= 0;
}

static inline uint8_t apRightScrollMuxPure(const uint8_t data[8]) {
  return data ? (uint8_t)(data[0] & 0x03u) : 0xFFu;
}

static inline uint8_t apRightScrollValuePure(const uint8_t data[8]) {
  return data ? (uint8_t)(data[3] & 0x3Fu) : 0u;
}

static inline void apRightScrollSetValuePure(uint8_t data[8], uint8_t value) {
  if (!data) return;
  data[3] = (uint8_t)((data[3] & 0xC0u) | (value & 0x3Fu));
}

static inline ApRightScrollActionPure apRightScrollStepPure(
    ApRightScrollStatePure &state,
    uint32_t nowMs,
    bool gateOpen,
    uint16_t regularIntervalSeconds,
    uint8_t warningRepeatSeconds,
    uint32_t warningEpoch,
    bool warningActive,
    bool isMux1,
    uint8_t physicalRightValue) {
  if (!gateOpen) {
    state = {};
    return AP_RIGHT_SCROLL_ACTION_NONE_PURE;
  }
  if (!isMux1) return AP_RIGHT_SCROLL_ACTION_NONE_PURE;

  const uint32_t intervalMs =
      (uint32_t)apRightScrollIntervalSanitizePure(regularIntervalSeconds) * 1000UL;
  const uint32_t warningMs =
      (uint32_t)apRightScrollWarningRepeatSanitizePure(warningRepeatSeconds) * 1000UL;
  if (!state.gateWasOpen) {
    state = {};
    state.gateWasOpen = true;
    state.regularIntervalMs = intervalMs;
    state.warningRepeatMs = warningMs;
    state.nextDueMs = nowMs + intervalMs;
    state.warningEpochSeen = warningEpoch;
    state.warningActive = warningActive;
    if (warningActive && warningEpoch != 0u) {
      state.warningDueArmed = true;
      state.warningDueMs = nowMs;
    }
  } else {
    state.regularIntervalMs = intervalMs;
    state.warningRepeatMs = warningMs;
  }

  if (warningEpoch != state.warningEpochSeen) {
    state.warningEpochSeen = warningEpoch;
    if (warningActive) {
      state.warningDueArmed = true;
      state.warningDueMs = nowMs;
    }
  }
  state.warningActive = warningActive;
  if (!warningActive && !state.downPending) state.warningDueArmed = false;

  if ((physicalRightValue & 0x3Fu) != 0u) {
    state.downPending = false;
    state.downFromWarning = false;
    state.attemptedFromWarning = false;
    state.nextDueMs = nowMs + intervalMs;
    state.warningDueArmed = warningActive;
    state.warningDueMs = nowMs + warningMs;
    return AP_RIGHT_SCROLL_ACTION_NONE_PURE;
  }
  if (state.downPending) {
    state.attemptedFromWarning = state.downFromWarning;
    return AP_RIGHT_SCROLL_ACTION_DOWN_PURE;
  }

  if (warningActive && state.warningDueArmed &&
      apRightScrollDeadlineReachedPure(nowMs, state.warningDueMs)) {
    state.attemptedFromWarning = true;
    return AP_RIGHT_SCROLL_ACTION_UP_PURE;
  }
  if (!apRightScrollDeadlineReachedPure(nowMs, state.nextDueMs))
    return AP_RIGHT_SCROLL_ACTION_NONE_PURE;
  state.attemptedFromWarning = false;
  return AP_RIGHT_SCROLL_ACTION_UP_PURE;
}

static inline void apRightScrollTxResultPure(
    ApRightScrollStatePure &state,
    uint32_t nowMs,
    ApRightScrollActionPure attempted,
    bool txOk) {
  if (attempted == AP_RIGHT_SCROLL_ACTION_NONE_PURE || !state.gateWasOpen)
    return;

  if (!txOk) {
    state.downPending = false;
    state.downFromWarning = false;
    state.attemptedFromWarning = false;
    state.nextDueMs = nowMs + state.regularIntervalMs;
    state.warningDueArmed = state.warningActive;
    state.warningDueMs = nowMs + state.warningRepeatMs;
    return;
  }

  if (attempted == AP_RIGHT_SCROLL_ACTION_UP_PURE) {
    state.downPending = true;
    state.downFromWarning = state.attemptedFromWarning;
    state.attemptedFromWarning = false;
    return;
  }

  const bool completedWarningPair = state.downFromWarning;
  state.downPending = false;
  state.downFromWarning = false;
  state.attemptedFromWarning = false;
  state.nextDueMs = nowMs + state.regularIntervalMs;
  if (completedWarningPair) {
    state.warningDueArmed = state.warningActive;
    state.warningDueMs = nowMs + state.warningRepeatMs;
  }
}
