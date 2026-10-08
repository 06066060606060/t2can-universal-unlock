"""Run the production update handler: strict input and failed NVS writes."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
support=ROOT/'tests/test_v313_nag_persistence_host.py'
ns={'__file__':str(support)}
exec(compile(support.read_text().split('with tempfile.TemporaryDirectory')[0],str(support),'exec'),ns)
code=ns['code'].split('struct String:')[0]
code+=r'''
struct String:std::string {
 using std::string::string;
 String(const std::string&s):std::string(s){}
 String(unsigned i):std::string(std::to_string(i)){}
 int toInt()const{return std::atoi(c_str());}
 char charAt(size_t i)const{return at(i);}
};
struct Server {
 std::map<std::string,String> args;int status=0;
 bool hasArg(const String& k){return args.count(k);}
 String arg(const String& k){return args[k];}
 void send(int s,const char*,const String&){status=s;}
}server;
void nagCfgCommit(const NagConfig&c){nagCfg=c;}
String nagCfgToJson(){return "{}";}
bool nagTorqueRightScrollEnabled=false,nagTsl9RightPeriodicEnabled=false;
uint16_t nagTorqueRightScrollIntervalSeconds=30,nagTsl9RightPeriodicIntervalSeconds=30;
uint8_t nagTorqueRightScrollPattern=1,activeVehicleProfile=2,activeVehicleTopology=3;
uint8_t nagRightScrollPatternSanitize(uint8_t n){return n==0?0:1;}
bool quiesce=true,hold=false;
bool tsl9InputQuiesceForConfig(){return quiesce;}
void setCanTxAdministrativeHold(bool h){hold=h;}
void nagTsl9RuntimeReset(){}
void featureCfgSave(){}
void tsl9InputHardResetAfterQuiesce(bool){}
void nagExactEchoReset(){}
void nagHumanRuntimeReset(bool){}
'''
extract=ns['extract'];api=(ROOT/'web_api.h').read_text();core=(ROOT/'can_core.h').read_text()
if 'static bool nagIgnoreApStatePersist' in core:code+=extract(core,'nagIgnoreApStatePersist')
for f in ('httpBoolArg','httpDecimalU16InRange','httpNagUpdate'):code+=extract(api,f)
code+=r'''
int main(){
 nagCfgDefaultsModeH(nagCfg);nagCfgSave();
 for(const auto*bad:{"", "2", "TRUE", "true!", " 1", "-1"}){
  server.args={{"ignoreApState",bad}};httpNagUpdate();assert(server.status==400&&!nagCfg.ignoreApState);
 }
 for(const auto*value:{"1","0","true","false","on","off"}){
  server.args={{"ignoreApState",value}};httpNagUpdate();assert(server.status==200);
  bool expected=std::string(value)=="1"||std::string(value)=="true"||std::string(value)=="on";
  assert(nagCfg.ignoreApState==expected);nagCfgLoad();assert(nagCfg.ignoreApState==expected);
 }
 // Rejected save must not publish a runtime-only bypass or claim success.
 prefs.fail=true;server.args={{"ignoreApState","1"}};httpNagUpdate();
 assert(server.status==500&&!nagCfg.ignoreApState&&!hold);
 prefs.fail=false;
 prefs.failWrite=true;httpNagUpdate();assert(server.status==500&&!nagCfg.ignoreApState&&!hold);prefs.failWrite=false;
 // TSL9 selection preserves the setting without changing its AP gate implementation.
 server.args={{"ignoreApState","1"},{"method","1"}};httpNagUpdate();assert(server.status==200&&nagCfg.method==1&&nagCfg.ignoreApState);
 server.args={{"method","0"}};httpNagUpdate();assert(server.status==200&&nagCfg.ignoreApState);
 // Failed input cleanup leaves the bypass and persisted value unchanged.
 quiesce=false;server.args={{"ignoreApState","0"},{"method","1"}};httpNagUpdate();assert(server.status==503&&nagCfg.ignoreApState);nagCfgLoad();assert(nagCfg.ignoreApState);
}
'''
with tempfile.TemporaryDirectory(prefix='nag-api-host-') as d:
 src=Path(d)/'test.cpp';exe=Path(d)/'test';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS real NAG update API: parsing, NVS failure, method changes, cleanup rejection')
