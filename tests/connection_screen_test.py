#!/usr/bin/env python3
"""Cold start stays idle, app-owned labels stay English, no OS request on resume."""
import re,time
from emulator_support import *
guard();package='dev.t2can.unlock'
try:
    adb('shell','svc','wifi','disable')
    adb('shell','cmd','locale','set-app-localeconfig',package,'--locales','en,ko-KR')
    adb('shell','cmd','locale','set-app-locales',package,'--locales','ko-KR')
    adb('shell','am','force-stop',package);adb('shell','am','start','-n',package+'/.MainActivity')
    wait_label('Devices / connection settings')
    text=[n.get('text','') for n in nodes()]
    assert {'Connect','Manual connection','Devices / connection settings'}.issubset(text)
    assert not any(re.search(r'[\uac00-\ud7a3]',t) for t in text)
    assert not any(n.get('class')=='android.webkit.WebView' for n in nodes())
    adb('shell','input','keyevent','KEYCODE_HOME');adb('shell','am','start','-n',package+'/.MainActivity')
    wait_label('Devices / connection settings')
    assert not any(n.get('class')=='android.webkit.WebView' for n in nodes())
    print('Cold start/resume idle and English app labels PASS')
finally:
    adb('shell','cmd','locale','set-app-locales',package)
    adb('shell','cmd','locale','set-app-localeconfig',package)
    adb('shell','svc','wifi','enable')
