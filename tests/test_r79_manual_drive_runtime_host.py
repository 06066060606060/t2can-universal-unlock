"""Execute all three production manual authorization consumers consistently."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
logic=(ROOT/'vehicle_logic.h').read_text()
def function(name):
 matches=list(re.finditer(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',logic,re.M));assert matches,name
 start=matches[-1].start();end=logic.index('{',start)+1;depth=1
 while depth:
  depth+=(logic[end]=='{')-(logic[end]=='}');end+=1
 return logic[start:end].replace('static ','[[maybe_unused]] static ',1)+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <initializer_list>
#include "summon_state_pure.h"
#include "runtime_gate_pure.h"
#include "r79_ap_gate_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
int stateMux=0,r79LabMux=0,canTxBarrierMutex=1;
constexpr int portMAX_DELAY=1000,pdTRUE=1;
bool held=false,busy=false;
int xSemaphoreTake(int,int){assert(!held);if(busy)return 0;held=true;return 1;}
void xSemaphoreGive(int){assert(held);held=false;}
void portENTER_CRITICAL(int*){}void portEXIT_CRITICAL(int*){}
uint32_t fakeNow=100,lastDASStatusMillis=100;uint32_t millis(){return fakeNow;}
constexpr uint32_t R79_AP_DAS_FRESH_MS=3000;
bool dasAutopilotStateValid=true,gateSummoning=false,canTxAdministrativeHold=false,twaiReady=true,r79LabStockValid=true;
uint8_t dasAutopilotState4=2,observedGear=TESLA_GEAR_D;
bool gearValid=true,remoteEvidence=false;
bool r79FreshGearRawLocked(uint32_t,uint8_t &raw){raw=observedGear;return gearValid;}
bool r79RemoteStartupEvidenceLocked(uint32_t){return remoteEvidence;}
R79ManualSuppressionPure r79ManualSuppression{};
R79ApGateConfigPure r79ApGateConfig{false,0,2,false};R79ApGateSessionPure r79ApGateSession{};
volatile uint32_t r79ApGateGeneration=1,r79Mode1PostMux2Generation=1;
volatile bool countryOverrideR79AuthorizationOpen=false;
volatile uint32_t countryOverrideCancelGeneration=1;
std::map<std::string,uint32_t> nvs;bool nvsOpen=true,nvsWrite=true;
struct Preferences{bool begin(const char*,bool){assert(held);return nvsOpen;}void end(){}
 unsigned putUInt(const char*k,uint32_t v){assert(held);if(!nvsWrite)return 0;nvs[k]=v;return 4;}};
R79FixedQuietStatePure r79FixedQuietState{};R79Mode2DelayedStatePure r79Mode2DelayedState{};
bool r79RetryPending=false;uint8_t r79RetryIndex=0,r79RetryOriginKind=0;
uint32_t r79RetryDueMs=0,r79RetryGeneration=0;constexpr uint8_t R79LAB_TX_NONE=0;
'''
for declaration in ('enum R79RuntimeTxState : uint8_t {','struct R79RuntimeStatus {'):
 start=logic.index(declaration);code+=logic[start:logic.index('};',start)+2]+'\n'
for name in ('countryOverrideUpdateR79AuthorizationLocked','r79ApDasFreshLocked','r79ApGateDecisionLocked','r79ApGateObserveLocked','updateR79ManualSuppressionLocked','r79RuntimeStatusSnapshot','r79FastReactiveGateOpen','r79ApControlApply'):
 code+=function(name)
code+=r'''
void reset(){fakeNow=lastDASStatusMillis=100;dasAutopilotStateValid=true;dasAutopilotState4=2;gateSummoning=false;
 canTxAdministrativeHold=busy=false;twaiReady=r79LabStockValid=gearValid=true;remoteEvidence=false;observedGear=TESLA_GEAR_D;
 r79ManualSuppression={};r79ApGateConfig={false,0,2,false};r79ApGateSession={};countryOverrideR79AuthorizationOpen=false;nvsOpen=nvsWrite=true;}
void assertManualState(bool allow){
 auto status=r79RuntimeStatusSnapshot(fakeNow);
 assert(countryOverrideR79AuthorizationOpen==allow);
 assert(status.manualLatchActive&&!status.summonSessionActive&&status.decision.txEnabled==allow&&status.decision.manualSuppressed==!allow);
 assert(status.state==(allow?R79_TX_STATE_ACTIVE:R79_TX_STATE_SUSPENDED));
 assert(r79FastReactiveGateOpen()==allow);
}
int main(){
 for(uint8_t gear:{TESLA_GEAR_D,TESLA_GEAR_R})for(bool master:{false,true})for(uint8_t mode:{0,1}) {
  reset();observedGear=gear;r79ApGateConfig={master,mode,2,false};
  updateR79ManualSuppressionLocked(fakeNow,true);assert(r79ManualSuppression.active&&r79ManualSuppression.gearRaw==gear);assertManualState(false);
  assert(r79ApControlApply({master,mode,2,true}));assert(!held&&r79ManualSuppression.active);assertManualState(true);
  assert(r79ApControlApply({master,mode,2,false}));assert(!held&&r79ManualSuppression.active);assertManualState(false);
 }
 // The new setting cannot defeat AP block/delay, freshness or transport policy.
 reset();assert(r79ApControlApply({true,0,2,true}));dasAutopilotState4=3;
 updateR79ManualSuppressionLocked(fakeNow,true);auto status=r79RuntimeStatusSnapshot(fakeNow);
 assert(!status.manualLatchActive&&status.decision.reason==R79_TX_REASON_AUTOPILOT&&status.state==R79_TX_STATE_AP_BLOCKED&&!r79FastReactiveGateOpen());
 assert(r79ApControlApply({true,1,2,true}));fakeNow=2099;lastDASStatusMillis=2099;updateR79ManualSuppressionLocked(fakeNow,true);
 assert(r79RuntimeStatusSnapshot(fakeNow).state==R79_TX_STATE_AP_WAIT&&!r79FastReactiveGateOpen());
 fakeNow=2100;lastDASStatusMillis=2100;updateR79ManualSuppressionLocked(fakeNow,true);assert(r79FastReactiveGateOpen());
 fakeNow=5101;assert(!r79FastReactiveGateOpen()&&r79RuntimeStatusSnapshot(fakeNow).state==R79_TX_STATE_AP_UNKNOWN);
 reset();assert(r79ApControlApply({false,0,2,true}));updateR79ManualSuppressionLocked(fakeNow,true);
 canTxAdministrativeHold=true;assert(!r79FastReactiveGateOpen()&&r79RuntimeStatusSnapshot(fakeNow).state==R79_TX_STATE_ADMIN_HOLD);canTxAdministrativeHold=false;
 twaiReady=false;assert(!r79FastReactiveGateOpen()&&r79RuntimeStatusSnapshot(fakeNow).state==R79_TX_STATE_CAN_OFFLINE);twaiReady=true;
 r79LabStockValid=false;assert(r79RuntimeStatusSnapshot(fakeNow).state==R79_TX_STATE_WAIT_TEMPLATE);r79LabStockValid=true;
 // Failed writes retain all three consumer results and the manual latch.
 reset();updateR79ManualSuppressionLocked(fakeNow,true);auto generation=r79ApGateGeneration;
 nvsWrite=false;assert(!r79ApControlApply({false,0,2,true}));assertManualState(false);assert(r79ApGateGeneration==generation&&!held);nvsWrite=true;
 busy=true;assert(!r79ApControlApply({false,0,2,true}));assertManualState(false);assert(r79ApGateGeneration==generation&&!held);busy=false;
 // Removing the opt-in while the latch is established immediately suppresses all consumers.
 assert(r79ApControlApply({false,0,2,true}));assertManualState(true);
 assert(r79ApControlApply({false,0,2,false}));assertManualState(false);
}
'''
with tempfile.TemporaryDirectory() as d:
 source=Path(d)/'manual.cpp';exe=Path(d)/'manual';source.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Actual manual authorization / status / fast gate / atomic update consistency PASS')
