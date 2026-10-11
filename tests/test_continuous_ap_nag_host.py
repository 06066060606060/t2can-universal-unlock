from continuous_ap_host_fixture import run,ROOT
import continuous_ap_host_fixture as fixture
import re
source=(ROOT/'vehicle_logic.h').read_text()
def function(name):
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',source,re.M);assert m,name
 end=source.index('{',m.start())+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
run(r"""
#include "tsl9_input_scheduler_pure.h"
// Existing transport entrypoints for the unmodified Nag send functions.
[[maybe_unused]] constexpr uint8_t CAN_TX_FRESH_VH=2;
bool canTxMcpSend(const can_frame*m,uint32_t e,MCP2515::ERROR&r,void*){can_frame out=*m;auto v=[](can_frame*,void*){return true;};return canTxMcpSendValidated(&out,e,v,nullptr,r);}
esp_err_t canTxTwaiTransmitWithMask(const twai_message_t*m,uint32_t e,uint8_t mask){twai_message_t out=*m;auto v=[](twai_message_t*,void*){return true;};return canTxTwaiTransmitValidated(&out,e,mask,v,nullptr);}
void researchCaptureObserveTxVh(uint16_t,uint8_t,const uint8_t*,bool){}
"""+function('tsl9InputSendCanA')+function('tsl9InputSendCanB')+r"""
int main(){
 continuousApConfig={true,ContApMethod::ScrollDouble};
 feed(100,3,1);continuousApService(ContApRoute::BodyA);feed(101,2,1);continuousApService(ContApRoute::BodyA);feed(102,2,0);continuousApService(ContApRoute::BodyA);feed(1102,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);
 can_frame pressed={};pressed.can_id=0x3c2;pressed.can_dlc=8;memcpy(pressed.data,sentData,8);
 clockMs=1103;continuousApObserveStock(0,0x3c2,pressed.data,8,epoch,clockMs);continuousApService(ContApRoute::BodyA);
 clockMs=1604;continuousApService(ContApRoute::BodyA);assert(continuousApSnapshot().cleanupUnconfirmed);int sends=sent;
 Tsl9InputCommandPure command={};command.kind=TSL9_INPUT_COMMAND_STEP_PURE;command.mode=TSL9_INPUT_MODE_RIGHT_SPEED_PURE;command.rightTick=1;
 assert(!tsl9InputSendCanA(pressed,command)&&sent==sends); // Never replay accepted click from stale Nag template.
 feed(1605,3,0);assert(tsl9InputSendCanA(pressed,command)&&sent==sends+1&&(sentData[1]&0x30)==0x10&&sentData[3]==1); // Compose from latest released stock.
 // Unsupported Party CAP leaves independent NAG transport available.
 continuousApResetForEpoch(++epoch);activeVehicleTopology=3;feed(1700,3,0);
 twai_message_t neutral={};neutral.identifier=0x3c2;neutral.data_length_code=8;
 neutral.data[0]=0x29;neutral.data[1]=0x55;neutral.data[6]=0x10;
 sends=sent;assert(tsl9InputSendCanB(neutral,command)&&sent==sends+1);
 assert((sentData[1]&0x30)==0x10&&sentData[3]==1);
 assert(continuousApFsm.ownedRoute()==ContApRoute::None&&!continuousApSnapshot().releaseOwed);

}
""")
print('Shared production Nag sends do not replay Continuous AP clicks PASS')

# Exercise both production owners together. A busy-flag stub cannot expose the
# cleanup/route-ownership cycle that otherwise consumes the entire CAP episode.
original_prefix = fixture.PREFIX
fixture.PREFIX = original_prefix.replace(
    'struct NagState{bool active=false,cleanupPending=false;uint8_t mode=0;};struct NagScheduler{NagState state;NagState snapshot(uint32_t){return state;}} tsl9InputScheduler;',
    '#include "tsl9_input_scheduler_pure.h"\nTsl9InputSchedulerPure tsl9InputScheduler;')
