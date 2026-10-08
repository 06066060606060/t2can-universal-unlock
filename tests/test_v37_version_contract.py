from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


assert '#define FW_VERSION "v3.26.3"' in INO
assert "nagCfgVersion < 18u" in CORE
assert 'prefs.putUChar("v", 20u)' in CORE
assert "nagHumanV4MigrateV17DefaultPure(rev4Cfg)" in CORE
assert "nagModeHDefaultStopBehaviorPure()" in CORE
assert "Number(v.ho2ThresholdNm??2).toFixed(2)" in DASHBOARD
assert "Number(v.stopBehavior??0)" in DASHBOARD

print("PASS v3.7.3 version and Mode H default contract")
