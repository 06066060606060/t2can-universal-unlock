from pathlib import Path

root = Path(__file__).resolve().parents[1]
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
ble = (root / 's3xy_ble.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')


# Volatile session control must expose direct STOCK/CHILL/SPORT/PERFORMANCE
# selection, plus a dedicated S3XY Performance action.
for token in ('PEDAL_MAP_RAW_STOCK', 'requestPedalMapMode', 'pedalMapClearSessionOnParkTransition'):
    assert token in logic, f'missing runtime session symbol {token}'
assert 'S3XY_ACTION_PERFORMANCE_MODE' in ble
assert 'performance_mode' in ble and 'performance_mode' in html
assert 'Acceleration Mode Toggle' in ble and 'accel_mode_toggle' in html

# LAB direct selection + dedicated Performance button.
for token in ('Pedal Response', 'pedalMapSelect', 'pedalMapPerformance'):
    assert token in html, f'missing LAB PedalMap UI {token}'
for mode in ('STOCK', 'CHILL', 'SPORT', 'PERFORMANCE'):
    assert mode in html

# Session control is runtime-only: API must not persist any pedal-map mode.
assert '/api/pedalmap/set' in api
pedal_api_start = api.index('static void httpPedalMapSet')
pedal_api_end = api.index('static String nagCfgToJson', pedal_api_start)
pedal_api = api[pedal_api_start:pedal_api_end]
for forbidden in ('Preferences p;', '.putUChar(', '.putInt(', '.putBool(', '.putString('):
    assert forbidden not in pedal_api, f'PedalMap session API must not write NVS: {forbidden}'

# A real transition into Park from both supported gear paths must clear session.
handle118 = logic[logic.index('static void handle280'):logic.index('static void handle390')]
handle186 = logic[logic.index('static void handle390'):logic.index('static void handle921')]
assert 'pedalMapClearSessionOnParkTransition' in handle118
assert 'pedalMapClearSessionOnParkTransition' in handle186
assert 'prevGs' in handle118 and 'prevGs' in handle186

# AP Pedal / Regen Profile may temporarily own the outgoing 0x334, but it must not
# destroy a manual drive-session target; Performance resumes when AP exits.
obs = logic[logic.index('static bool pedalMapObserveAndPrepare'):logic.index('// Model Y L: 0x334', logic.index('static bool pedalMapObserveAndPrepare'))]
assert 'pedalMapSessionClearPure' not in obs, 'AP ownership must not erase manual PedalMap session'

print('v3.6b4 PedalMap session contract OK')
