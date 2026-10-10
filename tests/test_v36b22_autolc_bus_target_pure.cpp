#include <cassert>
#include <cstdint>
#include "auto_lane_change_enable_pure.h"

int main() {
  assert(uiAutoLaneChangeBusTargetValidPure(UI_AUTO_LC_BUS_A_PURE));
  assert(uiAutoLaneChangeBusTargetValidPure(UI_AUTO_LC_BUS_B_PURE));
  assert(uiAutoLaneChangeBusTargetValidPure(UI_AUTO_LC_BUS_BOTH_PURE));
  assert(!uiAutoLaneChangeBusTargetValidPure(0));
  assert(!uiAutoLaneChangeBusTargetValidPure(4));

  assert(uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_A_PURE, UI_AUTO_LC_BUS_A_PURE));
  assert(!uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_A_PURE, UI_AUTO_LC_BUS_B_PURE));
  assert(!uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_B_PURE, UI_AUTO_LC_BUS_A_PURE));
  assert(uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_B_PURE, UI_AUTO_LC_BUS_B_PURE));
  assert(uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_BOTH_PURE, UI_AUTO_LC_BUS_A_PURE));
  assert(uiAutoLaneChangeBusAllowedPure(UI_AUTO_LC_BUS_BOTH_PURE, UI_AUTO_LC_BUS_B_PURE));
  return 0;
}
