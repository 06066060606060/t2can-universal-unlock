from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')
assert '#define FW_VERSION "v3.28.0"' in ino
assert '#include "driver_monitor_capture_pure.h"' in ino
assert '#include "driver_monitor_capture.h"' in ino

pure = (root / 'driver_monitor_capture_pure.h').read_text(encoding='utf-8')
cap = (root / 'driver_monitor_capture.h').read_text(encoding='utf-8')
runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

# Universal fixed-window, read-only driver-monitor recorder.
for token in ['DRIVER_MONITOR_PRE_MS = 2000', 'DRIVER_MONITOR_POST_MS = 5000',
              'DRIVER_MONITOR_ID_STATUS2 = 0x389', 'DRIVER_MONITOR_ID_CARLOG = 0x5D9',
              'DRIVER_MONITOR_ID_AUTOPILOT_DEBUG = 0x247']:
    assert token in cap, token
assert 'activeProfileIsYl()' not in cap
assert 'canTx' not in cap and 'twai_transmit' not in cap and 'Can_A.sendMessage' not in cap

# Bus routing: both physical CAN A/B feed the universal observer.
assert 'driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_A' in runtime
assert 'driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_B' in runtime
assert 'driverMonitorCaptureTick' in runtime

# A-F fixed experiment labels.
for label in ['FRONT · NORMAL', 'SCREEN GLANCE', 'SIDE LOOK', 'LOOK DOWN',
              'TORQUE + LOOK AWAY', 'FRONT · NO TORQUE']:
    assert label in html, label
for slot in 'ABCDEF':
    assert f'id="driverMonCap{slot}"' in html

# LAB endpoints and CSV export.
for route in ['/api/drivermonitor/stats', '/api/drivermonitor/start',
              '/api/drivermonitor/reset', '/api/drivermonitor/log.csv']:
    assert route in api, route
dm_api = api[api.index('// ─── Universal Driver Monitoring Capture'):api.index('static void httpSystemStats()')]
assert 'activeProfileIsYl()' not in dm_api
assert 'Driver_Monitoring_Capture.csv' in html
assert 'Driver Monitoring Capture · YL ONLY' not in html
assert 'ALL VEHICLES' in html
assert 'READ ONLY' in html

# 0x389 interaction decoding is explicit and CSV carries raw + decoded state.
for token in ['driverMonitorInteractionLevelPure', 'DRIVER_INTERACTING',
              'DRIVER_NOT_INTERACTING', 'CONTINUED_DRIVER_NOT_INTERACTING']:
    assert token in pure + cap

print('Driver Monitoring Capture universal regression contract OK')
