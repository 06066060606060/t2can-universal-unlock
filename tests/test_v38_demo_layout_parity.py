from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


# The production page has no 50 px mock iPhone status bar. Content therefore
# starts 8 px below the 48 px fixed header, as it does inside the approved demo.
assert (
    'body.t2-2027 .page,body.t2-2027 .page[data-page="lab"]'
    '{display:none;max-width:520px;min-height:100vh;min-height:100svh;'
    'min-height:100dvh;'
    'padding:calc(56px + env(safe-area-inset-top)) 18px '
    'calc(120px + env(safe-area-inset-bottom))'
) in DASH
assert 'body.t2-2027 .page[data-page="home"]{padding-top:calc(56px + env(safe-area-inset-top))}' in DASH
assert 'body.t2-2027 .sectionhead{padding:10px 6px 0;margin:10px 0 8px;align-items:center}' in DASH
assert 'body.t2-2027 .globaltop .live:before{content:"";width:7px;height:7px;border-radius:50%;background:#e5484d;box-shadow:none}' in DASH
assert 'body.t2-2027 .globaltop .live.ok:before{background:var(--ok);box-shadow:none}' in DASH
assert 'body.t2-2027 .globaltop .live.busy:before{background:var(--busy);box-shadow:none}' in DASH

# The approved R79 card is one horizontal row: copy left, two counters right.
# Explicit flex direction prevents the legacy homeFeatureCard column rule from
# leaking into the 2027 layout.
assert (
    'body.t2-2027 .homeR79Card{grid-area:r79;min-height:0;padding:17px 18px;'
    'display:grid;grid-template-columns:minmax(0,1fr) auto;gap:16px;'
    'align-items:center;text-align:left;flex-direction:row}'
) in DASH
assert (
    'body.t2-2027 .homeInjectionStats{display:grid;'
    'grid-template-columns:repeat(2,52px);gap:10px;text-align:center}'
) in DASH

# CAN cards use their content height like the demo, rather than the older
# fixed 124/128 px production cards.
assert 'body.t2-2027 .homeBus{min-height:0;padding:16px 18px' in DASH
assert 'body.t2-2027 .homeBus{padding:15px;min-height:0}' in DASH

# Home metrics follow the demo's value-first reading order.
assert '<div class="metric"><div class="v" id="torque">—</div><div class="k" id="homeNagSignalLabel">Stock torque Nm</div></div>' in DASH
assert '<div class="metric"><div class="v" id="homeS3xy">0 / 0</div><div class="k">S3XY</div></div>' in DASH
assert '<div class="metric" id="homeBlinkMetric"><div class="v" id="blinkerState">—</div><div class="k">Blinker</div></div>' in DASH
assert 'body.t2-2027 .metric .k{margin-top:5px;' in DASH
assert 'body.t2-2027 .metric .v,body.t2-2027 #homeMetrics .metric .v{margin-top:0;' in DASH
assert 'body.t2-2027 .homeFeatureV{margin-top:7px;' in DASH
assert 'body.t2-2027 .homeFeatureLast{margin-top:5px;' in DASH
assert 'body.t2-2027 .homeInjectionStats .homeMetaV{margin-top:5px;' in DASH
assert 'body.t2-2027 .homeBusVal{margin-top:14px;' in DASH
assert 'body.t2-2027 .homeBusMeta{margin-top:6px;' in DASH
assert 'id="homeCanAName">CAN A</div>' in DASH
assert 'id="homeCanBName">CAN B</div>' in DASH
assert "a.dataset.role=s.canA||t.canA.replace('CAN A · ','')" in DASH
assert "b.dataset.role=s.canB||t.canB.replace('CAN B · ','')" in DASH
assert "role=$(nameId)?.dataset.role||''" in DASH

# LAB reproduces the demo's separated three-cell state row and inset list.
assert (
    'body.t2-2027 .page[data-page="lab"] .labDirectionRow{display:grid;'
    'grid-template-columns:1fr 1.1fr 1fr;gap:6px;align-items:stretch;'
    'margin-top:16px;background:transparent;border-radius:0;overflow:visible}'
) in DASH
assert (
    'body.t2-2027 .page[data-page="lab"] .labDirectionCell{min-width:0;'
    'min-height:76px;padding:12px;border:1px solid var(--line);border-radius:18px'
) in DASH
assert '<section class="card labToolsCard">' in DASH
assert (
    'body.t2-2027 .page[data-page="lab"] .labToolsCard{padding:6px 18px}' in DASH
    and 'body.t2-2027 .page[data-page="lab"] .labToolsCard .listrow'
        '{min-height:0;padding:13px 0;gap:12px}' in DASH
)
assert 'id="labDmsNagValue"' not in DASH
assert 'id="nagDmsToggle"' not in DASH
assert 'id="driverMonitoringToggle"' in DASH
assert 'id="labCanARxValue"' not in DASH
assert 'id="labCanARxMode"' not in DASH
assert '<span class="value">EXP</span><span class="chev">›</span>' in DASH

# The demo footer keeps 0x399 and 0x239 freshness as two independent values.
assert '<span id="alcStateSub">0x399 —</span>' in DASH
assert '<span id="lane239AgeSub">0x239 age —</span>' in DASH
assert "$('lane239AgeSub').textContent=a.lane239Valid?('0x239 age '+fmtMs(a.lane239AgeMs)):'0x239 age —'" in DASH

print("PASS v3.8 approved-demo layout parity contract")
