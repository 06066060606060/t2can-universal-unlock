#include <cassert>
#include <cstdint>

#include "../tsl9_hands_on_0x399_pure.h"

static uint8_t handsOnFrame(uint8_t data[8], uint8_t apState,
                            uint8_t handsOn) {
  data[0] = apState;
  data[5] = (uint8_t)(handsOn << 2);
  data[6] = 0x20u;
  data[7] = 0u;
  return handsOn;
}

int main() {
  static_assert(TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE ==
                    TSL9_DOWNGRADE_FIRST_12S_PURE,
                "existing installs keep the 12-second default");
  assert(tsl9DowngradeWindowSanitizePure(99u) ==
         TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE);

  uint8_t frame[8] = {};
  Tsl9HandsOnStatePure first12 = {};
  handsOnFrame(frame, 3u, TSL9_HANDS_ON_CHIME_1_PURE);
  assert(tsl9ApplyHandsOnDowngradePure(
             first12, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
             TSL9_DOWNGRADE_FIRST_12S_PURE, frame, 8u, 1000u)
             .modified);
  handsOnFrame(frame, 3u, TSL9_HANDS_ON_CHIME_1_PURE);
  assert(!tsl9ApplyHandsOnDowngradePure(
              first12, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
              TSL9_DOWNGRADE_FIRST_12S_PURE, frame, 8u, 13000u)
              .modified);

  Tsl9HandsOnStatePure fullSession = {};
  handsOnFrame(frame, 3u, TSL9_HANDS_ON_REQUIRED_NOT_DETECTED_PURE);
  assert(tsl9ApplyHandsOnDowngradePure(
             fullSession, true, TSL9_SEQUENCE_EXTENDED_PURE,
             TSL9_DOWNGRADE_AP_SESSION_PURE, frame, 8u, 1000u)
             .modified);
  handsOnFrame(frame, 6u, TSL9_HANDS_ON_REQUIRED_NOT_DETECTED_PURE);
  assert(tsl9ApplyHandsOnDowngradePure(
             fullSession, true, TSL9_SEQUENCE_EXTENDED_PURE,
             TSL9_DOWNGRADE_AP_SESSION_PURE, frame, 8u, 61000u)
             .modified);

  handsOnFrame(frame, 2u, TSL9_HANDS_ON_CHIME_1_PURE);
  assert(!tsl9ApplyHandsOnDowngradePure(
              fullSession, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
              TSL9_DOWNGRADE_AP_SESSION_PURE, frame, 8u, 62000u)
              .modified);
  return 0;
}
