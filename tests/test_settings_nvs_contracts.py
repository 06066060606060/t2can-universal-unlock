"""Settings Nvs contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class SettingsNvsContracts(unittest.TestCase):

    def test_schema3_migration(self):
        """Retained from test_v37_schema3_migration_static.py."""
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
        run_contract_cases(self, [Schema3MigrationContract], expected=5)

    def test_nvs_keep_ble_reset(self):
        """Retained from test_v390_nvs_keep_ble_reset_contract.py."""
        import pathlib
        import re
        import unittest


        ROOT = pathlib.Path(__file__).resolve().parents[1]
        API = (ROOT / "web_api.h").read_text()
        INO = (ROOT / (ROOT.name + ".ino")).read_text()


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
                self.assertIn("NVS_KEEP_BLE_RESET_GUARD_NAMESPACE, true", guard_reader)
                self.assertIn("CheckedPreferences p", guard_reader)
                self.assertIn("p.ok()", guard_reader)
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
        run_contract_cases(self, [NvsKeepBleResetContract], expected=4)

    def test_feature_domain(self):
        """Retained from test_feature_domain_contract.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        logic = (root / "vehicle_logic.h").read_text()
        api = (root / "web_api.h").read_text()
        ino = next(root.glob("*.ino")).read_text()
        runtime = (root / "can_runtime.h").read_text()
        product = logic + api + ino + runtime


        def function_body(source: str, signature: str) -> str:
            start = source.index(signature)
            brace = source.index("{", start)
            depth = 0
            for pos in range(brace, len(source)):
                if source[pos] == "{":
                    depth += 1
                elif source[pos] == "}":
                    depth -= 1
                    if depth == 0:
                        return source[brace : pos + 1]
            raise AssertionError(f"unclosed function: {signature}")


        for required in [
            'begin("ulc"',
            '"alcOff"',
            '"blind"',
            '"ulcOff"',
            '"confirm"',
            '"timing"',
            'begin("alc293lab"',
            '"enabled"',
            '"bus"',
            'server.on("/api/ulc/stats"',
            'server.on("/api/ulc/update"',
            'server.on("/api/lab/auto-lane-change/stats"',
            'server.on("/api/lab/auto-lane-change/update"',
        ]:
            assert required in product, f"missing production domain contract: {required}"

        assert "/api/lab3f8/" not in api
        for obsolete in ["ulcBus", "accfd", "ulcspd"]:
            assert f'getUChar("{obsolete}"' not in product
            assert f'putUChar("{obsolete}"' not in product

        assert "static constexpr uint16_t NVS_SCHEMA_ULC = 2;" in logic
        assert "static constexpr uint16_t NVS_SCHEMA_CURRENT = 3;" in logic
        migration = function_body(logic, "static bool featureConfigMigrateToSchema2()")
        verify_pos = migration.index("featureConfigSchema2ReadBackMatches(")
        schema_pos = migration.index('putUShort("schema", NVS_SCHEMA_ULC)')
        assert verify_pos < schema_pos

        ulc_update = function_body(api, "static void httpUlcUpdate()")
        assert "httpRequireLab()" not in ulc_update
        alc_update = function_body(api, "static void httpAutoLaneChangeLabUpdate()")
        assert "httpRequireLab()" in alc_update
        assert "httpUlcMonitorLabStats" not in api
        assert "/api/lab/ulc-monitor/" not in api

        for removed in [
            "lab3f8AccFollowRaw",
            "lab3f8UlcSpeedMode",
            "ulcNoConfirmTargetBus",
            "ulcNoConfirmInjectCanA",
            "tlsscGreen",
        ]:
            assert removed not in product, f"removed feature remains: {removed}"

        print("production ULC domains, schema migration, and experiment removal: PASS")

    def test_promoted_feature_api(self):
        """Retained from test_promoted_feature_api_contract.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        api = (root / "web_api.h").read_text()
        logic = (root / "vehicle_logic.h").read_text()


        def function_body(source: str, signature: str) -> str:
            start = source.index(signature)
            brace = source.index("{", start)
            depth = 0
            for pos in range(brace, len(source)):
                if source[pos] == "{":
                    depth += 1
                elif source[pos] == "}":
                    depth -= 1
                    if depth == 0:
                        return source[brace : pos + 1]
            raise AssertionError(f"unclosed function: {signature}")


        assert 'server.on("/api/nag/h-profile/stats"' in api
        assert 'server.on("/api/nag/h-profile/update"' in api
        assert 'server.on("/api/nag/h-profile/reset"' in api
        assert "/api/nag-human-lab/" not in api
        for handler in [
            "httpNagHumanProfileStats",
            "httpNagHumanProfileUpdate",
            "httpPedalMapStats",
            "httpPedalMapSet",
        ]:
            body = function_body(api, f"static void {handler}()")
            assert "httpRequireLab()" not in body
        assert "labMenuEnabled && enabled && apActive" not in logic

        for route in [
            'server.on("/api/pedalmap/stats"',
            'server.on("/api/pedalmap/set"',
        ]:
            assert route in api
        assert 'server.on("/api/ap-right-scroll' not in api
        assert 'hasArg("tsl9InputMode")' in api
        assert 'hasArg("tsl9IsaChimeSuppress")' not in api
        assert 'server.on("/api/isa-suppression/config", HTTP_GET' in api
        assert 'server.on("/api/isa-suppression/config", HTTP_POST' in api
        assert 'server.on("/api/driver-monitoring/config", HTTP_GET' in api
        assert 'server.on("/api/driver-monitoring/config", HTTP_POST' in api
        assert 'hasArg("dmsControlEnabled")' not in api

        print("promoted Mode H, PedalMap, standalone DMS/ISA, and TSL9 API contract: PASS")


if __name__ == "__main__":
    unittest.main()
