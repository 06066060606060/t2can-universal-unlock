from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text()
logic = (root / 'vehicle_logic.h').read_text()
runtime = (root / 'can_runtime.h').read_text()
api = (root / 'web_api.h').read_text()
dash = (root / 'dashboard_source.html').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert '#include "tsl9_input_scheduler_pure.h"' in ino
assert '#include "ap_right_scroll_pure.h"' in ino
assert 'tsl9InputObserveCanA(rxf)' in runtime
assert 'tsl9InputObserveCanB(f)' in runtime
assert 'tsl9InputServiceCanA();' in runtime
assert 'tsl9InputServiceCanB();' in runtime
assert 'TSL9_INPUT_REPEAT_MIN_MS_PURE' in (root / 'tsl9_input_scheduler_pure.h').read_text()
assert 'nagEnabled' in logic and 'NAG_METHOD_TSL9_PURE' in logic
assert 'nagCtx.scrollWarningActive' in logic
assert 'nagCtx.visualWarningActive' in logic
assert 'in.periodicIntervalSeconds' in logic
assert '/api/ap-right-scroll' not in api
assert 'panelApRightScroll' not in dash
assert 'id="tsl9InputMode"' in dash
assert 'id="torqueRightScrollWrap"' in dash
assert 'id="tsl9RightPeriodicWrap"' in dash

print('PASS v3.28.0 Torque and TSL9 periodic right-scroll integration contract')
