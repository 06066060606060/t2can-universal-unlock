#pragma once
#include <stdint.h>

static constexpr uint8_t BLINKA_NOA_STABILIZE_MIN_S_PURE = 1u;
static constexpr uint8_t BLINKA_NOA_STABILIZE_MAX_S_PURE = 20u;
static constexpr uint8_t BLINKA_NOA_STABILIZE_DEFAULT_S_PURE = 10u;
static constexpr uint32_t BLINKA_NOA_EXIT_CONFIRM_MS_PURE = 2000u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_MIN_S_PURE = 10u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_MAX_S_PURE = 100u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE = 20u;

enum AutoBlinkerCancelActionPure : uint8_t {
  AUTO_BLINKER_CANCEL_REJECTED_PURE = 0,
  AUTO_BLINKER_CANCEL_START_PAUSE_PURE = 1,
  AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE = 2
};

enum AutoBlinkerNoaPhasePure : uint8_t {
  AUTO_BLINKER_NOA_INACTIVE_PURE = 0,
  AUTO_BLINKER_NOA_STABILIZING_PURE = 1,
  AUTO_BLINKER_NOA_READY_PURE = 2,
  AUTO_BLINKER_NOA_EXIT_WAIT_PURE = 3
};

// NOA session entry/exit and stabilization are intentionally independent of
// the manual cancel pause. A session becomes READY once, remains READY across
// sub-two-second raw-NOA dropouts, and is re-armed only after a confirmed exit.
struct AutoBlinkerNoaSessionStatePure {
  bool sessionActive;
  uint32_t enteredAtMs;
  uint32_t readyAtMs;
  bool exitPending;
  uint32_t exitStartedAtMs;
  uint32_t exitConfirmAtMs;
};

struct AutoBlinkerCancelPauseStatePure {
  bool active;
  uint32_t untilMs;
};

static inline uint8_t autoBlinkerNoaStabilizationSecondsSanitizePure(
    uint8_t seconds) {
  return seconds >= BLINKA_NOA_STABILIZE_MIN_S_PURE &&
                 seconds <= BLINKA_NOA_STABILIZE_MAX_S_PURE
             ? seconds
             : BLINKA_NOA_STABILIZE_DEFAULT_S_PURE;
}

static inline uint8_t autoBlinkerCancelPauseSecondsSanitizePure(
    uint8_t seconds) {
  return seconds >= BLINKA_CANCEL_PAUSE_MIN_S_PURE &&
                 seconds <= BLINKA_CANCEL_PAUSE_MAX_S_PURE
             ? seconds
             : BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE;
}

static inline bool autoBlinkerDeadlineReachedPure(uint32_t nowMs,
                                                  uint32_t deadlineMs) {
  return (int32_t)(nowMs - deadlineMs) >= 0;
}

static inline void autoBlinkerNoaSessionResetPure(
    AutoBlinkerNoaSessionStatePure &state) {
  state = {};
}

static inline void autoBlinkerNoaSessionStartPure(
    AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs,
    uint8_t stabilizationSeconds) {
  const uint8_t seconds =
      autoBlinkerNoaStabilizationSecondsSanitizePure(stabilizationSeconds);
  state = {};
  state.sessionActive = true;
  state.enteredAtMs = nowMs;
  state.readyAtMs = nowMs + (uint32_t)seconds * 1000u;
}

static inline AutoBlinkerNoaPhasePure autoBlinkerNoaPhasePure(
    const AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs) {
  if (!state.sessionActive) return AUTO_BLINKER_NOA_INACTIVE_PURE;
  if (state.exitPending) {
    return autoBlinkerDeadlineReachedPure(nowMs, state.exitConfirmAtMs)
               ? AUTO_BLINKER_NOA_INACTIVE_PURE
               : AUTO_BLINKER_NOA_EXIT_WAIT_PURE;
  }
  return autoBlinkerDeadlineReachedPure(nowMs, state.readyAtMs)
             ? AUTO_BLINKER_NOA_READY_PURE
             : AUTO_BLINKER_NOA_STABILIZING_PURE;
}

