# Tesla Unlock Android 1.2.1


## Download

[**Download Android APK — v1.2.1**](https://github.com/OWNER/REPOSITORY/releases/download/android-v1.2.1/Tesla-Unlock-1.2.1.apk)

[Release notes, source code, and checksums](https://github.com/OWNER/REPOSITORY/releases/tag/android-v1.2.1)

### Installation

1. Download the APK on your Android phone.
2. Open the downloaded file.
3. Allow installation from this source if Android asks.
4. Install the app and open Tesla Unlock.

When updating, install over the existing app to keep your saved devices.

An Android app for connecting to a T2CAN device and opening its dashboard. It supports saved device profiles, app-scoped local Wi-Fi access, automatic connection on launch, and one immediate retry when Android reports that a connection request is unavailable.

- Package: `dev.t2can.unlock`
- Version code: `5`
- Minimum Android version: Android 8 (API 26)
- Automatic Wi-Fi connection: Android 10 (API 29) or later
- Target and compile SDK: 36

The app checks the device's existing `/api/profile/status` response and verifies process routing before opening the dashboard. Other apps use Android's default internet route. Simultaneous cellular internet access requires physical-device verification; this app does not force other apps onto LTE or 5G.

## Quick start

1. Turn on your T2CAN device and enable Wi-Fi on your phone. Keep mobile data enabled if you want cellular internet access.
2. Open **Devices / connection settings → Devices** and enter the device's actual Wi-Fi name (SSID), security type, password, and dashboard URL.
3. Tap **Connect**, grant the requested permissions, and approve the Android Wi-Fi connection if prompted.
4. The app opens the dashboard after verifying the device response.

The initial default profile has an empty SSID and no password; it is intended for manual connection. See the [user guide](docs/user-guide.md) for detailed instructions.

Once a valid device is registered and the required Wi-Fi permission is granted, a fresh app launch starts a connection sequence for the selected device. Returning to an existing app session from the background does not start another sequence. Android may still display a connection approval prompt.

If automatic connection is unsupported or fails, use **Manual connection → Wi-Fi settings**, connect to the device, then return to **Manual connection → Check connection**.

## Connection retry behavior

If Android reports `onUnavailable()` during an automatic Wi-Fi connection, the app releases the old request and immediately makes one fresh request. This applies to both launch auto-connect and the **Connect** button.

- Each sequence has at most two attempts: the initial attempt and one retry.
- Each attempt has a 40-second overall deadline and a 35-second Android request timeout. A sequence may therefore take roughly 80 seconds.
- Other errors, including the overall deadline, do not automatically retry.
- Manual Wi-Fi mode and loss of an established connection do not trigger automatic retries.
- **Disconnect** or closing the connection manager cancels the sequence.
- Android does not distinguish approval rejection from other unavailable reasons in this callback. Rejecting the first request can therefore cause one more approval prompt.

After a terminal failure, tap **Connect** to start a new sequence. When an established connection is lost, the dashboard closes. Previous POST requests are never automatically replayed.

## Features and limits

- Up to 20 saved profiles, encrypted using Android Keystore-backed storage.
- Existing OTA file selection, Blob log export, dashboard back navigation, and theme integration.
- Local-only Wi-Fi requests through `WifiNetworkSpecifier`, with app process routing through `bindProcessToNetwork()`.
- HTTP is allowed only for `192.168.4.1`, including explicitly specified ports. Other private IPv4 addresses require HTTPS with a trusted certificate.
- No changes to firmware, CAN processing, or vehicle-control logic.
- Continuous background connectivity is not guaranteed.
- The APK contains no firmware binary, bundled dashboard copy, or mock device data.

## Documentation and validation

See the [original source audit](docs/t2can-network-audit.md), [implementation and limitations](docs/t2can-network-implementation.md), [development specification](docs/development-spec-2026-10-09.md), and [validation results](VALIDATION.md). Some supporting documents are in Korean.

Host tests, Android compilation, and emulator results are separate from physical Wi-Fi/cellular, OTA, and vehicle validation. Consult `VALIDATION.md` for the exact checks performed and remaining limitations.

## Build and signing

Use Java 21, Android SDK Platform 36, Build Tools 36.0.0, and Python 3. Gradle and external app libraries are not required. Java sources are compiled to Java 8 bytecode.

```sh
export T2CAN_JDK=/absolute/path/to/jdk/Contents/Home
export T2CAN_BUILD_TOOLS=/absolute/path/to/android-sdk/build-tools/36.0.0
export T2CAN_ANDROID_JAR=/absolute/path/to/android-sdk/platforms/android-36/android.jar
export T2CAN_SIGNING_KEY=/absolute/path/to/existing/release.p12
export T2CAN_SIGNING_PASSWORD_FILE=/absolute/path/to/existing/signing-password.txt
export T2CAN_OUTPUT=/absolute/path/to/Tesla-Unlock/releases/Tesla-Unlock-1.2.1
python3 tools/build.py
python3 tools/package_source.py
```

Reuse the existing signing key to install an update over an earlier release. The build stops if the key is missing; it does not generate a replacement key. Private keys, passwords, toolchains, and intermediate build files are excluded from the source ZIP.

Output names are `Tesla-Unlock-1.2.1.apk` and `Tesla-Unlock-1.2.1-source.zip`. Use the release directory's `SHA256SUMS.txt` to verify the distributed files.

## Source layout

- `src/dev/t2can/unlock/`: Activity, connection manager, profile/permission/URL policies, encrypted storage, and file export.
- `assets/dashboard-bridge.js`: Dashboard back navigation, file export, and transfer-state integration.
- `res/`: App icons, themes, and HTTP restrictions.
- `tests/`: Pure Java policy/session tests, JavaScript bridge tests, and local emulator checks.
- `tools/`: Android SDK build, source packaging, and local test tools.

Emulator tools are intended only for disposable test environments. Do not run them against physical vehicle, CAN, or serial devices.
