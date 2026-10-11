#pragma once
#include <stdint.h>

// Current Mode H engine and its dependency closure. No Arduino/ESP-IDF dependencies.
// Persisted field layout, arithmetic and RNG ordering are retained.

// Host-testable Mode H event engine. No Arduino/ESP-IDF dependencies.
// Tesla EPAS torsion-bar torque uses raw * 0.01 - 20.5 Nm; raw 2050 is 0 Nm.

static constexpr uint16_t MODE_H_BASE_TORQUE_CENTER_RAW = 2050u;
static constexpr uint16_t MODE_H_BASE_RAW_MAX = 0x0FFFu;
// Mode H LAB-editable primary-event magnitude limits, in 0.01 Nm units.
// The lower bound stays above the fixed correction ceiling (0.95 Nm).
// LAB may extend the primary Human Interaction peak up to 3.00 Nm; the
// production Rev.1 Plus default is 1.50–2.20 Nm.
static constexpr uint16_t MODE_H_BASE_PEAK_ALLOWED_MIN_RAW = 100u; // 1.00 Nm
static constexpr uint16_t MODE_H_BASE_PEAK_ALLOWED_MAX_RAW = 300u; // 3.00 Nm
static constexpr uint16_t MODE_H_BASE_PEAK_DEFAULT_MIN_RAW = 150u; // Rev.1 Plus 1.50 Nm
static constexpr uint16_t MODE_H_BASE_PEAK_DEFAULT_MAX_RAW = 220u; // Rev.1 Plus 2.20 Nm
static constexpr uint16_t MODE_H_BASE_WAIT_ALLOWED_MIN_MS = 300u;
static constexpr uint16_t MODE_H_BASE_WAIT_ALLOWED_MAX_MS = 5000u;
static constexpr uint16_t MODE_H_BASE_REFRACTORY_ALLOWED_MIN_MS = 300u;
static constexpr uint16_t MODE_H_BASE_REFRACTORY_ALLOWED_MAX_MS = 2500u;

enum ModeHBasePhasePure : uint8_t {
  MH_IDLE = 0,
  MH_WAIT = 1,
  MH_RAMP_IN = 2,
  MH_INTERACT = 3,
  MH_RAMP_OUT = 4,
  MH_REFRACTORY = 5,
  MH_PAUSED_STOPPED = 6
};

enum ModeHBaseEventTypePure : uint8_t {
  MH_EVENT_NONE = 0,
  MH_EVENT_SIMPLE = 1,
  MH_EVENT_CORRECTION = 2
};

enum ModeHBaseMotionPure : uint8_t {
  MH_MOTION_UNKNOWN = 0,
  MH_MOTION_STALE = 1,
  MH_MOTION_STOPPED = 2,
  MH_MOTION_CONFIRMING = 3,
  MH_MOTION_MOVING = 4
};

enum ModeHBaseBlockReasonPure : uint8_t {
  MH_BLOCK_NONE = 0,
  MH_BLOCK_RUN_GATE = 1,
  MH_BLOCK_SPEED_STALE = 2,
  MH_BLOCK_STOPPED = 3,
  MH_BLOCK_MOVE_CONFIRMING = 4
};

struct ModeHBaseConfigPure {
  uint16_t waitMinMs;
  uint16_t waitMaxMs;
  uint16_t rampInMinMs;
  uint16_t rampInMaxMs;
  uint16_t interactMinMs;
  uint16_t interactMaxMs;
  uint16_t rampOutMinMs;
  uint16_t rampOutMaxMs;
  uint16_t refractoryMinMs;
  uint16_t refractoryMaxMs;
  uint16_t peakMinRaw;
  uint16_t peakMaxRaw;
  uint16_t correctionMinRaw;
  uint16_t correctionMaxRaw;
  uint8_t directionPersistencePct;
  uint8_t correctionProbabilityPct;
  uint8_t hoOverridePct;
  int16_t stopThresholdKphX100;
  int16_t moveThresholdKphX100;
  uint16_t moveConfirmMs;
};

struct ModeHBaseEventPure {
  uint32_t waitDurationMs;
  uint32_t rampInDurationMs;
  uint32_t interactDurationMs;
  uint32_t rampOutDurationMs;
  uint32_t refractoryDurationMs;
  uint16_t peakRaw;
  uint16_t correctionRaw;
  int8_t direction;
  uint8_t type;
};

struct ModeHBaseStatePure {
  uint8_t phase;
  uint8_t motion;
  bool movingConfirmed;
  bool moveCandidateActive;
  bool eventValid;
  bool carrier;
  uint32_t moveCandidateStartMs;
  uint32_t phaseStartMs;
  uint32_t eventStartMs;
  uint32_t sessionStartMs;
  uint32_t rngState;
  int8_t previousDirection;
  uint16_t rampBaselineRaw;
  uint16_t outputRaw;
  ModeHBaseEventPure event;
  uint32_t eventCount;
  uint32_t simpleCount;
  uint32_t correctionCount;
  bool hoDecisionValid;
  bool hoOverrideActive;
  uint32_t hoDecisionStartMs;
  uint16_t hoDecisionDurationMs;
};

