#include <cassert>
#include <cstdint>
#include "../tsl9_hands_on_0x399_pure.h"
#include "../vehicle_profile.h"
int main() {
  // Every supported topology has an independent ISA capability; preserve Nag.
  for (uint8_t model=1; model<=5; ++model) {
    for (uint8_t topology=0; topology<=4; ++topology) {
      const bool valid = model==VEHICLE_MODEL_YL ? topology==1u
                                                : topology==2u || topology==3u;
      assert(vehicleProfileIsaSuppressionSupported(model,topology)==valid);
      if (model!=VEHICLE_MODEL_YL && topology==3u)
        assert(!vehicleProfileNagTsl9Supported(model,topology));
    }
  }
  assert(!vehicleProfileIsaSuppressionSupported(0u,2u));
  assert(!vehicleProfileIsaSuppressionSupported(255u,3u));
  // All AP nibble values: ISA blocks unknown/inactive states and accepts 3..6.
  for (uint8_t ap=0; ap<16; ++ap) {
    uint8_t data[8]={ap,0x85u,0,0,0,0xFFu,0,0};
    assert(isaSuppressSpeedWarningPure(data,8u)==(ap>=3u && ap<=6u));
    assert(data[1]==(ap>=3u && ap<=6u ? 0xA5u : 0x85u));
  }
  uint8_t shortFrame[8]={3u,0,0,0,0,0,0,0};
  assert(!isaSuppressSpeedWarningPure(nullptr,8u));
  assert(!isaSuppressSpeedWarningPure(shortFrame,7u));

  for (uint8_t ho=0; ho<16; ++ho) {
    uint8_t data[8]={3u,0u,0xAAu,0x55u,0x12u,(uint8_t)(ho<<2),0xF5u,0u};
    Tsl9HandsOnStatePure state={};
    auto result=tsl9ApplyDasTransformForCanIdPure(state,false,0u,0u,true,0x399u,data,8u,1000u);
    assert(result.isaModified && !result.handsOnModified);
    assert(data[1]==0x20u && data[5]==(ho<<2) && data[6]==5u);
    assert(data[2]==0xAAu && data[3]==0x55u && data[4]==0x12u);
    assert(data[7]==tsl9ChecksumForCanIdPure(0x399u,data));
    result=tsl9ApplyDasTransformForCanIdPure(state,false,0u,0u,true,0x399u,data,8u,1001u);
    assert(!result.modified && data[6]==5u);
  }
  return 0;
}
