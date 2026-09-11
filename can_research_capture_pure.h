#pragma once
#include <stdint.h>


enum ResearchAlcTransitionKind : uint8_t {
  RESEARCH_ALC_NONE = 0,
  RESEARCH_ALC_CLOSE = 1,
  RESEARCH_ALC_OPEN = 2
};

// LEFT research event semantics: LEFT is planner-available only in
// ALC_AVAILABLE_ONLY_L (6) or ALC_AVAILABLE_BOTH (8). Every other valid
// DAS_autoLaneChangeState is treated as LEFT blocked/unavailable for research.
// This intentionally captures explicit reasons such as LANE_TYPE_LEFT (26),
// SIDE_OBSTACLE_PRESENT_L (15), POOR_VIEW_RANGE (17), NO_LANES (1), etc.
static inline bool researchAlcLeftOpenPure(uint8_t alc) {
  return alc == 6 || alc == 8;
}

static inline ResearchAlcTransitionKind researchAlcTransitionKindPure(uint8_t previousAlc, uint8_t currentAlc) {
  const bool wasOpen = researchAlcLeftOpenPure(previousAlc);
  const bool isOpen = researchAlcLeftOpenPure(currentAlc);
  if (wasOpen && !isOpen) return RESEARCH_ALC_CLOSE;
  if (!wasOpen && isOpen) return RESEARCH_ALC_OPEN;
  return RESEARCH_ALC_NONE;
}

// AUTO ALC persistence filter. `stableAlc` is the last committed planner state.
// A candidate LEFT open/close transition must remain semantically open/closed
// for persistenceMs before it is emitted. Changes within the same semantic
// class (for example BLOCKED 26 -> 15 or OPEN 8 -> 6) do not restart the timer.
struct ResearchAlcPersistenceState {
  uint8_t stableAlc;
  uint8_t candidateKind;
  uint32_t candidateStartMs;
};

struct ResearchAlcPersistenceResult {
  ResearchAlcTransitionKind kind;
  uint32_t transitionMs;
};

static inline ResearchAlcPersistenceState researchAlcPersistenceInitialPure() {
  ResearchAlcPersistenceState s = {0xFF, RESEARCH_ALC_NONE, 0};
  return s;
}

static inline ResearchAlcPersistenceResult researchAlcPersistenceStepPure(
    ResearchAlcPersistenceState &state, uint8_t currentAlc,
    uint32_t now, uint32_t persistenceMs) {
  ResearchAlcPersistenceResult out = {RESEARCH_ALC_NONE, 0};

  if (state.stableAlc == 0xFF) {
    state.stableAlc = currentAlc;
    state.candidateKind = RESEARCH_ALC_NONE;
    state.candidateStartMs = 0;
    return out;
  }

  if (currentAlc == state.stableAlc) {
    state.candidateKind = RESEARCH_ALC_NONE;
    state.candidateStartMs = 0;
    return out;
  }

  const ResearchAlcTransitionKind transition = researchAlcTransitionKindPure(state.stableAlc, currentAlc);
  if (transition == RESEARCH_ALC_NONE) {
    // Preserve the old direct-transition semantics: unrelated intermediate
    // planner states become the new baseline rather than bridging an event.
    state.stableAlc = currentAlc;
    state.candidateKind = RESEARCH_ALC_NONE;
    state.candidateStartMs = 0;
    return out;
  }

  if (state.candidateKind != transition) {
    state.candidateKind = (uint8_t)transition;
    state.candidateStartMs = now;
    return out;
  }

  if ((uint32_t)(now - state.candidateStartMs) < persistenceMs) return out;

  out.kind = transition;
  out.transitionMs = state.candidateStartMs;
  state.stableAlc = currentAlc;
  state.candidateKind = RESEARCH_ALC_NONE;
  state.candidateStartMs = 0;
  return out;
}

struct ResearchRawCopyPlan {
  uint32_t ringStart;
  uint32_t count;
  bool truncated;
};

// Plan a chronological copy from a wrapped PRE ring. The production path uses
// the result for at most two bulk memcpy operations instead of one archive
// append per CAN frame.
template <typename Entry>
static inline ResearchRawCopyPlan researchRawCopyPlanPure(
    const Entry *ring, uint32_t capacity, uint32_t start, uint32_t frameCount,
    uint32_t triggerNow, uint32_t preWindowMs, uint32_t archiveAvailable) {
  ResearchRawCopyPlan out = {0, 0, false};
  if (!ring || capacity == 0 || frameCount == 0 || start >= capacity) return out;
  if (frameCount > capacity) frameCount = capacity;

  uint32_t skip = 0;
  while (skip < frameCount) {
    const uint32_t idx = (start + skip) % capacity;
    if ((uint32_t)(triggerNow - ring[idx].timestampMs) <= preWindowMs) break;
    skip++;
  }
  out.ringStart = (start + skip) % capacity;
  uint32_t eligible = 0;
  while (skip + eligible < frameCount) {
    const uint32_t idx = (start + skip + eligible) % capacity;
    if ((uint32_t)(triggerNow - ring[idx].timestampMs) > preWindowMs) break;
    eligible++;
  }
  out.count = eligible < archiveAvailable ? eligible : archiveAvailable;
  out.truncated = out.count < eligible;
  return out;
}


// Pure Tesla lane/ALC decode helpers retained for CAN Research Capture.
struct DasLane239Decoded {
  bool valid;
  bool leftLaneExists;
  bool rightLaneExists;
  uint8_t leftLineUsage;
  uint8_t rightLineUsage;
  uint8_t leftFork;
  uint8_t rightFork;
};

static inline DasLane239Decoded dasLane239DecodePure(const uint8_t *data, uint8_t dlc) {
  DasLane239Decoded out{};
  if (!data || dlc < 7) return out;
  out.valid = true;
  out.leftLaneExists = (data[0] & 0x01u) != 0;
  out.rightLaneExists = ((data[0] >> 1) & 0x01u) != 0;
  out.leftLineUsage = (uint8_t)(data[6] & 0x03u);
  out.rightLineUsage = (uint8_t)((data[6] >> 2) & 0x03u);
  out.leftFork = (uint8_t)((data[6] >> 4) & 0x03u);
  out.rightFork = (uint8_t)((data[6] >> 6) & 0x03u);
  return out;
}

static inline bool dasLeftResearchLaneQualifiedPure(const DasLane239Decoded &lane) {
  return lane.valid && lane.leftLaneExists && lane.leftLineUsage == 2;
}

static inline uint8_t das399ReadAlcPure(const uint8_t *data, uint8_t dlc = 8) {
  if (!data || dlc < 7) return 0xFF;
  return (uint8_t)(((data[5] >> 6) & 0x03u) | ((data[6] & 0x07u) << 2));
}
