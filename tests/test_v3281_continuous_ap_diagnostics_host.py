from continuous_ap_host_fixture import run,ROOT
import re
s=(ROOT/'web_api.h').read_text()
def function(name):
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',s,re.M);assert m,name
 end=s.index('{',m.start())+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[m.start():end]+'\n'
run(r"""
#include <vector>
using String=std::string;
struct JsonWriterArduino{String &s;bool first=true;JsonWriterArduino(String&o):s(o){s="{";}void key(const char*k){if(!first)s+=",";first=false;s+=String("\"")+k+"\":";}void boolean(const char*k,bool v){key(k);s+=v?"true":"false";}void u32(const char*k,uint32_t v){key(k);s+=std::to_string(v);}void string(const char*k,const char*v){key(k);s+=String("\"")+v+"\"";}void finish(){s+="}";}};
struct Server{std::vector<std::pair<String,String>> query;int status=0;String body;int args(){return query.size();}bool hasArg(const char*k){for(auto &a:query)if(a.first==k)return true;return false;}String arg(const char*k){for(auto&a:query)if(a.first==k)return a.second;return "";}String argName(int n){return query[n].first;}void send(int c,const char*,String b){status=c;body=b;}}server;
"""+function('httpBoolArg')+function('continuousApControlJson')+function('httpContinuousApControlConfig')+function('httpContinuousApControlUpdate')+r"""
int main(){
 activeVehicleTopology=3;
 continuousApConfig={true,ContApMethod::ScrollSingle};
 httpContinuousApControlConfig();
 assert(server.body.find("\"signalReadiness\":\"Waiting for signals\"")!=String::npos);
 feed(100,3,0);httpContinuousApControlConfig();
 assert(server.body.find("\"signalReadiness\":\"Available\"")!=String::npos);
 clockMs=351;httpContinuousApControlConfig();
 assert(server.body.find("\"signalReadiness\":\"Stale / Invalid\"")!=String::npos);
 // Signal availability is about RX, independent of AP/brake/gear permission.
 feed(500,1,0);continuousApBrake.value=2;continuousApGear118.value=1;
 httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Available\"")!=String::npos);
 assert(server.body.find("\"rxTorqueBus\":\"Party CAN A\"")!=String::npos);
 assert(server.body.find("\"rxTorqueId\":\"0x370\"")!=String::npos);
 assert(server.body.find("\"rxBrakeId\":\"0x39D\"")!=String::npos);
 assert(server.body.find("\"rxControlBus\":\"Chassis CAN B\"")!=String::npos);
 for(int n=0;n<6;++n){
  feed(1000,3,0);
  ContApObservation *samples[]={&continuousApAp,&continuousApTurn,&continuousApBrake,&continuousApGear118,&continuousApTorqueParty,&continuousApScrollControl};
  samples[n]->seen=false;httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Waiting for signals\"")!=String::npos);
  feed(1000,3,0);samples[n]->valid=false;httpContinuousApControlConfig();
  assert(server.body.find("\"signalReadiness\":\"Available\"")==String::npos);
  feed(1000,3,0);samples[n]->epoch=epoch+1;httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Available\"")==String::npos);
 }
 feed(0xfffffff0u,3,0);clockMs=20;httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Available\"")!=String::npos);
 clockMs=300;httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Stale / Invalid\"")!=String::npos);
 activeVehicleProfile=1;httpContinuousApControlConfig();assert(server.body.find("\"signalReadiness\":\"Unsupported\"")!=String::npos);
 activeVehicleProfile=4;activeVehicleTopology=2;
 feed(2000,3,0);clockMs=3101;httpContinuousApControlConfig();
 assert(server.body.find("\"rxGearStatus\":\"Stale\"")!=String::npos);
 assert(server.body.find("\"rxTorqueStatus\":\"Stale\"")!=String::npos);
 assert(server.body.find("\"rxGearAgeMs\":1101")!=String::npos);
 feed(3200,3,0);continuousApGear118.valid=false;continuousApTorqueS.valid=false;httpContinuousApControlConfig();
 assert(server.body.find("\"rxGearStatus\":\"Invalid\"")!=String::npos);
 assert(server.body.find("\"rxTorqueStatus\":\"Invalid\"")!=String::npos);


 server.query={{"enabled","1"},{"method","3"}};httpContinuousApControlUpdate();assert(server.status==200&&stored==0x107);
 httpContinuousApControlConfig();assert(server.status==200&&server.body.find("\"enabled\":true")!=String::npos&&server.body.find("\"scrollSingleSupported\":true")!=String::npos);
 for(auto q:std::vector<std::vector<std::pair<String,String>>>{{{"enabled","1"}},{{"enabled","1"},{"method","0"}},{{"enabled","1"},{"method","4"}},{{"enabled","1"},{"method","2junk"}},{{"enabled","1"},{"enabled","0"}},{{"enabled","yes"},{"method","2"}},{{"enabled","1"},{"method","2"},{"unknown","1"}}}){server.query=q;httpContinuousApControlUpdate();assert(server.status==400&&stored==0x107);}
 activeVehicleProfile=1;server.query={{"enabled","1"},{"method","2"}};httpContinuousApControlUpdate();assert(server.status==409&&stored==0x107);
 server.query={{"enabled","0"},{"method","2"}};httpContinuousApControlUpdate();assert(server.status==200&&stored==0x102);
 putOk=false;server.query={{"enabled","0"},{"method","0"}};httpContinuousApControlUpdate();assert(server.status==503&&stored==0x102);
}
""")
print('Continuous AP strict production HTTP config API PASS')
