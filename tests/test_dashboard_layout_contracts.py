"""Dashboard Layout contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class DashboardLayoutContracts(unittest.TestCase):

    def test_home_screen_icon(self):
        """Retained from test_home_screen_icon.py."""
        """Catch missing iOS metadata, invalid PNGs, and stale embedded icon bytes."""
        import gzip
        from html.parser import HTMLParser
        from pathlib import Path
        import re
        import struct
        import unittest

        ROOT = Path(__file__).resolve().parents[1]


        def embedded_bytes(path):
            return bytes(int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", path.read_text()))


        class Head(HTMLParser):
            def __init__(self):
                super().__init__()
                self.icons = []
                self.title = None

            def handle_starttag(self, tag, attrs):
                attrs = dict(attrs)
                if tag == "link" and attrs.get("rel") == "apple-touch-icon":
                    self.icons.append(attrs)
                if tag == "meta" and attrs.get("name") == "apple-mobile-web-app-title":
                    self.title = attrs.get("content")


        class HomeScreenIcon(unittest.TestCase):
            def test_source_and_shipped_dashboard_advertise_local_icon(self):
                for html in ((ROOT / "dashboard_source.html").read_text(),
                             gzip.decompress(embedded_bytes(ROOT / "index_html.h")).decode()):
                    with self.subTest(shipped=html.startswith("<!DOCTYPE html>\n<html")):
                        head = Head()
                        head.feed(html)
                        self.assertEqual(len(head.icons), 1, "Safari cannot discover the home-screen icon")
                        self.assertEqual(head.icons[0].get("href"), "/apple-touch-icon.png")
                        self.assertEqual(head.icons[0].get("sizes"), "180x180")
                        self.assertEqual(head.title, "Tesla Unlock")

            def test_embedded_icon_is_exact_opaque_180px_png(self):
                asset = ROOT / "assets" / "apple-touch-icon.png"
                header = ROOT / "dashboard_icon.h"
                self.assertTrue(asset.is_file(), "home-screen PNG is missing")
                self.assertTrue(header.is_file(), "embedded PNG is missing")
                png = asset.read_bytes()
                self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
                self.assertEqual(struct.unpack(">II", png[16:24]), (180, 180))
                self.assertEqual(png[25], 2, "icon must use opaque RGB")
                self.assertLessEqual(len(png), 40_000)
                self.assertEqual(embedded_bytes(header), png, "firmware icon is stale or truncated")
        run_contract_cases(self, [HomeScreenIcon], expected=2)

    def test_mobile_select_guard(self):
        """Retained from test_v382_mobile_select_guard.py."""
        import gzip
        from pathlib import Path
        import re
        import unittest


        ROOT = Path(__file__).resolve().parents[1]


        def embedded_dashboard() -> str:
            header = (ROOT / "index_html.h").read_text(encoding="utf-8")
            match = re.search(r"INDEX_HTML_GZ\[\].*?=\s*\{(.*?)\};", header, re.S)
            if match is None:
                raise AssertionError("embedded dashboard array is missing")
            payload = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))
            return gzip.decompress(payload).decode("utf-8")


        class MobileSelectGuardContract(unittest.TestCase):
            def test_source_and_embedded_dashboard_block_native_reopen(self):
                documents = {
                    "source": (ROOT / "dashboard_source.html").read_text(encoding="utf-8"),
                    "embedded": embedded_dashboard(),
                }
                for name, html in documents.items():
                    with self.subTest(document=name):
                        self.assertNotIn("previous.focus", html)
                        self.assertIn("document.activeElement.blur()", html)
                        self.assertNotIn("['pointerdown','mousedown','touchstart','click']", html)
                        # Native suppression remains, but opening must follow completed activation.
                        self.assertTrue("select.addEventListener('pointerdown',suppress" in html,
                                        f"{name}: pointerdown must only suppress native selection")
                        self.assertTrue("select.addEventListener('click',activate" in html,
                                        f"{name}: completed click must activate the sheet")
                        suppress = re.search(r"const suppress=event=>\{([^}]+)\}", html)
                        self.assertIsNotNone(suppress, f"{name}: native select guard missing")
                        self.assertNotIn("openSelectSheet", suppress.group(1))
                        self.assertTrue("select.addEventListener('touchend'" in html,
                                        f"{name}: legacy touch needs completed activation")
        run_contract_cases(self, [MobileSelectGuardContract], expected=1)

    def test_demo_layout_parity(self):
        """Retained from test_v38_demo_layout_parity.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


        # The production page has no 50 px mock iPhone status bar. Content therefore
        # starts 8 px below the 48 px fixed header, as it does inside the approved demo.
        assert (
            'body.t2-2027 .page,body.t2-2027 .page[data-page="lab"]'
            '{display:none;max-width:520px;min-height:100vh;min-height:100svh;'
            'min-height:100dvh;'
            'padding:calc(56px + env(safe-area-inset-top)) 18px '
            'calc(120px + env(safe-area-inset-bottom))'
        ) in DASH
        assert 'body.t2-2027 .page[data-page="home"]{padding-top:calc(56px + env(safe-area-inset-top))}' in DASH
        assert 'body.t2-2027 .sectionhead{padding:10px 6px 0;margin:10px 0 8px;align-items:center}' in DASH
        assert 'body.t2-2027 .globaltop .live:before{content:"";width:7px;height:7px;border-radius:50%;background:#e5484d;box-shadow:none}' in DASH
        assert 'body.t2-2027 .globaltop .live.ok:before{background:var(--ok);box-shadow:none}' in DASH
        assert 'body.t2-2027 .globaltop .live.busy:before{background:var(--busy);box-shadow:none}' in DASH

        # The approved R79 card is one horizontal row: copy left, two counters right.
        # Explicit flex direction prevents the legacy homeFeatureCard column rule from
        # leaking into the 2027 layout.
        assert (
            'body.t2-2027 .homeR79Card{grid-area:r79;min-height:0;padding:17px 18px;'
            'display:grid;grid-template-columns:minmax(0,1fr) auto;gap:16px;'
            'align-items:center;text-align:left;flex-direction:row}'
        ) in DASH
        assert (
            'body.t2-2027 .homeInjectionStats{display:grid;'
            'grid-template-columns:repeat(2,52px);gap:10px;text-align:center}'
        ) in DASH

        # CAN cards use their content height like the demo, rather than the older
        # fixed 124/128 px production cards.
        assert 'body.t2-2027 .homeBus{min-height:0;padding:16px 18px' in DASH
        assert 'body.t2-2027 .homeBus{padding:15px;min-height:0}' in DASH

        # Home metrics follow the demo's value-first reading order.
        assert '<div class="metric"><div class="v" id="torque">—</div><div class="k" id="homeNagSignalLabel">Stock torque Nm</div></div>' in DASH
        assert '<div class="metric"><div class="v" id="homeS3xy">0 / 0</div><div class="k">S3XY</div></div>' in DASH
        assert '<div class="metric" id="homeBlinkMetric"><div class="v" id="blinkerState">—</div><div class="k">Blinker</div></div>' in DASH
        assert 'body.t2-2027 .metric .k{margin-top:5px;' in DASH
        assert 'body.t2-2027 .metric .v,body.t2-2027 #homeMetrics .metric .v{margin-top:0;' in DASH
        assert 'body.t2-2027 .homeFeatureV{margin-top:7px;' in DASH
        assert 'body.t2-2027 .homeFeatureLast{margin-top:5px;' in DASH
        assert 'body.t2-2027 .homeInjectionStats .homeMetaV{margin-top:5px;' in DASH
        assert 'body.t2-2027 .homeBusVal{margin-top:14px;' in DASH
        assert 'body.t2-2027 .homeBusMeta{margin-top:6px;' in DASH
        assert 'id="homeCanAName">CAN A</div>' in DASH
        assert 'id="homeCanBName">CAN B</div>' in DASH
        assert "a.dataset.role=s.canA||t.canA.replace('CAN A · ','')" in DASH
        assert "b.dataset.role=s.canB||t.canB.replace('CAN B · ','')" in DASH
        assert "role=$(nameId)?.dataset.role||''" in DASH

        # LAB reproduces the demo's separated three-cell state row and inset list.
        assert (
            'body.t2-2027 .page[data-page="lab"] .labDirectionRow{display:grid;'
            'grid-template-columns:1fr 1.1fr 1fr;gap:6px;align-items:stretch;'
            'margin-top:16px;background:transparent;border-radius:0;overflow:visible}'
        ) in DASH
        assert (
            'body.t2-2027 .page[data-page="lab"] .labDirectionCell{min-width:0;'
            'min-height:76px;padding:12px;border:1px solid var(--line);border-radius:18px'
        ) in DASH
        assert '<section class="card labToolsCard">' in DASH
        assert (
            'body.t2-2027 .page[data-page="lab"] .labToolsCard{padding:6px 18px}' in DASH
            and 'body.t2-2027 .page[data-page="lab"] .labToolsCard .listrow'
                '{min-height:0;padding:13px 0;gap:12px}' in DASH
        )
        assert 'id="labDmsNagValue"' not in DASH
        assert 'id="nagDmsToggle"' not in DASH
        assert 'id="driverMonitoringToggle"' in DASH
        assert 'id="labCanARxValue"' not in DASH
        assert 'id="labCanARxMode"' not in DASH
        assert '<span class="value">EXP</span><span class="chev">›</span>' in DASH

        # The demo footer keeps 0x399 and 0x239 freshness as two independent values.
        assert '<span id="alcStateSub">0x399 —</span>' in DASH
        assert '<span id="lane239AgeSub">0x239 age —</span>' in DASH
        assert "$('lane239AgeSub').textContent=a.lane239Valid?('0x239 age '+fmtMs(a.lane239AgeMs)):'0x239 age —'" in DASH

        print("PASS v3.8 approved-demo layout parity contract")

    def test_lab_reference_parity(self):
        """Retained from test_v38_lab_reference_parity.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")
        INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")

        lab = DASH.split('<main class="page" data-page="lab">', 1)[1].split('</main>', 1)[0]

        # The supplied HTML uses a compact driving card and a separate, inset tool list.
        assert '<div class="labDirectionRow">' in lab
        assert lab.count('class="labDirectionCell') == 3
        assert 'id="alcLeftLaneBig"' not in lab
        assert 'id="alcRightLaneBig"' not in lab
        assert '<div class="sectiontitle">Research Tools</div>' in lab
        assert '<div class="sectionnote">open a dedicated LAB page</div>' not in lab

        # Overview rows show a value and open their controls in a detail page.
        assert 'data-panel="panelLabDmsNag"' not in lab
        assert 'data-panel="panelLabCanARx"' not in lab
        assert 'id="labDmsNagValue"' not in lab
        assert 'id="labCanARxValue"' not in lab
        assert 'id="labDmsNagToggle"' not in lab
        assert 'id="labCanARxMode"' not in lab
        assert 'id="labDmsNagToggle"' not in DASH
        assert 'id="nagDmsToggle"' not in DASH
        assert 'id="driverMonitoringToggle"' in DASH
        assert 'id="labCanARxMode"' not in DASH

        # An old .page[data-page="lab"] #alcStateBig rule has ID specificity.
        # The final LAB rules must explicitly replace its centered legacy geometry.
        assert 'body.t2-2027 .page[data-page="lab"] #alcStateBig.driveStateValue{' in DASH
        assert 'body.t2-2027 .page[data-page="lab"] .labDirectionCell .driveStateValue{' in DASH
        assert 'display:block' in DASH.split('body.t2-2027 .page[data-page="lab"] .labDirectionCell .driveStateValue{', 1)[1].split('}', 1)[0]

        # The reference's Latin font is served locally to the ESP32 dashboard.
        assert 'font-family:"Geist"' in DASH
        assert '/fonts/geist-lab.woff2' in DASH
        assert '/fonts/geist-mono-lab.woff2' in DASH
        assert 'server.on("/fonts/geist-lab.woff2"' in API
        assert 'server.on("/fonts/geist-mono-lab.woff2"' in API
        assert '#include "lab_fonts.h"' in INO

        print("PASS v3.8 supplied LAB HTML parity and legacy-style override")

    def test_production_controls_and_fixed_status(self):
        """Retained from test_v37_dashboard_contract.py."""
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
        run_contract_cases(self, [DashboardV37Contract], expected=6)

    def test_dashboard_owner_visibility(self):
        """Retained from test_v390_dashboard_contract.py."""
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
        run_contract_cases(self, [V390DashboardContract], expected=5)

    def test_browser_harness(self):
        """Retained from test_browser_harness_contract.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        TESTS = ROOT / "tests"


        def test_browser_tests_use_sandbox_safe_shared_runtime():
            runtime = TESTS / "browser_test_runtime.js"
            assert runtime.is_file(), "shared browser runtime is required"

            runtime_source = runtime.read_text()
            assert "chromium.launch" in runtime_source
            assert "--single-process" in runtime_source
            assert "--no-zygote" in runtime_source
            assert "NetworkServiceInProcess" in runtime_source
            assert "/Applications/Google Chrome.app" not in runtime_source

            for name in ("test_dashboard_2027_browser.js", "test_nag_layout_browser.js"):
                source = (TESTS / name).read_text()
                assert "require('./browser_test_runtime')" in source
                assert "/Applications/Google Chrome.app" not in source

            dashboard_source = (TESTS / "test_dashboard_2027_browser.js").read_text()
            assert "require('node:http')" not in dashboard_source
            assert "server.listen(" not in dashboard_source
            assert "page.route(" in dashboard_source



        test_browser_tests_use_sandbox_safe_shared_runtime()

        print("sandbox-safe browser harness contract: PASS")


if __name__ == "__main__":
    unittest.main()
