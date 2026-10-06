#pragma once
#include <stdint.h>
#include "vehicle_profile.h"

// Host-testable Summon routing and state decisions. Physical bus A is the
// MCP2515 side (Party or Body by topology); physical bus B is CHASSIS
// (VH or Chassis by topology).
enum SummonBusMaskPure : uint8_t {
  SUMMON_BUS_NONE = 0x00,
  SUMMON_BUS_A = 0x01,
  SUMMON_BUS_B = 0x02,
  SUMMON_BUS_BOTH = SUMMON_BUS_A | SUMMON_BUS_B
};

struct SummonRoutePure {
  bool valid;
  uint8_t gearBusMask;       // 0x118 primary and optional 0x186 fallback
  uint8_t dasBusMask;        // 0x399 AP/DAS state
  uint8_t sprBusMask;        // 0x3F8 SPR
  uint8_t transportBusMask;  // 0x3FD R79 transport/template
  uint8_t requiredTxFreshMask;
  bool allow186Fallback;
};

static inline SummonRoutePure summonRoutePure(uint8_t profileId, uint8_t topology) {
  SummonRoutePure r = {};
  if (!vehicleProfileTopologyValid(profileId, topology)) return r;

  // TMR has three physical buses. This legacy two-mask model keeps A/B as
  // logical feature lanes: PARTY for Model Y L, CHASSIS for standard 3/Y.
  // The third physical bus (BODY) is handled by the TMR CAN runtime.
  r.valid = true;
  if (profileId == VEHICLE_MODEL_YL) {
    r.gearBusMask = SUMMON_BUS_A;       // logical PARTY
    r.dasBusMask = SUMMON_BUS_A;        // logical PARTY
    r.sprBusMask = SUMMON_BUS_B;        // logical BODY/VH
    r.transportBusMask = SUMMON_BUS_B;  // logical BODY/VH
    r.requiredTxFreshMask = SUMMON_BUS_B;
    r.allow186Fallback = false;
  } else {
    r.gearBusMask = SUMMON_BUS_B;       // CHASSIS
    r.dasBusMask = SUMMON_BUS_B;        // CHASSIS
    r.sprBusMask = SUMMON_BUS_B;        // CHASSIS
    r.transportBusMask = SUMMON_BUS_B;  // CHASSIS
    r.requiredTxFreshMask = SUMMON_BUS_B;
    r.allow186Fallback = true;
  }
  return r;
}

struct SummonInvalidationPure {
  bool invalidateGear;
  bool invalidateDas;
  bool invalidateSpr;
  bool invalidateTemplate;
};

static inline SummonInvalidationPure summonInvalidationPure(
    const SummonRoutePure &route, uint8_t invalidatedBusMask) {
  SummonInvalidationPure r = {};
  if (!route.valid) {
    r.invalidateGear = true;
    r.invalidateDas = true;
    r.invalidateSpr = true;
    r.invalidateTemplate = true;
    return r;
  }
  r.invalidateGear = (route.gearBusMask & invalidatedBusMask) != 0;
  r.invalidateDas = (route.dasBusMask & invalidatedBusMask) != 0;
  r.invalidateSpr = (route.sprBusMask & invalidatedBusMask) != 0;
  r.invalidateTemplate = (route.transportBusMask & invalidatedBusMask) != 0;
  return r;
}

static inline bool summonAgeFreshPure(uint32_t now, uint32_t observedMs,
                                      uint32_t timeoutMs) {
  return observedMs != 0 && (uint32_t)(now - observedMs) <= timeoutMs;
}

enum SummonGearSourcePure : uint8_t {
  SUMMON_GEAR_NONE = 0,
  SUMMON_GEAR_118 = 1,
  SUMMON_GEAR_186 = 2
};

struct SummonGearObservationPure {
  int8_t state;      // 1=PARK, 0=non-PARK, -1=unknown
  uint32_t observedMs;
  bool valid;
};

