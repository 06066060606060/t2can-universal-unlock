"""Exercise actual MUX1 final transmit composition with platform doubles."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT.parent/'T2CAN-Universal-v3.18.0-LP_YL' if os.environ.get('VISION3FD_BASELINE') == '1' else ROOT
logic=(SOURCE/'vehicle_logic.h').read_text()
def function(name):
 match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',logic,re.M)
 assert match,name
 end=logic.index('{',match.start())+1;depth=1
 while depth:
  depth+=(logic[end]=='{')-(logic[end]=='}');end+=1
 return logic[match.start():end].replace('static ', '[[maybe_unused]] static ', 1)+'\n'

code=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "vision_speed_control_pure.h"
bool r79DmsApplyFinal(uint8_t*){return false;}
#include "runtime_gate_pure.h"
using esp_err_t=int;
[[maybe_unused]] constexpr int ESP_FAIL=7;
constexpr int ESP_OK=0,ESP_ERR_INVALID_ARG=1,ESP_ERR_INVALID_STATE=2,pdTRUE=1;
#define pdMS_TO_TICKS(x) (x)
struct twai_message_t{uint32_t identifier=0x3FD;uint8_t data_length_code=8;bool extd=false,rtr=false;uint8_t data[8]={1,0,0x0C,0,0,0x80,0,0};};
int canTxBarrierMutex=1,stateMux=0,r79LabMux=0;bool held=false,busy=false;
void(*takeHook)()=nullptr;
int xSemaphoreTake(int,int){if(takeHook){auto h=takeHook;takeHook=nullptr;h();}if(busy)return 0;assert(!held);held=true;return 1;}
void xSemaphoreGive(int){assert(held);held=false;}
void portENTER_CRITICAL(int*){}void portEXIT_CRITICAL(int*){}
uint32_t fakeNow=100,lastDASStatusMillis=100,r79LabLastStockMs=100;
uint32_t millis(){return fakeNow;}
bool dasAutopilotStateValid=true;uint8_t dasAutopilotState4=3;
bool labMenuEnabled=true,twaiReady=true,mcpReady=true,canTxAdministrativeHold=false;
uint8_t activeVehicleProfile=3,activeVehicleTopology=2;
bool r79LabStockValid=true;
uint8_t readMuxID(const uint8_t*d){return d[0]&7;}
volatile uint32_t r79ApGateGeneration=1;
volatile uint32_t &generation=r79ApGateGeneration;bool r79Gate=false,pending=false,admission=true;
uint32_t r79ApGateGenerationSnapshot(){return generation;}
bool r79FastReactiveGateOpen(){return r79Gate;}
bool r79DmsWorkPendingSnapshot(){return pending;}
bool twaiNonSummonAdmissionOpen(){return admission;}
uint32_t canTxCancellationGenerationSnapshot(const volatile uint32_t *p){return *p;}
struct Barrier{uint32_t epoch;uint8_t freshMask;}canTxBarrierState{1,3};
constexpr uint8_t CAN_TX_FRESH_VH=2;[[maybe_unused]] constexpr uint8_t CAN_TX_FRESH_PARTY=1;
[[maybe_unused]] constexpr uint8_t CAN_TX_TRACE_SOURCE_DEFAULT=0;
uint32_t canTxEpochSnapshot(){return canTxBarrierState.epoch;}
bool canTxBarrierAllowsMaskedPure(Barrier b,uint32_t epoch,uint8_t mask){return b.epoch==epoch&&(b.freshMask&mask)==mask;}
struct can_frame{uint32_t can_id=0x3FD;uint8_t can_dlc=8;uint8_t data[8]={1,0,0x0C,0,0,0x80,0xA2,0};};
can_frame sentA{},tracedA{};int sendsA=0,tracesA=0;bool mcpFail=false;
struct MCP2515{enum ERROR{ERROR_OK,ERROR_FAIL};ERROR sendMessage(const can_frame*f){assert(held);sentA=*f;++sendsA;return mcpFail?ERROR_FAIL:ERROR_OK;}}Can_A;
void canATraceRecordTx(const can_frame*f,McpTxResultReason,MCP2515::ERROR,uint8_t){tracedA=*f;++tracesA;}
twai_message_t sent{},traced{};int sends=0,traces=0;esp_err_t result=0;
esp_err_t twai_transmit(const twai_message_t*f,int){assert(held);sent=*f;++sends;return result;}
esp_err_t can3fdTimingTransmit(const twai_message_t*f,int wait,uint8_t){return twai_transmit(f,wait);}
void canBTraceRecordTx(const twai_message_t*f,esp_err_t){traced=*f;++traces;}
#if __has_include("vision_speed_control_pure.h")
#include "vision_speed_control_pure.h"
#endif
bool visionControlRequestDisabledState=true;uint8_t visionControlBus=0;
VisionControlStockPure visionControlStock[2]{};
uint32_t visionControlTxOk=0,visionControlTxFail=0;
uint32_t visionControlGeneration=1;
'''
for name in ('visionControlSupported','visionControlBusSupported','visionControlBusSnapshot','visionControlRequestDisabled','visionControlGateOpen','visionControlApplyFinal','visionControlRecordTx','visionControlCacheStock'):
 if re.search(r'^static\s+[^;{}]*?\b'+name+r'\(',logic,re.M): code+=function(name)
for name in ('mux1DisplayTransmit','r79ApGateTransmitGuarded','r79LabDirectTwaiTransmitGuarded','visionControlObserveStock','visionControlObserveBody'):
 code+=function(name)
code+=r'''
void reset(){fakeNow=lastDASStatusMillis=r79LabLastStockMs=100;canTxBarrierState={1,3};
 activeVehicleProfile=3;activeVehicleTopology=2;dasAutopilotStateValid=true;dasAutopilotState4=3;
 labMenuEnabled=twaiReady=mcpReady=admission=true;canTxAdministrativeHold=busy=mcpFail=pending=false;takeHook=nullptr;
 generation=1;r79Gate=false;sends=sendsA=traces=tracesA=0;result=0;
 visionControlRequestDisabledState=true;visionControlBus=0;visionControlTxOk=visionControlTxFail=0;
 visionControlStock[0]={};visionControlStock[1]={};}
void releaseAp(){dasAutopilotState4=2;}
void expireRx(){fakeNow=3101;lastDASStatusMillis=3101;}
void recover(){++canTxBarrierState.epoch;}
void settingOff(){visionControlRequestDisabledState=false;++generation;}
void labOff(){labMenuEnabled=false;}
void selectBody(){visionControlBus=1;++generation;}
int main(){
 twai_message_t stock;stock.data[6]=0xA2;
 r79Gate=true;
 visionControlStock[0].valid=true;visionControlStock[0].lastMs=100;visionControlStock[0].epoch=1;
 visionControlStock[0].profile=3;visionControlStock[0].topology=2;memcpy(visionControlStock[0].raw,stock.data,8);
 assert(r79ApGateTransmitGuarded(&stock,0,1)==ESP_OK);
 assert(sends==1&&sent.data[6]==0xA0);
 assert(stock.data[6]==0xA2);
 // The final R79 retry/delay sender composes the same independent policy.
 reset();r79Gate=true;visionControlCacheStock(0,stock.data,1,100);volatile uint32_t token=1;
 assert(r79LabDirectTwaiTransmitGuarded(&stock,0,&token,1,1)==ESP_OK);assert(sent.data[6]==0xA0&&visionControlTxOk==1);
 // All profile/topology combinations, both selectable routes, and every AP state.
 can_frame body;
 for(uint8_t profile=1;profile<=5;++profile)for(uint8_t topology=1;topology<=3;++topology)
 for(uint8_t bus=0;bus<2;++bus)for(uint8_t ap=0;ap<16;++ap){
  reset();activeVehicleProfile=profile;activeVehicleTopology=topology;visionControlBus=bus;dasAutopilotState4=ap;
  bool valid=vehicleProfileTopologyValid(profile,topology)&&(bus==0||vehicleProfileCanAIsBody(profile,topology))&&ap>=3&&ap<=6;
  if(bus==0){visionControlCacheStock(0,stock.data,1,100);visionControlObserveStock(stock,1,false,100);assert(sends==int(valid));if(sends)assert(sent.data[6]==0xA0);}
  else{visionControlObserveBody(body,1,100);assert(sendsA==int(valid));if(sendsA)assert(sentA.data[6]==0xA0);}
 }
 // A missing selected-bus template never falls back to the other physical bus.
 reset();visionControlBus=1;visionControlCacheStock(0,stock.data,1,100);visionControlObserveStock(stock,1,false,100);assert(sends==0);
 reset();visionControlCacheStock(1,body.data,1,100);visionControlObserveBody(body,1,100);assert(sendsA==0);
 // Raw zero emits nothing extra, and diagnostics count actual changed-bit attempts only.
 reset();auto zero=stock;zero.data[6]=0xA0;visionControlCacheStock(0,zero.data,1,100);visionControlObserveStock(zero,1,false,100);assert(!sends);
 r79Gate=true;assert(r79ApGateTransmitGuarded(&zero,0,1)==ESP_OK);assert(sends==1&&!visionControlTxOk);
 reset();visionControlBus=1;auto bodyZero=body;bodyZero.data[6]=0xA0;visionControlObserveBody(bodyZero,1,100);assert(!sendsA);
 // Retired Lane Graph bit45 stays stock on Chassis and Body, even with Vision enabled.
 for(int bit=0;bit<2;++bit){
  reset();auto sample=stock;sample.data[5]=bit?0xA0:0x80;
  visionControlCacheStock(0,sample.data,1,100);visionControlObserveStock(sample,1,false,100);
  assert(sends==1&&sent.data[5]==sample.data[5]&&sent.data[6]==0xA0);
  reset();visionControlBus=1;auto sampleBody=body;sampleBody.data[5]=bit?0xA0:0x80;
  visionControlObserveBody(sampleBody,1,100);
  assert(sendsA==1&&sentA.data[5]==sampleBody.data[5]&&sentA.data[6]==0xA0);
  reset();r79Gate=true;visionControlRequestDisabledState=false;
  assert(r79ApGateTransmitGuarded(&sample,0,1)==ESP_OK);
  assert(sends==1&&sent.data[5]==sample.data[5]&&sent.data[6]==sample.data[6]);
 }
 // In Park with inactive AP, selected Vision never sends a parked test clone.
 reset();dasAutopilotState4=2;visionControlCacheStock(0,stock.data,1,100);
 visionControlObserveStock(stock,1,false,100);assert(!sends);
 reset();visionControlBus=1;dasAutopilotState4=2;visionControlObserveBody(body,1,100);assert(!sendsA);
 // A claimed R79/ULC clone or pending R79 work owns the Chassis frame.
 for(bool claimed:{false,true}){reset();visionControlCacheStock(0,stock.data,1,100);pending=!claimed;visionControlObserveStock(stock,1,claimed,100);assert(!sends);}
 // Final admission rechecks expiration, AP, epoch and cancellation after mutex acquisition.
 for(auto hook:{releaseAp,expireRx,recover,settingOff,selectBody}){
  reset();visionControlCacheStock(0,stock.data,1,100);takeHook=hook;visionControlObserveStock(stock,1,false,100);assert(!sends&&!held);
  reset();visionControlBus=1;takeHook=hook;visionControlObserveBody(body,1,100);assert(!sendsA&&!held);
 }
 // Production Visual Speed Control remains active when the LAB menu is OFF.
 reset();visionControlCacheStock(0,stock.data,1,100);takeHook=labOff;visionControlObserveStock(stock,1,false,100);assert(sends==1&&!held);
 reset();visionControlBus=1;takeHook=labOff;visionControlObserveBody(body,1,100);assert(sendsA==1&&!held);
 // In R79-owned sends AP/stock loss preserves stock bit49 while existing enqueue continues.
 for(auto hook:{releaseAp,expireRx}){reset();r79Gate=true;visionControlCacheStock(0,stock.data,1,100);takeHook=hook;
  assert(r79ApGateTransmitGuarded(&stock,0,1)==ESP_OK);assert(sends==1&&sent.data[6]==0xA2&&visionControlTxOk==0);}
 // Pre-observer delays retain actual received timestamp, including exact boundary and wrap.
 reset();fakeNow=lastDASStatusMillis=3101;visionControlCacheStock(0,stock.data,1,100);visionControlObserveStock(stock,1,false,100);assert(!sends&&visionControlStock[0].lastMs==100);
 reset();fakeNow=lastDASStatusMillis=3100;visionControlCacheStock(0,stock.data,1,100);visionControlObserveStock(stock,1,false,100);assert(sends==1);
 reset();fakeNow=20;lastDASStatusMillis=UINT32_MAX-20;visionControlCacheStock(0,stock.data,1,UINT32_MAX-20);visionControlObserveStock(stock,1,false,UINT32_MAX-20);assert(sends==1);
 // Transport and stock profile/epoch mismatch close only the new overlay admission.
 for(unsigned reason=0;reason<8;++reason){reset();visionControlCacheStock(0,stock.data,1,100);
  if(reason==0)canTxAdministrativeHold=true;if(reason==1)twaiReady=false;if(reason==2)busy=true;
  if(reason==3)canTxBarrierState.freshMask=1;if(reason==4)++canTxBarrierState.epoch;
  if(reason==5)dasAutopilotStateValid=false;if(reason==6)visionControlStock[0].profile=1;if(reason==7)visionControlRequestDisabledState=false;
  visionControlObserveStock(stock,1,false,100);assert(!sends);}
 reset();result=ESP_FAIL;visionControlCacheStock(0,stock.data,1,100);visionControlObserveStock(stock,1,false,100);assert(sends==1&&visionControlTxFail==1&&visionControlTxOk==0);
 reset();visionControlBus=1;mcpFail=true;visionControlObserveBody(body,1,100);assert(sendsA==1&&visionControlTxFail==1);
 // MUX1 is the only page eligible; standard-ID/DLC checks reject malformed templates.
 for(unsigned reason=0;reason<5;++reason){reset();auto bad=stock;bad.data[0]=reason==0?0:1;
  if(reason==1)bad.extd=true;if(reason==2)bad.rtr=true;if(reason==3)bad.data_length_code=7;if(reason==4)bad.identifier=0x399;
  visionControlObserveStock(bad,1,false,100);assert(!sends);}
}
'''
with tempfile.TemporaryDirectory() as d:
 source=Path(d)/'vision3fd.cpp';exe=Path(d)/'vision3fd';source.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Production Visual Speed Control actual runtime composition PASS')
