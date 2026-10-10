#pragma once

#include <stdint.h>

enum Tsl9InputModePure : uint8_t {
  TSL9_INPUT_MODE_LEFT_VOLUME_PURE = 0,
  TSL9_INPUT_MODE_RIGHT_SPEED_PURE = 1,
};

static constexpr uint8_t TSL9_INPUT_MODE_DEFAULT_PURE =
    TSL9_INPUT_MODE_LEFT_VOLUME_PURE;
static constexpr uint16_t TSL9_INPUT_STEP_MS_PURE = 100u;
static constexpr uint16_t TSL9_INPUT_RETRY_MS_PURE = 250u;
static constexpr uint16_t TSL9_INPUT_MANUAL_QUIET_MS_PURE = 300u;
static constexpr uint16_t TSL9_INPUT_REPEAT_MIN_MS_PURE = 2000u;
static constexpr uint16_t TSL9_INPUT_REPEAT_MAX_MS_PURE = 3000u;
static constexpr uint16_t TSL9_INPUT_REPEAT_QUANTUM_MS_PURE = 100u;
static constexpr uint16_t TSL9_INPUT_VOLUME_TEMPLATE_MAX_AGE_MS_PURE = 1000u;
static constexpr uint16_t TSL9_INPUT_SPEED_TEMPLATE_MAX_AGE_MS_PURE = 250u;
static constexpr uint8_t TSL9_INPUT_MUX1_PURE = 1u;
static constexpr uint8_t TSL9_INPUT_PATTERN_FOUR_STEP_PURE = 0u;
static constexpr uint8_t TSL9_INPUT_PATTERN_PAIR_PURE = 1u;

enum Tsl9InputCommandKindPure : uint8_t {
  TSL9_INPUT_COMMAND_NONE_PURE = 0,
  TSL9_INPUT_COMMAND_STEP_PURE = 1,
  TSL9_INPUT_COMMAND_CENTER_PURE = 2,
};

enum Tsl9InputWorkSourcePure : uint8_t {
  TSL9_INPUT_WORK_NONE_PURE = 0,
  TSL9_INPUT_WORK_WARNING_PURE = 1,
  TSL9_INPUT_WORK_PERIODIC_PURE = 2,
};

enum Tsl9InputFailurePure : uint8_t {
  TSL9_INPUT_FAILURE_NONE_PURE = 0,
  TSL9_INPUT_FAILURE_WARNING_CLEARED_PURE,
  TSL9_INPUT_FAILURE_DISABLED_PURE,
  TSL9_INPUT_FAILURE_MODE_CHANGED_PURE,
  TSL9_INPUT_FAILURE_CAN_UNAVAILABLE_PURE,
  TSL9_INPUT_FAILURE_TEMPLATE_STALE_PURE,
  TSL9_INPUT_FAILURE_SEND_FAILED_PURE,
  TSL9_INPUT_FAILURE_MANUAL_ACTIVITY_PURE,
};

struct Tsl9InputInputsPure {
  uint32_t nowMs;
  bool enabled;
  bool warningActive;
  bool txAllowed;
  bool templateFresh;
  uint8_t mode;
  uint32_t randomValue;
  // Zero keeps the legacy warning-only behavior. Non-zero enables a periodic
  // sequence and is expressed in seconds to match the web/NVS contract.
  uint16_t periodicIntervalSeconds;
  uint8_t sequencePattern;
  // Torque uses warning edges as one-shot immediate triggers. TSL9 keeps its
  // legacy randomized warning repeat while the warning remains active.
  bool warningEdgeOnly;
};

struct Tsl9InputCommandPure {
  Tsl9InputCommandKindPure kind;
  uint8_t mode;
  uint8_t step;
  int8_t leftTick;
  int8_t rightTick;
};

struct Tsl9InputSnapshotPure {
  bool active;
  bool retryPending;
  bool cleanupPending;
  uint8_t mode;
  uint8_t step;
  uint32_t currentIntervalMs;
  uint32_t nextActionInMs;
  uint32_t triggerCount;
  uint32_t successCount;
  uint32_t failureCancelCount;
  uint32_t manualDeferralCount;
  Tsl9InputFailurePure lastFailure;
};

static inline uint8_t tsl9InputModeSanitizePure(uint8_t mode) {
  return mode == TSL9_INPUT_MODE_RIGHT_SPEED_PURE
      ? TSL9_INPUT_MODE_RIGHT_SPEED_PURE
      : TSL9_INPUT_MODE_LEFT_VOLUME_PURE;
}

