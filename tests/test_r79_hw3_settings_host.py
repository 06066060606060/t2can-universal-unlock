"""Run production HW3 apply, NVS load/reset and HTTP handler on host doubles."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
logic = (ROOT / 'vehicle_logic.h').read_text()
api = (ROOT / 'web_api.h').read_text()


def function(src, name):
    definitions = list(re.finditer(r'^static\s+[^;{}]*?\b' + re.escape(name) +
                                  r'\([^;{}]*?\)\s*\{', src, re.MULTILINE))
    assert definitions, f'Production definition missing: {name}'
    start = definitions[-1].start()
    end = src.index('{', start) + 1
    depth = 1
    while depth:
        depth += (src[end] == '{') - (src[end] == '}')
        end += 1
    return src[start:end] + '\n'


code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include "r79_fixed_policy_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_fixed_policy_pure.h"
#include "vehicle_profile.h"
struct String:std::string {
 using std::string::string;
 String(const std::string&s):std::string(s){}
 int toInt() const { return std::stoi(*this); }
};
std::map<std::string,uint32_t> nvs;
std::map<std::string,std::vector<uint8_t>> blobs;
bool nvsOpen=true,nvsWrite=true,nvsRead=true;int writes=0,ordinarySaves=0;
bool featureConfigNvsReadOk=true,r79CfgReadOk=true;
struct Preferences {
 std::string ns;bool good=true;
 bool begin(const char*n,bool){ns=n;good=nvsOpen;return nvsOpen;}
 bool ok()const{return good;}
 void end(){}
 uint32_t getUInt(const char*k,uint32_t d){if(!nvsRead){good=false;return d;}auto key=ns+"/"+k;return nvs.count(key)?nvs[key]:d;}
 uint8_t getUChar(const char*k,uint8_t d){return getUInt(k,d);}
 uint16_t getUShort(const char*k,uint16_t d){return getUInt(k,d);}
 bool getBool(const char*k,bool d){return getUInt(k,d);}
 unsigned putBool(const char*k,bool v){++writes;if(!nvsWrite)return 0;nvs[ns+"/"+k]=v;return 1;}
 bool clear(){for(auto it=nvs.begin();it!=nvs.end();){if(it->first.rfind(ns+"/",0)==0)it=nvs.erase(it);else ++it;}for(auto it=blobs.begin();it!=blobs.end();){if(it->first.rfind(ns+"/",0)==0)it=blobs.erase(it);else ++it;}return true;}
} prefs;
using CheckedPreferences=Preferences;
bool nvsCheckedReadBlob(const char*ns,const char*key,void*out,size_t size,bool&found){
 if(!nvsOpen||!nvsRead)return false;
 const std::string q=std::string(ns)+"/"+key;
 found=blobs.count(q)!=0;
 if(!found)return true;
 if(blobs[q].size()!=size)return false;
 std::memcpy(out,blobs[q].data(),size);return true;
}
bool nvsCheckedWriteBlob(const char*ns,const char*key,const void*data,size_t size){
 ++writes;++ordinarySaves;if(!nvsOpen||!nvsWrite)return false;const auto bytes=static_cast<const uint8_t*>(data);
 blobs[std::string(ns)+"/"+key]={bytes,bytes+size};return true;
}
int stateMux=0,r79LabMux=0,canTxBarrierMutex=1;
constexpr int portMAX_DELAY=1000,pdTRUE=1;
bool held=false;int takes=0;
void (*admissionHook)()=nullptr;
void portENTER_CRITICAL(int*){}
void portEXIT_CRITICAL(int*){}
int xSemaphoreTake(int,int){assert(!held);if(admissionHook){auto h=admissionHook;admissionHook=nullptr;h();}held=true;++takes;return pdTRUE;}
void xSemaphoreGive(int){assert(held);held=false;}
uint8_t activeVehicleProfile=VEHICLE_MODEL_Y_LEGACY,activeVehicleTopology=VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
bool activeProfileR79Hw3Supported(){return vehicleProfileR79Hw3Supported(activeVehicleProfile,activeVehicleTopology);}
volatile bool r79Hw3Enabled=false;
R79ApGateConfigPure r79ApGateConfig{false,0,2};
R79ApGateSessionPure r79ApGateSession{};
volatile uint32_t r79ApGateGeneration=1,r79Mode1PostMux2Generation=1;
uint8_t r79Bit18Policy=0,r79TransportMode=1,r79Mode1TxWaitMode=1;
bool r79Mode1ReinjectEnabled=true,r79Mode2ReinjectEnabled=false,r79RetryPending=false;
uint16_t r79Mode1DelayMs=150,r79Mode2DelayMs=150;
R79FixedQuietStatePure r79FixedQuietState{};
R79Mode2DelayedStatePure r79Mode2DelayedState{};
uint8_t r79RetryIndex=0,r79RetryOriginKind=0;
uint32_t r79RetryDueMs=0,r79RetryGeneration=0;
constexpr uint8_t R79LAB_TX_NONE=0,R79LAB_TX_PERIODIC=3;
void setCanTxAdministrativeHold(bool){}
void canTxCancellationGenerationAdvance(volatile uint32_t*g){++*g;}
String r79StatsToJson(){return "{}";}
#include "text_format_arduino.h"
struct Server {
 std::map<String,String> args;int status=0;String body;
 bool hasArg(const char*k){return args.count(k);}
 String arg(const char*k){return args[k];}
 void send(int c,const char*,const String&b){status=c;body=b;}
} server;
'''
code += logic[logic.index('struct R79CfgSnapshot {'):logic.index('static R79CfgSnapshot r79CfgRuntimeSnapshot()')]
for name in ('featureNvsU16', 'r79CfgPersistCandidate', 'r79Hw3Active', 'r79CfgLoad', 'r79Hw3Apply'):
    code += function(logic, name)
code += function(api, 'resetFirmwareSettingsPreserveProfileAndBle')
code += function(api, 'httpR79Update')
code += r'''
void reset() {
 nvs.clear();blobs.clear();nvsOpen=true;nvsWrite=true;nvsRead=true;featureConfigNvsReadOk=true;r79CfgReadOk=true;writes=0;takes=0;ordinarySaves=0;held=false;
 activeVehicleProfile=VEHICLE_MODEL_Y_LEGACY;activeVehicleTopology=VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
 r79Hw3Enabled=false;r79ApGateGeneration=1;r79Mode1PostMux2Generation=1;
 r79ApGateConfig={true,1,4};r79ApGateSession={true,1234,true};
 r79FixedQuietState={};r79Mode2DelayedState={};r79RetryPending=false;
}
void assertBeforePublication() {
 assert(!r79Hw3Enabled&&r79ApGateGeneration==1&&r79Mode1PostMux2Generation==1);
 assert(nvs["r79/hw3"]==1 && "NVS success must precede live publication");
 assert(r79RetryPending&&r79FixedQuietState.pending&&r79Mode2DelayedState.pending);
}
int main() {
 reset();r79Hw3Enabled=true;r79CfgLoad();assert(!r79Hw3Enabled&&!r79Hw3Active());
 reset();nvsOpen=false;assert(!r79Hw3Apply(true));assert(!r79Hw3Enabled&&writes==0&&takes==0);
 nvsOpen=true;nvsWrite=false;assert(!r79Hw3Apply(true));assert(!r79Hw3Enabled&&r79ApGateGeneration==1&&takes==0);
 nvsWrite=true;r79RetryPending=true;r79RetryOriginKind=3;r79RetryIndex=2;r79RetryGeneration=1;r79RetryDueMs=100;
 r79FixedQuietState.pending=true;r79Mode2DelayedState.pending=true;admissionHook=assertBeforePublication;
 assert(r79Hw3Apply(true));assert(r79Hw3Enabled&&r79Hw3Active()&&nvs["r79/hw3"]==1);
 assert(r79ApGateGeneration==2&&r79Mode1PostMux2Generation==2&&!held&&takes==1);
 assert(!r79RetryPending&&r79RetryIndex==0&&r79RetryOriginKind==0&&r79RetryGeneration==0&&r79RetryDueMs==0);
 assert(!r79FixedQuietState.pending&&!r79Mode2DelayedState.pending);
 assert(r79ApGateConfig.enabled&&r79ApGateConfig.mode==1&&r79ApGateConfig.delaySeconds==4);
 assert(r79ApGateSession.active&&r79ApGateSession.startedMs==1234&&r79ApGateSession.released);
 int before=writes;assert(r79Hw3Apply(true));assert(writes==before&&takes==1&&r79ApGateGeneration==2);
 r79Hw3Enabled=false;r79CfgLoad();assert(r79Hw3Enabled&&r79Hw3Active());
 // Both supported Legacy models/topologies activate; every other profile fails closed.
 for(uint8_t model=0;model<=6;++model)for(uint8_t topology=0;topology<=4;++topology) {
  activeVehicleProfile=model;activeVehicleTopology=topology;
  assert(r79Hw3Active()==vehicleProfileR79Hw3Supported(model,topology));
  if(!activeProfileR79Hw3Supported()) {int old=writes;assert(!r79Hw3Apply(true));assert(writes==old);}
 }
 assert(r79Hw3Apply(false));assert(!r79Hw3Active());r79CfgLoad();assert(!r79Hw3Enabled);
 reset();assert(r79Hw3Apply(true));nvs["v3profile/profile"]=3;nvs["s3xy/token"]=99;
 assert(resetFirmwareSettingsPreserveProfileAndBle());assert(nvs.count("r79/hw3")==0);
 assert(nvs["v3profile/profile"]==3&&nvs["s3xy/token"]==99);r79CfgLoad();assert(!r79Hw3Enabled);
 // Strict endpoint scalar syntax. Invalid/mixed requests never commit or write.
 reset();
 for(const char* invalid : {"","true","false","2","-1","0x1","1x"," 1","1 ","01"}) {
  server.args={{"hw3Enabled",invalid}};int old=writes;httpR79Update();assert(server.status==400&&writes==old&&!r79Hw3Enabled);
 }
 for(const char* mixed : {"mode","bit18Mode","mode1TxWaitMode","mode1Reinject","mode1DelayMs","mode2Reinject","mode2DelayMs"}) {
  server.args={{"hw3Enabled","1"},{mixed,"1"}};int old=writes;httpR79Update();assert(server.status==400&&writes==old&&!r79Hw3Enabled);
 }
 server.args.clear();httpR79Update();assert(server.status==400&&writes==0);
 activeVehicleProfile=VEHICLE_MODEL_Y_JUNIPER;server.args={{"hw3Enabled","1"}};httpR79Update();assert(server.status==409&&writes==0);
 server.args={{"hw3Enabled","0"}};httpR79Update();assert(server.status==200&&!r79Hw3Enabled);
 activeVehicleProfile=VEHICLE_MODEL_3_LEGACY;nvsWrite=false;server.args={{"hw3Enabled","1"}};httpR79Update();assert(server.status==503&&!r79Hw3Enabled);
 nvsWrite=true;httpR79Update();assert(server.status==200&&r79Hw3Enabled&&nvs["r79/hw3"]==1);
 server.args={{"hw3Enabled","0"}};httpR79Update();assert(server.status==200&&!r79Hw3Enabled&&nvs["r79/hw3"]==0);
 assert(ordinarySaves==0&&blobs.empty());
 // Ordinary R79 snapshot saves preserve HW3 and AP-control independent keys.
 nvs["r79/hw3"]=1;nvs["r79/apctl"]=(4u<<8)|3u;
 server.args={{"mode","2"},{"mode2Reinject","1"},{"mode2DelayMs","321"}};
 httpR79Update();assert(server.status==200&&ordinarySaves==1);
 assert(nvs["r79/hw3"]==1&&nvs["r79/apctl"]==((4u<<8)|3u));
 assert(blobs.count("r79/cfg3282")==1&&blobs["r79/cfg3282"].size()==10);
 r79TransportMode=1;r79Mode2ReinjectEnabled=false;r79Mode2DelayMs=150;r79Hw3Enabled=false;
 r79CfgLoad();assert(r79CfgReadOk&&r79TransportMode==2&&r79Mode2ReinjectEnabled&&r79Mode2DelayMs==321&&r79Hw3Enabled);
 assert(r79ApGateConfig.enabled&&r79ApGateConfig.mode==1&&r79ApGateConfig.delaySeconds==4);
 const auto retained=blobs["r79/cfg3282"];nvsWrite=false;server.args={{"mode","1"}};
 httpR79Update();assert(server.status==500&&r79TransportMode==2&&blobs["r79/cfg3282"]==retained);
 nvsRead=false;r79CfgLoad();assert(!r79CfgReadOk&&r79TransportMode==2&&r79Hw3Enabled&&nvs["r79/hw3"]==1);
 nvsRead=true;blobs["r79/cfg3282"].pop_back();r79CfgLoad();assert(!r79CfgReadOk&&r79TransportMode==2&&r79Hw3Enabled);
}
'''
with tempfile.TemporaryDirectory(prefix='r79-hw3-settings-') as temp:
    source = Path(temp) / 'settings.cpp'
    binary = Path(temp) / 'settings'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('R79 HW3 production settings: persistence, defaults/reset, capability, generations, AP session preservation, strict HTTP PASS')
