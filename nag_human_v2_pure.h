#pragma once
#include <stdint.h>

// Host-testable Mode H Rev.2 "Natural Grip Peak" engine for v3.6b20.
// No Arduino/ESP-IDF dependencies.
// Tesla EPAS torsion-bar torque uses raw * 0.01 - 20.5 Nm; raw 2050 is 0 Nm.
//
// b20 intentionally keeps this engine narrow:
//   - balanced low Natural Hold
//   - occasional bounded hold excursions
//   - one periodic 1.80–2.20 Nm interaction waveform every 3–6 s
//   - exact 4 negative : 1 positive peak-direction bag
// The failed b18 PRE-WARN/VISUAL reactive branch and its presets/gates are not restored.

static constexpr uint16_t NAG_HUMAN_V2_TORQUE_CENTER_RAW = 2050u;
static constexpr uint16_t NAG_HUMAN_V2_RAW_MAX = 0x0FFFu;

enum NagHumanV2PhasePure : uint8_t {
  H_IDLE = 0,
  H_HOLD = 1,
  H_TAP_ATTACK = 2,
  H_TAP_PEAK = 3,
  H_TAP_RELEASE = 4,
  H_PAUSED_STOPPED = 5
};

enum NagHumanV2EventTypePure : uint8_t {
  H_EVENT_NONE = 0,
  H_EVENT_NATURAL_TAP = 1
};

enum NagHumanV2MotionPure : uint8_t {
  H_MOTION_UNKNOWN = 0,
  H_MOTION_STALE = 1,
  H_MOTION_STOPPED = 2,
  H_MOTION_CONFIRMING = 3,
  H_MOTION_MOVING = 4
};

enum NagHumanV2BlockReasonPure : uint8_t {
  H_BLOCK_NONE = 0,
  H_BLOCK_RUN_GATE = 1,
  H_BLOCK_SPEED_STALE = 2,
  H_BLOCK_STOPPED = 3,
  H_BLOCK_MOVE_CONFIRMING = 4
};

struct NagHumanV2ConfigPure {
  // Natural Hold. Values are magnitudes around the 0 Nm raw center.
  uint16_t holdNormalMinRaw;      // 0.15 Nm
  uint16_t holdNormalMaxRaw;      // 0.45 Nm
  uint16_t holdExcursionMinRaw;   // 0.45 Nm
  uint16_t holdExcursionMaxRaw;   // 0.65 Nm
  uint8_t holdExcursionPct;       // 8%
  uint16_t holdDwellMinMs;        // 0.35 s
  uint16_t holdDwellMaxMs;        // 1.10 s
  uint8_t quickTransitionPct;
  uint16_t smoothTransitionMinMs;
  uint16_t smoothTransitionMaxMs;
  uint16_t quickTransitionMinMs;
  uint16_t quickTransitionMaxMs;

  // HO=1 is always asserted during a strong interaction. A hold transition may
  // also produce a short HO pulse if the torque delta is meaningful.
  uint16_t hoDeltaThresholdRaw;
  uint16_t hoPulseMinMs;
  uint16_t hoPulseMaxMs;

  // Approved b20 Rev.2 periodic interaction.
  uint32_t naturalTapIntervalMinMs; // start-to-start
  uint32_t naturalTapIntervalMaxMs;
  uint16_t naturalTapMinRaw;         // 1.80 Nm
  uint16_t naturalTapMaxRaw;         // 2.20 Nm
  uint16_t tapAttackMinMs;
  uint16_t tapAttackMaxMs;
  uint16_t tapPeakMinMs;
  uint16_t tapPeakMaxMs;
  uint16_t tapReleaseMinMs;
  uint16_t tapReleaseMaxMs;

  int16_t stopThresholdKphX100;
  int16_t moveThresholdKphX100;
  uint16_t moveConfirmMs;
};

struct NagHumanV2EventPure {
  uint8_t type;
  int8_t direction;
  uint16_t peakRaw;
  uint16_t startRaw;
  uint16_t returnRaw;
  uint16_t attackMs;
  uint16_t peakMs;
  uint16_t releaseMs;
};

struct NagHumanV2StatePure {
  uint8_t phase;
  uint8_t motion;
  bool movingConfirmed;
  bool moveCandidateActive;
  uint32_t moveCandidateStartMs;
  uint32_t sessionStartMs;
  bool sessionStarted;
  uint32_t rngState;

