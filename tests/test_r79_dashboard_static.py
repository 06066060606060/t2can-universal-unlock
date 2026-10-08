from pathlib import Path

root = Path(__file__).resolve().parents[1]
api = (root / 'web_api.h').read_text()
html = (root / 'dashboard_source.html').read_text()
logic = (root / 'vehicle_logic.h').read_text()

for key in ('fixedPolicy', 'transport', 'periodic', 'bit18Mode', 'bit18ModeName',
            'runtimeState', 'stockTemplateValid', 'stockMux1Rx', 'txOk', 'txFail',
            'fastAttempts', 'fastTxOk', 'fastTxFail', 'periodicTxOk',
            'periodicTxFail', 'quietArm', 'quietFire', 'quietGuardSkip'):
    assert f'"{key}"' in api, f'missing R79 API field {key}'

assert '/api/r79/stats' in api and '/api/r79/update' in api
assert 'R79 Status' in html and 'Read-only production status' in html
assert "jget('/api/r79/stats')" in html
for dom in ('r79FixedPolicyValue', 'r79FixedTransportValue', 'r79FixedPeriodicValue',
            'r79FixedBit18Value', 'r79FixedRuntimeValue', 'r79FixedTotalsValue',
            'r79FixedFastValue', 'r79FixedPeriodicTotalsValue', 'r79FixedQuietValue'):
    assert f'id="{dom}"' in html

panel = html[html.index('<section class="panel" id="panelLabR79"'):html.index('<section class="panel" id="panelNagModeH"')]
for interactive in ('<input', '<select', '<button class="btn"'):
    assert interactive not in panel, f'R79 status card must remain read-only: {interactive}'
for retired in ('POST-MUX2 TX', 'PRE-MUX1 TX', 'R79 Transport', 'ROAMING Mirror Ratio'):
    assert retired not in panel

assert 'r79FixedFastEcho' in logic and 'r79FixedTick' in logic
print('R79 v3.7 dashboard/API static checks passed')
