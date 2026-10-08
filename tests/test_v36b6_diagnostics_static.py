from pathlib import Path

root = Path(__file__).resolve().parents[1]
core = (root / 'can_core.h').read_text(encoding='utf-8')
runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
capture = (root / 'can_research_capture.h').read_text(encoding='utf-8')
forward = (root / 't2can_forward.h').read_text(encoding='utf-8')
pins = (root / 'pin_config.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')

def has_json_key(text: str, key: str) -> bool:
    return f'\\"{key}\\"' in text or f'"{key}"' in text

# Version target.
assert '#define FW_VERSION "' in ino

# CAN A forensic ring must observe every MCP application TX through the single hardware gate.
assert 'CAN_A_TX_TRACE_CAPACITY = 64' in core
assert 'struct CanATxTraceEntry' in core
assert 'static void canATraceRecordTx(' in core
tagged_start = core.index('static bool canTxMcpSendTagged(')
mcp_start = core.index('static bool canTxMcpSend(', tagged_start)
mcp_end = core.index('// ═══════════════════════════════════════════════════════════════\n// CAN RECOVERY SUPERVISOR', mcp_start)
mcp_tagged_fn = core[tagged_start:mcp_start]
mcp_fn = core[mcp_start:mcp_end]
assert 'canATraceRecordTx(' in mcp_tagged_fn
assert 'canTxMcpSendTagged(' in mcp_fn

# BUS-OFF detection must freeze CAN A forensic data before MCP reinit; normal recovery conditions stay intact.
assert 'static void canATraceFreezeBusOff(' in runtime
status_start = runtime.index('// ── STATUS CHECK / RECOVERY (1 Hz) ──')
status_end = runtime.index('vTaskDelay(1);', status_start)
status = runtime[status_start:status_end]
assert 'bool busOff = (eflg & MCP2515::EFLG_TXBO) || (consecutive > 5);' in status
assert 'canATraceFreezeBusOff(' in status
assert status.index('canATraceFreezeBusOff(') < status.index('mcpReinit()')

# CAN A diagnostics are exposed without removing the existing CAN B forensic path.
for key in (
    'mcpErrorFlags','mcpTxFailConsecutive','mcpBusOffCount','mcpBusOffSnapshotValid',
    'mcpBusOffSnapshotAgeMs','mcpBusOffSnapshotEflg','mcpBusOffSnapshotTxFailSeq',
    'mcpBusOffSnapshotRxAgeMs','canATxTraceCount','canATxTraceAgeMs','canATxTraceBusOffOrdinal',
):
    assert has_json_key(api, key), f'missing system diagnostic key {key}'
assert 'static void httpCanATxTraceCsv()' in api
assert 'server.on("/api/system/cana-tx.csv"' in api
assert 'server.on("/api/system/canb-tx.csv"' in api
assert 'CAN A TX CSV' in html
assert 'id="diagCanATxCsv"' in html
assert 'id="diagCanBTxCsv"' in html
assert 'canATraceReset();' in api
assert 'canBTraceReset();' in api

# CAN Research CSV progress: server advertises row count and UI reads stream progress.
research_start = api.index('static void httpResearchCaptureCsv() {')
research_end = api.index('static void httpSystemStats()', research_start)
research_fn = api[research_start:research_end]
assert 'X-T2CAN-CSV-Rows' in research_fn
assert 'researchCaptureCsvExportRows' in research_fn
assert 'async function downloadResearchCaptureCsv()' in html
assert "response.body.getReader()" in html
assert 'new Blob(parts' in html
assert 'downloadProgress' in html
assert "Downloading… ${downloadProgress}%" in html
assert "$('researchCapDownload').onclick=downloadResearchCaptureCsv" in html
assert 'if(otaUploading||downloadInProgress||document.hidden)return;' in html
assert "downloadFile('/api/researchcapture/log.csv'" not in html

# Final source-level dead-code cleanup: remove only unreachable wrappers/branches.
for symbol in (
    'autoBlinkerEligibleRequestDir', 'researchCaptureObserveTxParty',
    'canTxFreshMaskSnapshot', 'static inline uint8_t sccm249Crc8(',
):
    assert symbol not in (core + runtime + logic + capture + forward), f'dead symbol remains: {symbol}'
for macro in ('MCP2515_INT', 'T_2Can_Fd', 'MCP2518_CS', 'MCP2518_INT', 'ESP_BOOT'):
    assert macro not in pins, f'dead pin/config macro remains: {macro}'
# Keep the actually-used pure SCCM checksum implementation.
assert 'sccm249Crc8Pure' in (root / 'auto_blinker_pure.h').read_text(encoding='utf-8')
# labLite used to compute a separate laneValid local that was never consumed.
lab_start = api.index('static String labLiteSnapshotToJson() {')
lab_end = api.index('// Lightweight HOME snapshot.', lab_start)
assert 'const bool laneValid =' not in api[lab_start:lab_end]

print('v3.6b6 diagnostics/progress/dead-code contract OK')