static inline void autoBlinkerObserveNoaPure(
    AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs,
    bool stateValid, bool noaActive, uint8_t stabilizationSeconds) {
  if (!stateValid) {
    autoBlinkerNoaSessionResetPure(state);
    return;
  }

  if (noaActive) {
    if (!state.sessionActive) {
      autoBlinkerNoaSessionStartPure(state, nowMs, stabilizationSeconds);
      return;
    }
    if (!state.exitPending) return;
    if (autoBlinkerDeadlineReachedPure(nowMs, state.exitConfirmAtMs)) {
      autoBlinkerNoaSessionStartPure(state, nowMs, stabilizationSeconds);
      return;
    }
    state.exitPending = false;
    state.exitStartedAtMs = 0;
    state.exitConfirmAtMs = 0;
    return;
  }

  if (!state.sessionActive) return;
  if (!state.exitPending) {
    state.exitPending = true;
    state.exitStartedAtMs = nowMs;
    state.exitConfirmAtMs = nowMs + BLINKA_NOA_EXIT_CONFIRM_MS_PURE;
    return;
  }
  if (autoBlinkerDeadlineReachedPure(nowMs, state.exitConfirmAtMs))
    autoBlinkerNoaSessionResetPure(state);
}

static inline bool autoBlinkerNoaReadyPure(
    const AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs) {
  return autoBlinkerNoaPhasePure(state, nowMs) ==
         AUTO_BLINKER_NOA_READY_PURE;
}

static inline uint32_t autoBlinkerNoaRemainingMsPure(
    const AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs) {
  if (autoBlinkerNoaPhasePure(state, nowMs) !=
      AUTO_BLINKER_NOA_STABILIZING_PURE)
    return 0;
  return (uint32_t)(state.readyAtMs - nowMs);
}

static inline uint32_t autoBlinkerNoaExitRemainingMsPure(
    const AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs) {
  if (autoBlinkerNoaPhasePure(state, nowMs) !=
      AUTO_BLINKER_NOA_EXIT_WAIT_PURE)
    return 0;
  return (uint32_t)(state.exitConfirmAtMs - nowMs);
}

// Returns true only when an active STABILIZING session received a new
// dashboard value and its deadline was replaced from nowMs. READY sessions
// stay open until a confirmed NOA exit; inactive sessions use the saved value
// on their next entry.
static inline bool autoBlinkerNoaReconfigurePure(
    AutoBlinkerNoaSessionStatePure &state, uint32_t nowMs,
    uint8_t stabilizationSeconds) {
  if (!state.sessionActive) return false;
  if (state.exitPending &&
      autoBlinkerDeadlineReachedPure(nowMs, state.exitConfirmAtMs)) {
    autoBlinkerNoaSessionResetPure(state);
    return false;
  }
  if (autoBlinkerDeadlineReachedPure(nowMs, state.readyAtMs)) return false;
  const uint8_t seconds =
      autoBlinkerNoaStabilizationSecondsSanitizePure(stabilizationSeconds);
  state.enteredAtMs = nowMs;
  state.readyAtMs = nowMs + (uint32_t)seconds * 1000u;
  return true;
}

static inline bool autoBlinkerPauseActivePure(
    AutoBlinkerCancelPauseStatePure &state, uint32_t nowMs) {
  if (!state.active) return false;
  if (!autoBlinkerDeadlineReachedPure(nowMs, state.untilMs))
    return true;
  state.active = false;
  state.untilMs = 0;
  return false;
}

static inline uint32_t autoBlinkerPauseRemainingMsPure(
    AutoBlinkerCancelPauseStatePure &state, uint32_t nowMs) {
  if (!autoBlinkerPauseActivePure(state, nowMs)) return 0;
  return (uint32_t)(state.untilMs - nowMs);
}

static inline AutoBlinkerCancelActionPure autoBlinkerCancelTogglePure(
    AutoBlinkerCancelPauseStatePure &state, uint32_t nowMs,
    bool cancelEligible, uint8_t pauseSeconds) {
  if (autoBlinkerPauseActivePure(state, nowMs)) {
    state.active = false;
    state.untilMs = 0;
    return AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE;
  }
  if (!cancelEligible) return AUTO_BLINKER_CANCEL_REJECTED_PURE;
  const uint8_t seconds =
      autoBlinkerCancelPauseSecondsSanitizePure(pauseSeconds);
  state.active = true;
  state.untilMs = nowMs + (uint32_t)seconds * 1000u;
  return AUTO_BLINKER_CANCEL_START_PAUSE_PURE;
}

