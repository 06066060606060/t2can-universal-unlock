"""Production observer/FSM/transport regressions for the bounded readiness window."""
from continuous_ap_host_fixture import run

run(r"""
void resetCase(){
 beforeValidate=afterTransport=afterEnqueue=nullptr;sent=0;txAccept=true;
 canTxAdministrativeHold=usbHold=false;mcpReady=twaiReady=true;
 continuousApResetForEpoch(++epoch);activeVehicleProfile=4;activeVehicleTopology=2;
 continuousApConfig={true,ContApMethod::ScrollSingle};tsl9InputScheduler.state={};
}
void service(){continuousApService(ContApRoute::BodyA);}
void tick(uint32_t t,uint8_t ap,uint8_t turn){feed(t,ap,turn);service();}
void control(uint8_t click=1,uint8_t rotation=0,uint8_t tilt=1,bool valid=true){
 uint8_t d[8]={0x29,0x55,0,0,0,0,0x10,0};
 d[1]=(d[1]&0xcf)|(click<<4);d[2]=rotation;d[0]=(d[0]&0xe7)|(tilt<<3);
 if(!valid)d[6]=0;
 continuousApObserveStock(0,0x3c2,d,8,epoch,clockMs);
}
void acceptedPressInterrupted(){
 afterEnqueue=nullptr;control(1,1);control();
}
void torque(int value){
 const unsigned raw=(unsigned)(2050+value);uint8_t d[8]={};
 d[2]=(raw>>8)&15;d[3]=raw&255;
 continuousApObserveStock(0,0x372,d,8,epoch,clockMs);
}
void begin(uint32_t t){tick(t,3,1);tick(t+1,1,1);tick(t+2,1,0);}
int main(){
 // Fresh AP1 after a witnessed active+single-turn intent is eligible to wait only.
 resetCase();tick(100,3,1);tick(101,1,1);
 assert(continuousApSnapshot().state==ContApState::WaitSignalOff);
 tick(102,1,0);tick(1102,1,0);assert(sent==0);
 tick(1500,2,0);assert(sent==1);
 // Signal-off delay remains mandatory even when AP becomes available early.
 resetCase();begin(2000);tick(2500,2,0);assert(sent==0);
 tick(3001,2,0);assert(sent==0);tick(3002,2,0);assert(sent==1);
 // The intent survives a batch in which the blinker is already off at AP loss.
 resetCase();tick(4000,3,1);tick(4001,1,0);tick(4002,1,0);
 tick(5002,2,0);assert(sent==1);
 // Indicator OFF may precede AP loss by a service tick. Retain only the
 // witnessed, still-fresh indicated AP-active intent; keep the full off delay.
 for(uint32_t t:{100000u,0xfffffc00u}){
  resetCase();tick(t,3,1);tick(t+1u,3,0);tick(t+2u,1,0);
  assert(continuousApSnapshot().state==ContApState::WaitDelayAndReady);
  tick(t+1001u,2,0);assert(sent==0);
  tick(t+1002u,2,0);assert(sent==1);
  // A lone OFF RX must work with the previous, fresh AP-active observation.
  resetCase();tick(t,3,1);clockMs=t+1u;uint8_t off[8]={};
  continuousApObserveStock(1,0x3f5,off,8,epoch,clockMs);service();
  tick(t+2u,1,0);tick(t+1001u,2,0);assert(sent==0);
  tick(t+1002u,2,0);assert(sent==1);
  // AP-active/OFF updates cannot refresh the last indicated intent timestamp.
  // The existing freshness bound includes 1000 ms and excludes 1001 ms.
  for(uint32_t age:{1000u,1001u}){
   resetCase();tick(t,3,1);tick(t+500u,3,0);tick(t+900u,3,0);
   tick(t+age,1,0);tick(t+age+1000u,2,0);
   assert(sent==(age==1000u?1:0));
  }
  // AP-active/OFF alone never establishes intent, including across wrap.
  resetCase();tick(t,3,0);tick(t+1u,3,0);tick(t+2u,1,0);
  tick(t+1002u,2,0);assert(sent==0);
  // Hazard, invalid signal, physical click and invalid controls still cancel
  // during the OFF grace, even if a later RX in the batch repairs the sample.
  for(unsigned fault=0;fault<4;++fault){
   resetCase();tick(t,3,1);tick(t+1u,3,0);feed(t+2u,3,0);
   if(fault<2){uint8_t d[8]={};d[0]=fault==0?5:3;
    continuousApObserveStock(1,0x3f5,d,8,epoch,clockMs);
    d[0]=0;continuousApObserveStock(1,0x3f5,d,8,epoch,clockMs);
   } else {control(fault==2?2:1,0,1,fault!=3);control();}
   service();tick(t+3u,1,0);tick(t+1003u,2,0);assert(sent==0);
   assert(continuousApSnapshot().lastCancelReason==
    (fault==1||fault==3?ContApReason::InputUnavailable:ContApReason::DriverInput));
  }
 }
 // Positive intent and AP loss can both be drained before any service tick.
 // Stock observers may retain safe intent but must never transmit a gesture.
 for(uint32_t t:{110000u,0xfffffc00u}){
  resetCase();feed(t,3,1);assert(sent==0);feed(t+1u,1,0);assert(sent==0);service();
  assert(continuousApSnapshot().state==ContApState::WaitDelayAndReady);
  tick(t+1000u,2,0);assert(sent==0);tick(t+1001u,2,0);assert(sent==1);
  // A cancelling event in the same batch cannot be erased by later safe stock.
  for(unsigned fault=0;fault<5;++fault){
   resetCase();feed(t,3,1);assert(sent==0);clockMs=t+1u;uint8_t d[8]={};
   if(fault==0){d[2]=2;continuousApObserveStock(1,0x39d,d,3,epoch,clockMs);}
   if(fault==1){d[2]=0x60;continuousApObserveStock(1,0x118,d,8,epoch,clockMs);}
   if(fault==2){d[0]=5;continuousApObserveStock(1,0x3f5,d,8,epoch,clockMs);}
   if(fault==3){d[0]=15;continuousApObserveStock(1,0x399,d,8,epoch,clockMs);}
   if(fault==4)control(2);
   feed(t+2u,1,0);service();tick(t+1002u,2,0);assert(sent==0);
  }
  // Missing required state at the active observation cannot be supplied later
  // by AP-inactive stock to retroactively authorize reactivation.
  resetCase();feed(t,3,0);continuousApTorqueS={};
  uint8_t turn[8]={1};continuousApObserveStock(1,0x3f5,turn,8,epoch,t);
  feed(t+1u,1,0);service();tick(t+1001u,2,0);assert(sent==0);
  // A physical click before any intent is still subject to the quiet period.
  resetCase();clockMs=t;control(2);feed(t+1u,3,1);feed(t+2u,1,0);service();
  tick(t+1002u,2,0);assert(sent==0);
 }
 // An active AP observation without single-turn intent never authorizes resume.
 resetCase();tick(6000,3,0);tick(6001,1,0);tick(7002,2,0);assert(sent==0);
 resetCase();tick(8000,3,0);tick(8001,2,0);tick(9002,2,0);assert(sent==0);
 // High torque exceeding five seconds can recover inside the original ten seconds.
 resetCase();begin(10000);
 for(uint32_t t=11002;t<=17002;t+=1000){feed(t,2,0);torque(400);service();assert(sent==0);}
 tick(17500,2,0);assert(sent==1);
 // Rotation and tilt defer the gesture without dropping intent or the deadline.
 for(unsigned kind=0;kind<2;++kind){
  resetCase();begin(20000);
  for(uint32_t t=21002;t<=27002;t+=1000){feed(t,2,0);control(1,kind==0?1:0,kind==1?2:1);service();assert(sent==0);}
  tick(27500,2,0);assert(sent==1);
 }
 // A real click latched during RX cannot be erased by a later neutral sample.
 resetCase();begin(30000);feed(31002,2,0);control(2);control();service();assert(sent==0);
 tick(32002,2,0);assert(sent==0);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::DriverInput);
 // Invalid controls remain cancelling even if the same batch repairs the sample.
 resetCase();begin(33000);feed(34002,2,0);control(1,0,1,false);control();service();assert(sent==0);
 tick(35002,2,0);assert(sent==0);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::InputUnavailable);
 // Physical stalk use cancels a stalk-mode pending episode, even after neutral RX.
 resetCase();activeVehicleProfile=3;continuousApConfig.method=ContApMethod::StalkDouble;
 begin(35010);feed(35500,2,0);ContApStockFrame stalk;
 continuousApBuildStalkPure(4,1,stalk);continuousApObserveStock(0,0x229,stalk.data,3,epoch,clockMs);
 continuousApBuildStalkPure(0,2,stalk);continuousApObserveStock(0,0x229,stalk.data,3,epoch,clockMs);
 service();tick(36500,2,0);assert(sent==0);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::DriverInput);
 // Stale required vehicle/control samples cancel; availability cannot revive intent.
 resetCase();begin(36000);clockMs=37002;service();assert(sent==0);
 tick(38002,2,0);assert(sent==0);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::Stale);
 // AP1 after an accepted press is a cancellation, never a renewed waiting window.
 resetCase();begin(40000);tick(41002,2,0);assert(sent==1);
 tick(41003,1,0);assert(continuousApSnapshot().state==ContApState::CleanupPending||continuousApSnapshot().state==ContApState::Idle);
 tick(41254,2,0);tick(42000,2,0);tick(43500,2,0);
 assert(sent<=2);assert(continuousApSnapshot().state==ContApState::Idle);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::ApState);
 // Unsafe AP states and brake/gear/transport faults do not receive grace time.
 for(uint8_t ap:{0,8,9,14,15}){
  resetCase();begin(45000);tick(46002,ap,0);tick(47002,2,0);assert(sent==0);
  assert(continuousApSnapshot().lastCancelReason==ContApReason::ApState);
 }
 for(unsigned fault=0;fault<3;++fault){
  resetCase();begin(48000);feed(49002,2,0);uint8_t d[8]={};
  if(fault==0){d[2]=2;continuousApObserveStock(1,0x39d,d,3,epoch,clockMs);}
  if(fault==1){d[2]=0x20;continuousApObserveStock(1,0x118,d,8,epoch,clockMs);}
  if(fault==2)canTxAdministrativeHold=true;
  service();canTxAdministrativeHold=false;tick(50002,2,0);assert(sent==0);
  assert(continuousApSnapshot().lastCancelReason==(fault==0?ContApReason::Brake:fault==1?ContApReason::Gear:ContApReason::Transport));
 }
 // Signal wait stays bounded at six seconds from AP loss.
 resetCase();tick(51000,3,1);tick(51001,1,1);tick(57001,1,1);
 tick(57002,2,0);tick(58002,2,0);assert(sent==0);
 // Total window expires at ten seconds, including unavailable/control-busy time.
 resetCase();begin(60000);tick(69000,1,0);tick(70001,2,0);assert(sent==0);
 tick(71001,2,0);assert(sent==0);
 // Millisecond wrap uses elapsed unsigned time for both readiness and expiry.
 resetCase();const uint32_t wrap=0xfffffc00u;begin(wrap);
 tick(wrap+1001u,2,0);assert(sent==0);tick(wrap+1002u,2,0);assert(sent==1);
 resetCase();begin(wrap);tick(wrap+9999u,1,0);tick(wrap+10001u,2,0);assert(sent==0);
 // Retries remain capped at three attempts, with no additional press afterward.
 resetCase();begin(80000);
 for(uint32_t t=81002;t<=89502;t+=50)tick(t,2,0);
 assert(continuousApSnapshot().attempts==3);assert(sent==6);
 tick(90002,2,0);assert(sent==6);
 // Once a press is accepted, wheel activity must retain bounded cleanup.
 for(unsigned kind=0;kind<2;++kind){
  resetCase();begin(95000);tick(96002,2,0);assert(sent==1);
  feed(96003,2,0);control(1,kind==0?1:0,kind==1?2:1);service();
  assert(continuousApSnapshot().state==ContApState::CleanupPending);
  feed(96504,2,0);control(1,kind==0?1:0,kind==1?2:1);service();
  assert(continuousApSnapshot().cleanupUnconfirmed);
  assert(!continuousApSnapshot().releaseOwed);
  tick(97500,2,0);assert(sent==1);
 }
 // A held-input interruption cannot disappear inside a drained RX batch.
 resetCase();begin(98000);tick(99002,2,0);assert(sent==1);
 feed(99003,2,0);control(1,1);control();service();
 assert(continuousApSnapshot().state==ContApState::CleanupPending);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::DriverInput);
 // Preserve RX interruption between enqueue acceptance and TX bookkeeping.
 resetCase();begin(99500);feed(100502,2,0);afterEnqueue=acceptedPressInterrupted;service();
 assert(sent==1&&continuousApCancelLatched);
 clockMs=100503;service();
 assert(continuousApSnapshot().state==ContApState::CleanupPending);
 assert(continuousApSnapshot().lastCancelReason==ContApReason::DriverInput);
 // Successful AP activation completes an episode without inventing a cancellation.
 resetCase();begin(100000);tick(101002,2,0);assert(sent==1);
 const auto priorCancel=continuousApSnapshot().lastCancelReason;
 tick(101003,3,0);
 assert(continuousApSnapshot().lastCancelReason==priorCancel);
 assert(continuousApSnapshot().lastCancelReason!=ContApReason::ApState);
 assert(continuousApSnapshot().reason==ContApReason::Complete||continuousApSnapshot().state==ContApState::CleanupPending);

}
""")
print('v3.29.1 production runtime readiness, cancellation, bounds PASS')
