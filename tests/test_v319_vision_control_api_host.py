"""Exercise real HTTP parsing and reset allowlists with controlled storage."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
api = (ROOT / 'web_api.h').read_text()

def function(name):
    match = re.search(r'^static\s+[^;{}]*?\b' + name + r'\([^;{}]*?\)\s*\{', api, re.M)
    assert match, name
    end = api.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (api[end] == '{') - (api[end] == '}')
        end += 1
    return api[match.start():end] + '\n'

code = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include "nvs_keep_ble_reset_pure.h"
struct String : std::string { using std::string::string; String(const std::string&s):std::string(s){}; };
struct Server {
  std::vector<std::pair<String,String>> parameters; int status=0;
  int args() { return (int)parameters.size(); }
  bool hasArg(const char *key) { for(auto&p:parameters)if(p.first==key)return true;return false; }
  String arg(const char *key) { for(auto&p:parameters)if(p.first==key)return p.second;return ""; }
  void send(int code,const char*,const String&) { status=code; }
} server;
bool labMenuEnabled=true,supported=true,bodySupported=true,saveOk=true,disabled=false;
uint8_t bus=0; unsigned writes=0;
bool httpRequireLab() { if(labMenuEnabled)return true;server.status=403;return false; }
bool visionControlSupported() { return supported; }
bool visionControlRequestDisabled() { return disabled; }
uint8_t visionControlBusSnapshot() { return bus; }
bool visionControlBusSupported(uint8_t value) { return supported && (value==0 || (value==1&&bodySupported)); }
bool visionControlApplySettings(bool d,uint8_t b) { ++writes;if(!saveOk)return false;disabled=d;bus=b;return true; }
String getVisionControlStatsJson() { return "{}"; }
std::map<std::string,bool> stored;
struct Preferences {
  std::string ns;
  bool begin(const char*name,bool) { ns=name;return true; }
  bool clear() { stored[ns]=false;return true; }
  void end() {}
};
'''
for name in ('httpVisionControlStats', 'httpVisionControlUpdate',
             'resetFirmwareSettingsPreserveProfileAndBle', 'resetNvsKeepBleClearApplicationNamespaces'):
    code += function(name)
code += r'''
void request(std::initializer_list<std::pair<String,String>>fields,int want) {
 server.parameters=fields;server.status=0;httpVisionControlUpdate();assert(server.status==want);
}
int main() {
 request({{"disabled","true"}},200);assert(disabled&&bus==0);
 request({{"bus","1"}},200);assert(disabled&&bus==1);
 request({{"disabled","false"}},200);assert(!disabled&&bus==1);
 request({{"disabled","1"},{"bus","0"}},200);assert(disabled&&bus==0);
 for(const char*raw:{"0","1","true","false"})request({{"disabled",raw}},200);
 unsigned before=writes;
 for(const char*raw:{"","2","01"," 1","1 ","TRUE","truex"})request({{"disabled",raw}},400);
 for(const char*raw:{"","2","01"," 1","true","-1"})request({{"bus",raw}},400);
 request({},400);request({{"disabled","1"},{"extra","1"}},400);
 request({{"disabled","1"},{"disabled","0"}},400);
 request({{"bus","0"},{"bus","1"}},400);assert(writes==before);
 labMenuEnabled=false;request({{"disabled","1"}},200);httpVisionControlStats();assert(server.status==200);assert(writes==before+1);labMenuEnabled=true;
 supported=false;request({{"disabled","1"}},409);request({{"bus","0"}},409);assert(writes==before+1);supported=true;
 bodySupported=false;request({{"bus","1"}},409);assert(writes==before+1);bodySupported=true;
 disabled=true;bus=1;saveOk=false;request({{"disabled","0"},{"bus","0"}},503);assert(disabled&&bus==1);saveOk=true;
 // Both explicit reset actions must remove the new namespace while preserving BLE.
 for(bool keepBle:{false,true}) {
   stored={{"labv3fd",true},{"s3xy",true},{"s3xyreg",true},{"v3profile",true},{"wifiap",true}};
   assert(keepBle?resetNvsKeepBleClearApplicationNamespaces():resetFirmwareSettingsPreserveProfileAndBle());
   assert(!stored["labv3fd"]&&stored["s3xy"]&&stored["s3xyreg"]);
   assert(stored["v3profile"]!=keepBle&&stored["wifiap"]!=keepBle);
 }
}
'''
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS strict production Visual Speed Control HTTP updates, selection preservation and reset cleanup')
