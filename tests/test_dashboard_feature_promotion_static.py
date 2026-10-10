from pathlib import Path


root = Path(__file__).resolve().parents[1]
dashboard = (root / "dashboard_source.html").read_text()

for required in [
    "panelUlc", "panelPedalMap", "tsl9InputMode",
    "/api/ulc/stats", "/api/nag/h-profile/stats",
    "blinkTxMode", "350 ms BURST", "/api/blinkA/tx-mode",
    "/api/lab/auto-lane-change/stats",
    "R79 bit18", "NOA Start Wait", "Cancel Wait",
    "ISA Suppression", "isaSuppressionToggle", "/api/isa-suppression/config",
    "driverMonitoringToggle", "/api/driver-monitoring/config",
    "Visual Speed Control", "/api/vision-control/stats", "/api/r79/update?bit18Mode=",
]:
    assert required in dashboard, f"missing promoted dashboard contract: {required}"

for removed in [
    "panelLabLaneGraph", "panelLabParkedInjection", "panelLabCanARx", "panelLabUlcMonitor",
    "/api/lab/ulc-monitor/",
    "panelLabAcc", "labUlcSpeed", "labUlcNoConfirmBus",
    "panelLabTlsscGreen", "/api/lab3f8/", "/api/nag-human-lab/",
    "panelLabBlinkerTx", "/api/lab/blinker-tx/",
    "tsl9ScrollAssistWrap",
    "panelApRightScroll", "/api/ap-right-scroll",
    "ACC Follow Distance", "TLSSC Green-Light Experiment",
    "nagDmsToggle", "panelLabVisionControl",
    "tsl9IsaToggle", "tsl9IsaChimeSuppress",
]:
    assert removed not in dashboard, f"obsolete dashboard feature remains: {removed}"

assert "bit 56" in dashboard and "bit 15" in dashboard
assert "BUS OFF" in dashboard and "Single TX" in dashboard
print("dashboard feature promotion contract: PASS")
