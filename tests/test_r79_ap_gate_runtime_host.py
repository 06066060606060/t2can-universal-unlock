"""Execute the production fast authorization gate with real policy helpers."""
from pathlib import Path
import os, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
logic = (ROOT / 'vehicle_logic.h').read_text()
def function(name):
    start = logic.rindex('static bool ' + name + '(')
    brace = logic.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (logic[end] == '{') - (logic[end] == '}')
        end += 1
    return logic[start:end]
code = r'''
#include <cassert>
#include <cstdint>
#include "summon_state_pure.h"
#include "runtime_gate_pure.h"
#include "r79_ap_gate_pure.h"
int stateMux = 0;
void portENTER_CRITICAL(int*) {}
void portEXIT_CRITICAL(int*) {}
bool canTxAdministrativeHold=false, twaiReady=true;
bool dasAutopilotStateValid=true, gateSummoning=false;
uint8_t dasAutopilotState4=3;
R79ManualSuppressionPure r79ManualSuppression{};
R79ApGateConfigPure r79ApGateConfig{false,0,2};
R79ApGateSessionPure r79ApGateSession{};
uint32_t fakeNow=100;
uint32_t millis() { return fakeNow; }
R79ApGateDecisionPure r79ApGateDecisionLocked(uint32_t now) {
 return r79ApGateDecisionPure(r79ApGateConfig,r79ApGateSession,now,
   dasAutopilotStateValid,dasStateApActivePure(dasAutopilotStateValid,dasAutopilotState4));
}
'''
code += function('r79FastReactiveGateOpen')
code += r'''
int main() {
 // Removing the opt-in conjunction from the production gate must fail here.
 r79ApGateConfig={true,0,2};
 r79ApGateObservePure(r79ApGateSession,r79ApGateConfig,fakeNow,true,true);
 assert(!r79FastReactiveGateOpen() && "AP block must close production fast gate");
 // Master OFF exactly preserves all old AP/Summon/manual combinations.
 r79ApGateConfig.enabled=false;
 for (int valid=0;valid<2;++valid) for(int ap=0;ap<16;++ap)
 for(int summon=0;summon<2;++summon) for(int manual=0;manual<2;++manual) {
  dasAutopilotStateValid=valid; dasAutopilotState4=ap; gateSummoning=summon;
  r79ManualSuppression={bool(manual),TESLA_GEAR_D};
  assert(r79FastReactiveGateOpen()==r79TxDecisionPure(
   dasStateApActivePure(valid,ap),summon,r79ManualSuppression).txEnabled);
 }
 r79ApGateConfig={true,1,2};r79ApGateSession={};
 dasAutopilotStateValid=true;dasAutopilotState4=3;gateSummoning=false;
 r79ManualSuppression={};fakeNow=100;
 r79ApGateObservePure(r79ApGateSession,r79ApGateConfig,fakeNow,true,true);
 fakeNow=2099; r79ApGateObservePure(r79ApGateSession,r79ApGateConfig,fakeNow,true,true);
 assert(!r79FastReactiveGateOpen());
 fakeNow=2100; r79ApGateObservePure(r79ApGateSession,r79ApGateConfig,fakeNow,true,true);
 assert(r79FastReactiveGateOpen());
 canTxAdministrativeHold=true; assert(!r79FastReactiveGateOpen());
 canTxAdministrativeHold=false;twaiReady=false;assert(!r79FastReactiveGateOpen());
}
'''
with tempfile.TemporaryDirectory(prefix='r79-ap-runtime-') as temp:
 src=Path(temp)/'gate.cpp';binary=Path(temp)/'gate';src.write_text(code)
 compiler=os.environ.get('CXX','c++')
 subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(src),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
print('Production AP gate: block/delay/default-off parity PASS')
