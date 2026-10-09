import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class NagRightScrollContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ino = (ROOT / "T2CAN-Universal-v3.28.0-LP_YL.ino").read_text()
        cls.logic = (ROOT / "vehicle_logic.h").read_text()
        cls.runtime = (ROOT / "can_runtime.h").read_text()
        cls.api = (ROOT / "web_api.h").read_text()
        cls.dashboard = (ROOT / "dashboard_source.html").read_text()

    def test_timer_scheduler_is_the_single_right_scroll_owner(self):
        self.assertIn('#include "ap_right_scroll_pure.h"', self.ino)
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


if __name__ == "__main__":
    unittest.main()
