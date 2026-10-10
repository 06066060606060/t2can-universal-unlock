#include <cassert>
#include <cstdint>
#include <cstring>

#include "../r79_fixed_policy_pure.h"

static void assertOnlyPolicyBitsChanged(const uint8_t stock[8], const uint8_t out[8]) {
  for (uint8_t byte = 0u; byte < 8u; ++byte) {
    uint8_t allowed = 0u;
    if (byte == 2u) allowed = (uint8_t)((1u << 2) | (1u << 3));
    if (byte == 5u) allowed = (uint8_t)(1u << 7);
    assert((((uint8_t)(stock[byte] ^ out[byte])) & (uint8_t)~allowed) == 0u);
  }
}

template <typename State>
static auto observeMode1PostMux2(
    State &state, bool enabled, uint16_t delayMs, int)
    -> decltype(r79FixedQuietObserveStockPure(
        state, 2u, 1000u, enabled, delayMs)) {
  return r79FixedQuietObserveStockPure(
      state, 2u, 1000u, enabled, delayMs);
}

template <typename State>
static R79FixedQuietObserveResultPure observeMode1PostMux2(
    State &, bool, uint16_t, long) {
  // Keeps the regression runnable before the new overload exists so RED is
  // an assertion failure rather than an invalid compile.
  R79FixedQuietObserveResultPure missing = {};
  missing.armed = true;
  return missing;
}

template <typename Pending, typename Periodic>
static auto mode1DisableCancelsRetry(Pending pending, Periodic periodic, int)
    -> decltype(r79Mode1DisableCancelsRetryPure(pending, periodic)) {
  return r79Mode1DisableCancelsRetryPure(pending, periodic);
}

template <typename Pending, typename Periodic>
static bool mode1DisableCancelsRetry(Pending, Periodic, long) {
  return true;
}

