#include <cassert>
#include <cstdint>

#include "../ap_right_scroll_pure.h"

static ApRightScrollActionPure step(
    ApRightScrollStatePure &s, uint32_t now, bool gate,
    uint16_t regular, uint8_t repeat, uint32_t epoch,
    bool warning, uint8_t physical = 0u) {
  return apRightScrollStepPure(
      s, now, gate, regular, repeat, epoch, warning, true, physical);
}

int main() {
  assert(apRightScrollWarningRepeatValidPure(0u));
  assert(apRightScrollWarningRepeatValidPure(1u));
  assert(apRightScrollWarningRepeatValidPure(5u));
  assert(!apRightScrollWarningRepeatValidPure(6u));
  assert(apRightScrollWarningRepeatSanitizePure(0u) == 0u);

  // A zero repeat interval means warning-edge-only: fire immediately once,
  // finish the generated pair, and do not repeat while the warning remains.
  ApRightScrollStatePure oneShot = {};
  assert(step(oneShot, 1000u, true, 30u, 0u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto oneShotAction = step(oneShot, 1100u, true, 30u, 0u, 1u, true);
  assert(oneShotAction == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(oneShot, 1100u, oneShotAction, true);
  oneShotAction = step(oneShot, 1120u, true, 30u, 0u, 1u, true);
  assert(oneShotAction == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(oneShot, 1120u, oneShotAction, true);
  assert(step(oneShot, 10000u, true, 30u, 0u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);

  // The selectable four-step torque waveform has the same warning-edge-only
  // policy when repeat is zero.
  Tsl9RightScrollStatePure fourStepOneShot = {};
  assert(tsl9RightScrollStepPure(fourStepOneShot, 2000u, true, 30u, 0u,
                                 0u, false, true, 0u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);
  for (uint8_t index = 0u; index < 4u; ++index) {
    const uint32_t now = 2100u + (uint32_t)index * 100u;
    const Tsl9RightScrollActionPure fourStepAction =
        tsl9RightScrollStepPure(fourStepOneShot, now, true, 30u, 0u,
                                1u, true, true, 0u);
    assert(fourStepAction == tsl9RightScrollActionPure(index));
    tsl9RightScrollTxResultPure(
        fourStepOneShot, now, fourStepAction, true);
  }
  assert(tsl9RightScrollStepPure(fourStepOneShot, 20000u, true, 30u, 0u,
                                 1u, true, true, 0u) ==
         TSL9_RIGHT_SCROLL_NONE_PURE);

  // A visual-warning edge fires immediately, completes a full pair, and then
  // repeats at the configured interval while the same warning remains active.
  ApRightScrollStatePure s = {};
  assert(step(s, 1000u, true, 30u, 2u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto action = step(s, 1100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(s, 1100u, action, true);
  action = step(s, 1120u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(s, 1120u, action, true);
  assert(step(s, 3119u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(s, 3120u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);

  // Clearing a warning stops repeats. A later epoch fires immediately again.
  apRightScrollTxResultPure(s, 3120u, action, true);
  action = step(s, 3140u, true, 30u, 2u, 1u, false);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(s, 3140u, action, true);
  assert(step(s, 6000u, true, 30u, 2u, 1u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(s, 6100u, true, 30u, 2u, 2u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);

  // A physical scroll input wins both before UP and while DOWN is pending.
  ApRightScrollStatePure physical = {};
  assert(step(physical, 100u, true, 30u, 2u, 0u, false) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(step(physical, 200u, true, 30u, 2u, 1u, true, 1u) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!physical.downPending);
  assert(step(physical, 2199u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(physical, 2200u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(physical, 2200u, action, true);
  assert(physical.downPending);
  assert(step(physical, 2210u, true, 30u, 2u, 1u, true, 2u) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!physical.downPending);

  // A failed generated action abandons the whole pair and postpones retries.
  ApRightScrollStatePure failed = {};
  (void)step(failed, 1000u, true, 30u, 2u, 0u, false);
  action = step(failed, 1100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(failed, 1100u, action, false);
  assert(!failed.downPending);
  assert(step(failed, 3099u, true, 30u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  action = step(failed, 3100u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(failed, 3100u, action, true);
  action = step(failed, 3110u, true, 30u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(failed, 3110u, action, false);
  assert(!failed.downPending);

  // Warning takes priority when its edge collides with a regular deadline.
  ApRightScrollStatePure collision = {};
  (void)step(collision, 1000u, true, 1u, 2u, 0u, false);
  action = step(collision, 2000u, true, 1u, 2u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  assert(collision.attemptedFromWarning);

  // Gate close fully resets generated state.
  apRightScrollTxResultPure(collision, 2000u, action, true);
  assert(step(collision, 2010u, false, 1u, 2u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!collision.gateWasOpen && !collision.downPending);

  // Warning repeat deadlines remain correct across uint32_t rollover.
  ApRightScrollStatePure rollover = {};
  (void)step(rollover, 0xFFFFFF00u, true, 30u, 1u, 0u, false);
  action = step(rollover, 0xFFFFFF10u, true, 30u, 1u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(rollover, 0xFFFFFF10u, action, true);
  action = step(rollover, 0xFFFFFF20u, true, 30u, 1u, 1u, true);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(rollover, 0xFFFFFF20u, action, true);
  const uint32_t rolloverDue = 0x00000308u;
  assert(step(rollover, rolloverDue - 1u, true, 30u, 1u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(step(rollover, rolloverDue, true, 30u, 1u, 1u, true) ==
         AP_RIGHT_SCROLL_ACTION_UP_PURE);

  return 0;
}
