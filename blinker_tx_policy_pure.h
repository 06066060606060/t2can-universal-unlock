#pragma once

#include <stdint.h>

enum BlinkerTxModePure : uint8_t {
  BLINKER_TX_MODE_SINGLE_PURE = 0,
  BLINKER_TX_MODE_LEGACY_PURE = 1
};

enum BlinkerTxSourcePure : uint8_t {
  BLINKER_TX_SOURCE_NONE_PURE = 0,
  BLINKER_TX_SOURCE_AUTO_PURE = 1,
  BLINKER_TX_SOURCE_S3XY_PURE = 2
};

struct BlinkerTxRequestStatePure {
  uint8_t pendingDir;
  uint8_t pendingSource;
  uint32_t requestedMs;
  uint32_t lastToken[2];
};

struct BlinkerTxConsumeResultPure {
  bool transmit;
  uint8_t dir;
  uint8_t source;
};

struct BlinkerTxModeTransitionPure {
  bool valid;
  uint8_t effectiveMode;
  bool cancelLegacy;
};

static inline bool blinkerTxArmPure(BlinkerTxRequestStatePure &state,
                                    uint8_t dir, uint8_t source,
                                    uint32_t eventToken, uint32_t now) {
  if ((dir != 1 && dir != 2) ||
      (source != BLINKER_TX_SOURCE_AUTO_PURE &&
       source != BLINKER_TX_SOURCE_S3XY_PURE)) {
    return false;
  }
  const uint8_t tokenIndex = (uint8_t)(source - BLINKER_TX_SOURCE_AUTO_PURE);
  if (eventToken != 0 && state.lastToken[tokenIndex] == eventToken) return false;
  if (eventToken != 0) state.lastToken[tokenIndex] = eventToken;
  state.pendingDir = dir;
  state.pendingSource = source;
  state.requestedMs = now;
  return true;
}

static inline BlinkerTxConsumeResultPure blinkerTxConsumeStockPure(
    BlinkerTxRequestStatePure &state, uint32_t now,
    bool physicalStalkIdle, uint32_t timeoutMs) {
  BlinkerTxConsumeResultPure result = {};
  if (state.pendingDir == 0) return result;

  const uint8_t dir = state.pendingDir;
  const uint8_t source = state.pendingSource;
  const uint32_t age = (uint32_t)(now - state.requestedMs);
  state.pendingDir = 0;
  state.pendingSource = BLINKER_TX_SOURCE_NONE_PURE;
  state.requestedMs = 0;
  if (!physicalStalkIdle || age > timeoutMs) return result;

  result.transmit = true;
  result.dir = dir;
  result.source = source;
  return result;
}

static inline uint8_t blinkerTxEffectiveModePure(uint8_t requested,
                                                 bool labEnabled) {
  return labEnabled && requested == BLINKER_TX_MODE_LEGACY_PURE
      ? BLINKER_TX_MODE_LEGACY_PURE
      : BLINKER_TX_MODE_SINGLE_PURE;
}

static inline BlinkerTxModeTransitionPure blinkerTxModeTransitionPure(
    uint8_t current, uint8_t requested, bool labEnabled) {
  BlinkerTxModeTransitionPure transition = {};
  if (current > BLINKER_TX_MODE_LEGACY_PURE ||
      requested > BLINKER_TX_MODE_LEGACY_PURE ||
      (!labEnabled && requested == BLINKER_TX_MODE_LEGACY_PURE)) {
    return transition;
  }
  transition.valid = true;
  transition.effectiveMode = blinkerTxEffectiveModePure(requested, labEnabled);
  transition.cancelLegacy = current == BLINKER_TX_MODE_LEGACY_PURE &&
      transition.effectiveMode == BLINKER_TX_MODE_SINGLE_PURE;
  return transition;
}
