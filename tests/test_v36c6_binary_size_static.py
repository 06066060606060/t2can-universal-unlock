from pathlib import Path
import gzip
import importlib.util
import re

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
core = (root/'can_core.h').read_text(encoding='utf-8')
web = (root/'web_api.h').read_text(encoding='utf-8')
s3xy = (root/'s3xy_ble.h').read_text(encoding='utf-8')
idx = (root/'index_html.h').read_text(encoding='utf-8')
spec = importlib.util.spec_from_file_location('build_dashboard', root/'tools'/'build_dashboard.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

assert '#define FW_VERSION "v3.26.3"' in ino
assert (root/'fixed_point_pure.h').exists()
assert 'strtod(' not in web
for dead in ('steeringScale', 'steeringOffset', 'steeringAngleDeg', 'lastModeCTorqueNm'):
    assert dead not in core, dead
assert 'volatile float    nagRealTorque' not in core
assert 'volatile float    nagLastInjectedNm' not in core
assert 'T2CAN_SERIAL_DIAGNOSTICS' in ino
assert '#define T2CAN_SERIAL_DIAGNOSTICS 0' in ino
assert '#if T2CAN_SERIAL_DIAGNOSTICS' in core
# Diagnostic-only rate limiting / heartbeat / AP-IP lookup must disappear from
# the normal preprocessed build rather than survive around a no-op print macro.
assert '#if T2CAN_SERIAL_DIAGNOSTICS\nstatic unsigned long nagLastTxFailLog' in core
assert '#if T2CAN_SERIAL_DIAGNOSTICS\n  esp_reset_reason_t reset_reason' in ino
assert '#if T2CAN_SERIAL_DIAGNOSTICS\n  static unsigned long lastBeatLog' in ino
assert '#if T2CAN_SERIAL_DIAGNOSTICS\n  IPAddress ip = WiFi.softAPIP();' in (root/'web_api.h').read_text(encoding='utf-8')

# Normal builds keep S3XY heavy diagnostics compiled out and diagnostic-only
# message construction should be guarded as well.
assert '#if S3XY_DIAGNOSTICS_ENABLED' in s3xy
assert 'S3XY_DIAGNOSTIC_ONLY_BEGIN' not in s3xy  # no placeholder markers

# d9 adds PRE-MUX1, guarded phase-walk, per-slot telemetry and edit protection.
# Keep the embedded dashboard within the canonical build budget while retaining gzip embedding.
m = re.search(r'INDEX_HTML_GZ\[\].*?=\s*\{(.*?)\};', idx, re.S)
assert m, 'embedded gzip array missing'
vals = re.findall(r'0x([0-9A-Fa-f]{2})', m.group(1))
blob = bytes(int(x, 16) for x in vals)
assert builder.MAX_EMBEDDED_GZIP_BYTES == 100_000, builder.MAX_EMBEDDED_GZIP_BYTES
assert len(blob) < builder.MAX_EMBEDDED_GZIP_BYTES, len(blob)
html = gzip.decompress(blob).decode('utf-8')
assert '<!DOCTYPE html>' in html or '<!doctype html>' in html.lower()
assert 'TESLA UNLOCK' in html
expected = builder.minify_dashboard((root/'dashboard_source.html').read_text(encoding='utf-8'))
assert html == expected, 'index_html.h is stale relative to dashboard_source.html'
print(f'v3.6d9a2 binary-size contract OK (embedded gzip={len(blob)} bytes)')
