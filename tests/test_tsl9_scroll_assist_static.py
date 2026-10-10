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


if __name__ == "__main__":
    unittest.main()