  uint16_t outputRaw;
  uint16_t holdStartRaw;
  uint16_t holdTargetRaw;
  bool holdTransitionQuick;
  uint32_t holdTransitionStartMs;
  uint16_t holdTransitionDurationMs;
  uint32_t nextHoldDecisionMs;

  // Strong peak scheduler/direction bag.
  uint32_t nextNaturalTapMs;
  uint8_t naturalTapBagIndex;
  uint8_t naturalTapPositiveSlot;
  uint32_t hoPulseUntilMs;

  NagHumanV2EventPure event;
  bool eventValid;
  uint32_t phaseStartMs;
  uint32_t eventCount;
  uint32_t naturalTapCount;
  bool carrier;
};

struct NagHumanV2StepResultPure {
  bool tx;
  bool setHo;
  bool carrier;
  uint16_t raw;
  uint8_t phase;
  uint8_t motion;
  uint8_t blockReason;
};

static inline NagHumanV2ConfigPure nagHumanV2DefaultConfigPure() {
  NagHumanV2ConfigPure c = {};
  c.holdNormalMinRaw = 15u;
  c.holdNormalMaxRaw = 45u;
  c.holdExcursionMinRaw = 45u;
  c.holdExcursionMaxRaw = 65u;
  c.holdExcursionPct = 8u;
  // Match the approved representative Rev.2 plots: low hold plateaus generally
  // last about 0.35–1.10 s before the next target is chosen.
  c.holdDwellMinMs = 350u;
  c.holdDwellMaxMs = 1100u;
  c.quickTransitionPct = 35u;
  c.smoothTransitionMinMs = 80u;
  c.smoothTransitionMaxMs = 180u;
  c.quickTransitionMinMs = 30u;
  c.quickTransitionMaxMs = 80u;

  c.hoDeltaThresholdRaw = 55u;
  c.hoPulseMinMs = 120u;
  c.hoPulseMaxMs = 220u;

  c.naturalTapIntervalMinMs = 3000u;
  c.naturalTapIntervalMaxMs = 6000u;
  c.naturalTapMinRaw = 180u;
  c.naturalTapMaxRaw = 220u;

  // Total waveform min/max = 80+80+170 = 330 ms / 150+180+370 = 700 ms.
  c.tapAttackMinMs = 80u;
  c.tapAttackMaxMs = 150u;
  c.tapPeakMinMs = 80u;
  c.tapPeakMaxMs = 180u;
  c.tapReleaseMinMs = 170u;
  c.tapReleaseMaxMs = 370u;

  c.stopThresholdKphX100 = 16;
  c.moveThresholdKphX100 = 64;
  c.moveConfirmMs = 300u;
  return c;
}

static inline uint32_t nagHumanV2SanitizeSeedPure(uint32_t seed) {
  return seed == 0u ? 0x6D2B79F5u : seed;
}

static inline uint32_t nagHumanV2NextRandomPure(uint32_t &state) {
  uint32_t x = nagHumanV2SanitizeSeedPure(state);
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = nagHumanV2SanitizeSeedPure(x);
  return state;
}

static inline uint32_t nagHumanV2RangePure(uint32_t &state, uint32_t lo, uint32_t hi) {
  if (hi < lo) { const uint32_t t = lo; lo = hi; hi = t; }
  const uint32_t span = hi - lo + 1u;
  return lo + (span == 0u ? 0u : (nagHumanV2NextRandomPure(state) % span));
}

static inline uint16_t nagHumanV2ClampRawPure(int32_t raw) {
  if (raw < 0) return 0u;
  if (raw > (int32_t)NAG_HUMAN_V2_RAW_MAX) return NAG_HUMAN_V2_RAW_MAX;
  return (uint16_t)raw;
}

static inline uint16_t nagHumanV2SignedTargetPure(int8_t direction, uint16_t magnitudeRaw) {
  return nagHumanV2ClampRawPure((int32_t)NAG_HUMAN_V2_TORQUE_CENTER_RAW +
                               (int32_t)direction * (int32_t)magnitudeRaw);
}

static inline uint32_t nagHumanV2ProgressQ15Pure(uint32_t elapsed, uint32_t duration) {
  if (duration == 0u || elapsed >= duration) return 32768u;
  return (uint32_t)(((uint64_t)elapsed * 32768u) / duration);
}

