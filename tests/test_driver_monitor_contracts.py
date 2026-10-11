"""Driver Monitor contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class DriverMonitorContracts(unittest.TestCase):

    def test_driver_monitor(self):
        """Retained from test_v36b9_driver_monitor_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')
        assert '#include "driver_monitor_capture_pure.h"' in ino
        assert '#include "driver_monitor_capture.h"' in ino

        pure = (root / 'driver_monitor_capture_pure.h').read_text(encoding='utf-8')
        cap = (root / 'driver_monitor_capture.h').read_text(encoding='utf-8')
        runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

        # Universal fixed-window, read-only driver-monitor recorder.
        for token in ['DRIVER_MONITOR_PRE_MS = 2000', 'DRIVER_MONITOR_POST_MS = 5000',
                      'DRIVER_MONITOR_ID_STATUS2 = 0x389', 'DRIVER_MONITOR_ID_CARLOG = 0x5D9',
                      'DRIVER_MONITOR_ID_AUTOPILOT_DEBUG = 0x247']:
            assert token in cap, token
        assert 'activeProfileIsYl()' not in cap
        assert 'canTx' not in cap and 'twai_transmit' not in cap and 'Can_A.sendMessage' not in cap

        # Bus routing: both physical CAN A/B feed the universal observer.
        assert 'driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_A' in runtime
        assert 'driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_B' in runtime
        assert 'driverMonitorCaptureTick' in runtime

        # A-F fixed experiment labels.
        for label in ['FRONT · NORMAL', 'SCREEN GLANCE', 'SIDE LOOK', 'LOOK DOWN',
                      'TORQUE + LOOK AWAY', 'FRONT · NO TORQUE']:
            assert label in html, label
        for slot in 'ABCDEF':
            assert f'id="driverMonCap{slot}"' in html

        # LAB endpoints and CSV export.
        for route in ['/api/drivermonitor/stats', '/api/drivermonitor/start',
                      '/api/drivermonitor/reset', '/api/drivermonitor/log.csv']:
            assert route in api, route
        dm_api = api[api.index('// ─── Universal Driver Monitoring Capture'):api.index('static void httpSystemStats()')]
        assert 'activeProfileIsYl()' not in dm_api
        assert 'Driver_Monitoring_Capture.csv' in html
        assert 'Driver Monitoring Capture · YL ONLY' not in html
        assert 'ALL VEHICLES' in html
        assert 'READ ONLY' in html

        # 0x389 interaction decoding is explicit and CSV carries raw + decoded state.
        for token in ['driverMonitorInteractionLevelPure', 'DRIVER_INTERACTING',
                      'DRIVER_NOT_INTERACTING', 'CONTINUED_DRIVER_NOT_INTERACTING']:
            assert token in pure + cap

        print('Driver Monitoring Capture universal regression contract OK')

    def test_driver_monitor_all_profiles(self):
        """Retained from test_v36b21_driver_monitor_all_profiles_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
        cap = (root / 'driver_monitor_capture.h').read_text(encoding='utf-8')
        pure = (root / 'driver_monitor_capture_pure.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

        assert 'driverMonitorCaptureInit();' in ino
        assert 'if (activeProfileIsYl()) driverMonitorCaptureInit();' not in ino

        # Driver Monitoring is universal/read-only: no profile gate in capture core or API block.
        assert 'activeProfileIsYl()' not in cap
        api_dm = api[api.index('// ─── Universal Driver Monitoring Capture'):api.index('static void httpSystemStats()')]
        assert 'activeProfileIsYl()' not in api_dm
        assert 'available only on Model Y L' not in api_dm

        # UI must not advertise YL-only support.
        assert 'Driver Monitoring Capture · YL ONLY' not in html
        assert 'YL ONLY · cabin/DMS CAN research' not in html
        assert 'Driver Monitoring Capture' in html
        assert 'ALL VEHICLES' in html
        assert 'READ ONLY' in html

        # CSV carries physical CAN side plus current profile bus-role name.
        assert 'physical_bus,bus_role' in api
        assert 'driverMonitorPhysicalBusName' in cap
        assert 'driverMonitorProfileBusName' in cap

        # Both physical buses must accept all research IDs.
        for token in ['BUS_A', 'BUS_B', '0x389', '0x5D9', '0x247', '0x399', '0x370']:
            assert token in pure

        print('v3.6d9a2 Driver Monitoring all-profile contract OK')

    def test_dms_nag_lab(self):
        """Retained from test_v372_dms_nag_lab_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob("*.ino")).read_text()
        vl = (ROOT / "vehicle_logic.h").read_text()
        web = (ROOT / "web_api.h").read_text()
        dash = (ROOT / "dashboard_source.html").read_text()
        core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())

        assert '#include "r79_dms_composition_pure.h"' in ino
        assert 'bool     dmsControlEnabled;' not in core
        active = vl[vl.index('static bool r79DmsControlActive()'):vl.index('static inline bool r79DmsApplyFinal')]
        assert 'driverMonitoringControlSnapshot()' in active
        assert 'nagCfg.' not in active
        assert 'labMenuEnabled' not in active
        assert 'r79DmsApplyFinal(out.data)' in vl
        assert 'getBool("dms43", migrated)' in vl
        assert 'featureBoolPersist("dmsDisable", enabled)' in vl
        assert 'hasArg("dmsControlEnabled")' not in web
        assert '/api/driver-monitoring/config' in web
        assert 'server.on("/api/lab/dms-nag' not in web
        assert 'id="nagDmsToggle"' not in dash
        assert 'id="driverMonitoringToggle"' in dash
        assert 'id="labDmsNagToggle"' not in dash
        assert 'Request cabin-camera monitoring off during Autopilot.' in dash
        assert 'id="r79DmsBit43Value"' in dash
        print('PASS standalone production DMS/R79 composition contract')

        assert 'p.putBool(key, enabled) > 0u' in (ROOT / "vehicle_logic.h").read_text()

    def test_dms_nag_profiles(self):
        """Retained from test_v372_dms_nag_profiles_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        vp = (ROOT / "vehicle_profile.h").read_text(encoding="utf-8")
        vl = (ROOT / "vehicle_logic.h").read_text(encoding="utf-8")
        web = (ROOT / "web_api.h").read_text(encoding="utf-8")
        dash = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

        assert "vehicleProfileDmsNagSupported" in vp
        assert "return vehicleProfileEuUnlockSupported(id, topology);" in vp
        assert "activeProfileDmsNagSupported" in vp
        active = vl[vl.index('static bool r79DmsControlActive()'):vl.index('static inline bool r79DmsApplyFinal')]
        assert "activeProfileDmsNagSupported()" in active
        assert "driverMonitoringControlSnapshot()" in active
        assert "nagCfg." not in active
        assert "labMenuEnabled" not in active
        assert 'jw.boolean("supported", activeProfileDmsNagSupported());' in web
        assert 'jw.boolean("dmsNagSupported", activeProfileDmsNagSupported());' in web
        assert "YL ONLY" not in dash
        assert "Request cabin-camera monitoring off during Autopilot." in dash
        print("PASS standalone DMS profile capability contract")

    def test_r79_dms_ownership(self):
        """Retained from test_v390_r79_dms_ownership_contract.py."""
        import pathlib
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]


        def block(text: str, start: str, end: str) -> str:
            begin = text.rindex(start)
            finish = text.index(end, begin)
            return text[begin:finish]


        class R79DmsOwnershipContract(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.ino = (ROOT / (ROOT.name + ".ino")).read_text()
                cls.core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
                cls.logic = (ROOT / "vehicle_logic.h").read_text()
                cls.api = (ROOT / "web_api.h").read_text()

            def test_dms_is_standalone_production_policy(self):
                self.assertIn('#include "r79_dms_composition_pure.h"', self.ino)
                self.assertNotIn("bool     dmsControlEnabled;", self.core)
                self.assertIn('getBool("dms43", migrated)', self.logic)
                self.assertIn('featureBoolPersist("dmsDisable", enabled)', self.logic)
                active = block(self.logic, "static bool r79DmsControlActive()", "static inline bool r79DmsApplyFinal")
                self.assertNotIn("labMenuEnabled", active)
                self.assertNotIn("nagCfg.", active)
                self.assertIn("driverMonitoringControlSnapshot()", active)
                self.assertIn("nagApGateSnapshot(apValid, apActive)", active)
                self.assertIn("apValid, apActive", active)

            def test_every_r79_mux1_creation_path_gets_final_overlay(self):
                for start, end in [
                    ("static esp_err_t r79ApGateTransmitGuarded", "static esp_err_t r79LabDirectTwaiTransmit"),
                    ("static esp_err_t r79LabDirectTwaiTransmitGuarded", "static bool r79Mode1PostMux2GenerationCurrent"),
                    ("static esp_err_t mux1DisplayTransmit", "static void mux1CancelPendingLocked"),
                ]:
                    self.assertIn("r79DmsApplyFinal", block(self.logic, start, end))

            def test_stock_generation_has_r79_first_single_owner(self):
                process = block(self.logic, "static bool r79ProcessStockFrame", "static void r79Mode2Tick")
                self.assertIn("r79Claimed", process)
                self.assertIn("r79DmsStockDispositionPure", process)
                self.assertIn("r79DmsOnlyTransmit", process)
                dms_only = block(self.logic, "static bool r79DmsOnlyTransmit", "static bool r79ProcessStockFrame")
                self.assertIn("mux1DisplayTransmit(&out, canTxEpochSnapshot(), CAN_TX_FRESH_VH, false,", dms_only)
                self.assertNotIn("r79RetrySchedule", dms_only)
                self.assertNotIn("r79LabRecordTxResult", dms_only)

            def test_api_is_independent_of_nag_config(self):
                self.assertIn('server.on("/api/driver-monitoring/config", HTTP_GET', self.api)
                self.assertIn('server.on("/api/driver-monitoring/config", HTTP_POST', self.api)
                self.assertNotIn('hasArg("dmsControlEnabled")', self.api)



        assert 'p.putBool(key, enabled) > 0u' in (ROOT / "vehicle_logic.h").read_text()
        run_contract_cases(self, [R79DmsOwnershipContract], expected=4)

    def test_driver_window_runtime(self):
        """Retained from test_v3913_driver_window_runtime_contract.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        INO = (ROOT / (ROOT.name + ".ino")).read_text()
        CORE = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        RUNTIME = (ROOT / "can_runtime.h").read_text()
        LOGIC = (ROOT / "vehicle_logic.h").read_text()
        WEB = (ROOT / "web_api.h").read_text()

        assert '#include "driver_window_lab_pure.h"' in INO
        assert "CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB" in CORE

        assert "static void handleDriverWindowLab3C2CanB(" in LOGIC
        profile_gate = LOGIC.split("static bool driverWindowLabProfileSupported()", 1)[1].split("\n}", 1)[0]
        assert "driverWindowLabProfileSupportedPure" in profile_gate
        assert "activeCanBIsChassis()" not in profile_gate
        handler = LOGIC.split("static void handleDriverWindowLab3C2CanB(", 1)[1]
        handler = handler.split("\n}\n", 1)[0]
        assert "driverWindowLabConsumeStockPure" in handler
        assert "CAN_TX_FRESH_VH" in handler
        assert "CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB" in handler
        assert "canTxTwaiTransmitWithMaskTaggedGuarded" in handler
        assert "driverWindowLabArmContextSnapshot" in handler
        assert "twai_transmit(" not in handler
        assert "DRIVER_WINDOW_CONSUME_TX_FAILED" in handler
        assert "driverWindowLabLastConsumeReason = DRIVER_WINDOW_CONSUME_TX_FAILED" in handler

        case = RUNTIME.split("case VCLEFT_SWITCH_ID:", 1)[1].split("break;", 1)[0]
        assert "activeProfileIsYl()" in case
        assert "handleDriverWindowLab3C2CanB(f);" in case
        assert "driverWindowLabResetRuntimeUnderTxBarrier();" in RUNTIME

        assert "static void httpDriverWindowLabStats()" in WEB
        assert "static void httpDriverWindowLabOpen()" in WEB
        open_handler = WEB.split("static void httpDriverWindowLabOpen()", 1)[1]
        open_handler = open_handler.split("\n}\n", 1)[0]
        assert "driverWindowLabRequestOpen" in open_handler
        assert "twai_transmit(" not in open_handler
        assert "canTxTwaiTransmit" not in open_handler
        assert 'server.on("/api/lab/driver-window/stats", HTTP_GET, httpDriverWindowLabStats);' in WEB
        assert 'server.on("/api/lab/driver-window/open", HTTP_POST, httpDriverWindowLabOpen);' in WEB
        assert "driverWindowLabResetRuntime();" in WEB.split("static void httpFeatureLab()", 1)[1].split("static void httpFeatureDoorCancel", 1)[0]
        assert "canTxCancellationGenerationAdvance" in LOGIC
        assert "driverWindowLabServiceTick();" in RUNTIME
        assert "driverWindowLabResetRuntimeUnderTxBarrier();" in RUNTIME
        assert "driverWindowLabLastResult" in LOGIC
        assert "DRIVER_WINDOW_RESULT_COMPLETED" in LOGIC
        stats = WEB.split("static String driverWindowLabStatsToJson()", 1)[1].split("static void httpDriverWindowLabStats", 1)[0]
        assert "driverWindowLabLastResult" in stats
        assert "completed != 0u" not in stats

        for field in (
            '"supported"', '"available"', '"pending"', '"stockAgeMs"',
            '"requests"', '"completed"', '"txOk"', '"txFail"',
            '"blocked"', '"lastResult"', '"stockRaw"', '"lastTxRaw"'
        ):
            assert field in WEB

        print("PASS v3.29.1 driver-window LAB runtime/API contract")


if __name__ == "__main__":
    unittest.main()
