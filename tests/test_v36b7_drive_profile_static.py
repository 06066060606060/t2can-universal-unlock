from pathlib import Path

root = Path(__file__).resolve().parents[1]
profile = (root / 'vehicle_profile.h').read_text(encoding='utf-8')
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')
assert '#define FW_VERSION "v3.26.3"' in ino

# AP Pedal / Regen Profile follows the 0x334-capable topology, not a YL model check.
assert 'vehicleProfileApDriveProfileSupported' in profile
assert 'activeProfileApDriveProfileSupported' in profile
assert 'activeProfileApDriveProfileSupported()' in logic
assert 'if (!activeProfileIsYl()) return false;' not in logic[logic.index('static bool apDriveProfileGateOpen()'):logic.index('static bool manualDrivingGateOpen')]

# Body+Chassis must be supported while Party+Chassis remains unsupported because
# it has no Body 0x334 route.
assert 'return vehicleProfilePedalMapSupported(id, topology);' in profile

# Three explicit regen targets; Reduced remains the compatibility default.
for token in ('AP_DRIVE_REGEN_STANDARD_RAW', 'AP_DRIVE_REGEN_REDUCED_RAW', 'AP_DRIVE_REGEN_MINIMAL_RAW'):
    assert token in logic or (root / 'ap_drive_profile_pure.h').exists(), token
assert 'apDriveRegenRaw' in logic
assert 'apRegen' in logic
assert 'AP_DRIVE_REGEN_REDUCED_RAW' in logic
assert 'outData[2]=apRegen' in logic.replace(' ', '')

# CAN A / Body AP-profile TX must contribute to AP profile TX diagnostics too.
can_a = logic[logic.index('static void handlePedalMap334OnCanA'):logic.index('static String pedalMapStatsJson')]
assert 'apDriveProfileTx' in can_a
assert 'apDriveProfileTxOk' in can_a and 'apDriveProfileTxFail' in can_a

# API exposes and updates the regen profile without requiring the enabled flag.
assert 'apDriveProfileRegenRaw' in api
assert 'server.hasArg("regen")' in api

# PedalMap select applies immediately; old Apply button is gone.
assert 'id="pedalMapApply"' not in html
assert "q('pedalMapSelect').onchange" in html or "q('pedalMapSelect'))q('pedalMapSelect').onchange" in html

# Performance button is a true session toggle with visible ON/OFF state.
assert 'pedalMapPerformanceState' in html
assert 'aria-pressed="false"' in html
assert 'setPedalMap(perfActive?\'stock\':\'performance\')' in html.replace(' ', '')

# Settings exposes the three AP regen choices as a production feature.
assert 'id="apDriveRegenSelect"' in html
assert 'value="20">STANDARD' in html
assert 'value="10">REDUCED' in html
assert 'value="1">MINIMAL · 1' in html

print('v3.6b7 AP Pedal / Regen Profile / PedalMap contract OK')