struct ModeHBaseStepResultPure {
  bool tx;
  bool setHo;
  bool hoOverrideValid;
  uint8_t hoLevel;
  bool carrier;
  uint16_t raw;
  uint8_t phase;
  uint8_t motion;
  uint8_t blockReason;
};

static inline ModeHBaseConfigPure modeHBaseRev1PlusConfigPure() {
  ModeHBaseConfigPure c = {};
  c.waitMinMs = 1200u;
  c.waitMaxMs = 3000u;
  c.rampInMinMs = 260u;
  c.rampInMaxMs = 520u;
  c.interactMinMs = 700u;
  c.interactMaxMs = 1300u;
  c.rampOutMinMs = 320u;
  c.rampOutMaxMs = 680u;
  c.refractoryMinMs = 800u;
  c.refractoryMaxMs = 1800u;
  c.peakMinRaw = MODE_H_BASE_PEAK_DEFAULT_MIN_RAW;
  c.peakMaxRaw = MODE_H_BASE_PEAK_DEFAULT_MAX_RAW;
  c.correctionMinRaw = 55u;
  c.correctionMaxRaw = 95u;
  c.directionPersistencePct = 65u;
  c.correctionProbabilityPct = 30u;
  c.hoOverridePct = 100u;
  c.stopThresholdKphX100 = 16;
  c.moveThresholdKphX100 = 64;
  c.moveConfirmMs = 300u;
  return c;
}


static inline bool modeHBaseTimingValidPure(uint16_t waitMinMs, uint16_t waitMaxMs,
                                            uint16_t refractoryMinMs, uint16_t refractoryMaxMs) {
  return waitMinMs >= MODE_H_BASE_WAIT_ALLOWED_MIN_MS &&
         waitMaxMs <= MODE_H_BASE_WAIT_ALLOWED_MAX_MS &&
         waitMinMs <= waitMaxMs &&
         refractoryMinMs >= MODE_H_BASE_REFRACTORY_ALLOWED_MIN_MS &&
         refractoryMaxMs <= MODE_H_BASE_REFRACTORY_ALLOWED_MAX_MS &&
         refractoryMinMs <= refractoryMaxMs;
}

static inline bool modeHBasePeakRangeValidPure(uint16_t minRaw, uint16_t maxRaw) {
  return minRaw >= MODE_H_BASE_PEAK_ALLOWED_MIN_RAW &&
         maxRaw <= MODE_H_BASE_PEAK_ALLOWED_MAX_RAW &&
         minRaw <= maxRaw;
}


static inline uint32_t modeHBaseSanitizeSeedPure(uint32_t seed) {
  return seed == 0u ? 0x6D2B79F5u : seed;
}

static inline uint32_t modeHBaseNextRandomPure(uint32_t &state) {
  uint32_t x = modeHBaseSanitizeSeedPure(state);
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = modeHBaseSanitizeSeedPure(x);
  return state;
}

static inline uint32_t modeHBaseRangePure(uint32_t &state, uint32_t lo, uint32_t hi) {
  if (hi < lo) {
    const uint32_t t = lo;
    lo = hi;
    hi = t;
  }
  const uint32_t span = hi - lo + 1u;
  return lo + (span == 0u ? 0u : (modeHBaseNextRandomPure(state) % span));
}

static inline void modeHBaseClearEventPure(ModeHBaseStatePure &s) {
  s.event = ModeHBaseEventPure{};
  s.eventValid = false;
  s.carrier = false;
  s.phaseStartMs = 0u;
  s.eventStartMs = 0u;
  s.sessionStartMs = 0u;
  s.rampBaselineRaw = MODE_H_BASE_TORQUE_CENTER_RAW;
  s.outputRaw = MODE_H_BASE_TORQUE_CENTER_RAW;
  s.hoDecisionValid = false;
  s.hoOverrideActive = false;
  s.hoDecisionStartMs = 0u;
  s.hoDecisionDurationMs = 0u;
}

static inline void modeHBaseResetRuntimePure(ModeHBaseStatePure &s, uint8_t phase) {
  modeHBaseClearEventPure(s);
  s.phase = phase;
  s.motion = phase == MH_PAUSED_STOPPED ? MH_MOTION_STOPPED : MH_MOTION_UNKNOWN;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}

static inline void modeHBaseInitPure(ModeHBaseStatePure &s, uint32_t seed) {
  s = ModeHBaseStatePure{};
  s.rngState = modeHBaseSanitizeSeedPure(seed);
  s.phase = MH_IDLE;
  s.motion = MH_MOTION_UNKNOWN;
  s.rampBaselineRaw = MODE_H_BASE_TORQUE_CENTER_RAW;
  s.outputRaw = MODE_H_BASE_TORQUE_CENTER_RAW;
}


static inline uint32_t modeHBaseProgressQ15Pure(uint32_t elapsed, uint32_t duration) {
  if (duration == 0u || elapsed >= duration) return 32768u;
  return (uint32_t)(((uint64_t)elapsed * 32768u) / duration);
}

