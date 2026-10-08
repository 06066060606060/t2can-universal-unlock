from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = root / 'can_usb_logger_pure.h'
assert header.exists(), 'USB full-frame logger and bounded queue are not implemented'
ino = next(root.glob('*.ino')).read_text()
runtime = (root / 'can_runtime.h').read_text()
assert ino.index('#include "can_usb_logger.h"') < ino.index('#include "can_core.h"')
assert 'Serial.begin(115200);' in ino and 'canUsbStartTask();' in ino
assert runtime.index('canUsbObserve(0,') < runtime.index('if ((rxf.can_id & 0xC0000000UL)')
twai = runtime.split('static void canTaskTwai(', 1)[1]
assert twai.index('canUsbObserve(1,') < twai.index('r79ProcessStockFrame(')
assert 'if (canUsbPassive())' in runtime.split('static void canSupervisorTask(', 1)[1]
print('PASS USB raw frame hooks and survey single-bus integration')
