from pathlib import Path

root = Path(__file__).resolve().parents[1]
dashboard = (root / "dashboard_source.html").read_text()
profile = (root / "vehicle_profile.h").read_text()
preview = (root / "tools" / "v38_preview_mock.js").read_text()
api = (root / "web_api.h").read_text()
core = (root / "can_core.h").read_text()

support = profile.split("static inline bool vehicleProfileNagTsl9Supported", 1)[1].split("\n}", 1)[0]
assert "vehicleProfileCanAIsBody(id, topology)" in support
assert "vehicleProfileCanBIsChassis(id, topology)" not in support
assert "me.options[1].hidden=!ss" in dashboard
assert "$('tsl9SequenceWrap').classList.toggle('tuUiHidden',!tsl9||!ss)" in dashboard
assert "nagTsl9Supported=topology!==3" in preview
assert "requested == NAG_METHOD_TSL9_PURE && !activeProfileNagTsl9Supported()" in api
assert "nagCfgApplyActiveProfilePolicy(nagCfg, true)" in core

print("PASS Party + Chassis TSL9 option hidden")
