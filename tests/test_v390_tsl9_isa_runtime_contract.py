import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class Tsl9IsaRuntimeContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.core = (ROOT / "can_core.h").read_text()
        cls.api = (ROOT / "web_api.h").read_text()
        cls.logic = (ROOT / "vehicle_logic.h").read_text()
        cls.state = (ROOT / "t2can_core_state.h").read_text()

    def test_saved_toggle_and_composed_runtime_transform(self):
        self.assertNotIn("tsl9IsaChimeSuppress", self.core)
        self.assertIn("isaSuppressionEnabled", self.state)
        self.assertIn('getBool("isaSuppress"', self.logic)
        self.assertIn('putBool("isaSuppress"', self.logic)
        self.assertIn('getBool("tsl9isa"', self.logic)
        self.assertGreaterEqual(
            self.core.count("tsl9ApplyDasTransformForCanIdPure("), 2)
        self.assertNotIn("tsl9ApplyHandsOnDowngradeForCanIdPure(\n      nagTsl9State", self.core)
        self.assertGreaterEqual(
            self.core.count("&isaSuppressionGeneration, isaSuppressionGenerationSnapshot"),
            2,
        )
        self.assertIn("canTxMcpSendTaggedGuarded(", self.core)
        self.assertIn("canTxTwaiTransmitWithMaskTaggedGuarded(", self.core)

    def test_api_reports_and_updates_toggle(self):
        self.assertNotIn('"tsl9IsaChimeSuppress"', self.api)
        self.assertIn('server.on("/api/isa-suppression/config", HTTP_GET', self.api)
        self.assertIn('server.on("/api/isa-suppression/config", HTTP_POST', self.api)
        self.assertIn("httpIsaSuppressionControlUpdate", self.api)

    def test_dms_migration_failure_does_not_block_saved_isa_restore(self):
        load = self.logic.split("static void featureCfgLoad()", 1)[1].split(
            "static bool blinkerTxModePersist", 1
        )[0]
        dms = load.split("if (hasDriverMonitoringControl)", 1)[1].split(
            "if (hasIsaSuppressionControl)", 1
        )[0]
        self.assertNotIn('if (!oldNag.begin("nag", false)) return;', dms)


if __name__ == "__main__":
    unittest.main()
