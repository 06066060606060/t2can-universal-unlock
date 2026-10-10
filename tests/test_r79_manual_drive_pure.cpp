#include <cassert>
#include <initializer_list>
#include "../summon_state_pure.h"
#include "../r79_ap_gate_pure.h"

// Compile the pre-feature policy so RED measures authorization, not missing API.
template<class T> auto manualDecision(bool ap,bool summon,const T &manual,bool allow,int)
  ->decltype(r79TxDecisionPure(ap,summon,manual,allow)) {
  return r79TxDecisionPure(ap,summon,manual,allow);
}
template<class T> R79TxDecisionPure manualDecision(bool ap,bool summon,const T &manual,bool,long) {
  return r79TxDecisionPure(ap,summon,manual);
}
int main() {
  for(uint8_t gear:{TESLA_GEAR_D,TESLA_GEAR_R}) {
    const R79ManualSuppressionPure manual={true,gear};
    const auto allowed=manualDecision(false,false,manual,true,0);
    assert(allowed.txEnabled&&!allowed.manualSuppressed);
    const auto blocked=manualDecision(false,false,manual,false,0);
    assert(!blocked.txEnabled&&blocked.manualSuppressed);
    assert(blocked.reason==(gear==TESLA_GEAR_R?R79_TX_REASON_MANUAL_R:R79_TX_REASON_MANUAL_D));
    assert(manualDecision(true,false,manual,true,0).reason==R79_TX_REASON_AUTOPILOT);
    assert(manualDecision(false,true,manual,true,0).reason==R79_TX_REASON_SUMMON);
  }
  // Manual permission only changes the latch clause; AP control stays separate.
  R79ApGateConfigPure config={true,0,2,true};R79ApGateSessionPure session={};
  assert(!r79ApGateDecisionPure(config,session,100,true,true).allowed);
  assert(r79ApGateDecisionPure(config,session,100,true,false).allowed);
  assert(!r79ApGateDecisionPure(config,session,100,false,false).allowed);
  config.mode=1;r79ApGateObservePure(session,config,100,true,true);
  assert(!r79ApGateDecisionPure(config,session,2099,true,true).allowed);
  assert(r79ApGateDecisionPure(config,session,2100,true,true).allowed);
}
