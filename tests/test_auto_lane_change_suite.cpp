// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "auto_lane_change_enable_pure.h"

// Coverage origin: test_auto_lane_change_enable_pure.cpp
namespace case_auto_lane_change_enable_pure {
int run(){
  uint8_t party[8]={0x00,0x0C,0x55,0x91,0x45,0x04,0xA4,0x74};
  uint8_t vh[8]={0x01,0x0C,0x55,0x91,0x55,0x04,0xE2,0xC3};
  assert(uiAutoLaneChangeReadRawPure(party,8)==1);
  assert(uiAutoLaneChangeReadRawPure(vh,8)==1);
  assert(ui293ChecksumPure(party)==0x74);
  assert(ui293ChecksumPure(vh)==0xC3);
  assert(uiAutoLaneChangeGateOpenPure(true,true,true,3));
  assert(uiAutoLaneChangeGateOpenPure(true,true,true,4));
  assert(uiAutoLaneChangeGateOpenPure(true,true,true,5));
  assert(uiAutoLaneChangeGateOpenPure(true,true,true,6));
  assert(!uiAutoLaneChangeGateOpenPure(true,true,true,2));
  assert(!uiAutoLaneChangeGateOpenPure(true,true,false,5));
  assert(!uiAutoLaneChangeGateOpenPure(true,false,true,5));
  uint8_t off[8]={0x00,0x0C,0x55,0x90,0x45,0x04,0xA4,0x00};
  assert(uiAutoLaneChangeFinalizePure(off,8));
  assert((off[3]&0x03)==1);
  assert(((off[6]>>4)&0x0F)==0x0B);
  assert(off[7]==0x84);
  return 0;
  return 0;
}
} // namespace case_auto_lane_change_enable_pure

// Coverage origin: test_v36b22_autolc_bus_target_pure.cpp
namespace case_autolc_bus_target_pure {
int run() {
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
  return 0;
}
} // namespace case_autolc_bus_target_pure

int main() {
  if (case_auto_lane_change_enable_pure::run()) return 1;
  if (case_autolc_bus_target_pure::run()) return 1;
  return 0;
}
