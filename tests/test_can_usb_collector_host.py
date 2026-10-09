"""Firmware-emitted records traverse the real macOS parser and CSV writer."""
from pathlib import Path
import importlib.util
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
collector = ROOT.parents[2] / 'tools/can_usb_logger/collect.py'
spec = importlib.util.spec_from_file_location('can_usb_collector', collector)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory(prefix='can-usb-e2e-') as tmp:
    work = Path(tmp)
    code = work / 'writer.cpp'
    code.write_text(r'''
#include "can_usb_logger_pure.h"
#include <cstdio>
int main() {
  std::puts("@HELLO,1,v3.28.0");
  std::puts("@START,1,PASSIVE");
  std::puts("@CTRL,1,1,0,0,0,1");
  CanUsbQueue<4> queue; queue.start();
  CanUsbFrame frame = {};
  frame.timestampUs = 4294967300ULL; frame.id = 0x399; frame.dlc = 2;
  frame.data[0] = 0xAB; frame.data[1] = 0xCD;
  queue.observe(frame);
  frame.timestampUs++; frame.bus = 1; frame.flags = 3; frame.id = 0x1FFFFFFF; frame.dlc = 8;
  queue.observe(frame);
  char line[128];
  while (queue.pop(frame)) { if (!canUsbFormatFrame(line, sizeof(line), frame)) return 1; std::fputs(line, stdout); }
  std::puts("@CTRL,1,1,0,0,0,1");
  std::puts("@STOP,1,2,2,0,0");
}
''')
    executable = work / 'writer'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                    '-Werror', '-I', str(ROOT), str(code), '-o', str(executable)], check=True)
    records = subprocess.check_output([str(executable)])
    capture = module.Capture(work / 'capture', 'PASSIVE', 'X179_BODY', 'X177_CHASSIS')
    for start in range(0, len(records), 7):
        capture.feed(records[start:start + 7])
    summary = capture.close('stopped')
    assert summary['complete'], summary
    assert summary['received_frames'] == 2
    lines = (work / 'capture/frames.csv').read_text()
    assert '4294967300,A,X179_BODY,0,399,2,ABCD' in lines, lines
    assert '4294967301,B,X177_CHASSIS,3,1FFFFFFF,8,' in lines, lines
print('PASS firmware formatter -> chunked USB parser -> CSV + complete summary')
