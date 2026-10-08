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
#include <string>
#include <map>
#include "r79_ap_gate_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
#include "vehicle_profile.h"
struct String:std::string {
 using std::string::string;
 String(const std::string&s):std::string(s){}
 int toInt() const { return std::stoi(*this); }
};
std::map<std::string,uint32_t> nvs;
bool nvsOpen=true,nvsWrite=true;int writes=0,legacySaves=0;
struct Preferences {
 std::string ns;
 bool begin(const char*n,bool){ns=n;return nvsOpen;}
 void end(){}
 uint32_t getUInt(const char*k,uint32_t d){auto key=ns+"/"+k;return nvs.count(key)?nvs[key]:d;}
 uint8_t getUChar(const char*k,uint8_t d){return getUInt(k,d);}
 uint16_t getUShort(const char*k,uint16_t d){return getUInt(k,d);}
 bool getBool(const char*k,bool d){return getUInt(k,d);}
 unsigned putBool(const char*k,bool v){++writes;if(!nvsWrite)return 0;nvs[ns+"/"+k]=v;return 1;}
 bool clear(){for(auto it=nvs.begin();it!=nvs.end();){if(it->first.rfind(ns+"/",0)==0)it=nvs.erase(it);else ++it;}return true;}
} prefs;
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
void r79CfgSave(){++legacySaves;}
String r79StatsToJson(){return "{}";}
struct Server {
 std::map<String,String> args;int status=0;String body;
 bool hasArg(const char*k){return args.count(k);}
 String arg(const char*k){return args[k];}
 void send(int c,const char*,const String&b){status=c;body=b;}
} server;
'''
for name in ('r79Hw3Active', 'r79CfgLoad', 'r79Hw3Apply'):
    code += function(logic, name)
code += function(api, 'resetFirmwareSettingsPreserveProfileAndBle')
code += function(api, 'httpR79Update')
code += r'''
void reset() {
 nvs.clear();nvsOpen=true;nvsWrite=true;writes=0;takes=0;legacySaves=0;held=false;
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
 assert(legacySaves==0);
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
