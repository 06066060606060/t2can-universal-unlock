# Tesla Unlock 1.1.0 validation

Date: 2026-10-09 (Asia/Seoul). Package `dev.t2can.unlock`, versionCode 3.

## Final build

- minSdk 26, target/compile SDK36, Android Build Tools 36.0.0, Temurin JDK21.0.12.1+1, Java8 bytecode.
- APK: **181,105 bytes**.
- SHA-256: `481da1267f3f607b833b5118126c4be54ea6b895750fef21a243743f270550c1`.
- Signing certificate SHA-256: `8c695a189677dbc6fd8640081867475ee12944f5baf31e02035baeac362be6e4` (same as existing 1.0.1).
- SDK build, D8, zipalign, APK v2/v3 signature verification: PASS.
- Existing 1.0.1 → 1.1.0 update install in disposable Android16 emulator: PASS.
- Final rebuilt APK ZIP payload is byte-for-byte identical to the emulator-tested APK. ZIP/signature container hashes vary with build timestamps.
- Compiler warnings: Java8 source/target are obsolete under JDK21; legacy Android API calls include deprecated APIs. No compiler/link errors.

## Automated host checks

`T2CAN_NODE=/path/to/node python3 tools/run_tests.py`: PASS.

- Connection session: 17 assertions.
- Device profile: 12 assertions.
- Permission policy: 14 assertions, including Android12 FINE+COARSE request and target36 on API37.
- Configurable origin policy: 24 assertions.
- Existing URL/filename policy: 16 assertions.
- Total pure Java: **83 assertions PASS**.
- JavaScript bridge: **32 assertions PASS** (navigation, OTA/save busy guard, theme, Blob chunking, cancellation).
- Python tooling/test syntax: PASS.

## Android16 emulator checks

Disposable Google APIs ARM64 API36 image, Emulator37.1.11, default1080×1920/420dpi. Dedicated emulator-5570 and ADB5041; no physical device access. Device HTTP was redirected only inside this emulator to a localhost fixture using the unchanged v3.26.3 dashboard source. No fixture code is packaged in the APK.

- `tests/emulator_test.py`: PASS — native health → process bind → dashboard; back navigation; 180,011-byte Blob CSV exact round trip; SAF file picker reads harmless .bin; actual multipart POST through WebView to mock `/update`; OTA-busy connection menu guard; Wi-Fi loss destroys WebView; no automatic reconnect; explicit manual reconnect; live dashboard theme; no app crash log.
- CSV SHA256: `bd46f1df9fe3640a5a683c8f2d16301f9e94d6ec788e4b66e695e8c96ab45ac3`.
- `tests/profile_emulator_test.py`: PASS — real Android Keystore encrypted persistence; no plaintext SSID/password in preferences; process restart preserves selected profile; Nearby permission denial; resume does not repeat prompt; explicit retry; OS Wi-Fi search and cancellation/timeout cleanup.
- `tests/connection_screen_test.py`: PASS — idle cold start/resume, app-owned English strings under Korean app locale.
- `tests/connection_theme_test.py`: PASS — 320/390/430dp, light/dark (six configurations), visible controls within viewport and live theme background refresh. Screenshots visually inspected against baseline card styling.
- Test harness corrections: Google permission controller package differs from AOSP; match visible permission prompt. Android Wi-Fi picker may retain a search and acknowledgement dialog until timeout; handle its actual UI instead of assuming immediate dismissal.

## Not verified

Physical AP association and simultaneous other-app LTE/5G Internet: **NEEDS_DEVICE_TEST**. Emulation is not evidence of RF/network coexistence.
Android8–15 and Android17 runtime, Pixel/Galaxy/Xiaomi OEM behavior, real OTA flash/reboot, real-device log APIs, VPN/PrivateDNS combinations, long background/lock/process-death scenarios: **NEEDS_DEVICE_TEST**.
No CAN, serial, vehicle, controller flashing or road test was performed. Firmware sources were not modified.

See [acceptance matrix](docs/t2can-network-test-results.md) and [user guide](docs/user-guide.md).
