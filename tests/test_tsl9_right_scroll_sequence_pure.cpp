#include <cassert>
#include <cstdint>

#include "../ap_right_scroll_pure.h"

static Tsl9RightScrollActionPure step(
    Tsl9RightScrollStatePure &state, uint32_t nowMs, bool gateOpen,
    uint32_t warningEpoch, bool warningActive, uint8_t physicalValue = 0u) {
  return tsl9RightScrollStepPure(
      state, nowMs, gateOpen, 30u, 2u, warningEpoch, warningActive,
      true, physicalValue);
}

int main() {
  // The standalone feature depends on CAN support, its own switch, AP, and TX
  // readiness. It must not require a NAG mode or NAG enabled flag.
  assert(apRightScrollGateOpenPure(true, true, true, true, false));
  assert(!apRightScrollGateOpenPure(true, true, false, true, false));
  assert(!apRightScrollGateOpenPure(true, true, true, false, false));
  assert(!apRightScrollGateOpenPure(true, true, true, true, true));

  // A TSL9 warning emits the exact V8.2 right-speed waveform with 100 ms
  // between accepted stock-frame-derived transmissions.
  Tsl9RightScrollStatePure state = {};
  auto action = step(state, 1000u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1000u, action, true);
  assert(step(state, 1099u, true, 1u, true) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  action = step(state, 1100u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1100u, action, true);
  action = step(state, 1200u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_NEGATIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1200u, action, true);
  action = step(state, 1300u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE);
  tsl9RightScrollTxResultPure(state, 1300u, action, true);
  assert(!state.sequenceActive);

  // Payload behavior matches V8.2: Byte3 carries signed six-bit ticks and
  // Byte6 bit4 marks the two centered stages.
  const Tsl9RightScrollActionPure actions[4] = {
      TSL9_RIGHT_SCROLL_POSITIVE_PURE,
      TSL9_RIGHT_SCROLL_CENTER_AFTER_POSITIVE_PURE,
      TSL9_RIGHT_SCROLL_NEGATIVE_PURE,
      TSL9_RIGHT_SCROLL_CENTER_AFTER_NEGATIVE_PURE,
  };
  const uint8_t expectedValues[4] = {0x01u, 0x00u, 0x3Fu, 0x00u};
  const bool expectedCenters[4] = {false, true, false, true};
  for (uint8_t i = 0u; i < 4u; ++i) {
    uint8_t payload[8] = {0x01u, 0u, 0u, 0xC0u, 0u, 0u, 0xA5u, 0u};
    tsl9RightScrollApplyActionPure(payload, actions[i]);
    assert((payload[3] & 0x3Fu) == expectedValues[i]);
    assert(((payload[6] & 0x10u) != 0u) == expectedCenters[i]);
  }

  // Real driver input cancels a generated sequence and postpones assistance.
  Tsl9RightScrollStatePure physical = {};
  action = step(physical, 2000u, true, 1u, true);
  assert(action == TSL9_RIGHT_SCROLL_POSITIVE_PURE);
  tsl9RightScrollTxResultPure(physical, 2000u, action, true);
  assert(physical.sequenceActive);
  assert(step(physical, 2050u, true, 1u, true, 1u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  assert(!physical.sequenceActive);

  return 0;
}
