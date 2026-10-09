from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')

# c3 is a conservative cleanup-only release.
assert '#define FW_VERSION "v3.28.0"' in ino

# Confirmed dead production symbols must stay removed.
checks = {
    'vehicle_profile.h': [
        'vehicleProfileShortName',
        'const char *shortName;',
    ],
    'summon_state_pure.h': [
        'summonBusMaskNamePure',
    ],
    'nag_human_v1_pure.h': [
        'nagHumanV1DefaultConfigPure',
        'nagHumanV1SanitizePeakRangePure',
    ],
    'driver_monitor_capture.h': [
        'DRIVER_MONITOR_BUS_PARTY',
        'DRIVER_MONITOR_BUS_VH',
    ],
    'driver_monitor_capture_pure.h': [
        'BUS_PARTY = BUS_A',
        'BUS_VH = BUS_B',
    ],
    'ulc_policy_pure.h': [
        'UI_ULC_SPEED_DISABLED_PURE',
        'UI_ULC_SPEED_MILD_PURE',
        'UI_ULC_SPEED_AVERAGE_PURE',
        'UI_ULC_SPEED_MAD_MAX_PURE',
    ],
    'auto_lane_change_enable_pure.h': [
        'UI_AUTO_LANE_CHANGE_OFF_PURE',
    ],
    'ulc_stalk_confirm_pure.h': [
        'ulcNoConfirmEffectiveBusPure',
        'ulcNoConfirmGateOpenPure',
    ],
}
for rel, banned in checks.items():
    text = (root / rel).read_text(encoding='utf-8')
    for token in banned:
        assert token not in text, f'dead compatibility/dead symbol remains in {rel}: {token}'

# Active tests must use current APIs rather than keeping production shims alive.
active_tests = '\n'.join(
    p.read_text(encoding='utf-8')
    for p in sorted((root / 'tests').iterdir())
    if p.is_file() and p.name != 'test_v36c3_cleanup_static.py'
)
for token in (
    'ulcNoConfirmEffectiveBusPure',
    'ulcNoConfirmGateOpenPure',
    'nagHumanV1DefaultConfigPure',
):
    assert token not in active_tests, f'active test still depends on retired compatibility helper: {token}'

# Historical source backups / retired tests should not ship in the source package.
assert not (root / 'backup').exists(), 'historical backup directory still ships'
def retired_test_artifact(name):
    # Archive location/markers describe obsolete copies; a test's subject
    # (for example retired-feature prevention) does not describe its status.
    return (name == 'retired' or name.startswith('retired_') or
            '.retired.' in name or name.endswith(('.retired', '.bak', '.old')))

# Active removal regressions are not archived obsolete test copies.
for name in (
    'test_v37_r79_retired_absent.py',
    'test_dashboard_v3200_retired_speed.js',
    'test_retired_speed_runtime_host.py',
    'test_retired_speed_compositor_pure.cpp',
    'test_v320_retired_speed_routes_host.py',
):
    assert not retired_test_artifact(name), f'active removal regression misclassified: {name}'
for name in ('retired', 'retired_test_ulc.cpp', 'test_ulc.cpp.retired', 'test_ulc.retired.py', 'test_ulc.cpp.bak', 'test_ulc.cpp.old'):
    assert retired_test_artifact(name), f'archived test artifact allowed: {name}'

retired = [
    p for p in (root / 'tests').iterdir()
    if retired_test_artifact(p.name)
]
assert not retired, f'retired tests still ship: {[p.name for p in retired]}'

print('v3.6d9a2 conservative cleanup contract OK')
