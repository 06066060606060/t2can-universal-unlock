// Feature regression suite. Independent cases retain their original helper scopes.
#include <assert.h>
#include <stdint.h>
#include "../tsl9_hands_on_0x399_pure.h"
#include <cassert>
#include <cstdint>
#include "../vehicle_profile.h"

// Coverage origin: test_v326_isa_standalone_pure.cpp
namespace case_isa_standalone_pure {
static void makeFrame(uint8_t data[8], uint8_t apState, uint8_t handsOn,
                      uint8_t counter = 5u) {
  for (uint8_t i = 0; i < 8u; ++i) data[i] = 0u;
  data[0] = apState;
  data[5] = (uint8_t)(handsOn << 2);
  data[6] = (uint8_t)(counter << 4);
}

int run() {
  uint8_t frame[8];
  Tsl9HandsOnStatePure state = {};

  // ISA is independent: with the NAG Hands-On transform disabled, an active
  // AP CHIME 1 frame still gets only the ISA bit, counter and checksum update.
  makeFrame(frame, 3u, TSL9_HANDS_ON_CHIME_1_PURE, 7u);
  Tsl9DasTransformResultPure result = tsl9ApplyDasTransformForCanIdPure(
      state, false, TSL9_SEQUENCE_EXTENDED_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u,
      frame, 8u, 1000u);
  assert(result.modified);
  assert(!result.handsOnModified);
  assert(result.isaModified);
  assert(((frame[5] >> 2) & 0x0Fu) == TSL9_HANDS_ON_CHIME_1_PURE);
  assert((frame[1] & 0x20u) != 0u);
  assert((frame[6] >> 4) == 8u);
  assert(frame[7] == tsl9ChecksumForCanIdPure(0x399u, frame));

  // When both controls are enabled they compose into one generated frame and
  // advance the rolling counter exactly once.
  state = {};
  makeFrame(frame, 5u, TSL9_HANDS_ON_CHIME_1_PURE, 15u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_EXTENDED_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x39Bu,
      frame, 8u, 2000u);
  assert(result.handsOnModified && result.isaModified);
  assert(((frame[5] >> 2) & 0x0Fu) == TSL9_HANDS_ON_DETECTED_PURE);
  assert((frame[6] >> 4) == 0u);
  assert(frame[7] == tsl9ChecksumForCanIdPure(0x39Bu, frame));

  // ISA remains fail-closed outside active AP and independent of Hands-On.
  state = {};
  makeFrame(frame, 2u, TSL9_HANDS_ON_CHIME_1_PURE);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, false, TSL9_SEQUENCE_EXTENDED_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u,
      frame, 8u, 3000u);
  assert(!result.modified);
  state = {};
  makeFrame(frame, 3u, TSL9_HANDS_ON_REQUIRED_NOT_DETECTED_PURE);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, false, TSL9_SEQUENCE_EXTENDED_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u,
      frame, 8u, 4000u);
  assert(result.isaModified && !result.handsOnModified);
  return 0;
  return 0;
}
} // namespace case_isa_standalone_pure

// Coverage origin: test_v327_isa_independent_pure.cpp
namespace case_isa_independent_pure {
int run() {
  // Every supported topology has an independent ISA capability; preserve Nag.
  for (uint8_t model=1; model<=5; ++model) {
    for (uint8_t topology=0; topology<=4; ++topology) {
      const bool valid = model==VEHICLE_MODEL_YL ? topology==1u
                                                : topology==2u || topology==3u;
      assert(vehicleProfileIsaSuppressionSupported(model,topology)==valid);
      if (model!=VEHICLE_MODEL_YL && topology==3u)
        assert(!vehicleProfileNagTsl9Supported(model,topology));
    }
  }
  assert(!vehicleProfileIsaSuppressionSupported(0u,2u));
  assert(!vehicleProfileIsaSuppressionSupported(255u,3u));
  // All AP nibble values: ISA blocks unknown/inactive states and accepts 3..6.
  for (uint8_t ap=0; ap<16; ++ap) {
    uint8_t data[8]={ap,0x85u,0,0,0,0xFFu,0,0};
    assert(isaSuppressSpeedWarningPure(data,8u)==(ap>=3u && ap<=6u));
    assert(data[1]==(ap>=3u && ap<=6u ? 0xA5u : 0x85u));
  }
  uint8_t shortFrame[8]={3u,0,0,0,0,0,0,0};
  assert(!isaSuppressSpeedWarningPure(nullptr,8u));
  assert(!isaSuppressSpeedWarningPure(shortFrame,7u));

  for (uint8_t ho=0; ho<16; ++ho) {
    uint8_t data[8]={3u,0u,0xAAu,0x55u,0x12u,(uint8_t)(ho<<2),0xF5u,0u};
    Tsl9HandsOnStatePure state={};
    auto result=tsl9ApplyDasTransformForCanIdPure(state,false,0u,0u,true,0x399u,data,8u,1000u);
    assert(result.isaModified && !result.handsOnModified);
    assert(data[1]==0x20u && data[5]==(ho<<2) && data[6]==5u);
    assert(data[2]==0xAAu && data[3]==0x55u && data[4]==0x12u);
    assert(data[7]==tsl9ChecksumForCanIdPure(0x399u,data));
    result=tsl9ApplyDasTransformForCanIdPure(state,false,0u,0u,true,0x399u,data,8u,1001u);
    assert(!result.modified && data[6]==5u);
  }
  return 0;
  return 0;
}
} // namespace case_isa_independent_pure