static inline uint32_t nagHumanV2SmoothstepQ15Pure(uint32_t x) {
  if (x >= 32768u) return 32768u;
  const uint64_t x2 = ((uint64_t)x * x + 16384u) >> 15;
  const uint32_t threeMinusTwoX = 98304u - 2u * x;
  const uint64_t y = (x2 * threeMinusTwoX + 16384u) >> 15;
  return (uint32_t)(y > 32768u ? 32768u : y);
}

static inline uint16_t nagHumanV2LerpRawPure(uint16_t a, uint16_t b, uint32_t q15) {
  if (q15 >= 32768u) return b;
  const int32_t delta = (int32_t)b - (int32_t)a;
  const int64_t scaled = (int64_t)delta * (int64_t)q15;
  const int32_t rounded = scaled >= 0
      ? (int32_t)((scaled + 16384) / 32768)
      : (int32_t)((scaled - 16384) / 32768);
  return nagHumanV2ClampRawPure((int32_t)a + rounded);
}

static inline uint16_t nagHumanV2AbsDeltaRawPure(uint16_t a, uint16_t b) {
  return a > b ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static inline void nagHumanV2InitPure(NagHumanV2StatePure &s, uint32_t seed) {
  s = NagHumanV2StatePure{};
  s.rngState = nagHumanV2SanitizeSeedPure(seed);
  s.phase = H_IDLE;
  s.motion = H_MOTION_UNKNOWN;
  s.outputRaw = NAG_HUMAN_V2_TORQUE_CENTER_RAW;
  s.holdStartRaw = NAG_HUMAN_V2_TORQUE_CENTER_RAW;
  s.holdTargetRaw = NAG_HUMAN_V2_TORQUE_CENTER_RAW;
  s.naturalTapBagIndex = 5u; // force a fresh randomized 4:1 bag on first peak
}

static inline void nagHumanV2ResetRuntimePure(NagHumanV2StatePure &s, uint8_t phase) {
  const uint32_t rng = s.rngState;
  const uint32_t events = s.eventCount;
  const uint32_t natural = s.naturalTapCount;
  nagHumanV2InitPure(s, rng);
  s.phase = phase;
  s.motion = phase == H_PAUSED_STOPPED ? H_MOTION_STOPPED : H_MOTION_UNKNOWN;
  s.eventCount = events;
  s.naturalTapCount = natural;
}

static inline void nagHumanV2ReseedPure(NagHumanV2StatePure &s, uint32_t seed) {
  const uint32_t events = s.eventCount;
  const uint32_t natural = s.naturalTapCount;
  nagHumanV2InitPure(s, seed);
  s.eventCount = events;
  s.naturalTapCount = natural;
}

static inline void nagHumanV2PausePure(NagHumanV2StatePure &s, uint8_t motion) {
  const uint32_t rng = s.rngState;
  const uint32_t events = s.eventCount;
  const uint32_t natural = s.naturalTapCount;
  nagHumanV2InitPure(s, rng);
  s.phase = H_PAUSED_STOPPED;
  s.motion = motion;
  s.eventCount = events;
  s.naturalTapCount = natural;
}

static inline bool nagHumanV2MotionAllowsPure(NagHumanV2StatePure &s,
                                               const NagHumanV2ConfigPure &c,
                                               uint32_t nowMs,
                                               bool speedValid,
                                               bool speedFresh,
                                               uint16_t speedRaw,
                                               uint8_t &blockReason) {
  if (!speedValid || !speedFresh) {
    nagHumanV2PausePure(s, H_MOTION_STALE);
    blockReason = H_BLOCK_SPEED_STALE;
    return false;
  }
  const int32_t speedKphX100 = (int32_t)speedRaw * 8 - 4000;
  if (s.movingConfirmed) {
    if (speedKphX100 <= c.stopThresholdKphX100) {
      nagHumanV2PausePure(s, H_MOTION_STOPPED);
      blockReason = H_BLOCK_STOPPED;
      return false;
    }
    s.motion = H_MOTION_MOVING;
    blockReason = H_BLOCK_NONE;
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
      s.motion = H_MOTION_MOVING;
      s.phase = H_IDLE;
      blockReason = H_BLOCK_NONE;
      return true;
    }
    s.phase = H_PAUSED_STOPPED;
    s.motion = H_MOTION_CONFIRMING;
    blockReason = H_BLOCK_MOVE_CONFIRMING;
    return false;
  }
  s.moveCandidateActive = false;
  s.moveCandidateStartMs = 0u;
  s.phase = H_PAUSED_STOPPED;
  s.motion = H_MOTION_STOPPED;
  blockReason = H_BLOCK_STOPPED;
  return false;
}

