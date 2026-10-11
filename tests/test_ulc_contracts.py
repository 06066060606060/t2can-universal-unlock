"""Ulc contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class UlcContracts(unittest.TestCase):

    def test_ulc_single_sender(self):
        """Retained from test_ulc_single_sender_contract.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        logic = (root / "vehicle_logic.h").read_text()
        runtime = (root / "can_runtime.h").read_text()
        stalk = (root / "ulc_compositor_pure.h").read_text()
        api = (root / "web_api.h").read_text()
        sources = logic + runtime + stalk + api


        def function_body(source: str, signature: str) -> str:
            start = source.index(signature)
            brace = source.index("{", start)
            depth = 0
            for pos in range(brace, len(source)):
                if source[pos] == "{":
                    depth += 1
                elif source[pos] == "}":
                    depth -= 1
                    if depth == 0:
                        return source[brace : pos + 1]
            raise AssertionError(f"unclosed function: {signature}")


        inject = function_body(
            logic, "static void injectDriverAssistControl(const twai_message_t &src)"
        )
        assert inject.count("canTxTwaiTransmitValidated(") == 1
        assert "ulcCompose3f8Pure(" in inject
        assert "ulcNoConfirmInjectCanA" not in runtime
        for removed in [
            "ulcNoConfirmTargetBus",
            "ULC_NO_CONFIRM_BUS_A_PURE",
            "ulcNoConfirmEffectiveTargetBus",
            "ulcNoConfirmTxAOk",
            "ulcNoConfirmTxAFail",
        ]:
            assert removed not in sources, f"old target-bus symbol remains: {removed}"

        print("single CAN-B 0x3F8 compositor contract: PASS")

    def test_autolc(self):
        """Retained from test_v36b10_autolc_static.py."""
        from pathlib import Path
        root=Path(__file__).resolve().parents[1]
        logic=(root/'vehicle_logic.h').read_text()
        runtime=(root/'can_runtime.h').read_text()
        api=(root/'web_api.h').read_text()
        html=(root/'dashboard_source.html').read_text()
        ino=next(root.glob('*.ino')).read_text()
        pure=(root/'auto_lane_change_enable_pure.h').read_text() if (root/'auto_lane_change_enable_pure.h').exists() else ''
        # Confirm-Free must be AP-active, not NOA-only.
        ulc=(root/'ulc_compositor_pure.h').read_text()
        # Follow the production classifier rather than requiring its implementation
        # to be duplicated inside the feature policy header.
        assert '#include "tesla_can_fields_pure.h"' in ulc
        assert 'return teslaApStateActivePure(dasState4);' in ulc
        assert 'if (!dasStateValid) return false;' in ulc
        fields=(root/'tesla_can_fields_pure.h').read_text()
        classifier=fields.split('static inline bool teslaApStateActivePure(uint8_t state) {',1)[1].split('}',1)[0]
        ulc += classifier.replace('state', 'dasState4')
        assert 'dasState4 == 3' in ulc and 'dasState4 == 4' in ulc and 'dasState4 == 5' in ulc and 'dasState4 == 6' in ulc
        # 0x293 research toggle and pure codec.
        assert 'UI_CHASSIS_CONTROL_ID' in logic
        assert 'uiAutoLaneChangeEnabled' in logic
        assert 'uiAutoLaneChangeObserveAndInjectCanA' in logic
        assert 'uiAutoLaneChangeObserveAndInjectCanB' in logic
        assert 'uiAutoLaneChangeReadRawPure' in pure
        assert 'uiAutoLaneChangeFinalizePure' in pure
        assert 'case UI_CHASSIS_CONTROL_ID' in runtime
        assert 'uiAutoLaneChangeObserveAndInjectCanA' in runtime
        assert 'uiAutoLaneChangeObserveAndInjectCanB' in runtime
        # Persistent LAB setting/API/UI beneath Confirm-Free.
        assert 'begin("alc293lab"' in logic
        assert 'getBool("enabled", false)' in logic
        assert 'putBool("enabled"' in logic
        assert '/api/lab/auto-lane-change/update' in api
        assert 'id="labAutoLaneChangeToggle"' in html
        assert 'UI_autoLaneChangeEnable' in html
        assert 'AP active' in html
        # UI must expose stock A/B observations and TX telemetry.
        for token in ['labAutoLcStockA','labAutoLcStockB','labAutoLcGate','labAutoLcTxA','labAutoLcTxB']:
            assert token in html, token
        print('v3.6b10 AP-gated 0x293 AutoLC contract OK')

    def test_ulc_policy(self):
        """Retained from test_v36b11_ulc_policy_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        vl = (ROOT / "vehicle_logic.h").read_text()
        web = (ROOT / "web_api.h").read_text()
        dash = (ROOT / "dashboard_source.html").read_text()
        ino = next(ROOT.glob("*.ino")).read_text()

        assert '#include "ulc_compositor_pure.h"' in ino
        assert 'lab3f8UlcOffHighwayMode' in vl
        assert 'uiUlcOffHighway' in vl
        assert 'getBit(data, 15)' in vl
        assert 'ulcPolicyApGateOpenPure' in vl
        assert 'begin("ulc"' in vl
        assert 'getUChar("ulcOff"' in vl
        assert 'putUChar("ulcOff"' in vl
        assert 'server.hasArg("ulcOff")' in web
        assert 'ulcOffHighwayMode' in web
        assert 'lab3f8UlcSpeedMode' not in vl
        assert 'readBitsLE(data, 50, 2)' not in vl
        assert 'server.hasArg("ulcspeed")' not in web
        print('production ULC off-highway policy static tests passed')

    def test_ulc_stalk_confirm_timing(self):
        """Retained from test_v36b17_ulc_stalk_confirm_timing_static.py."""
        from pathlib import Path
        r = Path(__file__).resolve().parents[1]
        ino = next(r.glob('*.ino')).read_text(errors='ignore')
        vl = (r/'vehicle_logic.h').read_text(errors='ignore')
        web = (r/'web_api.h').read_text(errors='ignore')
        html = (r/'dashboard_source.html').read_text(errors='ignore')
        pure = (r/'ulc_compositor_pure.h').read_text(errors='ignore')

        assert 'ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE' in pure
        assert 'ULC_NO_CONFIRM_TIMING_PRE_AP_PURE' in pure
        assert 'ulcNoConfirmGateOpenWithTimingPure' in pure
        assert 'ulcNoConfirmTimingMode' in vl
        assert 'getUChar("timing"' in vl
        assert 'putUChar("timing"' in vl
        assert 'stalkConfirmTimingMode' in web
        assert 'server.hasArg("timing")' in web
        assert 'const String arg = server.arg("timing")' in web
        assert 'arg != "0" && arg != "1"' in web
        assert 'ulcNoConfirmGateOpenWithTimingPure' in vl
        assert 'selected.confirmFreeEnabled && !gates.confirmFreeOpen' in vl
        print('PASS production Confirm-Free timing contract')

    def test_ulc_confirm_capture(self):
        """Retained from test_v36c1_ulc_confirm_capture_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
        cap = (root / 'can_research_capture.h').read_text(encoding='utf-8')
        pure = (root / 'can_research_capture_pure.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

        assert 'RESEARCH_CAPTURE_MODE_ULC_CONFIRM = 3' in cap
        assert 'RESEARCH_CAPTURE_ULC_CONFIRM_PRE_MS = 3000' in cap
        assert 'RESEARCH_CAPTURE_ULC_CONFIRM_POST_MS = 7000' in cap
        assert 'researchUlcConfirmTargetIdPure' in pure
        for token in ['0x247', '0x3F8', '0x3E9', '0x24A', '0x3FD', '0x293']:
            assert token in pure

        # ULC_CONFIRM is a RAW mode but records only the six targeted RX IDs.
        assert 'mode == RESEARCH_CAPTURE_MODE_ULC_CONFIRM' in cap
        assert '!researchUlcConfirmTargetIdPure(id)' in cap
        assert 'RESEARCH_CAPTURE_ULC_CONFIRM_PRE_MS' in cap
        assert 'RESEARCH_CAPTURE_ULC_CONFIRM_POST_MS' in cap

        # The legacy targeted recorder implementation remains available for historical
        # CSV decoding, but v3.29.1 deliberately retires it from the public API/UI.
        mode_handler = api[api.index('static void httpResearchCaptureMode()'):
                           api.index('static void httpResearchCaptureReset()')]
        assert 'raw == "ULC_CONFIRM"' not in mode_handler
        mode_select = html[html.index('id="researchCapMode"'):html.index('</select>', html.index('id="researchCapMode"'))]
        assert 'ULC_CONFIRM' not in mode_select

        # CSV must retain physical CAN side and logical profile bus role independently.
        assert 'physical_bus,bus_role' in api
        assert 'researchCapturePhysicalBusName' in api
        assert 'researchCaptureProfileBusName' in api

        print('v3.6d9a2 ULC Confirm Capture contract OK')

    def test_autolc_bus_target(self):
        """Retained from test_v36b22_autolc_bus_target_static.py."""
        from pathlib import Path
        root = Path(__file__).resolve().parents[1]
        logic = (root/'vehicle_logic.h').read_text()
        api = (root/'web_api.h').read_text()
        html = (root/'dashboard_source.html').read_text()
        ino = next(root.glob('*.ino')).read_text()
        pure = (root/'auto_lane_change_enable_pure.h').read_text()

        assert 'UI_AUTO_LC_BUS_BOTH_PURE' in pure
        assert 'uiAutoLaneChangeBusAllowedPure' in pure
        assert 'uiAutoLaneChangeTargetBus' in logic
        assert 'begin("alc293lab"' in logic
        assert 'getUChar("bus"' in logic
        assert 'putUChar("bus"' in logic
        assert 'uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_A_PURE)' in logic
        assert 'uiAutoLaneChangeBusAllowedPure(targetBus, UI_AUTO_LC_BUS_B_PURE)' in logic
        assert 'server.hasArg("bus")' in api
        assert 'autoLaneChangeTargetBus' in api
        assert 'id="labAutoLaneChangeBus"' in html
        assert '0x293 TX Target' in html
        assert 'updateAutoLaneChange293Bus' in html
        print('v3.6d9a2 0x293 independent bus-target contract OK')


if __name__ == "__main__":
    unittest.main()
