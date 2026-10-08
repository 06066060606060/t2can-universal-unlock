from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
variant = (root / 'nag_mode_h_variant_pure.h').read_text(encoding='utf-8')
core = (root / 'can_core.h').read_text(encoding='utf-8')
vehicle = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
web = (root / 'web_api.h').read_text(encoding='utf-8')
dash = (root / 'dashboard_source.html').read_text(encoding='utf-8')
fixed = (root / 'r79_fixed_policy_pure.h').read_text(encoding='utf-8')

assert '#define FW_VERSION "v3.26.3"' in ino
assert 'H_VARIANT_REV3' not in variant
assert '#include "nag_human_v3_pure.h"' in ino
assert 'nagHumanV3StepPure' not in core
assert 'nagHumanV4StepPure' in core
assert 'hoOverrideValid' in core and 'hoLevel' in core
assert 'txf.data[4] = (uint8_t)((txf.data[4] & 0x3Fu)' in core

assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
assert 'r79FixedFastEcho' in vehicle and 'r79FixedTick' in vehicle
assert '/api/r79/stats' in web and '/api/r79/update' in web
assert 'R79 Status' in dash and 'Read-only production status' in dash

assert 'modeHRev3' not in dash
assert 'humanV3Direction' not in dash
assert 'humanV3HoPolicy' not in dash
assert 'humanV3Ho1Nm' not in dash
assert 'humanV3Ho2Nm' not in dash
print('v3.7 Rev.3 / fixed R79 contract OK')
