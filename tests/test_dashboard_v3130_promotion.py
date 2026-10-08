from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'dashboard_source.html').read_text()
assert 'id="nagIgnoreApStateToggle"' in s, 'Torque AP bypass control missing'
assert "toggleNag(e.target.checked,'ignoreApState')" in s
assert 'data-panel="panelCountry"' in s and '/api/country/stats' in s
assert 'panelLabCountry' not in s and '/api/lab/country/' not in s
for removed in ['modeHRev1','modeHRev3','modeHRev4','Rev.4','REV.4','Rev.1','Rev.2','Rev.3','MODE H · BETA','id="humanV3TuningWrap"','id="humanV2SummaryWrap"']:
    assert removed not in s, removed
settings=s.split('<main class="page" data-page="settings"',1)[1].split('</main>',1)[0]
assert 'data-panel="panelCountry"' in settings
print('v3.26.3 dashboard promotion: PASS')
