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
static constexpr uint16_t TSL9_RIGHT_SCROLL_STEP_MS_PURE = 100;

// V8.2 DND scroll assistance treats the documented Hands-On warning ranges as
// one warning epoch. Keep this separate from Mode H's narrower visual-rescue
// predicate so enabling TSL9 Scroll Assist cannot change torque-mode behavior.
static inline bool tsl9ScrollWarningStatePure(uint8_t handsOnState) {
  return (handsOnState >= 3u && handsOnState <= 6u) ||
         (handsOnState >= 9u && handsOnState <= 10u);
}

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

enum Tsl9RightScrollActionPure : uint8_t {
  TSL9_RIGHT_SCROLL_NONE_PURE = 0,
  TSL9_RIGHT_SCROLL_POSITIVE_PURE = 1,
  TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE = 2,
  TSL9_RIGHT_SCROLL_NEGATIVE_PURE = 3,
  TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE = 4,
};

struct Tsl9RightScrollStatePure {
  bool gateWasOpen;
  bool sequenceActive;
  bool sequenceFromWarning;
  bool warningActive;
  bool warningDueArmed;
  uint8_t stepIndex;
  uint32_t stepDueMs;
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
  return seconds == 0u ||
         (seconds >= AP_RIGHT_SCROLL_MIN_WARNING_REPEAT_S_PURE &&
          seconds <= AP_RIGHT_SCROLL_MAX_WARNING_REPEAT_S_PURE);
}

static inline uint8_t apRightScrollWarningRepeatSanitizePure(uint8_t seconds) {
  if (seconds == 0u) return 0u;
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

static inline bool apRightScrollGateOpenPure(
    bool supported,
    bool enabled,
    bool apActive,
    bool txReady,
    bool administrativeHold) {
  return supported && enabled && apActive && txReady && !administrativeHold;
}

static inline Tsl9RightScrollActionPure tsl9RightScrollActionPure(
    uint8_t stepIndex) {
  switch (stepIndex) {
    case 0u: return TSL9_RIGHT_SCROLL_POSITIVE_PURE;
    case 1u: return TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE;
    case 2u: return TSL9_RIGHT_SCROLL_NEGATIVE_PURE;
    case 3u: return TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE;
    default: return TSL9_RIGHT_SCROLL_NONE_PURE;
  }
}

static inline uint8_t tsl9RightScrollValuePure(
    Tsl9RightScrollActionPure action) {
  if (action == TSL9_RIGHT_SCROLL_POSITIVE_PURE) return 0x01u;
  if (action == TSL9_RIGHT_SCROLL_NEGATIVE_PURE) return 0x3Fu;
  return 0x00u;
}

static inline void tsl9RightScrollApplyActionPure(
    uint8_t data[8],
    Tsl9RightScrollActionPure action) {
  if (!data || action == TSL9_RIGHT_SCROLL_NONE_PURE) return;
  const uint8_t value = tsl9RightScrollValuePure(action);
  apRightScrollSetValuePure(data, value);
  if (value == 0u) data[6] |= 0x10u;
  else data[6] &= (uint8_t)~0x10u;
}

static inline Tsl9RightScrollActionPure tsl9RightScrollStepPure(
    Tsl9RightScrollStatePure &state,
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
    return TSL9_RIGHT_SCROLL_NONE_PURE;
  }
  if (!isMux1) return TSL9_RIGHT_SCROLL_NONE_PURE;

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
  if (!warningActive && !state.sequenceActive)
    state.warningDueArmed = false;

  if ((physicalRightValue & 0x3Fu) != 0u) {
    state.sequenceActive = false;
    state.sequenceFromWarning = false;
    state.stepIndex = 0u;
    state.stepDueMs = 0u;
    state.nextDueMs = nowMs + intervalMs;
    state.warningDueArmed = warningActive && warningMs != 0u;
    state.warningDueMs = nowMs + warningMs;
    return TSL9_RIGHT_SCROLL_NONE_PURE;
  }

  if (state.sequenceActive) {
    if (!apRightScrollDeadlineReachedPure(nowMs, state.stepDueMs))
      return TSL9_RIGHT_SCROLL_NONE_PURE;
    return tsl9RightScrollActionPure(state.stepIndex);
  }

  if (warningActive && state.warningDueArmed &&
      apRightScrollDeadlineReachedPure(nowMs, state.warningDueMs)) {
    state.sequenceActive = true;
    state.sequenceFromWarning = true;
    state.stepIndex = 0u;
    state.stepDueMs = nowMs;
    return TSL9_RIGHT_SCROLL_POSITIVE_PURE;
  }
  if (!apRightScrollDeadlineReachedPure(nowMs, state.nextDueMs))
    return TSL9_RIGHT_SCROLL_NONE_PURE;
  state.sequenceActive = true;
  state.sequenceFromWarning = false;
  state.stepIndex = 0u;
  state.stepDueMs = nowMs;
  return TSL9_RIGHT_SCROLL_POSITIVE_PURE;
}

static inline void tsl9RightScrollTxResultPure(
    Tsl9RightScrollStatePure &state,
    uint32_t nowMs,
    Tsl9RightScrollActionPure attempted,
    bool txOk) {
  if (attempted == TSL9_RIGHT_SCROLL_NONE_PURE ||
      !state.gateWasOpen || !state.sequenceActive) return;

  if (!txOk) {
    state.sequenceActive = false;
    state.sequenceFromWarning = false;
    state.stepIndex = 0u;
    state.stepDueMs = 0u;
    state.nextDueMs = nowMs + state.regularIntervalMs;
    state.warningDueArmed = state.warningActive && state.warningRepeatMs != 0u;
    state.warningDueMs = nowMs + state.warningRepeatMs;
    return;
  }

  if (state.stepIndex < 3u) {
    state.stepIndex++;
    state.stepDueMs = nowMs + TSL9_RIGHT_SCROLL_STEP_MS_PURE;
    return;
  }

  const bool completedWarningSequence = state.sequenceFromWarning;
  state.sequenceActive = false;
  state.sequenceFromWarning = false;
  state.stepIndex = 0u;
  state.stepDueMs = 0u;
  state.nextDueMs = nowMs + state.regularIntervalMs;
  if (completedWarningSequence) {
    state.warningDueArmed = state.warningActive && state.warningRepeatMs != 0u;
    state.warningDueMs = nowMs + state.warningRepeatMs;
  }
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
    state.warningDueArmed = warningActive && warningMs != 0u;
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
    state.warningDueArmed = state.warningActive && state.warningRepeatMs != 0u;
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
    state.warningDueArmed = state.warningActive && state.warningRepeatMs != 0u;
    state.warningDueMs = nowMs + state.warningRepeatMs;
  }
}
