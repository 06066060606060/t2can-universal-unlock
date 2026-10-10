"""Execute actual atomic NVS settings and actual diagnostic JSON."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
logic=(ROOT/'vehicle_logic.h').read_text()
def function(name):
 match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',logic,re.M);assert match,name
 end=logic.index('{',match.start())+1;depth=1
 while depth:
  depth+=(logic[end]=='{')-(logic[end]=='}');end+=1
 return logic[match.start():end].replace('static ','[[maybe_unused]] static ',1)+'\n'
harness=(ROOT/'tests/test_vision3fd_runtime_host.py').read_text()
code=harness.split("code=r'''",1)[1].split("'''",1)[0]
code=code.replace('constexpr int ESP_OK=', '[[maybe_unused]] constexpr int ESP_OK=')
code+=r'''
#include <string>
#include <map>
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
constexpr int portMAX_DELAY=1000;
R79FixedQuietStatePure r79FixedQuietState{};R79Mode2DelayedStatePure r79Mode2DelayedState{};
volatile uint32_t r79Mode1PostMux2Generation=1;
bool r79RetryPending=false;uint8_t r79RetryIndex=0,r79RetryOriginKind=0;
uint32_t r79RetryDueMs=0,r79RetryGeneration=0;constexpr uint8_t R79LAB_TX_NONE=0;
struct String:std::string {using std::string::string;String(const std::string&s):std::string(s){}};
std::map<std::string,uint16_t> nvs;bool openOk=true,writeOk=true;int writes=0;
struct Preferences {std::string ns;bool begin(const char*n,bool){ns=n;return openOk;}void end(){}
 uint16_t getUShort(const char*k,uint16_t d){return nvs.count(ns+"/"+k)?nvs[ns+"/"+k]:d;}
 unsigned putUShort(const char*k,uint16_t v){assert(held);++writes;if(!writeOk)return 0;nvs[ns+"/"+k]=v;return 2;}};
std::map<std::string,std::string> jsonFields;
struct JsonWriterArduino {explicit JsonWriterArduino(String&){jsonFields.clear();}void finish(){}
 void boolean(const char*k,bool v){jsonFields[k]=v?"true":"false";}void u32(const char*k,uint32_t v){jsonFields[k]=std::to_string(v);}
 void i32(const char*k,int32_t v){jsonFields[k]=std::to_string(v);}void string(const char*k,const char*v){jsonFields[k]=v;}};
'''
for name in ('visionControlSupported','visionControlBusSupported','visionControlBusSnapshot','visionControlRequestDisabled','visionControlCacheStock','visionControlGateOpen','visionControlApplyFinal','visionControlRecordTx','mux1CancelPendingLocked','visionControlCfgLoad','visionControlApplySettings','visionControlApplySelection','visionControlApplyBus','getVisionControlStatsJson'):
 code+=function(name)
code+=r'''
void partyTopology(){activeVehicleTopology=3;}
void switchYl(){activeVehicleProfile=1;activeVehicleTopology=1;}
int main(){
 nvs.clear();visionControlCfgLoad();assert(!visionControlRequestDisabled()&&visionControlBusSnapshot()==0);
 for(uint16_t selection:{0u,1u,256u,257u}){nvs["labv3fd/selection"]=selection;visionControlCfgLoad();assert(visionControlRequestDisabled()==bool(selection&1)&&visionControlBusSnapshot()==uint8_t(selection>>8));}
 for(uint16_t bad:{2u,255u,258u,512u,65535u}){nvs["labv3fd/selection"]=bad;visionControlCfgLoad();assert(!visionControlRequestDisabled()&&visionControlBusSnapshot()==0);}
 // Both bool and route persist as one scalar, publish only after a successful write.
 assert(visionControlApplySettings(true,1));assert(nvs["labv3fd/selection"]==257&&visionControlBusSnapshot()==1&&visionControlRequestDisabled());
 auto savedGeneration=generation;int oldWrites=writes;
 busy=true;assert(!visionControlApplySettings(false,0));assert(writes==oldWrites&&visionControlRequestDisabled()&&visionControlBusSnapshot()==1);busy=false;
 openOk=false;assert(!visionControlApplySettings(false,0)&&!held&&writes==oldWrites);openOk=true;
 writeOk=false;assert(!visionControlApplySettings(false,0)&&!held&&generation==savedGeneration);writeOk=true;
 visionControlRequestDisabledState=false;visionControlBus=0;visionControlCfgLoad();assert(visionControlRequestDisabled()&&visionControlBusSnapshot()==1);
 assert(!visionControlApplySettings(true,2));
 // Profile/topology change at barrier entry rechecks support and normalization.
 takeHook=partyTopology;oldWrites=writes;assert(!visionControlApplySettings(false,1));assert(!held&&writes==oldWrites&&visionControlRequestDisabled());
 assert(visionControlApplySettings(true,0));assert(visionControlBusSnapshot()==0);
 activeVehicleTopology=2;takeHook=switchYl;assert(visionControlApplySettings(true,1));assert(visionControlBusSnapshot()==0&&nvs["labv3fd/selection"]==1);
 nvs["labv3fd/selection"]=257;visionControlCfgLoad();assert(visionControlBusSnapshot()==0&&visionControlRequestDisabled());
 assert(!visionControlApplySettings(true,2));
 activeVehicleProfile=3;activeVehicleTopology=3;nvs["labv3fd/selection"]=257;visionControlCfgLoad();
 assert(visionControlBusSnapshot()==1&&!visionControlBusSupported(1));getVisionControlStatsJson();assert(jsonFields["state"]=="UNSUPPORTED"&&jsonFields["busSupported"]=="false");
 assert(visionControlApplyBus(0));assert(visionControlRequestDisabled()&&visionControlBusSnapshot()==0);
 assert(visionControlApplySelection(false));assert(!visionControlRequestDisabled()&&visionControlBusSnapshot()==0);
 activeVehicleTopology=2;
 // Settings cancellation invalidates all existing prepared MUX1 send generations.
 r79RetryPending=true;r79RetryIndex=2;r79RetryDueMs=120;r79RetryGeneration=7;r79RetryOriginKind=2;r79FixedQuietState.pending=true;r79Mode2DelayedState.pending=true;
 savedGeneration=generation;assert(visionControlApplySettings(true,0));assert(generation==savedGeneration+1&&!r79RetryPending&&!r79FixedQuietState.pending&&!r79Mode2DelayedState.pending);
 // Report only selected-bus observations, including stock zero and epoch invalidation.
 getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["stockBit"]=="-1");
 twai_message_t stock;stock.data[6]=2;visionControlCacheStock(0,stock.data,1,100);getVisionControlStatsJson();
 assert(jsonFields["state"]=="READY"&&jsonFields["gateOpen"]=="true"&&jsonFields["stockBit"]=="1"&&jsonFields["rxAgeMs"]=="0");
 busy=true;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["gateOpen"]=="false");busy=false;
 canTxAdministrativeHold=true;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["gateOpen"]=="false");canTxAdministrativeHold=false;
 twaiReady=false;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["gateOpen"]=="false");twaiReady=true;
 assert(visionControlApplyFinal(stock.data,0));visionControlRecordTx(true,0,true);getVisionControlStatsJson();assert(jsonFields["txOk"]=="1");
 stock.data[6]=0;visionControlCacheStock(0,stock.data,1,100);getVisionControlStatsJson();assert(jsonFields["state"]=="STOCK_ZERO"&&jsonFields["stockBit"]=="0");
 dasAutopilotState4=2;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_AP"&&jsonFields["gateOpen"]=="false");dasAutopilotState4=3;
 fakeNow=3101;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["rxAgeMs"]=="3001");fakeNow=100;
 ++canTxBarrierState.epoch;getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["stockBit"]=="-1");
 visionControlCacheStock(0,stock.data,2,100);canTxBarrierState.freshMask=1;getVisionControlStatsJson();assert(jsonFields["gateOpen"]=="false"&&jsonFields["state"]=="WAIT_STOCK");canTxBarrierState.freshMask=3;
 assert(visionControlApplySettings(true,1));getVisionControlStatsJson();assert(jsonFields["state"]=="WAIT_STOCK"&&jsonFields["rxCount"]=="0");
 visionControlCacheStock(1,stock.data,2,100);getVisionControlStatsJson();assert(jsonFields["state"]=="STOCK_ZERO"&&jsonFields["busSelectorVisible"]=="true");
 labMenuEnabled=false;getVisionControlStatsJson();assert(jsonFields["state"]=="STOCK_ZERO"&&jsonFields["labEnabled"]=="true");labMenuEnabled=true;
 assert(visionControlApplySelection(false));getVisionControlStatsJson();assert(jsonFields["state"]=="OFF");
 activeVehicleProfile=1;activeVehicleTopology=1;visionControlCfgLoad();getVisionControlStatsJson();assert(jsonFields["busSelectorVisible"]=="false"&&jsonFields["bus"]=="0");
}
'''
with tempfile.TemporaryDirectory() as d:
 source=Path(d)/'settings.cpp';exe=Path(d)/'settings';source.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Production Visual Speed Control atomic settings / failure rollback / actual stats PASS')
