from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
vl = (ROOT / "vehicle_logic.h").read_text()
web = (ROOT / "web_api.h").read_text()
dash = (ROOT / "dashboard_source.html").read_text()
ino = next(ROOT.glob("*.ino")).read_text()

assert '#include "ulc_policy_pure.h"' in ino
assert 'lab3f8UlcOffHighwayMode' in vl
assert 'uiUlcOffHighway' in vl
assert 'getBit(data, 15)' in vl
assert 'ulcPolicyApGateOpenPure' in vl
assert 'begin("ulc"' in vl
assert 'getUChar("ulcOff"' in vl
assert 'putUChar("ulcOff"' in vl
assert 'server.hasArg("ulcOff")' in web
assert 'ulcOffHighwayMode' in web
assert 'lab3f8UlcSpeedMode' not in vl
assert 'readBitsLE(data, 50, 2)' not in vl
assert 'server.hasArg("ulcspeed")' not in web
print('production ULC off-highway policy static tests passed')