static inline int8_t nagHumanV2RandomDirectionPure(NagHumanV2StatePure &s) {
  return (nagHumanV2NextRandomPure(s.rngState) & 1u) ? 1 : -1;
}

// Every completed five-peak block is exactly 4 negative + 1 positive. The
// positive slot is randomized at block creation, preserving randomness while
// bounding long-run direction bias at the requested 80/20 ratio.
static inline int8_t nagHumanV2NextPeakDirectionPure(NagHumanV2StatePure &s) {
  if (s.naturalTapBagIndex >= 5u) {
    s.naturalTapPositiveSlot = (uint8_t)nagHumanV2RangePure(s.rngState, 0u, 4u);
    s.naturalTapBagIndex = 0u;
  }
  const int8_t direction = s.naturalTapBagIndex == s.naturalTapPositiveSlot ? 1 : -1;
  s.naturalTapBagIndex++;
  return direction;
}

static inline void nagHumanV2ScheduleNextPeakPure(NagHumanV2StatePure &s,
                                                   const NagHumanV2ConfigPure &c,
                                                   uint32_t fromStartMs) {
  s.nextNaturalTapMs = fromStartMs + nagHumanV2RangePure(
      s.rngState, c.naturalTapIntervalMinMs, c.naturalTapIntervalMaxMs);
}

static inline void nagHumanV2StartHoldSessionPure(NagHumanV2StatePure &s,
                                                   const NagHumanV2ConfigPure &c,
                                                   uint32_t nowMs,
                                                   uint16_t sourceRaw) {
  s.phase = H_HOLD;
  s.motion = H_MOTION_MOVING;
  s.sessionStartMs = nowMs;
  s.sessionStarted = true;
  s.outputRaw = sourceRaw;
  s.holdStartRaw = sourceRaw;
  s.holdTargetRaw = sourceRaw;
  s.nextHoldDecisionMs = nowMs + nagHumanV2RangePure(s.rngState, c.holdDwellMinMs, c.holdDwellMaxMs);
  nagHumanV2ScheduleNextPeakPure(s, c, nowMs);
  s.carrier = true;
}

static inline void nagHumanV2ChooseHoldTargetPure(NagHumanV2StatePure &s,
                                                   const NagHumanV2ConfigPure &c,
                                                   uint32_t nowMs) {
  const bool excursion = nagHumanV2RangePure(s.rngState, 0u, 99u) < c.holdExcursionPct;
  const uint16_t magnitude = (uint16_t)nagHumanV2RangePure(
      s.rngState,
      excursion ? c.holdExcursionMinRaw : c.holdNormalMinRaw,
      excursion ? c.holdExcursionMaxRaw : c.holdNormalMaxRaw);
  // Hold direction remains unbiased; only the strong peak bag is 80/20.
  const int8_t direction = nagHumanV2RandomDirectionPure(s);
  const uint16_t target = nagHumanV2SignedTargetPure(direction, magnitude);

  s.holdStartRaw = s.outputRaw;
  s.holdTargetRaw = target;
  s.holdTransitionStartMs = nowMs;
  s.holdTransitionQuick = nagHumanV2RangePure(s.rngState, 0u, 99u) < c.quickTransitionPct;
  s.holdTransitionDurationMs = (uint16_t)nagHumanV2RangePure(
      s.rngState,
      s.holdTransitionQuick ? c.quickTransitionMinMs : c.smoothTransitionMinMs,
      s.holdTransitionQuick ? c.quickTransitionMaxMs : c.smoothTransitionMaxMs);
  s.nextHoldDecisionMs = nowMs + nagHumanV2RangePure(s.rngState, c.holdDwellMinMs, c.holdDwellMaxMs);

  if (nagHumanV2AbsDeltaRawPure(s.holdStartRaw, target) >= c.hoDeltaThresholdRaw) {
    s.hoPulseUntilMs = nowMs + nagHumanV2RangePure(s.rngState, c.hoPulseMinMs, c.hoPulseMaxMs);
  }
}

