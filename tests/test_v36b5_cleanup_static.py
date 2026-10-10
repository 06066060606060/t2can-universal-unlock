from pathlib import Path

root = Path(__file__).resolve().parents[1]
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
capture = (root / 'can_research_capture.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')

def has_json_key(text: str, key: str) -> bool:
    return f'\\"{key}\\"' in text or f'"{key}"' in text

# Removed state was write-only / unused: no live consumer existed in v3.6b4.
retired = [
    'YL_REGEN_STANDARD_RAW',
    'dasAutoLaneChangePrevState','dasAutoLaneChangeLastChangeMs','dasAutoLaneChangeChangeCount',
    'last249Ms','seen3C2','rx3C2','last3C2Mux1Ms','real3C2LeftButton','real3C2RightButton',
    'stalklessTxOk','stalklessTxFail','doorButtonRx','doorCancelAccepted','doorCancelBlocked',
]
combined = logic + '\n' + runtime
for symbol in retired:
    assert symbol not in combined, f'dead/write-only symbol remains: {symbol}'
assert 'researchCaptureRawFirstFrameMs' not in capture

# HOME fast snapshot: only current embedded HOME rendering contract remains.
start = api.index('static String homeFastSnapshotToJson() {')
end = api.index('static String homeSlowSnapshotToJson() {', start)
home_fast = api[start:end]
for key in (
    'sessionActive','loadSheddingActive','parked','aca','spr','txQueueNow','txQueueMax',
    'txEnabled','manualSuppressed','gearRaw','summonSessionActive','remoteStartupEvidence','manualLatchActive',
):
    assert not has_json_key(home_fast, key), f'home-fast still emits unused key {key}'
for key in (
    'torque','stoppedGate','apActive','canAState','enabled','noaActive','dasStateValid','dasState',
    'canState','canStateName','mcpTrafficSeen','mcpTrafficOnline','mcpTrafficAgeMs',
    'twaiTrafficSeen','twaiTrafficOnline','twaiTrafficAgeMs',
    'txState','txReason','gearName','txOk','txFail','lastTxValid','lastTxAgeMs',
):
    assert has_json_key(home_fast, key), f'home-fast lost required key {key}'

# Blink panel reuses its existing 800ms request for 0x24A RX. Full DAS API stays available.
blink_start = api.index('static String blinkAStatsToJson() {')
blink_end = api.index('static String dasTelemetryStatsToJson() {', blink_start)
blink_json = api[blink_start:blink_end]
assert has_json_key(blink_json, 'visualDebugRx')
assert 'server.on("/api/das/stats"' in api, 'full diagnostic DAS endpoint must remain'
assert 'async function fetchDas' not in html
assert 'async function fetchCanTraffic' not in html
assert has_json_key(blink_json, 'visualDebugRx')
assert "pollDue('das'" not in html
assert "else if(p==='panelBlink')pollDue('blink',800,fetchBlink,now,force);" in html

# D/E/F are retired; all supported modes retain the same AP-driven HOME ACTIVE state.
home_render_start = html.index('function renderHomeFast(s){')
home_render_end = html.index('function renderHomeSlow(s){', home_render_start)
home_render = html[home_render_start:home_render_end]
assert '[4,5,6]' not in home_render
assert 'nagActive=!!n.apActive' in home_render

print('v3.6b5 zero-behavior cleanup contract OK')
