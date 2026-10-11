"""Package Identity contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class PackageIdentityContracts(unittest.TestCase):

    def test_package_version_identity(self):
        """One owner for directory, sketch, firmware and documentation identity."""
        import re
        from pathlib import Path

        package = Path(__file__).resolve().parents[1]
        match = re.fullmatch(r"T2CAN-Universal-(v(?:0|[1-9][0-9]*)(?:\.(?:0|[1-9][0-9]*)){2,3})-LP_YL", package.name)
        self.assertIsNotNone(match, "Package name must contain a numeric release/build version")
        self.assertTrue(package.is_dir())
        version = match.group(1)
        sketch = package / (package.name + ".ino")
        self.assertEqual(list(package.glob("*.ino")), [sketch], "Exactly one correctly named sketch is required")
        self.assertTrue(sketch.is_file())
        text = sketch.read_text()
        self.assertEqual(re.findall(r'^#define FW_VERSION "([^"\n]+)"$', text, re.M), [version])
        self.assertTrue(text.startswith("// T2CAN Universal " + version))
        self.assertTrue((package / "CHANGELOG.md").read_text().startswith("# T2CAN Universal " + version + "\n"))
        self.assertTrue((package / "VALIDATION.md").read_text().startswith("# T2CAN Universal " + version + " — Validation\n"))

    def test_compile(self):
        """Retained from test_compile_contract.py."""
        from pathlib import Path
        import re

        root = Path(__file__).resolve().parents[1]
        forward = (root / 't2can_forward.h').read_text(encoding='utf-8')

        # t2can_forward.h is included before vehicle_logic.h. The R79 type therefore
        # must be declared before the forward prototype that returns it by value.
        proto = 'static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now);'
        assert proto in forward, 'R79 runtime snapshot prototype missing'
        proto_pos = forward.index(proto)
        prior = forward[:proto_pos]
        assert re.search(r'\bstruct\s+R79RuntimeStatus\s*;', prior), (
            'R79RuntimeStatus must be forward-declared before r79RuntimeStatusSnapshot prototype'
        )
        print('compile contract OK')


        # b14 Mode H reads ALC state from can_core.h, which is included before vehicle_logic.h.
        # Shared state must therefore be defined in t2can_core_state.h, not first declared later.
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
        core_state = (root / 't2can_core_state.h').read_text(encoding='utf-8')
        vehicle_logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        assert ino.index('#include "t2can_core_state.h"') < ino.index('#include "can_core.h"') < ino.index('#include "vehicle_logic.h"')
        for token in (
            'static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;',
            'static volatile uint8_t dasAutoLaneChangeState = 0xFF;',
            'static volatile bool dasAutoLaneChangeStateValid = false;',
        ):
            assert token in core_state, f'shared declaration must precede can_core.h: {token}'
            assert token not in vehicle_logic, f'duplicate late declaration remains in vehicle_logic.h: {token}'
        print('b14 shared-state include-order contract OK')


        # v3.7 web API calls the administrative TX hold helper during profile/reset
        # transitions. It must be defined in can_core.h before web_api.h is included.
        core = (root / 'can_core.h').read_text(encoding='utf-8').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        web = (root / 'web_api.h').read_text(encoding='utf-8')
        assert 'static void setCanTxAdministrativeHold(bool hold)' in core
        assert 'setCanTxAdministrativeHold(true);' in web
        assert ino.index('#include "can_core.h"') < ino.index('#include "web_api.h"')
        print('v3.7 administrative TX hold compile contract OK')

    def test_current_version_and_preserved_features(self):
        """Retained from test_v36b17_version_static.py."""
        from pathlib import Path

        ROOT=Path(__file__).resolve().parents[1]
        ino=next(ROOT.glob('*.ino')).read_text(errors='ignore')
        changelog=(ROOT/'CHANGELOG.md').read_text(errors='ignore')
        validation=(ROOT/'VALIDATION.md').read_text(errors='ignore')
        html=(ROOT/'dashboard_source.html').read_text(errors='ignore')

        assert 'T2CAN_2027_FULL_BLEED' in html
        assert 'T2CAN_V38_DETAIL_COMPONENTS' in html
        assert 'Mode H' in html and 'Mode H · Rev.' not in html
        assert 'Natural Grip' not in html
        assert 'OPPOSITE CARRIER' in html
        print('v3.7.3 version/dashboard contract passed')


    def test_current_version_and_release_files(self):
        """Retained from test_v37_version_contract.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


        assert "nagCfgVersion < 18u" in CORE
        assert 'stored.config = config;' in CORE
        assert "modeHMigrateV17DefaultPure(rev4Cfg)" in CORE
        assert "nagModeHDefaultStopBehaviorPure()" in CORE
        assert "Number(v.ho2ThresholdNm??2).toFixed(2)" in DASHBOARD
        assert "Number(v.stopBehavior??0)" in DASHBOARD

        print("PASS v3.7.3 version and Mode H default contract")

    def test_release(self):
        """Retained from test_v38_release_contract.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        VARIANT = (ROOT / "mode_h_pure.h").read_text(encoding="utf-8")
        DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
        WEB = (ROOT / "web_api.h").read_text(encoding="utf-8")
        CHANGELOG = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
        VALIDATION = (ROOT / "VALIDATION.md").read_text(encoding="utf-8")



        # Fresh NVS, invalid values, and explicit NAG reset all converge on Mode H Rev.4.
        assert "nagModeHDefaultVariantPure" in VARIANT
        assert "return H_VARIANT_REV4;" in VARIANT
        assert "static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();" in CORE
        assert CORE.count("nagCfgDefaultsModeH(nagCfg);") >= 2
        assert 'reader.getUChar("mode", MODE_H)' in CORE
        assert "CheckedPreferences reader;" in CORE
        assert "if (!reader.ok()) {" in CORE
        assert 'reader.getUChar("hv", nagModeHDefaultVariantPure())' in CORE

        # Approved production dashboard refinements.
        assert '<div class="logo">TESLA UNLOCK</div>' in DASH
        assert 'NAG ✓ · ADV EAP ✓ · EU UNLOCK ✓' not in DASH
        assert 'function toast(t)' not in DASH
        assert 'id="toast"' not in DASH
        assert 'liveDetailsCard' not in DASH
        assert 'homeLiveDetails' not in DASH
        assert '&live=1' not in DASH
        assert 'homeLiveSnapshotToJson' not in WEB
        assert 'if (groups == "home-lite")' in WEB
        assert 'body.t2-2027 .quickMini .toggle{margin-top:15px}' in DASH
        assert "+'\\n'+fmtMs(age)" in DASH
        assert "white-space:pre-line" in DASH

        print("PASS v3.29.1 release identity, Rev.4 defaults, and dashboard contract")




if __name__ == "__main__":
    unittest.main()
