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

  // ISA remains fail-closed outside active AP and for non-target states.
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
  assert(!result.modified);
  return 0;
}
