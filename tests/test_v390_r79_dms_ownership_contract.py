import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


def block(text: str, start: str, end: str) -> str:
    begin = text.rindex(start)
    finish = text.index(end, begin)
    return text[begin:finish]


class R79DmsOwnershipContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ino = (ROOT / "T2CAN-Universal-v3.26.3-LP_YL.ino").read_text()
        cls.core = (ROOT / "can_core.h").read_text()
        cls.logic = (ROOT / "vehicle_logic.h").read_text()
        cls.api = (ROOT / "web_api.h").read_text()

    def test_dms_is_standalone_production_policy(self):
        self.assertIn('#include "r79_dms_composition_pure.h"', self.ino)
        self.assertNotIn("bool     dmsControlEnabled;", self.core)
        self.assertIn('getBool("dms43", migrated)', self.logic)
        self.assertIn('putBool("dmsDisable", enabled)', self.logic)
        active = block(self.logic, "static bool r79DmsControlActive()", "static inline bool r79DmsApplyFinal")
        self.assertNotIn("labMenuEnabled", active)
        self.assertNotIn("nagCfg.", active)
        self.assertIn("driverMonitoringControlSnapshot()", active)
        self.assertIn("nagApGateSnapshot(apValid, apActive)", active)
        self.assertIn("apValid, apActive", active)

    def test_every_r79_mux1_creation_path_gets_final_overlay(self):
        for start, end in [
            ("static esp_err_t r79ApGateTransmitGuarded", "static esp_err_t r79LabDirectTwaiTransmit"),
            ("static esp_err_t r79LabDirectTwaiTransmitGuarded", "static bool r79Mode1PostMux2GenerationCurrent"),
            ("static esp_err_t mux1DisplayTransmit", "static void mux1CancelPendingLocked"),
        ]:
            self.assertIn("r79DmsApplyFinal", block(self.logic, start, end))

    def test_stock_generation_has_r79_first_single_owner(self):
        process = block(self.logic, "static bool r79ProcessStockFrame", "static void r79Mode2Tick")
        self.assertIn("r79Claimed", process)
        self.assertIn("r79DmsStockDispositionPure", process)
        self.assertIn("r79DmsOnlyTransmit", process)
        dms_only = block(self.logic, "static bool r79DmsOnlyTransmit", "static bool r79ProcessStockFrame")
        self.assertIn("mux1DisplayTransmit(&out, canTxEpochSnapshot(), CAN_TX_FRESH_VH, false,", dms_only)
        self.assertNotIn("r79RetrySchedule", dms_only)
        self.assertNotIn("r79LabRecordTxResult", dms_only)

    def test_api_is_independent_of_nag_config(self):
        self.assertIn('server.on("/api/driver-monitoring/config", HTTP_GET', self.api)
        self.assertIn('server.on("/api/driver-monitoring/config", HTTP_POST', self.api)
        self.assertNotIn('hasArg("dmsControlEnabled")', self.api)


if __name__ == "__main__":
    unittest.main()
