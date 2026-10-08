from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")
INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")

lab = DASH.split('<main class="page" data-page="lab">', 1)[1].split('</main>', 1)[0]

# The supplied HTML uses a compact driving card and a separate, inset tool list.
assert '<div class="labDirectionRow">' in lab
assert lab.count('class="labDirectionCell') == 3
assert 'id="alcLeftLaneBig"' not in lab
assert 'id="alcRightLaneBig"' not in lab
assert '<div class="sectiontitle">Research Tools</div>' in lab
assert '<div class="sectionnote">open a dedicated LAB page</div>' not in lab

# Overview rows show a value and open their controls in a detail page.
assert 'data-panel="panelLabDmsNag"' not in lab
assert 'data-panel="panelLabCanARx"' not in lab
assert 'id="labDmsNagValue"' not in lab
assert 'id="labCanARxValue"' not in lab
assert 'id="labDmsNagToggle"' not in lab
assert 'id="labCanARxMode"' not in lab
assert 'id="labDmsNagToggle"' not in DASH
assert 'id="nagDmsToggle"' not in DASH
assert 'id="driverMonitoringToggle"' in DASH
assert 'id="labCanARxMode"' not in DASH

# An old .page[data-page="lab"] #alcStateBig rule has ID specificity.
# The final LAB rules must explicitly replace its centered legacy geometry.
assert 'body.t2-2027 .page[data-page="lab"] #alcStateBig.driveStateValue{' in DASH
assert 'body.t2-2027 .page[data-page="lab"] .labDirectionCell .driveStateValue{' in DASH
assert 'display:block' in DASH.split('body.t2-2027 .page[data-page="lab"] .labDirectionCell .driveStateValue{', 1)[1].split('}', 1)[0]

# The reference's Latin font is served locally to the ESP32 dashboard.
assert 'font-family:"Geist"' in DASH
assert '/fonts/geist-lab.woff2' in DASH
assert '/fonts/geist-mono-lab.woff2' in DASH
assert 'server.on("/fonts/geist-lab.woff2"' in API
assert 'server.on("/fonts/geist-mono-lab.woff2"' in API
assert '#include "lab_fonts.h"' in INO

print("PASS v3.8 supplied LAB HTML parity and legacy-style override")
