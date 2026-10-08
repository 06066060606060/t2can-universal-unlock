"""Execute production NAG defaults, NVS migration/load/save, and mode switch."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
core=(ROOT/'can_core.h').read_text()
api=(ROOT/'web_api.h').read_text()
def extract(src,name):
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',src,re.M);assert m,name
 i=src.index('{',m.start())+1;depth=1
 while depth:
  depth+=(src[i]=='{')-(src[i]=='}');i+=1
 return src[m.start():i]
code=r'''
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "das_status_pure.h"
#include "tsl9_hands_on_0x399_pure.h"
#include "tsl9_input_scheduler_pure.h"
#include "nag_human_v4_pure.h"
#include "nag_mode_h_variant_pure.h"
#include "vehicle_profile.h"
#define T2CAN_SERIAL_DIAGNOSTICS 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
struct Preferences {
 std::map<std::string,std::map<std::string,unsigned>> data;
 std::string ns;
 bool fail=false,failWrite=false;
 bool begin(const char*n,bool ro){ns=n;return !fail&&(!ro||data.count(ns));}
 bool isKey(const char*k){return data[ns].count(k);}
 unsigned get(const char*k,unsigned d){auto &m=data[ns];return m.count(k)?m[k]:d;}
 bool getBool(const char*k,bool d){return get(k,d);}
 uint8_t getUChar(const char*k,uint8_t d){return get(k,d);}
 uint16_t getUShort(const char*k,uint16_t d){return get(k,d);}
 size_t getBytes(const char*,void*,size_t){return 0;}
 size_t putBool(const char*k,bool v){if(failWrite)return 0;data[ns][k]=v;return 1;}
 size_t putUChar(const char*k,uint8_t v){data[ns][k]=v;return 1;}
 size_t putUShort(const char*k,uint16_t v){data[ns][k]=v;return 2;}
 size_t putBytes(const char*,const void*,size_t n){return n;}
 void end(){}
} prefs;
bool activeProfileNagTorqueSupported(){return true;}
bool activeProfileNagTsl9Supported(){return true;}
void nagRxSelectorsRefreshFromConfig(){}
uint8_t nagHumanVariant=1;
NagHumanV4ConfigPure nagHumanV4Config=nagHumanV4DefaultConfigPure();
NagHumanV4StatePure nagHumanV4State={};
bool nagHumanV4Initialized=false;
'''
code+=core[core.index('static const uint16_t NAG_TORQUE_RAW_MAX'):core.index('static portMUX_TYPE nagCfgMux')]
for name in ('nagHumanRuntimeLoadDefaults','nagClampTorque','nagCfgSetCommonDefaults','nagCfgDefaultsModeA','nagCfgDefaultsModeB','nagCfgDefaultsModeC','nagCfgDefaultsModeH','nagCfgClampAll','nagCfgApplyActiveProfilePolicy','nagModePersistedIdSupported','nagCfgLoad','nagCfgSave'):
 code+=extract(core,name)+'\n'
code+=r'''
struct String:std::string{using std::string::string;int toInt()const{return std::atoi(c_str());}};
struct Server{String mode;int status=0;String arg(const char*){return mode;}void send(int s,const char*,const String&){status=s;}}server;
void nagCfgCommit(const NagConfig& c){nagCfg=c;}
String nagCfgToJson(){return "{}";}
'''+extract(api,'httpNagSetMode')+r'''
int main(){
 nagCfgLoad(); assert(!nagCfg.enabled&&!nagCfg.ignoreApState&&nagCfg.mode==MODE_H);
 prefs.data["nag"]={};nagCfgLoad();assert(!nagCfg.enabled&&!nagCfg.ignoreApState);
 for(unsigned en=0;en<3;en++)for(unsigned hv: {0u,1u,2u,3u,255u}){
  prefs.data["nag"]={{"v",19},{"hv",hv},{"h4pmin",190},{"h4pmax",250}};
  if(en<2)prefs.data["nag"]["en"]=en;
  nagCfgLoad();assert(nagCfg.enabled==(en==1));assert(!nagCfg.ignoreApState);
  assert(nagHumanVariant==1&&prefs.data["nag"]["hv"]==1);
  assert(nagHumanV4Config.base.peakMinRaw==190&&nagHumanV4Config.base.peakMaxRaw==250);
 }
 for(bool en:{false,true})for(bool bypass:{false,true}){
  nagCfg.enabled=en;nagCfg.ignoreApState=bypass;nagCfgSave();
  nagCfg.enabled=!en;nagCfg.ignoreApState=!bypass;nagCfgLoad();
  assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);
  for(auto mode:{"0","1","3","7"}){server.mode=mode;httpNagSetMode();assert(server.status==200);assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);nagCfgLoad();assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);}
 }
 // No schema key: preserve an explicit user enabled selection.
 prefs.data["nag"]={{"en",1}};nagCfgLoad();assert(nagCfg.enabled&&!nagCfg.ignoreApState);
 prefs.data["nag"]={{"en",1},{"iap",1}};nagCfgLoad();assert(nagCfg.enabled&&nagCfg.ignoreApState);
}
'''
with tempfile.TemporaryDirectory(prefix='nag-nvs-host-') as d:
 src=Path(d)/'test.cpp';exe=Path(d)/'test';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS real NVS defaults/load/save/migration + mode-switch preservation')