static inline uint8_t tsl9InputPatternSanitizePure(uint8_t pattern) {
  return pattern == TSL9_INPUT_PATTERN_PAIR_PURE
      ? TSL9_INPUT_PATTERN_PAIR_PURE
      : TSL9_INPUT_PATTERN_FOUR_STEP_PURE;
}

static inline uint8_t tsl9InputMuxPure(const uint8_t data[8]) {
  return data ? (uint8_t)(data[0] & 0x03u) : 0xFFu;
}

static inline bool tsl9InputWarningStatePure(uint8_t state) {
  return (state >= 3u && state <= 6u) ||
         (state >= 9u && state <= 10u);
}

static inline bool tsl9InputDeadlineReachedPure(uint32_t now,
                                                uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static inline uint16_t tsl9InputRandomRepeatMsPure(uint32_t randomValue) {
  constexpr uint16_t slots =
      (TSL9_INPUT_REPEAT_MAX_MS_PURE - TSL9_INPUT_REPEAT_MIN_MS_PURE) /
          TSL9_INPUT_REPEAT_QUANTUM_MS_PURE +
      1u;
  return (uint16_t)(TSL9_INPUT_REPEAT_MIN_MS_PURE +
      (randomValue % slots) * TSL9_INPUT_REPEAT_QUANTUM_MS_PURE);
}

static inline uint16_t tsl9InputTemplateMaxAgeMsPure(uint8_t mode) {
  return tsl9InputModeSanitizePure(mode) == TSL9_INPUT_MODE_RIGHT_SPEED_PURE
      ? TSL9_INPUT_SPEED_TEMPLATE_MAX_AGE_MS_PURE
      : TSL9_INPUT_VOLUME_TEMPLATE_MAX_AGE_MS_PURE;
}

static inline uint8_t tsl9InputEncodeSignedSixBitPure(int8_t tick) {
  return (uint8_t)tick & 0x3Fu;
}

static inline int8_t tsl9InputDecodeSignedSixBitPure(uint8_t encoded) {
  encoded &= 0x3Fu;
  return (encoded & 0x20u) != 0u
      ? (int8_t)(encoded | 0xC0u)
      : (int8_t)encoded;
}

static inline void tsl9InputApplyCommandPure(
    const Tsl9InputCommandPure &command, uint8_t data[8]) {
  if (!data || command.kind == TSL9_INPUT_COMMAND_NONE_PURE) return;
  if (tsl9InputModeSanitizePure(command.mode) ==
      TSL9_INPUT_MODE_RIGHT_SPEED_PURE) {
    data[3] = (uint8_t)((data[3] & 0xC0u) |
        tsl9InputEncodeSignedSixBitPure(command.rightTick));
    if (command.rightTick == 0) data[6] |= 0x10u;
    else data[6] &= (uint8_t)~0x10u;
  } else {
    data[2] = (uint8_t)((data[2] & 0xC0u) |
        tsl9InputEncodeSignedSixBitPure(command.leftTick));
  }
}

class Tsl9InputSchedulerPure {
 public:
  Tsl9InputCommandPure service(const Tsl9InputInputsPure &in) {
    const uint8_t mode = tsl9InputModeSanitizePure(in.mode);
    const uint8_t pattern = tsl9InputPatternSanitizePure(in.sequencePattern);
    const uint16_t periodicSeconds = in.periodicIntervalSeconds;
    const bool policyChanged = initialized_ &&
        (mode != configuredMode_ || pattern != configuredPattern_ ||
         periodicSeconds != configuredPeriodicSeconds_ ||
         in.warningEdgeOnly != configuredWarningEdgeOnly_);
    if (!initialized_) {
      initialized_ = true;
      configuredMode_ = mode;
      configuredPattern_ = pattern;
      configuredPeriodicSeconds_ = periodicSeconds;
      configuredWarningEdgeOnly_ = in.warningEdgeOnly;
      if (periodicSeconds != 0u) armPeriodic(in.nowMs);
    } else if (policyChanged) {
      cancelWork(TSL9_INPUT_FAILURE_MODE_CHANGED_PURE, in.nowMs);
      configuredMode_ = mode;
      configuredPattern_ = pattern;
      configuredPeriodicSeconds_ = periodicSeconds;
      configuredWarningEdgeOnly_ = in.warningEdgeOnly;
      previousWarning_ = false;
      if (periodicSeconds != 0u && !configQuiesceRequested_)
        armPeriodic(in.nowMs);
    }

    if (!in.enabled) {
      previousWarning_ = false;
      warningEdgePending_ = false;
      cancelWork(TSL9_INPUT_FAILURE_DISABLED_PURE, in.nowMs);
    } else {
      const bool warningRising = in.warningActive && !previousWarning_;
      const bool warningFalling = !in.warningActive && previousWarning_;
      previousWarning_ = in.warningActive;

      if (warningRising) {
        ++triggerCount_;
        if (active_) warningEdgePending_ = true;
        else scheduleWarning(in.nowMs, 0u);
      }

      if (warningFalling && !configuredWarningEdgeOnly_) {
        warningEdgePending_ = false;
        if (active_ && activeSource_ == TSL9_INPUT_WORK_WARNING_PURE) {
          cancelWork(TSL9_INPUT_FAILURE_WARNING_CLEARED_PURE, in.nowMs);
          if (configuredPeriodicSeconds_ != 0u) armPeriodic(in.nowMs);
        } else if (!active_ &&
                   pendingSource_ == TSL9_INPUT_WORK_WARNING_PURE) {
          clearPending();
          if (configuredPeriodicSeconds_ != 0u) armPeriodic(in.nowMs);
        }
      }

      if (active_ && manualSeen_ &&
          (uint32_t)(in.nowMs - lastManualMs_) <
              TSL9_INPUT_MANUAL_QUIET_MS_PURE) {
        ++manualDeferralCount_;
        cancelForManualActivity(in.nowMs);
      } else if (active_ && (!in.txAllowed || !in.templateFresh)) {
        const Tsl9InputFailurePure reason = in.txAllowed
            ? TSL9_INPUT_FAILURE_TEMPLATE_STALE_PURE
            : TSL9_INPUT_FAILURE_CAN_UNAVAILABLE_PURE;
        cancelAndRetry(reason, in.nowMs, TSL9_INPUT_RETRY_MS_PURE);
      } else if (!active_ && !configQuiesceRequested_ &&
                 configuredPeriodicSeconds_ != 0u && !startPending_) {
        armPeriodic(in.nowMs);
      }
    }

    if (cleanupPending_) return serviceCleanup(in);
    if (configQuiesceRequested_) return none();

    if (active_) {
      if (!tsl9InputDeadlineReachedPure(in.nowMs, stepDeadlineMs_))
        return none();
      retryPending_ = false;
      return currentStepCommand();
    }

    if (!in.enabled) {
      startPending_ = false;
      retryPending_ = false;
      repeatDeadlineMs_ = 0u;
      currentIntervalMs_ = 0u;
      return none();
    }

    if (!startPending_ ||
        !tsl9InputDeadlineReachedPure(in.nowMs, repeatDeadlineMs_))
      return none();

    if (!in.txAllowed || !in.templateFresh) {
      retryPending_ = true;
      currentIntervalMs_ = TSL9_INPUT_RETRY_MS_PURE;
      repeatDeadlineMs_ = in.nowMs + TSL9_INPUT_RETRY_MS_PURE;
      lastFailure_ = in.txAllowed
          ? TSL9_INPUT_FAILURE_TEMPLATE_STALE_PURE
          : TSL9_INPUT_FAILURE_CAN_UNAVAILABLE_PURE;
      return none();
    }

    if (manualSeen_) {
      const uint32_t age = (uint32_t)(in.nowMs - lastManualMs_);
      if (age < TSL9_INPUT_MANUAL_QUIET_MS_PURE) {
        repeatDeadlineMs_ = in.nowMs +
            (TSL9_INPUT_MANUAL_QUIET_MS_PURE - age);
        ++manualDeferralCount_;
        lastFailure_ = TSL9_INPUT_FAILURE_MANUAL_ACTIVITY_PURE;
        return none();
      }
    }

    active_ = true;
    activeSource_ = pendingSource_;
    actionMode_ = mode;
    actionPattern_ = configuredPattern_;
    actionWarningEdgeOnly_ = configuredWarningEdgeOnly_;
    actionPeriodicSeconds_ = configuredPeriodicSeconds_;
    step_ = 0u;
    stepDeadlineMs_ = in.nowMs;
    clearPending();
    retryPending_ = false;
    return currentStepCommand();
  }

  Tsl9InputCommandPure onCommandResult(
      const Tsl9InputCommandPure &command, bool success,
      uint32_t nowMs, uint32_t randomValue) {
    if (command.kind == TSL9_INPUT_COMMAND_CENTER_PURE) {
      if (!cleanupPending_ ||
          tsl9InputModeSanitizePure(command.mode) != cleanupMode_)
        return none();
      if (success) {
        cleanupPending_ = false;
        cleanupDeadlineMs_ = 0u;
        if (!startPending_) retryPending_ = false;
      } else {
        cleanupDeadlineMs_ = nowMs + TSL9_INPUT_RETRY_MS_PURE;
        lastFailure_ = TSL9_INPUT_FAILURE_SEND_FAILED_PURE;
      }
      return none();
    }
    return onStepCommandResult(command, success, nowMs, randomValue);
  }

  void observeManualTicks(int8_t left, int8_t right, uint32_t nowMs) {
    if (left == 0 && right == 0) return;
    manualSeen_ = true;
    lastManualMs_ = nowMs;
    if (!active_ && startPending_ &&
        pendingSource_ == TSL9_INPUT_WORK_PERIODIC_PURE &&
        configuredPeriodicSeconds_ != 0u)
      armPeriodic(nowMs);
  }

  Tsl9InputCommandPure forceCancel(Tsl9InputFailurePure reason,
                                   uint32_t nowMs) {
    previousWarning_ = false;
    warningEdgePending_ = false;
    cancelWork(reason, nowMs);
    return none();
  }

  Tsl9InputCommandPure requestConfigQuiesce(
      Tsl9InputFailurePure reason, uint32_t nowMs) {
    configQuiesceRequested_ = true;
    previousWarning_ = false;
    warningEdgePending_ = false;
    cancelWork(reason, nowMs);
    return none();
  }

  bool configQuiesceComplete() const {
    return configQuiesceRequested_ && !active_ && !cleanupPending_;
  }

  void resumeAfterConfigQuiesce() {
    if (!configQuiesceComplete()) return;
    configQuiesceRequested_ = false;
    cleanupMode_ = TSL9_INPUT_MODE_DEFAULT_PURE;
  }

  void resetCounters() {
    triggerCount_ = 0u;
    successCount_ = 0u;
    failureCancelCount_ = 0u;
    manualDeferralCount_ = 0u;
    lastFailure_ = TSL9_INPUT_FAILURE_NONE_PURE;
  }

  Tsl9InputSnapshotPure snapshot(uint32_t nowMs) const {
    const uint32_t next = !startPending_ ||
        tsl9InputDeadlineReachedPure(nowMs, repeatDeadlineMs_)
        ? 0u : (uint32_t)(repeatDeadlineMs_ - nowMs);
    return {active_, retryPending_ || cleanupPending_, cleanupPending_,
            cleanupPending_ ? cleanupMode_ : actionMode_,
            (uint8_t)(active_ ? step_ + 1u : 0u), currentIntervalMs_, next,
            triggerCount_, successCount_, failureCancelCount_,
            manualDeferralCount_, lastFailure_};
  }

 private:
  static Tsl9InputCommandPure none() {
    return {TSL9_INPUT_COMMAND_NONE_PURE,
            TSL9_INPUT_MODE_DEFAULT_PURE, 0u, 0, 0};
  }

  static Tsl9InputCommandPure center(uint8_t mode) {
    return {TSL9_INPUT_COMMAND_CENTER_PURE,
            tsl9InputModeSanitizePure(mode), 0u, 0, 0};
  }

  Tsl9InputCommandPure onStepCommandResult(
      const Tsl9InputCommandPure &command, bool success,
      uint32_t nowMs, uint32_t randomValue) {
    if (command.kind != TSL9_INPUT_COMMAND_STEP_PURE || !active_ ||
        command.step != step_ ||
        tsl9InputModeSanitizePure(command.mode) != actionMode_)
      return none();

    if (!success) {
      ++failureCancelCount_;
      lastFailure_ = TSL9_INPUT_FAILURE_SEND_FAILED_PURE;
      const uint8_t failedMode = actionMode_;
      const Tsl9InputWorkSourcePure failedSource = activeSource_;
      active_ = false;
      activeSource_ = TSL9_INPUT_WORK_NONE_PURE;
      step_ = 0u;
      clearPending();
      if (!configQuiesceRequested_) {
        if (warningEdgePending_ ||
            (failedSource == TSL9_INPUT_WORK_WARNING_PURE &&
             previousWarning_)) {
          warningEdgePending_ = false;
          scheduleWarning(nowMs, TSL9_INPUT_RETRY_MS_PURE);
        } else if (configuredPeriodicSeconds_ != 0u) {
          schedulePeriodicRetry(nowMs, TSL9_INPUT_RETRY_MS_PURE);
        }
      }
      armCleanup(failedMode, nowMs);
      return center(failedMode);
    }

    if (step_ + 1u < sequenceLength()) {
      ++step_;
      stepDeadlineMs_ = nowMs + TSL9_INPUT_STEP_MS_PURE;
      return none();
    }

    active_ = false;
    const Tsl9InputWorkSourcePure completedSource = activeSource_;
    activeSource_ = TSL9_INPUT_WORK_NONE_PURE;
    step_ = 0u;
    retryPending_ = false;
    ++successCount_;
    if (configQuiesceRequested_) {
      startPending_ = false;
      currentIntervalMs_ = 0u;
      repeatDeadlineMs_ = 0u;
    } else if (warningEdgePending_) {
      warningEdgePending_ = false;
      scheduleWarning(nowMs, 0u);
    } else if (completedSource == TSL9_INPUT_WORK_WARNING_PURE &&
               previousWarning_ && !actionWarningEdgeOnly_) {
      scheduleWarning(nowMs, tsl9InputRandomRepeatMsPure(randomValue));
    } else if (actionPeriodicSeconds_ != 0u) {
      armPeriodic(nowMs);
    } else {
      startPending_ = false;
      currentIntervalMs_ = 0u;
      repeatDeadlineMs_ = 0u;
    }
    return none();
  }

  Tsl9InputCommandPure currentStepCommand() const {
    static constexpr int8_t fourStepTicks[4] = {1, 0, -1, 0};
    static constexpr int8_t pairTicks[2] = {1, -1};
    const int8_t tick = actionPattern_ == TSL9_INPUT_PATTERN_PAIR_PURE
        ? pairTicks[step_] : fourStepTicks[step_];
    return {TSL9_INPUT_COMMAND_STEP_PURE, actionMode_, step_,
            (int8_t)(actionMode_ == TSL9_INPUT_MODE_LEFT_VOLUME_PURE
                         ? tick : 0),
            (int8_t)(actionMode_ == TSL9_INPUT_MODE_RIGHT_SPEED_PURE
                         ? tick : 0)};
  }

  void armCleanup(uint8_t mode, uint32_t nowMs) {
    cleanupPending_ = true;
    cleanupMode_ = tsl9InputModeSanitizePure(mode);
    cleanupDeadlineMs_ = nowMs;
  }

  Tsl9InputCommandPure serviceCleanup(const Tsl9InputInputsPure &in) {
    if (!cleanupPending_ || !in.txAllowed || !in.templateFresh ||
        !tsl9InputDeadlineReachedPure(in.nowMs, cleanupDeadlineMs_))
      return none();
    if (manualSeen_ &&
        (uint32_t)(in.nowMs - lastManualMs_) <
            TSL9_INPUT_MANUAL_QUIET_MS_PURE)
      return none();
    return center(cleanupMode_);
  }

  void cancelWork(Tsl9InputFailurePure reason, uint32_t nowMs) {
    const bool wasActive = active_;
    const bool hadWork = wasActive || startPending_ || retryPending_;
    const uint8_t cleanupMode = wasActive ? actionMode_ : configuredMode_;
    active_ = false;
    activeSource_ = TSL9_INPUT_WORK_NONE_PURE;
    clearPending();
    step_ = 0u;
    stepDeadlineMs_ = 0u;
    repeatDeadlineMs_ = 0u;
    currentIntervalMs_ = 0u;
    if (hadWork) {
      ++failureCancelCount_;
      lastFailure_ = reason;
    }
    if (wasActive && !cleanupPending_) armCleanup(cleanupMode, nowMs);
  }

  void cancelAndRetry(Tsl9InputFailurePure reason,
                      uint32_t nowMs, uint16_t delayMs) {
    const Tsl9InputWorkSourcePure cancelledSource = activeSource_;
    const bool retryWarning = warningEdgePending_ ||
        (cancelledSource == TSL9_INPUT_WORK_WARNING_PURE && previousWarning_);
    cancelWork(reason, nowMs);
    if (retryWarning) {
      warningEdgePending_ = false;
      scheduleWarning(nowMs, delayMs);
      retryPending_ = true;
    } else if (configuredPeriodicSeconds_ != 0u)
      schedulePeriodicRetry(nowMs, delayMs);
  }

  static uint32_t periodicMs(uint16_t seconds) {
    return (uint32_t)seconds * 1000u;
  }

  void armPeriodic(uint32_t nowMs) {
    startPending_ = true;
    pendingSource_ = TSL9_INPUT_WORK_PERIODIC_PURE;
    retryPending_ = false;
    currentIntervalMs_ = periodicMs(configuredPeriodicSeconds_);
    repeatDeadlineMs_ = nowMs + currentIntervalMs_;
  }

  void schedulePeriodicRetry(uint32_t nowMs, uint16_t delayMs) {
    startPending_ = true;
    pendingSource_ = TSL9_INPUT_WORK_PERIODIC_PURE;
    retryPending_ = true;
    currentIntervalMs_ = delayMs;
    repeatDeadlineMs_ = nowMs + delayMs;
  }

  void scheduleWarning(uint32_t nowMs, uint16_t delayMs) {
    startPending_ = true;
    pendingSource_ = TSL9_INPUT_WORK_WARNING_PURE;
    retryPending_ = delayMs != 0u;
    currentIntervalMs_ = delayMs;
    repeatDeadlineMs_ = nowMs + delayMs;
  }

  void clearPending() {
    startPending_ = false;
    pendingSource_ = TSL9_INPUT_WORK_NONE_PURE;
    retryPending_ = false;
    currentIntervalMs_ = 0u;
    repeatDeadlineMs_ = 0u;
  }

  void cancelForManualActivity(uint32_t nowMs) {
    const Tsl9InputWorkSourcePure cancelledSource = activeSource_;
    const bool retryWarning = warningEdgePending_ ||
        (cancelledSource == TSL9_INPUT_WORK_WARNING_PURE && previousWarning_);
    cancelWork(TSL9_INPUT_FAILURE_MANUAL_ACTIVITY_PURE, nowMs);
    if (retryWarning) {
      warningEdgePending_ = false;
      scheduleWarning(nowMs, TSL9_INPUT_MANUAL_QUIET_MS_PURE);
    } else if (configuredPeriodicSeconds_ != 0u) {
      // Physical input has priority. A cancelled periodic gesture restarts its
      // full configured interval rather than firing as soon as quiet expires.
      armPeriodic(nowMs);
    }
  }

  uint8_t sequenceLength() const {
    return actionPattern_ == TSL9_INPUT_PATTERN_PAIR_PURE ? 2u : 4u;
  }

  bool initialized_ = false;
  bool previousWarning_ = false;
  bool warningEdgePending_ = false;
  bool active_ = false;
  bool startPending_ = false;
  bool retryPending_ = false;
  bool manualSeen_ = false;
  bool configQuiesceRequested_ = false;
  bool cleanupPending_ = false;
  uint8_t configuredMode_ = TSL9_INPUT_MODE_DEFAULT_PURE;
  uint8_t configuredPattern_ = TSL9_INPUT_PATTERN_FOUR_STEP_PURE;
  uint16_t configuredPeriodicSeconds_ = 0u;
  bool configuredWarningEdgeOnly_ = false;
  uint8_t actionMode_ = TSL9_INPUT_MODE_DEFAULT_PURE;
  Tsl9InputWorkSourcePure pendingSource_ = TSL9_INPUT_WORK_NONE_PURE;
  Tsl9InputWorkSourcePure activeSource_ = TSL9_INPUT_WORK_NONE_PURE;
  uint8_t actionPattern_ = TSL9_INPUT_PATTERN_FOUR_STEP_PURE;
  uint16_t actionPeriodicSeconds_ = 0u;
  bool actionWarningEdgeOnly_ = false;
  uint8_t cleanupMode_ = TSL9_INPUT_MODE_DEFAULT_PURE;
  uint8_t step_ = 0u;
  uint32_t stepDeadlineMs_ = 0u;
  uint32_t repeatDeadlineMs_ = 0u;
  uint32_t cleanupDeadlineMs_ = 0u;
  uint32_t lastManualMs_ = 0u;
  uint32_t currentIntervalMs_ = 0u;
  uint32_t triggerCount_ = 0u;
  uint32_t successCount_ = 0u;
  uint32_t failureCancelCount_ = 0u;
  uint32_t manualDeferralCount_ = 0u;
  Tsl9InputFailurePure lastFailure_ = TSL9_INPUT_FAILURE_NONE_PURE;
};
