from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
vp = (ROOT / "vehicle_profile.h").read_text(encoding="utf-8")
vl = (ROOT / "vehicle_logic.h").read_text(encoding="utf-8")
web = (ROOT / "web_api.h").read_text(encoding="utf-8")
dash = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

assert "vehicleProfileDmsNagSupported" in vp
assert "return vehicleProfileEuUnlockSupported(id, topology);" in vp
assert "activeProfileDmsNagSupported" in vp
active = vl[vl.index('static bool r79DmsControlActive()'):vl.index('static inline bool r79DmsApplyFinal')]
assert "activeProfileDmsNagSupported()" in active
assert "driverMonitoringControlSnapshot()" in active
assert "nagCfg." not in active
assert "labMenuEnabled" not in active
assert 'jw.boolean("supported", activeProfileDmsNagSupported());' in web
assert 'jw.boolean("dmsNagSupported", activeProfileDmsNagSupported());' in web
assert "YL ONLY" not in dash
assert "Request cabin-camera monitoring off during Autopilot." in dash
print("PASS standalone DMS profile capability contract")
