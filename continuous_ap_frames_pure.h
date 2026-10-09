#pragma once
#include "continuous_ap_types.h"
static inline uint8_t continuousApStalkCrcPure(uint8_t pos,uint8_t counter) {
  static const uint8_t neutral[16]={0x46,0x44,0x52,0x6d,0x43,0x41,0xdd,0xf9,0x4c,0xa5,0xf6,0x8c,0x49,0x2f,0x31,0x3b};
  static const uint8_t delta[5]={0,0xe0,0xef,0x0f,0xf1};
  return neutral[counter&15]^delta[pos];
}
static inline bool continuousApStalkCrcValidPure(const ContApStockFrame &f) {
  const uint8_t pos=f.data[1]>>4;
  return f.valid&&f.id==0x229&&f.dlc==3&&f.route==ContApRoute::BodyA&&pos<=4&&f.data[2]==0
      &&f.data[0]==continuousApStalkCrcPure(pos,f.data[1]&15);
}
static inline bool continuousApScrollIdlePure(const ContApStockFrame &f) {
  const uint8_t *d=f.data;
  return f.valid&&f.id==0x3c2&&f.dlc==8&&(d[0]&3)==1&&(d[6]&0x10)
      &&((d[1]>>4)&3)==1&&(d[1]&3)==1&&((d[1]>>2)&3)==1
      &&((d[1]>>6)&3)==1&&((d[0]>>3)&3)==1&&((d[0]>>5)&3)==1
      &&(d[2]&63)==0&&(d[3]&63)==0&&(d[5]&6)==0;
}
static inline bool continuousApDecodeFramePure(const ContApStockFrame &f,ContApDecodedInputs &o) {
  o={}; const uint8_t *d=f.data; const bool b=f.route==ContApRoute::ChassisB;
  if(b&&f.id==0x399) o.signal=ContApSignal::Ap;
  else if(b&&f.id==0x3f5) o.signal=ContApSignal::Turn;
  else if(b&&f.id==0x39d) o.signal=ContApSignal::Brake;
  else if(b&&(f.id==0x118||f.id==0x186)) o.signal=ContApSignal::Gear;
  else if((f.route==ContApRoute::BodyA&&(f.id==0x372||f.id==0x373))||(f.route==ContApRoute::PartyA&&f.id==0x370)) o.signal=ContApSignal::Torque;
  else if(f.route==ContApRoute::BodyA&&f.id==0x229) o.signal=ContApSignal::Stalk;
  else if((b||f.route==ContApRoute::BodyA)&&f.id==0x3c2) { if(f.dlc==8&&(d[0]&3)!=1)return false; o.signal=ContApSignal::Scroll; }
  else return false;
  if(!f.valid||f.dlc>8) return true;
  if(o.signal==ContApSignal::Brake) {
    if(f.dlc!=3&&f.dlc!=5)return true;
    o.value=d[2]&3;o.valid=o.value==1||o.value==2;return true;
  }
  if(o.signal==ContApSignal::Stalk) {
    o.valid=continuousApStalkCrcValidPure(f);o.value=d[1]>>4;o.idle=o.valid&&o.value==0;return true;
  }
  if(f.dlc!=8)return true;
  o.valid=true;
  switch(o.signal){
    case ContApSignal::Ap:o.value=d[0]&15;break;
    case ContApSignal::Turn:o.left=d[0]&3;o.right=(d[0]>>2)&3;o.value=d[0];o.valid=o.left<3&&o.right<3;break;
    case ContApSignal::Gear:o.value=(d[2]>>5)&7;o.valid=o.value>=1&&o.value<=4;break;
    case ContApSignal::Torque:{uint16_t raw=((uint16_t)(d[2]&15)<<8)|d[3];o.value=(int32_t)raw-2050;o.valid=raw<4094&&(d[0]&4)==0;break;}
    case ContApSignal::Scroll:o.value=(d[1]>>4)&3;o.valid=(o.value==1||o.value==2)&&(d[6]&0x10);o.idle=o.valid&&continuousApScrollIdlePure(f);break;
    default:o.valid=false;break;
  }
  return true;
}
static inline bool continuousApBuildStalkPure(uint8_t pos,uint8_t counter,ContApStockFrame &out) {
  if(pos!=0&&pos!=4)return false;
  out={};out.valid=true;out.route=ContApRoute::BodyA;out.id=0x229;out.dlc=3;
  out.data[1]=(pos<<4)|(counter&15);out.data[0]=continuousApStalkCrcPure(pos,counter);return true;
}
static inline bool continuousApBuildClickPure(const ContApStockFrame &stock,uint8_t pressed,ContApStockFrame &out) {
  if((pressed!=1&&pressed!=2)||!continuousApScrollIdlePure(stock))return false;
  out=stock;out.data[1]=(stock.data[1]&0xcf)|(pressed<<4);return true;
}