int main() {
  static_assert(R79_FIXED_FAST_WAIT_MS_PURE == 2u, "Fast Echo waits at most 2 ms");
  static_assert(R79_FIXED_QUIET_DELAY_MS_PURE == 150u, "quiet slot is MUX2 +150 ms");
  static_assert(R79_FIXED_QUIET_HARD_END_MS_PURE == 340u, "quiet slot hard guard");

  // Mode 1 keeps the current 2 ms behavior as the upgrade-safe default while
  // restoring the old zero-wait Fast Echo comparison option.
  assert(r79Mode1WaitModeSanitizePure(R79_MODE1_FAST_ECHO_PURE) ==
         R79_MODE1_FAST_ECHO_PURE);
  assert(r79Mode1WaitModeSanitizePure(R79_MODE1_2MS_WAIT_PURE) ==
         R79_MODE1_2MS_WAIT_PURE);
  assert(r79Mode1WaitModeSanitizePure(99u) == R79_MODE1_WAIT_DEFAULT_PURE);
  assert(r79Mode1TxWaitMsPure(R79_MODE1_FAST_ECHO_PURE) == 0u);
  assert(r79Mode1TxWaitMsPure(R79_MODE1_2MS_WAIT_PURE) == 2u);

  uint16_t parsedDelay = 999u;
  assert(!r79DelayTextParsePure("", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("abc", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("-1", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("341", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(r79DelayTextParsePure("0", R79_FIXED_QUIET_HARD_END_MS_PURE,
                               parsedDelay));
  assert(parsedDelay == 0u);
  assert(r79DelayTextParsePure("340", R79_FIXED_QUIET_HARD_END_MS_PURE,
                               parsedDelay));
  assert(parsedDelay == 340u);

  const uint8_t stock[8] = {0xA5u, 0x5Au, 0xFFu, 0x12u, 0x34u, 0x00u, 0x78u, 0x9Au};
  uint8_t out[8];

  std::memcpy(out, stock, sizeof(out));
  r79FixedApplyBitsPure(out, R79_BIT18_STOCK_PURE);
  assert(((out[2] >> 2) & 1u) == ((stock[2] >> 2) & 1u));
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyPolicyBitsChanged(stock, out);

  std::memcpy(out, stock, sizeof(out));
  r79FixedApplyBitsPure(out, R79_BIT18_FORCE_0_PURE);
  assert(((out[2] >> 2) & 1u) == 0u);
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyPolicyBitsChanged(stock, out);
  assert(r79Bit18PolicySanitizePure(99u) == R79_BIT18_DEFAULT_PURE);
  assert(R79_BIT18_DEFAULT_PURE == R79_BIT18_STOCK_PURE);

  // One MUX2 anchor produces exactly one shot at +150 ms.
  R79FixedQuietStatePure quiet = {};
  assert(!r79FixedQuietObserveStockPure(quiet, 1u, 1000u).armed);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 1050u).armed);
  assert(r79FixedQuietStepPure(quiet, 1199u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 1200u, true) == R79_FIXED_QUIET_FIRE_PURE);
  assert(r79FixedQuietStepPure(quiet, 1201u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Mode 1 Post-MUX2 remains upgrade-safe ON, but an explicit OFF prevents a
  // new reservation and clears an already pending reservation immediately.
  quiet = {};
  assert(observeMode1PostMux2(quiet, true, 150u, 0).armed);
  const R79FixedQuietObserveResultPure disabled =
      observeMode1PostMux2(quiet, false, 150u, 0);
  assert(!disabled.armed);
  assert(disabled.cancelled);
  assert(!quiet.pending);
  assert(!mode1DisableCancelsRetry(true, false, 0));
  assert(mode1DisableCancelsRetry(true, true, 0));
  assert(!mode1DisableCancelsRetry(false, true, 0));

  // A user-selected Mode 1 Post-MUX2 delay changes the actual one-shot
  // deadline. Values outside the existing 340 ms safety window fall back to
  // the established +150 ms default.
  quiet = {};
  assert(r79FixedQuietDelaySanitizePure(0u) == 0u);
  assert(r79FixedQuietDelaySanitizePure(340u) == 340u);
  assert(r79FixedQuietDelaySanitizePure(341u) == R79_FIXED_QUIET_DELAY_MS_PURE);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 1300u, 275u).armed);
  assert(quiet.dueMs == 1575u);
  assert(r79FixedQuietStepPure(quiet, 1574u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 1575u, true) == R79_FIXED_QUIET_FIRE_PURE);

  // The complete selectable range is schedulable. At the hard endpoint the
  // enqueue budget becomes zero so the sender cannot block past the window.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2000u, 0u).armed);
  assert(r79FixedQuietStepPure(quiet, 2000u, true) == R79_FIXED_QUIET_FIRE_PURE);
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 3000u, 340u).armed);
  assert(r79FixedQuietStepPure(quiet, 3339u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 3340u, true) == R79_FIXED_QUIET_FIRE_PURE);
  const uint32_t hardDeadline = r79FixedQuietHardDeadlinePure(quiet);
  assert(hardDeadline == 3340u);
  assert(r79FixedDeadlineWaitBudgetPure(3338u, hardDeadline, 2u) == 2u);
  assert(r79FixedDeadlineWaitBudgetPure(3339u, hardDeadline, 2u) == 1u);
  assert(r79FixedDeadlineWaitBudgetPure(3340u, hardDeadline, 2u) == 0u);
  assert(!r79FixedDeadlineExpiredPure(3340u, hardDeadline));
  assert(r79FixedDeadlineExpiredPure(3341u, hardDeadline));
  assert(r79FixedDeadlineWaitBudgetPure(UINT32_MAX - 1u, 2u, 2u) == 2u);
  assert(!r79FixedDeadlineExpiredPure(2u, 2u));
  assert(r79FixedDeadlineExpiredPure(3u, 2u));
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 4000u, 340u);
  assert(r79FixedQuietStepPure(quiet, 4341u, true) ==
         R79_FIXED_QUIET_GUARD_SKIP_PURE);

  // A newer MUX2 re-anchors the pending slot.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2000u).armed);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2100u).armed);
  assert(r79FixedQuietStepPure(quiet, 2249u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 2250u, true) == R79_FIXED_QUIET_FIRE_PURE);

  // A new stock MUX0/MUX1 cycle cancels the prior MUX2 slot.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 3000u).armed);
  const R79FixedQuietObserveResultPure cancelled =
      r79FixedQuietObserveStockPure(quiet, 0u, 3060u);
  assert(cancelled.cancelled && !cancelled.armed);
  assert(r79FixedQuietStepPure(quiet, 3150u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Authorization failure consumes the one slot rather than retrying forever.
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 4000u);
  assert(r79FixedQuietStepPure(quiet, 4150u, false) == R79_FIXED_QUIET_DISALLOWED_SKIP_PURE);
  assert(r79FixedQuietStepPure(quiet, 4151u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Delayed servicing beyond the hard window drops the slot.
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 5000u);
  assert(r79FixedQuietStepPure(quiet, 5341u, true) == R79_FIXED_QUIET_GUARD_SKIP_PURE);
  assert(r79FixedQuietStepPure(quiet, 5342u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // uint32_t rollover keeps both due and hard-window comparisons correct.
  quiet = {};
  const uint32_t nearWrap = UINT32_MAX - 100u;
  r79FixedQuietObserveStockPure(quiet, 2u, nearWrap);
  assert(r79FixedQuietStepPure(quiet, 48u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 49u, true) == R79_FIXED_QUIET_FIRE_PURE);

  return 0;
}
