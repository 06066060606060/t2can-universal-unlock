from continuous_ap_host_fixture import run
run(r"""
void start(uint32_t t,ContApRoute route){feed(t,3,1);continuousApService(route);feed(t+1,2,1);continuousApService(route);feed(t+2,2,0);continuousApService(route);}
void cancelImmediately(){continuousApConfig.enabled=false;++continuousApGeneration;continuousApFsm.cancel(continuousApGeneration,epoch,clockMs);}
void brakeImmediately(){uint8_t d[3]={0,0,2};continuousApObserveStock(1,0x39d,d,3,epoch,clockMs);}
void epochImmediately(){continuousApResetForEpoch(++epoch);}
void reset(){beforeValidate=afterTransport=nullptr;sent=0;continuousApResetForEpoch(++epoch);activeVehicleProfile=4;activeVehicleTopology=2;continuousApConfig={true,ContApMethod::ScrollDouble};tsl9InputScheduler.state={};}
void espBrake(uint8_t bus,uint8_t value,uint32_t t,uint8_t dlc=8){uint8_t d[8]={};d[3]=value<<5;continuousApObserveStock(bus,0x145,d,dlc,epoch,t);}
void feedWithoutBrake(uint32_t t,uint8_t ap,uint8_t turn){
 clockMs=t;uint8_t d[8]={};d[0]=ap;continuousApObserveStock(1,0x399,d,8,epoch,t);
 d[0]=turn;continuousApObserveStock(1,0x3f5,d,8,epoch,t);
 d[0]=0;d[2]=0x80;continuousApObserveStock(1,0x118,d,8,epoch,t);
 d[2]=8;d[3]=2;continuousApObserveStock(0,activeVehicleTopology==2?0x372:0x370,d,8,epoch,t);
 memset(d,0,8);d[0]=0x29;d[1]=0x55;d[6]=0x10;continuousApObserveStock(activeVehicleTopology==2?0:1,0x3c2,d,8,epoch,t);
}
void feedEsp(uint32_t t,uint8_t ap,uint8_t turn,uint8_t bus){espBrake(bus,1,t);feedWithoutBrake(t,ap,turn);}
void startEsp(uint32_t t,uint8_t bus){feedEsp(t,3,1,bus);continuousApService(ContApRoute::BodyA);feedEsp(t+1,2,1,bus);continuousApService(ContApRoute::BodyA);feedEsp(t+2,2,0,bus);continuousApService(ContApRoute::BodyA);}
void advanceOneMs(){++clockMs;}
void espBrakeImmediately(){espBrake(0,2,clockMs);espBrake(1,1,clockMs);}
ContApObservation selectedBrake(uint32_t now){return continuousApInputsLocked(now,epoch,false).brake;}
int main(){
 // ESP_status alone must provide the mandatory brake interlock on either bus.
 for(uint8_t bus:{uint8_t(0),uint8_t(1)}){
  reset();startEsp(50000,bus);feedEsp(51002,2,0,bus);continuousApService(ContApRoute::BodyA);
  assert(sent==1&&sentId==0x3c2&&(sentData[1]&0x30)==0x20);
 }
 reset();
 continuousApConfig={true,ContApMethod::ScrollDouble};
 feed(100,3,1);continuousApService(ContApRoute::BodyA);
 feed(101,2,1);continuousApService(ContApRoute::BodyA);
 feed(102,2,0);continuousApService(ContApRoute::BodyA);
 feed(1102,2,0);continuousApService(ContApRoute::BodyA);
 assert(sent==1&&sentId==0x3c2&&(sentData[1]&0x30)==0x20);
 continuousApConfig.enabled=false;++continuousApGeneration;continuousApFsm.cancel(continuousApGeneration,epoch,clockMs);
 feed(1103,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2&&(sentData[1]&0x30)==0x10);
 continuousApResetForEpoch(++epoch);feed(1104,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2);
 // Observers never send. Wrong-route aliases cannot overwrite AP or templates.
 reset();feed(2000,3,1);assert(sent==0);uint8_t wrong[8]={2};continuousApObserveStock(0,0x399,wrong,8,epoch,2001);assert(continuousApAp.value==3);
 start(2100,ContApRoute::BodyA);feed(3102,2,0);beforeValidate=cancelImmediately;continuousApService(ContApRoute::BodyA);assert(sent==0);
 reset();start(4000,ContApRoute::BodyA);feed(5002,2,0);beforeValidate=brakeImmediately;continuousApService(ContApRoute::BodyA);assert(sent==0&&continuousApCancelLatched);
 beforeValidate=nullptr;continuousApService(ContApRoute::BodyA);assert(sent==0&&!continuousApCancelLatched);
 reset();start(6000,ContApRoute::BodyA);feed(7002,2,0);beforeValidate=epochImmediately;continuousApService(ContApRoute::BodyA);assert(sent==0);
 reset();start(8000,ContApRoute::BodyA);clockMs=9002;continuousApService(ContApRoute::BodyA);assert(sent==0); // Mandatory states stale.
 reset();start(10000,ContApRoute::BodyA);feed(11002,2,0);tsl9InputScheduler.state.cleanupPending=true;continuousApService(ContApRoute::BodyA);assert(sent==0);
 // Identical press echo is ambiguous and blocks subsequent work, not treated as confirmation.
 reset();start(12000,ContApRoute::BodyA);feed(13002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);continuousApObserveStock(0,sentId,sentData,8,epoch,13003);assert(continuousApEchoAmbiguous);
 clockMs=13042;continuousApService(ContApRoute::BodyA);assert(sent==1);
 // Party+Chassis never activates Continuous AP, even with every input fresh.
 for(auto method:{ContApMethod::StalkDouble,ContApMethod::ScrollSingle,ContApMethod::ScrollDouble}){
  reset();activeVehicleTopology=3;continuousApConfig.method=method;
  start(14000,ContApRoute::ChassisB);feed(15002,2,0);
  continuousApService(ContApRoute::BodyA);continuousApService(ContApRoute::ChassisB);
  assert(sent==0&&!continuousApSnapshot().releaseOwed);
  assert(continuousApSnapshot().reason==ContApReason::Unsupported);
 }
 // Legacy Body stalk counter wraps 15 -> 0, CRC fixture is constrained.
 reset();activeVehicleProfile=3;continuousApConfig.method=ContApMethod::StalkDouble;start(16000,ContApRoute::BodyA);feed(17002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1&&sentId==0x229&&sentData[1]==0x40&&sentData[0]==(0x46^0xf1));
 reset();activeVehicleProfile=1;start(18000,ContApRoute::BodyA);feed(19002,2,0);continuousApService(ContApRoute::BodyA);continuousApService(ContApRoute::ChassisB);assert(sent==0);
 reset();start(20000,ContApRoute::BodyA);feed(21002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);held=true;continuousApService(ContApRoute::BodyA);held=false;assert(continuousApSnapshot().releaseOwed);feed(21252,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2);
 // Scroll template admission includes 500 ms, while 501 ms cannot enqueue.
 for(uint32_t age:{500u,501u}){
  reset();start(30000,ContApRoute::BodyA);feed(31002,2,0);
  continuousApScrollControl.rxMs=31002-age;continuousApScrollStock.rxMs=31002-age;
  continuousApService(ContApRoute::BodyA);assert(sent==(age==500?1:0));
 }

 // 0x145 is observed on both physical buses, including Party A. Party still cannot engage.
 reset();activeVehicleTopology=3;espBrake(0,1,60000);feedWithoutBrake(60000,3,1);
 assert(selectedBrake(60000).seen&&selectedBrake(60000).valid&&selectedBrake(60000).value==1);
 continuousApService(ContApRoute::BodyA);continuousApService(ContApRoute::ChassisB);assert(sent==0);
 // Arbitration prefers the freshest released source, including the legacy 0x39D source.
 reset();feed(61000,3,1);espBrake(0,1,61010);
 assert(selectedBrake(61020).sourceId==0x145&&selectedBrake(61020).rxMs==61010);
 feed(61030,3,1);assert(selectedBrake(61030).sourceId==0x39d&&selectedBrake(61030).rxMs==61030);
 espBrake(1,1,61040);assert(selectedBrake(61040).sourceId==0x145&&selectedBrake(61040).rxMs==61040);
 // Pressed/invalid on either ESP source cannot be masked by newer release elsewhere,
 // even after the bad observation ages out. Only that source's release clears it.
 for(uint8_t bus:{uint8_t(0),uint8_t(1)})for(uint8_t value:{uint8_t(0),uint8_t(2),uint8_t(3)}){
  reset();espBrake(bus,value,62000);espBrake(1-bus,1,62001);feed(62002,3,1);
  const auto bad=selectedBrake(62002);assert(bad.seen&&(!bad.valid||bad.value!=1));
  feed(62300,3,1);espBrake(1-bus,1,62300);
  const auto aged=selectedBrake(62300);ContApInputs i;i.nowMs=62300;i.epoch=epoch;
  assert(!continuousApFreshPure(aged,i,250));
  espBrake(bus,1,62301);const auto released=selectedBrake(62301);
  assert(released.valid&&released.value==1&&released.rxMs==62301);
 }
 // Legacy 0x39D brake press retains the same protection against newer ESP release.
 reset();clockMs=63000;brakeImmediately();espBrake(0,1,63001);espBrake(1,1,63300);
 assert(selectedBrake(63300).value==2&&selectedBrake(63300).rxMs==63000);
 feed(63301,3,1);assert(selectedBrake(63301).valid&&selectedBrake(63301).value==1);
 // Wrong DLC is observed as invalid rather than satisfying the release requirement.
 for(uint8_t dlc:{uint8_t(0),uint8_t(3),uint8_t(4),uint8_t(7)}){
  reset();espBrake(0,1,64000,dlc);espBrake(1,1,64001);
  assert(selectedBrake(64001).seen&&!selectedBrake(64001).valid);
 }
 // Same drained batch: a pressed event remains latched when another source releases.
 reset();startEsp(65000,0);espBrake(0,2,65003);espBrake(1,1,65003);
 assert(continuousApCancelLatched);continuousApService(ContApRoute::BodyA);
 assert(sent==0&&continuousApSnapshot().lastCancelReason==ContApReason::Brake);
 // Final transport validation rechecks a newly observed ESP brake cancellation.
 reset();startEsp(66000,0);feedEsp(67002,2,0,0);beforeValidate=espBrakeImmediately;
 continuousApService(ContApRoute::BodyA);assert(sent==0&&continuousApCancelLatched);
 beforeValidate=nullptr;continuousApService(ContApRoute::BodyA);assert(sent==0&&!continuousApCancelLatched);
 // Brake freshness remains exactly 250 ms, including across uint32_t wrap.
 for(uint32_t base:{68000u,UINT32_MAX-901u})for(uint8_t bus:{uint8_t(0),uint8_t(1)})for(uint32_t age:{250u,251u}){
  reset();startEsp(base,bus);const uint32_t now=base+1002u;
  espBrake(bus,1,now-age);feedWithoutBrake(now,2,0);continuousApService(ContApRoute::BodyA);
  assert(sent==(age==250?1:0));
 }
 // A brake that crosses 250 -> 251 ms after preparation is rejected at final TX.
 for(uint8_t bus:{uint8_t(0),uint8_t(1)}){
  reset();startEsp(72000,bus);espBrake(bus,1,72752);feedWithoutBrake(73002,2,0);
  beforeValidate=advanceOneMs;continuousApService(ContApRoute::BodyA);assert(sent==0);
 }
 // A stale release on A cannot win over a fresh release on B; modulo age wins at wrap.
 reset();espBrake(0,1,UINT32_MAX-100u);espBrake(1,1,20u);
 assert(selectedBrake(50u).valid&&selectedBrake(50u).rxMs==20u);
 // Epoch reset discards each source's pressed/invalid blocker.
 reset();espBrake(0,2,69000);espBrake(1,3,69000);clockMs=69000;brakeImmediately();
 continuousApResetForEpoch(++epoch);assert(!selectedBrake(69001).seen);
 startEsp(70000,1);feedEsp(71002,2,0,1);continuousApService(ContApRoute::BodyA);assert(sent==1);
}
""")
print('Continuous AP production runtime route/gesture/cancellation PASS')