// Coverage origin: test_v390_tsl9_isa_transform_pure.cpp
namespace case_tsl9_isa_transform_pure {
static void makeFrame(uint8_t data[8], uint8_t apState, uint8_t handsOn,
                      uint8_t counter = 5u) {
  for (uint8_t i = 0; i < 8u; ++i) data[i] = 0u;
  data[0] = apState;
  data[5] = (uint8_t)(handsOn << 2);
  data[6] = (uint8_t)(counter << 4);
}

int run() {
  // Hands-On only keeps ISA untouched and advances the rolling counter once.
  uint8_t frame[8];
  Tsl9HandsOnStatePure state = {};
  makeFrame(frame, 3u, 4u);
  Tsl9DasTransformResultPure result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, false, 0x399u, frame, 8u, 1000u);
  assert(result.modified && result.handsOnModified && !result.isaModified);
  assert(((frame[5] >> 2) & 0x0Fu) == 1u);
  assert((frame[1] & 0x20u) == 0u);
  assert((frame[6] >> 4) == 6u);
  assert(frame[7] == tsl9ChecksumForCanIdPure(0x399u, frame));

  // ISA remains active independently of the selected Hands-On sequence.
  state = {};
  makeFrame(frame, 3u, 4u, 9u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, 99u, TSL9_DOWNGRADE_AP_SESSION_PURE,
      true, 0x399u, frame, 8u, 2000u);
  // Invalid sequence sanitizes to Extended, so use a state outside Extended
  // for the true ISA-only vector below.
  assert(result.handsOnModified);

  state = {};
  makeFrame(frame, 3u, 4u, 9u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_FIRST_12S_PURE, true, 0x399u, frame, 8u,
      14000u);
  // The first active frame starts the 12 s window at now, so establish AP
  // first and then evaluate after the window expires.
  state = {true, 1000u};
  makeFrame(frame, 3u, 4u, 9u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_FIRST_12S_PURE, true, 0x399u, frame, 8u,
      14000u);
  assert(result.modified && !result.handsOnModified && result.isaModified);
  assert(((frame[5] >> 2) & 0x0Fu) == 4u);
  assert((frame[1] & 0x20u) != 0u);
  assert((frame[6] >> 4) == 10u);

  // Combined transform advances the counter exactly once and checksums 0x39B.
  state = {};
  makeFrame(frame, 5u, 4u, 15u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x39Bu, frame, 8u, 3000u);
  assert(result.handsOnModified && result.isaModified);
  assert((frame[6] >> 4) == 0u);
  assert(frame[7] == tsl9ChecksumForCanIdPure(0x39Bu, frame));

  // ISA remains independent when the Hands-On transform is disabled.
  state = {};
  makeFrame(frame, 3u, 4u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, false, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u, frame, 8u, 4000u);
  assert(result.modified && !result.handsOnModified && result.isaModified);
  assert((frame[1] & 0x20u) != 0u);
  // AP-inactive inputs stay blocked; non-HO4 active inputs suppress ISA.
  state = {};
  makeFrame(frame, 2u, 4u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u, frame, 8u, 5000u);
  assert(!result.modified);
  state = {};
  makeFrame(frame, 3u, 3u);
  result = tsl9ApplyDasTransformForCanIdPure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      TSL9_DOWNGRADE_AP_SESSION_PURE, true, 0x399u, frame, 8u, 6000u);
  assert(result.isaModified && !result.handsOnModified);

  return 0;
  return 0;
}
} // namespace case_tsl9_isa_transform_pure

int main() {
  if (case_isa_standalone_pure::run()) return 1;
  if (case_isa_independent_pure::run()) return 1;
  if (case_tsl9_isa_transform_pure::run()) return 1;
  return 0;
}
