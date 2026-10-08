#include <cassert>
#include <cstdint>
#include "ulc_stalk_confirm_pure.h"

int main() {
  // Any supported dual-CAN profile with 0x3F8 on CAN B is eligible.
  assert(ulcNoConfirmSupportedPure(true));
  assert(!ulcNoConfirmSupportedPure(false));

  // Production no-confirm injection is AP-active by default and no longer
  // depends on the global LAB switch.
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 3));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 4));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 5));
  assert(ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 6));
  assert(!ulcNoConfirmGateOpenWithTimingPure(false, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 5));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, false, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 5));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, false, 5));
  assert(!ulcNoConfirmGateOpenWithTimingPure(true, true, ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE, true, 2));
  return 0;
}
