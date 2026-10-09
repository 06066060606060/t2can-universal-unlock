import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class Tsl9InputRuntimeContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ino = (ROOT / "T2CAN-Universal-v3.28.0-LP_YL.ino").read_text()
        cls.core = (ROOT / "can_core.h").read_text()
        cls.logic = (ROOT / "vehicle_logic.h").read_text()
        cls.runtime = (ROOT / "can_runtime.h").read_text()

    def test_scheduler_is_part_of_firmware_and_nag_config(self):
        self.assertIn('#include "tsl9_input_scheduler_pure.h"', self.ino)
        self.assertIn("uint8_t  tsl9InputMode;", self.core)
        self.assertIn("TSL9_INPUT_MODE_DEFAULT_PURE", self.core)
        self.assertIn('getUChar("tsl9in"', self.core)
        self.assertIn('putUChar("tsl9in"', self.core)

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
        self.assertGreaterEqual(
            self.logic.count("tsl9InputScheduler.onCommandResult("), 4
        )
        self.assertGreaterEqual(self.logic.count("in.templateFresh &&"), 4)
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


if __name__ == "__main__":
    unittest.main()
