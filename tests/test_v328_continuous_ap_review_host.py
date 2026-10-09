from continuous_ap_host_fixture import run
run(r"""
void offAfterTransport(){afterTransport=nullptr;assert(continuousApControlApply({false,ContApMethod::StalkDouble}));}
void start(uint32_t t,ContApRoute route){feed(t,3,1);continuousApService(route);feed(t+1,2,1);continuousApService(route);feed(t+2,2,0);continuousApService(route);}
void reset(){beforeValidate=afterTransport=nullptr;sent=0;continuousApResetForEpoch(++epoch);activeVehicleProfile=4;activeVehicleTopology=2;continuousApConfig={true,ContApMethod::ScrollDouble};}
int main(){
 reset();activeVehicleProfile=3;continuousApConfig.method=ContApMethod::StalkDouble;start(100,ContApRoute::BodyA);feed(1102,2,0);afterTransport=offAfterTransport;continuousApService(ContApRoute::BodyA);assert(sent==1&&sentData[1]==0x40);feed(1103,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2&&sentData[1]==0x01); // New counter after cancelled first press.
 // Forbidden observations must remain cancelled when a later RX in the same
 // batch restores an otherwise-ready latest value.
 for(int kind=0;kind<3;++kind){reset();start(3000,ContApRoute::BodyA);feed(4002,2,0);uint8_t d[8]={};
  if(kind==0){d[0]=1;continuousApObserveStock(1,0x3f5,d,8,epoch,4002);}
  if(kind==1){d[2]=0x60;continuousApObserveStock(1,0x118,d,8,epoch,4002);}
  if(kind==2){d[0]=15;continuousApObserveStock(1,0x399,d,8,epoch,4002);}
  feed(4003,2,0);continuousApService(ContApRoute::BodyA);assert(sent==0);feed(5003,2,0);continuousApService(ContApRoute::BodyA);assert(sent==0);
 }
}
""")
print('Continuous AP reviewed admission/result races PASS')
