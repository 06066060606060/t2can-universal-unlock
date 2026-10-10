"""Exercise actual legacy-group saves, checked loads and migration read faults."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
LOGIC=(ROOT/'vehicle_logic.h').read_text()
def function(name):
    match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',LOGIC,re.M)
    assert match,name
    start=match.start();b=LOGIC.index('{',start);depth=1;e=b+1
    while depth:
        depth+=(LOGIC[e]=='{')-(LOGIC[e]=='}');e+=1
    return LOGIC[start:e]+'\n'
prefix=r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "feature_config_migration_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
#include "auto_blinker_pure.h"
#include "ap_right_scroll_pure.h"
#include "blinker_tx_policy_pure.h"
static std::map<std::string,unsigned> disk;
static std::map<std::string,std::vector<uint8_t>> blobs;
static bool beginOk=true,putOk=true,readOk=true;
static std::string failKey;
static unsigned writes=0;
static int canTxBarrierMutex=1;
constexpr int portMAX_DELAY=0,pdTRUE=1;
static bool barrierHeld=false,summonHeartbeatOverrideEnabled=false;
static uint32_t lab3f8Generation=0;
static unsigned cancelledPending=0;
static int xSemaphoreTake(int,int){barrierHeld=true;return pdTRUE;}
static void xSemaphoreGive(int){barrierHeld=false;}
static void mux1CancelPendingLocked(){++cancelledPending;}
struct Preferences {
 std::string ns;
 bool good=true;
 bool begin(const char *value,bool) {ns=value;good=beginOk;return beginOk;}
 bool ok() const {return good;}
 bool isKey(const char *key){return disk.count(ns+"/"+key)>0;}
 unsigned read(const char *key,unsigned fallback){const auto q=ns+"/"+key;if(!readOk||q==failKey){good=false;return fallback;}return disk.count(q)?disk[q]:fallback;}
 bool getBool(const char *key,bool fallback){return read(key,fallback)!=0;}
 uint8_t getUChar(const char *key,uint8_t fallback){return read(key,fallback);}
 uint16_t getUShort(const char *key,uint16_t fallback){return read(key,fallback);}
 uint32_t getUInt(const char *key,uint32_t fallback){return read(key,fallback);}
 size_t write(const char *key,unsigned value,size_t size) {++writes;const auto q=ns+"/"+key;if(!beginOk||!putOk||q==failKey)return 0;disk[q]=value;return size;}
 size_t putBool(const char *key,bool value){return write(key,value,1);}
 size_t putUChar(const char *key,uint8_t value){return write(key,value,1);}
 size_t putUShort(const char *key,uint16_t value){return write(key,value,2);}
 bool remove(const char *key){++writes;disk.erase(ns+"/"+key);return true;}
 void end() {}
};
using CheckedPreferences=Preferences;
static bool nvsCheckedWriteBlob(const char *ns,const char *key,const void *src,size_t length){++writes;const std::string q=std::string(ns)+"/"+key;if(!beginOk||!putOk||q==failKey)return false;const auto data=static_cast<const uint8_t *>(src);blobs[q]={data,data+length};return true;}
static bool nvsCheckedReadBlob(const char *ns,const char *key,void *out,size_t length,bool &found){const std::string q=std::string(ns)+"/"+key;if(!beginOk||!readOk||q==failKey)return false;found=blobs.count(q)>0;if(!found)return true;if(blobs[q].size()!=length)return false;memcpy(out,blobs[q].data(),length);return true;}
static void portENTER_CRITICAL(int *){}
static void portEXIT_CRITICAL(int *){}
static int stateMux=0,nagRightScrollMux=0,blinkAMux=0,r79LabMux=0,lab3f8Mux=0,autoLc293Mux=0;
static int driverMonitoringControlMux=0,isaSuppressionControlMux=0;
static bool labMenuEnabled=false,bannedCar=false,doorOpenCancelEnabled=false,tlsscRestoreEnabled=false,apDriveProfileEnabled=false;
static uint8_t apDriveProfileRegenRaw=10;
static bool nagTorqueRightScrollEnabled=false,nagTsl9RightPeriodicEnabled=false;
static uint16_t nagTorqueRightScrollIntervalSeconds=30,nagTsl9RightPeriodicIntervalSeconds=30;
static uint8_t nagTorqueRightScrollPattern=0;
static bool tlsscEnabled=false,tlsscHighwayGateEnabled=false,tlsscBlockInNoa=false,blinkAEnabled=false;
static bool tlsscInjectedActive=false,tlsscClearPending=false;
static bool oneShotDirect=false;
static uint8_t oneShotTurn=0,oneShotSource=0,activeTurn=0,lastReqDir=0;
static uint32_t oneShotUntil=0,oneShotReleaseAt=0,autoRequestLastSeenMs=0;
constexpr uint8_t STALK_IDLE=0;
static unsigned blinkerCancels=0,blinkerEvaluations=0;
constexpr int S3XY_LOG_INFO=0;
static std::string buttonLog;
static void s3xyLogPush(int,const char *message){buttonLog=message;}
static void autoBlinkerClearPendingLocked(){++blinkerCancels;}
static void blinkerTxClearPendingSourceLocked(uint8_t){++blinkerCancels;}
static void evaluateAutoBlinker(){++blinkerEvaluations;}
static bool activeProfileAdvancedEapSupported(){return true;}
static uint32_t blinkADelayMs=0;
static uint8_t blinkANoaStabilizationSeconds=10,blinkACancelPauseSeconds=20;
static uint8_t r79Bit18Policy=0,r79TransportMode=0,r79Mode1TxWaitMode=0;
static bool r79Mode1ReinjectEnabled=true,r79Mode2ReinjectEnabled=false;
static uint16_t r79Mode1DelayMs=2000,r79Mode2DelayMs=2000;
static uint16_t nvsSchemaVersion=3,nvsSchemaBootVersion=3;
constexpr uint16_t NVS_SCHEMA_ULC=2,NVS_SCHEMA_CURRENT=3;
constexpr uint8_t AP_DRIVE_REGEN_REDUCED_RAW=10,NAG_RIGHT_SCROLL_PATTERN_DEFAULT=0;
constexpr uint8_t UI_AUTO_LC_BUS_BOTH_PURE=3,LAB3F8_STOCK=255,LAB3F8_ALC_STOCK=0,LAB3F8_ALC_FORCE_ON=2;
constexpr uint8_t ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE=0;
constexpr uint32_t BLINKA_AUTO_DELAY_DEFAULT_MS=1000;
static uint8_t lab3f8AlcMode=0,lab3f8UlcBlindMode=255,lab3f8UlcOffHighwayMode=255,ulcNoConfirmTimingMode=0;
static bool ulcNoConfirmEnabled=false,uiAutoLaneChangeEnabled=false;
static uint8_t uiAutoLaneChangeTargetBus=3;
static uint8_t blinkerTxMode=0;
static bool driverMonitoringDisableEnabled=false,isaSuppressionEnabled=false;
static bool activeProfileBodyControlsSupported(){return true;}
static bool activeProfileIsYl(){return false;}
static bool activeProfileBannedCarSupported(){return true;}
static bool activeProfileTlsscRestoreSupported(){return true;}
static bool activeProfileApDriveProfileSupported(){return true;}
static bool activeProfileUlcNoConfirmSupported(){return true;}
static bool apDriveRegenRawSelectablePure(uint8_t v){return v==10;}
static uint8_t nagRightScrollPatternSanitize(uint8_t v){return v;}
static bool uiAutoLaneChangeBusTargetValidPure(uint8_t v){return v>=1&&v<=3;}
static bool ulcNoConfirmTimingValidPure(uint8_t v){return v<=1;}
static bool driverMonitoringControlApply(bool v){driverMonitoringDisableEnabled=v;return true;}
static bool isaSuppressionControlApply(bool v){isaSuppressionEnabled=v;return true;}
template<typename T>static T constrain(T v,T lo,T hi){return std::max(lo,std::min(v,hi));}
#define T2CAN_SERIAL_PRINTLN(value) ((void)0)
"""
# These are production declarations, not duplicated configuration logic.
prefix+=LOGIC[LOGIC.index('static bool featureConfigNvsReadOk'):LOGIC.index('static FeatureCfgSnapshot featureCfgRuntimeSnapshot')]
prefix+=LOGIC[LOGIC.index('struct SummonCfgSnapshot'):LOGIC.index('static bool summonCfgPersistCandidate')]
prefix+=LOGIC[LOGIC.index('struct R79CfgSnapshot'):LOGIC.index('static R79CfgSnapshot r79CfgRuntimeSnapshot')]
functions=['featureCfgRuntimeSnapshot','featureCfgRuntimeRestore','featureNvsU16','featureNvsU32','featureCfgPersistCandidate','featureCfgSave','summonCfgRuntimeSnapshot','summonCfgRuntimeRestore','summonCfgPersistCandidate','summonCfgSave','r79CfgRuntimeSnapshot','r79CfgRuntimeRestore','r79CfgPersistCandidate','r79CfgSave','nvsSchemaRead','featureConfigWriteSchema3Values','featureConfigSchema3ReadBackMatches','featureConfigCleanupRetiredR79Keys','featureConfigMigrateToSchema3','featureConfigWriteSchema2Values','featureConfigSchema2ReadBackMatches','featureConfigMigrateToSchema2','featureConfigRuntimeSnapshot','ulcCfgPersistCandidate','ulcCfgSave','autoLaneChangeLabCfgPersistCandidate','autoLaneChangeLabCfgSave','featureCfgLoad','summonCfgLoad','ulcCfgLoadAndMigrate','autoLaneChangeLabCfgLoadAndMigrate']
if re.search(r'^static bool featureCfgPersistLab\(bool enabled\)\s*\{',LOGIC,re.M):
    functions.append('featureCfgPersistLab')
