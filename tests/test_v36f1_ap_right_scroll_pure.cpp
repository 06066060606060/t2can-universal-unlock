#include <cassert>
#include <cstdint>

#include "../ap_right_scroll_pure.h"

int main() {
  assert(!apRightScrollIntervalValidPure(0));
  assert(apRightScrollIntervalValidPure(1));
  assert(apRightScrollIntervalValidPure(600));
  assert(!apRightScrollIntervalValidPure(601));
  assert(apRightScrollIntervalSanitizePure(0) == AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE);
  assert(apRightScrollIntervalSanitizePure(42) == 42);

  uint8_t frame[8] = {0x29, 0x55, 0xAA, 0xC0, 0x11, 0x22, 0x31, 0x15};
  assert(apRightScrollMuxPure(frame) == 1);
  assert(apRightScrollValuePure(frame) == 0);
  apRightScrollSetValuePure(frame, AP_RIGHT_SCROLL_UP_PURE);
  assert(frame[3] == 0xC1);
  assert(frame[0] == 0x29 && frame[2] == 0xAA && frame[7] == 0x15);
  apRightScrollSetValuePure(frame, AP_RIGHT_SCROLL_DOWN_PURE);
  assert(frame[3] == 0xFF);

  ApRightScrollStatePure state = {};
  assert(apRightScrollStepPure(state, 1000, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.gateWasOpen);
  assert(state.nextDueMs == 11000);
  assert(apRightScrollStepPure(state, 10999, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  auto action = apRightScrollStepPure(state, 11000, true, 10, 2, 0, false, true, 0);
  assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  apRightScrollTxResultPure(state, 11000, action, true);
  action = apRightScrollStepPure(state, 11100, true, 10, 2, 0, false, true, 0);
  assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
  apRightScrollTxResultPure(state, 11100, action, true);
  assert(state.nextDueMs == 21100);

  // A real right-scroll input wins and restarts the configured interval.
  assert(apRightScrollStepPure(state, 21100, true, 10, 2, 0, false, true, 0x01) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.nextDueMs == 31100);

  // Closing and reopening the gate never produces an immediate pulse.
  assert(apRightScrollStepPure(state, 22000, false, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(!state.gateWasOpen && state.nextDueMs == 0);
  assert(apRightScrollStepPure(state, 23000, true, 10, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  assert(state.nextDueMs == 33000);

  // millis() rollover uses signed-deadline comparison.
  state = {};
  assert(apRightScrollStepPure(state, 0xFFFFFF00u, true, 1, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
  const uint32_t wrappedDue = state.nextDueMs;
  assert(apRightScrollStepPure(state, wrappedDue, true, 1, 2, 0, false, true, 0) == AP_RIGHT_SCROLL_ACTION_UP_PURE);
  return 0;
}
