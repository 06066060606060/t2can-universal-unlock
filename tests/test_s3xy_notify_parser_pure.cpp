#include <cassert>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include "fixtures/s3xy_notify_legacy.h"
#include "../s3xy_notify_pure.h"
static unsigned comparisons=0;
static unsigned legacy(const uint8_t *data,size_t len){
  return (s3xyFindToken2(data,len,0xC1,0x01)>=0?1u:0u)|
         (s3xyFindToken2(data,len,0xC1,0x02)>=0?2u:0u)|
         (s3xyFindToken2(data,len,0xC3,0x01)>=0?4u:0u)|
         (s3xyFindToken3(data,len,0xC7,0x00,0x01,0xC3)>=0?8u:0u);
}
static unsigned current(const uint8_t *data,size_t len){
  const auto t=s3xyParseNotifyTokensPure(data,len);
  return (t.single?1u:0u)|(t.dbl?2u:0u)|(t.lng?4u:0u)|(t.ack?8u:0u);
}
static void check(const uint8_t *data,size_t len){assert(legacy(data,len)==current(data,len));++comparisons;}
static uint32_t rng=1;
static uint32_t randomWord(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static volatile unsigned checksum=0;
static unsigned (*volatile benchmarkLegacy)(const uint8_t*,size_t)=legacy;
static unsigned (*volatile benchmarkCurrent)(const uint8_t*,size_t)=current;
int main(){
  for(size_t len=0;len<16;++len)check(nullptr,len);
  // Exhaustive relevant alphabet includes both ACK endings, overlapping ACK
  // and long tokens, duplicate gestures, and every multi-gesture ambiguity.
  const uint8_t alphabet[]={0,1,2,0xC1,0xC3,0xC7,0xFF};uint8_t data[512]={};
  for(size_t len=0;len<=6;++len){unsigned total=1;for(size_t i=0;i<len;++i)total*=7;
    for(unsigned n=0;n<total;++n){unsigned code=n;for(size_t i=0;i<len;++i){data[i]=alphabet[code%7];code/=7;}check(data,len);}}
  for(unsigned n=0;n<100000;++n){size_t len=randomWord()%513;for(size_t i=0;i<len;++i)data[i]=uint8_t(randomWord());
    if(len>8&&n%2==0){size_t pos=randomWord()%(len-8);const uint8_t tokens[]={0xC1,1,0xC1,2,0xC7,0,0xC3,1};for(size_t j=0;j<8;++j)data[pos+j]=tokens[j];}check(data,len);}
  // Observational host benchmark, not a pass/fail timing threshold.
  for(size_t len:{size_t(2),size_t(64),size_t(512)}){
    for(size_t i=0;i<len;++i)data[i]=0;data[len-2]=0xC1;data[len-1]=1;
    auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<100000;++i)checksum+=benchmarkLegacy(data,len);auto middle=std::chrono::steady_clock::now();
    for(unsigned i=0;i<100000;++i)checksum+=benchmarkCurrent(data,len);auto end=std::chrono::steady_clock::now();
    std::printf("notify length=%zu legacy=%lldus single-pass=%lldus\n",len,(long long)std::chrono::duration_cast<std::chrono::microseconds>(middle-start).count(),(long long)std::chrono::duration_cast<std::chrono::microseconds>(end-middle).count());
  }
  std::printf("notify differential %u buffers PASS\n",comparisons);
}
