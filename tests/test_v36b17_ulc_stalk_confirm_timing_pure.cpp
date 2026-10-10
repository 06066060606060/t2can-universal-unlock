#include <cassert>
#include <cstdint>
#include "ulc_stalk_confirm_pure.h"

int main() {
  assert(ulcNoConfirmTimingValidPure(ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE));
  assert(ulcNoConfirmTimingValidPure(ULC_NO_CONFIRM_TIMING_PRE_AP_PURE));
  assert(!ulcNoConfirmTimingValidPure(2u));

  // AP ACTIVE ONLY preserves v3.6b16 behavior.
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 3));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 6));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 2));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, false, 0));

  // PRE-AP + AP ACTIVE deliberately ignores DAS/AP state. If a live CAN B
  // 0x3F8 reaches the production compositor, bit1 may be overlaid before AP.
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_PRE_AP_PURE, false, 0));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_PRE_AP_PURE, true, 1));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true,
      ULC_NO_CONFIRM_TIMING_PRE_AP_PURE, true, 5));

  // Both timing modes still require a supported profile and feature ON.
  assert(!ulcNoConfirmGateOpenWithTimingPure(false, true,
      ULC_NO_CONFIRM_TIMING_PRE_AP_PURE, false, 0));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, false,
      ULC_NO_CONFIRM_TIMING_PRE_AP_PURE, false, 0));

  // Corrupt/unknown timing fails closed.
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, true, 2u, true, 5));

  return 0;
}
