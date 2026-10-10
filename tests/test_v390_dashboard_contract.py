#!/usr/bin/env python3
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")


class V390DashboardContract(unittest.TestCase):
    def test_tsl9_controls_live_inside_nag(self):
        for control_id in (
            "tsl9InputMode",
            "tsl9LegacyRoute",
        ):
            self.assertIn(f'id="{control_id}"', DASH)
        self.assertIn("Left Volume", DASH)
        self.assertIn("Right Speed", DASH)
        self.assertIn("Body 0x39B", DASH)
        self.assertIn("Chassis 0x399", DASH)
        self.assertNotIn('id="tsl9IsaToggle"', DASH)

    def test_isa_suppression_is_standalone(self):
        self.assertIn('id="isaSuppressionRow"', DASH)
        self.assertIn('id="isaSuppressionToggle"', DASH)
        self.assertIn("/api/isa-suppression/config", DASH)
        self.assertIn('server.on("/api/isa-suppression/config", HTTP_GET', API)
        self.assertIn('server.on("/api/isa-suppression/config", HTTP_POST', API)

    def test_independent_ap_right_scroll_is_removed(self):
        self.assertNotIn('id="panelApRightScroll"', DASH)
        self.assertNotIn("/api/ap-right-scroll", DASH)
        self.assertNotIn('server.on("/api/ap-right-scroll', API)
        self.assertNotIn("httpApRightScrollStats", API)
        self.assertNotIn("httpApRightScrollUpdate", API)

    def test_dms_is_promoted_out_of_lab(self):
        self.assertNotIn('id="panelLabDmsNag"', DASH)
        self.assertNotIn('id="labDmsNagToggle"', DASH)
        self.assertNotIn('server.on("/api/lab/dms-nag', API)
        self.assertIn('id="driverMonitoringToggle"', DASH)
        self.assertNotIn('id="nagDmsToggle"', DASH)
        settings = DASH.split('<main class="page" data-page="settings">', 1)[1].split('</main>', 1)[0]
        self.assertIn('id="driverMonitoringRow"', settings)
        self.assertIn("/api/driver-monitoring/config", DASH)

    def test_ble_preserving_nvs_reset_is_exposed(self):
        self.assertIn('id="resetNvsKeepBleBtn"', DASH)
        self.assertIn('id="resetNvsKeepBleWarningModal"', DASH)
        self.assertIn("/api/system/reset-nvs-keep-ble", DASH)
        self.assertIn(
            'server.on("/api/system/reset-nvs-keep-ble", HTTP_POST, '
            "httpResetNvsPreserveS3xyBle);",
            API,
        )


if __name__ == "__main__":
    unittest.main()
