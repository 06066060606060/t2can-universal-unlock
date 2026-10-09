# Tesla Unlock Android 1.2.0 validation

Date: 2026-10-09 (Asia/Seoul). Package `dev.t2can.unlock`; versionCode4; minSdk26; target/compile SDK36.

## Change

With a valid selected device and already-granted Wi-Fi permission, a fresh app launch makes one automatic connection attempt. Permission missing: explanation only, explicit Connect requests permission. No automatic retry on ordinary resume, after failure or explicit disconnect in the same Activity session. Configuration-only recreation retains the consumed decision; a fresh process creates a new decision.

No network manager, storage schema, firmware, CAN or dashboard bridge changes. Existing 1.1.0 source/artifacts are preserved.

## Host validation

`T2CAN_NODE=/path/to/node python3 tools/run_tests.py`: PASS.

- Launch policy: 12 new assertions; observed RED for missing automatic launch before implementation, GREEN afterward.
- Existing Java policy suites: 83 assertions PASS; total **95**.
- Existing JavaScript bridge: **32** assertions PASS.
- Independent read-only lifecycle review: no important findings.

## SDK build and artifact

- Temurin JDK21.0.12.1+1, Java8 bytecode, SDK36 / Build Tools36.0.0.
- `python3 tools/build.py`: PASS; D8, zipalign and APK v2/v3 signatures PASS.
- Same signing certificate as 1.1.0: `8c695a189677dbc6fd8640081867475ee12944f5baf31e02035baeac362be6e4`.
- Delivered APK is copied byte-for-byte from the emulator-tested SDK build.
- APK size: **181105 bytes**.
- APK SHA256: `d31e260ad76755509bffc009cd096966220e9c9c164337dc2d630cde08c7d089`.
- Existing Java8 obsolescence and Android deprecated-API compiler warnings remain; no compile errors.

## Current Android16 emulator regression

`python3 tests/launch_emulator_test.py`: PASS. Disposable API36 emulator-5570 / dedicated ADB5041; no physical phone/AP/vehicle.

1. Install 1.1.0, register a fake AP, update to 1.2.0: encrypted selected profile remains readable.
2. Missing permission: action guidance, no unsolicited runtime permission loop.
3. Grant permission to the test fixture and disable emulator Wi-Fi: fresh launch reaches Wi-Fi-off error automatically without tapping Connect, proving a connection attempt occurred.
4. Enable Wi-Fi and resume the same failed Activity: no repeat OS request.
5. Explicit Disconnect and resume: remains IDLE.
6. Fresh launch with Wi-Fi ON: Android system Wi-Fi search dialog opens without tapping Connect.
7. No app crash log.

## Limits

Actual T2CAN AP association / cellular coexistence / OEM background handling: **NEEDS_DEVICE_TEST**. Android8–15 and17 physical runtime not tested. Real OTA, CAN, serial, vehicle and road tests were not performed. No controller was flashed.

The broader CSV/OTA/theme emulator checks from 1.1.0 are preserved in [historical validation](docs/validation-1.1.0.md); they were not rerun as new 1.2.0 end-to-end evidence. Their bridge/policy host tests did run in this version.
