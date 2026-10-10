#include <cassert>
#include <cstdint>
#include <cstring>
#include "auto_lane_change_enable_pure.h"
int main(){
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
}
