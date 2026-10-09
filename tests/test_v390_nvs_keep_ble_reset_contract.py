import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
API = (ROOT / "web_api.h").read_text()
INO = (ROOT / "T2CAN-Universal-v3.28.0-LP_YL.ino").read_text()


class NvsKeepBleResetContract(unittest.TestCase):
    def test_exact_application_namespace_allowlist(self):
        helper = API.split("static bool resetNvsKeepBleClearApplicationNamespaces()", 1)[1].split(
            "static bool resetNvsKeepBleFinalize()", 1)[0]
        expected = {
            "v3profile", "wifiap", "nag", "summon", "lab3f8", "labv3fd", "ulc",
            "alc293lab", "countrylab", "r79lab", "r79", "lanegraph", "researchcap", "features",
            "canDiag", "t2meta",
        }
        found = set(re.findall(r'"([A-Za-z0-9]+)"', helper))
        self.assertEqual(expected, found)
        self.assertNotIn("s3xy", helper)
        self.assertNotIn("s3xyreg", helper)
        self.assertNotIn("nvs_flash_erase", helper)

    def test_durable_guard_brackets_every_destructive_boundary(self):
        guard_reader = API.split("static bool resetNvsKeepBleGuardRead", 1)[1].split(
            "static bool resetNvsKeepBleGuardWrite", 1
        )[0]
        self.assertIn("NVS_KEEP_BLE_RESET_GUARD_NAMESPACE, false", guard_reader)
        handler = API.split("static void httpResetNvsPreserveS3xyBle()", 1)[1].split(
            "static void httpFactoryReset()", 1
        )[0]
        self.assertLess(
            handler.index("resetNvsKeepBleGuardWrite(true)"),
            handler.index("resetNvsKeepBleFinalize()"),
        )
        finalizer = API.split("static bool resetNvsKeepBleFinalize()", 1)[1].split(
            "static bool resetNvsKeepBleRecoverIfNeeded()", 1
        )[0]
        self.assertLess(
            finalizer.index("resetNvsKeepBleClearApplicationNamespaces()"),
            finalizer.index("vehicleProfileWriteBootstrapMarker(false)"),
        )
        self.assertLess(
            finalizer.index("vehicleProfileWriteBootstrapMarker(false)"),
            finalizer.index("resetNvsKeepBleGuardWrite(false)"),
        )

    def test_boot_recovery_precedes_first_universal_full_erase(self):
        self.assertIn('#include "nvs_keep_ble_reset_pure.h"', INO)
        self.assertIn("resetNvsKeepBleGuardRead", INO)
        self.assertIn("NVS_KEEP_BLE_GUARD_RECOVER_PURE", INO)
        self.assertIn("resetNvsKeepBleRecoverIfNeeded()", INO)
        self.assertLess(
            INO.index("NVS_KEEP_BLE_GUARD_RECOVER_PURE"),
            INO.index("nvs_flash_erase()"),
        )

    def test_endpoint_holds_tx_and_returns_to_profile_setup(self):
        handler = API.split("static void httpResetNvsPreserveS3xyBle()", 1)[1].split(
            "static void httpFactoryReset()", 1)[0]
        self.assertIn("setCanTxAdministrativeHold(true)", handler)
        self.assertIn("invalidateCanTxStateForFullRecovery()", handler)
        self.assertIn("resetNvsKeepBleGuardWrite(true)", handler)
        self.assertIn("resetNvsKeepBleFinalize()", handler)
        self.assertIn("prepareCanForMaintenance()", handler)
        self.assertIn("restartT2CanSafely()", handler)
        self.assertIn('\\"profileSetup\\":true', handler)
        self.assertIn('\\"blePreserved\\":true', handler)
        self.assertIn(
            'server.on("/api/system/reset-nvs-keep-ble", HTTP_POST, httpResetNvsPreserveS3xyBle);',
            API,
        )


if __name__ == "__main__":
    unittest.main()
