from pathlib import Path
root = Path(__file__).resolve().parents[1]
logic = (root/'vehicle_logic.h').read_text()
api = (root/'web_api.h').read_text()
html = (root/'dashboard_source.html').read_text()
ino = next(root.glob('*.ino')).read_text()
pure = (root/'auto_lane_change_enable_pure.h').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'UI_AUTO_LC_BUS_BOTH_PURE' in pure
assert 'uiAutoLaneChangeBusAllowedPure' in pure
assert 'uiAutoLaneChangeTargetBus' in logic
assert 'begin("alc293lab"' in logic
assert 'getUChar("bus"' in logic
assert 'putUChar("bus"' in logic
assert 'uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_A_PURE)' in logic
assert 'uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_B_PURE)' in logic
assert 'server.hasArg("bus")' in api
assert 'autoLaneChangeTargetBus' in api
assert 'id="labAutoLaneChangeBus"' in html
assert '0x293 TX Target' in html
assert 'updateAutoLaneChange293Bus' in html
print('v3.6d9a2 0x293 independent bus-target contract OK')
