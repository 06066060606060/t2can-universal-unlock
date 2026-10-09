from pathlib import Path
root=Path(__file__).resolve().parents[1]
logic=(root/'vehicle_logic.h').read_text()
runtime=(root/'can_runtime.h').read_text()
api=(root/'web_api.h').read_text()
html=(root/'dashboard_source.html').read_text()
ino=next(root.glob('*.ino')).read_text()
pure=(root/'auto_lane_change_enable_pure.h').read_text() if (root/'auto_lane_change_enable_pure.h').exists() else ''
assert '#define FW_VERSION "v3.28.0"' in ino
# Confirm-Free must be AP-active, not NOA-only.
ulc=(root/'ulc_stalk_confirm_pure.h').read_text()
assert 'dasState4 == 3' in ulc and 'dasState4 == 4' in ulc and 'dasState4 == 5' in ulc and 'dasState4 == 6' in ulc
# 0x293 research toggle and pure codec.
assert 'UI_CHASSIS_CONTROL_ID' in logic
assert 'uiAutoLaneChangeEnabled' in logic
assert 'uiAutoLaneChangeObserveAndInjectCanA' in logic
assert 'uiAutoLaneChangeObserveAndInjectCanB' in logic
assert 'uiAutoLaneChangeReadRawPure' in pure
assert 'uiAutoLaneChangeFinalizePure' in pure
assert 'case UI_CHASSIS_CONTROL_ID' in runtime
assert 'uiAutoLaneChangeObserveAndInjectCanA' in runtime
assert 'uiAutoLaneChangeObserveAndInjectCanB' in runtime
# Persistent LAB setting/API/UI beneath Confirm-Free.
assert 'begin("alc293lab"' in logic
assert 'getBool("enabled", false)' in logic
assert 'putBool("enabled"' in logic
assert '/api/lab/auto-lane-change/update' in api
assert 'id="labAutoLaneChangeToggle"' in html
assert 'UI_autoLaneChangeEnable' in html
assert 'AP active' in html
# UI must expose stock A/B observations and TX telemetry.
for token in ['labAutoLcStockA','labAutoLcStockB','labAutoLcGate','labAutoLcTxA','labAutoLcTxB']:
    assert token in html, token
print('v3.6b10 AP-gated 0x293 AutoLC contract OK')