static inline uint16_t nagHumanV2CurrentHoldRawPure(const NagHumanV2StatePure &s,
                                                     uint32_t nowMs) {
  if (s.holdTransitionDurationMs == 0u) return s.holdTargetRaw;
  const uint32_t elapsed = (uint32_t)(nowMs - s.holdTransitionStartMs);
  if (elapsed >= s.holdTransitionDurationMs) return s.holdTargetRaw;
  const uint32_t p0 = nagHumanV2ProgressQ15Pure(elapsed, s.holdTransitionDurationMs);
  const uint32_t p = s.holdTransitionQuick ? p0 : nagHumanV2SmoothstepQ15Pure(p0);
  return nagHumanV2LerpRawPure(s.holdStartRaw, s.holdTargetRaw, p);
}

static inline void nagHumanV2StartPeakPure(NagHumanV2StatePure &s,
                                            const NagHumanV2ConfigPure &c,
                                            uint32_t nowMs) {
  s.event = NagHumanV2EventPure{};
  s.event.type = H_EVENT_NATURAL_TAP;
  s.event.direction = nagHumanV2NextPeakDirectionPure(s);
  s.event.peakRaw = (uint16_t)nagHumanV2RangePure(s.rngState, c.naturalTapMinRaw, c.naturalTapMaxRaw);
  s.event.startRaw = s.outputRaw;
  s.event.returnRaw = nagHumanV2CurrentHoldRawPure(s, nowMs);
  s.event.attackMs = (uint16_t)nagHumanV2RangePure(s.rngState, c.tapAttackMinMs, c.tapAttackMaxMs);
  s.event.peakMs = (uint16_t)nagHumanV2RangePure(s.rngState, c.tapPeakMinMs, c.tapPeakMaxMs);
  s.event.releaseMs = (uint16_t)nagHumanV2RangePure(s.rngState, c.tapReleaseMinMs, c.tapReleaseMaxMs);
  s.eventValid = true;
  s.phase = H_TAP_ATTACK;
  s.phaseStartMs = nowMs;
  s.eventCount++;
  s.naturalTapCount++;
  s.carrier = false;
  // Approved cadence is peak-start to peak-start, exactly as shown in the
  // comparison plots. Waveform duration therefore does not extend 3–6 s.
  nagHumanV2ScheduleNextPeakPure(s, c, nowMs);
}

static inline void nagHumanV2FinishPeakPure(NagHumanV2StatePure &s,
                                             const NagHumanV2ConfigPure &c,
                                             uint32_t nowMs) {
  s.outputRaw = s.event.returnRaw;
  s.holdStartRaw = s.outputRaw;
  s.holdTargetRaw = s.outputRaw;
  s.holdTransitionStartMs = nowMs;
  s.holdTransitionDurationMs = 0u;
  s.nextHoldDecisionMs = nowMs + nagHumanV2RangePure(s.rngState, c.holdDwellMinMs, c.holdDwellMaxMs);
  s.phase = H_HOLD;
  s.eventValid = false;
  s.event = NagHumanV2EventPure{};
  s.carrier = true;
}

static inline const char* nagHumanV2PhaseNamePure(uint8_t phase) {
  switch (phase) {
    case H_HOLD: return "NATURAL_HOLD";
    case H_TAP_ATTACK: return "PEAK_ATTACK";
    case H_TAP_PEAK: return "PEAK_HOLD";
    case H_TAP_RELEASE: return "PEAK_RELEASE";
    case H_PAUSED_STOPPED: return "PAUSED_STOPPED";
    default: return "IDLE";
  }
}

static inline const char* nagHumanV2EventTypeNamePure(uint8_t type) {
  return type == H_EVENT_NATURAL_TAP ? "INTERACTION_PEAK" : "NONE";
}

static inline const char* nagHumanV2MotionNamePure(uint8_t motion) {
  switch (motion) {
    case H_MOTION_STALE: return "STALE";
    case H_MOTION_STOPPED: return "STOPPED";
    case H_MOTION_CONFIRMING: return "CONFIRMING";
    case H_MOTION_MOVING: return "MOVING";
    default: return "UNKNOWN";
  }
}

