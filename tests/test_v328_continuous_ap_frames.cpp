#include <cassert>
#include <cstring>
#include <initializer_list>
#include "../continuous_ap_frames_pure.h"
int main(){
  ContApStockFrame f; f.valid=true; f.route=ContApRoute::ChassisB; f.id=0x399; f.dlc=8;
  ContApDecodedInputs d;
  assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==0);
  for(int a=0;a<16;a++){f.data[0]=a;continuousApDecodeFramePure(f,d);assert(d.value==a);}
  f.id=0x3f5;f.data[0]=8;assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.right==2&&d.left==0);
  f.data[0]=3;continuousApDecodeFramePure(f,d);assert(!d.valid);
  f.id=0x39d;f.dlc=3;f.data[2]=1;assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==1);
  f.data[2]=3;continuousApDecodeFramePure(f,d);assert(!d.valid);
  f.id=0x372;f.route=ContApRoute::BodyA;f.dlc=8;f.data[0]=0;f.data[2]=8;f.data[3]=2;
  assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==0);
  f.data[2]=8;f.data[3]=0xfc;continuousApDecodeFramePure(f,d);assert(d.valid&&d.value==250);
  f.data[2]=7;f.data[3]=8;continuousApDecodeFramePure(f,d);assert(d.valid&&d.value==-250);
  f.data[2]=15;f.data[3]=254;continuousApDecodeFramePure(f,d);assert(!d.valid);
  f.route=ContApRoute::ChassisB;assert(!continuousApDecodeFramePure(f,d));
  const uint8_t crc[16]={0x46,0x44,0x52,0x6d,0x43,0x41,0xdd,0xf9,0x4c,0xa5,0xf6,0x8c,0x49,0x2f,0x31,0x3b};
  for(int c=0;c<16;c++){
    assert(continuousApBuildStalkPure(0,c,f)); assert(f.data[0]==crc[c]&&f.data[1]==c&&f.data[2]==0);
    assert(continuousApStalkCrcValidPure(f));
    assert(continuousApBuildStalkPure(4,c,f));assert(f.data[0]==(crc[c]^0xf1));
    assert(continuousApStalkCrcValidPure(f));f.data[2]=1;assert(!continuousApStalkCrcValidPure(f));
  }
  f={};f.valid=true;f.route=ContApRoute::ChassisB;f.id=0x3c2;f.dlc=8;
  f.data[0]=0x29;f.data[1]=0x55;f.data[6]=0x10;f.data[7]=0xa6;
  ContApStockFrame out;
  assert(continuousApBuildClickPure(f,2,out)); assert(out.data[1]==0x65&&f.data[1]==0x55);
  for(int i=0;i<8;i++)if(i!=1)assert(out.data[i]==f.data[i]);
  assert(continuousApBuildClickPure(f,1,out)&&out.data[1]==0x55);
  for(int raw:{0,3})assert(!continuousApBuildClickPure(f,raw,out));
  f.data[3]=1;assert(!continuousApBuildClickPure(f,2,out));f.data[3]=0;
  f.data[6]=0;assert(!continuousApBuildClickPure(f,2,out));f.data[6]=0x10;
  f.data[0]&=~3;assert(!continuousApBuildClickPure(f,2,out));
}
