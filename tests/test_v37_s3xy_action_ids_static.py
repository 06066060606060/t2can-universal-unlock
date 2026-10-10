from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
s3xy = (ROOT / 's3xy_ble.h').read_text()

expected = [
    ('S3XY_ACTION_NONE', 0, 'none'),
    ('S3XY_ACTION_NOA_CANCEL', 1, 'noa_cancel'),
    ('S3XY_ACTION_ACCEL_MODE_TOGGLE', 2, 'accel_mode_toggle'),
    ('S3XY_ACTION_RESEARCH_CAPTURE_C', 3, 'research_capture_c'),
    ('S3XY_ACTION_RESEARCH_CAPTURE_A', 4, 'research_capture_a'),
    ('S3XY_ACTION_RESEARCH_CAPTURE_B', 5, 'research_capture_b'),
    ('S3XY_ACTION_RESEARCH_CAPTURE_D', 6, 'research_capture_d'),
    ('S3XY_ACTION_RESEARCH_CAPTURE_RESET', 7, 'research_capture_reset'),
    ('S3XY_ACTION_TLSSC_TOGGLE', 8, 'tlssc_toggle'),
    ('S3XY_ACTION_AUTO_BLINKER_TOGGLE', 9, 'auto_blinker_toggle'),
    ('S3XY_ACTION_PERFORMANCE_MODE', 10, 'performance_mode'),
    ('S3XY_ACTION_LEFT_BLINKER', 11, 'left_blinker'),
    ('S3XY_ACTION_RIGHT_BLINKER', 12, 'right_blinker'),
]
for name, value, code in expected:
    assert re.search(rf'\b{name}\s*=\s*{value}\b', s3xy), (name, value)
    assert f'return "{code}";' in s3xy, code

assert 'S3XY_ACTION_NAG' not in s3xy
assert 'nag_killer' not in s3xy.lower()
print('v3.7 S3XY action IDs remain stable: PASS')
