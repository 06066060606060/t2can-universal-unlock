#pragma once

#include <stdint.h>

enum NagMethodPure : uint8_t {
  NAG_METHOD_TORQUE_PURE = 0,
  NAG_METHOD_TSL9_PURE = 1
};

enum Tsl9SequencePure : uint8_t {
  TSL9_SEQUENCE_V82_ORIGINAL_PURE = 0,
  TSL9_SEQUENCE_EXTENDED_PURE = 1
};

enum Tsl9DowngradeWindowPure : uint8_t {
  TSL9_DOWNGRADE_FIRST_12S_PURE = 0,
  TSL9_DOWNGRADE_AP_SESSION_PURE = 1
};

enum Tsl9LegacyRoutePure : uint8_t {
  TSL9_LEGACY_ROUTE_BODY_39B_PURE = 0,
  TSL9_LEGACY_ROUTE_CHASSIS_399_PURE = 1,
};

static constexpr uint8_t NAG_METHOD_DEFAULT_PURE = NAG_METHOD_TORQUE_PURE;
static constexpr uint8_t TSL9_SEQUENCE_DEFAULT_PURE =
    TSL9_SEQUENCE_EXTENDED_PURE;
static constexpr uint8_t TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE =
    TSL9_DOWNGRADE_FIRST_12S_PURE;
static constexpr uint8_t TSL9_LEGACY_ROUTE_DEFAULT_PURE =
    TSL9_LEGACY_ROUTE_BODY_39B_PURE;
static constexpr uint32_t TSL9_DOWNGRADE_WINDOW_MS_PURE = 12000u;
static constexpr uint8_t TSL9_HANDS_ON_DETECTED_PURE = 1u;
static constexpr uint8_t TSL9_HANDS_ON_REQUIRED_NOT_DETECTED_PURE = 2u;
static constexpr uint8_t TSL9_HANDS_ON_CHIME_1_PURE = 4u;

static inline uint8_t nagMethodSanitizePure(uint8_t method) {
  return method == NAG_METHOD_TORQUE_PURE || method == NAG_METHOD_TSL9_PURE
      ? method : NAG_METHOD_DEFAULT_PURE;
}

static inline bool nagMethodSupportedPure(uint8_t method, bool torqueSupported,
                                          bool tsl9Supported) {
  const uint8_t clean = nagMethodSanitizePure(method);
  return clean == NAG_METHOD_TSL9_PURE ? tsl9Supported : torqueSupported;
}

// Resolve a persisted/requested NAG method against the active vehicle topology.
// Preserve a supported selection. If it is impossible on the active topology,
// prefer TSL9 (the Body+Chassis path), then Torque.
static inline uint8_t nagMethodResolveForCapabilitiesPure(
    uint8_t method, bool torqueSupported, bool tsl9Supported) {
  const uint8_t clean = nagMethodSanitizePure(method);
  if (nagMethodSupportedPure(clean, torqueSupported, tsl9Supported)) return clean;
  if (tsl9Supported) return NAG_METHOD_TSL9_PURE;
  if (torqueSupported) return NAG_METHOD_TORQUE_PURE;
  return clean;
}

static inline const char *nagMethodNamePure(uint8_t method) {
  return nagMethodSanitizePure(method) == NAG_METHOD_TSL9_PURE
      ? "TSL9_0x399" : "TORQUE";
}

static inline uint8_t tsl9SequenceSanitizePure(uint8_t sequence) {
  return sequence == TSL9_SEQUENCE_V82_ORIGINAL_PURE ||
         sequence == TSL9_SEQUENCE_EXTENDED_PURE
      ? sequence : TSL9_SEQUENCE_DEFAULT_PURE;
}

static inline const char *tsl9SequenceNamePure(uint8_t sequence) {
  return tsl9SequenceSanitizePure(sequence) ==
             TSL9_SEQUENCE_EXTENDED_PURE
      ? "EXTENDED_2_3_4_TO_1" : "V8_2_ORIGINAL_4_TO_1";
}

static inline uint8_t tsl9DowngradeWindowSanitizePure(uint8_t window) {
  return window == TSL9_DOWNGRADE_FIRST_12S_PURE ||
         window == TSL9_DOWNGRADE_AP_SESSION_PURE
      ? window : TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE;
}

static inline uint8_t tsl9LegacyRouteSanitizePure(uint8_t route) {
  return route == TSL9_LEGACY_ROUTE_CHASSIS_399_PURE
      ? TSL9_LEGACY_ROUTE_CHASSIS_399_PURE
      : TSL9_LEGACY_ROUTE_BODY_39B_PURE;
}

static inline const char *tsl9DowngradeWindowNamePure(uint8_t window) {
  return tsl9DowngradeWindowSanitizePure(window) ==
             TSL9_DOWNGRADE_AP_SESSION_PURE
      ? "AP_SESSION" : "FIRST_12_SECONDS";
}

static inline bool tsl9ApActivePure(uint8_t apState) {
  return apState >= 3u && apState <= 6u;
}

static inline uint8_t tsl9ChecksumForCanIdPure(
    uint16_t canId, const uint8_t data[8]) {
  uint16_t sum = (uint16_t)((canId >> 8) & 0x07u) +
                 (uint16_t)(canId & 0xFFu);
  for (uint8_t i = 0; i < 7u; ++i) sum += data[i];
  return (uint8_t)sum;
}

