"""Drive Profile contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class DriveProfileContracts(unittest.TestCase):

    def test_pedalmap(self):
        """Retained from test_v36b4_pedalmap_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        ble = (root / 's3xy_ble.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')


        # Volatile session control must expose direct STOCK/CHILL/SPORT/PERFORMANCE
        # selection, plus a dedicated S3XY Performance action.
        for token in ('PEDAL_MAP_RAW_STOCK', 'requestPedalMapMode', 'pedalMapClearSessionOnParkTransition'):
            assert token in logic, f'missing runtime session symbol {token}'
        assert 'S3XY_ACTION_PERFORMANCE_MODE' in ble
        assert 'performance_mode' in ble and 'performance_mode' in html
        assert 'Acceleration Mode Toggle' in ble and 'accel_mode_toggle' in html

        # LAB direct selection + dedicated Performance button.
        for token in ('Pedal Response', 'pedalMapSelect', 'pedalMapPerformance'):
            assert token in html, f'missing LAB PedalMap UI {token}'
        for mode in ('STOCK', 'CHILL', 'SPORT', 'PERFORMANCE'):
            assert mode in html

        # Session control is runtime-only: API must not persist any pedal-map mode.
        assert '/api/pedalmap/set' in api
        pedal_api_start = api.index('static void httpPedalMapSet')
        pedal_api_end = api.index('static String nagCfgToJson', pedal_api_start)
        pedal_api = api[pedal_api_start:pedal_api_end]
        for forbidden in ('Preferences p;', '.putUChar(', '.putInt(', '.putBool(', '.putString('):
            assert forbidden not in pedal_api, f'PedalMap session API must not write NVS: {forbidden}'

        # A real transition into Park from both supported gear paths must clear session.
        handle118 = logic[logic.index('static void handle280'):logic.index('static void handle390')]
        handle186 = logic[logic.index('static void handle390'):logic.index('static void handle921')]
        assert 'pedalMapClearSessionOnParkTransition' in handle118
        assert 'pedalMapClearSessionOnParkTransition' in handle186
        assert 'prevGs' in handle118 and 'prevGs' in handle186

        # AP Pedal / Regen Profile may temporarily own the outgoing 0x334, but it must not
        # destroy a manual drive-session target; Performance resumes when AP exits.
        obs = logic[logic.index('static bool pedalMapObserveAndPrepare'):logic.index('// Model Y L: 0x334', logic.index('static bool pedalMapObserveAndPrepare'))]
        assert 'pedalMapSessionClearPure' not in obs, 'AP ownership must not erase manual PedalMap session'

        print('v3.6b4 PedalMap session contract OK')

    def test_drive_profile(self):
        """Retained from test_v36b7_drive_profile_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        profile = (root / 'vehicle_profile.h').read_text(encoding='utf-8')
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')

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
            assert token in logic or (root / 'pedal_map_session_pure.h').exists(), token
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

    def test_ap_drive_dashboard_scope(self):
        """Retained from test_v36b19_ap_drive_dashboard_scope.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        dash = (root / 'dashboard_source.html').read_text(errors='ignore')

        # b18 root cause: setApDriveRegen lived in the first/global script but called
        # applyFeatures()/refreshFeatures(), which are local to the later profile IIFE.
        # A successful backend POST therefore fell into the catch path with
        # ReferenceError: applyFeatures is not defined and showed the generic failure.
        iife_marker = '<script>(() => {'
        iife_start = dash.index(iife_marker, dash.index('<script>\nconst $='))
        global_script = dash[dash.index('<script>\nconst $='):iife_start]
        iife_script = dash[iife_start:dash.index('</script>', iife_start)]

        assert 'async function setApDriveRegen' not in global_script, \
            'AP Drive regen handler must not live outside applyFeatures scope'
        assert 'function applyFeatures' in iife_script
        assert 'async function refreshFeatures' in iife_script
        assert 'async function setApDriveRegen' in iife_script
        assert "q('apDriveRegenSelect').onchange=e=>setApDriveRegen(e.target.value)" in iife_script
        assert 'featurePostJson' in iife_script or 'featurePostJson' in global_script
        print('v3.6b19 AP Drive dashboard scope regression passed')

    def test_drive_and_lane_feature_controls(self):
        """Retained from test_v36b8_features_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        profile = (root / 'vehicle_profile.h').read_text(encoding='utf-8')
        forward = (root / 't2can_forward.h').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')

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
        save = logic[logic.index('static bool ulcCfgPersistCandidate('):logic.index('static bool ulcCfgSave()')]
        assert 'getBool("confirm", false)' in load
        assert 'activeProfileUlcNoConfirmSupported() && confirm' in load
        assert '(uint8_t)config.ulc.confirmFreeEnabled' in save
        assert 'nvsCheckedWriteBlob("ulc", "cfg3282"' in save

        api_fn = api[api.index('static void httpUlcUpdate()'):api.index('static void httpAutoLaneChangeLabStats()')]
        assert 'ulcCfgPersistCandidate(next)' in api_fn
        assert 'httpRequireLab()' not in api_fn

        # Turning the LAB master OFF must not alter the production selection.
        feature_lab = api[api.index('static void httpFeatureLab()'):api.index('static void httpFeatureDoorCancel()')]
        assert 'ulcNoConfirmEnabled = false' not in feature_lab
        snap = logic[logic.index('static bool ulcNoConfirmEnabledSnapshot()'):logic.index('static bool ulcNoConfirmGateOpen() {')]
        assert 'labMenuEnabled' not in snap and 'activeProfileUlcNoConfirmSupported()' in snap

        print('v3.6b8 immediate PedalMap + production Confirm-Free contract OK')

    def test_ap_aggressiveness_removed(self):
        """Retained from test_v392_ap_aggressiveness_removed_static.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        ino_files = list(root.glob("*.ino"))

        production_files = [
            ino_files[0],
            root / "can_core.h",
            root / "can_runtime.h",
            root / "vehicle_logic.h",
            root / "web_api.h",
            root / "dashboard_source.html",
        ]
        production = "\n".join(path.read_text(encoding="utf-8") for path in production_files)
        dashboard = (root / "dashboard_source.html").read_text(encoding="utf-8")

        # Removal contract: the failed AP Aggressiveness experiment must not leave any
        # runtime, API, persistence, trace, or dashboard integration behind.
        assert not (root / "ap_aggressiveness_pure.h").exists()
        assert not (root / "tests" / "test_ap_aggressiveness_pure.cpp").exists()

        for marker in (
            "apAgg",
            "AP_AGG",
            "AP Aggressiveness",
            "AP_AGGRESSIVENESS",
            "/api/lab/ap-aggressiveness/",
            '"agEn"',
            '"agTx"',
            '"agAuto"',
            '"agHw"',
            '"agProf"',
        ):
            assert marker not in production, marker

        # The independent, existing 0x334 AP pedal/regen feature remains supported.
        assert "AP Accel / Regen" in dashboard

        print("PASS v3.29.1 AP Aggressiveness removal contract")


if __name__ == "__main__":
    unittest.main()
