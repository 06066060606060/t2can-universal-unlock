#include <cassert>
#include <cstring>
#include "../country_override_pure.h"
int main(){
 for(unsigned country=0;country<4;++country)for(unsigned map=0;map<3;++map)for(unsigned source=0;source<16;++source){
  uint8_t d[8]={3,uint8_t(0xB0|source),0x12,0x34,0x56,0x78,0x9A,0xBC},stock[8];memcpy(stock,d,8);
  unsigned target=map==1?0:7;
  bool expected=map!=0&&source<=10&&source!=target;
  assert(countryOverrideApply7ffPure(d,8,country,map)==expected);
  assert(d[1]==(expected?uint8_t(0xB0|target):stock[1]));
  for(unsigned i=0;i<8;++i)if(i!=1)assert(d[i]==stock[i]);
 }
 for(unsigned country=0;country<4;++country)for(unsigned map=0;map<3;++map){
  uint8_t d[8]={1,0xB6,'E','D',0x56,0x78,0x9A,0xBC},stock[8];memcpy(stock,d,8);
  assert(countryOverrideApply7ffPure(d,8,country,map)==(country!=0));
  if(country){assert(d[2]==(country==1?'S':country==2?'R':'Z'));assert(d[3]==(country==1?'U':country==2?'K':'N'));}
  for(unsigned i=0;i<8;++i)if(i!=2&&i!=3)assert(d[i]==stock[i]);
 }
 for(unsigned country=0;country<256;++country)for(unsigned map=3;map<255;++map){
  uint8_t d[8]={3,6,1,2,3,4,5,6},stock[8];memcpy(stock,d,8);
  assert(!countryOverrideApply7ffPure(d,8,country,map));assert(!memcmp(d,stock,8));
 }
 for(unsigned legacy=0;legacy<4;++legacy)assert(mapRegionFromLegacyCountryPure(legacy)==(legacy==1?1:legacy==2?2:0));
 assert(!countryOverrideApply7ffPure(nullptr,8,1,1));
}
