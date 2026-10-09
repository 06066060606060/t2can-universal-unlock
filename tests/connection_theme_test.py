#!/usr/bin/env python3
"""Connection UI at 320/390/430dp in both themes, with accessible visible controls."""
import struct,time,re
from emulator_support import *
guard();package='dev.t2can.unlock'
try:
    adb('shell','am','force-stop',package);adb('shell','am','start','-n',package+'/.MainActivity')
    wait_label('Devices / connection settings')
    for width in (320,390,430):
        adb('shell','wm','density','160');adb('shell','wm','size',str(width)+'x800')
        for dark in (False,True):
            adb('shell','cmd','uimode','night','yes' if dark else 'no');time.sleep(.6)
            snapshot=nodes()
            for label in ('Connect','Manual connection','Devices / connection settings'):
                item=next(n for n in snapshot if n.get('text')==label)
                x1,y1,x2,y2=map(int,re.findall(r'\d+',item.get('bounds')))
                assert 0<=x1<x2<=width and 0<=y1<y2<=800,(label,item.get('bounds'))
            data=adb('exec-out','screencap',binary=True)
            w,h,fmt,space=struct.unpack('<IIII',data[:16]);assert w==width and fmt==1
            color=data[16+4*(70*w+5):16+4*(70*w+5)+3]
            assert (sum(color)/3<100)==dark,(width,dark,list(color))
            screenshot('connection-'+str(width)+('-dark' if dark else '-light'))
    print('320/390/430dp light/dark connection controls + background PASS (6 configurations)')
finally:
    adb('shell','wm','size','reset');adb('shell','wm','density','reset');adb('shell','cmd','uimode','night','no')
