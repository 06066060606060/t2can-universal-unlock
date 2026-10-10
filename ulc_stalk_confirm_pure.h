#pragma once
#include <stdint.h>

// Production UI_ulcStalkConfirm policy. The feature uses the fixed stock-follow
// CAN B / Chassis 0x3F8 path. Timing remains selectable without a LAB gate.
static constexpr uint8_t ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE = 0u;
static constexpr uint8_t ULC_NO_CONFIRM_TIMING_PRE_AP_PURE = 1u;

static inline bool ulcNoConfirmTimingValidPure(uint8_t timingMode) {
  return timingMode == ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE ||
         timingMode == ULC_NO_CONFIRM_TIMING_PRE_AP_PURE;
}

static inline bool ulcNoConfirmSupportedPure(bool profileAvailable) {
  return profileAvailable;
}

static inline bool ulcNoConfirmGateOpenWithTimingPure(bool profileAvailable,
                                                       bool featureEnabled,
                                                       uint8_t timingMode,
                                                       bool dasStateValid,
                                                       uint8_t dasState4) {
  if (!profileAvailable || !featureEnabled ||
      !ulcNoConfirmTimingValidPure(timingMode)) return false;
  if (timingMode == ULC_NO_CONFIRM_TIMING_PRE_AP_PURE) return true;
  if (!dasStateValid) return false;
  return dasState4 == 3 || dasState4 == 4 || dasState4 == 5 || dasState4 == 6;
}
