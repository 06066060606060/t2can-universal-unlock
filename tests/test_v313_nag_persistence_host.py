"""Execute production NAG defaults, NVS migration/load/save, and mode switch."""
from pathlib import Path
import os,subprocess,tempfile
from nag_nvs_host_fixture import ROOT,core,extract,make_code
api=(ROOT/'web_api.h').read_text()
code=make_code()
code+=r'''
struct String:std::string{using std::string::string;int toInt()const{return std::atoi(c_str());}};
struct Server{String mode;int status=0;String arg(const char*){return mode;}void send(int s,const char*,const String&){status=s;}}server;
void nagCfgCommit(const NagConfig& c){nagCfg=c;}
String nagCfgToJson(){return "{}";}
'''+extract(api,'httpNagSetMode')+r'''
int main(){
 nagCfgLoad(); assert(!nagCfg.enabled&&!nagCfg.ignoreApState&&nagCfg.mode==MODE_H);
 blobs["nag"].clear();prefs.data["nag"]={};nagCfgLoad();assert(!nagCfg.enabled&&!nagCfg.ignoreApState);
 for(unsigned en=0;en<3;en++)for(unsigned hv: {0u,1u,2u,3u,255u}){
  blobs["nag"].clear();prefs.data["nag"]={{"v",19},{"hv",hv},{"h4pmin",190},{"h4pmax",250}};
  if(en<2)prefs.data["nag"]["en"]=en;
  nagCfgLoad();assert(nagCfg.enabled==(en==1));assert(!nagCfg.ignoreApState);
  assert(nagHumanVariant==1);
  assert(nagHumanV4Config.base.peakMinRaw==190&&nagHumanV4Config.base.peakMaxRaw==250);
 }
 for(bool en:{false,true})for(bool bypass:{false,true}){
  nagCfg.enabled=en;nagCfg.ignoreApState=bypass;nagCfgSave();
  nagCfg.enabled=!en;nagCfg.ignoreApState=!bypass;nagCfgLoad();
  assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);
  for(auto mode:{"0","1","3","7"}){server.mode=mode;httpNagSetMode();assert(server.status==200);assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);nagCfgLoad();assert(nagCfg.enabled==en&&nagCfg.ignoreApState==bypass);}
 }
 // No schema key: preserve an explicit user enabled selection.
 blobs["nag"].clear();prefs.data["nag"]={{"en",1}};nagCfgLoad();assert(nagCfg.enabled&&!nagCfg.ignoreApState);
 blobs["nag"].clear();prefs.data["nag"]={{"en",1},{"iap",1}};nagCfgLoad();assert(nagCfg.enabled&&nagCfg.ignoreApState);
}
'''
with tempfile.TemporaryDirectory(prefix='nag-nvs-host-') as d:
 (Path(d)/'nvs.h').write_text('#pragma once\n');src=Path(d)/'test.cpp';exe=Path(d)/'test';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-I',str(Path(d)),'-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS real NVS defaults/load/save/migration + mode-switch preservation')
