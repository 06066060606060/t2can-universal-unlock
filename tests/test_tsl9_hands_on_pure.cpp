#include <cassert>
#include <cstdint>
#include <cstring>

#include "../tsl9_hands_on_0x399_pure.h"

static uint8_t expectedChecksum(const uint8_t data[8]) {
  uint16_t sum = 0x03u + 0x99u;
  for (uint8_t i = 0; i < 7; ++i) sum += data[i];
  return (uint8_t)sum;
}

int main() {
  static_assert(NAG_METHOD_DEFAULT_PURE == NAG_METHOD_TORQUE_PURE,
                "existing installs retain torque NAG");
  static_assert(TSL9_SEQUENCE_DEFAULT_PURE == TSL9_SEQUENCE_EXTENDED_PURE,
                "fresh and reset TSL9 settings default to Extended");
  static_assert(TSL9_DOWNGRADE_WINDOW_MS_PURE == 12000u,
                "TSL9 window matches the reference implementation");
  assert(nagMethodSanitizePure(NAG_METHOD_TORQUE_PURE) == NAG_METHOD_TORQUE_PURE);
  assert(nagMethodSanitizePure(NAG_METHOD_TSL9_PURE) == NAG_METHOD_TSL9_PURE);
  assert(nagMethodSanitizePure(99u) == NAG_METHOD_DEFAULT_PURE);
  assert(nagMethodResolveForCapabilitiesPure(
             NAG_METHOD_TORQUE_PURE, false, true) == NAG_METHOD_TSL9_PURE);
  assert(nagMethodResolveForCapabilitiesPure(
             NAG_METHOD_TSL9_PURE, true, false) == NAG_METHOD_TORQUE_PURE);
  assert(nagMethodResolveForCapabilitiesPure(
             NAG_METHOD_TSL9_PURE, true, true) == NAG_METHOD_TSL9_PURE);
  assert(nagMethodResolveForCapabilitiesPure(
             NAG_METHOD_TORQUE_PURE, true, true) == NAG_METHOD_TORQUE_PURE);
  assert(tsl9SequenceSanitizePure(TSL9_SEQUENCE_V82_ORIGINAL_PURE) ==
         TSL9_SEQUENCE_V82_ORIGINAL_PURE);
  assert(tsl9SequenceSanitizePure(TSL9_SEQUENCE_EXTENDED_PURE) ==
         TSL9_SEQUENCE_EXTENDED_PURE);
  assert(tsl9SequenceSanitizePure(99u) == TSL9_SEQUENCE_DEFAULT_PURE);

  Tsl9HandsOnStatePure state = {};
  uint8_t frame[8] = {3u, 0x11u, 0x22u, 0x33u, 0x44u,
                      (uint8_t)((4u << 2) | 0xC1u), 0xF5u, 0u};
  const uint8_t original0 = frame[0];
  const uint8_t original5Outer = (uint8_t)(frame[5] & 0xC3u);
  const Tsl9HandsOnResultPure first =
      tsl9ApplyHandsOnDowngradePure(
          state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE, frame, 8u, 1000u);
  assert(first.modified);
  assert(first.apActive);
  assert(first.handsOnBefore == TSL9_HANDS_ON_CHIME_1_PURE);
  assert(((frame[5] >> 2) & 0x0Fu) == TSL9_HANDS_ON_DETECTED_PURE);
  assert((frame[5] & 0xC3u) == original5Outer);
  assert((frame[6] >> 4) == 0u);  // 0xF rolls over to 0
  assert((frame[6] & 0x0Fu) == 5u);
  assert(frame[7] == expectedChecksum(frame));
  assert(frame[0] == original0);

  // The V8.2 original sequence passes states 2 and 3 through unchanged.
  const uint8_t earlierWarnings[] = {2u, 3u};
  for (const uint8_t warning : earlierWarnings) {
    Tsl9HandsOnStatePure warningState = {};
    uint8_t warningFrame[8] = {
        3u, 0x11u, 0x22u, 0x33u, 0x44u,
        (uint8_t)((warning << 2) | 0xC1u), 0x25u, 0u};
    const uint8_t warningOuter = (uint8_t)(warningFrame[5] & 0xC3u);
    const Tsl9HandsOnResultPure warningResult =
        tsl9ApplyHandsOnDowngradePure(
            warningState, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
            warningFrame, 8u, 1000u);
    assert(!warningResult.modified);
    assert(warningResult.handsOnBefore == warning);
    assert(((warningFrame[5] >> 2) & 0x0Fu) == warning);
    assert((warningFrame[5] & 0xC3u) == warningOuter);
    assert((warningFrame[6] >> 4) == 2u);
    assert((warningFrame[6] & 0x0Fu) == 5u);
    assert(warningFrame[7] == 0u);
  }

  // The current extended sequence remains selectable and downgrades the full
  // 2/3/4 warning range to DETECTED.
  for (const uint8_t warning : earlierWarnings) {
    Tsl9HandsOnStatePure warningState = {};
    uint8_t warningFrame[8] = {
        3u, 0x11u, 0x22u, 0x33u, 0x44u,
        (uint8_t)((warning << 2) | 0xC1u), 0x25u, 0u};
    const Tsl9HandsOnResultPure warningResult =
        tsl9ApplyHandsOnDowngradePure(
            warningState, true, TSL9_SEQUENCE_EXTENDED_PURE,
            warningFrame, 8u, 1000u);
    assert(warningResult.modified);
    assert(((warningFrame[5] >> 2) & 0x0Fu) ==
           TSL9_HANDS_ON_DETECTED_PURE);
    assert(warningFrame[7] == expectedChecksum(warningFrame));
  }

  // AP states 3..6 are one continuous active session. A state change inside
  // that range does not restart the 12-second window.
  frame[0] = 6u;
  frame[5] = (uint8_t)(4u << 2);
  frame[6] = 0x20u;
  assert(tsl9ApplyHandsOnDowngradePure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      frame, 8u, 12999u).modified);
  frame[5] = (uint8_t)(4u << 2);
  assert(!tsl9ApplyHandsOnDowngradePure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      frame, 8u, 13000u).modified);

  // An inactive AP frame resets the session. Re-engagement starts a fresh
  // window even when the method was disabled during the inactive frame.
  frame[0] = 2u;
  frame[5] = (uint8_t)(4u << 2);
  assert(!tsl9ApplyHandsOnDowngradePure(
      state, false, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      frame, 8u, 14000u).modified);
  assert(!state.apActive);
  frame[0] = 3u;
  assert(tsl9ApplyHandsOnDowngradePure(
      state, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      frame, 8u, 20000u).modified);

  // Disabled method, short DLC, non-active AP, and a non-warning hands-on state
  // all pass the stock frame through unchanged.
  uint8_t untouched[8] = {3u, 1u, 2u, 3u, 4u, (uint8_t)(4u << 2), 0x10u, 0xAAu};
  uint8_t before[8];
  std::memcpy(before, untouched, 8);
  Tsl9HandsOnStatePure disabled = {};
  assert(!tsl9ApplyHandsOnDowngradePure(
      disabled, false, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      untouched, 8u, 0u).modified);
  assert(std::memcmp(before, untouched, 8) == 0);
  assert(!tsl9ApplyHandsOnDowngradePure(
      disabled, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      untouched, 7u, 1u).modified);

  Tsl9HandsOnStatePure wrongHo = {};
  untouched[5] = (uint8_t)(5u << 2);
  std::memcpy(before, untouched, 8);
  assert(!tsl9ApplyHandsOnDowngradePure(
      wrongHo, true, TSL9_SEQUENCE_V82_ORIGINAL_PURE,
      untouched, 8u, 2u).modified);
  assert(std::memcmp(before, untouched, 8) == 0);
  return 0;
}