static inline NagHumanV2StepResultPure nagHumanV2StepPure(
    NagHumanV2StatePure &s,
    const NagHumanV2ConfigPure &c,
    uint32_t nowMs,
    uint16_t sourceRaw,
    bool runAllowed,
    bool speedValid,
    bool speedFresh,
    uint16_t speedRaw) {
  NagHumanV2StepResultPure result = {};
  result.raw = sourceRaw;
  result.phase = s.phase;
  result.motion = s.motion;

  if (!runAllowed) {
    nagHumanV2ResetRuntimePure(s, H_IDLE);
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = H_BLOCK_RUN_GATE;
    return result;
  }

  uint8_t motionBlock = H_BLOCK_NONE;
  if (!nagHumanV2MotionAllowsPure(s, c, nowMs, speedValid, speedFresh, speedRaw, motionBlock)) {
    result.phase = s.phase;
    result.motion = s.motion;
    result.blockReason = motionBlock;
    return result;
  }

  if (s.phase == H_IDLE || s.phase == H_PAUSED_STOPPED || !s.sessionStarted) {
    nagHumanV2StartHoldSessionPure(s, c, nowMs, sourceRaw);
  }

  if (!s.eventValid && s.phase == H_HOLD) {
    if ((int32_t)(nowMs - s.nextHoldDecisionMs) >= 0) {
      nagHumanV2ChooseHoldTargetPure(s, c, nowMs);
    }
    s.outputRaw = nagHumanV2CurrentHoldRawPure(s, nowMs);
    if ((int32_t)(nowMs - s.nextNaturalTapMs) >= 0) {
      nagHumanV2StartPeakPure(s, c, nowMs);
    }
  }

  // Advance phase boundaries using the scheduled edge, not the delayed task
  // timestamp, so scheduler jitter does not accumulate inside one waveform.
  for (uint8_t transitions = 0; transitions < 4u && s.eventValid; transitions++) {
    const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
    uint32_t duration = 0u;
    if (s.phase == H_TAP_ATTACK) duration = s.event.attackMs;
    else if (s.phase == H_TAP_PEAK) duration = s.event.peakMs;
    else if (s.phase == H_TAP_RELEASE) duration = s.event.releaseMs;
    else break;
    if (duration != 0u && elapsed < duration) break;
    s.phaseStartMs += duration;
    if (s.phase == H_TAP_ATTACK) s.phase = H_TAP_PEAK;
    else if (s.phase == H_TAP_PEAK) s.phase = H_TAP_RELEASE;
    else {
      nagHumanV2FinishPeakPure(s, c, s.phaseStartMs);
      break;
    }
  }

  if (s.eventValid) {
    const uint16_t peakTarget = nagHumanV2SignedTargetPure(s.event.direction, s.event.peakRaw);
    const uint32_t elapsed = (uint32_t)(nowMs - s.phaseStartMs);
    if (s.phase == H_TAP_ATTACK) {
      const uint32_t p = nagHumanV2SmoothstepQ15Pure(
          nagHumanV2ProgressQ15Pure(elapsed, s.event.attackMs));
      s.outputRaw = nagHumanV2LerpRawPure(s.event.startRaw, peakTarget, p);
    } else if (s.phase == H_TAP_PEAK) {
      s.outputRaw = peakTarget;
    } else if (s.phase == H_TAP_RELEASE) {
      const uint32_t p = nagHumanV2SmoothstepQ15Pure(
          nagHumanV2ProgressQ15Pure(elapsed, s.event.releaseMs));
      s.outputRaw = nagHumanV2LerpRawPure(peakTarget, s.event.returnRaw, p);
    }
  } else if (s.phase == H_HOLD) {
    s.outputRaw = nagHumanV2CurrentHoldRawPure(s, nowMs);
  }

  s.carrier = !s.eventValid;
  result.tx = true;
  result.setHo = s.eventValid ||
      (s.hoPulseUntilMs != 0u && (int32_t)(s.hoPulseUntilMs - nowMs) > 0);
  result.carrier = s.carrier;
  result.raw = s.outputRaw;
  result.phase = s.phase;
  result.motion = s.motion;
  result.blockReason = H_BLOCK_NONE;
  return result;
}
