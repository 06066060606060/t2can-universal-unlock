#include <cassert>
#include <cstring>
#include <initializer_list>
#include "../vision_speed_control_pure.h"
int main() {
  for(unsigned byte=0;byte<256;++byte)for(unsigned page=0;page<8;++page)for(bool active:{false,true}) {
    uint8_t data[8]={uint8_t(0xE0|page),0xAA,0x55,0xFF,0x77,0xD3,uint8_t(byte),0x98};
    uint8_t expected[8];memcpy(expected,data,8);
    const bool changed=active&&page==1&&(byte&2)!=0;
    if(changed)expected[6]=uint8_t(byte&0xFD);
    assert(visionControlApplyPure(data,8,active)==changed);
    assert(memcmp(data,expected,8)==0);
    if(page==1) {auto before=data[6];assert(!visionControlApplyPure(data,8,active));assert(data[6]==before);}
  }
  uint8_t stock[8]={1,0,0,0,0,0,2,0};
  assert(!visionControlApplyPure(nullptr,8,true));assert(!visionControlApplyPure(stock,7,true));
  assert(visionControlFramePure(0x3FD,8,false,false,stock));
  assert(!visionControlFramePure(0x3FD,8,true,false,stock));assert(!visionControlFramePure(0x3FD,8,false,true,stock));
  assert(!visionControlFramePure(0x399,8,false,false,stock));assert(!visionControlFramePure(0x3FD,7,false,false,stock));
  unsigned combinations=0;
  for(uint8_t model=0;model<7;++model)for(uint8_t topology=0;topology<5;++topology) {
    bool chassis=model==1?topology==1:model>=2&&model<=5&&(topology==2||topology==3);
    assert(visionControlBusSupportedPure(model,topology,0)==chassis);
    assert(visionControlBusSupportedPure(model,topology,1)==(model>=2&&model<=5&&topology==2));
    assert(!visionControlBusSupportedPure(model,topology,2));if(chassis)++combinations;
  }
  assert(combinations==9);
  for(uint8_t ap=0;ap<16;++ap)for(unsigned invalid=0;invalid<=7;++invalid) {
    bool selected=true,lab=true,supported=true,transport=true,apValid=true,stockValid=true;
    uint32_t apMs=100,rxMs=100;
    if(invalid==1)selected=false;if(invalid==2)lab=false;if(invalid==3)supported=false;
    if(invalid==4)transport=false;if(invalid==5)apValid=false;if(invalid==6)stockValid=false;if(invalid==7)rxMs=99;
    const bool shouldPass=(invalid==0||invalid==2)&&ap>=3&&ap<=6;
    assert(visionControlGatePure(selected,lab,supported,transport,apValid,ap,3100,apMs,stockValid,rxMs)==shouldPass);
  }
  assert(visionControlGatePure(true,false,true,true,true,3,3100,100,true,100));
  assert(!visionControlGatePure(true,true,true,true,true,3,3101,100,true,100));
  assert(visionControlGatePure(true,true,true,true,true,3,20,UINT32_MAX-20,true,UINT32_MAX-20));
}
