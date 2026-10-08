from pathlib import Path


root = Path(__file__).resolve().parents[1]
ino_files = list(root.glob("*.ino"))
assert len(ino_files) == 1

production_files = [
    ino_files[0],
    root / "can_core.h",
    root / "can_runtime.h",
    root / "vehicle_logic.h",
    root / "web_api.h",
    root / "dashboard_source.html",
]
production = "\n".join(path.read_text(encoding="utf-8") for path in production_files)
dashboard = (root / "dashboard_source.html").read_text(encoding="utf-8")

# Removal contract: the failed AP Aggressiveness experiment must not leave any
# runtime, API, persistence, trace, or dashboard integration behind.
assert '#define FW_VERSION "v3.26.3"' in production
assert not (root / "ap_aggressiveness_pure.h").exists()
assert not (root / "tests" / "test_ap_aggressiveness_pure.cpp").exists()

for marker in (
    "apAgg",
    "AP_AGG",
    "AP Aggressiveness",
    "AP_AGGRESSIVENESS",
    "/api/lab/ap-aggressiveness/",
    '"agEn"',
    '"agTx"',
    '"agAuto"',
    '"agHw"',
    '"agProf"',
):
    assert marker not in production, marker

# The independent, existing 0x334 AP pedal/regen feature remains supported.
assert "AP Accel / Regen" in dashboard

print("PASS v3.26.3 AP Aggressiveness removal contract")
