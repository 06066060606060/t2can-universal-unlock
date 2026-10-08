#include <cassert>
#include <cstring>
#include "../ulc_compositor_pure.h"

// Exercise the retired selections when present, and their removal afterwards.
template<class T> auto retiredSelected(T &v,int)->decltype(v.visionSpeedDisabled=true,v.adaptiveSpeedDisabled=true,void()) {
  v.visionSpeedDisabled=true;v.adaptiveSpeedDisabled=true;
}
template<class T> void retiredSelected(T &,long){}
template<class T> auto retiredGate(T &v,int)->decltype(v.visionSpeedOpen=true,v.adaptiveSpeedOpen=true,void()) {
  v.visionSpeedOpen=true;v.adaptiveSpeedOpen=true;
}
template<class T> void retiredGate(T &,long){}
int main() {
  UlcCompositeSelectionPure selected={};
  selected.alcOffHighwayEnabled=true;selected.ulcOffHighwayMode=1;selected.blindSpotMode=1;selected.confirmFreeEnabled=true;
  UlcCompositeGatesPure gates={true,true,true,false};
  retiredSelected(selected,0);retiredGate(gates,0);
  uint8_t stock[8]={0xFE,0x25,0xF7,0xCA,0xB9,0x80,0xE2,0x16};
  uint8_t data[8];memcpy(data,stock,8);
  const uint8_t expected[8]={0xFC,0xA5,0xF7,0xCA,0xB9,0x80,0xD2,0x17};
  const auto result=ulcCompose3f8Pure(data,8,selected,gates);
  assert(result.changed);
  assert(memcmp(data,expected,8)==0);
  assert(memcmp(stock,data,8)!=0);
}
