"""Helpers restricted to a disposable emulator; never use a physical phone."""
import os, re, subprocess, time, xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ADB=Path(os.environ.get('T2CAN_ADB',ROOT/'.toolchain/platform-tools/adb'))
SERIAL=os.environ.get('T2CAN_QA_SERIAL','emulator-5570')
PORT=os.environ.get('T2CAN_QA_ADB_PORT','5041')
if not re.fullmatch(r'emulator-\d+',SERIAL): raise RuntimeError('Emulator serial required')
def adb(*args,binary=False):
    result=subprocess.run([str(ADB),'-P',PORT,'-s',SERIAL,*map(str,args)],check=True,capture_output=True)
    return result.stdout if binary else result.stdout.decode()
def guard():
    # Cold emulator startup is asynchronous; wait only for this explicit emulator serial.
    until=time.monotonic()+30
    while True:
        try:
            assert adb('shell','getprop','ro.kernel.qemu').strip()=='1','Physical device prohibited'
            if adb('shell','getprop','sys.boot_completed').strip()=='1': return
        except subprocess.CalledProcessError:
            pass
        if time.monotonic()>=until: raise RuntimeError('Disposable emulator not ready within 30 seconds')
        time.sleep(.5)
def nodes():
    adb('shell','uiautomator','dump','/sdcard/t2can-ui.xml')
    return list(ET.fromstring(adb('shell','cat','/sdcard/t2can-ui.xml')).iter('node'))
def tap(label):
    for n in nodes():
        if n.get('text','').casefold()==label.casefold() or n.get('content-desc','').casefold()==label.casefold():
            x1,y1,x2,y2=map(int,re.findall(r'\d+',n.get('bounds')))
            adb('shell','input','tap',(x1+x2)//2,(y1+y2)//2);return
    raise AssertionError('Missing UI label '+label)
def wait_label(label,timeout=12):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        if any(n.get('text')==label for n in nodes()):return
        time.sleep(.3)
    raise AssertionError('Missing '+label)
def screenshot(name):
    out=ROOT/'artifacts/qa';out.mkdir(parents=True,exist_ok=True)
    path=out/(name+'.png');path.write_bytes(adb('exec-out','screencap','-p',binary=True));return path
