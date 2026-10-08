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


if __name__ == "__main__":
    unittest.main()
