from continuous_ap_host_fixture import run
run(r"""
int main(){
 continuousApControlLoad();assert(!continuousApControlSnapshot().enabled&&continuousApControlSnapshot().method==ContApMethod::Unset);
 const auto gen=continuousApGeneration;assert(continuousApControlApply({true,ContApMethod::ScrollDouble}));assert(stored==0x107&&continuousApGeneration==gen+1&&continuousApControlSnapshot().enabled);
 beginOk=false;auto previous=continuousApGeneration;assert(!continuousApControlApply({false,ContApMethod::Unset}));assert(continuousApGeneration==previous&&continuousApControlSnapshot().enabled);beginOk=true;
 putOk=false;assert(!continuousApControlApply({false,ContApMethod::Unset}));assert(continuousApGeneration==previous&&continuousApControlSnapshot().enabled);putOk=true;
 held=true;assert(!continuousApControlApply({false,ContApMethod::Unset}));held=false;
 assert(!continuousApControlApply({true,ContApMethod::Unset}));activeVehicleProfile=1;assert(!continuousApControlApply({true,ContApMethod::ScrollSingle}));assert(continuousApControlApply({false,ContApMethod::ScrollSingle}));assert(stored==0x102);
 stored=0x107;continuousApControlLoad();assert(continuousApControlSnapshot().enabled);stored=0x104;continuousApControlLoad();assert(!continuousApControlSnapshot().enabled&&continuousApControlSnapshot().method==ContApMethod::Unset);
 // Durable method change during accepted press owes release through old method.
 activeVehicleProfile=4;assert(continuousApControlApply({true,ContApMethod::ScrollDouble}));feed(100,3,1);continuousApService(ContApRoute::BodyA);feed(101,2,1);continuousApService(ContApRoute::BodyA);feed(102,2,0);continuousApService(ContApRoute::BodyA);feed(1102,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);
 assert(continuousApControlApply({true,ContApMethod::ScrollSingle}));feed(1103,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2&&(sentData[1]&0x30)==0x10);feed(1210,2,0);continuousApService(ContApRoute::BodyA);feed(2210,2,0);continuousApService(ContApRoute::BodyA);assert(sent==2);
}
""")
print('Continuous AP durable config transaction/restore PASS')
