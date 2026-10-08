"""Actual configuration apply/load against an in-memory NVS device boundary."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
logic=(ROOT/'vehicle_logic.h').read_text()
def function(name):
 m=list(re.finditer(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',logic,re.M))[-1]
 start=m.start();end=logic.index('{',start)+1;depth=1
 while depth:
  depth+=(logic[end]=='{')-(logic[end]=='}');end+=1
 return logic[start:end]+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <map>
#include "r79_ap_gate_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
std::map<std::string,uint32_t> nvs;
bool nvsOpen=true,nvsWrite=true;int writes=0;bool held=false;
struct Preferences {
 bool begin(const char*,bool){return nvsOpen;}
 void end(){}
 uint32_t getUInt(const char*k,uint32_t d){return nvs.count(k)?nvs[k]:d;}
 uint8_t getUChar(const char*k,uint8_t d){return getUInt(k,d);}
 uint16_t getUShort(const char*k,uint16_t d){return getUInt(k,d);}
 bool getBool(const char*k,bool d){return getUInt(k,d);}
 unsigned putUInt(const char*k,uint32_t v){assert(held);++writes;if(!nvsWrite)return 0;nvs[k]=v;return 4;}
} prefs;
int stateMux=0,r79LabMux=0,canTxBarrierMutex=1;
constexpr int portMAX_DELAY=1000,pdTRUE=1;
bool busy=false;
void portENTER_CRITICAL(int*){}
void portEXIT_CRITICAL(int*){}
int xSemaphoreTake(int,int){assert(!held);if(busy)return 0;held=true;return pdTRUE;}
void xSemaphoreGive(int){assert(held);held=false;}
uint32_t fakeNow=100;
uint32_t millis(){return fakeNow;}
R79ApGateConfigPure r79ApGateConfig{false,0,2};
R79ApGateSessionPure r79ApGateSession{};
volatile uint32_t r79ApGateGeneration=1,r79Mode1PostMux2Generation=1;
bool dasFresh=true,apActive=true;
void r79ApGateObserveLocked(uint32_t now){r79ApGateObservePure(r79ApGateSession,r79ApGateConfig,now,dasFresh,apActive);}
void updateR79ManualSuppressionLocked(uint32_t now,bool locked){assert(held&&locked);r79ApGateObserveLocked(now);}
bool r79Hw3Enabled=false;
uint8_t r79Bit18Policy=0,r79TransportMode=1,r79Mode1TxWaitMode=1;
bool r79Mode1ReinjectEnabled=true,r79Mode2ReinjectEnabled=false,r79RetryPending=false;
uint16_t r79Mode1DelayMs=150,r79Mode2DelayMs=150;
R79FixedQuietStatePure r79FixedQuietState{};
R79Mode2DelayedStatePure r79Mode2DelayedState{};
uint8_t r79RetryIndex=0,r79RetryOriginKind=0;
uint32_t r79RetryDueMs=0,r79RetryGeneration=0;
constexpr uint8_t R79LAB_TX_NONE=0;
'''
code+=function('r79CfgLoad')+function('r79ApControlApply')
code+=r'''
int main(){
 r79CfgLoad();assert(!r79ApGateConfig.enabled&&r79ApGateConfig.mode==0&&r79ApGateConfig.delaySeconds==2&&!r79ApGateConfig.allowManualDriving);
 busy=true;auto oldWrites=writes;assert(!r79ApControlApply({false,0,2,true}));
 assert(writes==oldWrites&&nvs.empty()&&!r79ApGateConfig.allowManualDriving&&!held);busy=false;
 nvsOpen=false;assert(!r79ApControlApply({true,1,4}));assert(!r79ApGateConfig.enabled&&writes==0);
 nvsOpen=true;nvsWrite=false;assert(!r79ApControlApply({true,1,4}));assert(!r79ApGateConfig.enabled&&r79ApGateGeneration==1);
 nvsWrite=true;r79RetryPending=true;
 assert(r79ApControlApply({true,1,4}));assert(nvs["apctl"]==1027);
 assert(r79ApGateConfig.enabled&&r79ApGateSession.active&&r79ApGateSession.startedMs==100);
 assert(!r79RetryPending&&r79ApGateGeneration>1&&r79Mode1PostMux2Generation>1);
 fakeNow=1000;const auto gen=r79ApGateGeneration;const auto count=writes;
 assert(r79ApControlApply({true,1,4}));assert(r79ApGateGeneration==gen&&r79ApGateSession.startedMs==100&&writes==count);
 assert(r79ApControlApply({true,1,10}));assert(r79ApGateSession.startedMs==1000&&!r79ApGateSession.released);
 r79ApGateConfig={};r79ApGateSession={true,1,true};r79CfgLoad();
 assert(r79ApGateConfig.enabled&&r79ApGateConfig.mode==1&&r79ApGateConfig.delaySeconds==10&&!r79ApGateSession.active);
 assert(r79ApControlApply({false,0,2}));assert(!r79ApGateConfig.enabled&&!r79ApGateSession.active);
 r79CfgLoad();assert(!r79ApGateConfig.enabled);
 for(int mode=0;mode<2;++mode)for(int seconds=2;seconds<=10;++seconds){
  assert(r79ApControlApply({true,(uint8_t)mode,(uint8_t)seconds}));r79CfgLoad();
  assert(r79ApGateConfig.enabled&&r79ApGateConfig.mode==mode&&r79ApGateConfig.delaySeconds==seconds);
 }
 auto before=writes;assert(!r79ApControlApply({true,2,2}));assert(!r79ApControlApply({true,1,1}));assert(!r79ApControlApply({true,1,11}));assert(writes==before);
 nvs.clear();r79CfgLoad();assert(!r79ApGateConfig.enabled&&r79ApGateConfig.delaySeconds==2);
 // Historical packed values have no bit2 and always restore manual opt-in OFF.
 for(uint32_t old:{512u,513u,515u,1027u,2563u}){nvs["apctl"]=old;r79CfgLoad();assert(!r79ApGateConfig.allowManualDriving);}
 assert(r79ApControlApply({false,0,2,true}));assert(nvs["apctl"]==516u&&r79ApGateConfig.allowManualDriving);
 auto manualGeneration=r79ApGateGeneration;auto saved=nvs["apctl"];oldWrites=writes;
 busy=true;assert(!r79ApControlApply({false,0,2,false}));assert(writes==oldWrites&&nvs["apctl"]==saved&&r79ApGateConfig.allowManualDriving&&r79ApGateGeneration==manualGeneration);busy=false;
 nvsOpen=false;assert(!r79ApControlApply({false,0,2,false})&&!held&&nvs["apctl"]==saved);nvsOpen=true;
 nvsWrite=false;assert(!r79ApControlApply({false,0,2,false})&&!held&&nvs["apctl"]==saved&&r79ApGateConfig.allowManualDriving&&r79ApGateGeneration==manualGeneration);nvsWrite=true;
 r79ApGateConfig={};r79CfgLoad();assert(r79ApGateConfig.allowManualDriving&&!r79ApGateConfig.enabled);
 r79FixedQuietState.pending=true;r79Mode2DelayedState.pending=true;r79RetryPending=true;r79RetryIndex=2;r79RetryDueMs=500;r79RetryGeneration=9;
 assert(r79ApControlApply({false,0,2,false}));assert(!r79ApGateConfig.allowManualDriving&&r79ApGateGeneration>manualGeneration);
 assert(!r79FixedQuietState.pending&&!r79Mode2DelayedState.pending&&!r79RetryPending&&!r79RetryIndex&&!r79RetryDueMs&&!r79RetryGeneration);
 for(bool allow:{false,true})for(int master=0;master<2;++master)for(int mode=0;mode<2;++mode)for(int seconds=2;seconds<=10;++seconds){
  assert(r79ApControlApply({bool(master),(uint8_t)mode,(uint8_t)seconds,allow}));r79CfgLoad();
  assert(r79ApGateConfig.enabled==bool(master)&&r79ApGateConfig.mode==mode&&r79ApGateConfig.delaySeconds==seconds&&r79ApGateConfig.allowManualDriving==allow);
 }
 nvs["apctl"]=520u;r79CfgLoad();assert(!r79ApGateConfig.allowManualDriving&&!r79ApGateConfig.enabled); // Reserved bit3 fails closed.
}
'''
with tempfile.TemporaryDirectory(prefix='r79-ap-nvs-') as d:
 p=Path(d)/'test.cpp';exe=Path(d)/'test';p.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Actual AP config: persistence failure, reboot load, defaults, session reset, no-op update PASS')