struct SummonGearDecisionPure {
  bool valid;
  bool parked;
  uint8_t source;
  uint32_t observedMs;
};

enum SummonConfirmedGearStatePure : uint8_t {
  SUMMON_CONFIRMED_GEAR_UNKNOWN = 0,
  SUMMON_CONFIRMED_GEAR_PARK = 1,
  SUMMON_CONFIRMED_GEAR_NON_PARK = 2
};

struct SummonConfirmedGearLatchPure {
  uint8_t state;
  uint8_t source;
  uint32_t observedMs;
};

// Apply only a real, decoded gear decision. An invalid/stale decision preserves
// the last confirmed state, so silence can never turn D/R/N back into PARK.
// Returns true only when the confirmed PARK/NON-PARK state changes.
static inline bool summonGearLatchApplyDecisionPure(
    SummonConfirmedGearLatchPure &latch,
    const SummonGearDecisionPure &decision) {
  if (!decision.valid) return false;
  const uint8_t nextState = decision.parked
      ? SUMMON_CONFIRMED_GEAR_PARK
      : SUMMON_CONFIRMED_GEAR_NON_PARK;
  const bool changed = latch.state != nextState;
  latch.state = nextState;
  latch.source = decision.source;
  latch.observedMs = decision.observedMs;
  return changed;
}

static inline SummonGearDecisionPure summonFreshGearPure(
    uint32_t now, const SummonGearObservationPure &gear118,
    const SummonGearObservationPure &gear186, bool allow186Fallback,
    uint32_t freshnessMs) {
  SummonGearDecisionPure r = {};
  r.source = SUMMON_GEAR_NONE;

  // 0x118 is the primary source. 0x186 is fallback only; it must never
  // override a still-fresh valid 0x118 observation just because it arrived later.
  if (gear118.valid && gear118.state >= 0 &&
      summonAgeFreshPure(now, gear118.observedMs, freshnessMs)) {
    r.valid = true;
    r.parked = gear118.state == 1;
    r.source = SUMMON_GEAR_118;
    r.observedMs = gear118.observedMs;
    return r;
  }

  if (allow186Fallback && gear186.valid && gear186.state >= 0 &&
      summonAgeFreshPure(now, gear186.observedMs, freshnessMs)) {
    r.valid = true;
    r.parked = gear186.state == 1;
    r.source = SUMMON_GEAR_186;
    r.observedMs = gear186.observedMs;
  }
  return r;
}


// v3.6 R79 policy: injection is default-on after a stock template exists.
// It is suspended only when manual driving is positively confirmed in Drive
// or Reverse.
enum TeslaGearRawPure : uint8_t {
  TESLA_GEAR_INVALID = 0,
  TESLA_GEAR_P = 1,
  TESLA_GEAR_R = 2,
  TESLA_GEAR_N = 3,
  TESLA_GEAR_D = 4,
  TESLA_GEAR_SNA = 7
};

// CAN-B transport priority used by the v3.6d2 R79 hardening path.
// NORMAL has no queue reservation. PARK_STANDBY is entered only from a fresh,
// decoded real PARK observation; the legacy 0x118-stale PARK compatibility
// fallback is intentionally not an input here. Remote startup evidence upgrades
// to READY before the sticky ACA+SPR Summon session reaches ACTIVE.
enum SummonTxPriorityStatePure : uint8_t {
  SUMMON_PRIORITY_NORMAL = 0,
  SUMMON_PRIORITY_PARK_STANDBY = 1,
  SUMMON_PRIORITY_READY = 2,
  SUMMON_PRIORITY_ACTIVE = 3
};

