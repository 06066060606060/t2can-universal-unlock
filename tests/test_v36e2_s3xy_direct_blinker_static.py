from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'s3xy_ble.h').read_text()
v=(R/'vehicle_logic.h').read_text()
f=(R/'t2can_forward.h').read_text()
d=(R/'dashboard_source.html').read_text()
assert 'oneShotDirect' in v
assert 'static bool requestTurnSignalPulseFromButton(uint8_t dir)' in v
fn=v.split('static bool requestTurnSignalPulseFromButton(uint8_t dir) {',1)[1].split('\n}',1)[0]
assert 'activeProfileAdvancedEapSupported()' not in fn
assert 'ulcNoConfirmEnabledSnapshot()' not in fn
assert 'activeTurnSignalVariant == TURN_SIGNAL_UNSET' in fn
assert 'requestBlinkerTx(dir, BLINKER_TX_SOURCE_S3XY_PURE' in fn
assert 'sendStalkFrameCanB(uint8_t turn, uint32_t txEpoch, bool directUser)' in v
assert 'sendStalkFrameCanA(uint8_t turn, uint32_t txEpoch, bool directUser)' in v
assert 'if (!directUser && ulcNoConfirmEnabledSnapshot()) return;' in v
assert 'if (!oneShotDirect)' in v
support=s.split('static bool s3xyActionSupportedForCurrentProfile(uint8_t action) {',1)[1].split('\n}',1)[0]
assert 'activeTurnSignalVariant != TURN_SIGNAL_UNSET' in support
assert 'activeProfileAdvancedEapSupported() && activeTurnSignalVariant' not in support
assert "v!=='auto_blinker_toggle'||!!window.__t2AdvancedEapSupported" in d
assert "['left_blinker','right_blinker'].includes(v)||!!window.__t2TurnSignalSupported" in d
assert 'window.__t2TurnSignalSupported=Number(s.turn)!==0' in d
print('PASS v3.6f1 direct S3XY blinker policy split')
