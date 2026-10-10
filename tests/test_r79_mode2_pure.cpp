#include <cassert>
#include <cstdint>
#include <cstring>

#include "../r79_mode2_pure.h"

static void assertOnlyMode2BitsChanged(const uint8_t stock[8], const uint8_t out[8]) {
  for (uint8_t byte = 0; byte < 8; ++byte) {
    uint8_t allowed = 0;
    if (byte == 2) allowed = (uint8_t)(1u << 3);  // bit19 only
    if (byte == 5) allowed = (uint8_t)(1u << 7);  // bit47 only
    assert((((uint8_t)(stock[byte] ^ out[byte])) & (uint8_t)~allowed) == 0u);
  }
}

int main() {
  static_assert(R79_MODE_DEFAULT_PURE == R79_MODE_1_PURE,
                "upgrades retain the existing transport");
  static_assert(R79_MODE2_TX_WAIT_MS_PURE == 0u,
                "Mode 2 uses a non-blocking TWAI enqueue");
  static_assert(R79_MODE2_DELAY_DEFAULT_MS_PURE == 150u,
                "optional MUX2 reinjection defaults to +150 ms");
  static_assert(R79_MODE2_DELAY_MAX_MS_PURE == 2000u,
                "dashboard delay range is bounded");

  assert(r79ModeSanitizePure(R79_MODE_1_PURE) == R79_MODE_1_PURE);
  assert(r79ModeSanitizePure(R79_MODE_2_PURE) == R79_MODE_2_PURE);
  assert(r79ModeSanitizePure(0xFFu) == R79_MODE_DEFAULT_PURE);
  assert(r79Mode2DelaySanitizePure(0u) == 0u);
  assert(r79Mode2DelaySanitizePure(2000u) == 2000u);
  assert(r79Mode2DelaySanitizePure(2001u) == R79_MODE2_DELAY_DEFAULT_MS_PURE);

  // Mode 2 preserves stock bit18, clears bit19, and sets bit47. No other bit
  // may move. Exercise both possible stock bit18 values.
  const uint8_t stockSet[8] = {0x01u, 0xA5u, 0x0Cu, 0x33u, 0x55u, 0x00u, 0x77u, 0x99u};
  uint8_t out[8];
  std::memcpy(out, stockSet, sizeof(out));
  assert(r79Mode2ApplyBitsPure(out));
  assert(((out[2] >> 2) & 1u) == 1u);
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyMode2BitsChanged(stockSet, out);

  uint8_t stockClear[8] = {0x01u, 0xA5u, 0x08u, 0x33u, 0x55u, 0x80u, 0x77u, 0x99u};
  std::memcpy(out, stockClear, sizeof(out));
  assert(r79Mode2ApplyBitsPure(out));
  assert(((out[2] >> 2) & 1u) == 0u);
  assertOnlyMode2BitsChanged(stockClear, out);
  assert(!r79Mode2ApplyBitsPure(out));  // already at the Mode 2 target

  // Disabled delayed reinjection never arms.
  R79Mode2DelayedStatePure delayed = {};
  assert(!r79Mode2ObserveStockPure(delayed, 2u, 1000u, false, 150u).armed);
  assert(!delayed.pending);

  // Enabled MUX2 arms exactly one user-selected delayed shot.
  assert(r79Mode2ObserveStockPure(delayed, 2u, 2000u, true, 275u).armed);
  assert(delayed.pending && delayed.dueMs == 2275u);
  assert(r79Mode2DelayedStepPure(delayed, 2274u, true) == R79_MODE2_DELAY_WAIT_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 2275u, true) == R79_MODE2_DELAY_FIRE_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 2276u, true) == R79_MODE2_DELAY_WAIT_PURE);

  // A stock MUX0/MUX1 begins the next cycle and cancels the pending MUX2 shot.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, 3000u, true, 150u);
  const R79Mode2ObserveResultPure cancelled =
      r79Mode2ObserveStockPure(delayed, 1u, 3050u, true, 150u);
  assert(cancelled.cancelled && !delayed.pending);

  // The manual D/R policy is passed in as the common authorization result.
  // A denied delayed slot is consumed once and never retried.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, 4000u, true, 0u);
  assert(r79Mode2DelayedStepPure(delayed, 4000u, false) ==
         R79_MODE2_DELAY_DISALLOWED_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 4001u, true) ==
         R79_MODE2_DELAY_WAIT_PURE);

  // uint32 rollover preserves the configurable deadline comparison.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, UINT32_MAX - 20u, true, 50u);
  assert(r79Mode2DelayedStepPure(delayed, 28u, true) == R79_MODE2_DELAY_WAIT_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 29u, true) == R79_MODE2_DELAY_FIRE_PURE);
  return 0;
}
