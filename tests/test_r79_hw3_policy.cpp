#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "../r79_fixed_policy_pure.h"
#include "../r79_mode2_pure.h"
#include "../vehicle_profile.h"

int main() {
  for (uint8_t model=0;model<=6;++model) for (uint8_t topology=0;topology<=4;++topology) {
    const bool expected=(model==VEHICLE_MODEL_Y_LEGACY || model==VEHICLE_MODEL_3_LEGACY) &&
        (topology==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS || topology==VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS);
    assert(vehicleProfileR79Hw3Supported(model,topology)==expected);
  }
  for (unsigned byte2=0;byte2<256;++byte2) for (unsigned byte5=0;byte5<256;++byte5) {
    const uint8_t stock[8]={1,0x92,(uint8_t)byte2,0xAC,0x63,(uint8_t)byte5,0x71,0xE8};
    for (bool preserve : {false,true}) {
      for (uint8_t bit18 : {0,1}) {
        uint8_t out[8];std::memcpy(out,stock,8);
        r79FixedApplyBitsPure(out,bit18,preserve);
        for (unsigned i=0;i<8;++i) {
          const uint8_t expected=i==2?(uint8_t)(stock[i] & (bit18?0xF3:0xF7)):
              i==5?(preserve?stock[i]:(uint8_t)(stock[i]|0x80)):stock[i];
          assert(out[i]==expected);
        }
      }
      uint8_t out[8];std::memcpy(out,stock,8);
      const bool changed=r79Mode2ApplyBitsPure(out,preserve);
      assert(changed==((stock[2]&0x08)!=0 || (!preserve && (stock[5]&0x80)==0)));
      for (unsigned i=0;i<8;++i) assert(out[i]==(i==2?(uint8_t)(stock[i]&0xF7):
          i==5?(preserve?stock[i]:(uint8_t)(stock[i]|0x80)):stock[i]));
    }
    uint8_t defaultFixed[8],defaultMode2[8];
    std::memcpy(defaultFixed,stock,8);std::memcpy(defaultMode2,stock,8);
    r79FixedApplyBitsPure(defaultFixed,0);r79Mode2ApplyBitsPure(defaultMode2);
    assert(defaultFixed[5]==(uint8_t)(stock[5]|0x80));
    assert(std::memcmp(defaultFixed,defaultMode2,8)==0);
  }
  r79FixedApplyBitsPure(nullptr,0,true);assert(!r79Mode2ApplyBitsPure(nullptr,true));
}
