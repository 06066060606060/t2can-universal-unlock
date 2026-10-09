from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob('*.ino'))
if not ino.exists():
    ino = next(ROOT.glob('T2CAN-Universal-*.ino'))
text = ino.read_text()
web = (ROOT / 'web_api.h').read_text()
driver = (ROOT / 'driver_monitor_capture.h').read_text()

assert '#define FW_VERSION "v3.28.0"' in text, 'firmware version must be v3.28.0'
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
