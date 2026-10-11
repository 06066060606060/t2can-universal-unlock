"""Dashboard Profiles contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class DashboardProfilesContracts(unittest.TestCase):

    def test_dashboard_compact_profile(self):
        """Retained from test_dashboard_compact_profile_static.py."""
        from pathlib import Path
        import re


        root = Path(__file__).resolve().parents[1]
        dashboard = (root / "dashboard_source.html").read_text()

        assert "profileCompact" in dashboard
        assert dashboard.count('class="profileCompactMain"') == 1
        assert dashboard.count('class="profileChips"') == 1
        assert 'class="profileChangeInline" id="changeProfileBtn"' in dashboard
        assert "activeProfileCaps" in dashboard
        assert "activeProfileTarget" not in dashboard
        assert ".profileCell" not in dashboard
        assert "profileGrid" not in dashboard

        mobile_rules = re.findall(r"body\.tu-mobile-only \.profile[^\{]*\{([^}]*)\}", dashboard)
        assert all("grid-template-columns" not in rule for rule in mobile_rules)
        assert all("margin:" not in rule or "profileCompact" in rule for rule in mobile_rules)
        print("compact firmware profile contract: PASS")

    def test_dashboard_feature_promotion(self):
        """Retained from test_dashboard_feature_promotion_static.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        dashboard = (root / "dashboard_source.html").read_text()

        for required in [
            "panelUlc", "panelPedalMap", "tsl9InputMode",
            "/api/ulc/stats", "/api/nag/h-profile/stats",
            "blinkTxMode", "350 ms BURST", "/api/blinkA/tx-mode",
            "/api/lab/auto-lane-change/stats",
            "R79 bit18", "NOA Start Wait", "Cancel Wait",
            "ISA Suppression", "isaSuppressionToggle", "/api/isa-suppression/config",
            "driverMonitoringToggle", "/api/driver-monitoring/config",
            "Visual Speed Control", "/api/vision-control/stats", "/api/r79/update?bit18Mode=",
        ]:
            assert required in dashboard, f"missing promoted dashboard contract: {required}"

        for removed in [
            "panelLabLaneGraph", "panelLabParkedInjection", "panelLabCanARx", "panelLabUlcMonitor",
            "/api/lab/ulc-monitor/",
            "panelLabAcc", "labUlcSpeed", "labUlcNoConfirmBus",
            "panelLabTlsscGreen", "/api/lab3f8/", "/api/nag-human-lab/",
            "panelLabBlinkerTx", "/api/lab/blinker-tx/",
            "tsl9ScrollAssistWrap",
            "panelApRightScroll", "/api/ap-right-scroll",
            "ACC Follow Distance", "TLSSC Green-Light Experiment",
            "nagDmsToggle", "panelLabVisionControl",
            "tsl9IsaToggle", "tsl9IsaChimeSuppress",
        ]:
            assert removed not in dashboard, f"obsolete dashboard feature remains: {removed}"

        assert "bit 56" in dashboard and "bit 15" in dashboard
        assert "BUS OFF" in dashboard and "Single TX" in dashboard
        print("dashboard feature promotion contract: PASS")

    def test_dashboard_profile_visibility(self):
        """Retained from test_dashboard_profile_visibility_regression.py."""
        from pathlib import Path
        import re

        ROOT = Path(__file__).resolve().parents[1]
        DASH = (ROOT / 'dashboard_source.html').read_text()

        # Profile visibility must remain authoritative in the current dashboard skin.
        assert '.hiddenByProfile{display:none!important}' in DASH, \
            'global profile-hidden contract must remain authoritative'

        # Layout rules must not force display with !important, or they can override
        # the profile-hidden contract through selector specificity.
        quick_blocks = re.findall(r'body\.t2-2027 \.quickMini\{([^}]*)\}', DASH)
        nav_blocks = re.findall(r'body\.t2-2027 \.navbtn\{([^}]*)\}', DASH)
        assert quick_blocks and any('display:flex' in b for b in quick_blocks)
        assert nav_blocks and any('display:grid' in b for b in nav_blocks)
        # Any !important display declaration on the profile-hideable tile/button itself
        # can outrank .hiddenByProfile because the mobile selector is more specific.
        display_important = re.compile(r'display\s*:\s*[^;!}]+\s*!important', re.I)
        assert all(not display_important.search(b) for b in quick_blocks), \
            'mobile quick controls must not force any display value with !important'
        assert all(not display_important.search(b) for b in nav_blocks), \
            'mobile nav buttons must not force any display value with !important'
        assert '#homeQuickStrip .quickMini.hiddenByProfile' not in DASH, \
            'selector-specific Quick Control hide workaround should not return'
        assert '#bottomNav .navbtn.hiddenByProfile' not in DASH, \
            'selector-specific nav hide workaround should not return'

        # The floating nav uses flex sizing and hidden profile entries are removed.
        assert 'body.t2-2027 .bottomnav.labHidden,body.t2-2027 .bottomnav.devicesHidden' in DASH
        assert 'body.t2-2027 .hiddenByProfile' in DASH

        # Empty S3XY registry remains a normal user-facing state.
        assert "if(!devs.length){line.textContent='Ready to add an S3XY Button'" in DASH
        assert 'No paired S3XY buttons.<br>Tap + Add Button to get started.' in DASH
        assert 'Bluetooth is off.<br>Saved button settings remain preserved.' in DASH

        print('dashboard profile/BLE visibility regression checks passed')

    def test_production_nag_dashboard_promotion(self):
        """Retained from test_dashboard_v3130_promotion.py."""
        from pathlib import Path
        s=(Path(__file__).resolve().parents[1]/'dashboard_source.html').read_text()
        assert 'id="nagIgnoreApStateToggle"' in s, 'Torque AP bypass control missing'
        assert "toggleNag(e.target.checked,'ignoreApState')" in s
        assert 'data-panel="panelCountry"' in s and '/api/country/stats' in s
        assert 'panelLabCountry' not in s and '/api/lab/country/' not in s
        for removed in ['modeHRev1','modeHRev3','modeHRev4','Rev.4','REV.4','Rev.1','Rev.2','Rev.3','MODE H · BETA','id="humanV3TuningWrap"','id="humanV2SummaryWrap"']:
            assert removed not in s, removed
        settings=s.split('<main class="page" data-page="settings"',1)[1].split('</main>',1)[0]
        assert 'data-panel="panelCountry"' in settings
        print('v3.29.1 dashboard promotion: PASS')

    def test_removed_lab(self):
        """Retained from test_dashboard_v3220_removed_lab_static.py."""
        from pathlib import Path
        ROOT = Path(__file__).resolve().parents[1]
        html = (ROOT / "dashboard_source.html").read_text()
        api = (ROOT / "web_api.h").read_text()
        for token in ["panelLabLaneGraph", "panelLabParkedInjection", "panelLabCanARx", "panelLabUlcMonitor", "fetchLaneGraphLab", "fetchParkedInjectionLab", "updateCanARxLab", "fetchUlcMonitorLab"]:
            assert token not in html, f"removed LAB feature remains: {token}"
        for route in ["/api/lab/lane-graph/", "/api/lab/parked-injection/", "/api/lab/can-a-rx/", "/api/lab/ulc-monitor/"]:
            assert route not in api, f"removed LAB API remains: {route}"
        assert 'id="labUlcBlind"' in html and 'server.hasArg("blind")' in api
        assert 'id="panelLabVisionControl"' not in html
        assert 'id="panelVisionControl"' in html
        assert 'data-panel="panelVisionControl"' in html
        print("PASS removed LAB surfaces and preserved blind-spot injection / production Visual Speed Control")

    def test_party_chassis_tsl9_hidden(self):
        """Retained from test_party_chassis_tsl9_hidden.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        dashboard = (root / "dashboard_source.html").read_text()
        profile = (root / "vehicle_profile.h").read_text()
        preview = (root / "tools" / "v38_preview_mock.js").read_text()
        api = (root / "web_api.h").read_text()
        core = (root / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())

        support = profile.split("static inline bool vehicleProfileNagTsl9Supported", 1)[1].split("\n}", 1)[0]
        assert "vehicleProfileCanAIsBody(id, topology)" in support
        assert "vehicleProfileCanBIsChassis(id, topology)" not in support
        assert "me.options[1].hidden=!ss" in dashboard
        assert "$('tsl9SequenceWrap').classList.toggle('tuUiHidden',!tsl9||!ss)" in dashboard
        assert "nagTsl9Supported=topology!==3" in preview
        assert "requested == NAG_METHOD_TSL9_PURE && !activeProfileNagTsl9Supported()" in api
        assert "nagCfgApplyActiveProfilePolicy(nagCfg, true)" in core

        print("PASS Party + Chassis TSL9 option hidden")

    def test_body_tsl9_ui(self):
        """Retained from test_v382_body_tsl9_ui_regression.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
        PURE = (ROOT / "tsl9_hands_on_0x399_pure.h").read_text(encoding="utf-8")

        assert "nagMethodResolveForCapabilitiesPure" in PURE
        assert "static bool nagCfgApplyActiveProfilePolicy" in CORE
        assert "profileMethodMigrated = nagCfgApplyActiveProfilePolicy(nagCfg, true)" in CORE
        assert 'stored.config = config;' in CORE
        assert "nagCfgApplyActiveProfilePolicy(nc, false);" in API
        assert "nagCfgApplyActiveProfilePolicy(nc, true);" in API

        # Backend resolves stale Torque before the dashboard config is loaded; the UI
        # must lock the single valid method without disabling the master toggle.
        assert "me.disabled=!(ts&&ss)" in DASH
        assert "$(id).disabled=!window.__t2NagSupported" in DASH
        assert "function nagTsl9RouteInfo()" in DASH
        assert "Body CAN A · 0x39B" in DASH
        assert "id=\"tsl9Window\"" in DASH

        # Android/mobile LAB regression: unknown gear is compact and columns cannot overlap.
        assert "?'N/A':gearRaw" in DASH
        assert ".labGear{flex:0 0 54px" in DASH
        assert "text-overflow:ellipsis" in DASH

        print("PASS v3.8.2 Body+Chassis TSL9 recovery and LAB gear layout regression")

    def test_nag_right_scroll_relocation(self):
        """Retained from test_v382_nag_right_scroll_relocation.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

        settings_start = DASH.index('<main class="page" data-page="settings">')
        settings_end = DASH.index('</main>', settings_start)
        settings = DASH[settings_start:settings_end]
        nag_start = DASH.index('<section class="panel" id="panelNag">')
        nag_end = DASH.index('</div></section>\n<section class="panel" id="panelBlink">', nag_start)
        nag = DASH[nag_start:nag_end]

        assert 'data-panel="panelApRightScroll"' not in settings
        assert 'data-panel="panelApRightScroll"' not in nag
        assert 'id="panelApRightScroll"' not in DASH
        assert 'id="tsl9InputMode"' in nag
        assert 'id="tsl9IsaToggle"' not in nag
        assert 'id="isaSuppressionToggle"' in settings
        assert 'id="nagDmsToggle"' not in nag
        assert 'id="driverMonitoringToggle"' in settings
        print("PASS AP Right Scroll retired, TSL9 input stays in Nag, and DMS/ISA are standalone")


if __name__ == "__main__":
    unittest.main()
