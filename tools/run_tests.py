#!/usr/bin/env python3
"""Run all host tests; emulator tests require an explicit separate command."""
import os, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
paths=sorted(ROOT.glob('.toolchain/jdk-*/Contents/Home'))
jdk=Path(os.environ.get('T2CAN_JDK',str(paths[-1]) if paths else ''))
node=os.environ.get('T2CAN_NODE') or shutil.which('node')
if not (jdk/'bin/javac').is_file() or not node:
    raise SystemExit('Set T2CAN_JDK and T2CAN_NODE; tools must exist before testing.')
out=ROOT/'build/host-tests';out.mkdir(parents=True,exist_ok=True)
src=ROOT/'src/dev/t2can/unlock'
classes=['UrlPolicy','DeviceProfile','PermissionPolicy','ConnectionSession','LaunchConnectionPolicy','ConnectionRetryPolicy']
tests=sorted((ROOT/'tests').glob('*Test.java'))
subprocess.run([str(jdk/'bin/javac'),'--release','8','-Xlint:-options','-d',str(out),*[str(src/(c+'.java')) for c in classes],*map(str,tests)],check=True)
for test in tests:
    subprocess.run([str(jdk/'bin/java'),'-cp',str(out),'dev.t2can.unlock.'+test.stem],check=True)
for test in sorted((ROOT/'tests').glob('*.test.js')):
    subprocess.run([node,str(test)],check=True)
print('All host test suites PASS; physical networking remains NEEDS_DEVICE_TEST.')
