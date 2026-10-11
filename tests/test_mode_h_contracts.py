"""Mode H contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class ModeHContracts(unittest.TestCase):

    def test_mode_h_module(self):
        """Retained from test_mode_h_module_contract.py."""
        from pathlib import Path
        import re

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob("*.ino")).read_text()
        legacy = ("nag_human_v1_pure.h", "nag_human_v2_pure.h", "nag_human_v3_pure.h", "nag_human_v4_pure.h", "nag_mode_h_variant_pure.h")
        assert not any(name in ino for name in legacy), "Production sketch still owns the legacy Mode H include chain"
        assert '#include "mode_h_pure.h"' in ino
        pure = (ROOT / "mode_h_pure.h").read_text()
        assert not any(name in pure for name in legacy), "Current engine must be self-contained"
        assert not re.search(r"\b(?:NagHumanV[134]|nagHumanV[134]|NAG_HUMAN_V[134])", pure)
        core = (ROOT / "can_core.h").read_text()
        runtime = (ROOT / "mode_h_runtime.h").read_text()
        assert '#include "mode_h_runtime.h"' in core
        assert 'static ModeHStatePure modeHState' in runtime
        assert 'static ModeHStatePure modeHState' not in core
        assert 'modeHStepPure(' in runtime
        assert 'H_VARIANT_REV4 = 1' in pure

        # Frozen independent v3.28.3 oracle: edits require an explicit new baseline audit.
        import hashlib
        ORACLE_SHA256 = {'nag_human_v1_pure.h': 'bfb42563768075d7e611ba8f655587b9ac2462b7369c337bc6a00166815c038f', 'nag_human_v2_pure.h': '115dd09b41c410f09488f6b3cf6e321537a775e4c3eb62f203569056920b11f1', 'nag_human_v3_pure.h': 'cb0137647dad99ede4ec93ed8758a41a8f10228fb250c494ef186ddc33b8b6a1', 'nag_human_v4_pure.h': '1c3d95e867f23ce5332b68e115cc4c03f9101445b55ac5944f983a43fd1398f1', 'nag_mode_h_variant_pure.h': 'f4d6c296d7def5ef607552e6ce55e14f4b7986e8673f6ac6121ebaf1c299c837'}
        for name, digest in ORACLE_SHA256.items():
            assert hashlib.sha256((ROOT / "tests/fixtures/mode_h_legacy" / name).read_bytes()).hexdigest() == digest, name
        for path in ROOT.glob("*.h"):
            text = path.read_text()
            assert "fixtures/mode_h_legacy" not in text, path.name
            assert not any('#include "' + name + '"' in text for name in legacy), path.name

        print("Mode H production ownership and frozen oracle contract: PASS")

    def test_nag_default_off(self):
        """Retained from test_v313_nag_default_off.py."""
        """Default-off persistence contract; preserve explicitly stored enabled values."""
        from pathlib import Path
        import re
        import unittest

        SOURCE = (Path(__file__).resolve().parents[1] / 'can_core.h').read_text()


        def body(name):
            start = SOURCE.index('static void ' + name + '(')
            begin = SOURCE.index('{', start)
            depth = 1
            end = begin + 1
            while depth:
                depth += (SOURCE[end] == '{') - (SOURCE[end] == '}')
                end += 1
            return SOURCE[begin + 1:end - 1]


        class NagDefaultOff(unittest.TestCase):
            def test_common_default_is_off(self):
                common = body('nagCfgSetCommonDefaults')
                self.assertRegex(common, r'c\.enabled\s*=\s*false\s*;')

            def test_new_namespace_and_missing_schema_use_common_defaults(self):
                load = body('nagCfgLoad')
                prefix = load[:load.index('const uint8_t nagCfgVersion')]
                self.assertIn('if (!reader.begin("nag", true))', prefix)
                self.assertIn('if (!reader.isKey("v"))', prefix)
                self.assertIn('nagCfgLoadSafeDefaults();', prefix)
                self.assertIn('nagCfgDefaultsModeH(nagCfg);', prefix)
                self.assertIn('nagCfgSetCommonDefaults(c);', body('nagCfgDefaultsModeH'))

            def test_missing_enabled_key_defaults_off(self):
                load = body('nagCfgLoad')
                self.assertRegex(load, r'nagCfg\.enabled\s*=\s*reader\.getBool\("en",\s*false\);')

            def test_existing_saved_true_and_false_are_read_without_override(self):
                load = body('nagCfgLoad')
                assignments = re.findall(r'nagCfg\.enabled\s*=\s*([^;]+);', load)
                self.assertEqual(assignments, ["savedEnabled", 'reader.getBool("en", false)'])
                self.assertIn('const bool savedEnabled = reader.getBool("en", false);', load)

            def test_save_keeps_enabled_snapshot(self):
                self.assertIn('stored.config = config;', SOURCE)
                self.assertIn('nvsCheckedWriteBlob("nag", "snapshot", &stored, sizeof(stored))', SOURCE)
        run_contract_cases(self, [NagDefaultOff], expected=5)

    def test_single_mode_h_tuning_and_retired_variants(self):
        """Retained from test_v36b20_contract_static.py."""
        from pathlib import Path
        import re

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(errors='ignore')
        core = (root/'can_core.h').read_text(errors='ignore').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        api = (root/'web_api.h').read_text(errors='ignore')
        dash = (root/'dashboard_source.html').read_text(errors='ignore')
        forward = (root/'t2can_forward.h').read_text(errors='ignore')
        v2 = (root/'tests/fixtures/mode_h_legacy/nag_human_v2_pure.h').read_text(errors='ignore')
        v3 = (root/'tests/fixtures/mode_h_legacy/nag_human_v3_pure.h').read_text(errors='ignore')
        v4 = (root/'mode_h_pure.h').read_text(errors='ignore')
        variants = (root/'mode_h_pure.h').read_text(errors='ignore')

        assert '#include "mode_h_pure.h"' in ino
        assert '#include "mode_h_pure.h"' in ino

        # One production Mode H with stable persisted slot 1 and existing h4 tuning.
        assert 'H_VARIANT_REV4 = 1' in variants
        for token in ['H_VARIANT_REV1', 'H_VARIANT_REV2', 'H_VARIANT_REV3']:
            assert token not in variants + core + api, token
        assert 'stored.variant = nagModeHVariantValidPure(variant)' in core
        retired_handler=api.split('static void httpNagSetHumanVariant()',1)[1].split('static void httpNagUpdate()',1)[0]
        assert 'server.send(410' in retired_handler and 'Mode H profiles retired' in retired_handler
        assert 'nagHumanVariant =' not in retired_handler
        for key in ['h4pmin','h4pmax','h4wmin','h4wmax','h4rmin','h4rmax','h4cmin','h4cmax','h4hop','h4ho1','h4ho2','h4vres','h4vdly']:
            assert '"'+key+'"' in core, key
        assert 'modeHRuntimeSetLabTuning' in api and 'modeHRuntimeConfigSnapshot' in api

        # Mode H is one promoted production profile; historical selector is removed.
        for token in ['id="modeHVariantWrap"', 'id="modeHRev1"', 'id="modeHRev3"', 'id="modeHRev4"', 'setModeHVariant']:
            assert token not in dash, token
        assert 'async function fetchNagHumanLab' in dash

        # Production runtime dispatch is exclusively the preserved Mode H engine.
        assert 'modeHStepPure' in core
        for retired in ['nagHumanV1StepPure','nagHumanV2StepPure','nagHumanV3StepPure']:
            assert retired not in core, retired
        assert 'modeHDefaultConfigPure' in v4
        # b20 Rev.2 is a clean Natural Grip Peak engine; the failed b18 warning-reactive
        # branch/presets are intentionally not restored.
        for retired in ['NagHumanV2PresetPure', 'PREWARN', 'VISUAL_RETRY', 'warningLeftBiasPct']:
            assert retired not in v2, retired

        # b19 production work remains present.
        assert 'id="apDriveProfileSettingsRow"' in dash
        assert 'body.t2-2027 .quickMini .toggle' in dash

        print('PASS v3.6b20 static contract')

    def test_mode_h_hands_on_and_fixed_r79(self):
        """Retained from test_v36c2_contract_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
        variant = (root / 'mode_h_pure.h').read_text(encoding='utf-8')
        core = (root / 'can_core.h').read_text(encoding='utf-8').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        vehicle = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        web = (root / 'web_api.h').read_text(encoding='utf-8')
        dash = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        fixed = (root / 'r79_fixed_policy_pure.h').read_text(encoding='utf-8')

        assert 'H_VARIANT_REV3' not in variant
        assert '#include "mode_h_pure.h"' in ino
        assert 'nagHumanV3StepPure' not in core
        assert 'modeHStepPure' in core
        assert 'hoOverrideValid' in core and 'hoLevel' in core
        assert 'txf.data[4] = (uint8_t)((txf.data[4] & 0x3Fu)' in core

        assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
        assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
        assert 'r79FixedFastEcho' in vehicle and 'r79FixedTick' in vehicle
        assert '/api/r79/stats' in web and '/api/r79/update' in web
        assert 'R79 Status' in dash and 'Read-only production status' in dash

        assert 'modeHRev3' not in dash
        assert 'humanV3Direction' not in dash
        assert 'humanV3HoPolicy' not in dash
        assert 'humanV3Ho1Nm' not in dash
        assert 'humanV3Ho2Nm' not in dash
        print('v3.7 Rev.3 / fixed R79 contract OK')

    def test_defaults(self):
        """Retained from test_v36d4_defaults_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text(errors='ignore')
        logic = (root / 'vehicle_logic.h').read_text(errors='ignore')
        core = (root / 'can_core.h').read_text(errors='ignore').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        v3 = (root / 'tests/fixtures/mode_h_legacy/nag_human_v3_pure.h').read_text(errors='ignore')
        fixed = (root / 'r79_fixed_policy_pure.h').read_text(errors='ignore')
        dash = (root / 'dashboard_source.html').read_text(errors='ignore')

        assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
        assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
        assert 'R79_BIT18_DEFAULT_PURE = R79_BIT18_STOCK_PURE' in fixed
        assert 'r79FixedFastEcho' in logic and 'r79FixedTick' in logic
        assert 'static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();' in core
        assert 'reader.getUChar("hv", nagModeHDefaultVariantPure())' in core
        for token in ['c.base.peakMaxRaw = 210u', 'c.base.waitMaxMs = 2500u',
                      'c.ho1ThresholdRaw = 40u', 'c.ho2ThresholdRaw = 175u',
                      'H3_CARRIER_FOLLOW_STOCK', 'H3_HO_TIERED_1_2']:
            assert token in v3, token
        assert 'Mode H' in dash
        assert 'R79 Status' in dash
        print('PASS v3.8 fixed defaults static')

    def test_mode_h_stop_carrier(self):
        """Retained from test_v36d5_mode_h_stop_carrier_static.py."""
        from pathlib import Path
        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob('*.ino')).read_text()
        core = (ROOT/'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        web = (ROOT/'web_api.h').read_text()
        dash = (ROOT/'dashboard_source.html').read_text()
        variant = (ROOT/'mode_h_pure.h').read_text()

        assert 'H_STOP_HARD_PAUSE' in variant and 'H_STOP_STOCK_CARRIER' in variant
        assert 'c.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();' in core
        assert 'const uint8_t storedStopDefault = nagCfgVersion < 18u' in core
        assert 'reader.getUChar("hsb", storedStopDefault)' in core
        assert 'stored.config = config;' in core
        assert 'stored.config = config;' in core
        assert 'nagModeHUseStopCarrierPure(modeHStopBehavior, confirmedStopped)' in core
        assert 'humanDecision.raw = tRaw;' in core
        assert 'humanDecision.hoOverrideValid = false;' in core
        assert 'modeHStopCarrier = true;' in core
        assert 'if (modeHStopCarrier) nagStopCarrierTxOk++;' in core
        assert 'modeHStopBehavior' in web and 'stopCarrierTxOk' in web
        assert 'id="humanStopBehavior"' in dash
        assert 'STOCK CARRIER' in dash and 'HARD PAUSE · 0-TX' in dash
        assert "modeHStopBehavior='+v" in dash
        print('v3.6d9a2 Mode H stop-carrier static contract: PASS')

    def test_mode_h_dashboard(self):
        """Retained from test_v36e2_mode_h_dashboard_static.py."""
        from pathlib import Path
        R=Path(__file__).resolve().parents[1]
        d=(R/'dashboard_source.html').read_text()
        a=(R/'web_api.h').read_text()
        c=(R/'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (R / "mode_h_runtime.h").read_text())
        v4=(R/'mode_h_pure.h').read_text()
        assert 'id="modeHRev4"' not in d and 'REV.4' not in d
        assert 'id="modeHRev1Plus"' not in d
        assert 'id="humanV4CarrierMinNm"' in d and 'id="humanV4CarrierMaxNm"' in d
        assert 'const humanPeakEdit={dirty:false,saving:false,epoch:0}' in d
        assert 'function markHumanPeakDirty()' in d
        assert 'const syncTuning=forceTuning||(!humanPeakEdit.dirty&&!humanPeakEdit.saving)' in d
        assert 'peakEpoch!==humanPeakEdit.epoch||stopEpoch!==humanStopEdit.epoch' in d
        assert 'humanPeakEdit.epoch++' in d
        assert 'H_VARIANT_REV4' in c and 'modeHRuntimeSetLabTuning' in c
        assert 'modeHRuntimeConfigSnapshot' in a and 'OPPOSITE_STOCK_PER_RX' in a
        assert 'MODE_H_STOCK_DEADBAND_RAW = 5u' in v4
        assert 'MODE_H_NEGATIVE_BIAS_PCT = 80u' in v4
        for token in ['c.carrierMinRaw = 10u','c.carrierMaxRaw = 60u','c.base.peakMinRaw = 180u','c.base.peakMaxRaw = 260u','c.base.waitMinMs = 900u','c.base.waitMaxMs = 3000u','c.base.refractoryMinMs = 500u','c.base.refractoryMaxMs = 1500u','c.hoPolicy = MH_HO_TIERED_1_2','c.ho1ThresholdRaw = 40u','c.ho2ThresholdRaw = 200u','c.visualRescueEnabled = true']:
            assert token in v4, token
        print('PASS v3.6f1 Mode H Rev.4/dashboard edit-state contract')

    def test_mode_h_visual_rescue(self):
        """Retained from test_v36f2_mode_h_visual_rescue_contract.py."""
        from html.parser import HTMLParser
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")


        class RescueUiParser(HTMLParser):
            def __init__(self):
                super().__init__()
                self.variant_buttons = []
                self.inputs = {}
                self.selects = {}

            def handle_starttag(self, tag, attrs):
                values = dict(attrs)
                element_id = values.get("id", "")
                if tag == "button" and element_id.startswith("modeHRev"):
                    self.variant_buttons.append(element_id)
                if tag == "input" and element_id:
                    self.inputs[element_id] = values
                if tag == "select" and element_id:
                    self.selects[element_id] = values


        parser = RescueUiParser()
        parser.feed(DASHBOARD)

        # The visible selector is numerical even though persisted IDs stay 0/3/1.
        assert parser.variant_buttons == []

        # Rev.4 exposes a persistent switch and a bounded 0.0..2.0 second delay.
        assert "humanV4VisualRescue" in parser.selects
        delay = parser.inputs["humanV4RescueDelay"]
        assert delay["type"] == "number"
        assert delay["min"] == "0"
        assert delay["max"] == "2"
        assert delay["step"] == "0.1"

        # Dashboard request/response fields use the same public API names.
        for token in ["visualRescueEnabled", "visualRescueDelayMs"]:
            assert token in DASHBOARD
            assert token in API

        # The actual DAS transition timestamp is carried into Rev.4, rather than
        # starting the delay on a later stock 0x370 frame.
        for token in [
            "visualWarningEpoch",
            "visualWarningEnterMs",
            "visualWarningActive",
        ]:
            assert token in CORE

        assert "modeHStepPure(modeHState" in CORE
        assert "visualWarningEpoch, visualWarningEnterMs, visualWarningActive" in CORE

        # NVS defaults/migration/save must cover both new settings.
        for key in ["h4vres", "h4vdly"]:
            assert f'getBool("{key}"' in CORE or f'getUShort("{key}"' in CORE
            assert "stored.human = human;" in CORE

        assert 'stored.config = config;' in CORE

        print("PASS v3.6f3 Mode H Visual Warning Rescue integration contract")

    def test_rev4_defaults_ho(self):
        """Retained from test_v36f4_rev4_defaults_ho_contract.py."""
        from html.parser import HTMLParser
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")


        class Rev4HoUiParser(HTMLParser):
            def __init__(self):
                super().__init__()
                self.inputs = {}
                self.selects = {}
                self.current_select = None

            def handle_starttag(self, tag, attrs):
                values = dict(attrs)
                element_id = values.get("id", "")
                if tag == "input" and element_id:
                    self.inputs[element_id] = values
                elif tag == "select" and element_id:
                    self.current_select = element_id
                    self.selects[element_id] = []
                elif tag == "option" and self.current_select:
                    self.selects[self.current_select].append(values.get("value"))

            def handle_endtag(self, tag):
                if tag == "select":
                    self.current_select = None


        parser = Rev4HoUiParser()
        parser.feed(DASHBOARD)

        assert parser.selects.get("humanV4HoPolicy") == ["0", "1", "2"], (
            "Rev.4 Hands-On Policy selector must expose the same three modes as Rev.3"
        )
        for element_id in ["humanV4Ho1Nm", "humanV4Ho2Nm"]:
            assert element_id in parser.inputs, f"missing Rev.4 input {element_id}"
            field = parser.inputs[element_id]
            assert field.get("type") == "number"
            assert field.get("min") == "0.1"
            assert field.get("max") == "3"
            assert field.get("step") == "0.05"

        for public_name in ["hoPolicy", "ho1ThresholdNm", "ho2ThresholdNm"]:
            assert public_name in API, f"Rev.4 API field missing: {public_name}"
            assert public_name in DASHBOARD, f"Rev.4 dashboard field missing: {public_name}"

        assert "hoPolicyName" in API, "Rev.4 API policy label missing"

        for key in ["h4hop", "h4ho1", "h4ho2"]:
            assert f'getUChar("{key}"' in CORE or f'getUShort("{key}"' in CORE
            assert "stored.human = human;" in CORE

        assert 'nagCfgVersion < 18u' in CORE
        assert 'stored.config = config;' in CORE
        assert 'modeHMigrateV16DefaultPure(rev4Cfg)' in CORE
        assert 'modeHMigrateV17DefaultPure(rev4Cfg)' in CORE

        print("PASS v3.6f4 Rev.4 defaults + Hands-On integration contract")


if __name__ == "__main__":
    unittest.main()
