from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
cap = (root / 'driver_monitor_capture.h').read_text(encoding='utf-8')
pure = (root / 'driver_monitor_capture_pure.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

assert '#define FW_VERSION "v3.26.3"' in ino
assert 'driverMonitorCaptureInit();' in ino
assert 'if (activeProfileIsYl()) driverMonitorCaptureInit();' not in ino

# Driver Monitoring is universal/read-only: no profile gate in capture core or API block.
assert 'activeProfileIsYl()' not in cap
api_dm = api[api.index('// ─── Universal Driver Monitoring Capture'):api.index('static void httpSystemStats()')]
assert 'activeProfileIsYl()' not in api_dm
assert 'available only on Model Y L' not in api_dm

# UI must not advertise YL-only support.
assert 'Driver Monitoring Capture · YL ONLY' not in html
assert 'YL ONLY · cabin/DMS CAN research' not in html
assert 'Driver Monitoring Capture' in html
assert 'ALL VEHICLES' in html
assert 'READ ONLY' in html

# CSV carries physical CAN side plus current profile bus-role name.
assert 'physical_bus,bus_role' in api
assert 'driverMonitorPhysicalBusName' in cap
assert 'driverMonitorProfileBusName' in cap

# Both physical buses must accept all research IDs.
for token in ['BUS_A', 'BUS_B', '0x389', '0x5D9', '0x247', '0x399', '0x370']:
    assert token in pure

print('v3.6d9a2 Driver Monitoring all-profile contract OK')
