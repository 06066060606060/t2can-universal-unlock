# Existing Android audit — 2026-10-09

- Baseline: `references/android/Tesla-Unlock-1.0.1-source.zip`, preserved unchanged. Work copy: `apps/android/Tesla-Unlock-1.1.0`, preserving the source layout.
- Native Java Activity/WebView, no Gradle or third-party runtime dependencies. SDK build tools 36.0.0, platform 36, Temurin JDK 21.0.12.1+1. minSdk 26 / targetSdk 36.
- Package `dev.t2can.unlock`, baseline versionCode 2. Signing key located in original private workspace; not copied into deliverables. Baseline compiled successfully; v2/v3 verification and zipalign passed. Certificate SHA256 `8c695a189677dbc6fd8640081867475ee12944f5baf31e02035baeac362be6e4` matches existing APK record.
- `MainActivity.connectWifi`: generic Wi-Fi request without SSID. Probes all Wi-Fi Networks using `Network.openConnection` to `/api/profile/status`; response must contain `setupMode` and `profile`. Binds process after probe. Unbounded 3-second retry. WebView is created before binding.
- `UrlPolicy.HOME`: `http://192.168.4.1/`, only port 80. NSC permits cleartext only to 192.168.4.1. Other origins blocked. No external authentication, analytics or update network clients.
- SSID/password are not stored/read in baseline. Current firmware v3.26.3 `web_api.h` contains MAC-derived `T2CAN-%02X%02X` defaults, user overrides in NVS. No exact physical-device SSID can be inferred. Profile entry must use user-provided SSID/security/password; do not prefill a guessed SSID/password.
- Existing dashboard health endpoint confirmed in firmware; firmware is read-only for this task.
- WebView JavaScript/DOM storage enabled; file access disabled; selected content URI allowed for OTA. No JavaScript interface; origin-scoped WebMessagePort + `dashboard-bridge.js` handles chunked Blob saves and back navigation. `FileExporter` uses SAF, 128 MiB limit. No DownloadManager.
- WebView blocks external origins; dashboard fetch/OTA run in WebView. Preserve file picker, Blob export, themes, insets, back handling and app-owned English labels.
- Orientation/screenSize/keyboardHidden/uiMode handled by existing Activity. Destruction unregisters callback and clears binding. Android process death naturally removes process binding; new launch must not recreate system approval without user action.
- Baseline device/emulator UI tests from 2026-09-29 are historical evidence only. Current physical phone, AP, cellular coexistence and OTA flashing have not been tested.
