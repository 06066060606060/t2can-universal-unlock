"""Run retired-key cleanup and the surviving production 0x3F8 TX compositor."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
logic=(ROOT/'vehicle_logic.h').read_text()
core=(ROOT/'can_core.h').read_text()
def function(source,name):
 match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',source,re.M);assert match,name
 end=source.index('{',match.start())+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[match.start():end].replace('static ','[[maybe_unused]] static ',1)+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <initializer_list>
#include "ulc_compositor_pure.h"
#include "vehicle_profile.h"
#include "das_status_pure.h"
#include "ulc_policy_pure.h"
#include "runtime_gate_pure.h"
using esp_err_t=int;
[[maybe_unused]] constexpr int ESP_OK=0,ESP_ERR_INVALID_ARG=1,ESP_ERR_TIMEOUT=2,ESP_ERR_INVALID_STATE=3,pdTRUE=1,portMAX_DELAY=1000;
constexpr uint8_t CAN_TX_FRESH_BOTH=3,CAN_TX_TRACE_SOURCE_DEFAULT=0;
constexpr uint8_t LAB3F8_STOCK=255,LAB3F8_ALC_FORCE_ON=2;
struct twai_message_t {uint32_t identifier=0x3F8,flags=0;uint8_t data_length_code=8;bool extd=false,rtr=false;uint8_t data[8]={0xFE,0x25,0xF7,0xCA,0x39,0x80,0xE2,0x16};};
int canTxBarrierMutex=1,stateMux=0,lab3f8Mux=0;bool held=false,busy=false;
void (*takeHook)()=nullptr;
int xSemaphoreTake(int,int) {if(takeHook){auto h=takeHook;takeHook=nullptr;h();}if(busy)return 0;assert(!held);held=true;return 1;}
void xSemaphoreGive(int) {assert(held);held=false;}
void portENTER_CRITICAL(int*) {} void portEXIT_CRITICAL(int*) {}
uint32_t fakeNow=100,lastDASStatusMillis=100;
uint32_t millis(){return fakeNow;}
bool dasAutopilotStateValid=true,labMenuEnabled=true,twaiReady=true,canTxAdministrativeHold=false,admission=true;
uint8_t dasAutopilotState4=3,activeVehicleProfile=1,activeVehicleTopology=1;
bool twaiNonSummonAdmissionOpen(){return admission;}
bool activeProfileUlcNoConfirmSupported(){return vehicleProfileTopologyValid(activeVehicleProfile,activeVehicleTopology);}
struct Barrier {uint32_t epoch;uint8_t mask;} canTxBarrierState{1,3};
bool canTxBarrierAllowsMaskedPure(Barrier b,uint32_t e,uint8_t m){return b.epoch==e&&(b.mask&m)==m;}
uint32_t canTxEpochSnapshot(){return canTxBarrierState.epoch;}
uint32_t canTxCancellationGenerationSnapshot(const volatile uint32_t*g){return *g;}
int sends=0;esp_err_t sendResult=0;twai_message_t sent{};
esp_err_t twai_transmit(const twai_message_t*out,int){assert(held);sent=*out;++sends;return sendResult;}
esp_err_t can3fdTimingTransmit(const twai_message_t*out,int wait,uint8_t=0){return twai_transmit(out,wait);}
void canBTraceRecordTx(const twai_message_t*,esp_err_t,uint8_t){}
void researchCaptureObserveTxVh(uint16_t,uint8_t,const uint8_t*,bool){}
bool getBit(const uint8_t*d,uint8_t bit){return (d[bit/8]>>(bit%8))&1;}
uint32_t readBitsLE(const uint8_t*d,uint8_t start,uint8_t length){uint32_t v=0;for(unsigned i=0;i<length;++i)v|=uint32_t(getBit(d,start+i))<<i;return v;}
volatile uint32_t lab3f8Generation=1;
uint32_t lab3f8FrameRxMs=100,lab3f8FrameRxEpoch=1;
bool lab3f8CanBValid=false;uint32_t lab3f8CanBLastMs=0,lab3f8CanBEpoch=0,lab3f8CanBPeriodMs=0,lab3f8CanBRx=0;
uint8_t lab3f8CanBDlc=0,lab3f8CanBData[8]={};
uint8_t lab3f8AlcMode=0,lab3f8UlcBlindMode=255,lab3f8UlcOffHighwayMode=255,ulcNoConfirmTimingMode=0;
bool uiUlcStalkConfirm=true,ulcNoConfirmEnabled=false;
bool summonHeartbeatOverrideEnabled=false,summonHeartbeatLastAppliedValid=false;
uint8_t summonHeartbeatOverrideValue=2,summonHeartbeatLastAppliedValue=255;
uint32_t summonHeartbeatLastAppliedMs=0,summonHeartbeatAppliedCount=0,summonHeartbeatTxFail=0,summonHeartbeatBlocked=0;
bool summonHeartbeatOverrideSupported(){return vehicleProfileTopologyValid(activeVehicleProfile,activeVehicleTopology);}
uint32_t lab3f8GateBlocked=0,lab3f8TxFail=0,lab3f8TxOk=0,lab3f8LastTxMs=0;
bool lab3f8LastTxValid=false,lab3f8LastTxBlindChanged=false,lab3f8LastTxBit56=false;
uint8_t lab3f8LastTxBlind=0,lab3f8LastTxStockBlind=0,lab3f8LastTxSelectedBlind=0,lab3f8LastTxResult=0;
uint32_t ulcOffHighwayGateBlocked=0,ulcOffHighwayTxFail=0,ulcOffHighwayTxOk=0,ulcOffHighwayLastTxMs=0;
bool ulcOffHighwayLastTxValid=false;uint8_t ulcOffHighwayLastTxRaw=0;
uint32_t ulcNoConfirmGateBlocked=0,ulcNoConfirmGateBlockedB=0,ulcNoConfirmTxFail=0,ulcNoConfirmTxBFail=0,ulcNoConfirmTxOk=0,ulcNoConfirmTxBOk=0,ulcNoConfirmLastTxMs=0;
bool ulcNoConfirmLastTxValid=false,ulcNoConfirmLastTxBit1=false;
struct String:std::string {using std::string::string;String(const std::string&s):std::string(s){}};
std::map<std::string,uint16_t> nvs;bool openOk=true,removeOk=true;int opens=0,ends=0;
std::vector<std::string> removed;
struct Preferences {std::string ns;bool begin(const char*n,bool readOnly){++opens;assert(std::string(n)=="lab3f8"&&!readOnly);ns=n;return openOk;}void end(){++ends;}
 bool isKey(const char*k){return nvs.count(ns+"/"+k);}
 bool remove(const char*k){assert(std::string(k)=="visionDisabled"||std::string(k)=="adaptiveOff");removed.push_back(ns+"/"+k);if(!removeOk)return false;return nvs.erase(ns+"/"+k)!=0;}};
std::map<std::string,std::string> json;
struct JsonWriterArduino {explicit JsonWriterArduino(String&){json.clear();}void finish(){}
 void boolean(const char*k,bool v){json[k]=v?"true":"false";}void u32(const char*k,uint32_t v){json[k]=std::to_string(v);}
 void i32(const char*k,int32_t v){json[k]=std::to_string(v);}void string(const char*k,const char*v){json[k]=v;}};
struct Server {std::map<String,String> parameters;int status=0;int args(){return parameters.size();}bool hasArg(const char*k){return parameters.count(k);}String arg(const char*k){return parameters[k];}void send(int c,const char*,const String&){status=c;}}server;
bool httpRequireLab(){if(labMenuEnabled)return true;server.status=403;return false;}
'''
start=logic.index('struct Lab3f8FinalContext {')
code+=logic[start:logic.index('};',start)+2]+'\n'
for name in ('retiredLabSpeedSettingsCleanup','lab3f8ObserveCanB','lab3f8FinalValidate'):
 code+=function(logic,name)
code+=function(core,'canTxTwaiTransmitValidated')
code+=function(logic,'injectDriverAssistControl')
code+=r'''
void resetRuntime(){fakeNow=lastDASStatusMillis=lab3f8FrameRxMs=100;lab3f8FrameRxEpoch=1;canTxBarrierState={1,3};
 dasAutopilotStateValid=labMenuEnabled=twaiReady=admission=true;dasAutopilotState4=3;activeVehicleProfile=1;activeVehicleTopology=1;
 canTxAdministrativeHold=busy=false;takeHook=nullptr;sends=0;sendResult=0;lab3f8Generation=1;
 lab3f8AlcMode=0;lab3f8UlcBlindMode=lab3f8UlcOffHighwayMode=255;ulcNoConfirmEnabled=false;
 summonHeartbeatOverrideEnabled=false;summonHeartbeatOverrideValue=2;summonHeartbeatLastAppliedValid=false;
 summonHeartbeatLastAppliedValue=255;summonHeartbeatLastAppliedMs=summonHeartbeatAppliedCount=summonHeartbeatTxFail=summonHeartbeatBlocked=0;}
void expireRx(){fakeNow=3101;}
void cancelSettings(){++lab3f8Generation;}
void recover(){++canTxBarrierState.epoch;}
void releaseAp(){dasAutopilotState4=2;}
int main(){
 // Only the two retired keys may be removed, even beside live feature/BLE data.
 nvs={{"lab3f8/visionDisabled",1},{"lab3f8/adaptiveOff",1},{"lab3f8/alc",2},{"lab3f8/ulcbs",2},{"labv3fd/selection",257},{"ulc/confirm",1},{"s3xy/paired",7},{"features/lab",1}};
 retiredLabSpeedSettingsCleanup();
 assert(opens==1&&ends==1&&removed.size()==2&&!nvs.count("lab3f8/visionDisabled")&&!nvs.count("lab3f8/adaptiveOff"));
 assert(nvs.size()==6&&nvs["lab3f8/alc"]==2&&nvs["lab3f8/ulcbs"]==2&&nvs["labv3fd/selection"]==257&&nvs["ulc/confirm"]==1&&nvs["s3xy/paired"]==7&&nvs["features/lab"]==1);
 removed.clear();retiredLabSpeedSettingsCleanup();assert(removed.empty()&&ends==2);
 nvs["lab3f8/visionDisabled"]=1;nvs["lab3f8/adaptiveOff"]=1;auto before=nvs;
 openOk=false;auto beforeEnds=ends;retiredLabSpeedSettingsCleanup();assert(nvs==before&&removed.empty()&&ends==beforeEnds);openOk=true;
 // Failed removal leaves saved ON, but there is no runtime path that can apply it.
 removeOk=false;retiredLabSpeedSettingsCleanup();assert(nvs==before&&removed.size()==2);removeOk=true;
 twai_message_t stock;stock.data[4]=0xB9;
 resetRuntime();injectDriverAssistControl(stock);assert(!sends);
 // Session override mirrors every valid stock frame and modifies bits 2-3 only.
 for(unsigned value=0;value<4;++value){
  resetRuntime();summonHeartbeatOverrideEnabled=true;summonHeartbeatOverrideValue=value;
  auto heartbeatStock=stock;const uint8_t before=heartbeatStock.data[0];
  injectDriverAssistControl(heartbeatStock);
  assert(sends==1&&summonHeartbeatAppliedCount==1&&summonHeartbeatLastAppliedValid);
  assert(summonHeartbeatLastAppliedValue==value&&((sent.data[0]>>2)&3u)==value);
  assert((sent.data[0]&0xF3u)==(before&0xF3u)&&memcmp(sent.data+1,heartbeatStock.data+1,7)==0);
 }
 // All four retained transformations run while retired bits stay exact stock.
 for(unsigned raw2=0;raw2<256;++raw2)for(unsigned raw4=0;raw4<256;++raw4){
  resetRuntime();lab3f8AlcMode=2;lab3f8UlcBlindMode=1;lab3f8UlcOffHighwayMode=1;ulcNoConfirmEnabled=true;
  stock.data[2]=raw2;stock.data[4]=raw4;injectDriverAssistControl(stock);
  assert(sends==1&&sent.data[2]==raw2&&sent.data[4]==raw4);
  const uint8_t expected[8]={0xFC,0xA5,uint8_t(raw2),0xCA,uint8_t(raw4),0x80,0xD2,0x17};
  assert(memcmp(sent.data,expected,8)==0&&stock.data[0]==0xFE&&stock.data[1]==0x25&&stock.data[6]==0xE2&&stock.data[7]==0x16);
 }
 // Shared protections survive removal: stale received stock, epoch and settings cancellation.
 for(auto hook:{expireRx,cancelSettings,recover,releaseAp}){
  resetRuntime();lab3f8AlcMode=2;takeHook=hook;injectDriverAssistControl(stock);assert(!sends&&!held);
 }
 for(unsigned reason=0;reason<6;++reason){resetRuntime();lab3f8AlcMode=2;
  if(reason==0)busy=true;if(reason==1)canTxAdministrativeHold=true;if(reason==2)twaiReady=false;
  if(reason==3)canTxBarrierState.mask=1;if(reason==4)admission=false;if(reason==5)lab3f8FrameRxEpoch=0;
  injectDriverAssistControl(stock);assert(!sends);
 }
 resetRuntime();lab3f8AlcMode=2;fakeNow=3100;injectDriverAssistControl(stock);assert(sends==1);
 resetRuntime();lab3f8AlcMode=2;fakeNow=3101;injectDriverAssistControl(stock);assert(!sends);
 resetRuntime();lab3f8AlcMode=2;fakeNow=20;lab3f8FrameRxMs=UINT32_MAX-20;injectDriverAssistControl(stock);assert(sends==1);
 for(unsigned reason=0;reason<5;++reason){resetRuntime();lab3f8AlcMode=2;auto bad=stock;
  if(reason==0)bad.extd=true;if(reason==1)bad.rtr=true;if(reason==2)bad.data_length_code=7;if(reason==3)bad.data_length_code=9;if(reason==4)bad.identifier=0x399;
  injectDriverAssistControl(bad);assert(!sends);
 }
 assert(nvs==before); // Runtime cannot revive or mutate saved retired values.
 removed.clear();retiredLabSpeedSettingsCleanup();
 assert(removed.size()==2&&!nvs.count("lab3f8/visionDisabled")&&!nvs.count("lab3f8/adaptiveOff"));
 assert(nvs.size()==6&&nvs["labv3fd/selection"]==257&&nvs["s3xy/paired"]==7);
}
'''
with tempfile.TemporaryDirectory() as d:
 source=Path(d)/'retired.cpp';exe=Path(d)/'retired';source.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Retired speed settings cleanup / stock field preservation / shared guards PASS')