static inline uint8_t tsl9Checksum399Pure(const uint8_t data[8]) {
  return tsl9ChecksumForCanIdPure(0x399u, data);
}

struct Tsl9HandsOnStatePure {
  bool apActive;
  uint32_t apActiveSinceMs;
};

struct Tsl9HandsOnResultPure {
  bool modified;
  bool apActive;
  uint8_t handsOnBefore;
};

struct Tsl9DasTransformResultPure {
  bool modified;
  bool handsOnModified;
  bool isaModified;
  bool apActive;
  uint8_t handsOnBefore;
};

// Compose every TSL9 DAS mutation before touching the rolling counter or
// checksum. The predicates intentionally inspect the original stock payload,
// so Hands-On 4 may both downgrade to 1 and suppress the ISA chime without a
// second generated frame or a second counter increment.
static inline Tsl9DasTransformResultPure
tsl9ApplyDasTransformForCanIdPure(
    Tsl9HandsOnStatePure &state, bool handsOnEnabled, uint8_t sequence,
    uint8_t downgradeWindow, bool isaChimeSuppress, uint16_t canId,
    uint8_t data[8], uint8_t dlc, uint32_t nowMs) {
  Tsl9DasTransformResultPure result = {};
  if (!data || dlc < 8u) return result;

  const bool activeNow = tsl9ApActivePure((uint8_t)(data[0] & 0x0Fu));
  if (activeNow && !state.apActive) state.apActiveSinceMs = nowMs;
  state.apActive = activeNow;
  result.apActive = activeNow;
  const uint8_t handsOn = (uint8_t)((data[5] >> 2) & 0x0Fu);
  result.handsOnBefore = handsOn;
  if (!activeNow || (!handsOnEnabled && !isaChimeSuppress)) return result;

  const bool insideDowngradeWindow =
      tsl9DowngradeWindowSanitizePure(downgradeWindow) ==
          TSL9_DOWNGRADE_AP_SESSION_PURE ||
      (uint32_t)(nowMs - state.apActiveSinceMs) <
          TSL9_DOWNGRADE_WINDOW_MS_PURE;
  const bool originalTarget = handsOn == TSL9_HANDS_ON_CHIME_1_PURE;
  const bool extendedTarget =
      handsOn >= TSL9_HANDS_ON_REQUIRED_NOT_DETECTED_PURE &&
      handsOn <= TSL9_HANDS_ON_CHIME_1_PURE;
  const bool downgradeTarget =
      tsl9SequenceSanitizePure(sequence) ==
          TSL9_SEQUENCE_V82_ORIGINAL_PURE
          ? originalTarget : extendedTarget;
  if (handsOnEnabled && insideDowngradeWindow && downgradeTarget) {
    data[5] = (uint8_t)((data[5] & 0xC3u) |
                        (TSL9_HANDS_ON_DETECTED_PURE << 2));
    result.handsOnModified = true;
  }

  if (isaChimeSuppress &&
      handsOn == TSL9_HANDS_ON_CHIME_1_PURE &&
      (data[1] & 0x20u) == 0u) {
    data[1] |= 0x20u;
    result.isaModified = true;
  }

  result.modified = result.handsOnModified || result.isaModified;
  if (result.modified) {
    const uint8_t counter = (uint8_t)(((data[6] >> 4) + 1u) & 0x0Fu);
    data[6] = (uint8_t)((data[6] & 0x0Fu) | (counter << 4));
    data[7] = tsl9ChecksumForCanIdPure(canId, data);
  }
  return result;
}

static inline Tsl9HandsOnResultPure
tsl9ApplyHandsOnDowngradeForCanIdPure(
    Tsl9HandsOnStatePure &state, bool enabled, uint8_t sequence,
    uint8_t downgradeWindow, uint16_t canId, uint8_t data[8],
    uint8_t dlc, uint32_t nowMs) {
  const Tsl9DasTransformResultPure composed =
      tsl9ApplyDasTransformForCanIdPure(
          state, enabled, sequence, downgradeWindow, false, canId,
          data, dlc, nowMs);
  return {composed.modified, composed.apActive, composed.handsOnBefore};
}

static inline Tsl9HandsOnResultPure tsl9ApplyHandsOnDowngradePure(
    Tsl9HandsOnStatePure &state, bool enabled, uint8_t sequence,
    uint8_t downgradeWindow, uint8_t data[8], uint8_t dlc,
    uint32_t nowMs) {
  return tsl9ApplyHandsOnDowngradeForCanIdPure(
      state, enabled, sequence, downgradeWindow, 0x399u,
      data, dlc, nowMs);
}

// Preserve the original call contract for host tests and older integrations.
static inline Tsl9HandsOnResultPure tsl9ApplyHandsOnDowngradePure(
    Tsl9HandsOnStatePure &state, bool enabled, uint8_t sequence,
    uint8_t data[8], uint8_t dlc, uint32_t nowMs) {
  return tsl9ApplyHandsOnDowngradePure(
      state, enabled, sequence, TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE,
      data, dlc, nowMs);
}
