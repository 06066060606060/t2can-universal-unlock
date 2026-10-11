// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include "../tsl9_input_scheduler_pure.h"

// Frozen historical engine, used only by the legacy regression cases below.
namespace legacy_scroll {

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


// Coverage origin: test_v36f1_ap_right_scroll_pure.cpp
namespace case_ap_right_scroll_pure {
int run() {
  assert(!apRightScrollIntervalValidPure(0));
  assert(apRightScrollIntervalValidPure(1));
  assert(apRightScrollIntervalValidPure(600));
  assert(!apRightScrollIntervalValidPure(601));
  assert(apRightScrollIntervalSanitizePure(0) == AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE);
  assert(apRightScrollIntervalSanitizePure(42) == 42);

  uint8_t frame[8] = {0x29, 0x55, 0xAA, 0xC0, 0x11, 0x22, 0x31, 0x15};
  assert(apRightScrollMuxPure(frame) == 1);
  assert(apRightScrollValuePure(frame) == 0);
  apRightScrollSetValuePure(frame, AP_RIGHT_SCROLL_UP_PURE);
  assert(frame[3] == 0xC1);
  assert(frame[0] == 0x29 && frame[2] == 0xAA && frame[7] == 0x15);
  apRightScrollSetValuePure(frame, AP_RIGHT_SCROLL_DOWN_PURE);
  assert(frame[3] == 0xFF);

  ApRightScrollStatePure state = {};
  assert(apRightScrollStepPure(state, 1000, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.gateWasOpen);
  assert(state.nextDueMs == 11000);
  assert(apRightScrollStepPure(state, 10999, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto action = apRightScrollStepPure(state, 11000, true, 10, 2, 0, false, true, 0);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(state, 11000, action, true);
  action = apRightScrollStepPure(state, 11100, true, 10, 2, 0, false, true, 0);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(state, 11100, action, true);
  assert(state.nextDueMs == 21100);

  // A real right-scroll input wins and restarts the configured interval.
  assert(apRightScrollStepPure(state, 21100, true, 10, 2, 0, false, true, 0x01) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.nextDueMs == 31100);

  // Closing and reopening the gate never produces an immediate pulse.
  assert(apRightScrollStepPure(state, 22000, false, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!state.gateWasOpen && state.nextDueMs == 0);
  assert(apRightScrollStepPure(state, 23000, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.nextDueMs == 33000);

  // millis() rollover uses signed-deadline comparison.
  state = {};
  assert(apRightScrollStepPure(state, 0xFFFFFF00u, true, 1, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  const uint32_t wrappedDue = state.nextDueMs;
  assert(apRightScrollStepPure(state, wrappedDue, true, 1, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  return 0;
  return 0;
}
} // namespace case_ap_right_scroll_pure

// Coverage origin: test_v37_ap_right_scroll_visual_pure.cpp
namespace case_ap_right_scroll_visual_pure {
static ApRightScrollActionPure step(
    ApRightScrollStatePure &s, uint32_t now, bool gate,
    uint16_t regular, uint8_t repeat, uint32_t epoch,
    bool warning, uint8_t physical = 0u) {
  return apRightScrollStepPure(
      s, now, gate, regular, repeat, epoch, warning, true, physical);
}

int run() {
  assert(apRightScrollWarningRepeatValidPure(0u));
  assert(apRightScrollWarningRepeatValidPure(1u));
  assert(apRightScrollWarningRepeatValidPure(5u));
  assert(!apRightScrollWarningRepeatValidPure(6u));
  assert(apRightScrollWarningRepeatSanitizePure(0u) == 0u);

  // A zero repeat interval means warning-edge-only: fire immediately once,
  // finish the generated pair, and do not repeat while the warning remains.
  ApRightScrollStatePure oneShot = {};
  assert(step(oneShot, 1000u, true, 30u, 0u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto oneShotAction = step(oneShot, 1100u, true, 30u, 0u, 1u, true);
  assert(oneShotAction == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(oneShot, 1100u, oneShotAction, true);
  oneShotAction = step(oneShot, 1120u, true, 30u, 0u, 1u, true);
  assert(oneShotAction == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(oneShot, 1120u, oneShotAction, true);
  assert(step(oneShot, 10000u, true, 30u, 0u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);

  // The selectable four-step torque waveform has the same warning-edge-only
  // policy when repeat is zero.
  Tsl9RightScrollStatePure fourStepOneShot = {};
  assert(tsl9RightScrollStepPure(fourStepOneShot, 2000u, true, 30u, 0u,
                                 0u, false, true, 0u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  for (uint8_t index = 0u; index < 4u; ++index) {
    const uint32_t now = 2100u + (uint32_t)index * 100u;
    const Tsl9RightScrollActionPure fourStepAction =
        tsl9RightScrollStepPure(fourStepOneShot, now, true, 30u, 0u,
                                1u, true, true, 0u);
    assert(fourStepAction == tsl9RightScrollActionPure(index));
    tsl9RightScrollTxResultPure(
        fourStepOneShot, now, fourStepAction, true);
  }
  assert(tsl9RightScrollStepPure(fourStepOneShot, 20000u, true, 30u, 0u,
                                 1u, true, true, 0u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);

  // A visual-warning edge fires immediately, completes a full pair, and then
  // repeats at the configured interval while the same warning remains active.
  ApRightScrollStatePure s = {};
  assert(step(s, 1000u, true, 30u, 2u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto action = step(s, 1100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(s, 1100u, action, true);
  action = step(s, 1120u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(s, 1120u, action, true);
  assert(step(s, 3119u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(s, 3120u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);

  // Clearing a warning stops repeats. A later epoch fires immediately again.
  apRightScrollTxResultPure(s, 3120u, action, true);
  action = step(s, 3140u, true, 30u, 2u, 1u, false);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(s, 3140u, action, true);
  assert(step(s, 6000u, true, 30u, 2u, 1u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(s, 6100u, true, 30u, 2u, 2u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);

  // A physical scroll input wins both before UP and while DOWN is pending.
  ApRightScrollStatePure physical = {};
  assert(step(physical, 100u, true, 30u, 2u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(step(physical, 200u, true, 30u, 2u, 1u, true, 1u) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!physical.downPending);
  assert(step(physical, 2199u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(physical, 2200u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(physical, 2200u, action, true);
  assert(physical.downPending);
  assert(step(physical, 2210u, true, 30u, 2u, 1u, true, 2u) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!physical.downPending);

  // A failed generated action abandons the whole pair and postpones retries.
  ApRightScrollStatePure failed = {};
  (void)step(failed, 1000u, true, 30u, 2u, 0u, false);
  action = step(failed, 1100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(failed, 1100u, action, false);
  assert(!failed.downPending);
  assert(step(failed, 3099u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(failed, 3100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(failed, 3100u, action, true);
  action = step(failed, 3110u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(failed, 3110u, action, false);
  assert(!failed.downPending);

  // Warning takes priority when its edge collides with a regular deadline.
  ApRightScrollStatePure collision = {};
  (void)step(collision, 1000u, true, 1u, 2u, 0u, false);
  action = step(collision, 2000u, true, 1u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  assert(collision.attemptedFromWarning);

  // Gate close fully resets generated state.
  apRightScrollTxResultPure(collision, 2000u, action, true);
  assert(step(collision, 2010u, false, 1u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!collision.gateWasOpen && !collision.downPending);

  // Warning repeat deadlines remain correct across uint32_t rollover.
  ApRightScrollStatePure rollover = {};
  (void)step(rollover, 0xFFFFFF00u, true, 30u, 1u, 0u, false);
  action = step(rollover, 0xFFFFFF10u, true, 30u, 1u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(rollover, 0xFFFFFF10u, action, true);
  action = step(rollover, 0xFFFFFF20u, true, 30u, 1u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(rollover, 0xFFFFFF20u, action, true);
  const uint32_t rolloverDue = 0x00000308u;
  assert(step(rollover, rolloverDue - 1u, true, 30u, 1u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(step(rollover, rolloverDue, true, 30u, 1u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_UP_PURE);

  return 0;
  return 0;
}
} // namespace case_ap_right_scroll_visual_pure

// Coverage origin: test_tsl9_right_scroll_sequence_pure.cpp
namespace case_tsl9_right_scroll_sequence_pure {
static Tsl9RightScrollActionPure step(
    Tsl9RightScrollStatePure &state, uint32_t nowMs, bool gateOpen,
    uint32_t warningEpoch, bool warningActive, uint8_t physicalValue = 0u) {
  return tsl9RightScrollStepPure(
      state, nowMs, gateOpen, 30u, 2u, warningEpoch, warningActive,
      true, physicalValue);
}

int run() {
  // The standalone feature depends on CAN support, its own switch, AP, and TX
  // readiness. It must not require a NAG mode or NAG enabled flag.
  assert(apRightScrollGateOpenPure(true, true, true, true, false));
  assert(!apRightScrollGateOpenPure(true, true, false, true, false));
  assert(!apRightScrollGateOpenPure(true, true, true, false, false));
  assert(!apRightScrollGateOpenPure(true, true, true, true, true));

  // A TSL9 warning emits the exact V8.2 right-speed waveform with 100 ms
  // between accepted stock-frame-derived transmissions.
  Tsl9RightScrollStatePure state = {};
  auto action = step(state, 1000u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1000u, action, true);
  assert(step(state, 1099u, true, 1u, true) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  action = step(state, 1100u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1100u, action, true);
  action = step(state, 1200u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_NEGATIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1200u, action, true);
  action = step(state, 1300u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1300u, action, true);
  assert(!state.sequenceActive);

  // Payload behavior matches V8.2: Byte3 carries signed six-bit ticks and
  // Byte6 bit4 marks the two centered stages.
  const Tsl9RightScrollActionPure actions[4] = {
      TSL9_RIGHT_SCROLL_POSITIVE_PURE,
      TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE,
      TSL9_RIGHT_SCROLL_NEGATIVE_PURE,
      TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE,
  };
  const uint8_t expectedValues[4] = {0x01u, 0x00u, 0x3Fu, 0x00u};
  const bool expectedCenters[4] = {false, true, false, true};
  for (uint8_t i = 0u; i < 4u; ++i) {
    uint8_t payload[8] = {0x01u, 0u, 0u, 0xC0u, 0u, 0u, 0xA5u, 0u};
    tsl9RightScrollApplyActionPure(payload, actions[i]);
    assert((payload[3] & 0x3Fu) == expectedValues[i]);
    assert(((payload[6] & 0x10u) != 0u) == expectedCenters[i]);
  }

  // Real driver input cancels a generated sequence and postpones assistance.
  Tsl9RightScrollStatePure physical = {};
  action = step(physical, 2000u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(physical, 2000u, action, true);
  assert(physical.sequenceActive);
  assert(step(physical, 2050u, true, 1u, true, 1u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  assert(!physical.sequenceActive);

  return 0;
  return 0;
}
} // namespace case_tsl9_right_scroll_sequence_pure

// Coverage origin: test_tsl9_scroll_warning_pure.cpp
namespace case_tsl9_scroll_warning_pure {
int run() {
  assert(!tsl9ScrollWarningStatePure(2u));
  assert(tsl9ScrollWarningStatePure(3u));
  assert(tsl9ScrollWarningStatePure(4u));
  assert(tsl9ScrollWarningStatePure(5u));
  assert(tsl9ScrollWarningStatePure(6u));
  assert(!tsl9ScrollWarningStatePure(7u));
  assert(!tsl9ScrollWarningStatePure(8u));
  assert(tsl9ScrollWarningStatePure(9u));
  assert(tsl9ScrollWarningStatePure(10u));
  assert(!tsl9ScrollWarningStatePure(11u));
  return 0;
  return 0;
}
} // namespace case_tsl9_scroll_warning_pure

} // namespace legacy_scroll

int main() {
  (void)legacy_scroll::AP_RIGHT_SCROLL_MUX1_PURE;
  // Current production interval behavior remains equivalent across uint16_t.
  for (unsigned value = 0; value <= 65535u; ++value) {
    assert(apRightScrollIntervalValidPure(value) == legacy_scroll::apRightScrollIntervalValidPure(value));
    assert(apRightScrollIntervalSanitizePure(value) == legacy_scroll::apRightScrollIntervalSanitizePure(value));
  }
  if (legacy_scroll::case_ap_right_scroll_pure::run()) return 1;
  if (legacy_scroll::case_ap_right_scroll_visual_pure::run()) return 1;
  if (legacy_scroll::case_tsl9_right_scroll_sequence_pure::run()) return 1;
  if (legacy_scroll::case_tsl9_scroll_warning_pure::run()) return 1;
  return 0;
}