static inline uint32_t modeHBaseSmoothstepQ15Pure(uint32_t x) {
  if (x >= 32768u) return 32768u;
  const uint64_t x2 = ((uint64_t)x * x + 16384u) >> 15;
  const uint32_t threeMinusTwoX = 98304u - (2u * x);
  uint64_t y = (x2 * threeMinusTwoX + 16384u) >> 15;
  if (y > 32768u) y = 32768u;
  return (uint32_t)y;
}

static inline uint16_t modeHBaseClampRawPure(int32_t raw) {
  if (raw < 0) return 0u;
  if (raw > (int32_t)MODE_H_BASE_RAW_MAX) return MODE_H_BASE_RAW_MAX;
  return (uint16_t)raw;
}

static inline uint16_t modeHBaseLerpRawPure(uint16_t a, uint16_t b, uint32_t q15) {
  if (q15 >= 32768u) return b;
  const int32_t delta = (int32_t)b - (int32_t)a;
  const int64_t scaled = (int64_t)delta * (int64_t)q15;
  const int32_t rounded = scaled >= 0
      ? (int32_t)((scaled + 16384) / 32768)
      : (int32_t)((scaled - 16384) / 32768);
  return modeHBaseClampRawPure((int32_t)a + rounded);
}

static inline uint16_t modeHBasePrimaryTargetRawPure(const ModeHBaseEventPure &e) {
  return modeHBaseClampRawPure((int32_t)MODE_H_BASE_TORQUE_CENTER_RAW +
                              (int32_t)e.direction * (int32_t)e.peakRaw);
}

static inline uint16_t modeHBaseCorrectionTargetRawPure(const ModeHBaseEventPure &e) {
  return modeHBaseClampRawPure((int32_t)MODE_H_BASE_TORQUE_CENTER_RAW -
                              (int32_t)e.direction * (int32_t)e.correctionRaw);
}

static inline uint16_t modeHBaseFinalEventRawPure(const ModeHBaseEventPure &e) {
  return e.type == MH_EVENT_CORRECTION
      ? modeHBaseCorrectionTargetRawPure(e)
      : modeHBasePrimaryTargetRawPure(e);
}

static inline void modeHBasePausePure(ModeHBaseStatePure &s, uint8_t motion) {
  modeHBaseClearEventPure(s);
  s.phase = MH_PAUSED_STOPPED;
  s.motion = motion;
  s.movingConfirmed = false;
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.previousDirection = 0;
}


static inline bool modeHBaseMotionAllowsPure(ModeHBaseStatePure &s,
                                            const ModeHBaseConfigPure &c,
                                            uint32_t nowMs,
                                            bool speedValid,
                                            bool speedFresh,
                                            uint16_t speedRaw,
                                            uint8_t &blockReason) {
  if (!speedValid || !speedFresh) {
    modeHBasePausePure(s, MH_MOTION_STALE);
    blockReason = MH_BLOCK_SPEED_STALE;
    return false;
  }

  const int32_t speedKphX100 = (int32_t)speedRaw * 8 - 4000;
  if (s.movingConfirmed) {
    if (speedKphX100 <= c.stopThresholdKphX100) {
      modeHBasePausePure(s, MH_MOTION_STOPPED);
      blockReason = MH_BLOCK_STOPPED;
      return false;
    }
    s.motion = MH_MOTION_MOVING;
    blockReason = MH_BLOCK_NONE;
    return true;
  }

  if (speedKphX100 >= c.moveThresholdKphX100) {
    if (!s.moveCandidateActive) {
      s.moveCandidateActive = true;
      s.moveCandidateStartMs = nowMs;
    }
    if (c.moveConfirmMs == 0u || (uint32_t)(nowMs - s.moveCandidateStartMs) >= c.moveConfirmMs) {
      s.movingConfirmed = true;
      s.moveCandidateActive = false;
      s.motion = MH_MOTION_MOVING;
      s.phase = MH_IDLE;
      blockReason = MH_BLOCK_NONE;
      return true;
    }
    s.phase = MH_PAUSED_STOPPED;
    s.motion = MH_MOTION_CONFIRMING;
    blockReason = MH_BLOCK_MOVE_CONFIRMING;
    return false;
  }

  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.phase = MH_PAUSED_STOPPED;
  s.motion = MH_MOTION_STOPPED;
  blockReason = MH_BLOCK_STOPPED;
  return false;
}


static inline const char* modeHBasePhaseNamePure(uint8_t phase) {
  switch (phase) {
    case MH_WAIT: return "WAIT";
    case MH_RAMP_IN: return "RAMP_IN";
    case MH_INTERACT: return "INTERACT";
    case MH_RAMP_OUT: return "RAMP_OUT";
    case MH_REFRACTORY: return "REFRACTORY";
    case MH_PAUSED_STOPPED: return "PAUSED_STOPPED";
    default: return "IDLE";
  }
}

static inline const char* modeHBaseEventTypeNamePure(uint8_t type) {
  if (type == MH_EVENT_SIMPLE) return "SIMPLE";
  if (type == MH_EVENT_CORRECTION) return "CORRECTION";
  return "NONE";
}

