#!/usr/bin/env python3
"""Exercise actual Keystore persistence and the Android nearby-device permission UI."""
import time,re
from emulator_support import *
guard();package='dev.t2can.unlock'
adb('shell','pm','clear',package) # Only this disposable emulator's test fixtures.
adb('shell','am','force-stop',package);adb('shell','am','start','-n',package+'/.MainActivity')
wait_label('Devices / connection settings');tap('Connect')
wait_label('Edit device')
def field(index,value):
    fields=[n for n in nodes() if n.get('class')=='android.widget.EditText']
    n=fields[index];x1,y1,x2,y2=map(int,re.findall(r'\d+',n.get('bounds')))
    adb('shell','input','tap',(x1+x2)//2,(y1+y2)//2)
    adb('shell','input','keyevent','KEYCODE_MOVE_END')
    adb('shell','input','keyevent',*(['KEYCODE_DEL']*80))
    adb('shell','input','text',value)
field(0,'T2CAN-QA');field(1,'T2CAN-QA-NOT-AN-AP')
adb('shell','input','keyevent','KEYCODE_BACK')
spinner=next(n for n in nodes() if n.get('class')=='android.widget.Spinner')
x1,y1,x2,y2=map(int,re.findall(r'\d+',spinner.get('bounds')))
adb('shell','input','tap',(x1+x2)//2,(y1+y2)//2);tap('WPA2')
field(2,'qa-pass-1234');adb('shell','input','keyevent','KEYCODE_BACK');tap('Save')
wait_label('T2CAN-QA')
stored=adb('shell','cat','/data/user/0/'+package+'/shared_prefs/device_profiles.xml')
assert 'encrypted_v1' in stored
assert 'qa-pass-1234' not in stored and 'T2CAN-QA-NOT-AN-AP' not in stored and 'T2CAN-QA' not in stored
adb('shell','am','force-stop',package);adb('shell','am','start','-n',package+'/.MainActivity')
wait_label('T2CAN-QA')
print('Android Keystore encrypted profile save/restart/selection PASS',flush=True)
tap('Connect');time.sleep(.6)
current=nodes();assert any('Allow Tesla Unlock' in n.get('text','') for n in current)
screenshot('nearby-permission')
# Refuse the runtime permission; there must be no request loop on resume.
for label in ('Don’t allow',"Don't allow",'DENY'):
    try:tap(label);break
    except AssertionError:pass
else:adb('shell','input','keyevent','KEYCODE_BACK')
wait_label('Permission required')
adb('shell','input','keyevent','KEYCODE_HOME');adb('shell','am','start','-n',package+'/.MainActivity')
time.sleep(.6)
assert not any('Allow Tesla Unlock' in n.get('text','') for n in nodes())
print('Nearby permission denied/resume without repeat popup PASS',flush=True)
# Explicit retry grants permission, then cancelling OS Wi-Fi selection must be recoverable.
tap('Connect');time.sleep(.5);tap('Allow');time.sleep(1)
screenshot('wifi-system-request')
adb('shell','input','keyevent','KEYCODE_BACK');time.sleep(.8)
# Android's Wi-Fi picker can keep a searching dialog until the request timeout,
# followed by its own acknowledgement. Do not confuse that OS screen with a leak.
until=time.monotonic()+45
while time.monotonic()<until:
    current=nodes()
    if any('application has cancelled' in n.get('text','') for n in current):
        tap('OK');continue
    if any(n.get('text')=='Devices / connection settings' for n in current):
        tap('Devices / connection settings');tap('Disconnect');break
    time.sleep(.4)
wait_label('Devices / connection settings')
assert not any(n.get('class')=='android.webkit.WebView' for n in nodes())
print('Explicit retry and system request cancellation/timeout cleanup PASS',flush=True)