static inline uint8_t summonTxPriorityStatePure(
    bool freshGearValid, uint8_t freshGearRaw,
    bool remoteStartupEvidence, bool summonConfirmed) {
  if (summonConfirmed) return SUMMON_PRIORITY_ACTIVE;
  if (remoteStartupEvidence) return SUMMON_PRIORITY_READY;
  if (freshGearValid && freshGearRaw == TESLA_GEAR_P)
    return SUMMON_PRIORITY_PARK_STANDBY;
  return SUMMON_PRIORITY_NORMAL;
}

static inline bool summonPriorityAllowsR79FlushPure(uint8_t state) {
  return state == SUMMON_PRIORITY_READY || state == SUMMON_PRIORITY_ACTIVE;
}

static inline bool summonPriorityNonR79AdmissionPure(
    uint8_t state, uint32_t queued, uint32_t parkLimit, uint32_t summonLimit) {
  if (state == SUMMON_PRIORITY_PARK_STANDBY) return queued < parkLimit;
  if (state == SUMMON_PRIORITY_READY || state == SUMMON_PRIORITY_ACTIVE)
    return queued < summonLimit;
  return true;
}

static inline uint16_t r79RetryDelayMsPure(uint8_t retryIndex) {
  switch (retryIndex) {
    case 0: return 5;
    case 1: return 15;
    case 2: return 30;
    default: return 0;
  }
}


enum R79TxReasonPure : uint8_t {
  R79_TX_REASON_DEFAULT = 0,
  R79_TX_REASON_SUMMON = 1,
  R79_TX_REASON_AUTOPILOT = 2,
  R79_TX_REASON_MANUAL_D = 3,
  R79_TX_REASON_MANUAL_R = 4
};

struct R79ManualSuppressionPure {
  bool active;
  uint8_t gearRaw;
};

// Manual suppression is hysteretic. A definite D/R + manual DAS observation
// can enter suppression only when no fresh remote-start evidence is present.
// Once positively confirmed, transient unknown/stale DAS or gear observations
// do not bounce R79 ACTIVE. AP, a confirmed Summon session, or explicit P/N
// exits suppression immediately.
static inline void r79ManualSuppressionUpdatePure(
    R79ManualSuppressionPure &state,
    bool gearValid, uint8_t gearRaw,
    bool dasValid, bool apActive, bool manualState,
    bool summonConfirmed, bool remoteStartupEvidence) {
  if (apActive || summonConfirmed) {
    state.active = false;
    state.gearRaw = TESLA_GEAR_INVALID;
    return;
  }

  if (gearValid && (gearRaw == TESLA_GEAR_P || gearRaw == TESLA_GEAR_N)) {
    state.active = false;
    state.gearRaw = TESLA_GEAR_INVALID;
    return;
  }

  if (gearValid && (gearRaw == TESLA_GEAR_D || gearRaw == TESLA_GEAR_R)) {
    if (state.active) {
      state.gearRaw = gearRaw;
      return;
    }
    if (dasValid && manualState && !remoteStartupEvidence) {
      state.active = true;
      state.gearRaw = gearRaw;
    }
  }
}

struct R79TxDecisionPure {
  bool txEnabled;
  bool manualSuppressed;
  uint8_t reason;
};

static inline R79TxDecisionPure r79TxDecisionPure(
    bool apActive, bool summonConfirmed,
    const R79ManualSuppressionPure &manual, bool allowManualDriving = false) {
  R79TxDecisionPure r = {true, false, R79_TX_REASON_DEFAULT};
  // AP reason wins if both flags are ever simultaneously present. This keeps
  // AUTOSTEER/NOA telemetry honest and prevents stale Summon evidence from
  // labeling an active AP session as SUMMON.
  if (apActive) {
    r.reason = R79_TX_REASON_AUTOPILOT;
    return r;
  }
  if (summonConfirmed) {
    r.reason = R79_TX_REASON_SUMMON;
    return r;
  }
  if (manual.active && !allowManualDriving) {
    r.txEnabled = false;
    r.manualSuppressed = true;
    r.reason = manual.gearRaw == TESLA_GEAR_R
        ? R79_TX_REASON_MANUAL_R
        : R79_TX_REASON_MANUAL_D;
  }
  return r;
}