static inline const char* modeHBaseMotionNamePure(uint8_t motion) {
  switch (motion) {
    case MH_MOTION_STALE: return "STALE";
    case MH_MOTION_STOPPED: return "STOPPED";
    case MH_MOTION_CONFIRMING: return "CONFIRMING";
    case MH_MOTION_MOVING: return "MOVING";
    default: return "UNKNOWN";
  }
}

enum ModeHHandsOnHoPolicyPure : uint8_t {
  MH_HO_ALWAYS_1 = 0,
  MH_HO_THRESHOLD_1 = 1,
  MH_HO_TIERED_1_2 = 2
};

static constexpr uint16_t MODE_H_HANDS_ON_HO_THRESHOLD_MIN_RAW = 10u;      // 0.10 Nm

static constexpr uint16_t MODE_H_HANDS_ON_HO_THRESHOLD_MAX_RAW = 300u;     // 3.00 Nm

struct ModeHHandsOnHoDecisionPure {
  bool overrideValid;
  uint8_t level;
};

static inline bool modeHHandsOnHoPolicyValidPure(uint8_t mode) {
  return mode == MH_HO_ALWAYS_1 || mode == MH_HO_THRESHOLD_1 || mode == MH_HO_TIERED_1_2;
}

static inline const char* modeHHandsOnHoPolicyNamePure(uint8_t mode) {
  switch (mode) {
    case MH_HO_THRESHOLD_1: return "THRESHOLD_HO1";
    case MH_HO_TIERED_1_2: return "TIERED_HO1_HO2";
    default: return "ALWAYS_HO1";
  }
}

static inline uint8_t modeHHandsOnSafeHoLevelPure(uint8_t level) {
  return level > 2u ? 2u : level;
}

static inline uint16_t modeHHandsOnAbsTorqueRawPure(uint16_t raw) {
  return raw >= MODE_H_BASE_TORQUE_CENTER_RAW
      ? (uint16_t)(raw - MODE_H_BASE_TORQUE_CENTER_RAW)
      : (uint16_t)(MODE_H_BASE_TORQUE_CENTER_RAW - raw);
}

static inline ModeHHandsOnHoDecisionPure modeHHandsOnHoDecisionPure(
    uint8_t policy, uint16_t torqueMagnitudeRaw,
    uint16_t ho1ThresholdRaw, uint16_t ho2ThresholdRaw) {
  ModeHHandsOnHoDecisionPure r = {false, 0u};
  if (!modeHHandsOnHoPolicyValidPure(policy)) return r;
  if (policy == MH_HO_ALWAYS_1) {
    r.overrideValid = true;
    r.level = 1u;
  } else if (policy == MH_HO_THRESHOLD_1) {
    if (torqueMagnitudeRaw >= ho1ThresholdRaw) {
      r.overrideValid = true;
      r.level = 1u;
    }
  } else {
    if (torqueMagnitudeRaw >= ho2ThresholdRaw) {
      r.overrideValid = true;
      r.level = 2u;
    } else if (torqueMagnitudeRaw >= ho1ThresholdRaw) {
      r.overrideValid = true;
      r.level = 1u;
    }
  }
  r.level = modeHHandsOnSafeHoLevelPure(r.level);
  return r;
}

// Mode H Rev.4: Human Interaction primary events with a per-stock-RX,
// stock-opposite carrier during WAIT and REFRACTORY.
// Torque uses 0.01 Nm raw units around the EPAS center (2050 = 0 Nm).
//
// Carrier rule (outside the near-zero deadband):
//   stock > 0 -> TX = -(abs(stock) + randomCarrier)
//   stock < 0 -> TX = +(abs(stock) + randomCarrier)
// The carrier magnitude is re-sampled on every eligible stock 0x370 RX.
// Primary-event direction is independently biased 80% negative / 20% positive.

static constexpr uint16_t MODE_H_CARRIER_MIN_ALLOWED_RAW = 10u; // 0.10 Nm
static constexpr uint16_t MODE_H_CARRIER_MAX_ALLOWED_RAW = 80u; // 0.80 Nm
static constexpr uint16_t MODE_H_STOCK_DEADBAND_RAW = 5u;       // ±0.05 Nm -> stock passthrough
static constexpr uint16_t MODE_H_VISUAL_RESCUE_MAX_DELAY_MS = 2000u;
static constexpr uint8_t MODE_H_NEGATIVE_BIAS_PCT = 80u;

static inline bool modeHVisualWarningActivePure(uint8_t handsOnState) {
  return handsOnState >= 3u && handsOnState <= 5u;
}

static inline bool modeHVisualWarningEdgePure(
    uint8_t previousHandsOnState, uint8_t handsOnState) {
  return previousHandsOnState < 3u &&
         modeHVisualWarningActivePure(handsOnState);
}