functions.extend(['countryOverrideSetLabEnabledWithBarrier','setTlsscEnabled','setAutoBlinkerEnabled','toggleTlsscEnabledFromButton'])
lab_call='countryOverrideSetLabEnabledWithBarrier(false,true)' if 'bool persist' in function('countryOverrideSetLabEnabledWithBarrier') else 'countryOverrideSetLabEnabledWithBarrier(false)'
main=r"""
int main(){
 labMenuEnabled=true;summonHeartbeatOverrideEnabled=true;putOk=false;
 const auto labGeneration=lab3f8Generation;const auto cancellation=cancelledPending;
 const bool labSaved=LAB_CALL;
 assert(labMenuEnabled&&summonHeartbeatOverrideEnabled);
 assert(lab3f8Generation==labGeneration&&cancelledPending==cancellation&&!barrierHeld);
 assert(!labSaved);putOk=true;
 assert(LAB_CALL);assert(!labMenuEnabled&&!summonHeartbeatOverrideEnabled&&!barrierHeld);
 // Failed Summon setters must not publish flags or cancel/evaluate CAN state.
 tlsscEnabled=false;putOk=false;toggleTlsscEnabledFromButton();
 assert(!tlsscEnabled&&buttonLog.find("failed")!=std::string::npos);
 tlsscEnabled=true;tlsscInjectedActive=true;tlsscClearPending=false;
 blinkAEnabled=true;oneShotUntil=700;const auto initialCancels=blinkerCancels;
 putOk=false;assert(!setTlsscEnabled(false,true));assert(!setAutoBlinkerEnabled(false,true));
 assert(tlsscEnabled&&!tlsscClearPending&&blinkAEnabled&&oneShotUntil==700);
 assert(blinkerCancels==initialCancels&&blinkerEvaluations==0);
 blinkAEnabled=false;assert(!setAutoBlinkerEnabled(true,true));assert(!blinkAEnabled&&blinkerEvaluations==0);
 putOk=true;assert(setTlsscEnabled(false,true));assert(!tlsscEnabled&&tlsscClearPending);
 assert(setAutoBlinkerEnabled(true,true));assert(blinkAEnabled&&blinkerEvaluations==1);
 assert(setAutoBlinkerEnabled(false,true));assert(!blinkAEnabled&&oneShotUntil==0);
 // Every legacy group commits one value; a failed candidate leaves all old
 // storage intact and returns false instead of silently succeeding.
 const auto originalSummon=summonCfgRuntimeSnapshot();summonCfgRuntimeRestore(originalSummon);
 const auto originalR79=r79CfgRuntimeSnapshot();r79CfgRuntimeRestore(originalR79);
 assert(featureCfgSave());assert(summonCfgSave());assert(r79CfgSave());
 assert(ulcCfgSave());assert(autoLaneChangeLabCfgSave());
 const auto before=blobs;
 doorOpenCancelEnabled=true;tlsscEnabled=true;r79Bit18Policy=1;
 lab3f8AlcMode=2;uiAutoLaneChangeEnabled=true;
 putOk=false;
 assert(!featureCfgSave());assert(!summonCfgSave());assert(!r79CfgSave());
 assert(!ulcCfgSave());assert(!autoLaneChangeLabCfgSave());
 assert(blobs==before);putOk=true;
 featureCfgLoad();summonCfgLoad();ulcCfgLoadAndMigrate();autoLaneChangeLabCfgLoadAndMigrate();
 assert(!doorOpenCancelEnabled&&!tlsscEnabled&&lab3f8AlcMode==0&&!uiAutoLaneChangeEnabled);
 // Snapshot presence takes precedence over unrelated old scalar values.
 disk["features/doorCancel"]=1;featureCfgLoad();assert(!doorOpenCancelEnabled);
 const auto ram=featureCfgRuntimeSnapshot();doorOpenCancelEnabled=true;
 featureCfgRuntimeRestore(ram);assert(!doorOpenCancelEnabled);
 // A typed schema read failure cannot authorize a default migration or save.
 failKey="t2meta/schema";const auto prior=disk;const auto priorBlobs=blobs;
 const auto priorWrites=writes;nvsSchemaRead();
 assert(!featureConfigNvsReadOk&&!featureConfigMigrateToSchema2()&&!featureConfigMigrateToSchema3());
 assert(!featureCfgSave()&&!summonCfgSave()&&!r79CfgSave()&&!ulcCfgSave());
 assert(disk==prior&&blobs==priorBlobs&&writes==priorWrites);
 failKey.clear();nvsSchemaRead();assert(featureConfigNvsReadOk);
 // Per-namespace load errors prohibit persisting fallback/current RAM over a
 // stored snapshot, but do not alter the stored record.
 failKey="features/cfg3282";featureCfgLoad();assert(!featureCfgSave());assert(blobs==priorBlobs);
 failKey.clear();featureCfgLoad();assert(featureCfgSave());
 // Scalar-only installations remain readable without eagerly overwriting NVS.
 blobs.clear();disk.clear();disk["t2meta/schema"]=3;nvsSchemaRead();
 disk["features/doorCancel"]=1;disk["summon/tlssc"]=1;
 disk["ulc/alcOff"]=1;disk["alc293lab/enabled"]=1;
 featureCfgLoad();summonCfgLoad();ulcCfgLoadAndMigrate();autoLaneChangeLabCfgLoadAndMigrate();
 assert(doorOpenCancelEnabled&&tlsscEnabled&&lab3f8AlcMode==2&&uiAutoLaneChangeEnabled);
 // A corrupt legacy typed key is distinct from a missing key/default.
 failKey="ulc/blind";assert(!ulcCfgLoadAndMigrate());assert(!ulcCfgSave());
 failKey="summon/tlHwy";summonCfgLoad();assert(!summonCfgSave());
 const auto saved=blobs;failKey="lab3f8/ulcNoConf";nvsSchemaVersion=0;featureConfigNvsReadOk=true;
 assert(!featureConfigMigrateToSchema2());assert(!featureConfigNvsReadOk);assert(blobs==saved);
}
"""
with tempfile.TemporaryDirectory(prefix='v3282-feature-nvs-') as d:
 source=Path(d)/'probe.cpp';binary=Path(d)/'probe'
 source.write_text(prefix+''.join(function(n) for n in functions)+main.replace("LAB_CALL",lab_call))
 subprocess.run([os.environ.get('CXX','/usr/bin/g++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Feature/Summon/R79/ULC/ALC checked NVS snapshots and migration faults PASS')
