"""Package Hygiene contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class PackageHygieneContracts(unittest.TestCase):

    def test_modes_removed(self):
        """Retained from test_modes_removed.py."""
        from pathlib import Path
        import re

        root = Path(__file__).resolve().parents[1]
        files = {
            name: (root / name).read_text(encoding='utf-8')
            for name in ['can_core.h', 'runtime_gate_pure.h', 't2can_forward.h', 'web_api.h', 'dashboard_source.html']
        }
        all_text = '\n'.join(files.values())

        # Removed production modes must not remain as selectable/runtime symbols.
        for symbol in ('MODE_D', 'MODE_E', 'MODE_F'):
            assert not re.search(rf'\b{symbol}\b', all_text), f'{symbol} still present'
        for phrase in ('Mode D', 'Mode E', 'Mode F', 'D/E/F'):
            assert phrase not in all_text, f'{phrase} still present'
        for element_id in ('modeD', 'modeE', 'modeF'):
            assert element_id not in files['dashboard_source.html'], f'{element_id} still in dashboard'

        # Preserve public/persisted IDs of A/B/C/H while leaving removed gaps unused.
        core = files['can_core.h']
        assert re.search(r'MODE_A\s*=\s*0', core)
        assert re.search(r'MODE_B\s*=\s*1', core)
        assert re.search(r'MODE_C\s*=\s*3', core)
        assert re.search(r'MODE_H\s*=\s*7', core)

        # Old persisted D/E/F IDs must fail safe to Mode A when loaded.
        assert 'nagModePersistedIdSupported' in core, 'missing persisted-mode sanitizer helper'

        # HTTP mode switching must expose only A/B/C/H presets.
        api = files['web_api.h']
        for fn in ('nagCfgDefaultsModeD', 'nagCfgDefaultsModeE', 'nagCfgDefaultsModeF'):
            assert fn not in api
        assert 'nagCfgDefaultsModeH' in api

        print('removed-mode contract OK')

    def test_removed_nag_controls(self):
        """Retained from test_v36b2_cleanup_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        files = {name: (root / name).read_text(encoding='utf-8') for name in [
            'vehicle_logic.h','can_runtime.h','can_core.h','t2can_forward.h',
            't2can_core_state.h','summon_state_pure.h','web_api.h','dashboard_source.html'
        ]}
        all_text = '\n'.join(files.values())

        retired = [
            'SUMMON_GATE_DROPOUT_GRACE_MS','summonGateGraceUntilMs','summonGateGraceEnterCount',
            'summonGateGraceRecoverCount','summonGateGraceExpireCount','summonGateGraceActiveLocked',
            'summonAuthorization','summonAuthorizationSinceMs','summonAuthorizationTransitions',
            'forceMode','summonInjectionGateOpen','gateBlockReason',
            'SUMMON_PRIORITY_FULL',
            'summonPriorityStateSinceMs','summonPriorityTransitions','summonPriorityFullEnterCount',
            'summonPriorityFullExitCount','summonPriorityFullInactiveSinceMs',
            'SUMMON_PRIORITY_FULL_EXIT_GRACE_MS','twaiStandbyShed','twaiFullShed',
            'twaiSummonQueueFlush','twaiSummonRetryOk','twaiSummonRetryFail',
            'canTxTwaiTransmitWithMaskWait','canTxTwaiClearQueueWithMask','canTxTwaiClearQueue',
            'priorityGear280State','priorityGear390State','priorityGear280Raw','priorityGear390Raw',
            'priorityGear280Ms','priorityGear390Ms','injectSummon(',
            'summonAuthorizationPure','summonRequiredTxFreshMaskPure',
            'summonRemoteFallbackAllowedPure','summonParkEntryAllowedPure',
            'summonV26CompatInitialPure','manualDasStatePure','canTxBarrierInvalidatePure',
        ]
        for symbol in retired:
            assert symbol not in all_text, f'retired v3.6b1 symbol remains: {symbol}'

        # New names/structure must be present.
        logic = files['vehicle_logic.h']
        for symbol in ('gear118State','gear186State','gear118Raw','gear186Raw',
                       'r79ManualSuppression','injectUlcSnooze3fdMux1'):
            assert symbol in logic, f'missing v3.6b2 cleanup symbol: {symbol}'
        assert 'static bool summonLoadSheddingActive()' in logic
        assert 'gateSummoning' in logic

        # Snapshot is read-only: it must not refresh or mutate derived state.
        start = logic.index('static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now) {')
        end = logic.index('static void refreshSummonState', start)
        snapshot = logic[start:end]
        assert 'refreshSummonDerivedStateLocked' not in snapshot
        assert 'r79ManualSuppressionUpdate' not in snapshot

        # Old dashboard concepts should be gone.
        html = files['dashboard_source.html']
        for phrase in ('SUMMON_FULL','Gate grace','Authorization'):
            assert phrase not in html, f'legacy dashboard concept remains: {phrase}'

        print('v3.6b2 cleanup contract OK (d1 transport priority exceptions acknowledged)')

    def test_removed_dashboard_compatibility(self):
        """Retained from test_v36b5_cleanup_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
        capture = (root / 'can_research_capture.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')

        def has_json_key(text: str, key: str) -> bool:
            return f'\\"{key}\\"' in text or f'"{key}"' in text

        # Removed state was write-only / unused: no live consumer existed in v3.6b4.
        retired = [
            'YL_REGEN_STANDARD_RAW',
            'dasAutoLaneChangePrevState','dasAutoLaneChangeLastChangeMs','dasAutoLaneChangeChangeCount',
            'last249Ms','seen3C2','rx3C2','last3C2Mux1Ms','real3C2LeftButton','real3C2RightButton',
            'stalklessTxOk','stalklessTxFail','doorButtonRx','doorCancelAccepted','doorCancelBlocked',
        ]
        combined = logic + '\n' + runtime
        for symbol in retired:
            assert symbol not in combined, f'dead/write-only symbol remains: {symbol}'
        assert 'researchCaptureRawFirstFrameMs' not in capture

        # HOME fast snapshot: only current embedded HOME rendering contract remains.
        start = api.index('static String homeFastSnapshotToJson() {')
        end = api.index('static String homeSlowSnapshotToJson() {', start)
        home_fast = api[start:end]
        for key in (
            'sessionActive','loadSheddingActive','parked','aca','spr','txQueueNow','txQueueMax',
            'txEnabled','manualSuppressed','gearRaw','summonSessionActive','remoteStartupEvidence','manualLatchActive',
        ):
            assert not has_json_key(home_fast, key), f'home-fast still emits unused key {key}'
        for key in (
            'torque','stoppedGate','apActive','canAState','enabled','noaActive','dasStateValid','dasState',
            'canState','canStateName','mcpTrafficSeen','mcpTrafficOnline','mcpTrafficAgeMs',
            'twaiTrafficSeen','twaiTrafficOnline','twaiTrafficAgeMs',
            'txState','txReason','gearName','txOk','txFail','lastTxValid','lastTxAgeMs',
        ):
            assert has_json_key(home_fast, key), f'home-fast lost required key {key}'

        # Blink panel reuses its existing 800ms request for 0x24A RX. Full DAS API stays available.
        blink_start = api.index('static String blinkAStatsToJson() {')
        blink_end = api.index('static String dasTelemetryStatsToJson() {', blink_start)
        blink_json = api[blink_start:blink_end]
        assert has_json_key(blink_json, 'visualDebugRx')
        assert 'server.on("/api/das/stats"' in api, 'full diagnostic DAS endpoint must remain'
        assert 'async function fetchDas' not in html
        assert 'async function fetchCanTraffic' not in html
        assert has_json_key(blink_json, 'visualDebugRx')
        assert "pollDue('das'" not in html
        assert "else if(p==='panelBlink')pollDue('blink',800,fetchBlink,now,force);" in html

        # D/E/F are retired; all supported modes retain the same AP-driven HOME ACTIVE state.
        home_render_start = html.index('function renderHomeFast(s){')
        home_render_end = html.index('function renderHomeSlow(s){', home_render_start)
        home_render = html[home_render_start:home_render_end]
        assert '[4,5,6]' not in home_render
        assert 'nagActive=!!n.apActive' in home_render

        print('v3.6b5 zero-behavior cleanup contract OK')

    def test_removed_compatibility_and_archives(self):
        """Retained from test_v36c3_cleanup_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')

        # c3 is a conservative cleanup-only release.

        # Confirmed dead production symbols must stay removed.
        checks = [
            ('vehicle_profile.h', [
                'vehicleProfileShortName',
                'const char *shortName;',
            ]),
            ('summon_state_pure.h', [
                'summonBusMaskNamePure',
            ]),
            ('mode_h_pure.h', [
                'nagHumanV1DefaultConfigPure',
                'nagHumanV1SanitizePeakRangePure',
            ]),
            ('driver_monitor_capture.h', [
                'DRIVER_MONITOR_BUS_PARTY',
                'DRIVER_MONITOR_BUS_VH',
            ]),
            ('driver_monitor_capture_pure.h', [
                'BUS_PARTY = BUS_A',
                'BUS_VH = BUS_B',
            ]),
            ('ulc_compositor_pure.h', [
                'UI_ULC_SPEED_DISABLED_PURE',
                'UI_ULC_SPEED_MILD_PURE',
                'UI_ULC_SPEED_AVERAGE_PURE',
                'UI_ULC_SPEED_MAD_MAX_PURE',
            ]),
            ('auto_lane_change_enable_pure.h', [
                'UI_AUTO_LANE_CHANGE_OFF_PURE',
            ]),
            ('ulc_compositor_pure.h', [
                'ulcNoConfirmEffectiveBusPure',
                'ulcNoConfirmGateOpenPure',
            ]),
        ]
        for rel, banned in checks:
            text = (root / rel).read_text(encoding='utf-8')
            for token in banned:
                assert token not in text, f'dead compatibility/dead symbol remains in {rel}: {token}'

        # Active tests must use current APIs rather than keeping production shims alive.
        import inspect
        own_check = inspect.getsource(type(self).test_removed_compatibility_and_archives)
        active_tests = '\n'.join(
            p.read_text(encoding='utf-8').replace(own_check, '')
            for p in sorted((root / 'tests').iterdir())
            if p.is_file()
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
            'test_retired_speed_routes_host.py',
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

    def test_removed_experimental_modes(self):
        """Retained from test_v373_cleanup_contract.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        logic = (root / "vehicle_logic.h").read_text()
        runtime = (root / "can_runtime.h").read_text()
        api = (root / "web_api.h").read_text()
        forward = (root / "t2can_forward.h").read_text()
        dashboard = (root / "dashboard_source.html").read_text()
        firmware = logic + runtime + api + forward
        all_source = firmware + "".join(
            path.read_text()
            for path in (
                root / "s3xy_ble.h",
                root / "fixed_point_pure.h",
                root / "text_format_arduino.h",
                root / "feature_config_migration_pure.h",
                root / "mode_h_pure.h",
                root / "tests/fixtures/mode_h_legacy/nag_human_v2_pure.h",
                root / "runtime_gate_pure.h",
                root / "ulc_compositor_pure.h",
                root / "ulc_compositor_pure.h",
            )
        )

        for retired in (
            "r79LabObserve7ff",
            "r79Lab7ffSeen",
            "r79Lab7ffBus",
            "r79Lab7ffPage",
            "r79Lab7ffRx",
            "r79Lab7ffLastMs",
            "r79Lab7ffRaw",
            "r79LabBit18Rx0",
            "r79LabBit18Rx1",
            "r79LabBit19Rx0",
            "r79LabBit19Rx1",
            "r79LabBit47Rx0",
            "r79LabBit47Rx1",
            "r79LabBit18Changes",
            "r79LabBit19Changes",
            "r79LabBit47Changes",
            "r79LabStockApply",
            "r79LabStockSmart",
            "r79LabStockHardCore",
            "r79LabEffectiveApply",
            "r79LabEffectiveSmart",
            "r79LabEffectiveHardCore",
            "r79LabLastTxKind",
            "r79LabLast3fdMs",
            "r79LabLastEffectiveRaw",
            "r79LabLastAttemptMs",
            "r79LabLastPeriodicRequestMs",
            "r79LabLastPeriodicTxMs",
            "r79LabNoTemplateSkip",
            "r79LabManualSuspendSkip",
            "r79LabCanOfflineSkip",
            "r79LabAdminHoldSkip",
            "r79LabAppliedFrames",
            "r79FastEchoBlocked",
            "r79FastEchoLatency",
            "r79FastEchoLt100Us",
            "r79FastEchoLt250Us",
            "r79FastEchoLt1000Us",
            "r79FastEchoGe1000Us",
            "r79FastSuccess",
            "r79LabLastBlockReason",
            "r79LabLastBlockMs",
            "r79QuietCancelCount",
            "r79QuietDueCount",
            "r79QuietDisallowedSkip",
            "r79PeriodicRetryCancelledByStock",
            "r79PeriodicSlotDue",
            "r79PeriodicSlotFire",
            "r79PeriodicSlotGuard",
        ):
            assert retired not in firmware, f"dead R79 telemetry remains: {retired}"

        for retained in (
            "r79DmsOnlyTxOk",
            "r79DmsOnlyTxFail",
            "r79DmsOnlyBlockedByR79",
            "r79LabBit43Rx0",
            "r79LabBit43Rx1",
            "r79LabBit43Changes",
            "r79LabStockCabinCamera",
            "r79LabEffectiveCabinCamera",
            "r79LabStockValid",
            "r79LabLastTxValid",
            "r79Lab3fdRx",
            "r79LabTxOk",
            "r79LabTxFail",
            "r79FastEchoAttempts",
            "r79FastEchoTxOk",
            "r79FastEchoTxFail",
            "r79LabPeriodicTxOk",
            "r79LabPeriodicTxFail",
            "r79QuietArmCount",
            "r79QuietFireCount",
            "r79QuietGuardSkip",
        ):
            assert retained in logic, f"required R79 state removed: {retained}"

        for key in (
            "bit43Rx0",
            "bit43Rx1",
            "bit43Changes",
            "stockMux1Rx",
            "txOk",
            "txFail",
            "fastAttempts",
            "fastTxOk",
            "fastTxFail",
            "periodicTxOk",
            "periodicTxFail",
            "quietArm",
            "quietFire",
            "quietGuardSkip",
        ):
            assert f'"{key}"' in api, f"required R79 API key removed: {key}"

        for missing_id in (
            "labAcc",
            "labNagHumanDesc",
            "labUlcOffBlocked",
            "labUlcOffLast",
            "labUlcPolicyGate",
            "labUlcPolicyRxAge",
            "retiredConfirmBus",
            "retiredUlcSpeed",
        ):
            assert f"$('" + missing_id + "')" not in dashboard
            assert f'q("{missing_id}")' not in dashboard

        for retired in (
            "r79LabPeriodValid",
            "r79PeriodicObserveStock",
            "r79PreObserveStock",
            "r79LabStatsToJson",
            "httpR79LabStats",
            "httpR79LabUpdate",
            "httpR79LabStock",
            "r79LabTxKindName",
            "s3xyParseAction",
            "LAB3F8_ALC_FORCE_OFF",
            "deciString",
            "r79SmartOverrideActivePure",
            "formatDeciPure",
            "FeatureConfigSchema3CommitPure",
            "featureConfigSchema3CommitPure",
            "NAG_HUMAN_V1_B19_DEFAULT_MIN_RAW",
            "NAG_HUMAN_V1_B19_DEFAULT_MAX_RAW",
            "nagHumanV1MigrateB19DefaultToRev1PlusPure",
            "nagHumanV2ConfigValidPure",
            "uiUlcOffHighwayApplyRawPure",
            "ulcNoConfirmApplyByte0Pure",
        ):
            assert retired not in all_source, f"dead helper remains: {retired}"

        assert "stalkFramePreparePure" in logic
        assert logic.count("stalkFramePreparePure") == 2
        assert api.count("canTxTraceRawHex") == 3
        assert "writeProfileCapabilityJson" in api
        assert api.count("writeNagHumanV1ConfigJson") == 2  # definition + sole Mode H engine
        assert api.count("writeNagHumanV1StateJson") == 2  # definition + sole Mode H engine

        print("v3.7.3 dead telemetry and stale dashboard cleanup contract: PASS")

    def test_can_shared_ownership(self):
        """Retained from test_can_shared_ownership_contract.py."""
        from pathlib import Path
        import unittest
        ROOT=Path(__file__).resolve().parents[1]
        class SharedOwnershipContract(unittest.TestCase):
         def test_duplicate_flows_have_one_owner(self):
          vehicle=(ROOT/'vehicle_logic.h').read_text()
          core=(ROOT/'can_core.h').read_text()
          self.assertIn('tsl9InputServiceRoute',vehicle)
          self.assertIn('tsl9InputObserveRoute',vehicle)
          self.assertIn('nagTsl9TransformStock',core)
          self.assertTrue((ROOT/'can_input_arbitration.h').exists())
          self.assertTrue((ROOT/'tesla_can_fields_pure.h').exists())
        run_contract_cases(self, [SharedOwnershipContract], expected=1)

    def test_retired_controls_runtime(self):
        """Retained from test_retired_controls_runtime_contract.py."""
        from pathlib import Path
        ROOT = Path(__file__).resolve().parents[1]
        owned = ['vehicle_logic.h', 'can_core.h', 'can_runtime.h', 't2can_core_state.h', 't2can_forward.h']
        text = '\n'.join((ROOT / name).read_text() for name in owned)
        for token in ('laneGraph', 'LaneGraph', 'parkedInjection', 'ParkedInjection', 'can3fdTiming', 'lab3f8LastTx', 'canARxSavedMode', 'canARxModePersist', 'CAN_A_RX_PREFETCH_4'):
            assert token not in text, f'retired runtime control remains: {token}'
        for name in ('lane_graph_pure.h', 'parked_injection_pure.h', 'can3fd_timing_pure.h'):
            assert not (ROOT / name).exists(), f'retired resource remains: {name}'
        logic = (ROOT / 'vehicle_logic.h').read_text()
        assert '"canARxMode"' not in logic, 'legacy saved settings must not control receive scheduling'
        runtime = (ROOT / 'can_runtime.h').read_text()
        assert 'MCP_PREFETCH_CAPACITY = canARxReadBatchBudgetPure()' in runtime
        assert 'canARxReadBatchBudgetPure()' in runtime
        # Normal BUS-OFF diagnostics and policy counters are independent of retired panels.
        for token in ('canTwaiErrorAlertWindow', 'canBTraceFreezeBusOff', 'canBusOffPersistenceMarkDirty'):
            assert token in text, token
        for token in ('lab3f8UlcBlindMode', 'uiUlcBlindSpotConfig', 'lab3f8TxOk', 'lab3f8TxFail'):
            assert token in logic, token

        rx_policy = (Path(__file__).resolve().parents[1] / "runtime_gate_pure.h").read_text()
        assert "CAN_A_RX_IMMEDIATE = 1u" in rx_policy
        assert "return CAN_A_RX_IMMEDIATE;" in rx_policy

    def test_fixed_r79_heartbeat_viewport(self):
        """Retained from test_v36d7_contract_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text()
        vehicle = (root / 'vehicle_logic.h').read_text()
        runtime = (root / 'can_runtime.h').read_text()
        core = (root / 'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        api = (root / 'web_api.h').read_text()
        dash = (root / 'dashboard_source.html').read_text()
        fixed = (root / 'r79_fixed_policy_pure.h').read_text()

        assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
        assert 'r79FixedQuietStepPure' in vehicle and 'r79FixedObserveStock' in vehicle
        dispatch = runtime.index('r79ProcessStockFrame(f, timingMux, r79FrameNowMs)')
        tick = runtime.index('r79TransportTick();')
        assert dispatch < tick
        for token in ('canTaskHeartbeatLastCause', 'canTaskHeartbeatLastAgeAms',
                      'canTaskHeartbeatLastAgeBms', 'canTaskHeartbeatTimeoutCountA',
                      'canTaskHeartbeatTimeoutCountB', 'canTaskHeartbeatTimeoutCountBoth'):
            assert token in core and token in api
        for key in ('quietArm', 'quietFire', 'quietGuardSkip'):
            assert f'"{key}"' in api
        for dom in ('r79FixedQuietValue', 'sysHeartbeatCause', 'sysHeartbeatAge',
                    'sysHeartbeatCounts'):
            assert dom in dash
        assert 'overflow-anchor:none' in dash.replace(' ', '')
        assert 'min-height:100svh' in dash.replace(' ', '')
        assert 'visualViewport' in dash and 'restoreViewportScroll' in dash
        print('v3.7 fixed R79 / heartbeat preservation contract: PASS')


    def test_consolidation_manifest_preserves_callables(self):
        """Historical callables must remain discoverable; new tests are welcome."""
        import ast
        import re
        from collections import defaultdict
        from pathlib import Path

        tests = Path(__file__).resolve().parent
        manifest = (tests / "COVERAGE.md").read_text()
        section = manifest.split("<!-- PYTHON-MAPPING-START -->", 1)[1].split(
            "<!-- PYTHON-MAPPING-END -->", 1
        )[0]
        rows = re.findall(
            r"^\| `([^`]+)` \| `([^`]+)` \| `([^`]+)` \| (\d+) \| (\d+) \| (\d+) \| (\d+) \| (\d+) \|$",
            section, re.M,
        )
        self.assertGreaterEqual(len(rows), 106)
        self.assertEqual(len({row[0] for row in rows}), len(rows))
        expected_methods = defaultdict(set)
        for original, filename, callable_name, _, _, _, _, nested_count in rows:
            tree = ast.parse((tests / filename).read_text())
            class_name, method_name = callable_name.split(".")
            classes = [n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == class_name]
            self.assertEqual(len(classes), 1, original)
            methods = [n for n in classes[0].body if isinstance(n, ast.FunctionDef) and n.name == method_name]
            self.assertEqual(len(methods), 1, original)
            if int(nested_count):
                runners = [n for n in ast.walk(methods[0]) if isinstance(n, ast.Call)
                           and isinstance(n.func, ast.Name) and n.func.id == "run_contract_cases"]
                self.assertEqual(len(runners), 1, original)
                expected = next(k.value.value for k in runners[0].keywords if k.arg == "expected")
                self.assertGreaterEqual(expected, int(nested_count), original)
            expected_methods[(filename, class_name)].add(method_name)
        for (filename, class_name), names in expected_methods.items():
            tree = ast.parse((tests / filename).read_text())
            cls = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == class_name)
            actual = {n.name for n in cls.body if isinstance(n, ast.FunctionDef) and n.name.startswith("test_")}
            self.assertTrue(names <= actual, filename)


if __name__ == "__main__":
    unittest.main()