struct ModeHConfigPure {
  ModeHBaseConfigPure base;
  uint16_t carrierMinRaw;
  uint16_t carrierMaxRaw;
  uint8_t hoPolicy;
  uint16_t ho1ThresholdRaw;
  uint16_t ho2ThresholdRaw;
  bool visualRescueEnabled;
  uint16_t visualRescueDelayMs;
};

struct ModeHStatePure {
  ModeHBaseStatePure base;
  uint32_t carrierRngState;
  uint16_t lastCarrierMagnitudeRaw;
  bool lastCarrierApplied;
  uint16_t rampOutTargetRaw;
  bool rampOutTargetValid;
  uint32_t visualWarningEpochSeen;
  uint32_t visualRescuePendingSinceMs;
  uint32_t visualRescueCount;
  bool visualRescuePending;
};

static inline ModeHConfigPure modeHDefaultConfigPure() {
  ModeHConfigPure c = {};
  // Preserve the established Human Interaction ramp/interact/correction shape,
  // but Rev.4 owns its primary envelope and cadence.
  c.base = modeHBaseRev1PlusConfigPure();
  c.base.peakMinRaw = 180u;              // 1.80 Nm
  c.base.peakMaxRaw = 260u;              // 2.60 Nm
  c.base.waitMinMs = 900u;               // 0.90 s
  c.base.waitMaxMs = 3000u;              // 3.00 s
  c.base.refractoryMinMs = 500u;         // 0.50 s
  c.base.refractoryMaxMs = 1500u;        // 1.50 s
  c.base.directionPersistencePct = 0u;   // Rev.4 uses fixed 80/20 direction bias.
  c.base.hoOverridePct = 0u;             // Rev.4 owns the HO decision below.
  c.carrierMinRaw = 10u;                 // 0.10 Nm
  c.carrierMaxRaw = 60u;                 // 0.60 Nm
  c.hoPolicy = MH_HO_TIERED_1_2;
  c.ho1ThresholdRaw = 40u;               // 0.40 Nm
  c.ho2ThresholdRaw = 200u;              // 2.00 Nm
  c.visualRescueEnabled = true;
  c.visualRescueDelayMs = 500u;
  return c;
}

static inline bool modeHMigrateV16DefaultPure(ModeHConfigPure &c) {
  const bool untouchedV16Default =
      c.base.peakMinRaw == 150u && c.base.peakMaxRaw == 210u &&
      c.base.waitMinMs == 900u && c.base.waitMaxMs == 2000u &&
      c.base.refractoryMinMs == 500u && c.base.refractoryMaxMs == 1500u &&
      c.carrierMinRaw == 10u && c.carrierMaxRaw == 60u &&
      !c.visualRescueEnabled && c.visualRescueDelayMs == 500u;
  if (!untouchedV16Default) return false;
  c = modeHDefaultConfigPure();
  return true;
}

static inline bool modeHMigrateV17DefaultPure(ModeHConfigPure &c) {
  const bool exactV17Default =
      c.base.peakMinRaw == 180u && c.base.peakMaxRaw == 260u &&
      c.base.waitMinMs == 900u && c.base.waitMaxMs == 3000u &&
      c.base.refractoryMinMs == 500u && c.base.refractoryMaxMs == 1500u &&
      c.carrierMinRaw == 10u && c.carrierMaxRaw == 60u &&
      c.hoPolicy == MH_HO_TIERED_1_2 &&
      c.ho1ThresholdRaw == 40u && c.ho2ThresholdRaw == 175u &&
      c.visualRescueEnabled && c.visualRescueDelayMs == 500u;
  if (!exactV17Default) return false;
  c = modeHDefaultConfigPure();
  return true;
}

static inline bool modeHConfigValidPure(const ModeHConfigPure &c) {
  return modeHBasePeakRangeValidPure(c.base.peakMinRaw, c.base.peakMaxRaw) &&
         modeHBaseTimingValidPure(c.base.waitMinMs, c.base.waitMaxMs,
                                   c.base.refractoryMinMs, c.base.refractoryMaxMs) &&
         c.carrierMinRaw >= MODE_H_CARRIER_MIN_ALLOWED_RAW &&
         c.carrierMaxRaw <= MODE_H_CARRIER_MAX_ALLOWED_RAW &&
         c.carrierMinRaw <= c.carrierMaxRaw &&
         modeHHandsOnHoPolicyValidPure(c.hoPolicy) &&
         c.ho1ThresholdRaw >= MODE_H_HANDS_ON_HO_THRESHOLD_MIN_RAW &&
         c.ho2ThresholdRaw <= MODE_H_HANDS_ON_HO_THRESHOLD_MAX_RAW &&
         c.ho1ThresholdRaw <= c.ho2ThresholdRaw &&
         c.visualRescueDelayMs <= MODE_H_VISUAL_RESCUE_MAX_DELAY_MS &&
         c.base.hoOverridePct <= 100u;
}

static inline uint32_t modeHSanitizeSeedPure(uint32_t seed) {
  return modeHBaseSanitizeSeedPure(seed ^ 0x52345634u);
}

static inline void modeHInitPure(ModeHStatePure &s, uint32_t seed) {
  s = ModeHStatePure{};
  modeHBaseInitPure(s.base, seed);
  s.carrierRngState = modeHSanitizeSeedPure(seed);
}

