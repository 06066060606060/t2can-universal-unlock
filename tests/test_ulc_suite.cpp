// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "../ulc_compositor_pure.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ulc_compositor_pure.h"

// Coverage origin: test_ulc_compositor_pure.cpp
namespace case_ulc_compositor_pure {
int run() {
  uint8_t frame[8] = {0x02, 0x00, 0x44, 0x55, 0x66, 0x77, 0x00, 0x00};
  UlcCompositeSelectionPure selected = {};
  selected.alcOffHighwayEnabled = true;
  selected.ulcOffHighwayMode = 1;
  selected.blindSpotMode = 2;
  selected.confirmFreeEnabled = true;
  UlcCompositeGatesPure gates = {true, true, true, false};
  const UlcCompositeResultPure result =
      ulcCompose3f8Pure(frame, 8, selected, gates);
  assert(result.changed);
  assert(result.alcOffHighwayChanged);
  assert(result.ulcOffHighwayChanged);
  assert(result.blindSpotChanged);
  assert(result.confirmFreeChanged);
  assert((frame[7] & 0x01u) != 0);       // bit 56
  assert((frame[1] & 0x80u) != 0);       // bit 15
  assert((frame[6] & 0x30u) == 0x20u);   // bits 52-53
  assert((frame[0] & 0x02u) == 0);       // bit 1

  const UlcCompositeResultPure again =
      ulcCompose3f8Pure(frame, 8, selected, gates);
  assert(!again.changed);

  uint8_t invalidFrame[8] = {0x02, 0x00, 0x44, 0x55, 0x66, 0x77, 0x00, 0x00};
  uint8_t original[8] = {};
  std::memcpy(original, invalidFrame, sizeof(original));
  selected.blindSpotMode = 3;
  assert(!ulcCompose3f8Pure(invalidFrame, 8, selected, gates).changed);
  assert(std::memcmp(invalidFrame, original, sizeof(original)) == 0);
  assert(!ulcCompose3f8Pure(invalidFrame, 7, {}, gates).changed);
  assert(std::memcmp(invalidFrame, original, sizeof(original)) == 0);

  return 0;
  return 0;
}
} // namespace case_ulc_compositor_pure

// Coverage origin: test_ulc_policy_pure.cpp
namespace case_ulc_policy_pure {
int run() {
  uint8_t data[8] = {0x8B,0x28,0x88,0x00,0x19,0xBD,0x9D,0x24};
  assert(uiUlcOffHighwayReadRawPure(data, 8) == 0);

  assert(ulcPolicyApGateOpenPure(true, true, 3));
  assert(ulcPolicyApGateOpenPure(true, true, 4));
  assert(ulcPolicyApGateOpenPure(true, true, 5));
  assert(ulcPolicyApGateOpenPure(true, true, 6));
  assert(!ulcPolicyApGateOpenPure(true, true, 2));
  assert(!ulcPolicyApGateOpenPure(true, false, 5));
  assert(!ulcPolicyApGateOpenPure(false, true, 5));

  puts("ulc policy pure tests passed");
  return 0;
  return 0;
}
} // namespace case_ulc_policy_pure

// Coverage origin: test_ulc_stalk_confirm_pure.cpp
namespace case_ulc_stalk_confirm_pure {
int run() {
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
  return 0;
}
} // namespace case_ulc_stalk_confirm_pure

// Coverage origin: test_v36b17_ulc_stalk_confirm_timing_pure.cpp
namespace case_ulc_stalk_confirm_timing_pure {
int run() {
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
  return 0;
}
} // namespace case_ulc_stalk_confirm_timing_pure

int main() {
  if (case_ulc_compositor_pure::run()) return 1;
  if (case_ulc_policy_pure::run()) return 1;
  if (case_ulc_stalk_confirm_pure::run()) return 1;
  if (case_ulc_stalk_confirm_timing_pure::run()) return 1;
  return 0;
}
