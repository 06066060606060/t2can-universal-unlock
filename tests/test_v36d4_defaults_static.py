from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(errors='ignore')
logic = (root / 'vehicle_logic.h').read_text(errors='ignore')
core = (root / 'can_core.h').read_text(errors='ignore')
v3 = (root / 'nag_human_v3_pure.h').read_text(errors='ignore')
fixed = (root / 'r79_fixed_policy_pure.h').read_text(errors='ignore')
dash = (root / 'dashboard_source.html').read_text(errors='ignore')

assert '#define FW_VERSION "v3.26.3"' in ino
assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
assert 'R79_BIT18_DEFAULT_PURE = R79_BIT18_STOCK_PURE' in fixed
assert 'r79FixedFastEcho' in logic and 'r79FixedTick' in logic
assert 'static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();' in core
assert 'prefs.getUChar("hv", nagModeHDefaultVariantPure())' in core
for token in ['c.base.peakMaxRaw = 210u', 'c.base.waitMaxMs = 2500u',
              'c.ho1ThresholdRaw = 40u', 'c.ho2ThresholdRaw = 175u',
              'H3_CARRIER_FOLLOW_STOCK', 'H3_HO_TIERED_1_2']:
    assert token in v3, token
assert 'Mode H' in dash
assert 'R79 Status' in dash
print('PASS v3.8 fixed defaults static')