static inline void modeHResetRuntimePure(ModeHStatePure &s, uint8_t phase) {
  const uint32_t carrierRng = s.carrierRngState;
  const uint32_t visualWarningEpochSeen = s.visualWarningEpochSeen;
  const uint32_t visualRescueCount = s.visualRescueCount;
  modeHBaseResetRuntimePure(s.base, phase);
  s.carrierRngState = modeHSanitizeSeedPure(carrierRng);
  s.lastCarrierMagnitudeRaw = 0u;
  s.lastCarrierApplied = false;
  s.rampOutTargetRaw = MODE_H_BASE_TORQUE_CENTER_RAW;
  s.rampOutTargetValid = false;
  s.visualWarningEpochSeen = visualWarningEpochSeen;
  s.visualRescueCount = visualRescueCount;
  s.visualRescuePendingSinceMs = 0u;
  s.visualRescuePending = false;
}

static inline void modeHReseedPure(ModeHStatePure &s, uint32_t seed) {
  const uint32_t events = s.base.eventCount;
  const uint32_t simple = s.base.simpleCount;
  const uint32_t correction = s.base.correctionCount;
  const uint32_t visualWarningEpochSeen = s.visualWarningEpochSeen;
  const uint32_t visualRescueCount = s.visualRescueCount;
  modeHInitPure(s, seed);
  s.base.eventCount = events;
  s.base.simpleCount = simple;
  s.base.correctionCount = correction;
  s.visualWarningEpochSeen = visualWarningEpochSeen;
  s.visualRescueCount = visualRescueCount;
}

static inline ModeHBaseEventPure modeHGenerateEventPure(
    ModeHStatePure &s, const ModeHConfigPure &c) {
  ModeHBaseEventPure e = {};
  e.waitDurationMs = modeHBaseRangePure(s.base.rngState, c.base.waitMinMs, c.base.waitMaxMs);
  e.rampInDurationMs = modeHBaseRangePure(s.base.rngState, c.base.rampInMinMs, c.base.rampInMaxMs);
  e.interactDurationMs = modeHBaseRangePure(s.base.rngState, c.base.interactMinMs, c.base.interactMaxMs);
  e.rampOutDurationMs = modeHBaseRangePure(s.base.rngState, c.base.rampOutMinMs, c.base.rampOutMaxMs);
  e.refractoryDurationMs = modeHBaseRangePure(
      s.base.rngState, c.base.refractoryMinMs, c.base.refractoryMaxMs);
  e.peakRaw = (uint16_t)modeHBaseRangePure(
      s.base.rngState, c.base.peakMinRaw, c.base.peakMaxRaw);
  e.correctionRaw = (uint16_t)modeHBaseRangePure(
      s.base.rngState, c.base.correctionMinRaw, c.base.correctionMaxRaw);

  const uint32_t directionRoll = modeHBaseRangePure(s.base.rngState, 0u, 99u);
  e.direction = directionRoll < MODE_H_NEGATIVE_BIAS_PCT ? -1 : 1;
  s.base.previousDirection = e.direction;

  const uint32_t typeRoll = modeHBaseRangePure(s.base.rngState, 0u, 99u);
  e.type = typeRoll < c.base.correctionProbabilityPct ? MH_EVENT_CORRECTION : MH_EVENT_SIMPLE;
  return e;
}

static inline void modeHBeginWaitPure(
    ModeHStatePure &s, const ModeHConfigPure &c,
    uint32_t nowMs, bool newSession) {
  s.base.event = modeHGenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = MH_WAIT;
  s.base.phaseStartMs = nowMs;
  s.base.eventStartMs = 0u;
  if (newSession || s.base.sessionStartMs == 0u) s.base.sessionStartMs = nowMs;
  s.base.carrier = true;
}

static inline void modeHBeginVisualRescuePeakPure(
    ModeHStatePure &s, const ModeHConfigPure &c, uint32_t nowMs) {
  s.base.event = modeHGenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = MH_INTERACT;
  s.base.phaseStartMs = nowMs;
  s.base.eventStartMs = nowMs;
  if (s.base.sessionStartMs == 0u) s.base.sessionStartMs = nowMs;
  s.base.eventCount++;
  if (s.base.event.type == MH_EVENT_CORRECTION) s.base.correctionCount++;
  else s.base.simpleCount++;
  s.base.carrier = false;
  s.lastCarrierApplied = false;
  s.rampOutTargetValid = false;
  s.visualRescuePending = false;
  s.visualRescuePendingSinceMs = 0u;
  s.visualRescueCount++;
}

