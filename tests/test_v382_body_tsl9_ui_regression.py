from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
PURE = (ROOT / "tsl9_hands_on_0x399_pure.h").read_text(encoding="utf-8")

assert "nagMethodResolveForCapabilitiesPure" in PURE
assert "static bool nagCfgApplyActiveProfilePolicy" in CORE
assert "profileMethodMigrated = nagCfgApplyActiveProfilePolicy(nagCfg, true)" in CORE
assert 'prefs.putBool("en", nagCfg.enabled);' in CORE
assert "nagCfgApplyActiveProfilePolicy(nc, false);" in API
assert "nagCfgApplyActiveProfilePolicy(nc, true);" in API

# Backend resolves stale Torque before the dashboard config is loaded; the UI
# must lock the single valid method without disabling the master toggle.
assert "me.disabled=!(ts&&ss)" in DASH
assert "$(id).disabled=!window.__t2NagSupported" in DASH
assert "function nagTsl9RouteInfo()" in DASH
assert "Body CAN A · 0x39B" in DASH
assert "id=\"tsl9Window\"" in DASH

# Android/mobile LAB regression: unknown gear is compact and columns cannot overlap.
assert "?'N/A':gearRaw" in DASH
assert ".labGear{flex:0 0 54px" in DASH
assert "text-overflow:ellipsis" in DASH

print("PASS v3.8.2 Body+Chassis TSL9 recovery and LAB gear layout regression")
