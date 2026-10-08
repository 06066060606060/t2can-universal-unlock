from html.parser import HTMLParser
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
DASHBOARD = (ROOT / "dashboard_source.html").read_text()


class InputCollector(HTMLParser):
    def __init__(self):
        super().__init__()
        self.by_id = {}

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        if values.get("id"):
            self.by_id[values["id"]] = (tag, values)


class DashboardV37Contract(unittest.TestCase):
    def test_required_production_controls_and_fixed_status_copy(self):
        for token in (
            "R79 bit18", "STOCK", "FORCE 0",
            "NOA Start Wait (s)", "Cancel Wait (s)",
            "TSL9 Input Assist", "ISA Suppression", "Fixed R79 Policy",
        ):
            self.assertIn(token, DASHBOARD)

    def test_retired_r79_controls_are_absent(self):
        for token in (
            "R79 Transport", "POST-MUX2 TX", "PRE-MUX1 TX",
            "Periodic Scheduler", "Quiet Window Event Anchor",
            "ROAMING Mirror Ratio", "ROAMING MUX1 Burst", "MUX0 → TX Delay",
        ):
            self.assertNotIn(token, DASHBOARD)

    def test_timing_input_ranges_and_defaults(self):
        parser = InputCollector()
        parser.feed(DASHBOARD)
        expected = {
            "blinkNoaStabilization": ("1", "20", "10"),
            "blinkCancelPause": ("10", "100", "20"),
        }
        for element_id, (minimum, maximum, default) in expected.items():
            tag, attrs = parser.by_id[element_id]
            self.assertEqual(tag, "input")
            self.assertEqual(attrs.get("min"), minimum)
            self.assertEqual(attrs.get("max"), maximum)
            self.assertEqual(attrs.get("value"), default)

    def test_lab_r79_card_is_read_only(self):
        panel_start = DASHBOARD.index('<section class="panel" id="panelLabR79">')
        panel_end = DASHBOARD.index('<section class="panel" id="panelNagModeH">', panel_start)
        panel = DASHBOARD[panel_start:panel_end]
        card = re.search(r'<section class="card">(.*?)</section>', panel, re.S)
        self.assertIsNotNone(card)
        self.assertIsNone(re.search(r"<(?:input|select|button)\b", card.group(1)))

    def test_production_routes_are_wired(self):
        for token in (
            "/api/r79/update?bit18Mode=",
            "/api/blinkA/timing?noaStabilizationSeconds=",
            "tsl9InputMode",
        ):
            self.assertIn(token, DASHBOARD)

    def test_home_blinker_distinguishes_session_states(self):
        for label in (
            "STANDBY", "STABILIZATION", "READY", "EXIT WAIT", "PAUSED",
        ):
            self.assertIn(label, DASHBOARD)
        self.assertIn("function renderHomeBlinkerState(", DASHBOARD)
        self.assertGreaterEqual(
            DASHBOARD.count("renderHomeBlinkerState("), 3,
            "the helper definition plus detail and HOME render paths must share one mapping",
        )


if __name__ == "__main__":
    unittest.main()
