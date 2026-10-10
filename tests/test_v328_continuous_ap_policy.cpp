#include <cassert>
#include "../continuous_ap_policy_pure.h"
int main() {
  assert(continuousApGestureRoutePure(3,2,ContApMethod::StalkDouble)==ContApRoute::BodyA);
  for(uint8_t p=0;p<7;p++) for(uint8_t t=0;t<5;t++) for(uint8_t m=0;m<5;m++) {
    const bool legacy=p==3||p==5, refresh=p==2||p==4;
    const bool want=(t==2&&legacy&&m==1)||((t==2||t==3)&&refresh&&(m==2||m==3));
    assert(vehicleProfileContinuousApMethodSupported(p,t,m)==want);
  }
  ContApConfig c;
  assert(continuousApConfigEncodePure({true,ContApMethod::StalkDouble})==0x105);
  assert(continuousApConfigEncodePure({true,ContApMethod::ScrollSingle})==0x106);
  assert(continuousApConfigEncodePure({true,ContApMethod::ScrollDouble})==0x107);
  for(uint32_t raw=0;raw<1024;raw++) {
    const bool want=raw>=0x100&&raw<=0x107&&raw!=0x104;
    assert(continuousApConfigDecodePure(raw,c)==want);
    if(want) assert(continuousApConfigEncodePure(c)==raw);
    else assert(!c.enabled&&c.method==ContApMethod::Unset);
  }
}
