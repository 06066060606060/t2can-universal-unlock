from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / "dashboard_source.html").read_text()
api = (ROOT / "web_api.h").read_text()
for token in ["panelLabLaneGraph", "panelLabParkedInjection", "panelLabCanARx", "panelLabUlcMonitor", "fetchLaneGraphLab", "fetchParkedInjectionLab", "updateCanARxLab", "fetchUlcMonitorLab"]:
    assert token not in html, f"removed LAB feature remains: {token}"
for route in ["/api/lab/lane-graph/", "/api/lab/parked-injection/", "/api/lab/can-a-rx/", "/api/lab/ulc-monitor/"]:
    assert route not in api, f"removed LAB API remains: {route}"
assert 'id="labUlcBlind"' in html and 'server.hasArg("blind")' in api
assert 'id="panelLabVisionControl"' not in html
assert 'id="panelVisionControl"' in html
assert 'data-panel="panelVisionControl"' in html
print("PASS removed LAB surfaces and preserved blind-spot injection / production Visual Speed Control")
