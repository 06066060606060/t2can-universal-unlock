from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob("*.ino")).read_text()
vl = (ROOT / "vehicle_logic.h").read_text()
web = (ROOT / "web_api.h").read_text()
dash = (ROOT / "dashboard_source.html").read_text()
core = (ROOT / "can_core.h").read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert '#include "r79_dms_composition_pure.h"' in ino
assert 'bool     dmsControlEnabled;' not in core
active = vl[vl.index('static bool r79DmsControlActive()'):vl.index('static inline bool r79DmsApplyFinal')]
assert 'driverMonitoringControlSnapshot()' in active
assert 'nagCfg.' not in active
assert 'labMenuEnabled' not in active
assert 'r79DmsApplyFinal(out.data)' in vl
assert 'getBool("dms43", migrated)' in vl
assert 'putBool("dmsDisable", enabled)' in vl
assert 'hasArg("dmsControlEnabled")' not in web
assert '/api/driver-monitoring/config' in web
assert 'server.on("/api/lab/dms-nag' not in web
assert 'id="nagDmsToggle"' not in dash
assert 'id="driverMonitoringToggle"' in dash
assert 'id="labDmsNagToggle"' not in dash
assert 'Request cabin-camera monitoring off during Autopilot.' in dash
assert 'id="r79DmsBit43Value"' in dash
print('PASS standalone production DMS/R79 composition contract')
