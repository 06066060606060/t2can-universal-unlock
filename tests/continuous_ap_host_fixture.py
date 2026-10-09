from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
PREFIX=r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>
#include "continuous_ap_types.h"
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
[[maybe_unused]] constexpr int pdTRUE=1,portMAX_DELAY=-1,ESP_OK=0,ESP_ERR_INVALID_STATE=1,ESP_ERR_INVALID_ARG=2,ESP_ERR_TIMEOUT=3;
using esp_err_t=int;
uint32_t clockMs=100,epoch=1;uint32_t millis(){return clockMs;}
uint8_t activeVehicleProfile=4,activeVehicleTopology=2;
bool mcpReady=true,twaiReady=true,canTxAdministrativeHold=false,usbHold=false;
int canTxBarrierMutex=1;bool held=false,beginOk=true,putOk=true;uint32_t stored=0x100;unsigned putCalls=0;
int xSemaphoreTake(int,int){if(held)return 0;held=true;return 1;}
void xSemaphoreGive(int){assert(held);held=false;}
bool canUsbTxHeld(){return usbHold;}
struct BarrierState{uint32_t &epoch;uint8_t freshMask=3;}canTxBarrierState{epoch};
bool canTxBarrierAllowsMaskedPure(const BarrierState&b,uint32_t e,uint8_t m){return m&&(b.epoch==e)&&((b.freshMask&m)==m);}
constexpr uint8_t CAN_TX_FRESH_BOTH=3;
struct Preferences{bool begin(const char*,bool){return beginOk;}void end(){};uint32_t getUInt(const char*,uint32_t){return stored;}size_t putUInt(const char*,uint32_t v){assert(held);++putCalls;if(putOk)stored=v;return putOk?4:0;}};
namespace MCP2515{enum ERROR{ERROR_OK,ERROR_FAIL};}
struct can_frame{uint32_t can_id=0;uint8_t can_dlc=0,data[8]={};};
struct twai_message_t{uint32_t identifier=0;uint8_t data_length_code=0,data[8]={};uint32_t flags=0;};
int sent=0;uint16_t sentId=0;uint8_t sentData[8]={};void (*beforeValidate)()=nullptr;void (*afterTransport)()=nullptr;
struct NagState{bool active=false,cleanupPending=false;};struct NagScheduler{NagState state;NagState snapshot(uint32_t){return state;}} tsl9InputScheduler;
int tsl9InputMux=0;
#include "runtime_gate_pure.h"
constexpr uint8_t CAN_TX_TRACE_SOURCE_DEFAULT=0;
void canATraceRecordTx(const can_frame*,McpTxResultReason,MCP2515::ERROR,uint8_t){}
void canBTraceRecordTx(const twai_message_t*,esp_err_t,uint8_t){}
bool txAccept=true;void (*afterEnqueue)()=nullptr;
struct Controller{MCP2515::ERROR sendMessage(const can_frame *msg){assert(held);if(!txAccept)return MCP2515::ERROR_FAIL;++sent;sentId=msg->can_id;memcpy(sentData,msg->data,8);if(afterEnqueue)afterEnqueue();return MCP2515::ERROR_OK;}}Can_A;
esp_err_t twai_transmit(const twai_message_t *msg,int){assert(held);if(!txAccept)return ESP_ERR_INVALID_STATE;++sent;sentId=msg->identifier;memcpy(sentData,msg->data,8);if(afterEnqueue)afterEnqueue();return ESP_OK;}
// PRODUCTION_TRANSPORT
bool canTxMcpSendValidated(can_frame*m,uint32_t e,bool(*v)(can_frame*,void*),void*c,MCP2515::ERROR&r){if(beforeValidate)beforeValidate();const bool ok=fixtureMcpValidated(m,e,v,c,r);if(afterTransport)afterTransport();return ok;}
esp_err_t canTxTwaiTransmitValidated(twai_message_t*m,uint32_t e,uint8_t mask,bool(*v)(twai_message_t*,void*),void*c){if(beforeValidate)beforeValidate();const auto ok=fixtureTwaiValidated(m,e,mask,v,c);if(afterTransport)afterTransport();return ok;}
#include "tsl9_input_scheduler_pure.h"
#include "continuous_ap_runtime.h"
void feed(uint32_t t,uint8_t ap,uint8_t turn){
 clockMs=t;uint8_t d[8]={};d[0]=ap;continuousApObserveStock(1,0x399,d,8,epoch,t);
 d[0]=turn;continuousApObserveStock(1,0x3f5,d,8,epoch,t);
 d[0]=0;d[2]=1;continuousApObserveStock(1,0x39d,d,3,epoch,t);
 d[2]=0x80;continuousApObserveStock(1,0x118,d,8,epoch,t);
 d[2]=8;d[3]=2;continuousApObserveStock(0,activeVehicleTopology==2?0x372:0x370,d,8,epoch,t);
 memset(d,0,8);d[0]=0x29;d[1]=0x55;d[6]=0x10;continuousApObserveStock(activeVehicleTopology==2?0:1,0x3c2,d,8,epoch,t);
 if(continuousApConfig.method==ContApMethod::StalkDouble){ContApStockFrame f;continuousApBuildStalkPure(0,15,f);continuousApObserveStock(0,0x229,f.data,3,epoch,t);}
}
"""
def production_function(name):
 import re
 source=(ROOT/'can_core.h').read_text()
 match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',source,re.M)
 assert match,name
 end=source.index('{',match.start())+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[match.start():end]+'\n'
transport=production_function('canTxEpochSnapshot')+production_function('canTxMcpSendValidated').replace('canTxMcpSendValidated(', 'fixtureMcpValidated(')+production_function('canTxTwaiTransmitValidated').replace('canTxTwaiTransmitValidated(', 'fixtureTwaiValidated(')
PREFIX=PREFIX.replace('// PRODUCTION_TRANSPORT',transport)
def run(body):
 with tempfile.TemporaryDirectory() as td:
  src=Path(td)/'host.cpp';binary=Path(td)/'host'
  src.write_text(PREFIX+body)
  try:
   subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-I',str(ROOT),str(src),'-o',str(binary)],check=True)
  except subprocess.CalledProcessError as error:
   raise FileNotFoundError('Host fixture compile/setup failure is not behavioral RED') from error
  subprocess.run([str(binary)],check=True)
