# Tesla Unlock Android 1.2.1 validation

Date: 2026-10-09. Package dev.t2can.unlock, versionCode 5, minSdk 26, target/compile SDK 36.

## Change

On automatic Wi-Fi request onUnavailable only, release the old session and immediately issue one fresh request, skipping existing-network probes. At most two attempts per Connect/launch sequence. Other failures and established-connection loss do not retry. Disconnect/close cancel remaining retry eligibility. Waiting text now describes Android Wi-Fi connection rather than assuming user approval is pending.

Android cannot distinguish user rejection from other unavailability via this callback: a rejected approval may prompt once again. Each attempt retains 35-second request timeout / 40-second overall deadline; two attempts can take roughly 80 seconds. This is recovery behavior, not a confirmed fix for the reported physical-device Wi-Fi stall.

## Verified

- Regression RED: inert retry policy failed First unavailable must immediately allow retry.
- Host tests: 107 Java assertions, 32 JavaScript assertions PASS.
- Android build (aapt2, javac, D8), APK v2/v3 signature, zipalign PASS.
- Certificate SHA-256: 8c695a189677dbc6fd8640081867475ee12944f5baf31e02035baeac362be6e4 (unchanged).
- APK size: 181105 bytes. SHA-256: 584741a228672270e28826d73390a0f63a3cef9f31cdb4660f611be9090a49b7.
- Independent read-only lifecycle review: no important findings.
- Disposable API36 emulator launch regression PASS: update from 1.1.0 preserves encrypted selected profile; missing permission guidance; Wi-Fi-off launch fails without Connect; background resume does not restart; Disconnect remains idle; fresh launch opens Android request.

- Disposable API36 actual timeout integration PASS: request 1 at 14:27:13.391 and request 2 at 14:27:48.400 (emulator log clock), then terminal ERROR; exactly two actual Android Wi-Fi requests. Disconnect/background return remained IDLE and generated no third request. No app crash.
- Test adaptation: system search dialog ignored Back, so cancellation-based assumption was discarded; verified real 35-second timeout instead.
- Earlier cold-boot test started before emulator readiness; emulator helper now waits for explicit emulator boot and was exercised successfully by retry integration.

## Limits

Actual T2CAN association, repeated successful physical reconnection, mobile data coexistence, Samsung/other OEM behavior: NEEDS_DEVICE_TEST. No physical devices, firmware flashing, CAN, or vehicle testing performed. Old CSV/OTA/theme checks in docs/validation-1.1.0.md and launch results in docs/validation-1.2.0.md are historical evidence, not fresh 1.2.1 tests.
