"""The production pending-reset guard cannot hide a read error as false."""
import os,subprocess,tempfile
from pathlib import Path
from test_v3282_profile_nvs_host import PREAMBLE
from nag_nvs_host_fixture import ROOT,extract
api=(ROOT/'web_api.h').read_text()
code=PREAMBLE.replace('constexpr nvs_type_t', '[[maybe_unused]] constexpr nvs_type_t')+r'''
static constexpr const char *NVS_KEEP_BLE_RESET_GUARD_NAMESPACE="t2reset";
static constexpr const char *NVS_KEEP_BLE_RESET_GUARD_KEY="pending";
'''+extract(api,'resetNvsKeepBleGuardRead')+r'''
int main(){
 bool active=true;
 disk["t2reset"]["pending"]=1;failKeys={"t2reset/pending"};
 assert(!resetNvsKeepBleGuardRead(active));
 failKeys.clear();assert(resetNvsKeepBleGuardRead(active)&&active);
 disk["t2reset"]["pending"]=0;assert(resetNvsKeepBleGuardRead(active)&&!active);
 disk.erase("t2reset");assert(resetNvsKeepBleGuardRead(active)&&!active);
}
'''
with tempfile.TemporaryDirectory(prefix='v3282-reset-guard-') as d:
 (Path(d)/'nvs.h').write_text('#pragma once\n')
 src=Path(d)/'probe.cpp';exe=Path(d)/'probe';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-unused-function','-I',d,'-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS reset guard: typed read failure, ON/OFF, genuinely absent namespace')