try:
 run(r"""
[[maybe_unused]] constexpr uint8_t CAN_TX_FRESH_VH=2;
void researchCaptureObserveTxVh(uint16_t,uint8_t,const uint8_t*,bool){}
"""+function('tsl9InputSendCanA')+function('tsl9InputSendCanB')+r"""
bool sendNag(ContApRoute route,const Tsl9InputCommandPure &command){
 // Deliberately stale cached click: admission must reconstruct released stock.
 can_frame a={};a.can_id=0x3c2;a.can_dlc=8;a.data[0]=0x29;a.data[1]=0x65;a.data[6]=0x10;
 if(route==ContApRoute::BodyA)return tsl9InputSendCanA(a,command);
 twai_message_t b={};b.identifier=a.can_id;b.data_length_code=8;memcpy(b.data,a.data,8);
 return tsl9InputSendCanB(b,command);
}
Tsl9InputInputsPure nagInputs(uint32_t t,bool enabled){
 Tsl9InputInputsPure in={};in.nowMs=t;in.enabled=enabled;in.warningActive=enabled;in.templateFresh=true;in.txAllowed=true;in.mode=TSL9_INPUT_MODE_RIGHT_SPEED_PURE;return in;
}
void resetOwners(ContApRoute route){
 continuousApResetForEpoch(++epoch);tsl9InputScheduler=Tsl9InputSchedulerPure();
 activeVehicleProfile=4;activeVehicleTopology=route==ContApRoute::BodyA?2:3;
 continuousApConfig={true,ContApMethod::ScrollSingle};sent=0;
}
void cleanupDuringPendingCap(ContApRoute route){
 resetOwners(route);feed(100,3,1);continuousApService(route);
 auto step=tsl9InputScheduler.service(nagInputs(100,true));
 assert(step.kind==TSL9_INPUT_COMMAND_STEP_PURE&&sendNag(route,step));
 assert((sentData[1]&0x30)==0x10); // Cached press is never replayed.
 tsl9InputScheduler.onCommandResult(step,true,100,0);
 feed(101,2,1);continuousApService(route);
 assert(continuousApFsm.ownedRoute()==route&&continuousApNagBusy());
 assert(!sendNag(route,step)); // New NAG ticks cannot preempt CAP ownership.
 auto fakeCenter=step;fakeCenter.kind=TSL9_INPUT_COMMAND_CENTER_PURE;fakeCenter.leftTick=fakeCenter.rightTick=0;
 assert(!sendNag(route,fakeCenter)); // No real cleanup is pending yet.
 auto cleanup=tsl9InputScheduler.service(nagInputs(101,false));
 assert(cleanup.kind==TSL9_INPUT_COMMAND_CENTER_PURE&&tsl9InputScheduler.snapshot(101).cleanupPending);
 auto wrongMode=cleanup;wrongMode.mode=TSL9_INPUT_MODE_LEFT_VOLUME_PURE;
 assert(!sendNag(route,wrongMode));
 auto nonneutral=cleanup;nonneutral.rightTick=1;assert(!sendNag(route,nonneutral));
 nonneutral=cleanup;nonneutral.leftTick=-1;assert(!sendNag(route,nonneutral));
 clockMs=602;assert(!sendNag(route,cleanup)); // Fresh stock remains mandatory.
 feed(603,2,0);continuousApService(route);
 canTxAdministrativeHold=true;assert(!sendNag(route,cleanup));canTxAdministrativeHold=false;
 usbHold=true;assert(!sendNag(route,cleanup));usbHold=false;
 mcpReady=false;assert(!sendNag(route,cleanup));mcpReady=true;
 twaiReady=false;assert(!sendNag(route,cleanup));twaiReady=true;
 uint8_t pressed[8]={0x29,0x65,0,0,0,0,0x10,0};
 continuousApObserveStock(route==ContApRoute::BodyA?0:1,0x3c2,pressed,8,epoch,603);
 assert(!sendNag(route,cleanup)); // Physical clicks still override cleanup.
 feed(604,2,0);continuousApService(route);
 // The physical click intentionally cancels that episode; start a fresh one.
 feed(1400,3,1);continuousApService(route);feed(1401,2,1);continuousApService(route);feed(1402,2,0);continuousApService(route);
 assert(continuousApFsm.ownedRoute()==route);
 const int beforeCleanup=sent;
 assert(sendNag(route,cleanup)); // Regression: neither owner can drain without this.
 assert(sent==beforeCleanup+1&&(sentData[1]&0x30)==0x10&&sentData[2]==0&&sentData[3]==0);
 tsl9InputScheduler.onCommandResult(cleanup,true,1402,0);
 assert(!continuousApNagBusy());
 feed(2402,2,0);continuousApService(route);
 assert(sent==beforeCleanup+2&&(sentData[1]&0x30)==0x20); // CAP can now press within its episode.
 // A real later NAG cleanup cannot take over an accepted CAP press.
 auto laterStep=tsl9InputScheduler.service(nagInputs(2402,true));
 assert(laterStep.kind==TSL9_INPUT_COMMAND_STEP_PURE);
 tsl9InputScheduler.onCommandResult(laterStep,true,2402,0);
 auto laterCleanup=tsl9InputScheduler.service(nagInputs(2402,false));
 assert(laterCleanup.kind==TSL9_INPUT_COMMAND_CENTER_PURE);
 assert(continuousApSnapshot().releaseOwed&&!sendNag(route,laterCleanup));
}
void cleanupCannotPreemptPendingPress(ContApRoute route){
 resetOwners(route);feed(3000,3,1);continuousApService(route);feed(3001,2,1);continuousApService(route);feed(3002,2,0);continuousApService(route);
 feed(4002,2,0);
 // Pause at the real FSM prepare/admission boundary, before transport result.
 auto in=continuousApInputsLocked(4002,epoch,false);
 auto prepared=continuousApFsm.service(in);
 assert(prepared.kind==ContApCommandKind::Press&&continuousApFsm.hasPendingPress());
 // Simulate the other owner's command having crossed its prepare boundary.
 auto step=tsl9InputScheduler.service(nagInputs(4002,true));
 assert(step.kind==TSL9_INPUT_COMMAND_STEP_PURE);
 auto cleanup=tsl9InputScheduler.onCommandResult(step,false,4002,0);
 assert(cleanup.kind==TSL9_INPUT_COMMAND_CENTER_PURE);
 assert(!sendNag(route,cleanup));
 continuousApFsm.onTxResult(prepared,false,4002);
 assert(!continuousApFsm.hasPendingPress()&&!continuousApSnapshot().releaseOwed);
 assert(sendNag(route,cleanup)); // Retry waiting without a pending press may yield.
}
int main(){
 for(auto route:{ContApRoute::BodyA}){
  cleanupDuringPendingCap(route);cleanupCannotPreemptPendingPress(route);
 }
}
""")
finally:
 fixture.PREFIX = original_prefix
print('Real NAG cleanup yields supported Body route to pending Continuous AP PASS')
