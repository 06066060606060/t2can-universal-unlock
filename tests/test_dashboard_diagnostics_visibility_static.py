from pathlib import Path
import re


root = Path(__file__).resolve().parents[1]
dashboard = (root / "dashboard_source.html").read_text()

assert 'id="diagBleLabel"' in dashboard
render = re.search(r"function renderDevices\(s\)\{(.*?)\nfunction ", dashboard, re.S)
assert render
body = render.group(1)
assert "diagBleLabel" in body and "diagBleCard" in body
assert "diagBleLabel.style.display=diagDisplay" in body
assert "diagBleCard.style.display=diagDisplay" in body

assert "PREVIOUS BOOT" in dashboard
assert "busOffEventUptimeMs" in dashboard
assert "busOffRecordBoot" in dashboard
assert "if(!r.ok)" in dashboard and "bus-off-persistence-clear-failed" in dashboard
assert "BUS OFF evidence" in dashboard
print("dashboard diagnostics visibility/persistence contract: PASS")
