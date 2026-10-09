from pathlib import Path

R = Path(__file__).resolve().parents[1]
ino = next(R.glob('*.ino')).read_text()
vl = (R / 'vehicle_logic.h').read_text()
api = (R / 'web_api.h').read_text()
dash = (R / 'dashboard_source.html').read_text()
fixed = (R / 'r79_fixed_policy_pure.h').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
a = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
fast = vl[a:vl.index('static void r79FixedObserveStock(', a)]
assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast
assert 'twai_transmit(&out, 0)' not in fast
assert 'r79FixedApplyBitsPure(out.data, bit18Policy, r79Hw3Active())' in fast
assert 'r79RetrySchedule(R79LAB_TX_IMMEDIATE' in fast
for key in ('transport', 'bit18Mode', 'bit18ModeName', 'fastAttempts',
            'fastTxOk', 'fastTxFail'):
    assert f'"{key}"' in api, key
assert 'D9_MUX1_2MS_WAIT' in api and 'D9_MUX1_FAST_ECHO_0MS' in api
settings = dash[dash.index('<section class="panel" id="panelR79"'):dash.index('<section class="panel" id="panelTlssc"')]
assert 'FAST ECHO · 0 ms' in settings and '2 ms WAIT' in settings
panel = dash[dash.index('<section class="panel" id="panelLabR79"'):dash.index('<section class="panel" id="panelNagModeH"')]
assert 'R79 Status' in panel
assert 'FAST ECHO · 0 ms' not in panel and 'V2.6 STYLE' not in panel
print('v3.7 fixed post-mux1 enqueue contract: PASS')
