"""Verify ULC/ALC handlers keep CAN-visible settings unchanged until persistence."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def extract(source, name):
    match = re.search(r'^static\s+[^;{}]*?\b' + name + r'\([^;{}]*?\)\s*\{', source, re.M)
    assert match, name
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


PREFIX = r'''
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <string>
#include "feature_config_migration_pure.h"
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
struct String : std::string {
 using std::string::string;
 String(const std::string &v):std::string(v){}
 int toInt() const{return std::atoi(c_str());}
};
struct Server {
 std::map<std::string,String> values;
 int status=0;
 bool hasArg(const char *key)const{return values.count(key)>0;}
 String arg(const char *key){return values[key];}
 void send(int code,const char *,const String &){status=code;}
} server;
bool labAllowed=true,profileAllowed=true,saveOk=false;
unsigned saveCalls=0,cancelCalls=0;
constexpr uint8_t LAB3F8_STOCK=255,LAB3F8_ALC_STOCK=0,LAB3F8_ALC_FORCE_ON=2,STALK_IDLE=0;
uint8_t lab3f8AlcMode=0,lab3f8UlcBlindMode=255,lab3f8UlcOffHighwayMode=255,ulcNoConfirmTimingMode=0;
bool ulcNoConfirmEnabled=false,uiAutoLaneChangeEnabled=false;
uint8_t uiAutoLaneChangeTargetBus=3;
uint8_t oneShotTurn=1,activeTurn=1;
uint32_t oneShotUntil=700,oneShotReleaseAt=800;
bool httpRequireLab(){if(labAllowed)return true;server.send(403,"application/json","{}");return false;}
bool activeProfileUlcNoConfirmSupported(){return profileAllowed;}
bool httpBoolArg(const char *key,bool &value){const auto raw=server.arg(key);if(raw!="0"&&raw!="1")return false;value=raw=="1";return true;}
void autoBlinkerClearPendingLocked(){++cancelCalls;}
String ulcStatsToJson(){return "{}";}
FeatureConfigMigrationPure expected{},persisted{};
bool observePersist(const FeatureConfigMigrationPure &candidate);
'''

BOUNDARY = r'''
bool observePersist(const FeatureConfigMigrationPure &candidate){
 ++saveCalls;
 // Simulate a concurrent CAN task observing RAM while flash save is pending.
 const auto live=featureConfigRuntimeSnapshot();
 assert(live.ulc.alcOffHighwayEnabled==expected.ulc.alcOffHighwayEnabled);
 assert(live.ulc.blindSpotMode==expected.ulc.blindSpotMode);
 assert(live.ulc.ulcOffHighwayMode==expected.ulc.ulcOffHighwayMode);
 assert(live.ulc.confirmFreeEnabled==expected.ulc.confirmFreeEnabled);
 assert(live.ulc.confirmFreeTiming==expected.ulc.confirmFreeTiming);
 assert(live.autoLaneChange.enabled==expected.autoLaneChange.enabled);
 assert(live.autoLaneChange.bus==expected.autoLaneChange.bus);
 assert(cancelCalls==0&&oneShotUntil==700&&oneShotReleaseAt==800);
 if(saveOk)persisted=candidate;
 return saveOk;
}
bool ulcCfgSave(){return observePersist(featureConfigRuntimeSnapshot());}
bool autoLaneChangeLabCfgSave(){return observePersist(featureConfigRuntimeSnapshot());}
bool ulcCfgPersistCandidate(const FeatureConfigMigrationPure &value){return observePersist(value);}
bool autoLaneChangeLabCfgPersistCandidate(const FeatureConfigMigrationPure &value){return observePersist(value);}
'''

CASES = r'''
int main(int argc,char **argv){
 assert(argc==2);const std::string test=argv[1];
 expected=featureConfigRuntimeSnapshot();persisted=expected;
 if(test=="ulc-fail"||test=="ulc-success"){
  saveOk=test=="ulc-success";
  server.values={{"alcOff","1"},{"blind","2"},{"ulcOff","1"},{"confirm","1"},{"timing","1"}};
  httpUlcUpdate();assert(saveCalls==1);assert(server.status==(saveOk?200:500));
  if(saveOk){
   assert(lab3f8AlcMode==2&&lab3f8UlcBlindMode==2&&lab3f8UlcOffHighwayMode==1);
   assert(ulcNoConfirmEnabled&&ulcNoConfirmTimingMode==1);
   assert(persisted.ulc.alcOffHighwayEnabled&&persisted.ulc.blindSpotMode==2&&persisted.ulc.ulcOffHighwayMode==1);
   assert(persisted.ulc.confirmFreeEnabled&&persisted.ulc.confirmFreeTiming==1);
   assert(cancelCalls==1&&oneShotUntil==0&&oneShotReleaseAt==0);
  }else{
   assert(lab3f8AlcMode==0&&lab3f8UlcBlindMode==255&&lab3f8UlcOffHighwayMode==255);
   assert(!ulcNoConfirmEnabled&&ulcNoConfirmTimingMode==0&&cancelCalls==0);
   assert(oneShotUntil==700&&oneShotReleaseAt==800);
  }
 }else if(test=="alc-fail"||test=="alc-success"){
  saveOk=test=="alc-success";server.values={{"enabled","1"},{"bus","1"}};
  httpAutoLaneChangeLabUpdate();assert(saveCalls==1);assert(server.status==(saveOk?200:500));
  assert(uiAutoLaneChangeEnabled==saveOk&&uiAutoLaneChangeTargetBus==(saveOk?1:3));
  assert(cancelCalls==0&&oneShotUntil==700&&oneShotReleaseAt==800);
  if(saveOk)assert(persisted.autoLaneChange.enabled&&persisted.autoLaneChange.bus==1);
 }else if(test=="invalid"){
  server.values={{"blind","3"}};httpUlcUpdate();assert(server.status==400&&saveCalls==0);
  server.values={{"bus","0"}};httpAutoLaneChangeLabUpdate();assert(server.status==400&&saveCalls==0);
  assert(!uiAutoLaneChangeEnabled&&uiAutoLaneChangeTargetBus==3&&lab3f8UlcBlindMode==255);
 }else if(test=="gates"){
  profileAllowed=false;server.values={{"confirm","1"}};httpUlcUpdate();assert(server.status==409&&saveCalls==0);
  labAllowed=false;server.values={{"enabled","1"}};httpAutoLaneChangeLabUpdate();assert(server.status==403&&saveCalls==0);
 }else return 2;
}
'''

api = (ROOT / 'web_api.h').read_text()
logic = (ROOT / 'vehicle_logic.h').read_text()
code = PREFIX + extract(logic, 'featureConfigRuntimeSnapshot') + BOUNDARY
code += extract(api, 'httpUlcUpdate') + extract(api, 'httpAutoLaneChangeLabUpdate') + CASES
with tempfile.TemporaryDirectory(prefix='v3282-lane-api-') as directory:
    source = Path(directory) / 'probe.cpp'
    binary = Path(directory) / 'probe'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', '/usr/bin/g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), str(source), '-o', str(binary)], check=True)
    for case in ('ulc-fail', 'alc-fail', 'ulc-success', 'alc-success', 'invalid', 'gates'):
        subprocess.run([str(binary), case], check=True)
print('ULC/ALC actual HTTP persist-before-publish: 6 cases PASS')