static inline bool modeHUpdateVisualRescuePure(
    ModeHStatePure &s,
    const ModeHConfigPure &c,
    uint32_t nowMs,
    uint32_t visualWarningEpoch,
    uint32_t visualWarningEnterMs,
    bool visualWarningActive) {
  if (visualWarningEpoch != s.visualWarningEpochSeen) {
    s.visualWarningEpochSeen = visualWarningEpoch;
    s.visualRescuePending = c.visualRescueEnabled &&
                            visualWarningEpoch != 0u &&
                            visualWarningActive;
    s.visualRescuePendingSinceMs = s.visualRescuePending ? visualWarningEnterMs : 0u;
  } else if (!c.visualRescueEnabled || !visualWarningActive) {
    s.visualRescuePending = false;
    s.visualRescuePendingSinceMs = 0u;
  }

  if (!s.visualRescuePending) return false;
  const uint32_t elapsed = (uint32_t)(nowMs - s.visualRescuePendingSinceMs);
  if (elapsed < c.visualRescueDelayMs) return false;
  modeHBeginVisualRescuePeakPure(s, c, nowMs);
  return true;
}

static inline uint16_t modeHCarrierOutputPure(
    ModeHStatePure &s, const ModeHConfigPure &c, uint16_t sourceRaw) {
  s.lastCarrierMagnitudeRaw = (uint16_t)modeHBaseRangePure(
      s.carrierRngState, c.carrierMinRaw, c.carrierMaxRaw);

  const int32_t stockSigned = (int32_t)sourceRaw - (int32_t)MODE_H_BASE_TORQUE_CENTER_RAW;
  const int32_t stockAbs = stockSigned < 0 ? -stockSigned : stockSigned;
  if (stockAbs <= (int32_t)MODE_H_STOCK_DEADBAND_RAW) {
    s.lastCarrierApplied = false;
    return sourceRaw;
  }

  const int32_t magnitude = stockAbs + (int32_t)s.lastCarrierMagnitudeRaw;
  const int32_t oppositeSigned = stockSigned > 0 ? -magnitude : magnitude;
  s.lastCarrierApplied = true;
  return modeHBaseClampRawPure((int32_t)MODE_H_BASE_TORQUE_CENTER_RAW + oppositeSigned);
}

