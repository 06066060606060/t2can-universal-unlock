"""Exercise every eligibility input against the production pure gate."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
header=(ROOT/'runtime_gate_pure.h').read_text()
# The old API has no bypass argument; that baseline ignores this requested flag.
extra=', bypass' if 'bool ignoreApState' in header else ''
code=r'''
#include <cassert>
#include "runtime_gate_pure.h"
int main(){
 for(unsigned flags=0;flags<256;flags++) {
  bool en=flags&1, boot=flags&2, seen=flags&4, own=flags&8;
  bool valid=flags&16, ap=flags&32, bypass=flags&64;
  uint8_t ho=(flags&128)?2:0;
  auto expected=!en?NAG_SKIP_DISABLED:!boot?NAG_SKIP_BOOT_DELAY:!seen?NAG_SKIP_WARMUP:own?NAG_SKIP_SELF_FRAME:ho>1?NAG_SKIP_HANDS_ON:(!bypass&&!valid)?NAG_SKIP_AP_INVALID:(!bypass&&!ap)?NAG_SKIP_AP_INACTIVE:NAG_SKIP_NONE;
  assert(nagEligibilityReasonPure(en,boot,seen,own,ho,valid,apEXTRA)==expected);
 }
}
'''.replace('apEXTRA','ap'+extra)
with tempfile.TemporaryDirectory() as d:
 src=Path(d)/'test.cpp';exe=Path(d)/'test';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS 256 torque AP policy combinations; other gates unchanged')
