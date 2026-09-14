#pragma once
#include <stdint.h>
#include "vehicle_profile.h"

// Host-testable Summon routing and state decisions. Physical bus A is the
// MCP2515 side (Party or Body by topology); physical bus B is TWAI
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

  if (profileId == VEHICLE_MODEL_YL && topology == VEHICLE_TOPOLOGY_YL_PARTY_VH) {
    r.valid = true;
    r.gearBusMask = SUMMON_BUS_A;
    r.dasBusMask = SUMMON_BUS_A;
    r.sprBusMask = SUMMON_BUS_B;
    r.transportBusMask = SUMMON_BUS_B;
    // V2.6 compatibility transport: only the bus carrying stock 0x3FD must
    // be fresh when R79 is emitted. Party remains the source of gear/AP state.
    r.requiredTxFreshMask = SUMMON_BUS_B;
    // The YL 0x186 samples captured so far do not validate the generic
    // data[2] gear mapping. Keep 0x118 authoritative until separately proven.
    r.allow186Fallback = false;
    return r;
  }

  if (profileId != VEHICLE_MODEL_YL &&
      (topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS ||
       topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS)) {
    r.valid = true;
    r.gearBusMask = SUMMON_BUS_B;
    r.dasBusMask = SUMMON_BUS_B;
    r.sprBusMask = SUMMON_BUS_B;
    r.transportBusMask = SUMMON_BUS_B;
    // Standard 3/Y Summon is self-contained on Chassis CAN. Body/Party CAN A
    // must not block remote-Summon R79 transport while it is still asleep.
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

static inline bool summonSessionConfirmedPure(
    uint32_t now,
    bool acaValid, bool acaActive, uint32_t acaObservedMs, uint32_t acaFreshMs,
    bool sprValid, bool sprActive, uint32_t sprObservedMs, uint32_t sprFreshMs) {
  return acaValid && acaActive &&
         summonAgeFreshPure(now, acaObservedMs, acaFreshMs) &&
         sprValid && sprActive &&
         summonAgeFreshPure(now, sprObservedMs, sprFreshMs);
}

enum SummonAuthorizationPure : uint8_t {
  SUMMON_AUTH_NONE = 0,
  SUMMON_AUTH_PARK = 1,             // current fresh P observation
  SUMMON_AUTH_AP = 2,
  SUMMON_AUTH_SUMMON = 3,
  SUMMON_AUTH_PARK_LATCHED = 4,     // last real decoded gear was P
  SUMMON_AUTH_REMOTE_FALLBACK = 5   // no decoded gear has ever been seen this boot
};


// Summon-Unlock V2.6 compatibility gate.
// This intentionally preserves the permissive legacy semantics requested for
// reliable remote Summon operation:
//   * boot begins PARK-open;
//   * 0x118 P opens, D/R/N closes;
//   * after 0x118 silence > timeout, PARK opens again;
//   * 0x186 may update gear only when 0x118 is absent/stale and the profile
//     explicitly validates that fallback;
//   * any non-zero SPR latches until ACA drops or PARK is seen with ACA inactive;
//   * AP independently authorizes R79 even while gear is D/R/N.
struct SummonV26CompatStatePure {
  bool parked;
  bool summoning;
  bool acaActive;
  bool sprSeen;
  uint32_t last118Ms;
};

static inline SummonV26CompatStatePure summonV26CompatInitialPure() {
  SummonV26CompatStatePure s = {};
  s.parked = true;
  return s;
}

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

static inline uint8_t summonV26CompatAuthorizationPure(
    const SummonV26CompatStatePure &s, bool apActive) {
  if (s.summoning) return SUMMON_AUTH_SUMMON;
  if (apActive) return SUMMON_AUTH_AP;
  if (s.parked) return SUMMON_AUTH_PARK;
  return SUMMON_AUTH_NONE;
}

// V2.6 was a single-CAN implementation. In Universal, preserve profile-aware
// source routing but require only the actual 0x3FD transport bus for R79 TX.
static inline uint8_t summonV26CompatRequiredTxFreshMaskPure(
    const SummonRoutePure &route) {
  if (!route.valid) return SUMMON_BUS_NONE;
  return route.transportBusMask;
}

static inline bool summonRemoteFallbackAllowedPure(
    const SummonRoutePure &route, uint8_t confirmedGearState) {
  return route.valid && confirmedGearState == SUMMON_CONFIRMED_GEAR_UNKNOWN;
}

static inline bool summonParkEntryAllowedPure(
    bool freshParked, uint8_t confirmedGearState,
    bool remoteFallbackAllowed) {
  if (freshParked) return true;
  if (confirmedGearState == SUMMON_CONFIRMED_GEAR_PARK) return true;
  return confirmedGearState == SUMMON_CONFIRMED_GEAR_UNKNOWN &&
         remoteFallbackAllowed;
}

static inline uint8_t summonAuthorizationPure(
    bool freshParked, uint8_t confirmedGearState,
    bool remoteFallbackAllowed, bool apActive,
    bool sessionConfirmed, bool sessionGraceActive) {
  if (sessionConfirmed || sessionGraceActive) return SUMMON_AUTH_SUMMON;
  // AP remains authoritative even when the last confirmed gear was D/R/N or
  // the gear source goes silent. This preserves R79 while Autopilot is active.
  if (apActive) return SUMMON_AUTH_AP;
  if (freshParked) return SUMMON_AUTH_PARK;
  if (confirmedGearState == SUMMON_CONFIRMED_GEAR_PARK)
    return SUMMON_AUTH_PARK_LATCHED;
  if (confirmedGearState == SUMMON_CONFIRMED_GEAR_UNKNOWN &&
      remoteFallbackAllowed)
    return SUMMON_AUTH_REMOTE_FALLBACK;
  return SUMMON_AUTH_NONE;
}

// YL normally requires both Party (authorization source) and VH (transport).
// Once PARK has been confirmed and latched—or before any gear has been seen on
// a cold remote wake—the R79-only compatibility path needs only the actual
// 0x3FD transport bus. Standard 3/Y already uses CAN B for every source.
static inline uint8_t summonRequiredTxFreshMaskPure(
    const SummonRoutePure &route, uint8_t authorization) {
  (void)authorization;
  return summonV26CompatRequiredTxFreshMaskPure(route);
}

static inline bool summonTxBarrierAllowsPure(
    uint32_t currentEpoch, uint8_t freshMask, uint32_t expectedEpoch,
    uint8_t requiredFreshMask) {
  if (requiredFreshMask == SUMMON_BUS_NONE) return false;
  return currentEpoch == expectedEpoch &&
         (uint8_t)(freshMask & requiredFreshMask) == requiredFreshMask;
}

// Preserve freshness only for a bus that was not reset and is still physically
// fresh at the recovery boundary. This prevents an old mask bit from crossing a
// local recovery while avoiding an unnecessary wait for the unaffected bus.
static inline uint8_t summonPreservedFreshMaskPure(
    uint8_t currentFreshMask, uint8_t invalidatedBusMask,
    uint8_t physicallyFreshMask) {
  return (uint8_t)(currentFreshMask & (uint8_t)~invalidatedBusMask & physicallyFreshMask);
}


enum R79PendingKindPure : uint8_t {
  R79_PENDING_NONE = 0,
  R79_PENDING_IMMEDIATE = 1,
  R79_PENDING_PERIODIC = 2
};

struct R79PendingPure {
  bool pending;
  uint8_t kind;
  uint32_t requestedMs;
  uint32_t lastAttemptMs;
  uint32_t sequence;
};

// One-deep coalescing queue. Immediate work outranks periodic maintenance.
// Every request advances the sequence so an older in-flight completion cannot
// clear work requested while the transmit was running.
static inline void r79PendingRequestPure(
    R79PendingPure &state, uint8_t kind, uint32_t now) {
  if (kind != R79_PENDING_IMMEDIATE && kind != R79_PENDING_PERIODIC) return;
  const bool wasPending = state.pending;
  const uint32_t oldestRequestMs = state.requestedMs;
  state.sequence++;
  if (state.sequence == 0) state.sequence = 1;
  state.pending = true;
  if (kind == R79_PENDING_IMMEDIATE || state.kind == R79_PENDING_NONE) {
    state.kind = kind;
  }
  // Preserve the first unresolved request time across coalescing. This makes
  // wake-convergence latency represent the whole wait, not merely the newest
  // stock refresh that happened to arrive while the prerequisite was missing.
  state.requestedMs = wasPending && oldestRequestMs != 0 ? oldestRequestMs : now;
  // A newly requested/coalesced item gets one immediate attempt. Failed attempts
  // are subsequently rate-limited by r79PendingReadyPure().
  state.lastAttemptMs = 0;
}

static inline bool r79PendingReadyPure(
    const R79PendingPure &state, uint32_t now, uint32_t retryMs) {
  if (!state.pending) return false;
  return state.lastAttemptMs == 0 ||
         (uint32_t)(now - state.lastAttemptMs) >= retryMs;
}

static inline void r79PendingMarkAttemptPure(
    R79PendingPure &state, uint32_t now) {
  if (state.pending) state.lastAttemptMs = now;
}

static inline void r79PendingCompletePure(
    R79PendingPure &state, uint32_t completedSequence) {
  if (!state.pending || state.sequence != completedSequence) return;
  state.pending = false;
  state.kind = R79_PENDING_NONE;
  state.requestedMs = 0;
  state.lastAttemptMs = 0;
}

static inline const char *r79PendingKindNamePure(uint8_t kind) {
  switch (kind) {
    case R79_PENDING_IMMEDIATE: return "IMMEDIATE";
    case R79_PENDING_PERIODIC: return "PERIODIC";
    default: return "NONE";
  }
}

static inline const char *summonBusMaskNamePure(uint8_t mask) {
  switch (mask) {
    case SUMMON_BUS_A: return "CAN_A";
    case SUMMON_BUS_B: return "CAN_B";
    case SUMMON_BUS_BOTH: return "CAN_A+B";
    default: return "NONE";
  }
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

static inline const char *summonAuthorizationNamePure(uint8_t auth) {
  switch (auth) {
    case SUMMON_AUTH_PARK: return "PARK";
    case SUMMON_AUTH_AP: return "AP";
    case SUMMON_AUTH_SUMMON: return "SUMMON";
    case SUMMON_AUTH_PARK_LATCHED: return "PARK_LATCHED";
    case SUMMON_AUTH_REMOTE_FALLBACK: return "REMOTE_STANDBY";
    default: return "NONE";
  }
}
