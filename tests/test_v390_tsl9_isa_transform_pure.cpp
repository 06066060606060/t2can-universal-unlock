#include <assert.h>
#include <stdint.h>

#include "../tsl9_hands_on_0x399_pure.h"

static void makeFrame(uint8_t data[8], uint8_t apState, uint8_t handsOn,
                      uint8_t counter = 5u) {
  for (uint8_t i = 0; i < 8u; ++i) data[i] = 0u;
  data[0] = apState;
  data[5] = (uint8_t)(handsOn << 2);
  data[6] = (uint8_t)(counter << 4);
}

int main() {
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

  // ISA only applies to the original HO=4 state, even when the selected
  // Hands-On sequence does not transform that frame.
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
  // AP-inactive and non-HO4 inputs do not suppress ISA.
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
  assert(!result.isaModified);

  return 0;
}
