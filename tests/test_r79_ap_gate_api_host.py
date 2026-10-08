"""Run the actual HTTP parser; bad input must not invoke the NVS/runtime transaction."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'web_api.h').read_text()
start=src.index('static void httpR79ApControlUpdate() {')
i=src.index('{',start)+1;depth=1
# The function contains balanced JSON braces inside literals as well.
while depth:
 depth+=(src[i]=='{')-(src[i]=='}');i+=1
handler=src[start:i]
start=src.index('static String r79ApControlStatsToJson() {')
i=src.index('{',start)+1;depth=1
while depth:
 depth+=(src[i]=='{')-(src[i]=='}');i+=1
stats=src[start:i]
code=r'''
#include <cassert>
#include <string>
#include <map>
#include "r79_ap_gate_pure.h"
#include "r79_fixed_policy_pure.h"
#include "runtime_gate_pure.h"
using String=std::string;
struct Server {
 std::map<String,String> args;int status=0;
 bool hasArg(const char*k){return args.count(k);}
 String arg(const char*k){return args[k];}
 void send(int c,const char*,const String&){status=c;}
} server;
bool labMenuEnabled=true, persistOk=true;
int commits=0;
R79ApGateConfigPure saved{false,0,2,false};
bool r79ApControlApply(const R79ApGateConfigPure&c){++commits;if(!persistOk)return false;saved=c;return true;}
R79ApGateConfigPure&r79ApGateConfig=saved;
uint32_t millis(){return 1000u;}int stateMux=0;
void portENTER_CRITICAL(int*){}void portEXIT_CRITICAL(int*){}
bool dasAutopilotStateValid=false;uint8_t dasAutopilotState4=0;
R79ApGateDecisionPure r79ApGateDecisionLocked(uint32_t){return {true,R79_AP_GATE_BYPASS_PURE,0};}
const char*r79ApGateReasonName(uint8_t){return "BYPASS";}
std::map<String,String>json;
struct JsonWriterArduino {
 explicit JsonWriterArduino(String&){json.clear();}
 void boolean(const char*k,bool v){json[k]=v?"true":"false";}
 void u32(const char*k,uint32_t v){json[k]=std::to_string(v);}
 void string(const char*k,const char*v){json[k]=v;}
 void finish(){}
};
'''+stats+'\n'+handler+r'''
int main(){
 server.args={{"enabled","0"},{"mode","0"},{"delaySeconds","2"},{"allowManualDriving","1"}};
 httpR79ApControlUpdate();assert(server.status==200&&saved.allowManualDriving&&!saved.enabled);
 assert(json["allowManualDriving"]=="true");
 for(auto invalid:{"","0","1","11","-2","2.5","2x"," 2","2 ","4294967298","99999999999999999999"}) {
  server.args={{"enabled","1"},{"mode","1"},{"delaySeconds",invalid},{"allowManualDriving","0"}};
  int before=commits;httpR79ApControlUpdate();assert(server.status==400&&commits==before);
 }
 for(auto invalid:{"true","2","-1","","0x1"}) {
  for(auto key:{"enabled","mode","allowManualDriving"}) {
   server.args={{"enabled","1"},{"mode","1"},{"delaySeconds","2"},{"allowManualDriving","0"}};
   server.args[key]=invalid;int before=commits;httpR79ApControlUpdate();assert(server.status==400&&commits==before);
  }
 }
 for(int enable=0;enable<2;++enable)for(int mode=0;mode<2;++mode)for(int delay=2;delay<=10;++delay)for(int manual=0;manual<2;++manual){
  server.args={{"enabled",std::to_string(enable)},{"mode",std::to_string(mode)},{"delaySeconds",std::to_string(delay)},{"allowManualDriving",std::to_string(manual)}};
  httpR79ApControlUpdate();assert(server.status==200);
  assert(saved.enabled==bool(enable)&&saved.mode==mode&&saved.delaySeconds==delay&&saved.allowManualDriving==bool(manual));
  assert(json["allowManualDriving"]==(manual?"true":"false"));
 }
 for(auto key:{"enabled","mode","delaySeconds","allowManualDriving"}){
  server.args={{"enabled","1"},{"mode","1"},{"delaySeconds","2"},{"allowManualDriving","1"}};server.args.erase(key);
  int before=commits;httpR79ApControlUpdate();assert(server.status==400&&commits==before);
 }
 server.args={{"enabled","1"},{"mode","1"},{"delaySeconds","2"},{"allowManualDriving","1"}};
 persistOk=false;httpR79ApControlUpdate();assert(server.status==503);
 labMenuEnabled=false;int before=commits;httpR79ApControlUpdate();assert(server.status==403&&commits==before);
}
'''
with tempfile.TemporaryDirectory(prefix='r79-ap-api-') as d:
 p=Path(d)/'test.cpp';exe=Path(d)/'test';p.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('AP control HTTP parser: 72 valid combinations, independent manual permission, stats, malformed input and failures PASS')