static inline void autoBlinkerCancelPauseResetPure(
    AutoBlinkerCancelPauseStatePure &state) {
  state = {};
}

static constexpr uint8_t SCCM249_CKSUM_CTR[16] = {
  0x9B, 0xE8, 0x2A, 0xD3, 0xD3, 0x83, 0x4C, 0x5E,
  0x3F, 0x5E, 0xE2, 0x28, 0x3A, 0x13, 0xAF, 0xCE
};

static inline uint8_t sccm249Crc8Pure(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x2F) : (uint8_t)(crc << 1);
  }
  return crc;
}

static inline uint8_t leftStalkChecksumPure(const uint8_t frame[4], uint8_t counter) {
  const uint8_t crcInput[4] = {(uint8_t)(frame[1] & 0xF0), frame[2], frame[3], 0x00};
  return (uint8_t)(sccm249Crc8Pure(crcInput, 4) ^ SCCM249_CKSUM_CTR[counter & 0x0F]);
}


static inline uint8_t vcleftMuxPure(const uint8_t data[8]) { return data[0] & 0x03u; }
static inline uint8_t vcleftLeftButtonPure(const uint8_t data[8]) { return (data[3] >> 6) & 0x03u; }
static inline uint8_t vcleftRightButtonPure(const uint8_t data[8]) { return (data[5] >> 4) & 0x03u; }
static inline void vcleftSetLeftButtonPure(uint8_t data[8], uint8_t state) {
  data[3] = (uint8_t)((data[3] & 0x3Fu) | ((state & 0x03u) << 6));
}
static inline void vcleftSetRightButtonPure(uint8_t data[8], uint8_t state) {
  data[5] = (uint8_t)((data[5] & 0xCFu) | ((state & 0x03u) << 4));
}
static inline bool doorOpenButtonPressedPure(const uint8_t *data, uint8_t dlc) {
  return data && dlc >= 4 && ((data[3] & 0x80u) != 0);
}

// v3.2 hotfix: common Advanced EAP lane-change eligibility helpers.
// reqDir: 1=LEFT, 2=RIGHT. ALC state 4 (EXITING_HIGHWAY) is accepted only
// when fresh map context confirms an active route and a direction-matching off-ramp.
static inline bool autoBlinkerAlcAllowsDirectionPure(
    uint8_t reqDir, uint8_t alcState, bool roadContextFresh, bool navRouteActive,
    bool leftOffRamp, bool rightOffRamp) {
  if (reqDir == 1 && (alcState == 6 || alcState == 8)) return true;
  if (reqDir == 2 && (alcState == 7 || alcState == 8)) return true;
  if (alcState != 4 || !roadContextFresh || !navRouteActive) return false;
  if (reqDir == 1) return leftOffRamp;
  if (reqDir == 2) return rightOffRamp;
  return false;
}

// v3.4b3 request-session model. The planner request is latched independently
// from temporary ALC eligibility. Once the delay expires, a blocked lane does
// not cancel the session; the caller keeps the request pending and retries at a
// bounded cadence until the lane opens or the request session really ends.
struct AutoBlinkerSessionDecisionPure {
  bool cancel;
  bool fire;
  bool retry;
};

static inline AutoBlinkerSessionDecisionPure autoBlinkerSessionDecisionPure(
    bool noaGateOpen,
    uint8_t pendingDir,
    uint8_t currentReqDir,
    bool requestSeenRecently,
    bool delayElapsed,
    bool pendingAlcAllowed,
    bool retryDue) {
  AutoBlinkerSessionDecisionPure out = {};
  if (!noaGateOpen || pendingDir == 0 || !requestSeenRecently ||
      (currentReqDir != 0 && currentReqDir != pendingDir)) {
    out.cancel = true;
    return out;
  }
  if (!delayElapsed || !retryDue) return out;
  if (pendingAlcAllowed) out.fire = true;
  else out.retry = true;
  return out;
}

static inline bool autoBlinkerShouldStartSessionPure(
    uint8_t currentReqDir, uint8_t lastReqDir, bool pending, bool pulseActive) {
  return currentReqDir != 0 && !pending && !pulseActive && currentReqDir != lastReqDir;
}
