from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'s3xy_ble.h').read_text()
v=(R/'vehicle_logic.h').read_text()
f=(R/'t2can_forward.h').read_text()
d=(R/'dashboard_source.html').read_text()
ino=next(R.glob('*.ino')).read_text()
assert '#define FW_VERSION "v3.28.0"' in ino
# Existing persisted IDs remain stable; new actions append only.
assert 'S3XY_ACTION_PERFORMANCE_MODE = 10' in s
assert 'S3XY_ACTION_LEFT_BLINKER = 11' in s
assert 'S3XY_ACTION_RIGHT_BLINKER = 12' in s
assert 'return "left_blinker"' in s and 'return "right_blinker"' in s
assert 'Left Blinker' in s and 'Right Blinker' in s
assert 'requestTurnSignalPulseFromButton(1)' in s
assert 'requestTurnSignalPulseFromButton(2)' in s
assert 'static bool requestTurnSignalPulseFromButton(uint8_t dir);' in f
# Planner-driven Auto Blinker keeps its legacy pulse lifetime. Direct S3XY
# stalk actions are covered separately and must not reuse this timed burst.
assert 'requestBlinkerTx(fireRequestDir, BLINKER_TX_SOURCE_AUTO_PURE' in v
assert 'oneShotReleaseAt = now + BLINKA_STALKLESS_PRESS_MS;' in v
assert 'activeTurnSignalVariant == TURN_SIGNAL_UNSET' in v
assert "['left_blinker','Left Blinker']" in d
assert "['right_blinker','Right Blinker']" in d
assert '.settingToggleRow:has(:disabled)' in d
assert 'input:disabled,select:disabled' in d
assert 'button:disabled' in d
assert '-webkit-text-fill-color:currentColor' in d
print('v3.6f1 S3XY blinker + disabled UI static contract: PASS')
