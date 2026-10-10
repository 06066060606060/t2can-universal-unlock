"""Fault injection executes real NAG load/save including atomic snapshots."""
from pathlib import Path
import os,subprocess,tempfile
from nag_nvs_host_fixture import ROOT,make_code
code=make_code()+r'''
int main(){
 disk["nag"]={{"v",20},{"en",1}};failKeys={"nag/v","nag/en"};
 nagCfgLoad();assert(disk["nag"]["en"]==1&&blobs["nag"].empty());
 assert(!nagCfg.enabled&&nagNvsReadError&&!nagCfgSave());
 failKeys.clear();nagCfgLoad();assert(nagCfg.enabled&&!nagNvsReadError);
 nagCfg.pauseAtZeroSpeed=true;testScroll={true,123,1,true,99};assert(nagCfgSave());
 auto before=blobs;NagConfig candidate=nagCfg;candidate.enabled=false;
 failWrite=true;assert(!nagCfgSaveCandidate(candidate));assert(blobs==before&&nagCfg.enabled);
 failWrite=false;failAnyOpen=true;assert(!nagCfgSaveCandidate(candidate));assert(blobs==before&&nagCfg.enabled);
 failAnyOpen=false;nagCfg.enabled=false;testScroll={};nagCfgLoad();
 assert(nagCfg.enabled&&nagCfg.pauseAtZeroSpeed&&testScroll.torqueEnabled&&testScroll.torqueInterval==123&&testScroll.tsl9Interval==99);
 blobDataReadError=true;nagCfgLoad();assert(!nagCfg.enabled&&nagNvsReadError&&blobs==before);
 blobDataReadError=false;nagCfgLoad();assert(nagCfg.enabled);
 blobs["nag"]["snapshot"].pop_back();before=blobs;nagCfgLoad();assert(nagNvsReadError&&!nagCfgSave()&&blobs==before);
}
'''
with tempfile.TemporaryDirectory(prefix='v3282-nag-nvs-') as d:
 (Path(d)/'nvs.h').write_text('#pragma once\n')
 src=Path(d)/'probe.cpp';exe=Path(d)/'probe';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-I',d,'-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS NAG typed read errors, atomic saves, failed publication, scroll snapshot and malformed records')
