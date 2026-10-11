#include <cassert>
#include "../tesla_can_fields_pure.h"
#include "../auto_lane_change_enable_pure.h"
#include "../country_override_pure.h"
#include "../tsl9_hands_on_0x399_pure.h"
#include "../runtime_gate_pure.h"
#include "../ulc_compositor_pure.h"
#include "../ulc_compositor_pure.h"
static uint8_t legacy(uint16_t id,const uint8_t *d) {
 uint16_t sum=(id>>8)+(id&255);
 for(unsigned i=0;i<7;++i)sum+=d[i];return uint8_t(sum);
}
int main(){
 uint8_t d[8]={};
 for(unsigned id=0;id<2048;++id)for(unsigned byte=0;byte<256;++byte){
  for(unsigned i=0;i<8;++i)d[i]=uint8_t(byte+i*37);
  assert(teslaAdditiveChecksumPure(id,d)==legacy(id,d));
  assert(tsl9ChecksumForCanIdPure(id|0xf800,d)==legacy(id,d));
  assert(ui293ChecksumPure(d)==legacy(0x293,d));
  assert(countryOverride238ChecksumPure(d)==legacy(0x238,d));
 }
 assert(ui293ChecksumPure(nullptr)==0&&countryOverride238ChecksumPure(nullptr)==0);
 for(unsigned state=0;state<256;++state)for(unsigned valid=0;valid<2;++valid)for(unsigned enabled=0;enabled<2;++enabled){
  bool ap=state>=3&&state<=6;
  assert(dasStateApActivePure(valid,state)==(valid&&ap));
  assert(tsl9ApActivePure(state)==ap);
  assert(ulcPolicyApGateOpenPure(enabled,valid,state)==(enabled&&valid&&ap));
  assert(uiAutoLaneChangeGateOpenPure(true,enabled,valid,state)==(enabled&&valid&&ap));
  assert(!uiAutoLaneChangeGateOpenPure(false,enabled,valid,state));
  assert(ulcNoConfirmGateOpenWithTimingPure(true,enabled,0,valid,state)==(enabled&&valid&&ap));
  assert(ulcNoConfirmGateOpenWithTimingPure(true,enabled,1,valid,state)==bool(enabled));
 }
}
