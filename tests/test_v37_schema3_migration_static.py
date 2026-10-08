from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class Schema3MigrationContract(unittest.TestCase):
    def test_schema_and_production_keys(self):
        text = (ROOT / "vehicle_logic.h").read_text()
        self.assertIn("NVS_SCHEMA_CURRENT = 3", text)
        self.assertIn('p.begin("r79", false)', text)
        self.assertIn('putUChar("bit18"', text)
        self.assertIn('putUChar("noaStabS"', text)
        self.assertIn('putUChar("cancelPauseS"', text)
        self.assertNotIn('putUChar("rsWarnS"', text)

    def test_marker_follows_readback_and_cleanup_follows_marker(self):
        text = (ROOT / "vehicle_logic.h").read_text()
        start = text.index("static bool featureConfigMigrateToSchema3()")
        end = text.index("static FeatureConfigMigrationPure featureConfigRuntimeSnapshot", start)
        block = text[start:end]
        self.assertLess(block.index("featureConfigSchema3ReadBackMatches"),
                        block.index('putUShort("schema", NVS_SCHEMA_CURRENT)'))
        self.assertLess(block.index('getUShort("schema", 0)'),
                        block.rindex("featureConfigCleanupRetiredR79Keys"))

    def test_all_retired_r79_keys_are_cleaned(self):
        text = (ROOT / "vehicle_logic.h").read_text()
        for key in (
            "smart18", "strategy", "period", "refresh", "sched", "shots",
            "postTx", "qAnchor", "post2En", "post2Off", "preEn", "preOff",
            "d2FastEcho", "d4Period", "d6Quiet",
        ):
            self.assertIn(f'remove("{key}")', text)

    def test_r79_uses_production_interface_only(self):
        logic = (ROOT / "vehicle_logic.h").read_text()
        api = (ROOT / "web_api.h").read_text()
        ino = next(ROOT.glob("*.ino")).read_text()
        merged = logic + api + ino
        self.assertIn("r79CfgLoad()", merged)
        self.assertIn("r79CfgSave()", merged)
        self.assertNotIn("r79LabCfgLoad", merged)
        self.assertNotIn("r79LabCfgSave", merged)

    def test_settings_reset_restores_v37_defaults_without_busoff_clear(self):
        text = (ROOT / "web_api.h").read_text()
        start = text.index("static void httpResetFirmwareSettings()")
        end = text.index("static void httpFactoryReset()", start)
        block = text[start:end]
        self.assertIn("featureConfigV37DefaultsPure", block)
        self.assertIn("featureConfigWriteSchema3Values", block)
        self.assertIn("autoBlinkerNoaSessionResetPure", block)
        self.assertIn("autoBlinkerCancelPauseResetPure", block)
        self.assertNotIn("canBusOffPersistenceReset", block)


if __name__ == "__main__":
    unittest.main()
