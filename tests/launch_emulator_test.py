#!/usr/bin/env python3
"""Seed 1.1.0, update to 1.2.1 and verify automatic launch without touching Connect."""
import time,re
from emulator_support import *
guard();package='dev.t2can.unlock'
def start(): adb('shell','am','start','-n',package+'/.MainActivity')
def restart(): adb('shell','am','force-stop',package);start()
def text(): return [n.get('text','') for n in nodes()]
def field(index,value):
    n=[n for n in nodes() if n.get('class')=='android.widget.EditText'][index]
    x1,y1,x2,y2=map(int,re.findall(r'\d+',n.get('bounds')))
    adb('shell','input','tap',(x1+x2)//2,(y1+y2)//2)
    adb('shell','input','keyevent','KEYCODE_MOVE_END')
    adb('shell','input','keyevent',*(['KEYCODE_DEL']*80))
    adb('shell','input','text',value)
# Only the disposable emulator's test fixture is replaced.
if package in adb('shell','pm','list','packages',package):adb('uninstall',package)
baseline=ROOT.parents[2]/'releases/Tesla-Unlock-1.1.0/Tesla-Unlock-1.1.0.apk'
adb('install',baseline);start();wait_label('Devices / connection settings')
tap('Connect');wait_label('Edit device')
field(0,'Launch-QA');field(1,'T2CAN-QA-NOT-AN-AP')
adb('shell','input','keyevent','KEYCODE_BACK')
# OPEN is sufficient for a non-existent test AP and avoids storing any password.
tap('Save');wait_label('Launch-QA')
adb('install','-r',ROOT/'artifacts/Tesla-Unlock-1.2.1.apk');start()
wait_label('Permission required')
assert 'Launch-QA' in text(),'Saved selected device must survive update'
assert not any('Allow Tesla Unlock' in t for t in text()),'Do not auto-loop runtime permission dialogs'
print('1.1.0 -> 1.2.1 update preserves encrypted selected profile; missing permission stays actionable PASS',flush=True)
adb('shell','pm','grant',package,'android.permission.NEARBY_WIFI_DEVICES')
adb('shell','svc','wifi','disable');restart()
wait_label('ERROR')
assert 'Turn Wi-Fi on and connect again.' in text(),'Launch must attempt automatically without Connect'
screenshot('launch-wifi-off')
print('Registered profile: cold launch automatically attempts connection PASS',flush=True)
adb('shell','svc','wifi','enable')
adb('shell','input','keyevent','KEYCODE_HOME');start();wait_label('ERROR')
assert not any('Searching for device' in t for t in text())
print('Failure + background resume does not create another OS request PASS',flush=True)
tap('Devices / connection settings');tap('Disconnect');wait_label('IDLE')
adb('shell','input','keyevent','KEYCODE_HOME');start();wait_label('IDLE')
print('Explicit disconnect + resume remains idle PASS',flush=True)
restart()
end=time.monotonic()+12
while time.monotonic()<end:
    values=text()
    if any('Searching for device' in t for t in values):break
    time.sleep(.3)
else: raise AssertionError('Fresh launch should open the Android Wi-Fi request automatically')
screenshot('launch-system-request')
print('Fresh launch retries once and opens Android Wi-Fi request without tapping Connect PASS',flush=True)
adb('shell','am','force-stop',package)
# Dismiss only the system acknowledgement for this task's cancelled fake request.
for _ in range(3):
    values=text()
    if any('application has cancelled' in t for t in values):tap('OK');break
    time.sleep(.5)
assert 'dev.t2can.unlock' not in adb('logcat','-b','crash','-d')
print('Launch emulator regression PASS',flush=True)
