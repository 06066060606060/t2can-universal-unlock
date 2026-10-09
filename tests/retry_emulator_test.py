#!/usr/bin/env python3
"""Verify two real Android request timeouts using the fake AP seeded by launch_emulator_test."""
from emulator_support import *
guard();package='dev.t2can.unlock'
adb('shell','am','force-stop',package)
adb('logcat','-c')
adb('shell','am','start','-n',package+'/.MainActivity')
def requests():
    log=adb('logcat','-d','-s','ConnectivityService')
    return [line for line in log.splitlines() if 'requestNetwork for uid/pid:' in line and 'Transports: WIFI' in line and 'RequestorPkg: '+package in line]
end=time.monotonic()+85
seen=0
while time.monotonic()<end:
    lines=requests()
    if len(lines)>seen:
        seen=len(lines);print('Actual Android Wi-Fi request '+str(seen)+': '+lines[-1][:80],flush=True)
    assert seen<=2,'No third Android request allowed'
    values=[n.get('text','') for n in nodes()]
    if any('application has cancelled' in t.lower() for t in values):
        tap('OK')
    if 'ERROR' in values:
        assert seen==2,'First unavailable must trigger one new request'
        assert any('Tap Connect to try again.' in t for t in values)
        break
    time.sleep(.5)
else: raise AssertionError('No terminal ERROR after two request timeouts')
time.sleep(3)
assert len(requests())==2
screenshot('retry-exhausted')
print('Two actual Android requests, then terminal ERROR without loop PASS',flush=True)
tap('Devices / connection settings');tap('Disconnect');wait_label('IDLE')
adb('shell','input','keyevent','KEYCODE_HOME')
adb('shell','am','start','-n',package+'/.MainActivity');wait_label('IDLE')
assert len(requests())==2
assert package not in adb('logcat','-b','crash','-d')
print('Disconnect and background resume stay idle; no app crash PASS',flush=True)
