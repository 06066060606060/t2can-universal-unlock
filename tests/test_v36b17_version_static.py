from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ino=next(ROOT.glob('*.ino')).read_text(errors='ignore')
changelog=(ROOT/'CHANGELOG.md').read_text(errors='ignore')
validation=(ROOT/'VALIDATION.md').read_text(errors='ignore')
html=(ROOT/'dashboard_source.html').read_text(errors='ignore')

assert '#define FW_VERSION "v3.26.3"' in ino
assert changelog.startswith('# T2CAN Universal v3.26.3\n')
assert validation.startswith('# T2CAN Universal v3.26.3 — Validation\n')
assert 'T2CAN_2027_FULL_BLEED' in html
assert 'T2CAN_V38_DETAIL_COMPONENTS' in html
assert 'Mode H' in html and 'Mode H · Rev.' not in html
assert 'Natural Grip' not in html
assert 'OPPOSITE CARRIER' in html
print('v3.7.3 version/dashboard contract passed')
