from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob('*.ino')).read_text()
vl = (ROOT / 'vehicle_logic.h').read_text()
cc = (ROOT / 'can_core.h').read_text()
cr = (ROOT / 'can_runtime.h').read_text()
web = (ROOT / 'web_api.h').read_text()
dash = (ROOT / 'dashboard_source.html').read_text()
fixed = (ROOT / 'r79_fixed_policy_pure.h').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
assert 'D9_MUX1_2MS_WAIT' in web
assert 'D9_MUX1_FAST_ECHO_0MS' in web
assert 'String periodic = "MUX2_PLUS_"' in web

# Select arrows retain their background image when the current skin changes
# only background-color. The original form rule owns repeat and placement.
assert '.selectArrow{background-image:url(' in dash
assert 'background-repeat:no-repeat!important' in dash
assert 'body.t2-2027 .selectArrow,body.t2-2027 .attach,body.t2-2027 .labSelect' in dash
assert 'background-color:transparent!important' in dash

# Generic CSV download must clear pause state in finally and resume polling.
fn_start = dash.index('async function downloadFile(')
fn_end = dash.index('\n', fn_start)
dlfn = dash[fn_start:fn_end]
assert 'await fetch(' in dlfn and 'new Blob(' in dlfn and 'saveDownloadedBlob(' in dlfn
assert 'finally{' in dlfn and 'downloadInProgress=false' in dlfn and 'downloadManaged=false' in dlfn
assert 'forcePoll()===' not in dlfn
assert 'setTimeout(()=>forcePoll(),0)' in dlfn

assert 'Session avg/min · bus / arb / BUS OFF' in dash
assert 'canDiagRatePrev' not in dash

assert 'MCP_PREFETCH_CAPACITY = 1' in cr
pref = cr[cr.index('MCP_PREFETCH_CAPACITY = 1'):cr.index('// ── STATUS CHECK / RECOVERY', cr.index('MCP_PREFETCH_CAPACITY = 1'))]
assert 'Can_A.readMessage(&prefetched[batch])' in pref
assert 'processed < MCP_RX_BUDGET' in pref
assert 'batch < readBatchBudget' in pref
assert 'canARxReadBatchBudgetPure()' in pref
assert pref.index('Can_A.readMessage(&prefetched[batch])') < pref.index('canRxObserve(CAN_RX_BUS_PARTY')

assert 'TWAI_ALERT_TX_SUCCESS' not in cr
assert 'r79FastReactiveObserveTxSuccessAlert' not in cr + vl
assert 'canBTxAcceptedSerial' not in cc and 'canBTxPipelineIdle' not in cc

reset = web[web.index('static bool resetRuntimeStats'):web.index('static void httpResetRuntimeStats', web.index('static bool resetRuntimeStats'))]
for name in ('r79FastEchoAttempts', 'r79FastEchoTxOk', 'r79FastEchoTxFail'):
    assert f'{name} = 0' in reset, name

print('v3.7 follow-up preservation contract: PASS')
