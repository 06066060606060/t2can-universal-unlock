from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
core = (ROOT/'can_core.h').read_text()
logic = (ROOT/'vehicle_logic.h').read_text()
runtime = (ROOT/'can_runtime.h').read_text()
web = (ROOT/'web_api.h').read_text()
dash = (ROOT/'dashboard_source.html').read_text()
dm = (ROOT/'driver_monitor_capture.h').read_text()
pure = (ROOT/'driver_monitor_capture_pure.h').read_text()

# Mode H engine-specific behavior moved to the current b19 contract test.

# 0x3F8 now has one CAN-B compositor and a fixed Confirm-Free route.
assert 'ulcCompose3f8Pure' in logic
assert 'lab3f8ObserveCanA' not in logic
assert 'ulcNoConfirmTargetBus' not in logic
assert 'ulcNoConfirmInjectCanA' not in logic
assert 'injectDriverAssistControl' in runtime
for token in ['driverAssistRxB', 'driverAssistRawB', 'stalkConfirmRoute']:
    assert token in web, token
assert 'driverAssistRxA' not in web
assert 'stalkConfirmTargetBus' not in web

# Driver Monitoring now includes DAS_status and EPAS_sysStatus on YL Party CAN.
assert '0x399' in pure and '0x370' in pure
assert 'DRIVER_MONITOR_ID_DAS_STATUS' in dm
assert 'DRIVER_MONITOR_ID_EPAS_STATUS' in dm
assert 'das_hands_on_raw' in web
assert 'epas_hands_on_raw' in web
assert 'epas_torque_nm' in web

print('fixed CAN-B ULC compositor and driver monitor contract passed')