static inline ModeHBaseStepResultPure modeHStepPure(
    ModeHStatePure &s,
    const ModeHConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw,
    uint32_t visualWarningEpoch = 0u,
    uint32_t visualWarningEnterMs = 0u,
    bool visualWarningActive = false) {
  ModeHBaseStepResultPure result = {};
  result.raw = sourceRaw;
  result.phase = s.base.phase;
  result.motion = s.base.motion;

  if (!runAllowed) {
    modeHResetRuntimePure(s, MH_IDLE);
    result.phase = s.base.phase;
    result.motion = s.base.motion;
    result.blockReason = MH_BLOCK_RUN_GATE;
    return result;
  }

  uint8_t motionBlock = MH_BLOCK_NONE;
  if (!modeHBaseMotionAllowsPure(
          s.base, c.base, nowMs, speedValid, speedFresh, speedRaw, motionBlock)) {
    s.lastCarrierApplied = false;
    result.phase = s.base.phase;
    result.motion = s.base.motion;
    result.blockReason = motionBlock;
    return result;
  }

  const bool visualRescueTriggered = modeHUpdateVisualRescuePure(
      s, c, nowMs, visualWarningEpoch, visualWarningEnterMs, visualWarningActive);

  if (!visualRescueTriggered &&
      (s.base.phase == MH_IDLE || s.base.phase == MH_PAUSED_STOPPED || !s.base.eventValid)) {
    modeHBeginWaitPure(s, c, nowMs, true);
  }

  // Preserve the established phase scheduler, but generate Rev.4 events and
  // start each ramp from the most recently transmitted carrier value.
  for (uint8_t transitions = 0; transitions < 8u; transitions++) {
    const uint32_t elapsed = (uint32_t)(nowMs - s.base.phaseStartMs);
    uint32_t duration = 0u;
    switch (s.base.phase) {
      case MH_WAIT: duration = s.base.event.waitDurationMs; break;
      case MH_RAMP_IN: duration = s.base.event.rampInDurationMs; break;
      case MH_INTERACT: duration = s.base.event.interactDurationMs; break;
      case MH_RAMP_OUT: duration = s.base.event.rampOutDurationMs; break;
      case MH_REFRACTORY: duration = s.base.event.refractoryDurationMs; break;
      default: duration = 0u; break;
    }
    if (duration == 0u || elapsed < duration) break;

    s.base.phaseStartMs += duration;
    if (s.base.phase == MH_WAIT) {
      s.base.phase = MH_RAMP_IN;
      s.base.eventStartMs = s.base.phaseStartMs;
      s.base.rampBaselineRaw = s.base.outputRaw != 0u ? s.base.outputRaw : sourceRaw;
      s.base.eventCount++;
      if (s.base.event.type == MH_EVENT_CORRECTION) s.base.correctionCount++;
      else s.base.simpleCount++;
    } else if (s.base.phase == MH_RAMP_IN) {
      s.base.phase = MH_INTERACT;
    } else if (s.base.phase == MH_INTERACT) {
      s.base.phase = MH_RAMP_OUT;
      // Refractory uses the Rev.4 opposite-stock carrier. Capture one carrier
      // target at the start of RAMP_OUT so the event decays toward the
      // upcoming carrier regime instead of snapping back to stock and then
      // flipping sign on the next 0x370 frame. Carrier-state frames still
      // re-sample independently on every RX as specified.
      s.rampOutTargetRaw = modeHCarrierOutputPure(s, c, sourceRaw);
      s.rampOutTargetValid = true;
      s.lastCarrierApplied = false;
    } else if (s.base.phase == MH_RAMP_OUT) {
      s.base.phase = MH_REFRACTORY;
      s.rampOutTargetValid = false;
    } else if (s.base.phase == MH_REFRACTORY) {
      modeHBeginWaitPure(s, c, s.base.phaseStartMs, false);
    } else {
      break;
    }
  }

  const uint32_t elapsed = (uint32_t)(nowMs - s.base.phaseStartMs);
  uint16_t raw = sourceRaw;
  bool carrier = false;

  if (s.base.phase == MH_WAIT || s.base.phase == MH_REFRACTORY) {
    raw = modeHCarrierOutputPure(s, c, sourceRaw);
    carrier = true;
  } else if (s.base.phase == MH_RAMP_IN) {
    s.lastCarrierApplied = false;
    const uint32_t p = modeHBaseSmoothstepQ15Pure(
        modeHBaseProgressQ15Pure(elapsed, s.base.event.rampInDurationMs));
    raw = modeHBaseLerpRawPure(
        s.base.rampBaselineRaw, modeHBasePrimaryTargetRawPure(s.base.event), p);
  } else if (s.base.phase == MH_INTERACT) {
    s.lastCarrierApplied = false;
    const uint16_t primary = modeHBasePrimaryTargetRawPure(s.base.event);
    if (s.base.event.type != MH_EVENT_CORRECTION) {
      raw = primary;
    } else {
      const uint32_t correctionStart = (s.base.event.interactDurationMs * 65u) / 100u;
      if (elapsed <= correctionStart || correctionStart >= s.base.event.interactDurationMs) {
        raw = primary;
      } else {
        const uint32_t correctionElapsed = elapsed - correctionStart;
        const uint32_t correctionDuration = s.base.event.interactDurationMs - correctionStart;
        const uint32_t p = modeHBaseSmoothstepQ15Pure(
            modeHBaseProgressQ15Pure(correctionElapsed, correctionDuration));
        raw = modeHBaseLerpRawPure(
            primary, modeHBaseCorrectionTargetRawPure(s.base.event), p);
      }
    }
  } else if (s.base.phase == MH_RAMP_OUT) {
    s.lastCarrierApplied = false;
    const uint32_t p = modeHBaseSmoothstepQ15Pure(
        modeHBaseProgressQ15Pure(elapsed, s.base.event.rampOutDurationMs));
    const uint16_t target = s.rampOutTargetValid ? s.rampOutTargetRaw : sourceRaw;
    raw = modeHBaseLerpRawPure(modeHBaseFinalEventRawPure(s.base.event), target, p);
  }

  s.base.outputRaw = raw;
  s.base.carrier = carrier;
  result.tx = true;
  const ModeHHandsOnHoDecisionPure ho = modeHHandsOnHoDecisionPure(
      c.hoPolicy, modeHHandsOnAbsTorqueRawPure(raw),
      c.ho1ThresholdRaw, c.ho2ThresholdRaw);
  result.hoOverrideValid = ho.overrideValid;
  result.hoLevel = ho.level;
  result.setHo = ho.overrideValid && ho.level == 1u;
  result.carrier = carrier;
  result.raw = raw;
  result.phase = s.base.phase;
  result.motion = s.base.motion;
  result.blockReason = MH_BLOCK_NONE;
  return result;
}

// Persisted value 1 is retained for existing Mode H tuning.
enum NagModeHVariantPure : uint8_t { H_VARIANT_REV4 = 1 };

static inline uint8_t nagModeHDefaultVariantPure() {
  return H_VARIANT_REV4;
}


enum NagModeHStopBehaviorPure : uint8_t {
  H_STOP_HARD_PAUSE = 0,
  H_STOP_STOCK_CARRIER = 1
};

static inline uint8_t nagModeHDefaultStopBehaviorPure() {
  return H_STOP_HARD_PAUSE;
}

static inline bool nagModeHStopBehaviorValidPure(uint8_t v) {
  return v == H_STOP_HARD_PAUSE || v == H_STOP_STOCK_CARRIER;
}

static inline const char* nagModeHStopBehaviorNamePure(uint8_t v) {
  return v == H_STOP_HARD_PAUSE ? "HARD_PAUSE" : "STOCK_CARRIER";
}

static inline bool nagModeHUseStopCarrierPure(uint8_t behavior, bool confirmedStopped) {
  return behavior == H_STOP_STOCK_CARRIER && confirmedStopped;
}

static inline bool nagModeHVariantValidPure(uint8_t v) { return v == H_VARIANT_REV4; }

static inline const char* nagModeHVariantCodePure(uint8_t) { return "H"; }

static inline const char* nagModeHVariantLabelPure(uint8_t) { return "Mode H"; }
