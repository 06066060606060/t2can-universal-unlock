"""Run production settings handlers against failing persistence boundaries.

Catches success responses/RAM changes after failed saves, missing rollback, and
administrative holds left asserted. Only flash/RTOS/JSON dependencies are mocked.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def extract(source, name):
    match = re.search(r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", source, re.M)
    assert match, name
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


COMMON = r'''
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include "das_status_pure.h"
#include "tsl9_hands_on_0x399_pure.h"
#include "tsl9_input_scheduler_pure.h"
#include "nag_human_v4_pure.h"
#include "nag_mode_h_variant_pure.h"
#include "vehicle_profile.h"
#include "ap_drive_profile_pure.h"
#include "ap_right_scroll_pure.h"
#include "auto_blinker_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
#include "fixed_point_pure.h"
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
struct String : std::string {
 using std::string::string;
 String(const std::string &s):std::string(s){}
 String(unsigned n):std::string(std::to_string(n)){}
 int toInt() const {return std::atoi(c_str());}
 void trim(){auto start=find_first_not_of(" \t\r\n");auto end=find_last_not_of(" \t\r\n");*this=start==npos?String():String(substr(start,end-start+1));}
 void toUpperCase(){for(char &c:*this)c=static_cast<char>(std::toupper(c));}
};
struct Server {
 std::map<std::string,String> values; int status=0; String body;
 bool hasArg(const String &key)const{return values.count(key);}
 String arg(const String &key){return values[key];}
 void send(int code,const char*,const String &json){status=code;body=json;}
} server;
bool saveOk=true, featureOk=true, supported=true, hold=false;
bool activeProfileNagTorqueSupported(){return supported;}
bool activeProfileNagTsl9Supported(){return supported;}
bool activeProfileBodyControlsSupported(){return supported;}
bool activeProfileIsYl(){return false;}
bool activeProfileApDriveProfileSupported(){return supported;}
bool activeProfileAdvancedEapSupported(){return supported;}
bool activeProfileR79Hw3Supported(){return supported;}
bool activeProfileBannedCarSupported(){return supported;}
bool activeProfileTlsscRestoreSupported(){return supported;}
void setCanTxAdministrativeHold(bool value){hold=value;}
bool httpBoolArg(const char *key,bool &out){if(!server.hasArg(key))return false;auto v=server.arg(key);if(v!="0"&&v!="1")return false;out=v=="1";return true;}
bool httpDecimalU16InRange(const String &s,uint16_t lo,uint16_t hi,uint16_t &out){char *end;auto n=std::strtol(s.c_str(),&end,10);if(s.empty()||*end||n<lo||n>hi)return false;out=static_cast<uint16_t>(n);return true;}
bool featureCfgSave(){return featureOk;}
bool labMenuEnabled=false,doorOpenCancelEnabled=false,apDriveProfileEnabled=false,bannedCar=false,tlsscRestoreEnabled=false;
uint8_t apDriveProfileRegenRaw=AP_DRIVE_REGEN_REDUCED_RAW;
String v3FeaturePolicyJson(){return "{}";}
uint8_t activeVehicleProfile=0,activeVehicleTopology=0;
bool nagTorqueRightScrollEnabled=false,nagTsl9RightPeriodicEnabled=false;
uint16_t nagTorqueRightScrollIntervalSeconds=60,nagTsl9RightPeriodicIntervalSeconds=60;
uint8_t nagTorqueRightScrollPattern=0;
uint8_t nagRightScrollPatternSanitize(uint8_t x){return x==1?1:0;}
bool tsl9InputQuiesceForConfig(){return true;}
void tsl9InputHardResetAfterQuiesce(bool){}
void nagTsl9RuntimeReset(){}
unsigned echoResets=0;
void nagExactEchoReset(){++echoResets;}
void nagHumanRuntimeReset(bool){}
bool nagIgnoreApStatePersist(bool){return saveOk;}
String nagCfgToJson(){return "{}";}String nagHumanProfileStatsToJson(){return "{}";}
NagHumanV4ConfigPure nagHumanV4Config=nagHumanV4DefaultConfigPure();
NagHumanV4StatePure nagHumanV4State{};bool nagHumanV4Initialized=false;uint8_t nagHumanVariant=1;
uint32_t nagRxFrames=0,nagEchoCount=0,mcpTxOk=0,mcpTxFail=0,nagTxOk=0,nagTxFail=0,nagSkipDisabled=0,nagSkipBootDelay=0,nagSkipWarmup=0,nagSkipSelfFrame=0,nagSkipHandsOn=0,nagSkipApInvalid=0,nagSkipApInactive=0,nagSkipDecision=0,nagSkipStopped=0,nagSkipSpeedStale=0,nagStopCarrierTxOk=0,nagBlockMutex=0,nagBlockMcpNotReady=0,nagBlockEpoch=0,nagBlockFreshMask=0,nagBlockInvalidMsg=0,nagSendError=0,nagLastTxOkMs=0,nagMaxTxGapMs=0,nagSessionTxOk=0,nagSessionStartMs=0,nagLastSkipReason=0,nagLastSkipMs=0,nagLastTxBlockReason=0,nagLastTxBlockMs=0,nagExactEchoLastTxMs=0,nagTsl9Rx=0,nagTsl9Modified=0,nagTsl9HandsOnModified=0,nagTsl9TxOk=0,nagTsl9TxFail=0;
bool nagExactEchoLastTxValid=false;uint8_t nagExactEchoLastTxRaw[8]={};int nagTsl9State=0;
constexpr uint8_t NAG_SKIP_NONE=0,MCP_TX_OK=0,NAG_RIGHT_SCROLL_PATTERN_DEFAULT=0;
bool watchHumanState=false,humanResetDuringSave=false;
void observeSaveBoundary(){if(watchHumanState&&(nagHumanV4State.base.eventCount!=7||nagHumanV4State.carrierRngState!=123||!nagHumanV4Initialized||echoResets!=0))humanResetDuringSave=true;}
bool summonCfgSave(){return saveOk;}
bool tlsscHighwayGateEnabled=false,tlsscBlockInNoa=false,gateNOAActive=false,tlsscInjectedActive=false,tlsscClearPending=false;
bool tlsscEnabled=false,blinkAEnabled=false;
bool setTlsscEnabled(bool value,bool){if(!saveOk)return false;tlsscEnabled=value;return true;}
bool setAutoBlinkerEnabled(bool value,bool){if(!saveOk)return false;blinkAEnabled=value;return true;}
String summonStatsToJson(){return "{}";} String blinkAStatsToJson(){return "{}";}
constexpr uint32_t BLINKA_AUTO_DELAY_DEFAULT_MS=1000;
uint32_t blinkADelayMs=1000;
uint8_t blinkANoaStabilizationSeconds=5,blinkACancelPauseSeconds=30;
AutoBlinkerNoaSessionStatePure autoBlinkerNoaSessionState{};
uint32_t millis(){return 12000;}
template<class T>T constrain(T x,T lo,T hi){return x<lo?lo:x>hi?hi:x;}
uint8_t r79TransportMode=1,r79Bit18Policy=0,r79Mode1TxWaitMode=0;
bool r79Mode1ReinjectEnabled=false,r79Mode2ReinjectEnabled=false;
uint16_t r79Mode1DelayMs=150,r79Mode2DelayMs=150;
R79FixedQuietStatePure r79FixedQuietState{};R79Mode2DelayedStatePure r79Mode2DelayedState{};
bool r79RetryPending=false;uint8_t r79RetryIndex=0,r79RetryOriginKind=0;uint32_t r79RetryDueMs=0,r79RetryGeneration=0,r79Mode1PostMux2Generation=1;
constexpr uint8_t R79LAB_TX_PERIODIC=1,R79LAB_TX_NONE=0;
void canTxCancellationGenerationAdvance(uint32_t *p){++*p;}
bool r79Hw3Apply(bool){return saveOk;}bool r79CfgSave(){return saveOk;}
String r79StatsToJson(){return "{}";}
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"assertion failed: %s (line %d)\n",#x,__LINE__);return 1;}}while(0)
'''

CASES = r'''
int main(int argc,char **argv){
 if(argc!=2)return 2;const std::string test=argv[1];
 nagCfgDefaultsModeA(nagCfg);nagCfg.enabled=true;nagCfg.hoRatePct=37;
 saveOk=false;featureOk=false;
 if(test=="mode"){
  server.values={{"m","1"}};httpNagSetMode();CHECK(server.status==500);CHECK(nagCfg.mode==MODE_A&&nagCfg.hoRatePct==37&&nagCfg.enabled);
 }else if(test=="nag"){
  server.values={{"enabled","0"}};httpNagUpdate();CHECK(server.status==500);CHECK(nagCfg.enabled&&nagCfg.hoRatePct==37);CHECK(!hold);
 }else if(test=="right-scroll"){
  server.values={{"torqueRightScrollEnabled","1"},{"torqueRightScrollIntervalSeconds","123"}};httpNagUpdate();CHECK(server.status==500);CHECK(!nagTorqueRightScrollEnabled&&nagTorqueRightScrollIntervalSeconds==60);CHECK(!hold);
 }else if(test=="door"){
  server.values={{"enabled","1"}};httpFeatureDoorCancel();CHECK(server.status==500);CHECK(!doorOpenCancelEnabled);
 }else if(test=="apdrive"){
  server.values={{"enabled","1"},{"regen","STANDARD"}};httpFeatureApDriveProfile();CHECK(server.status==500);CHECK(!apDriveProfileEnabled&&apDriveProfileRegenRaw==AP_DRIVE_REGEN_REDUCED_RAW);
 }else if(test=="tlssc-on"){
  httpSummonTlsscEnable();CHECK(server.status==500);CHECK(!tlsscEnabled);
 }else if(test=="tlssc-off"){
  tlsscEnabled=true;httpSummonTlsscDisable();CHECK(server.status==500);CHECK(tlsscEnabled);
 }else if(test=="highway"){
  server.values={{"enabled","1"}};httpSummonTlsscHighwayGate();CHECK(server.status==500);CHECK(!tlsscHighwayGateEnabled);
 }else if(test=="noa"){
  server.values={{"enabled","1"}};httpSummonTlsscNoaBlock();CHECK(server.status==500);CHECK(!tlsscBlockInNoa);
 }else if(test=="blink-on"){
  httpBlinkAEnable();CHECK(server.status==500);CHECK(!blinkAEnabled);
 }else if(test=="blink-off"){
  blinkAEnabled=true;httpBlinkADisable();CHECK(server.status==500);CHECK(blinkAEnabled);
 }else if(test=="blink-delay"){
  server.values={{"ms","4000"}};httpBlinkADelay();CHECK(server.status==500);CHECK(blinkADelayMs==1000);
 }else if(test=="blink-timing"){
  server.values={{"noaStabilizationSeconds","10"},{"cancelPauseSeconds","40"}};httpBlinkATiming();CHECK(server.status==500);CHECK(blinkANoaStabilizationSeconds==5&&blinkACancelPauseSeconds==30);
 }else if(test=="r79"){
  server.values={{"mode","2"}};httpR79Update();CHECK(server.status==500);CHECK(r79TransportMode==1);CHECK(!hold);
 }else if(test=="human-update"||test=="human-reset"||test=="nag-reset"){
  nagHumanV4Config.base.waitMinMs=1234;nagHumanV4State.base.eventCount=7;nagHumanV4State.carrierRngState=123;nagHumanV4Initialized=true;echoResets=0;watchHumanState=true;
  if(test=="human-update"){server.values={{"waitMinMs","500"}};httpNagHumanProfileUpdate();}else if(test=="human-reset") httpNagHumanProfileReset();else httpNagReset();
  CHECK(server.status==500);CHECK(!hold&&nagCfg.enabled&&nagCfg.mode==MODE_A);CHECK(nagHumanV4Config.base.waitMinMs==1234);CHECK(nagHumanV4State.base.eventCount==7&&nagHumanV4State.carrierRngState==123&&nagHumanV4Initialized&&echoResets==0&&!humanResetDuringSave);
 }else if(test=="invalid-regen"){
  saveOk=true;featureOk=true;server.values={{"enabled","1"},{"regen","INVALID"}};httpFeatureApDriveProfile();CHECK(server.status==400);CHECK(!apDriveProfileEnabled&&apDriveProfileRegenRaw==AP_DRIVE_REGEN_REDUCED_RAW);
 }else if(test=="success"){
  saveOk=true;featureOk=true;
  server.values={{"m","1"}};httpNagSetMode();CHECK(server.status==200&&nagCfg.mode==MODE_B);
  server.values={{"enabled","0"}};httpNagUpdate();CHECK(server.status==200&&!nagCfg.enabled&&!hold);
  server.values={{"enabled","1"}};httpFeatureDoorCancel();CHECK(server.status==200&&doorOpenCancelEnabled);
  server.values={{"enabled","1"},{"regen","STANDARD"}};httpFeatureApDriveProfile();CHECK(server.status==200&&apDriveProfileEnabled&&apDriveProfileRegenRaw==AP_DRIVE_REGEN_STANDARD_RAW);
  server.values={{"mode","2"}};httpR79Update();CHECK(server.status==200&&r79TransportMode==2&&!hold);
  server.values={{"torqueRightScrollEnabled","1"},{"torqueRightScrollIntervalSeconds","123"}};httpNagUpdate();CHECK(server.status==200&&nagTorqueRightScrollEnabled&&nagTorqueRightScrollIntervalSeconds==123&&!hold);
  httpSummonTlsscEnable();CHECK(server.status==200&&tlsscEnabled);httpSummonTlsscDisable();CHECK(server.status==200&&!tlsscEnabled);
  server.values={{"enabled","1"}};httpSummonTlsscHighwayGate();CHECK(server.status==200&&tlsscHighwayGateEnabled);httpSummonTlsscNoaBlock();CHECK(server.status==200&&tlsscBlockInNoa);
  httpBlinkAEnable();CHECK(server.status==200&&blinkAEnabled);httpBlinkADisable();CHECK(server.status==200&&!blinkAEnabled);
  server.values={{"ms","4000"}};httpBlinkADelay();CHECK(server.status==200&&blinkADelayMs==4000);
  server.values={{"noaStabilizationSeconds","10"},{"cancelPauseSeconds","40"}};httpBlinkATiming();CHECK(server.status==200&&blinkANoaStabilizationSeconds==10&&blinkACancelPauseSeconds==40);
  nagHumanV4State.base.eventCount=7;nagHumanV4Initialized=true;echoResets=0;server.values={{"waitMinMs","500"}};httpNagHumanProfileUpdate();CHECK(server.status==200&&nagHumanV4Config.base.waitMinMs==500&&!nagHumanV4Initialized&&nagHumanV4State.base.eventCount==0&&echoResets==1);
  server.values={};httpNagHumanProfileReset();CHECK(server.status==200&&nagHumanV4Config.base.waitMinMs==900&&echoResets==2);
 }else return 2;
 return 0;
}
'''

class SettingsApiHostTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        core=(ROOT/"can_core.h").read_text()
        api=Path(os.environ.get("V3282_API_SOURCE",str(ROOT/"web_api.h"))).read_text()
        logic=(ROOT/"vehicle_logic.h").read_text()
        code=COMMON+core[core.index("static const uint16_t NAG_TORQUE_RAW_MAX"):core.index("static portMUX_TYPE nagCfgMux")]+"\n"
        for source,name in ((core,"NagScrollStoredSnapshot"),(logic,"FeatureCfgSnapshot"),(logic,"SummonCfgSnapshot"),(logic,"R79CfgSnapshot")):
            match=re.search(r"^struct "+name+r" \{.*?^\};",source,re.M|re.S)
            assert match,name
            code+=match.group(0)+"\n"
        for name in ("featureCfgRuntimeSnapshot","featureCfgRuntimeRestore","summonCfgRuntimeSnapshot","summonCfgRuntimeRestore","r79CfgRuntimeSnapshot","r79CfgRuntimeRestore","nagScrollRuntimeSnapshot","nagScrollRuntimeRestore"):
            code+=extract(logic,name)
        code+=r'''
bool featureCfgPersistCandidate(const FeatureCfgSnapshot &){
 if(!featureOk)assert(!doorOpenCancelEnabled&&!apDriveProfileEnabled&&apDriveProfileRegenRaw==10);
 return featureOk;
}
bool summonCfgPersistCandidate(const SummonCfgSnapshot &){
 if(!saveOk)assert(!tlsscHighwayGateEnabled&&!tlsscBlockInNoa&&blinkADelayMs==1000&&blinkANoaStabilizationSeconds==5&&blinkACancelPauseSeconds==30);
 return saveOk;
}
bool r79CfgPersistCandidate(const R79CfgSnapshot &){
 if(!saveOk)assert(r79TransportMode==1&&r79Bit18Policy==0&&r79Mode1TxWaitMode==0&&!r79Mode1ReinjectEnabled&&!r79Mode2ReinjectEnabled&&r79Mode1DelayMs==150&&r79Mode2DelayMs==150);
 return saveOk;
}
'''
        for name in ("nagClampTorque","nagCfgSetCommonDefaults","nagCfgDefaultsModeA","nagCfgDefaultsModeB","nagCfgDefaultsModeC","nagCfgDefaultsModeH","nagCfgClampAll","nagCfgApplyActiveProfilePolicy","nagHumanV4RuntimeConfigSnapshot","nagHumanV4RuntimeSetLabTuning","nagHumanRuntimeLoadDefaults"):
            code+=extract(core,name)
        code+=r'''
void nagCfgCommit(const NagConfig &c){nagCfg=c;}
void observeNagSaveBoundary(){observeSaveBoundary();if(!saveOk)assert(nagCfg.enabled&&nagCfg.mode==MODE_A&&nagCfg.hoRatePct==37&&!nagTorqueRightScrollEnabled&&nagTorqueRightScrollIntervalSeconds==60);}
bool nagCfgSaveCandidateWithScroll(const NagConfig &,const NagScrollStoredSnapshot &){observeNagSaveBoundary();return saveOk;}
bool nagCfgPersistSnapshot(const NagConfig &,const NagHumanV4ConfigPure &,uint8_t,const NagScrollStoredSnapshot &){observeNagSaveBoundary();return saveOk;}
bool nagCfgSave(){observeSaveBoundary();return saveOk;}
bool nagCfgSaveCandidate(const NagConfig &){observeNagSaveBoundary();return saveOk;}
bool nagCfgPersist(const NagConfig &,const NagHumanV4ConfigPure &,uint8_t){observeNagSaveBoundary();return saveOk;}
'''
        for name in ("httpNagSetMode","httpNagUpdate","httpFeatureDoorCancel","httpFeatureApDriveProfile","httpSummonTlsscEnable","httpSummonTlsscDisable","httpSummonTlsscHighwayGate","httpSummonTlsscNoaBlock","httpBlinkAEnable","httpBlinkADisable","httpBlinkADelay","httpBlinkATiming","httpR79Update","httpParseHumanNm","httpNagHumanProfileUpdate","httpNagHumanProfileReset","httpNagReset"):
            code+=extract(api,name)
        cls.scratch=tempfile.TemporaryDirectory(prefix="v3282-api-")
        source=Path(cls.scratch.name)/"probe.cpp";source.write_text(code+CASES)
        cls.binary=Path(cls.scratch.name)/"probe"
        subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror","-Wno-unused-function","-Wno-unused-const-variable","-Wno-misleading-indentation","-I",str(ROOT),str(source),"-o",str(cls.binary)],check=True)

    @classmethod
    def tearDownClass(cls):
        cls.scratch.cleanup()

    def test_failed_settings_saves_preserve_controller_state(self):
        for scenario in ("mode","nag","right-scroll","door","apdrive","tlssc-on","tlssc-off","highway","noa","blink-on","blink-off","blink-delay","blink-timing","r79","invalid-regen","human-update","human-reset","nag-reset"):
            with self.subTest(scenario=scenario):
                result=subprocess.run([str(self.binary),scenario],capture_output=True,text=True)
                self.assertEqual(result.returncode,0,scenario+": "+result.stderr)

    def test_successful_saves_apply_requested_values(self):
        result=subprocess.run([str(self.binary),"success"],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

if __name__=="__main__":
    unittest.main()
