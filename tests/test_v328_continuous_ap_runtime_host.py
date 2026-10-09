from continuous_ap_host_fixture import run
run(r"""
void start(uint32_t t,ContApRoute route){feed(t,3,1);continuousApService(route);feed(t+1,2,1);continuousApService(route);feed(t+2,2,0);continuousApService(route);}
void cancelImmediately(){continuousApConfig.enabled=false;++continuousApGeneration;continuousApFsm.cancel(continuousApGeneration,epoch,clockMs);}
void brakeImmediately(){uint8_t d[3]={0,0,2};continuousApObserveStock(1,0x39d,d,3,epoch,clockMs);}
void epochImmediately(){continuousApResetForEpoch(++epoch);}
void reset(){beforeValidate=afterTransport=nullptr;sent=0;continuousApResetForEpoch(++epoch);activeVehicleProfile=4;activeVehicleTopology=2;continuousApConfig={true,ContApMethod::ScrollDouble};tsl9InputScheduler.state={};}
int main(){
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
 // Scroll is transmitted on Chassis B for Party+Chassis; Body ID aliases rejected.
 reset();activeVehicleTopology=3;start(14000,ContApRoute::ChassisB);feed(15002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==0);continuousApService(ContApRoute::ChassisB);assert(sent==1&&sentId==0x3c2);
 // Legacy Body stalk counter wraps 15 -> 0, CRC fixture is constrained.
 reset();activeVehicleProfile=3;continuousApConfig.method=ContApMethod::StalkDouble;start(16000,ContApRoute::BodyA);feed(17002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1&&sentId==0x229&&sentData[1]==0x40&&sentData[0]==(0x46^0xf1));
 reset();activeVehicleProfile=1;start(18000,ContApRoute::BodyA);feed(19002,2,0);continuousApService(ContApRoute::BodyA);continuousApService(ContApRoute::ChassisB);assert(sent==0);
 reset();start(20000,ContApRoute::BodyA);feed(21002,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);held=true;continuousApService(ContApRoute::BodyA);held=false;assert(continuousApSnapshot().releaseOwed);feed(21252,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2);
}
""")
print('Continuous AP production runtime route/gesture/cancellation PASS')
