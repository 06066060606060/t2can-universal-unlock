"""Web Serialization contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class WebSerializationContracts(unittest.TestCase):

    def test_binary_size(self):
        """Retained from test_v36c6_binary_size_static.py."""
        from pathlib import Path
        import gzip
        import importlib.util
        import re

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
        core = (root/'can_core.h').read_text(encoding='utf-8').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        web = (root/'web_api.h').read_text(encoding='utf-8')
        s3xy = (root/'s3xy_ble.h').read_text(encoding='utf-8')
        idx = (root/'index_html.h').read_text(encoding='utf-8')
        spec = importlib.util.spec_from_file_location('build_dashboard', root/'tools'/'build_dashboard.py')
        builder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(builder)

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

    def test_api_key(self):
        """Retained from test_v36c7_api_key_contract.py."""
        from pathlib import Path
        import json, re
        ROOT=Path(__file__).resolve().parents[1]
        expected=json.loads((ROOT/'tests/c7_api_keys_expected.json').read_text())

        # Deliberate v3.7 R79 retirement allowlist. Any future key removal still fails
        # unless it is explicitly reviewed and added here.
        RETIRED_R79_KEYS={
            'strategy','periodMs','refreshMode','schedulerMode','periodicShots',
            'postMux1Mode','quietAnchor','postMux2Enabled','postMux2OffsetMs',
            'preMux1Enabled','preMux1OffsetMs','mux0DelayMs','roamingMirrorRatio',
            'roamingBurst','roamingSilenceMs','timingCapture',
        }
        assert RETIRED_R79_KEYS.isdisjoint(expected['r79StatsToJson']['keys'])

        # v3.13 promotes one Mode H engine. Only fields belonging to retired
        # revision-specific tuning/telemetry may disappear from this endpoint.
        RETIRED_KEYS_BY_FUNCTION={
            'nagHumanProfileStatsToJson': {
                'carrierDirectionMode','carrierDirectionName','durationMaxMs',
                'durationMinMs','excursionMaxNm','hoOverridePct','holdMaxNm',
                'holdMinNm','intervalMaxMs','intervalMinMs',
            },
        }
        for function,keys in RETIRED_KEYS_BY_FUNCTION.items():
            assert keys <= set(expected[function]['keys']), function

        def extract(src,fn,return_type='String'):
            m=re.search(rf'static {return_type} {re.escape(fn)}\([^)]*\)\s*\{{',src,re.S)
            assert m, fn
            i=m.end(); depth=1; j=i; in_s=False; esc=False
            while j < len(src) and depth:
                c=src[j]
                if in_s:
                    if esc: esc=False
                    elif c=='\\': esc=True
                    elif c=='"': in_s=False
                else:
                    if c=='"': in_s=True
                    elif c=='{': depth+=1
                    elif c=='}': depth-=1
                j += 1
            return src[m.start():j]

        for fn,meta in expected.items():
            src=(ROOT/meta['path']).read_text()
            body=extract(src,fn)
            for helper in re.findall(r'\b(write[A-Za-z0-9_]+Json)\(jw\b', body):
                body += extract(src,helper,'void')
            literal_keys=set(re.findall(r'\\"([A-Za-z0-9_]+)\\":', body))
            writer_keys=set(re.findall(r'\b\w+\.(?:boolean|u32|i32|fixed|string|raw|beginObject|beginArray)\(\"([A-Za-z0-9_]+)\"', body))
            current=literal_keys | writer_keys
            retired=RETIRED_KEYS_BY_FUNCTION.get(fn,set())
            assert retired.isdisjoint(current), f'{fn} retirement allowlist includes live keys: {sorted(retired & current)}'
            missing=sorted(set(meta['keys'])-current-retired)
            assert not missing, f'{fn} lost API keys: {missing}'
        print('v3.6d9a2 API key preservation contract: PASS')

    def test_json_writer(self):
        """Retained from test_v36c7_json_writer_static.py."""
        from pathlib import Path
        import re

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob('*.ino'))
        if not ino.exists():
            ino = next(ROOT.glob('T2CAN-Universal-*.ino'))
        text = ino.read_text()
        web = (ROOT / 'web_api.h').read_text()
        driver = (ROOT / 'driver_monitor_capture.h').read_text()

        assert '#include "json_writer_arduino.h"' in text, 'compact JSON writer must be included'
        assert (ROOT / 'json_writer_arduino.h').exists(), 'compact JSON writer header missing'

        # The largest dashboard JSON builders must use the shared writer so repeated
        # String concatenation code does not expand independently in every function.
        for fn in [
            'nagStatsToJson',
            'summonStatsToJson',
            'ulcStatsToJson',
            'r79StatsToJson',
            'systemStatsToJson',
            'researchCaptureStatsToJson',
            'nagHumanProfileStatsToJson',
            'blinkAStatsToJson',
            'dasTelemetryStatsToJson',
            'canTrafficStatsToJson',
            'homeFastSnapshotToJson',
            'homeSlowSnapshotToJson',
            'settingsLiteSnapshotToJson',
            'labLiteSnapshotToJson',
            'homeSnapshotToJson',
            'nagCfgToJson',
            'v3FeaturePolicyJson',
            'vehicleProfileStatusJson',
            'wifiApStatusJson',
        ]:
            m = re.search(rf'static String {fn}\(\) \{{(.*?)(?=\nstatic (?:String|void|const char\*|bool|uint|int))', web, re.S)
            assert m, f'could not locate {fn}'
            body = m.group(1)
            assert 'JsonWriterArduino' in body, f'{fn} must use JsonWriterArduino'

        m = re.search(r'static String driverMonitorCaptureStatsToJson\(\) \{(.*)$', driver, re.S)
        assert m and 'JsonWriterArduino' in m.group(1), 'driverMonitorCaptureStatsToJson must use JsonWriterArduino'


        vehicle = (ROOT / 'vehicle_logic.h').read_text()
        m = re.search(r'static String pedalMapStatsJson\(\)\{(.*?)(?=\nstatic )', vehicle, re.S)
        assert m and 'JsonWriterArduino' in m.group(1), 'pedalMapStatsJson must use JsonWriterArduino'


        s3xy = (ROOT / 's3xy_ble.h').read_text()
        m = re.search(r'static String s3xyStatsToJson\(\) \{(.*?)(?=\nstatic )', s3xy, re.S)
        assert m and 'JsonWriterArduino' in m.group(1), 's3xyStatsToJson must use JsonWriterArduino'

        print('v3.6d9a2 JSON writer static contract: PASS')

        research = re.search(r'static String researchCaptureStatsToJson\(\) \{(.*?)(?=\nstatic void httpResearchCaptureStats)', web, re.S).group(1)
        assert 'jw.beginObject("labels")' in research
        assert 'jw.endObject();' in research, 'nested labels object must close without finishing the top-level writer'

        # Numeric JsonWriter calls must receive a numeric value only. A bulk refactor
        # must never leave legacy String concatenation inside u32()/i32()/u64().
        for lineno, line in enumerate(web.splitlines(), 1):
            if re.search(r'jw\.(?:u32|i32|u64|fixed)\s*\(', line):
                assert 'String(' not in line and '+ ",\\\"' not in line, \
                    f'malformed numeric JsonWriter call at web_api.h:{lineno}: {line.strip()}'

    def test_snapshot_table(self):
        """Retained from test_v36c7_snapshot_table_static.py."""
        from pathlib import Path
        ROOT=Path(__file__).resolve().parents[1]
        web=(ROOT/'web_api.h').read_text()
        assert 'struct SnapshotGroupSpec' in web
        assert 'SNAPSHOT_GROUP_SPECS' in web
        for name in ('nag','blink','das','summon','s3xy','system','cantraffic','ulc','r79','researchcapture'):
            assert f'{{"{name}",' in web, name
        assert 'for (size_t i = 0; i < SNAPSHOT_GROUP_SPEC_COUNT; ++i)' in web
        print('v3.6d9a2 snapshot table contract: PASS')


if __name__ == "__main__":
    unittest.main()