static inline const char *r79TxReasonNamePure(uint8_t reason) {
  switch (reason) {
    case R79_TX_REASON_SUMMON: return "SUMMON";
    case R79_TX_REASON_AUTOPILOT: return "AUTOPILOT";
    case R79_TX_REASON_MANUAL_D: return "MANUAL D";
    case R79_TX_REASON_MANUAL_R: return "MANUAL R";
    default: return "DEFAULT";
  }
}


// Summon Monitor V2.6-compatible state model.
// This preserves the legacy Park/Summon/AP telemetry and monitor semantics.
// v3.6 R79 TX no longer uses this positive authorization as its transmit gate:
//   * boot begins PARK-open;
//   * 0x118 P opens, D/R/N closes;
//   * after 0x118 silence > timeout, PARK opens again;
//   * 0x186 may update gear only when 0x118 is absent/stale and the profile
//     explicitly validates that fallback;
//   * any non-zero SPR latches until ACA drops or PARK is seen with ACA inactive;
//   * AP state remains separate from the confirmed Summon RX session.
struct SummonV26CompatStatePure {
  bool parked;
  bool summoning;
  bool acaActive;
  bool sprSeen;
  uint32_t last118Ms;
};


static inline void summonV26CompatRecomputeSessionPure(
    SummonV26CompatStatePure &s) {
  s.summoning = s.acaActive && s.sprSeen;
}

static inline void summonV26CompatApply118Pure(
    SummonV26CompatStatePure &s, int8_t gearState,
    bool acaActive, uint32_t now) {
  const bool oldAca = s.acaActive;
  s.last118Ms = now;

  if (gearState == 1) s.parked = true;
  else if (gearState == 0) s.parked = false;

  if (oldAca && !acaActive) s.sprSeen = false;
  s.acaActive = acaActive;
  summonV26CompatRecomputeSessionPure(s);

  // Legacy V2.6 clears a completed Summon latch when the vehicle is PARK and
  // ACA is no longer active.
  if (gearState == 1 && !s.acaActive) {
    s.summoning = false;
    s.sprSeen = false;
  }
}

static inline void summonV26CompatApply186Pure(
    SummonV26CompatStatePure &s, int8_t gearState, uint32_t now,
    uint32_t timeoutMs, bool allowFallback) {
  if (!allowFallback || gearState < 0) return;
  const bool primaryAbsentOrStale =
      s.last118Ms == 0 || (uint32_t)(now - s.last118Ms) > timeoutMs;
  if (!primaryAbsentOrStale) return;

  s.parked = gearState == 1;
  if (gearState == 1 && !s.acaActive) {
    s.summoning = false;
    s.sprSeen = false;
  }
}

static inline void summonV26CompatApplySprPure(
    SummonV26CompatStatePure &s, uint8_t sprRaw) {
  if (sprRaw != 0) s.sprSeen = true;
  summonV26CompatRecomputeSessionPure(s);
}

static inline void summonV26CompatTickPure(
    SummonV26CompatStatePure &s, uint32_t now, uint32_t timeoutMs) {
  if (s.last118Ms != 0 && (uint32_t)(now - s.last118Ms) > timeoutMs)
    s.parked = true;
  summonV26CompatRecomputeSessionPure(s);
}




static inline const char *summonGearSourceNamePure(uint8_t source) {
  switch (source) {
    case SUMMON_GEAR_118: return "0x118";
    case SUMMON_GEAR_186: return "0x186";
    default: return "NONE";
  }
}

static inline const char *summonConfirmedGearNamePure(uint8_t state) {
  switch (state) {
    case SUMMON_CONFIRMED_GEAR_PARK: return "PARK";
    case SUMMON_CONFIRMED_GEAR_NON_PARK: return "NON_PARK";
    default: return "UNKNOWN";
  }
}
