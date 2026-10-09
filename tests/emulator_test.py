#!/usr/bin/env python3
"""End-to-end QA against the local mock only; requires a disposable emulator."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from qa_command import command

from emulator_support import adb, nodes, tap, wait_label, screenshot, guard

guard()
def tap_label(*labels):
    for label in labels:
        try: tap(label);return
        except AssertionError: pass
    raise AssertionError("Missing labels " + repr(labels))
def tap_web(selector):
    rect=command("(()=>{let r=document.querySelector("+json.dumps(selector)+").getBoundingClientRect();return [r.x+r.width/2,r.y+r.height/2,devicePixelRatio]})()")
    view=next(n for n in nodes() if n.get('class')=='android.webkit.WebView')
    left,top,_,_=map(int,re.findall(r"\d+",view.get('bounds')))
    adb('shell','input','tap',str(round(left+rect[0]*rect[2])),str(round(top+rect[1]*rect[2])))

assert command("typeof window.__tuNativeBack") == "function"
screenshot("home")
command("showPage('settings');openPanel('panelR79');true")
adb("shell", "input", "keyevent", "KEYCODE_BACK")
time.sleep(.5)
assert command("document.querySelector('.panel.show')===null")
assert command("document.querySelector('.page.active').dataset.page") == "settings"
adb("shell", "input", "keyevent", "KEYCODE_BACK")
time.sleep(.5)
assert command("document.querySelector('.page.active').dataset.page") == "home"
print("Android back: panel -> Settings -> Home PASS", flush=True)

# Real Android system save picker, then exact comparison with the original Blob.
save_name = "T2CAN_QA_" + str(int(time.time())) + ".csv"
command("saveDownloadedBlob(new Blob(['time,value\\n'+'123,hello\\n'.repeat(18000)],{type:'text/csv'})," + json.dumps(save_name) + ");'started'")
tap_label("SAVE", "Save")
time.sleep(1)
if any(node.get("text") == "Replace" for node in nodes()): tap_label("Replace")
time.sleep(1)
saved = adb("exec-out", "cat", "/sdcard/Download/" + save_name, binary=True)
expected = ("time,value\n" + "123,hello\n" * 18000).encode()
assert saved == expected
print("Native blob export: " + str(len(saved)) + " bytes, SHA256 " + hashlib.sha256(saved).hexdigest() + " PASS", flush=True)

# File chooser uses a harmless test .bin; no vehicle firmware is flashed.
fixture_name = "T2CAN_QA_" + str(int(time.time())) + ".bin"
fixture_text = "T2CAN QA - NOT A FLASHABLE FIRMWARE\n"
# Saving through SAF registers the file with Android's Downloads provider;
# adb push alone does not make a raw .bin appear in its indexed file list.
command("saveDownloadedBlob(new Blob([" + json.dumps(fixture_text) + "])," + json.dumps(fixture_name) + ");'started'")
tap_label("SAVE", "Save")
time.sleep(1)
command("showPage('settings');openPanel('panelOta');true")
time.sleep(.5)
tap_web("#otaChooseFile")
tap_label("Show roots")
tap_label("Downloads")
tap_label(fixture_name)
time.sleep(.5)
assert command("document.getElementById('otaFile').files[0]?.name") == fixture_name
assert command("document.getElementById('otaFile').files[0]?.size") == len(fixture_text.encode())
assert command("document.getElementById('otaFile').files[0].text()") == fixture_text
upload = command("new Promise((resolve,reject)=>{const x=new XMLHttpRequest(); const f=new FormData(); f.append('update',document.getElementById('otaFile').files[0]); x.open('POST','/update'); x.onload=()=>resolve({status:x.status,response:x.responseText}); x.onerror=()=>reject(new Error('upload failed'));x.send(f)})")
assert upload['status']==200 and json.loads(upload['response'])['ok'] is True
print("Harmless OTA file POST through WebView to mock /update PASS",flush=True)
screenshot("ota-file-selected")
print("Native OTA chooser: content URI readable by WebView PASS", flush=True)

command("closePanels();showPage('home');true")
# User-initiated connection changes cannot interrupt an OTA transfer.
command("otaUploading=true;true")
tap('Connection')
assert not any(n.get('text')=='Disconnect' for n in nodes())
command("otaUploading=false;true")
print("Connection menu refuses OTA-busy route changes PASS",flush=True)
adb("shell", "svc", "wifi", "disable")
wait_label("Devices / connection settings")
screenshot("network-lost")
assert not any(n.get('class')=='android.webkit.WebView' for n in nodes())
adb("shell", "svc", "wifi", "enable")
time.sleep(2)
assert not any(n.get('class')=='android.webkit.WebView' for n in nodes()), "No automatic reconnect after loss"
tap('Manual connection');tap('Check connection')
for _ in range(15):
    try:
        if command("typeof window.__tuNativeBack",timeout=2)=='function':break
    except Exception: time.sleep(.3)
else: raise AssertionError("Explicit reconnect failed")
print("Wi-Fi loss tears down WebView; explicit reconnect PASS",flush=True)
adb("shell", "cmd", "uimode", "night", "yes")
time.sleep(1)
assert command("document.documentElement.dataset.theme") == "dark"
screenshot("home-dark")
adb("shell", "cmd", "uimode", "night", "no")
time.sleep(1)
assert command("document.documentElement.dataset.theme") == "light"
print("System theme change without reloading page PASS", flush=True)
crashes = adb("logcat", "-b", "crash", "-d")
assert "dev.t2can.unlock" not in crashes, crashes
print("Android 16 end-to-end QA PASS", flush=True)
