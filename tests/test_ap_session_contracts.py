"""Ap Session contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class ApSessionContracts(unittest.TestCase):

    def test_ap_display_hold(self):
        """Retained from test_v38_ap_display_hold_static.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
        LOGIC = (ROOT / "vehicle_logic.h").read_text(encoding="utf-8")
        RUNTIME = (ROOT / "can_runtime.h").read_text(encoding="utf-8")
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


        assert '#include "ap_display_hold_pure.h"' in INO
        assert "static ApDisplayHoldPure apDisplayHold" in LOGIC
        assert "apDisplayObserveValidPure(apDisplayHold, dasState4);" in LOGIC
        assert "apDisplayBeginHardInitPure(apDisplayHold" in RUNTIME

        home_fast_start = API.index("static String homeFastSnapshotToJson() {")
        home_fast_end = API.index("static String homeSlowSnapshotToJson() {", home_fast_start)
        home_fast = API[home_fast_start:home_fast_end]
        assert 'jw.boolean("dasStateValid", dasValid);' in home_fast
        assert 'jw.u32("dasState", dasState4);' in home_fast
        assert 'jw.boolean("displayStateValid", apDisplay.valid);' in home_fast
        assert 'jw.u32("displayState", apDisplay.state);' in home_fast
        assert 'jw.boolean("canReinitializing", canReinitializing);' in home_fast

        render_start = DASH.index("function renderHomeFast(s){")
        render_end = DASH.index("function renderHomeSlow(s){", render_start)
        render = DASH[render_start:render_end]
        assert "renderApPresentation(b.displayState??b.dasState,b.displayStateValid??b.dasStateValid)" in render
        assert "nagActive=!!n.apActive" in render

        # Keep the explicitly approved LAB mockup: left-aligned driving state and three
        # separate rounded cells. The global brand remains the later TESLA UNLOCK edit.
        assert '<div class="logo">TESLA UNLOCK</div>' in DASH
        assert '<section class="card labDrivingCard">' in DASH
        assert 'body.t2-2027 .page[data-page="lab"] .labDrivingTop{display:flex;align-items:flex-start;justify-content:space-between;gap:12px}' in DASH
        assert 'body.t2-2027 .page[data-page="lab"] .labDirectionRow{display:grid;grid-template-columns:1fr 1.1fr 1fr;gap:6px;' in DASH
        assert 'body.t2-2027 .page[data-page="lab"] .labDirectionCell{min-width:0;min-height:76px;padding:12px;border:1px solid var(--line);border-radius:18px' in DASH

        print("PASS v3.8 Hard Init AP display hold and approved LAB contract")

    def test_fixed_confirm_free_route(self):
        """Retained from test_v36b14_static.py."""
        from pathlib import Path
        ROOT=Path(__file__).resolve().parents[1]
        ino=next(ROOT.glob('*.ino')).read_text()
        core=(ROOT/'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        runtime=(ROOT/'can_runtime.h').read_text()
        logic=(ROOT/'vehicle_logic.h').read_text()
        api=(ROOT/'web_api.h').read_text()
        dash=(ROOT/'dashboard_source.html').read_text()
        ulc=(ROOT/'ulc_compositor_pure.h').read_text()
        # Confirm-Free is fixed to the production CAN-B 0x3F8 compositor.
        assert 'ulcNoConfirmBusSelectablePure' not in ulc
        assert 'static void lab3f8ObserveCanB' in logic
        assert 'static void injectDriverAssistControl' in logic
        assert 'stalkConfirmRoute' in api
        assert 'stalkConfirmBusSelectable' not in api
        assert 'ulcNoConfirmEffectiveTargetBus()' not in api

        # Mode H engine contract is covered by the current b19 regression; the retired alternate engine is intentionally absent.
        assert not (ROOT/'nag_human_pure.h').exists()
        assert 'Mode H' in dash and 'Mode H · Rev.' not in dash
        print('fixed-route ULC contract passed')


if __name__ == "__main__":
    unittest.main()
