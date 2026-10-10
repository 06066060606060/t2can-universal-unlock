from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

settings_start = DASH.index('<main class="page" data-page="settings">')
settings_end = DASH.index('</main>', settings_start)
settings = DASH[settings_start:settings_end]
nag_start = DASH.index('<section class="panel" id="panelNag">')
nag_end = DASH.index('</div></section>\n<section class="panel" id="panelBlink">', nag_start)
nag = DASH[nag_start:nag_end]

assert 'data-panel="panelApRightScroll"' not in settings
assert 'data-panel="panelApRightScroll"' not in nag
assert 'id="panelApRightScroll"' not in DASH
assert 'id="tsl9InputMode"' in nag
assert 'id="tsl9IsaToggle"' not in nag
assert 'id="isaSuppressionToggle"' in settings
assert 'id="nagDmsToggle"' not in nag
assert 'id="driverMonitoringToggle"' in settings
print("PASS AP Right Scroll retired, TSL9 input stays in Nag, and DMS/ISA are standalone")
