"""Nag Tsl9 contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class NagTsl9Contracts(unittest.TestCase):

    def test_legacy_y_body_scroll_runtime(self):
        """Retained from test_legacy_y_body_scroll_runtime.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        runtime = (root / "can_runtime.h").read_text()
        logic = (root / "vehicle_logic.h").read_text()
        api = (root / "web_api.h").read_text()
        profile = (root / "vehicle_profile.h").read_text()

        body_route = profile.split("static inline bool vehicleProfileTsl9InputOnBodyCanA", 1)[1].split("\n}", 1)[0]
        assert "return vehicleProfileCanAIsBody(id, topology);" in body_route
        assert "vehicleProfileTsl9InputSupported" in profile

        body_rx = runtime.split("} else if (activeCanAIsBody()) {", 1)[1].split("\n      }", 1)[0]
        assert "tsl9InputObserveCanA(rxf)" in body_rx
        assert "tsl9InputServiceCanA();" in runtime

        chassis_rx = runtime.split("case VCLEFT_SWITCH_ID:", 1)[1].split("break;", 1)[0]
        assert "activeProfileTsl9InputSupported()" in chassis_rx
        assert "!activeProfileTsl9InputOnBodyCanA()" in chassis_rx
        assert "tsl9InputObserveCanB(f)" in chassis_rx
        assert "tsl9InputServiceCanB();" in runtime

        assert "tsl9InputSendCanA" in logic and "canTxMcpSend" in logic
        assert "tsl9InputSendCanB" in logic and "canTxTwaiTransmitWithMask" in logic
        assert 'server.on("/api/ap-right-scroll' not in api

        print("PASS TSL9-only input scheduler Body/Chassis route integration")

    def test_tsl9_legacy_body_runtime(self):
        """Retained from test_tsl9_legacy_body_runtime_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        PROFILE = (ROOT / "vehicle_profile.h").read_text(encoding="utf-8")
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        RUNTIME = (ROOT / "can_runtime.h").read_text(encoding="utf-8")
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

        selectable = PROFILE.split(
            "static inline bool vehicleProfileLegacyTsl9RouteSelectable", 1
        )[1].split("\n}", 1)[0]
        assert "VEHICLE_MODEL_Y_LEGACY" in selectable
        assert "VEHICLE_MODEL_3_LEGACY" in selectable
        assert "VEHICLE_MODEL_Y_JUNIPER" not in selectable
        assert "VEHICLE_MODEL_3_HIGHLAND" not in selectable
        assert "vehicleProfileCanAIsBody" in selectable

        mcp_start = CORE.index("static bool nagProcessTsl9Mcp(")
        mcp_end = CORE.index("static bool nagProcessTsl9Twai399(", mcp_start)
        mcp = CORE[mcp_start:mcp_end]
        assert "nagTsl9TransformStock(config, handsOnRoute, isaRoute," in mcp
        mcp += CORE[CORE.index("struct NagTsl9TransformConfig {"):CORE.index("static bool nagProcessTsl9Mcp(")].replace("config.enabled", "enabled").replace("config.method", "method")
        assert "nagTsl9Body39BSelected()" in mcp
        assert "0x39Bu" in mcp
        assert "0x399u" in mcp
        assert "tsl9ApplyDasTransformForCanIdPure" in mcp
        assert "canTxMcpSend" in mcp

        twai_start = CORE.index("static bool nagProcessTsl9Twai399(")
        twai_end = CORE.index("// ── Nag process frame from MCP2515", twai_start)
        twai = CORE[twai_start:twai_end]
        assert "nagTsl9TransformStock(config, handsOnRoute, isaRoute," in twai
        twai += CORE[CORE.index("struct NagTsl9TransformConfig {"):CORE.index("static bool nagProcessTsl9Mcp(")].replace("config.enabled", "enabled").replace("config.method", "method")
        assert "nagTsl9Chassis399Selected()" in twai
        assert "tsl9ApplyDasTransformForCanIdPure" in twai
        assert "0x399u" in twai

        body_start = RUNTIME.index("} else if (activeCanAIsBody()) {")
        body_end = RUNTIME.index("\n      }", body_start)
        body = RUNTIME[body_start:body_end]
        assert "nagTsl9Body39BSelected()" in body
        assert "nagProcessTsl9Mcp(rxf)" in body

        case_399 = RUNTIME[RUNTIME.index("case 921:"):RUNTIME.index("case 0x331:")]
        assert "handle921(f.data, f.data_length_code)" in case_399
        assert "nagProcessTsl9Twai399(f)" in case_399

        recover_a = RUNTIME[
            RUNTIME.index("static void invalidateCanTxStateForCanARecovery()"):
            RUNTIME.index("static void invalidateCanTxStateForCanBRecovery()")
        ]
        assert "nagTsl9Body39BSelected()" in recover_a
        assert "nagTsl9State = {}" in recover_a

        assert "function nagTsl9RouteInfo()" in DASH
        route_ui = DASH[
            DASH.index("function nagTsl9RouteInfo()"):
            DASH.index("function renderNagCfg()")
        ]
        assert "p===3||p===5" in route_ui
        assert "t===2" in route_ui
        assert "Body CAN A · 0x39B" in route_ui
        assert "Chassis CAN B · 0x399" in route_ui
        assert "Party CAN A · 0x399" in route_ui
        assert "tsl9LegacyRoute" in route_ui

        print("PASS Legacy-only Body 0x39B TSL9 route contract")

    def test_tsl9_runtime(self):
        """Retained from test_tsl9_runtime_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text()
        core = (root / 'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        runtime = (root / 'can_runtime.h').read_text()
        api = (root / 'web_api.h').read_text()
        html = (root / 'dashboard_source.html').read_text()

        assert '#include "tsl9_hands_on_0x399_pure.h"' in ino
        assert 'uint8_t  method;' in core
        assert 'uint8_t  tsl9Sequence;' in core
        assert 'uint8_t  tsl9DowngradeWindow;' in core
        assert 'nagMethodSanitizePure' in core
        assert 'reader.getUChar("method"' in core
        assert 'stored.config = config;' in core
        assert 'reader.getUChar("tsl9seq"' in core
        assert 'stored.config = config;' in core
        assert 'reader.getUChar("tsl9win"' in core
        assert 'stored.config = config;' in core

        # YL 0x399 and Legacy Body 0x39B use their profile-selected MCP2515 path.
        assert 'static bool nagProcessTsl9Mcp(' in core
        mcp_start = core.index('static bool nagProcessTsl9Mcp(')
        mcp_end = core.index('static bool nagProcessTsl9Twai399(', mcp_start)
        mcp = core[mcp_start:mcp_end]
        assert "nagTsl9TransformStock(config, handsOnRoute, isaRoute," in mcp
        mcp += core[core.index("struct NagTsl9TransformConfig {"):core.index("static bool nagProcessTsl9Mcp(")].replace("config.enabled", "enabled").replace("config.method", "method")
        assert 'tsl9ApplyDasTransformForCanIdPure' in mcp
        assert 'canTxMcpSend' in mcp
        assert 'activeProfileIsYl()' in mcp
        assert 'nagTsl9Body39BSelected()' in mcp
        assert '0x39Bu' in mcp

        # Juniper and Highland keep their 0x399 TSL9 route on Chassis CAN B.
        twai_start = core.index('static bool nagProcessTsl9Twai399(')
        twai_end = core.index('// ── Nag process frame from MCP2515', twai_start)
        twai = core[twai_start:twai_end]
        assert "nagTsl9TransformStock(config, handsOnRoute, isaRoute," in twai
        twai += core[core.index("struct NagTsl9TransformConfig {"):core.index("static bool nagProcessTsl9Mcp(")].replace("config.enabled", "enabled").replace("config.method", "method")
        assert 'activeCanBIsChassis()' in twai
        assert 'canTxTwaiTransmitWithMaskTaggedGuarded(' in twai
        assert '&isaSuppressionGeneration' in twai
        assert 'tsl9ApplyDasTransformForCanIdPure' in twai
        assert 'nagProcessTsl9Twai399(f)' in runtime
        assert 'const bool handsOnRoute = nagTsl9Chassis399Selected();' in twai
        assert 'handsOnRoute && enabled && method == NAG_METHOD_TSL9_PURE' in twai
        selector = core.split('static bool nagTsl9Chassis399Selected()',1)[1].split('\n}',1)[0]
        assert 'activeProfileNagTsl9Supported()' in selector
        assert 'activeProfileIsaSuppressionSupported()' in twai
        assert 'nagTsl9Chassis399Selected()' in twai

        # The method switch is exclusive: TSL9 can still observe the AP-status frame,
        # but the legacy torque injection path exits when TSL9 is selected.
        assert 'nagProcessTsl9Mcp(rxf)' in core
        assert 'if (methodNow != NAG_METHOD_TORQUE_PURE) return;' in core

        for key in ('"method"', '"methodName"', '"tsl9Sequence"',
                    '"tsl9SequenceName"', '"tsl9Window"', '"tsl9WindowName"',
                    '"tsl9Rx"', '"tsl9TxOk"',
                    '"tsl9TxFail"'):
            assert key in api
        assert 'server.hasArg("method")' in api
        assert 'server.hasArg("tsl9Window")' in api
        update_start = api.index('static void httpNagUpdate()')
        update_end = api.index('\nstatic void ', update_start + 1)
        assert 'nagTsl9State = {}' not in api[update_start:update_end], (
            'method/enable changes must not restart the AP-active 12-second window'
        )
        assert 'id="nagMethod"' in html
        assert 'id="tsl9LegacyRoute"' in html
        assert 'id="tsl9SequenceWrap"' in html
        assert 'id="tsl9Sequence"' in html
        assert 'id="tsl9WindowWrap"' in html
        assert 'id="tsl9Window"' in html
        assert 'Entire AP session' in html
        assert 'V8.2 Original · 4 → 1' in html
        assert 'Extended · 2 / 3 / 4 → 1' in html
        assert 'updateNagMethod' in html
        assert 'updateTsl9Sequence' in html
        assert "toggleNag(e.target.checked,'tsl9Window')" in html

        print('TSL9 runtime/dashboard contract OK')

    def test_tsl9_scroll_assist(self):
        """Retained from test_tsl9_scroll_assist_static.py."""
        from pathlib import Path
        import unittest

        ROOT = Path(__file__).resolve().parents[1]


        class Tsl9ScrollAssistContract(unittest.TestCase):
            def test_runtime_unifies_tsl9_and_torque_right_scroll_ownership(self):
                text = (ROOT / "vehicle_logic.h").read_text()
                start = text.index("static Tsl9InputInputsPure tsl9InputInputsSnapshot")
                end = text.index("static bool tsl9InputSendCanA", start)
                block = text[start:end]
                self.assertIn("nagEnabled", block)
                self.assertIn("NAG_METHOD_TSL9_PURE", block)
                self.assertIn("nagCtx.scrollWarningActive", block)
                self.assertIn("nagCtx.visualWarningActive", block)
                self.assertIn("periodicIntervalSeconds", block)
                self.assertIn("warningEdgeOnly", block)
                self.assertIn("apActive", block)

            def test_timer_owns_v82_waveform_and_retry(self):
                scheduler = (ROOT / "tsl9_input_scheduler_pure.h").read_text()
                self.assertIn("TSL9_INPUT_STEP_MS_PURE = 100u", scheduler)
                self.assertIn("TSL9_INPUT_RETRY_MS_PURE = 250u", scheduler)
                self.assertIn("TSL9_INPUT_REPEAT_MIN_MS_PURE = 2000u", scheduler)
                self.assertIn("TSL9_INPUT_REPEAT_MAX_MS_PURE = 3000u", scheduler)
                self.assertIn(
                    "static constexpr int8_t fourStepTicks[4] = {1, 0, -1, 0}",
                    scheduler,
                )
                self.assertIn("static constexpr int8_t pairTicks[2] = {1, -1}", scheduler)

            def test_independent_api_and_dashboard_are_removed(self):
                api = (ROOT / "web_api.h").read_text()
                html = (ROOT / "dashboard_source.html").read_text()
                self.assertNotIn('/api/ap-right-scroll', api)
                self.assertNotIn('panelApRightScroll', html)
                self.assertIn('id="tsl9InputMode"', html)
                self.assertNotIn('id="tsl9IsaToggle"', html)
                self.assertIn('id="isaSuppressionToggle"', html)
                self.assertIn('/api/isa-suppression/config', html)
                self.assertNotIn('id="nagDmsToggle"', html)
                self.assertIn('id="driverMonitoringToggle"', html)
                self.assertIn('/api/driver-monitoring/config', html)
        run_contract_cases(self, [Tsl9ScrollAssistContract], expected=3)

    def test_scroll_burst(self):
        """Retained from test_v36f1_scroll_burst_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text()
        logic = (root / 'vehicle_logic.h').read_text()
        runtime = (root / 'can_runtime.h').read_text()
        api = (root / 'web_api.h').read_text()
        dash = (root / 'dashboard_source.html').read_text()

        assert '#include "tsl9_input_scheduler_pure.h"' in ino
        assert '#include "tsl9_input_scheduler_pure.h"' in ino
        assert 'tsl9InputObserveCanA(rxf)' in runtime
        assert 'tsl9InputObserveCanB(f)' in runtime
        assert 'tsl9InputServiceCanA();' in runtime
        assert 'tsl9InputServiceCanB();' in runtime
        assert 'TSL9_INPUT_REPEAT_MIN_MS_PURE' in (root / 'tsl9_input_scheduler_pure.h').read_text()
        assert 'nagEnabled' in logic and 'NAG_METHOD_TSL9_PURE' in logic
        assert 'nagCtx.scrollWarningActive' in logic
        assert 'nagCtx.visualWarningActive' in logic
        assert 'in.periodicIntervalSeconds' in logic
        assert '/api/ap-right-scroll' not in api
        assert 'panelApRightScroll' not in dash
        assert 'id="tsl9InputMode"' in dash
        assert 'id="torqueRightScrollWrap"' in dash
        assert 'id="tsl9RightPeriodicWrap"' in dash

        print('PASS v3.29.1 Torque and TSL9 periodic right-scroll integration contract')

    def test_ap_right_scroll_routes(self):
        """Retained from test_v37_ap_right_scroll_routes_static.py."""
        from pathlib import Path
        import unittest

        ROOT = Path(__file__).resolve().parents[1]


        class Tsl9InputRoutesContract(unittest.TestCase):
            def test_profile_helper_supports_tsl9_input_routes(self):
                text = (ROOT / "vehicle_profile.h").read_text()
                self.assertIn("vehicleProfileTsl9InputSupported", text)
                self.assertIn("vehicleProfileTsl9InputOnBodyCanA", text)
                self.assertIn("activeProfileTsl9InputSupported", text)

            def test_can_b_dispatch_observes_stock_template(self):
                text = (ROOT / "can_runtime.h").read_text()
                block = text[text.index("case VCLEFT_SWITCH_ID:"):text.index("case DOOR_SWITCH_ID:")]
                self.assertIn("activeProfileTsl9InputSupported()", block)
                self.assertIn("tsl9InputObserveCanB(f)", block)

            def test_timer_services_both_routes(self):
                runtime = (ROOT / "can_runtime.h").read_text()
                logic = (ROOT / "vehicle_logic.h").read_text()
                self.assertIn("tsl9InputServiceCanA();", runtime)
                self.assertIn("tsl9InputServiceCanB();", runtime)
                self.assertIn("CAN_TX_FRESH_VH", logic)
                self.assertIn("canTxMcpSend", logic)

            def test_chassis_399_still_observes_ap_state(self):
                logic = (ROOT / "vehicle_logic.h").read_text()
                start = logic.index("static void handle921(const uint8_t *data, uint8_t dlc) {")
                handle = logic[start:logic.index("static void handle1016", start)]
                self.assertIn("nagObserveHandsOnState", handle)

            def test_nag_api_reports_scheduler_fields(self):
                text = (ROOT / "web_api.h").read_text()
                for needle in ('"tsl9InputActive"', '"tsl9InputIntervalMs"',
                               '"tsl9InputTemplateAgeMs"', '"tsl9InputCleanupTx"'):
                    self.assertIn(needle, text)
        run_contract_cases(self, [Tsl9InputRoutesContract], expected=5)

    def test_tsl9_input_runtime(self):
        """Retained from test_v390_tsl9_input_runtime_contract.py."""
        import pathlib
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]


        class Tsl9InputRuntimeContractTests(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.ino = (ROOT / (ROOT.name + ".ino")).read_text()
                cls.core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
                cls.logic = (ROOT / "vehicle_logic.h").read_text()
                cls.runtime = (ROOT / "can_runtime.h").read_text()

            def test_scheduler_is_part_of_firmware_and_nag_config(self):
                self.assertIn('#include "tsl9_input_scheduler_pure.h"', self.ino)
                self.assertIn("uint8_t  tsl9InputMode;", self.core)
                self.assertIn("TSL9_INPUT_MODE_DEFAULT_PURE", self.core)
                self.assertIn('getUChar("tsl9in"', self.core)
                self.assertIn('stored.config = config;', self.core)

            def test_rx_observes_warning_templates_and_services_periodic_right_scroll(self):
                self.assertIn("tsl9InputObserveCanA(rxf);", self.runtime)
                self.assertIn("tsl9InputObserveCanB(f);", self.runtime)
                self.assertIn("tsl9InputServiceCanA();", self.runtime)
                self.assertIn("tsl9InputServiceCanB();", self.runtime)
                self.assertNotIn("handle3C2OnCanARightScroll", self.runtime)
                self.assertNotIn("handle3C2OnCanBRightScroll", self.runtime)

            def test_timer_owned_scheduler_handles_warning_and_opt_in_periodic_policy(self):
                self.assertIn("nagEnabled", self.logic)
                self.assertIn("NAG_METHOD_TSL9_PURE", self.logic)
                self.assertIn("nagCtx.scrollWarningActive", self.logic)
                self.assertIn("nagCtx.visualWarningActive", self.logic)
                self.assertIn("in.periodicIntervalSeconds", self.logic)
                self.assertIn("tsl9InputWarningStatePure", self.core)
                self.assertNotIn("tsl9ScrollWarningStatePure", self.core)
                self.assertIn("esp_random()", self.logic)
                self.assertIn(
                    "tsl9InputRequestCancel(TSL9_INPUT_FAILURE_CAN_UNAVAILABLE_PURE, true);",
                    self.runtime,
                )

            def test_cleanup_is_retry_owned_and_never_uses_a_stale_template(self):
                shared = self.logic.split("static void tsl9InputServiceRoute(", 1)[1].split("static void tsl9InputServiceCanA()", 1)[0]
                self.assertEqual(shared.count("tsl9InputScheduler.onCommandResult("), 2)
                self.assertEqual(shared.count("in.templateFresh &&"), 2)
                for route in ("true, tsl9InputCanATemplate", "false, tsl9InputCanBTemplate"):
                    self.assertIn("tsl9InputServiceRoute(" + route, self.logic)
                self.assertIn("if (!tsl9InputQuiesceForConfig())", (ROOT / "web_api.h").read_text())
                self.assertNotIn("onConfigCleanupResult", self.logic)
                self.assertNotIn("tsl9InputRuntimeReset", self.runtime)
                quiesce = self.logic.split("static bool tsl9InputQuiesceForConfig()", 1)[1].split(
                    "static void handle102LaneChangeCancel", 1
                )[0]
                self.assertNotIn("tsl9InputServiceCanA", quiesce)
                self.assertNotIn("tsl9InputServiceCanB", quiesce)
                self.assertIn("configQuiesceComplete", quiesce)

            def test_r79_tick_precedes_can_b_input_scheduler(self):
                r79 = self.runtime.index("r79TransportTick();")
                tsl9 = self.runtime.index("tsl9InputServiceCanB();")
                self.assertLess(r79, tsl9)
        run_contract_cases(self, [Tsl9InputRuntimeContractTests], expected=5)

    def test_tsl9_isa_runtime(self):
        """Retained from test_v390_tsl9_isa_runtime_contract.py."""
        import pathlib
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]


        class Tsl9IsaRuntimeContract(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
                cls.api = (ROOT / "web_api.h").read_text()
                cls.logic = (ROOT / "vehicle_logic.h").read_text()
                cls.state = (ROOT / "t2can_core_state.h").read_text()

            def test_saved_toggle_and_composed_runtime_transform(self):
                self.assertNotIn("tsl9IsaChimeSuppress", self.core)
                self.assertIn("isaSuppressionEnabled", self.state)
                self.assertIn('getBool("isaSuppress"', self.logic)
                self.assertIn('featureBoolPersist("isaSuppress"', self.logic)
                self.assertIn('getBool("tsl9isa"', self.logic)
                self.assertEqual(self.core.count("tsl9ApplyDasTransformForCanIdPure("), 1)
                self.assertEqual(self.core.count("nagTsl9TransformStock(config, handsOnRoute, isaRoute,"), 2)
                self.assertNotIn("tsl9ApplyHandsOnDowngradeForCanIdPure(\n      nagTsl9State", self.core)
                self.assertGreaterEqual(
                    self.core.count("&isaSuppressionGeneration, config.isaSuppressionGenerationSnapshot"),
                    2,
                )
                self.assertIn("canTxMcpSendTaggedGuarded(", self.core)
                self.assertIn("canTxTwaiTransmitWithMaskTaggedGuarded(", self.core)

            def test_api_reports_and_updates_toggle(self):
                self.assertNotIn('"tsl9IsaChimeSuppress"', self.api)
                self.assertIn('server.on("/api/isa-suppression/config", HTTP_GET', self.api)
                self.assertIn('server.on("/api/isa-suppression/config", HTTP_POST', self.api)
                self.assertIn("httpIsaSuppressionControlUpdate", self.api)

            def test_dms_migration_failure_does_not_block_saved_isa_restore(self):
                load = self.logic.split("static void featureCfgLoad()", 1)[1].split(
                    "static bool blinkerTxModePersist", 1
                )[0]
                dms = load.split("if (hasDriverMonitoringControl)", 1)[1].split(
                    "if (hasIsaSuppressionControl)", 1
                )[0]
                self.assertNotIn('if (!oldNag.begin("nag", false)) return;', dms)



        assert 'p.putBool(key, enabled) > 0u' in (ROOT / "vehicle_logic.h").read_text()
        run_contract_cases(self, [Tsl9IsaRuntimeContract], expected=3)

    def test_tsl9_legacy_route(self):
        """Retained from test_v390_tsl9_legacy_route_contract.py."""
        import pathlib
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]


        class Tsl9LegacyRouteContract(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
                cls.runtime = (ROOT / "can_runtime.h").read_text()
                cls.api = (ROOT / "web_api.h").read_text()

            def test_route_is_saved_and_reported(self):
                self.assertIn("uint8_t  tsl9LegacyRoute;", self.core)
                self.assertIn('getUChar("tsl9rt"', self.core)
                self.assertIn('stored.config = config;', self.core)
                self.assertIn('"tsl9LegacyRoute"', self.api)
                self.assertIn('hasArg("tsl9LegacyRoute")', self.api)

            def test_dispatch_is_exclusive_but_chassis_state_decode_remains(self):
                self.assertIn("nagTsl9Body39BSelected()", self.runtime)
                case_399 = self.runtime.split("case 921:", 1)[1].split("break;", 1)[0]
                self.assertIn("nagProcessTsl9Twai399(f)", case_399)
                self.assertIn("handle921(f.data", case_399)
                self.assertIn("nagTsl9Body39BSelected()", self.core)
                self.assertIn("nagTsl9Chassis399Selected()", self.core)

            def test_route_change_quiesces_old_transport(self):
                update = self.api.split("static void httpNagUpdate()", 1)[1].split(
                    "static void httpNagReset()", 1)[0]
                self.assertIn("tsl9InputQuiesceForConfig()", update)
                self.assertIn("nagTsl9RuntimeReset()", update)
                self.assertLess(update.index("tsl9InputQuiesceForConfig()"),
                                update.index("nagCfgCommit(nc)"))
        run_contract_cases(self, [Tsl9LegacyRouteContract], expected=3)

    def test_nag_right_scroll(self):
        """Retained from test_v392_nag_right_scroll_contract.py."""
        import pathlib
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]


        class NagRightScrollContractTests(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.ino = (ROOT / (ROOT.name + ".ino")).read_text()
                cls.logic = (ROOT / "vehicle_logic.h").read_text()
                cls.runtime = (ROOT / "can_runtime.h").read_text()
                cls.api = (ROOT / "web_api.h").read_text()
                cls.dashboard = (ROOT / "dashboard_source.html").read_text()

            def test_timer_scheduler_is_the_single_right_scroll_owner(self):
                self.assertIn('#include "tsl9_input_scheduler_pure.h"', self.ino)
                self.assertIn("periodicIntervalSeconds", self.logic)
                self.assertIn("warningEdgeOnly", self.logic)
                self.assertIn("sequencePattern", self.logic)
                self.assertIn("tsl9InputServiceCanA();", self.runtime)
                self.assertIn("tsl9InputServiceCanB();", self.runtime)
                self.assertNotIn("nagRightScrollPrepare", self.logic)
                self.assertNotIn("handle3C2OnCanARightScroll", self.runtime)
                self.assertNotIn("handle3C2OnCanBRightScroll", self.runtime)
                self.assertIn("nagTorqueRightScrollPattern", self.logic)
                self.assertIn("nagTsl9RightPeriodicEnabled", self.logic)

            def test_settings_persist_and_api_validates_both_methods(self):
                for persisted_read in (
                    r'getBool\(\s*"rsEnabled"',
                    r'getUShort\(\s*"rsInterval"',
                    r'getUChar\(\s*"rsPattern"',
                    r'getBool\(\s*"tsl9RsEn"',
                    r'getUShort\(\s*"tsl9RsInt"',
                ):
                    self.assertRegex(self.logic, persisted_read)
                for field in (
                    "torqueRightScrollEnabled",
                    "torqueRightScrollIntervalSeconds",
                    "torqueRightScrollPattern",
                    "tsl9RightPeriodicEnabled",
                    "tsl9RightPeriodicIntervalSeconds",
                ):
                    self.assertIn(f'"{field}"', self.api)
                self.assertIn("httpDecimalU16InRange", self.api)
                self.assertNotIn(
                    'server.arg("torqueRightScrollIntervalSeconds").toInt()', self.api
                )

            def test_dashboard_exposes_torque_and_tsl9_right_speed_controls(self):
                for control in (
                    "torqueRightScrollWrap",
                    "torqueRightScrollToggle",
                    "torqueRightScrollInterval",
                    "torqueRightScrollPattern",
                    "tsl9RightPeriodicWrap",
                    "tsl9RightPeriodicToggle",
                    "tsl9RightPeriodicInterval",
                ):
                    self.assertIn(f'id="{control}"', self.dashboard)
                self.assertIn("+1 → 0 → −1 → 0", self.dashboard)
                self.assertIn("+1 → −1", self.dashboard)
                self.assertIn("!tsl9||!ss||!im", self.dashboard)
        run_contract_cases(self, [NagRightScrollContractTests], expected=3)


if __name__ == "__main__":
    unittest.main()
