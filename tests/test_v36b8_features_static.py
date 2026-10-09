from pathlib import Path

root = Path(__file__).resolve().parents[1]
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
profile = (root / 'vehicle_profile.h').read_text(encoding='utf-8')
forward = (root / 't2can_forward.h').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')
assert '#define FW_VERSION "v3.28.0"' in ino

# PedalMap mode changes cache the full latest stock 0x334 template and attempt
# one immediate one-shot TX, while preserving the existing stock-follow overlay.
assert 'pedalMapStockData[8]' in logic
assert 'pedalMapStockDataValid' in logic
assert 'pedalMapTransmitImmediateFromCache' in logic
assert 'pedalMapImmediateTxOk' in logic and 'pedalMapImmediateTxFail' in logic
assert 'pedalMapFollowTxOk' in logic and 'pedalMapFollowTxFail' in logic
req = logic[logic.index('static bool requestPedalMapMode'):logic.index('static void requestPedalMapToggleFromButton')]
assert req.count('pedalMapTransmitImmediateFromCache') >= 2  # STOCK release + active target
observe = logic[logic.index('static bool pedalMapObserveAndPrepare'):logic.index('static void handlePedalMap334OnCanB')]
assert 'memcpy(pedalMapStockData, srcData, 8)' in observe

# Production UI_ulcStalkConfirm control. The user setting is persistent in the
# production ULC namespace and runtime injection remains profile-gated.
assert 'vehicleProfileUlcNoConfirmSupported' in profile
profile_fn = profile[profile.index('static inline bool vehicleProfileUlcNoConfirmSupported'):profile.index('// On Standard Party+Chassis', profile.index('static inline bool vehicleProfileUlcNoConfirmSupported'))]
assert 'vehicleProfileTopologyValid(id, topology)' in profile_fn
assert 'vehicleProfileCanAIsParty' not in profile_fn
assert 'activeProfileUlcNoConfirmSupported' in profile
assert 'ulcNoConfirmEnabled' in logic
assert 'ulcNoConfirmGateOpen' in logic
assert 'ulcCompose3f8Pure' in logic
assert 'UI_ulcStalkConfirm' in logic
assert 'stalkConfirm' in api
assert '/api/ulc/update' in api

# Auto Blinker fire path must be suppressed while the no-confirm experiment is
# actively selected, avoiding simultaneous fake-stalk and 0x3F8 policy input.
assert 'ulcNoConfirmEnabledSnapshot' in logic
blink_tick = logic[logic.index('static void blinkATxTick()'):logic.index('// Legacy 0x3F8 ULC injection')]
assert 'ulcNoConfirmEnabledSnapshot()' in blink_tick


# Defensive race guards also block the final physical fake-stalk TX paths.
for fn, end in [('static void sendStalkFrameCanB', 'static void handle249OnCanA'),
                ('static void sendStalkFrameCanA', 'static void handle3C2OnCanA'),
                ('static void handle3C2OnCanA', 'static void handle102LaneChangeCancel')]:
    block = logic[logic.index(fn):logic.index(end, logic.index(fn))]
    assert 'ulcNoConfirmEnabledSnapshot()' in block, fn

# The production preference must persist in the ULC NVS namespace. A saved ON
# value is restored only when the active topology supports the feature;
# unsupported profiles must not erase the stored preference.
load = logic[logic.index('static bool ulcCfgLoadAndMigrate()'):logic.index('static bool autoLaneChangeLabCfgLoadAndMigrate()')]
save = logic[logic.index('static bool ulcCfgSave()'):logic.index('static bool autoLaneChangeLabCfgSave()')]
assert 'getBool("confirm", false)' in load
assert 'activeProfileUlcNoConfirmSupported() && confirm' in load
assert 'putBool("confirm"' in save

api_fn = api[api.index('static void httpUlcUpdate()'):api.index('static void httpAutoLaneChangeLabStats()')]
assert 'ulcCfgSave()' in api_fn
assert 'httpRequireLab()' not in api_fn

# Turning the LAB master OFF must not alter the production selection.
feature_lab = api[api.index('static void httpFeatureLab()'):api.index('static void httpFeatureDoorCancel()')]
assert 'ulcNoConfirmEnabled = false' not in feature_lab
snap = logic[logic.index('static bool ulcNoConfirmEnabledSnapshot()'):logic.index('static bool ulcNoConfirmGateOpen() {')]
assert 'labMenuEnabled' not in snap and 'activeProfileUlcNoConfirmSupported()' in snap

print('v3.6b8 immediate PedalMap + production Confirm-Free contract OK')
