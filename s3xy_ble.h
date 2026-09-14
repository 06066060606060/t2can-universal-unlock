#pragma once

// S3XY BUTTON BLE / REGISTRY / AUTO-RECONNECT
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// S3XY BUTTON BLE + MULTI-DEVICE REGISTRY + NOA LANE-CHANGE CANCEL
//
// Current behavior:
//   - Up to three Gen2 S3XY Buttons are supported with persistent per-device
//     address, identity, name, gesture mapping, and auto-connect state.
//   - Reconnect link setup stays serialized and fail-closed. When two or more
//     saved Buttons are pending, one active registry scan gathers all exact
//     peers first; encrypted 3D49 recovery remains the fallback for changed addresses.
//   - Link setup is staged through connect, security, service discovery, notify
//     subscription, B6 handshake, and READY before button actions are accepted.
//   - Bluetooth Master is a persistent hardware-stack switch. When disabled,
//     BLEDevice is not initialized and scan/client/reconnect activity is absent.
//   - Registry/bond reset, per-device reconnect controls, and diagnostics are
//     available through the dashboard/API.
//   - The implemented vehicle action is NOA Lane Change Cancel. Unknown or
//     unimplemented mappings do not transmit to the vehicle.
//   - CAN-side NOA cancel is fail-closed: fresh NOA state 5 and a fresh 0x24A
//     LEFT/RIGHT request are both required.
// ═══════════════════════════════════════════════════════════════

static const char *S3XY_SERVICE_UUID = "00003d46-87d2-479e-7e45-8551415a6de1";
static const char *S3XY_NOTIFY_UUID  = "00003d50-87d2-479e-7e45-8551415a6de1";
static const char *S3XY_ID_UUID      = "00003d49-87d2-479e-7e45-8551415a6de1";

static constexpr uint8_t  S3XY_MAX_DEVICES = 3;
static constexpr uint8_t  S3XY_DISCOVERY_MAX = 8;

// Heavy BLE diagnostics are preserved in source but compiled out by default.
// Set to 1 (or pass -DS3XY_DIAGNOSTICS_ENABLED=1) for a diagnostic build.
#ifndef S3XY_DIAGNOSTICS_ENABLED
#define S3XY_DIAGNOSTICS_ENABLED 0
#endif

static constexpr uint16_t S3XY_LOG_MAX = 128;
static constexpr uint8_t  S3XY_LOG_DATA_MAX = 20;
static constexpr uint8_t  S3XY_PEER_ID_MAX = 20;

static constexpr uint32_t S3XY_AUTO_BOOT_DELAY_MS = 1500;
static constexpr uint32_t S3XY_AUTO_RETRY_SHORT_MS = 5000;
static constexpr uint32_t S3XY_AUTO_RETRY_LONG_MS = 15000;
static constexpr uint32_t S3XY_AUTO_DISCONNECT_RETRY_MS = 1200;
static constexpr uint32_t S3XY_DISCONNECT_SETTLE_MS = 120;
static constexpr uint32_t S3XY_GATT_SETTLE_MS = 80;
static constexpr uint32_t S3XY_BLE_OPERATION_GAP_MS = 250;
static constexpr uint32_t S3XY_AUTO_WAKE_SCAN_GAP_MS = 250;
static constexpr uint8_t  S3XY_AUTO_WAKE_SCAN_SECONDS = 2;
static constexpr uint8_t  S3XY_AUTO_BATCH_SCAN_SECONDS = 2;
static constexpr uint8_t  S3XY_AUTO_SCAN_SECONDS = 4;
static constexpr uint8_t  S3XY_DISCOVERY_SCAN_SECONDS = 8;
static constexpr uint32_t S3XY_IDENTITY_REJECT_MS = 5000;

static constexpr uint32_t ULC_SNOOZE_REQUEST_TIMEOUT_MS = 1000;
static portMUX_TYPE ulcSnoozeMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool ulcSnoozePending = false;
static volatile uint32_t ulcSnoozeExpireMs = 0;
static volatile uint32_t ulcSnoozeAccepted = 0;
static volatile uint32_t ulcSnoozeBlocked = 0;
static volatile uint32_t ulcSnoozeTxOk = 0;
static volatile uint32_t ulcSnoozeTxFail = 0;
static volatile uint32_t ulcSnoozeLastActionMs = 0;
static volatile uint8_t ulcSnoozeLastDir = 0;
static char ulcSnoozeLastResult[64] = "never";

static void handleS3xySingleAction();
static void requestPedalMapToggleFromButton();

enum S3xyMapState : uint8_t {
  S3XY_MAP_OFF = 0,
  S3XY_MAP_IDLE,
  S3XY_MAP_SCANNING,
  S3XY_MAP_FOUND,
  S3XY_MAP_CONNECTING,
  S3XY_MAP_CONNECTED,
  S3XY_MAP_SECURING,
  S3XY_MAP_DISCOVERING,
  S3XY_MAP_SUBSCRIBING,
  S3XY_MAP_HANDSHAKE,
  S3XY_MAP_READY,
  S3XY_MAP_RETRY,
  S3XY_MAP_ERROR
};

enum S3xyAction : uint8_t {
  S3XY_ACTION_NONE = 0,
  S3XY_ACTION_NOA_CANCEL = 1,
  S3XY_ACTION_ACCEL_MODE_TOGGLE = 2,
  S3XY_ACTION_RESEARCH_CAPTURE_C = 3,
  S3XY_ACTION_RESEARCH_CAPTURE_A = 4,
  S3XY_ACTION_RESEARCH_CAPTURE_B = 5,
  S3XY_ACTION_RESEARCH_CAPTURE_D = 6,
  S3XY_ACTION_RESEARCH_CAPTURE_RESET = 7
};

enum S3xyCommandType : uint8_t {
  S3XY_CMD_NONE = 0,
  S3XY_CMD_DISCOVERY_SCAN,
  S3XY_CMD_PAIR_ADDRESS,
  S3XY_CMD_CONNECT_SLOT,
  S3XY_CMD_DISCONNECT_SLOT,
  S3XY_CMD_FORGET_SLOT,
  S3XY_CMD_HANDSHAKE_SLOT,
  S3XY_CMD_RESET_ALL_BLUETOOTH
};

enum S3xyScanMode : uint8_t {
  S3XY_SCAN_NONE = 0,
  S3XY_SCAN_DISCOVERY,
  S3XY_SCAN_TARGET,
  S3XY_SCAN_REGISTRY,
  S3XY_SCAN_REGISTRY_BATCH
};

enum S3xyLogType : uint8_t {
  S3XY_LOG_INFO = 0,
  S3XY_LOG_SCAN_HIT,
  S3XY_LOG_CONNECT,
  S3XY_LOG_SECURITY,
  S3XY_LOG_GATT,
  S3XY_LOG_ID_READ,
  S3XY_LOG_ID_WRITE,
  S3XY_LOG_NOTIFY,
  S3XY_LOG_DISCONNECT,
  S3XY_LOG_ERROR
};

#if S3XY_DIAGNOSTICS_ENABLED
struct S3xyLogEntry {
  uint32_t ms;
  uint8_t type;
  int8_t slot; // -1 = registry/global
  int16_t rssi;
  uint8_t len;
  uint8_t data[S3XY_LOG_DATA_MAX];
  char detail[56];
};
#endif

struct S3xyDeviceSlot {
  volatile bool used;
  char address[24];
  char name[28];
  volatile uint8_t addressType;
  volatile bool addressTypeKnown;
  volatile uint8_t singleAction;
  volatile uint8_t doubleAction;
  volatile uint8_t longAction;
  volatile bool autoConnect;

  volatile uint8_t state;
  volatile bool connected;
  volatile bool subscribed;
  volatile bool secureOk;
  volatile bool manualPaused;
  volatile int16_t rssi;

  volatile uint32_t notifyCount;
  volatile uint32_t unparsedCount;
  volatile uint32_t singleCount;
  volatile uint32_t doubleCount;
  volatile uint32_t longCount;
  volatile uint32_t handshakeAckCount;
  volatile uint32_t lastNotifyMs;
  volatile uint8_t lastNotifyLen;
  uint8_t lastNotify[S3XY_LOG_DATA_MAX];
  char lastError[64];
  char idHex[64];
  uint8_t peerId[S3XY_PEER_ID_MAX];
  volatile uint8_t peerIdLen;
  volatile bool identityPersistVerified;

  volatile uint32_t nextAttemptMs;
  volatile uint32_t autoAttempts;
  volatile uint32_t autoReconnects;
  volatile uint32_t autoFailures;
  volatile uint32_t consecutiveFailures;

  // Last automatic reconnect timing. Runtime-only diagnostics; not persisted.
  volatile uint32_t autoAttemptStartMs;
  volatile uint32_t lastDiscoverMs;
  volatile uint32_t lastConnectMs;
  volatile uint32_t lastReadyMs;
  char lastAutoPath[24];

  BLEClient *client;
  BLERemoteCharacteristic *notifyChar;
  BLERemoteCharacteristic *idChar;
};

struct S3xyDiscoveredDevice {
  char address[24];
  char name[20];
  int16_t rssi;
  bool registered;
};

struct S3xyCommandRequest {
  volatile uint8_t type;
  volatile int8_t slot;
  char address[24];
};

// Arduino's sketch preprocessor can auto-generate this prototype before the
// user-defined S3xyCommandRequest type, which makes the generated .cpp fail.
// Providing the prototype explicitly here keeps it after the type declaration.
static bool s3xyTakeCommand(S3xyCommandRequest &out);
static void s3xyBestEffortRemoveBond(const char *address);

static portMUX_TYPE s3xyMux = portMUX_INITIALIZER_UNLOCKED;
static S3xyDeviceSlot s3xyDevices[S3XY_MAX_DEVICES] = {};
static S3xyDiscoveredDevice s3xyDiscovered[S3XY_DISCOVERY_MAX] = {};
static volatile uint8_t s3xyDiscoveredCount = 0;
static volatile bool s3xyDiscoveryScanning = false;
static char s3xyDiscoveryError[64] = "";

#if S3XY_DIAGNOSTICS_ENABLED
static S3xyLogEntry s3xyLog[S3XY_LOG_MAX] = {};
static volatile uint16_t s3xyLogHead = 0;
static volatile uint16_t s3xyLogCount = 0;
static volatile uint32_t s3xyLogDropped = 0;
#endif
static volatile uint32_t s3xyActionPending = 0;
static volatile uint32_t s3xyAccelActionPending = 0;
static volatile uint32_t s3xyResearchCaptureAPending = 0;
static volatile uint32_t s3xyResearchCaptureBPending = 0;
static volatile uint32_t s3xyResearchCaptureCPending = 0;
static volatile uint32_t s3xyResearchCaptureDPending = 0;
static volatile uint32_t s3xyResearchCaptureResetPending = 0;

static volatile bool s3xyAutoEnabled = true;
static volatile bool s3xyBluetoothEnabled = false;
static S3xyCommandRequest s3xyCommand = {};
static TaskHandle_t s3xyTaskHandle = nullptr;
static volatile bool s3xyBleInitialized = false;
static volatile bool s3xyRuntimeStopRequested = false;
static volatile bool s3xyRuntimeStopping = false;
static volatile uint32_t s3xyLastBleOperationMs = 0;

static volatile uint8_t s3xyScanMode = S3XY_SCAN_NONE;
static volatile int8_t s3xyScanSlot = -1;
static char s3xyScanTargetAddress[24] = "";
static volatile bool s3xyScanTargetFound = false;
static BLEAdvertisedDevice *s3xyTarget = nullptr;
static BLEAdvertisedDevice *s3xyBatchTargets[S3XY_MAX_DEVICES] = {};
static BLEAdvertisedDevice *s3xyBatchIdentityTarget = nullptr;
static volatile uint8_t s3xyBatchExpectedCount = 0;
static BLEClient *s3xyProbeClient = nullptr;
// BLEDevice keeps only the most recently created client internally. Track the
// same pointer so runtime shutdown can delete older clients first and let
// BLEDevice::deinit(false) own the final client without a double free.
static BLEClient *s3xyLastCreatedClient = nullptr;
static volatile uint32_t s3xyIdentityProbeAttempts = 0;
static volatile uint32_t s3xyIdentityProbeMatches = 0;
static volatile uint32_t s3xyIdentityProbeMismatches = 0;
static volatile uint32_t s3xyIdentityRebinds = 0;
static char s3xyIdentityRejectedAddress[24] = "";
static volatile uint32_t s3xyIdentityRejectedUntilMs = 0;
static volatile uint32_t s3xyAutoExactHits = 0;
static volatile uint32_t s3xyAutoExactMisses = 0;
static volatile uint32_t s3xyAutoLinkFailures = 0;
static volatile uint8_t s3xyAutoRoundRobinCursor = 0;
static char s3xyAutoTrace[112] = "not started";
static char s3xyBleStackName[16] = "UNKNOWN";
static volatile int16_t s3xyBootLocalBondCount = -1;
static volatile bool s3xyBootBondRepairArmed = false;
static volatile uint32_t s3xyBondRepairAttempts = 0;
static volatile uint32_t s3xyBondRepairReady = 0;
static volatile uint32_t s3xyBondPersistStillMissing = 0;

static const char *s3xyStateName(uint8_t st) {
  switch (st) {
    case S3XY_MAP_OFF:        return "OFF";
    case S3XY_MAP_IDLE:       return "IDLE";
    case S3XY_MAP_SCANNING:   return "SCANNING";
    case S3XY_MAP_FOUND:      return "FOUND";
    case S3XY_MAP_CONNECTING:  return "CONNECTING";
    case S3XY_MAP_CONNECTED:   return "CONNECTED";
    case S3XY_MAP_SECURING:    return "SECURING";
    case S3XY_MAP_DISCOVERING: return "DISCOVERING";
    case S3XY_MAP_SUBSCRIBING: return "SUBSCRIBING";
    case S3XY_MAP_HANDSHAKE:   return "HANDSHAKE";
    case S3XY_MAP_READY:       return "READY";
    case S3XY_MAP_RETRY:       return "RETRY";
    case S3XY_MAP_ERROR:       return "ERROR";
    default:                  return "UNKNOWN";
  }
}

static const char *s3xyActionCode(uint8_t action) {
  if (action == S3XY_ACTION_NOA_CANCEL) return "noa_cancel";
  if (action == S3XY_ACTION_ACCEL_MODE_TOGGLE) return "accel_mode_toggle";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_A) return "research_capture_a";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_B) return "research_capture_b";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_C) return "research_capture_c";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_D) return "research_capture_d";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_RESET) return "research_capture_reset";
  return "none";
}

static const char *s3xyActionLabel(uint8_t action) {
  if (action == S3XY_ACTION_NOA_CANCEL) return "NOA Lane Change Cancel";
  if (action == S3XY_ACTION_ACCEL_MODE_TOGGLE) return "Acceleration Mode Toggle";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_A) return "Research Capture A";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_B) return "Research Capture B";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_C) return "Research Capture C";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_D) return "Research Capture D";
  if (action == S3XY_ACTION_RESEARCH_CAPTURE_RESET) return "Research Capture Reset";
  return "None";
}

static bool s3xyActionSupportedForCurrentProfile(uint8_t action) {
  if (action == S3XY_ACTION_ACCEL_MODE_TOGGLE) return activeProfilePedalMapSupported();
  return true;
}

static uint8_t s3xyParseAction(const String &s) {
  if (s == "noa_cancel" || s == "NOA Lane Change Cancel") return S3XY_ACTION_NOA_CANCEL;
  if (s == "accel_mode_toggle" || s == "Acceleration Mode Toggle") return S3XY_ACTION_ACCEL_MODE_TOGGLE;
  if (s == "research_capture_a" || s == "Research Capture A" || s == "alc_capture_left_open" || s == "CAN Research Capture - LEFT OPEN") return S3XY_ACTION_RESEARCH_CAPTURE_A;
  if (s == "research_capture_b" || s == "Research Capture B" || s == "alc_capture_left_blocked" || s == "CAN Research Capture - LEFT BLOCKED") return S3XY_ACTION_RESEARCH_CAPTURE_B;
  if (s == "research_capture_c" || s == "Research Capture C" || s == "alc_capture" || s == "CAN Research Capture (Unlabeled)") return S3XY_ACTION_RESEARCH_CAPTURE_C;
  if (s == "research_capture_d" || s == "Research Capture D") return S3XY_ACTION_RESEARCH_CAPTURE_D;
  if (s == "research_capture_reset" || s == "Research Capture Reset") return S3XY_ACTION_RESEARCH_CAPTURE_RESET;
  return S3XY_ACTION_NONE;
}

#if S3XY_DIAGNOSTICS_ENABLED
static const char *s3xyLogTypeName(uint8_t t) {
  switch (t) {
    case S3XY_LOG_INFO:       return "INFO";
    case S3XY_LOG_SCAN_HIT:   return "SCAN_HIT";
    case S3XY_LOG_CONNECT:    return "CONNECT";
    case S3XY_LOG_SECURITY:   return "SECURITY";
    case S3XY_LOG_GATT:       return "GATT";
    case S3XY_LOG_ID_READ:    return "ID_READ";
    case S3XY_LOG_ID_WRITE:   return "ID_WRITE";
    case S3XY_LOG_NOTIFY:     return "NOTIFY";
    case S3XY_LOG_DISCONNECT: return "DISCONNECT";
    case S3XY_LOG_ERROR:      return "ERROR";
    default:                  return "UNKNOWN";
  }
}

static void s3xyLogPush(uint8_t type, const char *detail,
                        const uint8_t *data = nullptr, size_t len = 0,
                        int16_t rssi = -127, int8_t slot = -1) {
  S3xyLogEntry e = {};
  e.ms = millis();
  e.type = type;
  e.slot = slot;
  e.rssi = rssi;
  e.len = (uint8_t)min(len, (size_t)S3XY_LOG_DATA_MAX);
  if (data && e.len) memcpy(e.data, data, e.len);
  if (detail) {
    strncpy(e.detail, detail, sizeof(e.detail) - 1);
    e.detail[sizeof(e.detail) - 1] = '\0';
  }
  portENTER_CRITICAL(&s3xyMux);
  s3xyLog[s3xyLogHead] = e;
  s3xyLogHead = (uint16_t)((s3xyLogHead + 1) % S3XY_LOG_MAX);
  if (s3xyLogCount < S3XY_LOG_MAX) s3xyLogCount++;
  else s3xyLogDropped++;
  portEXIT_CRITICAL(&s3xyMux);
}

#else
// Macro form intentionally discards arguments without evaluating them. This avoids
// temporary String creation and locking in normal production builds.
#define s3xyLogPush(...) do {} while (0)
#endif

static void s3xyBytesToHex(const uint8_t *data, size_t len, char *out, size_t outLen) {
  if (!out || outLen == 0) return;
  out[0] = '\0';
  size_t used = 0;
  for (size_t i = 0; data && i < len; i++) {
    int n = snprintf(out + used, outLen - used, "%s%02X", i ? " " : "", data[i]);
    if (n <= 0 || (size_t)n >= outLen - used) break;
    used += (size_t)n;
  }
}

static uint8_t s3xyHexNibble(char c) {
  if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
  return 0xFF;
}

static size_t s3xyHexToBytes(const char *src, uint8_t *out, size_t outMax) {
  if (!src || !out || outMax == 0) return 0;
  size_t n = 0;
  while (*src && n < outMax) {
    while (*src == ' ' || *src == ':' || *src == '-' || *src == '\t') src++;
    if (!*src) break;
    const uint8_t hi = s3xyHexNibble(*src++);
    if (hi == 0xFF || !*src) return 0;
    const uint8_t lo = s3xyHexNibble(*src++);
    if (lo == 0xFF) return 0;
    out[n++] = (uint8_t)((hi << 4) | lo);
    while (*src == ' ' || *src == ':' || *src == '-' || *src == '\t') src++;
  }
  return n;
}

static bool s3xyPeerIdEquals(const uint8_t *a, size_t aLen, const uint8_t *b, size_t bLen) {
  return a && b && aLen > 0 && aLen == bLen && memcmp(a, b, aLen) == 0;
}

#if S3XY_DIAGNOSTICS_ENABLED
static void s3xySetAutoTrace(const String &detail) {
  portENTER_CRITICAL(&s3xyMux);
  strncpy(s3xyAutoTrace, detail.c_str(), sizeof(s3xyAutoTrace) - 1);
  s3xyAutoTrace[sizeof(s3xyAutoTrace) - 1] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoTimingBegin(uint8_t slot, const char *path) {
  if (slot >= S3XY_MAX_DEVICES) return;
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) {
    s3xyDevices[slot].autoAttemptStartMs = now;
    s3xyDevices[slot].lastDiscoverMs = 0;
    s3xyDevices[slot].lastConnectMs = 0;
    s3xyDevices[slot].lastReadyMs = 0;
    strncpy(s3xyDevices[slot].lastAutoPath, path ? path : "", sizeof(s3xyDevices[slot].lastAutoPath) - 1);
    s3xyDevices[slot].lastAutoPath[sizeof(s3xyDevices[slot].lastAutoPath) - 1] = '\0';
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoTimingPath(uint8_t slot, const char *path) {
  if (slot >= S3XY_MAX_DEVICES) return;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) {
    strncpy(s3xyDevices[slot].lastAutoPath, path ? path : "", sizeof(s3xyDevices[slot].lastAutoPath) - 1);
    s3xyDevices[slot].lastAutoPath[sizeof(s3xyDevices[slot].lastAutoPath) - 1] = '\0';
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoTimingAdopt(uint8_t fromSlot, uint8_t toSlot, const char *path) {
  if (fromSlot >= S3XY_MAX_DEVICES || toSlot >= S3XY_MAX_DEVICES) return;
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[toSlot].used) {
    if (fromSlot != toSlot && s3xyDevices[fromSlot].used) {
      const uint32_t startMs = s3xyDevices[fromSlot].autoAttemptStartMs;
      s3xyDevices[toSlot].autoAttemptStartMs = startMs ? startMs : now;
      s3xyDevices[toSlot].lastDiscoverMs = s3xyDevices[fromSlot].lastDiscoverMs;
      s3xyDevices[toSlot].lastConnectMs = 0;
      s3xyDevices[toSlot].lastReadyMs = 0;
    } else if (!s3xyDevices[toSlot].autoAttemptStartMs) {
      s3xyDevices[toSlot].autoAttemptStartMs = now;
    }
    strncpy(s3xyDevices[toSlot].lastAutoPath, path ? path : "", sizeof(s3xyDevices[toSlot].lastAutoPath) - 1);
    s3xyDevices[toSlot].lastAutoPath[sizeof(s3xyDevices[toSlot].lastAutoPath) - 1] = '\0';
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoTimingDiscover(uint8_t slot, uint32_t elapsedMs) {
  if (slot >= S3XY_MAX_DEVICES) return;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) s3xyDevices[slot].lastDiscoverMs = elapsedMs;
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoTimingReady(uint8_t slot) {
  if (slot >= S3XY_MAX_DEVICES) return;
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used && s3xyDevices[slot].autoAttemptStartMs)
    s3xyDevices[slot].lastReadyMs = now - s3xyDevices[slot].autoAttemptStartMs;
  portEXIT_CRITICAL(&s3xyMux);
}

#else
#define s3xySetAutoTrace(...) do {} while (0)
#define s3xyAutoTimingBegin(...) do {} while (0)
#define s3xyAutoTimingPath(...) do {} while (0)
#define s3xyAutoTimingAdopt(...) do {} while (0)
#define s3xyAutoTimingDiscover(...) do {} while (0)
#define s3xyAutoTimingReady(...) do {} while (0)
#endif

static String s3xyJsonEscape(const char *src) {
  String s = src ? String(src) : String("");
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "");
  return s;
}

static bool s3xyAddressEquals(const char *a, const char *b) {
  if (!a || !b || !a[0] || !b[0]) return false;
  while (*a && *b) {
    char ca = *a++, cb = *b++;
    if (ca >= 'A' && ca <= 'F') ca = (char)(ca - 'A' + 'a');
    if (cb >= 'A' && cb <= 'F') cb = (char)(cb - 'A' + 'a');
    if (ca != cb) return false;
  }
  return *a == '\0' && *b == '\0';
}

static int s3xyFindSlotByAddress(const char *address) {
  if (!address || !address[0]) return -1;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    bool used;
    char a[24];
    portENTER_CRITICAL(&s3xyMux);
    used = s3xyDevices[i].used;
    strncpy(a, s3xyDevices[i].address, sizeof(a) - 1);
    a[sizeof(a) - 1] = '\0';
    portEXIT_CRITICAL(&s3xyMux);
    if (used && s3xyAddressEquals(a, address)) return (int)i;
  }
  return -1;
}

static int s3xyFindSlotByPeerId(const uint8_t *peerId, size_t peerIdLen) {
  if (!peerId || peerIdLen == 0 || peerIdLen > S3XY_PEER_ID_MAX) return -1;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    bool used;
    uint8_t saved[S3XY_PEER_ID_MAX] = {};
    uint8_t savedLen = 0;
    portENTER_CRITICAL(&s3xyMux);
    used = s3xyDevices[i].used;
    savedLen = s3xyDevices[i].peerIdLen;
    if (savedLen > S3XY_PEER_ID_MAX) savedLen = 0;
    if (savedLen) memcpy(saved, s3xyDevices[i].peerId, savedLen);
    portEXIT_CRITICAL(&s3xyMux);
    if (used && s3xyPeerIdEquals(saved, savedLen, peerId, peerIdLen)) return (int)i;
  }
  return -1;
}

static bool s3xyHasEligibleSavedIdentity() {
  bool found = false;
  portENTER_CRITICAL(&s3xyMux);
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    const S3xyDeviceSlot &d = s3xyDevices[i];
    if (d.used && d.autoConnect && !d.connected && !d.manualPaused && d.peerIdLen > 0) {
      found = true;
      break;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  return found;
}

static bool s3xyIdentityCandidateCoolingDown(const char *address) {
  if (!address || !address[0]) return false;
  bool blocked = false;
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyIdentityRejectedAddress[0] && s3xyAddressEquals(s3xyIdentityRejectedAddress, address) &&
      (int32_t)(s3xyIdentityRejectedUntilMs - now) > 0) blocked = true;
  portEXIT_CRITICAL(&s3xyMux);
  return blocked;
}

static void s3xyRejectIdentityCandidate(const char *address) {
  if (!address || !address[0]) return;
  portENTER_CRITICAL(&s3xyMux);
  strncpy(s3xyIdentityRejectedAddress, address, sizeof(s3xyIdentityRejectedAddress) - 1);
  s3xyIdentityRejectedAddress[sizeof(s3xyIdentityRejectedAddress) - 1] = '\0';
  s3xyIdentityRejectedUntilMs = millis() + S3XY_IDENTITY_REJECT_MS;
  portEXIT_CRITICAL(&s3xyMux);
}

static int s3xyLocalBondCount() {
  bool enabled;
  portENTER_CRITICAL(&s3xyMux);
  enabled = s3xyBluetoothEnabled;
  portEXIT_CRITICAL(&s3xyMux);
  if (!enabled || !s3xyBleInitialized) return 0;

#if defined(CONFIG_BLUEDROID_ENABLED)
  const int n = esp_ble_get_bond_device_num();
  return n > 0 ? n : 0;
#elif defined(CONFIG_NIMBLE_ENABLED)
  // Arduino-ESP32 3.3+ can use the unified BLEDevice API on NimBLE targets.
  // Query the NimBLE bond store directly so the boot diagnostic reflects real bonds.
  ble_addr_t peers[8] = {};
  int n = 0;
  const int rc = ble_store_util_bonded_peers(peers, &n, 8);
  if (rc != 0 || n < 0) return 0;
  return n;
#else
  return 0;
#endif
}

static int s3xyFindFreeSlot() {
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    bool used;
    portENTER_CRITICAL(&s3xyMux);
    used = s3xyDevices[i].used;
    portEXIT_CRITICAL(&s3xyMux);
    if (!used) return (int)i;
  }
  return -1;
}

static uint8_t s3xyRegisteredCount() {
  uint8_t n = 0;
  portENTER_CRITICAL(&s3xyMux);
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) if (s3xyDevices[i].used) n++;
  portEXIT_CRITICAL(&s3xyMux);
  return n;
}

static uint8_t s3xyConnectedCount() {
  uint8_t n = 0;
  portENTER_CRITICAL(&s3xyMux);
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++)
    if (s3xyDevices[i].used && s3xyDevices[i].connected) n++;
  portEXIT_CRITICAL(&s3xyMux);
  return n;
}

static bool s3xySlotIsUsed(int slot) {
  if (slot < 0 || slot >= (int)S3XY_MAX_DEVICES) return false;
  bool used;
  portENTER_CRITICAL(&s3xyMux);
  used = s3xyDevices[slot].used;
  portEXIT_CRITICAL(&s3xyMux);
  return used;
}

static void s3xyDefaultName(uint8_t slot, char *out, size_t outLen) {
  snprintf(out, outLen, "S3XY Button %02u", (unsigned)(slot + 1));
}

static void s3xyRegistrySaveSlot(uint8_t slot) {
  if (slot >= S3XY_MAX_DEVICES) return;
  bool used;
  char addr[24], name[28], idHex[64];
  uint8_t peerId[S3XY_PEER_ID_MAX] = {};
  uint8_t peerIdLen = 0;
  uint8_t addressType, singleAction, doubleAction, longAction;
  bool addressTypeKnown, autoConnect;
  portENTER_CRITICAL(&s3xyMux);
  used = s3xyDevices[slot].used;
  strncpy(addr, s3xyDevices[slot].address, sizeof(addr) - 1); addr[sizeof(addr)-1] = '\0';
  strncpy(name, s3xyDevices[slot].name, sizeof(name) - 1); name[sizeof(name)-1] = '\0';
  strncpy(idHex, s3xyDevices[slot].idHex, sizeof(idHex) - 1); idHex[sizeof(idHex)-1] = '\0';
  peerIdLen = s3xyDevices[slot].peerIdLen;
  if (peerIdLen > S3XY_PEER_ID_MAX) peerIdLen = 0;
  if (peerIdLen) memcpy(peerId, s3xyDevices[slot].peerId, peerIdLen);
  addressType = s3xyDevices[slot].addressType;
  addressTypeKnown = s3xyDevices[slot].addressTypeKnown;
  singleAction = s3xyDevices[slot].singleAction;
  doubleAction = s3xyDevices[slot].doubleAction;
  longAction = s3xyDevices[slot].longAction;
  autoConnect = s3xyDevices[slot].autoConnect;
  portEXIT_CRITICAL(&s3xyMux);

  Preferences p;
  if (!p.begin("s3xyreg", false)) {
    if (peerIdLen) {
      portENTER_CRITICAL(&s3xyMux); s3xyDevices[slot].identityPersistVerified = false; portEXIT_CRITICAL(&s3xyMux);
      s3xyLogPush(S3XY_LOG_ERROR, "NVS open failed while saving 3D49 identity", nullptr, 0, -127, (int8_t)slot);
    }
    return;
  }
  String sa = "a" + String(slot), sn = "n" + String(slot), si = "i" + String(slot);
  String sb = "b" + String(slot);  // Raw stable 3D49 bytes
  String st = "t" + String(slot), sv = "v" + String(slot);
  String ss = "s" + String(slot), sd = "d" + String(slot), sl = "l" + String(slot);
  String sc = "c" + String(slot);
  if (used && addr[0]) {
    p.putString(sa.c_str(), addr);
    p.putString(sn.c_str(), name);
    if (peerIdLen) {
      p.putBytes(sb.c_str(), peerId, peerIdLen);
      p.remove(si.c_str());  // raw bN is canonical; retire the legacy hex-string duplicate
    } else {
      p.remove(sb.c_str());
      if (idHex[0]) p.putString(si.c_str(), idHex); else p.remove(si.c_str());
    }
    p.putUChar(st.c_str(), addressType);
    p.putBool(sv.c_str(), addressTypeKnown);
    p.putUChar(ss.c_str(), singleAction);
    p.putUChar(sd.c_str(), doubleAction);
    p.putUChar(sl.c_str(), longAction);
    p.putBool(sc.c_str(), autoConnect);
  } else {
    p.remove(sa.c_str()); p.remove(sn.c_str()); p.remove(si.c_str()); p.remove(sb.c_str());
    p.remove(st.c_str()); p.remove(sv.c_str());
    p.remove(ss.c_str()); p.remove(sd.c_str()); p.remove(sl.c_str()); p.remove(sc.c_str());
  }
  p.end();

  // Do not trust a successful-looking put call. Re-open NVS read-only and verify
  // the exact raw 3D49 bytes that must survive a T-2CAN reboot.
  bool verified = false;
  if (used && peerIdLen) {
    Preferences v;
    if (v.begin("s3xyreg", true)) {
      const size_t storedLen = v.getBytesLength(sb.c_str());
      uint8_t stored[S3XY_PEER_ID_MAX] = {};
      const size_t got = (storedLen > 0 && storedLen <= S3XY_PEER_ID_MAX) ?
                         v.getBytes(sb.c_str(), stored, sizeof(stored)) : 0;
      verified = (got == peerIdLen) && s3xyPeerIdEquals(stored, got, peerId, peerIdLen);
      v.end();
    }
  }
  portENTER_CRITICAL(&s3xyMux);
  if (slot < S3XY_MAX_DEVICES && s3xyDevices[slot].used)
    s3xyDevices[slot].identityPersistVerified = peerIdLen ? verified : false;
  portEXIT_CRITICAL(&s3xyMux);
  if (peerIdLen && !verified)
    s3xyLogPush(S3XY_LOG_ERROR, "3D49 NVS read-back verification FAILED", peerId, peerIdLen, -127, (int8_t)slot);
}

static void s3xyRegistryClearSlotRuntime(uint8_t slot, bool keepClient = true) {
  if (slot >= S3XY_MAX_DEVICES) return;
  BLEClient *client = s3xyDevices[slot].client;
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].used = false;
  s3xyDevices[slot].address[0] = '\0';
  s3xyDevices[slot].name[0] = '\0';
  s3xyDevices[slot].addressType = 0;
  s3xyDevices[slot].addressTypeKnown = false;
  s3xyDevices[slot].singleAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].doubleAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].longAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].autoConnect = true;
  s3xyDevices[slot].state = S3XY_MAP_IDLE;
  s3xyDevices[slot].connected = false;
  s3xyDevices[slot].subscribed = false;
  s3xyDevices[slot].secureOk = false;
  s3xyDevices[slot].manualPaused = false;
  s3xyDevices[slot].rssi = -127;
  s3xyDevices[slot].notifyCount = 0;
  s3xyDevices[slot].unparsedCount = 0;
  s3xyDevices[slot].singleCount = 0;
  s3xyDevices[slot].doubleCount = 0;
  s3xyDevices[slot].longCount = 0;
  s3xyDevices[slot].handshakeAckCount = 0;
  s3xyDevices[slot].lastNotifyMs = 0;
  s3xyDevices[slot].lastNotifyLen = 0;
  memset(s3xyDevices[slot].lastNotify, 0, sizeof(s3xyDevices[slot].lastNotify));
  s3xyDevices[slot].lastError[0] = '\0';
  s3xyDevices[slot].idHex[0] = '\0';
  memset(s3xyDevices[slot].peerId, 0, sizeof(s3xyDevices[slot].peerId));
  s3xyDevices[slot].peerIdLen = 0;
  s3xyDevices[slot].identityPersistVerified = false;
  s3xyDevices[slot].nextAttemptMs = 0;
  s3xyDevices[slot].autoAttempts = 0;
  s3xyDevices[slot].autoReconnects = 0;
  s3xyDevices[slot].autoFailures = 0;
  s3xyDevices[slot].consecutiveFailures = 0;
  s3xyDevices[slot].notifyChar = nullptr;
  s3xyDevices[slot].idChar = nullptr;
  if (!keepClient) s3xyDevices[slot].client = nullptr;
  else s3xyDevices[slot].client = client;
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyAutoLoadConfig() {
  bool autoEnabled = true;
  bool bluetoothEnabled = false;
  String oldAddr;
  bool migrateRawId[S3XY_MAX_DEVICES] = {};
  bool sanitizeActions[S3XY_MAX_DEVICES] = {};
  Preferences legacy;
  if (legacy.begin("s3xy", true)) {
    autoEnabled = legacy.getBool("auto", true);
    bluetoothEnabled = legacy.getBool("bt", false);
    oldAddr = legacy.getString("addr", "");
    legacy.end();
  }

  portENTER_CRITICAL(&s3xyMux);
  s3xyAutoEnabled = autoEnabled;
  s3xyBluetoothEnabled = bluetoothEnabled;
  portEXIT_CRITICAL(&s3xyMux);

  Preferences p;
  if (p.begin("s3xyreg", true)) {
    for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
      String sa = "a" + String(i), sn = "n" + String(i), si = "i" + String(i);
      String sb = "b" + String(i);
      String st = "t" + String(i), sv = "v" + String(i);
      String ss = "s" + String(i), sd = "d" + String(i), sl = "l" + String(i);
      String sc = "c" + String(i);
      String addr = p.getString(sa.c_str(), "");
      if (!addr.length()) continue;
      String name = p.getString(sn.c_str(), "");
      if (!name.length()) {
        char defName[28]; s3xyDefaultName(i, defName, sizeof(defName)); name = defName;
      }

      uint8_t peerId[S3XY_PEER_ID_MAX] = {};
      uint8_t peerIdLen = 0;
      const size_t rawLen = p.getBytesLength(sb.c_str());
      if (rawLen > 0 && rawLen <= S3XY_PEER_ID_MAX) {
        const size_t got = p.getBytes(sb.c_str(), peerId, sizeof(peerId));
        if (got == rawLen) peerIdLen = (uint8_t)got;
      }

      String idHex = p.getString(si.c_str(), "");
      if (!peerIdLen && idHex.length()) {
        peerIdLen = (uint8_t)s3xyHexToBytes(idHex.c_str(), peerId, sizeof(peerId));
        if (peerIdLen) migrateRawId[i] = true;
      }
      if (peerIdLen) {
        char normalized[64] = {};
        s3xyBytesToHex(peerId, peerIdLen, normalized, sizeof(normalized));
        idHex = normalized;
      }

      const uint8_t addressType = p.getUChar(st.c_str(), 0);
      const bool addressTypeKnown = p.getBool(sv.c_str(), false);
      const uint8_t storedS = p.getUChar(ss.c_str(), S3XY_ACTION_NONE);
      const uint8_t storedD = p.getUChar(sd.c_str(), S3XY_ACTION_NONE);
      const uint8_t storedL = p.getUChar(sl.c_str(), S3XY_ACTION_NONE);
      uint8_t s = storedS;
      uint8_t d = storedD;
      uint8_t l = storedL;
      if (!s3xyActionSupportedForCurrentProfile(s)) s = S3XY_ACTION_NONE;
      if (!s3xyActionSupportedForCurrentProfile(d)) d = S3XY_ACTION_NONE;
      if (!s3xyActionSupportedForCurrentProfile(l)) l = S3XY_ACTION_NONE;
      sanitizeActions[i] = (s != storedS) || (d != storedD) || (l != storedL);
      const bool c = p.getBool(sc.c_str(), true);
      portENTER_CRITICAL(&s3xyMux);
      s3xyDevices[i].used = true;
      strncpy(s3xyDevices[i].address, addr.c_str(), sizeof(s3xyDevices[i].address) - 1);
      s3xyDevices[i].address[sizeof(s3xyDevices[i].address) - 1] = '\0';
      strncpy(s3xyDevices[i].name, name.c_str(), sizeof(s3xyDevices[i].name) - 1);
      s3xyDevices[i].name[sizeof(s3xyDevices[i].name) - 1] = '\0';
      strncpy(s3xyDevices[i].idHex, idHex.c_str(), sizeof(s3xyDevices[i].idHex) - 1);
      s3xyDevices[i].idHex[sizeof(s3xyDevices[i].idHex) - 1] = '\0';
      memset(s3xyDevices[i].peerId, 0, sizeof(s3xyDevices[i].peerId));
      if (peerIdLen) memcpy(s3xyDevices[i].peerId, peerId, peerIdLen);
      s3xyDevices[i].peerIdLen = peerIdLen;
      s3xyDevices[i].identityPersistVerified = peerIdLen > 0;
      s3xyDevices[i].addressType = addressType;
      s3xyDevices[i].addressTypeKnown = addressTypeKnown;
      s3xyDevices[i].singleAction = s;
      s3xyDevices[i].doubleAction = d;
      s3xyDevices[i].longAction = l;
      s3xyDevices[i].autoConnect = c;
      s3xyDevices[i].state = bluetoothEnabled ? S3XY_MAP_IDLE : S3XY_MAP_OFF;
      s3xyDevices[i].rssi = -127;
      s3xyDevices[i].nextAttemptMs = (bluetoothEnabled && autoEnabled && c) ?
                                     (millis() + S3XY_AUTO_BOOT_DELAY_MS + (uint32_t)i * 500U) : 0;
      portEXIT_CRITICAL(&s3xyMux);
    }
    p.end();
  }

  // Upgrade legacy hex-string 3D49 identity into the raw-byte key
  // immediately. s3xyRegistrySaveSlot re-opens NVS and verifies the bytes.
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (migrateRawId[i] || sanitizeActions[i]) s3xyRegistrySaveSlot(i);
  }

  // One-time compatibility migration from the previous single-target build.
  if (s3xyRegisteredCount() == 0 && oldAddr.length()) {
    char defName[28]; s3xyDefaultName(0, defName, sizeof(defName));
    portENTER_CRITICAL(&s3xyMux);
    s3xyDevices[0].used = true;
    strncpy(s3xyDevices[0].address, oldAddr.c_str(), sizeof(s3xyDevices[0].address) - 1);
    s3xyDevices[0].address[sizeof(s3xyDevices[0].address) - 1] = '\0';
    strncpy(s3xyDevices[0].name, defName, sizeof(s3xyDevices[0].name) - 1);
    s3xyDevices[0].name[sizeof(s3xyDevices[0].name) - 1] = '\0';
    s3xyDevices[0].addressType = 0;
    s3xyDevices[0].addressTypeKnown = false;
    s3xyDevices[0].idHex[0] = '\0';
    memset(s3xyDevices[0].peerId, 0, sizeof(s3xyDevices[0].peerId));
    s3xyDevices[0].peerIdLen = 0;
    s3xyDevices[0].identityPersistVerified = false;
    s3xyDevices[0].singleAction = S3XY_ACTION_NOA_CANCEL;
    s3xyDevices[0].doubleAction = S3XY_ACTION_NONE;
    s3xyDevices[0].longAction = S3XY_ACTION_NONE;
    s3xyDevices[0].autoConnect = true;
    s3xyDevices[0].state = bluetoothEnabled ? S3XY_MAP_IDLE : S3XY_MAP_OFF;
    s3xyDevices[0].rssi = -127;
    s3xyDevices[0].nextAttemptMs = (bluetoothEnabled && autoEnabled) ? (millis() + S3XY_AUTO_BOOT_DELAY_MS) : 0;
    portEXIT_CRITICAL(&s3xyMux);
    s3xyRegistrySaveSlot(0);
    s3xyLogPush(S3XY_LOG_INFO, "migrated legacy saved target to registry", nullptr, 0, -127, 0);
  }

  // The old single-target key must not survive migration; otherwise a later
  // Forget of the last registry device would resurrect it on the next boot.
  if (oldAddr.length() && s3xyRegisteredCount() > 0) {
    Preferences cleanup;
    if (cleanup.begin("s3xy", false)) { cleanup.remove("addr"); cleanup.end(); }
  }
}

static void s3xyAutoSetEnabled(bool enabled, bool persist = true) {
  if (persist) {
    Preferences p;
    if (p.begin("s3xy", false)) {
      p.putBool("auto", enabled);
      p.end();
    }
  }
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  s3xyAutoEnabled = enabled;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    if (enabled) {
      s3xyDevices[i].manualPaused = false;
      if (s3xyDevices[i].autoConnect && !s3xyDevices[i].connected) s3xyDevices[i].nextAttemptMs = now + 250U + (uint32_t)i * 250U;
      else s3xyDevices[i].nextAttemptMs = 0;
    } else {
      s3xyDevices[i].nextAttemptMs = 0;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static bool s3xyBluetoothMasterPersist(bool enabled) {
  Preferences p;
  if (!p.begin("s3xy", false)) return false;
  const size_t n = p.putBool("bt", enabled);
  p.end();
  if (n == 0) return false;
  portENTER_CRITICAL(&s3xyMux);
  s3xyBluetoothEnabled = enabled;
  if (!enabled) {
    s3xyScanMode = S3XY_SCAN_NONE;
    s3xyDiscoveryScanning = false;
    for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
      if (!s3xyDevices[i].used) continue;
      s3xyDevices[i].nextAttemptMs = 0;
      s3xyDevices[i].state = S3XY_MAP_OFF;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  return true;
}

static bool s3xyBluetoothMasterIsEnabled() {
  bool enabled;
  portENTER_CRITICAL(&s3xyMux);
  enabled = s3xyBluetoothEnabled;
  portEXIT_CRITICAL(&s3xyMux);
  return enabled;
}

static bool s3xyRuntimeStopIsPending() {
  bool pending;
  portENTER_CRITICAL(&s3xyMux);
  pending = s3xyRuntimeStopRequested || s3xyRuntimeStopping;
  portEXIT_CRITICAL(&s3xyMux);
  return pending;
}

static bool s3xyMapperTaskIsRunning() {
  bool running;
  portENTER_CRITICAL(&s3xyMux);
  running = s3xyTaskHandle != nullptr;
  portEXIT_CRITICAL(&s3xyMux);
  return running;
}

static bool s3xyWaitForRuntimeStopped(uint32_t timeoutMs) {
  const uint32_t started = millis();
  while (s3xyMapperTaskIsRunning()) {
    if ((uint32_t)(millis() - started) >= timeoutMs) return false;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return true;
}

static void s3xyRuntimeRearmAutoReconnect() {
  bool autoEnabled;
  portENTER_CRITICAL(&s3xyMux);
  autoEnabled = s3xyAutoEnabled;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    s3xyDevices[i].state = S3XY_MAP_IDLE;
  }
  portEXIT_CRITICAL(&s3xyMux);

  // Reuse the proven global auto scheduler. Passing the existing setting with
  // persist=false preserves NVS while re-arming per-device nextAttemptMs.
  s3xyAutoSetEnabled(autoEnabled, false);
}

static void s3xyRequestRuntimeStop() {
  bool initialized;
  portENTER_CRITICAL(&s3xyMux);
  s3xyRuntimeStopRequested = true;
  initialized = s3xyBleInitialized;
  s3xyCommand.type = S3XY_CMD_NONE;
  s3xyCommand.slot = -1;
  s3xyCommand.address[0] = '\0';
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyScanSlot = -1;
  s3xyScanTargetFound = false;
  s3xyScanTargetAddress[0] = '\0';
  s3xyDiscoveryScanning = false;
  s3xyActionPending = 0;
  s3xyAccelActionPending = 0;
  s3xyResearchCaptureAPending = 0;
  s3xyResearchCaptureBPending = 0;
  s3xyResearchCaptureCPending = 0;
  s3xyResearchCaptureDPending = 0;
  s3xyResearchCaptureResetPending = 0;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    s3xyDevices[i].nextAttemptMs = 0;
    s3xyDevices[i].state = S3XY_MAP_OFF;
  }
  portEXIT_CRITICAL(&s3xyMux);

  // Interrupt a blocking discovery/target scan so the mapper can enter its
  // serialized shutdown path promptly. Deinit itself remains mapper-owned.
  if (initialized) {
    BLEScan *scan = BLEDevice::getScan();
    if (scan) scan->stop();
  }
}

static void s3xySetSlotError(uint8_t slot, const char *msg) {
  if (slot >= S3XY_MAX_DEVICES) return;
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].state = S3XY_MAP_ERROR;
  strncpy(s3xyDevices[slot].lastError, msg ? msg : "unknown", sizeof(s3xyDevices[slot].lastError) - 1);
  s3xyDevices[slot].lastError[sizeof(s3xyDevices[slot].lastError) - 1] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
  s3xyLogPush(S3XY_LOG_ERROR, msg ? msg : "unknown", nullptr, 0, -127, (int8_t)slot);
}

class S3xyClientCallbacks : public BLEClientCallbacks {
 public:
  explicit S3xyClientCallbacks(uint8_t slot) : slot_(slot) {}

  void onConnect(BLEClient *client) override {
    (void)client;
    if (slot_ >= S3XY_MAX_DEVICES) return;
    portENTER_CRITICAL(&s3xyMux);
    if (s3xyDevices[slot_].used) {
      s3xyDevices[slot_].connected = true;
      s3xyDevices[slot_].state = S3XY_MAP_CONNECTED;
    }
    portEXIT_CRITICAL(&s3xyMux);
    s3xyLogPush(S3XY_LOG_CONNECT, "BLE connected", nullptr, 0, -127, (int8_t)slot_);
  }

  void onDisconnect(BLEClient *client) override {
    (void)client;
    if (slot_ >= S3XY_MAX_DEVICES) return;
    const uint32_t now = millis();
    bool shouldRetry = false;
    portENTER_CRITICAL(&s3xyMux);
    if (s3xyDevices[slot_].used) {
      s3xyDevices[slot_].connected = false;
      s3xyDevices[slot_].subscribed = false;
      s3xyDevices[slot_].secureOk = false;
      s3xyDevices[slot_].notifyChar = nullptr;
      s3xyDevices[slot_].idChar = nullptr;
      if (s3xyDevices[slot_].state != S3XY_MAP_ERROR) s3xyDevices[slot_].state = S3XY_MAP_IDLE;
      shouldRetry = s3xyBluetoothEnabled && s3xyAutoEnabled && s3xyDevices[slot_].autoConnect && !s3xyDevices[slot_].manualPaused;
      if (shouldRetry) s3xyDevices[slot_].nextAttemptMs = now + S3XY_AUTO_DISCONNECT_RETRY_MS;
    }
    portEXIT_CRITICAL(&s3xyMux);
    s3xyLogPush(S3XY_LOG_DISCONNECT, shouldRetry ? "BLE disconnected; reconnect scheduled" : "BLE disconnected", nullptr, 0, -127, (int8_t)slot_);
  }

 private:
  uint8_t slot_;
};

static S3xyClientCallbacks *s3xyClientCallbacks[S3XY_MAX_DEVICES] = {};

static void s3xyRunMappedAction(uint8_t slot, uint8_t action, const char *gesture) {
  if (action == S3XY_ACTION_NONE) return;
  if (!s3xyActionSupportedForCurrentProfile(action)) {
    s3xyLogPush(S3XY_LOG_INFO, "Mapped action unavailable for current vehicle profile", nullptr, 0, -127, (int8_t)slot);
    return;
  }
  portENTER_CRITICAL(&s3xyMux);
  if (action == S3XY_ACTION_NOA_CANCEL) s3xyActionPending++;
  else if (action == S3XY_ACTION_ACCEL_MODE_TOGGLE) s3xyAccelActionPending++;
  else if (action == S3XY_ACTION_RESEARCH_CAPTURE_A) s3xyResearchCaptureAPending++;
  else if (action == S3XY_ACTION_RESEARCH_CAPTURE_B) s3xyResearchCaptureBPending++;
  else if (action == S3XY_ACTION_RESEARCH_CAPTURE_C) s3xyResearchCaptureCPending++;
  else if (action == S3XY_ACTION_RESEARCH_CAPTURE_D) s3xyResearchCaptureDPending++;
  else if (action == S3XY_ACTION_RESEARCH_CAPTURE_RESET) s3xyResearchCaptureResetPending++;
  portEXIT_CRITICAL(&s3xyMux);
  String d = String(gesture ? gesture : "gesture") + " -> " + s3xyActionLabel(action);
  s3xyLogPush(S3XY_LOG_INFO, d.c_str(), nullptr, 0, -127, (int8_t)slot);
}

// Gen2 button notifications are normally the bare two-byte tokens validated in
// the original single-button implementation (C1 01 / C1 02 / C3 01).  Some
// firmware/library combinations can deliver the same token inside a longer
// notification buffer.  Do not require len==2: look for the validated token
// anywhere in the 3D50 payload, while still accepting only known tokens.
static int s3xyFindToken2(const uint8_t *data, size_t len, uint8_t a, uint8_t b) {
  if (!data || len < 2) return -1;
  for (size_t i = 0; i + 1 < len; i++) {
    if (data[i] == a && data[i + 1] == b) return (int)i;
  }
  return -1;
}

static int s3xyFindToken3(const uint8_t *data, size_t len, uint8_t a, uint8_t b, uint8_t c1, uint8_t c2) {
  if (!data || len < 3) return -1;
  for (size_t i = 0; i + 2 < len; i++) {
    if (data[i] == a && data[i + 1] == b && (data[i + 2] == c1 || data[i + 2] == c2)) return (int)i;
  }
  return -1;
}

static void s3xyNotifyCallbackForSlot(uint8_t slot, BLERemoteCharacteristic *chr,
                                      uint8_t *data, size_t len, bool isNotify) {
  (void)chr;
  (void)isNotify;
  if (slot >= S3XY_MAX_DEVICES) return;

  bool used = false;
  portENTER_CRITICAL(&s3xyMux);
  used = s3xyDevices[slot].used;
  portEXIT_CRITICAL(&s3xyMux);
  if (!used) {
    s3xyLogPush(S3XY_LOG_NOTIFY, "notify for unused slot", data, len, -127, (int8_t)slot);
    return;
  }

  const bool single = s3xyFindToken2(data, len, 0xC1, 0x01) >= 0;
  const bool dbl    = s3xyFindToken2(data, len, 0xC1, 0x02) >= 0;
  const bool lng    = s3xyFindToken2(data, len, 0xC3, 0x01) >= 0;
  const bool ack    = s3xyFindToken3(data, len, 0xC7, 0x00, 0x01, 0xC3) >= 0;
  const uint8_t n = (uint8_t)min(len, (size_t)S3XY_LOG_DATA_MAX);
  uint8_t action = S3XY_ACTION_NONE;
  const char *gesture = nullptr;
  int16_t rssi = -127;

  // A notification should contain only one button gesture.  If an unexpected
  // payload contains more than one validated token, reject the gesture as
  // ambiguous rather than dispatching a vehicle action.
  const uint8_t gestureMatches = (single ? 1 : 0) + (dbl ? 1 : 0) + (lng ? 1 : 0);

  portENTER_CRITICAL(&s3xyMux);
  S3xyDeviceSlot &d = s3xyDevices[slot];
  d.notifyCount++;
  d.lastNotifyMs = millis();
  d.lastNotifyLen = n;
  if (n) memcpy(d.lastNotify, data, n);
  if (gestureMatches == 1) {
    if (single) { d.singleCount++; action = d.singleAction; gesture = "single"; }
    else if (dbl) { d.doubleCount++; action = d.doubleAction; gesture = "double"; }
    else if (lng) { d.longCount++; action = d.longAction; gesture = "long"; }
  } else if (!ack) {
    d.unparsedCount++;
  }
  if (ack) d.handshakeAckCount++;
  rssi = d.rssi;
  portEXIT_CRITICAL(&s3xyMux);

  if (gesture) s3xyRunMappedAction(slot, action, gesture);

  if (gestureMatches > 1) {
    s3xyLogPush(S3XY_LOG_NOTIFY, "ambiguous button notify", data, len, rssi, (int8_t)slot);
  } else if (gesture) {
    String detail = String("button ") + gesture;
    s3xyLogPush(S3XY_LOG_NOTIFY, detail.c_str(), data, len, rssi, (int8_t)slot);
  } else if (ack) {
    s3xyLogPush(S3XY_LOG_NOTIFY, "B6 ACK notify", data, len, rssi, (int8_t)slot);
  } else {
    s3xyLogPush(S3XY_LOG_NOTIFY, "unparsed button notify", data, len, rssi, (int8_t)slot);
  }
}

// Bind the BLE library callback to a fixed registry slot. The earlier implementation attempted to
// reverse-map a callback to a slot by comparing BLERemoteCharacteristic*
// pointers.  That is unnecessary and fragile with multiple clients.  Fixed
// wrappers make notification ownership deterministic.
static void s3xyNotifyCallbackSlot0(BLERemoteCharacteristic *chr, uint8_t *data, size_t len, bool isNotify) {
  s3xyNotifyCallbackForSlot(0, chr, data, len, isNotify);
}
static void s3xyNotifyCallbackSlot1(BLERemoteCharacteristic *chr, uint8_t *data, size_t len, bool isNotify) {
  s3xyNotifyCallbackForSlot(1, chr, data, len, isNotify);
}
static void s3xyNotifyCallbackSlot2(BLERemoteCharacteristic *chr, uint8_t *data, size_t len, bool isNotify) {
  s3xyNotifyCallbackForSlot(2, chr, data, len, isNotify);
}

static void (*s3xyNotifyCallbackForSlotIndex(uint8_t slot))(BLERemoteCharacteristic*, uint8_t*, size_t, bool) {
  switch (slot) {
    case 0: return s3xyNotifyCallbackSlot0;
    case 1: return s3xyNotifyCallbackSlot1;
    case 2: return s3xyNotifyCallbackSlot2;
    default: return nullptr;
  }
}

static bool s3xyIsRegisteredAddress(const char *address) {
  return s3xyFindSlotByAddress(address) >= 0;
}

static void s3xyDiscoveryAdd(const char *address, const char *name, int16_t rssi) {
  if (!address || !address[0]) return;
  portENTER_CRITICAL(&s3xyMux);
  int found = -1;
  for (uint8_t i = 0; i < s3xyDiscoveredCount; i++) {
    if (s3xyAddressEquals(s3xyDiscovered[i].address, address)) { found = i; break; }
  }
  if (found < 0 && s3xyDiscoveredCount < S3XY_DISCOVERY_MAX) found = s3xyDiscoveredCount++;
  if (found >= 0) {
    strncpy(s3xyDiscovered[found].address, address, sizeof(s3xyDiscovered[found].address) - 1);
    s3xyDiscovered[found].address[sizeof(s3xyDiscovered[found].address) - 1] = '\0';
    strncpy(s3xyDiscovered[found].name, (name && name[0]) ? name : "ENH_BTN", sizeof(s3xyDiscovered[found].name) - 1);
    s3xyDiscovered[found].name[sizeof(s3xyDiscovered[found].name) - 1] = '\0';
    s3xyDiscovered[found].rssi = rssi;
  }
  portEXIT_CRITICAL(&s3xyMux);
  if (found >= 0) {
    const bool registered = s3xyIsRegisteredAddress(address);
    portENTER_CRITICAL(&s3xyMux);
    s3xyDiscovered[found].registered = registered;
    portEXIT_CRITICAL(&s3xyMux);
  }
}

class S3xyAdvertisedCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    char addr[24] = {};
    snprintf(addr, sizeof(addr), "%s", dev.getAddress().toString().c_str());
    const uint8_t addressType = dev.getAddressType();
    const int16_t rssi = (int16_t)dev.getRSSI();

    // Registry reconnect always checks the exact saved address first. Identity recovery also
    // uses ACTIVE registry scanning so ENH_BTN / service 3D46 scan-response data
    // can identify a candidate whose over-the-air address changed after reboot.
    uint8_t mode;
    char target[24];
    int8_t slot;
    portENTER_CRITICAL(&s3xyMux);
    mode = s3xyScanMode;
    slot = s3xyScanSlot;
    strncpy(target, s3xyScanTargetAddress, sizeof(target) - 1);
    target[sizeof(target)-1] = '\0';
    portEXIT_CRITICAL(&s3xyMux);

    if (mode == S3XY_SCAN_REGISTRY_BATCH) {
      const int foundSlot = s3xyFindSlotByAddress(addr);
      bool eligible = false;
      if (foundSlot >= 0 && foundSlot < (int)S3XY_MAX_DEVICES) {
        portENTER_CRITICAL(&s3xyMux);
        const S3xyDeviceSlot &d = s3xyDevices[foundSlot];
        eligible = s3xyBluetoothEnabled && s3xyAutoEnabled && d.used && d.autoConnect &&
                   !d.connected && !d.manualPaused && d.nextAttemptMs != 0;
        portEXIT_CRITICAL(&s3xyMux);
      }
      if (eligible) {
        if (s3xyBatchTargets[foundSlot]) { delete s3xyBatchTargets[foundSlot]; s3xyBatchTargets[foundSlot] = nullptr; }
        s3xyBatchTargets[foundSlot] = new BLEAdvertisedDevice(dev);
        portENTER_CRITICAL(&s3xyMux);
        s3xyDevices[foundSlot].state = S3XY_MAP_FOUND;
        s3xyDevices[foundSlot].rssi = rssi;
        s3xyDevices[foundSlot].addressType = addressType;
        s3xyDevices[foundSlot].addressTypeKnown = true;
        portEXIT_CRITICAL(&s3xyMux);
        String detail = String("batch addr=") + addr + " t=" + String((unsigned)addressType) + " raw-match";
        s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, (int8_t)foundSlot);
        uint8_t batchFound = 0;
        for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) if (s3xyBatchTargets[i]) batchFound++;
        if (s3xyBatchExpectedCount > 0 && batchFound >= s3xyBatchExpectedCount) {
          s3xyLogPush(S3XY_LOG_INFO, "batch exact complete; scan stopped early");
          BLEDevice::getScan()->stop();
        }
        return; // otherwise keep scanning so Button 02/03 can be collected in the same window
      }

      // Keep one changed-address candidate from the same active scan. It will
      // be identity-probed only after the scan has ended and exact peers have
      // entered the normal serialized link pipeline.
      String name(dev.getName().c_str());
      const bool nameMatch = (name == "ENH_BTN");
      bool svcMatch = false;
      if (dev.haveServiceUUID()) svcMatch = dev.isAdvertisingService(BLEUUID(S3XY_SERVICE_UUID));
      if (foundSlot < 0 && (nameMatch || svcMatch) && s3xyHasEligibleSavedIdentity() &&
          !s3xyIdentityCandidateCoolingDown(addr) && !s3xyBatchIdentityTarget) {
        s3xyBatchIdentityTarget = new BLEAdvertisedDevice(dev);
        String detail = String("batch identity candidate addr=") + addr + " t=" + String((unsigned)addressType);
        s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, -1);
      }
      return;
    }

    if (mode == S3XY_SCAN_REGISTRY) {
      // Fast path: exact saved address. This avoids an identity probe when the
      // peripheral uses a stable public/static address.
      const int foundSlot = s3xyFindSlotByAddress(addr);
      bool eligible = false;
      if (foundSlot >= 0 && foundSlot < (int)S3XY_MAX_DEVICES) {
        portENTER_CRITICAL(&s3xyMux);
        const S3xyDeviceSlot &d = s3xyDevices[foundSlot];
        eligible = s3xyBluetoothEnabled && s3xyAutoEnabled && d.used && d.autoConnect &&
                   !d.connected && !d.manualPaused;
        portEXIT_CRITICAL(&s3xyMux);
      }
      if (eligible) {
        if (s3xyTarget) { delete s3xyTarget; s3xyTarget = nullptr; }
        s3xyTarget = new BLEAdvertisedDevice(dev);
        portENTER_CRITICAL(&s3xyMux);
        s3xyScanTargetFound = true;
        s3xyScanSlot = (int8_t)foundSlot;
        s3xyDevices[foundSlot].state = S3XY_MAP_FOUND;
        s3xyDevices[foundSlot].rssi = rssi;
        s3xyDevices[foundSlot].addressType = addressType;
        s3xyDevices[foundSlot].addressTypeKnown = true;
        portEXIT_CRITICAL(&s3xyMux);
        String detail = String("wake addr=") + addr + " t=" + String((unsigned)addressType) + " raw-match";
        s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, (int8_t)foundSlot);
        BLEDevice::getScan()->stop();
        return;
      }

      // Recovery path: a bonded/private-address S3XY Button can advertise from
      // an address that is different from the one stored at pairing time. Active
      // registry scanning gives us ENH_BTN / 3D46; encrypted 3D49 is then used as
      // the stable identity before ANY registry address is changed.
      String name(dev.getName().c_str());
      const bool nameMatch = (name == "ENH_BTN");
      bool svcMatch = false;
      if (dev.haveServiceUUID()) svcMatch = dev.isAdvertisingService(BLEUUID(S3XY_SERVICE_UUID));
      if ((nameMatch || svcMatch) && foundSlot < 0) {
        if (s3xyHasEligibleSavedIdentity()) {
          if (!s3xyIdentityCandidateCoolingDown(addr)) {
            if (s3xyTarget) { delete s3xyTarget; s3xyTarget = nullptr; }
            s3xyTarget = new BLEAdvertisedDevice(dev);
            portENTER_CRITICAL(&s3xyMux);
            s3xyScanTargetFound = true;
            s3xyScanSlot = -2; // identity candidate; slot selected by encrypted 3D49
            portEXIT_CRITICAL(&s3xyMux);
            String detail = String("identity candidate addr=") + addr + " t=" + String((unsigned)addressType) +
                            " N" + (nameMatch ? "1" : "0") + " S" + (svcMatch ? "1" : "0");
            s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, -1);
            BLEDevice::getScan()->stop();
            return;
          }
        } else {
          // Legacy registry entries may not have a persisted 3D49 value yet.
          // Never guess between multiple buttons; tell diagnostics exactly why the
          // changed-address candidate was not adopted. One normal READY connection
          // after READY seeds the stable ID for subsequent reboot recovery.
          String detail = String("identity candidate ignored: no saved 3D49 addr=") + addr;
          s3xyLogPush(S3XY_LOG_INFO, detail.c_str(), nullptr, 0, rssi, -1);
        }
      }
      return;
    }

    if (mode == S3XY_SCAN_TARGET && target[0] && s3xyAddressEquals(target, addr)) {
      if (s3xyTarget) { delete s3xyTarget; s3xyTarget = nullptr; }
      s3xyTarget = new BLEAdvertisedDevice(dev);
      portENTER_CRITICAL(&s3xyMux);
      s3xyScanTargetFound = true;
      if (slot >= 0 && slot < (int8_t)S3XY_MAX_DEVICES && s3xyDevices[slot].used) {
        s3xyDevices[slot].state = S3XY_MAP_FOUND;
        s3xyDevices[slot].rssi = rssi;
        s3xyDevices[slot].addressType = addressType;
        s3xyDevices[slot].addressTypeKnown = true;
      }
      portEXIT_CRITICAL(&s3xyMux);
      String detail = String("target addr=") + addr + " t=" + String((unsigned)addressType) + " raw-match";
      s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, slot);
      BLEDevice::getScan()->stop();
      return;
    }

    // Manual Add Button discovery is intentionally selective. Unlike registry
    // reconnect, it uses active scanning and should only show S3XY-like peers.
    if (mode == S3XY_SCAN_DISCOVERY) {
      String name(dev.getName().c_str());
      const bool nameMatch = (name == "ENH_BTN");
      bool svcMatch = false;
      if (dev.haveServiceUUID()) svcMatch = dev.isAdvertisingService(BLEUUID(S3XY_SERVICE_UUID));
      if (!nameMatch && !svcMatch) return;
      s3xyDiscoveryAdd(addr, name.length() ? name.c_str() : "ENH_BTN", rssi);
      String detail = String("discover addr=") + addr + " t=" + String((unsigned)addressType);
      s3xyLogPush(S3XY_LOG_SCAN_HIT, detail.c_str(), nullptr, 0, rssi, -1);
    }
  }
};
static S3xyAdvertisedCallbacks s3xyAdvertisedCallbacks;

static bool s3xyMapperInit() {
  if (!s3xyBluetoothMasterIsEnabled()) return false;
  // Keep compatibility with Arduino-ESP32 BLEDevice versions where init()
  // returns void. Master OFF is implemented as a boot-time no-init state.
  BLEDevice::init("T2CAN_S3XY_MULTI");
  // Compile-time stack name keeps this diagnostic compatible with Arduino-ESP32
  // releases that predate BLEDevice::getBLEStackString().
#if defined(CONFIG_BLUEDROID_ENABLED)
  String stack = "Bluedroid";
#elif defined(CONFIG_NIMBLE_ENABLED)
  String stack = "NimBLE";
#else
  String stack = "UNKNOWN";
#endif
  portENTER_CRITICAL(&s3xyMux);
  strncpy(s3xyBleStackName, stack.c_str(), sizeof(s3xyBleStackName) - 1);
  s3xyBleStackName[sizeof(s3xyBleStackName) - 1] = '\0';
  portEXIT_CRITICAL(&s3xyMux);

  BLESecurity::setCapability(ESP_IO_CAP_NONE);
  BLESecurity::setAuthenticationMode(true, false, true);  // bond + LE Secure Connections, Just Works
  BLESecurity::setKeySize(16);
  BLESecurity::setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  BLESecurity::setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  // A previously bonded S3XY Button may expect encryption immediately
  // after reconnect. Start SMP at link-up; the mapper serializes connections, so
  // the old reason for deferring security until protected 3D49 no longer applies.
  BLESecurity::setForceAuthentication(true);
  BLESecurity::resetSecurity();

  BLEScan *scan = BLEDevice::getScan();
  if (!scan) {
    s3xyLogPush(S3XY_LOG_ERROR, "BLE scan object unavailable");
    return false;
  }
  scan->setAdvertisedDeviceCallbacks(&s3xyAdvertisedCallbacks);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);

  // s3xyBleInitialized is set by the caller immediately after this returns.
  // Temporarily make the bond query legal so boot diagnostics reflect the real store.
  portENTER_CRITICAL(&s3xyMux);
  s3xyBleInitialized = true;
  portEXIT_CRITICAL(&s3xyMux);
  const int bootBonds = s3xyLocalBondCount();
  const uint8_t registryCount = s3xyRegisteredCount();
  portENTER_CRITICAL(&s3xyMux);
  s3xyBootLocalBondCount = bootBonds;
  s3xyBootBondRepairArmed = (registryCount > 0 && bootBonds == 0);
  portEXIT_CRITICAL(&s3xyMux);

  String initDetail = String("BLE mapper initialized stack=") + stack +
                      " bonds=" + String(bootBonds) +
                      " registry=" + String(registryCount);
  s3xyLogPush(S3XY_LOG_INFO, initDetail.c_str());
  if (registryCount > 0 && bootBonds == 0) {
    s3xyLogPush(S3XY_LOG_SECURITY,
                "boot bond store empty with saved registry; automatic re-pair recovery armed");
  }
  return true;
}

static void s3xyClearTarget() {
  if (s3xyTarget) {
    delete s3xyTarget;
    s3xyTarget = nullptr;
  }
}

static void s3xyClearBatchTargets() {
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (s3xyBatchTargets[i]) {
      delete s3xyBatchTargets[i];
      s3xyBatchTargets[i] = nullptr;
    }
  }
  if (s3xyBatchIdentityTarget) {
    delete s3xyBatchIdentityTarget;
    s3xyBatchIdentityTarget = nullptr;
  }
}

static void s3xyResetLinkRuntime(uint8_t slot, uint8_t stateAfter) {
  if (slot >= S3XY_MAX_DEVICES) return;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) {
    s3xyDevices[slot].connected = false;
    s3xyDevices[slot].subscribed = false;
    s3xyDevices[slot].secureOk = false;
    s3xyDevices[slot].notifyChar = nullptr;
    s3xyDevices[slot].idChar = nullptr;
    s3xyDevices[slot].rssi = -127;
    s3xyDevices[slot].state = stateAfter;
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyDisconnectFailedLink(uint8_t slot, uint8_t stateAfter) {
  if (slot >= S3XY_MAX_DEVICES) return;
  BLEClient *client = nullptr;
  portENTER_CRITICAL(&s3xyMux);
  client = s3xyDevices[slot].client;
  portEXIT_CRITICAL(&s3xyMux);
  if (client && client->isConnected()) {
    client->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
  }
  s3xyResetLinkRuntime(slot, stateAfter);
}

static void s3xyRuntimeShutdown() {
  BLEClient *clients[S3XY_MAX_DEVICES + 1] = {};
  uint8_t clientCount = 0;
  BLEClient *lastCreated = nullptr;
  bool initialized = false;

  portENTER_CRITICAL(&s3xyMux);
  s3xyRuntimeStopping = true;
  initialized = s3xyBleInitialized;
  lastCreated = s3xyLastCreatedClient;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    BLEClient *c = s3xyDevices[i].client;
    if (c) {
      bool duplicate = false;
      for (uint8_t n = 0; n < clientCount; n++) if (clients[n] == c) duplicate = true;
      if (!duplicate && clientCount < S3XY_MAX_DEVICES + 1) clients[clientCount++] = c;
    }
  }
  if (s3xyProbeClient) {
    bool duplicate = false;
    for (uint8_t n = 0; n < clientCount; n++) if (clients[n] == s3xyProbeClient) duplicate = true;
    if (!duplicate && clientCount < S3XY_MAX_DEVICES + 1) clients[clientCount++] = s3xyProbeClient;
  }
  portEXIT_CRITICAL(&s3xyMux);

  if (initialized) {
    BLEScan *scan = BLEDevice::getScan();
    if (scan) scan->stop();
  }

  for (uint8_t n = 0; n < clientCount; n++) {
    if (clients[n] && clients[n]->isConnected()) clients[n]->disconnect();
  }
  if (clientCount) vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));

  s3xyClearTarget();
  s3xyClearBatchTargets();

  // BLEDevice::deinit(false) deletes the most recently created BLEClient.
  // Delete older clients while the stack is still alive, then leave the final
  // pointer to BLEDevice so repeated OFF/ON cycles do not leak client objects.
  for (uint8_t n = 0; n < clientCount; n++) {
    if (clients[n] && clients[n] != lastCreated) delete clients[n];
  }

  portENTER_CRITICAL(&s3xyMux);
  s3xyProbeClient = nullptr;
  s3xyLastCreatedClient = nullptr;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    s3xyDevices[i].client = nullptr;
    s3xyDevices[i].connected = false;
    s3xyDevices[i].subscribed = false;
    s3xyDevices[i].secureOk = false;
    s3xyDevices[i].notifyChar = nullptr;
    s3xyDevices[i].idChar = nullptr;
    s3xyDevices[i].rssi = -127;
    s3xyDevices[i].nextAttemptMs = 0;
    if (s3xyDevices[i].used) s3xyDevices[i].state = S3XY_MAP_OFF;
  }
  portEXIT_CRITICAL(&s3xyMux);

  if (initialized) {
    // false is mandatory: releasing BT memory makes same-boot re-enable
    // impossible. Current Arduino-ESP32 deinit(false) tears down host/controller
    // and resets its initialized flag while preserving reinitialization support.
    BLEDevice::deinit(false);
  }

  portENTER_CRITICAL(&s3xyMux);
  s3xyBleInitialized = false;
  s3xyRuntimeStopRequested = false;
  s3xyRuntimeStopping = false;
  s3xyTaskHandle = nullptr;
  portEXIT_CRITICAL(&s3xyMux);
  s3xyLogPush(S3XY_LOG_INFO, "BLE runtime stopped; registry/bonds preserved");
}

static bool s3xyScanTargetMode(const char *address, uint8_t seconds, int8_t slot, bool activeScan) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return false;
  if (!address || !address[0]) return false;
  s3xyClearTarget();

  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_TARGET;
  s3xyScanSlot = slot;
  s3xyScanTargetFound = false;
  strncpy(s3xyScanTargetAddress, address, sizeof(s3xyScanTargetAddress) - 1);
  s3xyScanTargetAddress[sizeof(s3xyScanTargetAddress)-1] = '\0';
  if (slot >= 0 && slot < (int8_t)S3XY_MAX_DEVICES && s3xyDevices[slot].used)
    s3xyDevices[slot].state = S3XY_MAP_SCANNING;
  portEXIT_CRITICAL(&s3xyMux);

  BLEScan *scan = BLEDevice::getScan();
  if (!scan) return false;
  // Exact saved-address reconnect can use passive scan. Discovery/identity recovery
  // stays active so scan-response identity data remains available when needed.
  scan->setActiveScan(activeScan);
  scan->clearResults();
  scan->start(seconds, false);
  scan->clearResults();

  bool found;
  portENTER_CRITICAL(&s3xyMux);
  found = s3xyScanTargetFound;
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyScanSlot = -1;
  s3xyScanTargetAddress[0] = '\0';
  if (!found && slot >= 0 && slot < (int8_t)S3XY_MAX_DEVICES && s3xyDevices[slot].used && !s3xyDevices[slot].connected)
    s3xyDevices[slot].state = S3XY_MAP_IDLE;
  portEXIT_CRITICAL(&s3xyMux);
  return found;
}

static bool s3xyScanTarget(const char *address, uint8_t seconds, int8_t slot) {
  return s3xyScanTargetMode(address, seconds, slot, true);
}

static bool s3xyScanTargetPassive(const char *address, uint8_t seconds, int8_t slot) {
  return s3xyScanTargetMode(address, seconds, slot, false);
}

static bool s3xyScanRegisteredWake(uint8_t seconds, int8_t &foundSlot) {
  foundSlot = -1;
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return false;
  s3xyClearTarget();

  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_REGISTRY;
  s3xyScanSlot = -1;
  s3xyScanTargetFound = false;
  s3xyScanTargetAddress[0] = '\0';
  portEXIT_CRITICAL(&s3xyMux);

  BLEScan *scan = BLEDevice::getScan();
  if (!scan) return false;
  // Registry reconnect is active. Exact-address matching remains the
  // zero-probe fast path, while scan-response data enables safe 3D49 identity
  // recovery when the over-the-air address has changed after reboot.
  scan->setActiveScan(true);
  scan->clearResults();
  scan->start(seconds, false);
  scan->clearResults();

  bool found;
  portENTER_CRITICAL(&s3xyMux);
  found = s3xyScanTargetFound;
  foundSlot = s3xyScanSlot;
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyScanSlot = -1;
  portEXIT_CRITICAL(&s3xyMux);
  // foundSlot == -2 is a valid identity-recovery candidate.
  return found;
}

static uint8_t s3xyScanRegistryBatch(uint8_t seconds) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return 0;
  s3xyClearTarget();
  s3xyClearBatchTargets();
  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_REGISTRY_BATCH;
  s3xyScanSlot = -1;
  s3xyScanTargetFound = false;
  s3xyScanTargetAddress[0] = '\0';
  s3xyBatchExpectedCount = 0;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (s3xyDevices[i].used && s3xyDevices[i].autoConnect && !s3xyDevices[i].connected &&
        !s3xyDevices[i].manualPaused && s3xyDevices[i].nextAttemptMs != 0) {
      s3xyDevices[i].state = S3XY_MAP_SCANNING;
      s3xyBatchExpectedCount++;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);

  BLEScan *scan = BLEDevice::getScan();
  if (!scan) {
    portENTER_CRITICAL(&s3xyMux);
    s3xyScanMode = S3XY_SCAN_NONE;
    s3xyScanSlot = -1;
    s3xyBatchExpectedCount = 0;
    portEXIT_CRITICAL(&s3xyMux);
    s3xyClearBatchTargets();
    return 0;
  }
  scan->setActiveScan(true);
  scan->clearResults();
  scan->start(seconds, false);
  scan->clearResults();
  s3xyLastBleOperationMs = millis();

  uint8_t found = 0;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) if (s3xyBatchTargets[i]) found++;
  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyScanSlot = -1;
  s3xyBatchExpectedCount = 0;
  portEXIT_CRITICAL(&s3xyMux);
  return found;
}

static void s3xyDiscoveryScan() {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) {
    portENTER_CRITICAL(&s3xyMux);
    strncpy(s3xyDiscoveryError, "Bluetooth is OFF", sizeof(s3xyDiscoveryError)-1);
    s3xyDiscoveryError[sizeof(s3xyDiscoveryError)-1] = '\0';
    s3xyDiscoveryScanning = false;
    portEXIT_CRITICAL(&s3xyMux);
    return;
  }
  portENTER_CRITICAL(&s3xyMux);
  s3xyDiscoveredCount = 0;
  memset(s3xyDiscovered, 0, sizeof(s3xyDiscovered));
  s3xyDiscoveryError[0] = '\0';
  s3xyDiscoveryScanning = true;
  s3xyScanMode = S3XY_SCAN_DISCOVERY;
  s3xyScanSlot = -1;
  portEXIT_CRITICAL(&s3xyMux);

  s3xyLogPush(S3XY_LOG_INFO, "discovery scan started");
  BLEScan *scan = BLEDevice::getScan();
  scan->clearResults();
  scan->start(S3XY_DISCOVERY_SCAN_SECONDS, false);
  scan->clearResults();

  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyDiscoveryScanning = false;
  portEXIT_CRITICAL(&s3xyMux);
  s3xyLogPush(S3XY_LOG_INFO, "discovery scan complete");
}

static int s3xyProbeTargetIdentity() {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized || !s3xyTarget) return -1;

  char candidateAddr[24] = {};
  snprintf(candidateAddr, sizeof(candidateAddr), "%s", s3xyTarget->getAddress().toString().c_str());
  const uint8_t candidateType = s3xyTarget->getAddressType();
  const int16_t candidateRssi = (int16_t)s3xyTarget->getRSSI();

  portENTER_CRITICAL(&s3xyMux);
  s3xyIdentityProbeAttempts++;
  portEXIT_CRITICAL(&s3xyMux);

  if (!s3xyProbeClient) {
    s3xyProbeClient = BLEDevice::createClient();
    s3xyLastCreatedClient = s3xyProbeClient;
    if (!s3xyProbeClient) {
      s3xyLogPush(S3XY_LOG_ERROR, "identity probe: createClient failed", nullptr, 0, candidateRssi, -1);
      s3xyRejectIdentityCandidate(candidateAddr);
      s3xyClearTarget();
      return -1;
    }
  }

  if (s3xyProbeClient->isConnected()) {
    s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
  }

  // Reset the Arduino BLE security bookkeeping BEFORE opening the link. With
  // force-authentication enabled, SMP starts at connection establishment.
  BLESecurity::resetSecurity();
  BLESecurity::setForceAuthentication(true);
  if (!s3xyProbeClient->connect(s3xyTarget)) {
    String d = String("identity probe connect failed addr=") + candidateAddr;
    s3xyLogPush(S3XY_LOG_ERROR, d.c_str(), nullptr, 0, candidateRssi, -1);
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }

  const int16_t rssi = (int16_t)s3xyProbeClient->getRssi();
  // Authenticate before service discovery. If the local bond was lost,
  // secureConnection() negotiates a fresh Just-Works SC bond where the peer allows it.
  const bool probeSecure = s3xyProbeClient->secureConnection();
  if (!probeSecure || !s3xyProbeClient->isConnected()) {
    s3xyLogPush(S3XY_LOG_SECURITY, "identity probe: secureConnection failed", nullptr, 0, rssi, -1);
    if (s3xyProbeClient->isConnected()) s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }
  s3xyLogPush(S3XY_LOG_SECURITY, "identity probe: secure link ready before GATT", nullptr, 0, rssi, -1);

  vTaskDelay(pdMS_TO_TICKS(S3XY_GATT_SETTLE_MS));
  BLERemoteService *svc = s3xyProbeClient->getService(BLEUUID(S3XY_SERVICE_UUID));
  if (!svc) {
    s3xyLogPush(S3XY_LOG_ERROR, "identity probe: service 3D46 not found", nullptr, 0, rssi, -1);
    s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }
  BLERemoteCharacteristic *idChar = svc->getCharacteristic(BLEUUID(S3XY_ID_UUID));
  if (!idChar || !idChar->canRead()) {
    s3xyLogPush(S3XY_LOG_ERROR, "identity probe: readable 3D49 not found", nullptr, 0, rssi, -1);
    s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }

  // The link is already encrypted. The registry remains untouched until the
  // returned 3D49 identity matches a saved slot.
  String idv = idChar->readValue();
  if (!s3xyProbeClient->isConnected() || !idv.length()) {
    s3xyLogPush(S3XY_LOG_ERROR, "identity probe: encrypted 3D49 read failed", nullptr, 0, rssi, -1);
    if (s3xyProbeClient->isConnected()) s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }

  uint8_t raw[S3XY_LOG_DATA_MAX] = {};
  const size_t n = min((size_t)idv.length(), (size_t)S3XY_LOG_DATA_MAX);
  for (size_t i = 0; i < n; i++) raw[i] = (uint8_t)idv[i];
  char idHex[64] = {};
  s3xyBytesToHex(raw, n, idHex, sizeof(idHex));
  const int matchedSlot = s3xyFindSlotByPeerId(raw, n);

  if (matchedSlot < 0) {
    portENTER_CRITICAL(&s3xyMux);
    s3xyIdentityProbeMismatches++;
    portEXIT_CRITICAL(&s3xyMux);
    String d = String("identity probe no registry match addr=") + candidateAddr + " id=" + idHex;
    s3xyLogPush(S3XY_LOG_ID_READ, d.c_str(), raw, n, rssi, -1);
    s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    s3xyRejectIdentityCandidate(candidateAddr);
    s3xyClearTarget();
    return -1;
  }

  char oldAddr[24] = {};
  bool eligible = false;
  portENTER_CRITICAL(&s3xyMux);
  S3xyDeviceSlot &d = s3xyDevices[matchedSlot];
  strncpy(oldAddr, d.address, sizeof(oldAddr) - 1);
  oldAddr[sizeof(oldAddr) - 1] = '\0';
  strncpy(d.address, candidateAddr, sizeof(d.address) - 1);
  d.address[sizeof(d.address) - 1] = '\0';
  d.addressType = candidateType;
  d.addressTypeKnown = true;
  d.rssi = rssi;
  eligible = s3xyBluetoothEnabled && s3xyAutoEnabled && d.used && d.autoConnect &&
             !d.connected && !d.manualPaused;
  s3xyIdentityProbeMatches++;
  if (!s3xyAddressEquals(oldAddr, candidateAddr)) s3xyIdentityRebinds++;
  s3xyIdentityRejectedAddress[0] = '\0';
  s3xyIdentityRejectedUntilMs = 0;
  portEXIT_CRITICAL(&s3xyMux);

  // A successful encrypted ID match is authoritative enough to persist even if
  // the following operational reconnect needs another retry.
  s3xyRegistrySaveSlot((uint8_t)matchedSlot);
  String detail = String("identity match slot=") + String(matchedSlot + 1) +
                  " old=" + oldAddr + " new=" + candidateAddr + " id=" + idHex;
  s3xyLogPush(S3XY_LOG_ID_READ, detail.c_str(), raw, n, rssi, (int8_t)matchedSlot);

  s3xyProbeClient->disconnect();
  vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));

  if (!eligible) {
    // Correct device, but its per-device/global auto-connect gate is not eligible.
    s3xyClearTarget();
    return -1;
  }

  // The probe connection itself was deliberately temporary. Do not reuse a stale
  // pre-probe advertisement for the operational connection; the caller performs a
  // short fresh exact-address scan before entering the normal connect pipeline.
  s3xyClearTarget();
  return matchedSlot;
}

static bool s3xyWriteHandshake(uint8_t slot) {
  if (slot >= S3XY_MAX_DEVICES) return false;
  bool connected;
  BLERemoteCharacteristic *idChar;
  int16_t rssi;
  portENTER_CRITICAL(&s3xyMux);
  connected = s3xyDevices[slot].connected;
  idChar = s3xyDevices[slot].idChar;
  rssi = s3xyDevices[slot].rssi;
  portEXIT_CRITICAL(&s3xyMux);
  if (!connected || !idChar) {
    s3xySetSlotError(slot, "handshake: ID characteristic unavailable");
    return false;
  }
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) s3xyDevices[slot].state = S3XY_MAP_HANDSHAKE;
  portEXIT_CRITICAL(&s3xyMux);
  uint8_t b6 = 0xB6;
  bool ok = idChar->writeValue(&b6, 1, true);
  s3xyLogPush(S3XY_LOG_ID_WRITE, ok ? "write B6 ok" : "write B6 failed", &b6, 1, rssi, (int8_t)slot);
  if (!ok) {
    s3xySetSlotError(slot, "B6 handshake write failed");
  } else {
    portENTER_CRITICAL(&s3xyMux);
    if (s3xyDevices[slot].used && s3xyDevices[slot].connected) s3xyDevices[slot].state = S3XY_MAP_READY;
    portEXIT_CRITICAL(&s3xyMux);
  }
  return ok;
}

static bool s3xyEnsureClient(uint8_t slot) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return false;
  if (slot >= S3XY_MAX_DEVICES) return false;
  if (s3xyDevices[slot].client) return true;
  BLEClient *client = BLEDevice::createClient();
  s3xyLastCreatedClient = client;
  if (!client) {
    s3xySetSlotError(slot, "BLEDevice::createClient failed");
    return false;
  }
  if (!s3xyClientCallbacks[slot]) s3xyClientCallbacks[slot] = new S3xyClientCallbacks(slot);
  client->setClientCallbacks(s3xyClientCallbacks[slot]);
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].client = client;
  portEXIT_CRITICAL(&s3xyMux);
  return true;
}

static bool s3xyConnectSlot(uint8_t slot) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return false;
  if (slot >= S3XY_MAX_DEVICES) return false;
  bool used;
  char address[24];
  portENTER_CRITICAL(&s3xyMux);
  used = s3xyDevices[slot].used;
  strncpy(address, s3xyDevices[slot].address, sizeof(address)-1); address[sizeof(address)-1] = '\0';
  s3xyDevices[slot].lastError[0] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
  if (!used || !address[0]) return false;

  if (!s3xyTarget || !s3xyAddressEquals(s3xyTarget->getAddress().toString().c_str(), address)) {
    s3xySetSlotError(slot, "connect: target scan required");
    return false;
  }
  if (!s3xyEnsureClient(slot)) return false;

  BLEClient *client;
  portENTER_CRITICAL(&s3xyMux);
  client = s3xyDevices[slot].client;
  s3xyDevices[slot].state = S3XY_MAP_CONNECTING;
  portEXIT_CRITICAL(&s3xyMux);

  if (client->isConnected()) {
    client->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
  }

  // Reset global security bookkeeping before each serialized peer link.
  // With force authentication enabled, the stack can resume a saved bond or
  // negotiate a new Just-Works bond immediately after connection.
  BLESecurity::resetSecurity();
  BLESecurity::setForceAuthentication(true);
  const uint32_t connectStartedMs = millis();
  const bool connectedNow = client->connect(s3xyTarget);
  const uint32_t connectElapsedMs = millis() - connectStartedMs;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyDevices[slot].used) s3xyDevices[slot].lastConnectMs = connectElapsedMs;
  portEXIT_CRITICAL(&s3xyMux);
  s3xyClearTarget();
  if (!connectedNow) {
    s3xySetSlotError(slot, "connect to ENH_BTN failed");
    s3xyResetLinkRuntime(slot, S3XY_MAP_ERROR);
    return false;
  }

  const int16_t rssi = (int16_t)client->getRssi();
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].rssi = rssi;
  s3xyDevices[slot].state = S3XY_MAP_SECURING;
  portEXIT_CRITICAL(&s3xyMux);

  // Establish encryption BEFORE GATT discovery. Real S3XY buttons are bonded
  // peripherals; after a controller reboot they may reject or hide protected
  // GATT state until SMP has resumed. If our bond store is empty, this becomes
  // an automatic fresh pairing attempt rather than requiring dashboard Forget/Add.
  const bool secureNow = client->secureConnection();
  if (!secureNow || !client->isConnected()) {
    s3xyLogPush(S3XY_LOG_SECURITY, "secureConnection failed before GATT; short repair retry", nullptr, 0, rssi, (int8_t)slot);
    portENTER_CRITICAL(&s3xyMux);
    s3xyBondRepairAttempts++;
    portEXIT_CRITICAL(&s3xyMux);
    if (client->isConnected()) client->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
    // A stale local key can prevent re-pairing. Remove only this peer's local
    // entry; the S3XY registry/action mapping is deliberately preserved.
    s3xyBestEffortRemoveBond(address);
    s3xySetSlotError(slot, "security / bond resume failed");
    s3xyResetLinkRuntime(slot, S3XY_MAP_ERROR);
    return false;
  }
  s3xyLogPush(S3XY_LOG_SECURITY, "secure link ready before GATT", nullptr, 0, rssi, (int8_t)slot);

  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].state = S3XY_MAP_CONNECTED;
  portEXIT_CRITICAL(&s3xyMux);

  vTaskDelay(pdMS_TO_TICKS(S3XY_GATT_SETTLE_MS));
  BLERemoteService *svc = client->getService(BLEUUID(S3XY_SERVICE_UUID));
  if (!svc) {
    s3xySetSlotError(slot, "service 3D46 not found");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }
  s3xyLogPush(S3XY_LOG_GATT, "service 3D46 found", nullptr, 0, rssi, (int8_t)slot);

  std::map<std::string, BLERemoteCharacteristic *> *chars = svc->getCharacteristics();
  if (chars) {
    for (auto &kv : *chars) {
      BLERemoteCharacteristic *c = kv.second;
      if (!c) continue;
      String detail = String("char=") + c->getUUID().toString().c_str() +
                      " h=" + String((unsigned)c->getHandle()) +
                      " R" + (c->canRead() ? "1" : "0") +
                      " W" + (c->canWrite() ? "1" : "0") +
                      " N" + (c->canNotify() ? "1" : "0");
      s3xyLogPush(S3XY_LOG_GATT, detail.c_str(), nullptr, 0, rssi, (int8_t)slot);
    }
  }

  BLERemoteCharacteristic *notifyChar = svc->getCharacteristic(BLEUUID(S3XY_NOTIFY_UUID));
  BLERemoteCharacteristic *idChar = svc->getCharacteristic(BLEUUID(S3XY_ID_UUID));
  if (!notifyChar) {
    s3xySetSlotError(slot, "notify characteristic 3D50 not found");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }
  if (!idChar) {
    s3xySetSlotError(slot, "ID characteristic 3D49 not found");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }

  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].notifyChar = notifyChar;
  s3xyDevices[slot].idChar = idChar;
  portEXIT_CRITICAL(&s3xyMux);

  // The link was explicitly secured before service discovery. 3D49 can now be
  // read without using protected-GATT access as the trigger for pairing.
  String idv;
  if (idChar->canRead()) {
    idv = idChar->readValue();
  }
  if (!client->isConnected()) {
    s3xyLogPush(S3XY_LOG_SECURITY, "security/protected 3D49 access disconnected", nullptr, 0, rssi, (int8_t)slot);
    s3xySetSlotError(slot, "security / bonding disconnected");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }
  s3xyLogPush(S3XY_LOG_SECURITY, "protected 3D49 access completed", nullptr, 0, rssi, (int8_t)slot);

  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].state = S3XY_MAP_SUBSCRIBING;
  portEXIT_CRITICAL(&s3xyMux);

  if (notifyChar->canNotify()) {
    auto notifyCb = s3xyNotifyCallbackForSlotIndex(slot);
    if (!notifyCb) {
      s3xySetSlotError(slot, "notify callback slot unavailable");
      s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
      return false;
    }
    notifyChar->registerForNotify(notifyCb);
    vTaskDelay(pdMS_TO_TICKS(S3XY_GATT_SETTLE_MS));
    portENTER_CRITICAL(&s3xyMux); s3xyDevices[slot].subscribed = true; portEXIT_CRITICAL(&s3xyMux);
    s3xyLogPush(S3XY_LOG_GATT, "subscribed 3D50 notify", nullptr, 0, rssi, (int8_t)slot);
  } else {
    s3xySetSlotError(slot, "3D50 does not advertise NOTIFY");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }

  if (idChar->canRead()) {
    uint8_t raw[S3XY_LOG_DATA_MAX] = {};
    size_t n = min((size_t)idv.length(), (size_t)S3XY_LOG_DATA_MAX);
    for (size_t i = 0; i < n; i++) raw[i] = (uint8_t)idv[i];
    char idHex[64] = {};
    s3xyBytesToHex(raw, n, idHex, sizeof(idHex));
    if (n) {
      const uint8_t peerLen = (uint8_t)min(n, (size_t)S3XY_PEER_ID_MAX);
      portENTER_CRITICAL(&s3xyMux);
      strncpy(s3xyDevices[slot].idHex, idHex, sizeof(s3xyDevices[slot].idHex) - 1);
      s3xyDevices[slot].idHex[sizeof(s3xyDevices[slot].idHex) - 1] = '\0';
      memset(s3xyDevices[slot].peerId, 0, sizeof(s3xyDevices[slot].peerId));
      memcpy(s3xyDevices[slot].peerId, raw, peerLen);
      s3xyDevices[slot].peerIdLen = peerLen;
      s3xyDevices[slot].identityPersistVerified = false;
      portEXIT_CRITICAL(&s3xyMux);
    }
    s3xyLogPush(S3XY_LOG_ID_READ, n ? "encrypted ID read" : "encrypted ID read empty; kept saved ID", raw, n, rssi, (int8_t)slot);
  }

  if (!s3xyWriteHandshake(slot)) {
    s3xySetSlotError(slot, "secure B6 handshake failed");
    s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
    return false;
  }

  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].secureOk = true;
  s3xyDevices[slot].state = S3XY_MAP_READY;
  s3xyDevices[slot].manualPaused = false;
  s3xyDevices[slot].nextAttemptMs = 0;
  s3xyDevices[slot].consecutiveFailures = 0;
  portEXIT_CRITICAL(&s3xyMux);
  // Persist the address type and encrypted 3D49 identity after a complete,
  // authenticated READY transition. Older registry entries are upgraded
  // in place the first time they successfully reconnect.
  s3xyRegistrySaveSlot(slot);
  bool idVerified = false;
  uint8_t savedPeerLen = 0;
  portENTER_CRITICAL(&s3xyMux);
  idVerified = s3xyDevices[slot].identityPersistVerified;
  savedPeerLen = s3xyDevices[slot].peerIdLen;
  portEXIT_CRITICAL(&s3xyMux);
  s3xyLastBleOperationMs = millis();
  s3xyLogPush(S3XY_LOG_INFO,
              savedPeerLen ? (idVerified ? "device READY; 3D49 NVS verified" : "device READY; 3D49 NVS NOT verified")
                           : "device READY; no 3D49 ID available",
              nullptr, 0, rssi, (int8_t)slot);

  // Verify controller-side bond persistence independently from our app registry.
  // If this READY recovered a boot with an empty bond store, user interaction is
  // no longer required even if the underlying core fails to persist the new key.
  const int bondsAfterReady = s3xyLocalBondCount();
  const uint8_t registeredAfterReady = s3xyRegisteredCount();
  const bool bondStoreCoversRegistry = (registeredAfterReady == 0) ||
                                       (bondsAfterReady >= (int)registeredAfterReady);
  bool wasRepairArmed = false;
  portENTER_CRITICAL(&s3xyMux);
  wasRepairArmed = s3xyBootBondRepairArmed;
  if (wasRepairArmed) {
    s3xyBondRepairReady++;
    if (bondStoreCoversRegistry) s3xyBootBondRepairArmed = false;
    else s3xyBondPersistStillMissing++;
  }
  portEXIT_CRITICAL(&s3xyMux);
  if (wasRepairArmed) {
    String d = String("boot bond repair READY; local bonds=") + String(bondsAfterReady) +
               " registry=" + String(registeredAfterReady) +
               (bondStoreCoversRegistry ? " persistent" : " incomplete; recovery remains armed");
    s3xyLogPush(bondStoreCoversRegistry ? S3XY_LOG_SECURITY : S3XY_LOG_ERROR,
                d.c_str(), nullptr, 0, rssi, (int8_t)slot);
  }
  return true;
}

static void s3xyBestEffortRemoteUnpair(uint8_t slot) {
  if (slot >= S3XY_MAX_DEVICES) return;
  bool connected;
  BLERemoteCharacteristic *idChar;
  int16_t rssi;
  portENTER_CRITICAL(&s3xyMux);
  connected = s3xyDevices[slot].connected;
  idChar = s3xyDevices[slot].idChar;
  rssi = s3xyDevices[slot].rssi;
  portEXIT_CRITICAL(&s3xyMux);
  if (!connected || !idChar) {
    s3xyLogPush(S3XY_LOG_INFO, "remote unpair skipped; button offline", nullptr, 0, rssi, (int8_t)slot);
    return;
  }
  uint8_t a1 = 0xA1;
  const bool ok = idChar->writeValue(&a1, 1, true);
  s3xyLogPush(S3XY_LOG_ID_WRITE, ok ? "write A1 unpair ok" : "write A1 unpair failed", &a1, 1, rssi, (int8_t)slot);
  if (ok) vTaskDelay(pdMS_TO_TICKS(100));
}

static void s3xyBestEffortRemoveBond(const char *address) {
  if (!s3xyBleInitialized || !s3xyBluetoothMasterIsEnabled()) return;
  if (!address || !address[0]) return;
#if defined(CONFIG_BLUEDROID_ENABLED)
  unsigned int b[6] = {};
  if (sscanf(address, "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) return;
  esp_bd_addr_t bd = {(uint8_t)b[0], (uint8_t)b[1], (uint8_t)b[2], (uint8_t)b[3], (uint8_t)b[4], (uint8_t)b[5]};
  esp_err_t err = esp_ble_remove_bond_device(bd);
  String d = String("remove bond bluedroid: ") + esp_err_to_name(err);
  s3xyLogPush(S3XY_LOG_INFO, d.c_str());
#elif defined(CONFIG_NIMBLE_ENABLED)
  ble_addr_t peers[8] = {};
  int n = 0;
  const int listRc = ble_store_util_bonded_peers(peers, &n, 8);
  bool removed = false;
  if (listRc == 0) {
    for (int i = 0; i < n; i++) {
      BLEAddress pa(peers[i]);
      if (!s3xyAddressEquals(pa.toString().c_str(), address)) continue;
      const int rc = ble_gap_unpair(&peers[i]);
      String d = String("remove bond nimble rc=") + String(rc);
      s3xyLogPush(rc == 0 ? S3XY_LOG_INFO : S3XY_LOG_ERROR, d.c_str());
      removed = (rc == 0);
      break;
    }
  }
  if (!removed) {
    s3xyLogPush(S3XY_LOG_INFO, "remove bond nimble: peer not in local store");
  }
#else
  (void)address;
#endif
}

static bool s3xyPairAddress(const char *address) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) {
    strncpy(s3xyDiscoveryError, "Bluetooth is OFF", sizeof(s3xyDiscoveryError)-1);
    s3xyDiscoveryError[sizeof(s3xyDiscoveryError)-1] = '\0';
    return false;
  }
  if (!address || !address[0]) return false;
  if (s3xyFindSlotByAddress(address) >= 0) {
    strncpy(s3xyDiscoveryError, "device already registered", sizeof(s3xyDiscoveryError)-1);
    s3xyDiscoveryError[sizeof(s3xyDiscoveryError)-1] = '\0';
    return false;
  }
  const int slot = s3xyFindFreeSlot();
  if (slot < 0) {
    strncpy(s3xyDiscoveryError, "registry full (max 3)", sizeof(s3xyDiscoveryError)-1);
    s3xyDiscoveryError[sizeof(s3xyDiscoveryError)-1] = '\0';
    return false;
  }

  char defName[28]; s3xyDefaultName((uint8_t)slot, defName, sizeof(defName));
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].used = true; // provisional until READY
  strncpy(s3xyDevices[slot].address, address, sizeof(s3xyDevices[slot].address)-1);
  s3xyDevices[slot].address[sizeof(s3xyDevices[slot].address)-1] = '\0';
  strncpy(s3xyDevices[slot].name, defName, sizeof(s3xyDevices[slot].name)-1);
  s3xyDevices[slot].name[sizeof(s3xyDevices[slot].name)-1] = '\0';
  s3xyDevices[slot].addressType = 0;
  s3xyDevices[slot].addressTypeKnown = false;
  s3xyDevices[slot].idHex[0] = '\0';
  memset(s3xyDevices[slot].peerId, 0, sizeof(s3xyDevices[slot].peerId));
  s3xyDevices[slot].peerIdLen = 0;
  s3xyDevices[slot].identityPersistVerified = false;
  s3xyDevices[slot].singleAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].doubleAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].longAction = S3XY_ACTION_NONE;
  s3xyDevices[slot].autoConnect = true;
  s3xyDevices[slot].state = S3XY_MAP_SCANNING;
  s3xyDevices[slot].manualPaused = false;
  portEXIT_CRITICAL(&s3xyMux);

  const bool found = s3xyScanTarget(address, S3XY_AUTO_SCAN_SECONDS + 2, (int8_t)slot);
  const bool ready = found && s3xyConnectSlot((uint8_t)slot);
  if (!ready) {
    BLEClient *client = s3xyDevices[slot].client;
    if (client && client->isConnected()) client->disconnect();
    char err[64];
    portENTER_CRITICAL(&s3xyMux);
    strncpy(err, s3xyDevices[slot].lastError, sizeof(err)-1); err[sizeof(err)-1] = '\0';
    portEXIT_CRITICAL(&s3xyMux);
    s3xyBestEffortRemoveBond(address);
    s3xyRegistryClearSlotRuntime((uint8_t)slot, true);
    strncpy(s3xyDiscoveryError, err[0] ? err : "pair failed", sizeof(s3xyDiscoveryError)-1);
    s3xyDiscoveryError[sizeof(s3xyDiscoveryError)-1] = '\0';
    return false;
  }

  s3xyRegistrySaveSlot((uint8_t)slot);
  s3xyDiscoveryError[0] = '\0';
  s3xyLogPush(S3XY_LOG_INFO, "pair complete; registry saved", nullptr, 0, -127, (int8_t)slot);
  return true;
}

static void s3xyForgetSlot(uint8_t slot) {
  if (slot >= S3XY_MAX_DEVICES) return;
  char address[24];
  BLEClient *client;
  portENTER_CRITICAL(&s3xyMux);
  if (!s3xyDevices[slot].used) { portEXIT_CRITICAL(&s3xyMux); return; }
  s3xyDevices[slot].manualPaused = true;
  strncpy(address, s3xyDevices[slot].address, sizeof(address)-1); address[sizeof(address)-1] = '\0';
  client = s3xyDevices[slot].client;
  portEXIT_CRITICAL(&s3xyMux);

  // Tell the button to clear its peer first when the encrypted ID channel is still available.
  // The real Commander protocol uses 0xA1 on 3D49 for unpair/disconnect.
  s3xyBestEffortRemoteUnpair(slot);
  if (client && client->isConnected()) {
    client->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
  }
  s3xyBestEffortRemoveBond(address);
  s3xyRegistryClearSlotRuntime(slot, true);
  s3xyRegistrySaveSlot(slot);
  s3xyLogPush(S3XY_LOG_INFO, "device forgotten; registry entry removed", nullptr, 0, -127, (int8_t)slot);
}


static uint16_t s3xyRemoveAllLocalBonds() {
  uint16_t removed = 0;
  if (!s3xyBleInitialized || !s3xyBluetoothMasterIsEnabled()) return 0;
#if defined(CONFIG_BLUEDROID_ENABLED)
  int devNum = esp_ble_get_bond_device_num();
  if (devNum <= 0) return 0;
  esp_ble_bond_dev_t *list = (esp_ble_bond_dev_t *)calloc((size_t)devNum, sizeof(esp_ble_bond_dev_t));
  if (!list) {
    s3xyLogPush(S3XY_LOG_ERROR, "reset all: bond list allocation failed");
    return 0;
  }
  int count = devNum;
  esp_err_t listErr = esp_ble_get_bond_device_list(&count, list);
  if (listErr == ESP_OK) {
    for (int i = 0; i < count; i++) {
      if (esp_ble_remove_bond_device(list[i].bd_addr) == ESP_OK) removed++;
    }
  } else {
    String d = String("reset all: bond list failed: ") + esp_err_to_name(listErr);
    s3xyLogPush(S3XY_LOG_ERROR, d.c_str());
  }
  free(list);
#elif defined(CONFIG_NIMBLE_ENABLED)
  const int before = s3xyLocalBondCount();
  const int rc = ble_store_clear();
  if (rc == 0 && before > 0) removed = (uint16_t)before;
  if (rc != 0) {
    String d = String("reset all: nimble ble_store_clear rc=") + String(rc);
    s3xyLogPush(S3XY_LOG_ERROR, d.c_str());
  }
#endif
  return removed;
}

static void s3xyResetAllBluetoothData() {
  // This operation intentionally preserves the Bluetooth Master and global
  // Auto Connect preferences. Everything tied to paired S3XY peers is wiped.
  char addresses[S3XY_MAX_DEVICES][24] = {};
  BLEClient *clients[S3XY_MAX_DEVICES] = {};
  bool used[S3XY_MAX_DEVICES] = {};

  portENTER_CRITICAL(&s3xyMux);
  s3xyScanMode = S3XY_SCAN_NONE;
  s3xyDiscoveryScanning = false;
  s3xyScanTargetFound = false;
  s3xyScanSlot = -1;
  s3xyScanTargetAddress[0] = '\0';
  s3xyActionPending = 0; s3xyAccelActionPending = 0; s3xyResearchCaptureAPending = 0; s3xyResearchCaptureBPending = 0; s3xyResearchCaptureCPending = 0; s3xyResearchCaptureDPending = 0; s3xyResearchCaptureResetPending = 0;
  s3xyIdentityProbeAttempts = 0;
  s3xyIdentityProbeMatches = 0;
  s3xyIdentityProbeMismatches = 0;
  s3xyIdentityRebinds = 0;
  s3xyAutoExactHits = 0;
  s3xyAutoExactMisses = 0;
  s3xyAutoLinkFailures = 0;
  s3xyBootLocalBondCount = -1;
  s3xyBootBondRepairArmed = false;
  s3xyBondRepairAttempts = 0;
  s3xyBondRepairReady = 0;
  s3xyBondPersistStillMissing = 0;
  s3xyIdentityRejectedAddress[0] = '\0';
  s3xyIdentityRejectedUntilMs = 0;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    used[i] = s3xyDevices[i].used;
    if (used[i]) {
      s3xyDevices[i].manualPaused = true;
      s3xyDevices[i].nextAttemptMs = 0;
      strncpy(addresses[i], s3xyDevices[i].address, sizeof(addresses[i]) - 1);
      addresses[i][sizeof(addresses[i]) - 1] = '\0';
      clients[i] = s3xyDevices[i].client;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);

  if (s3xyProbeClient && s3xyProbeClient->isConnected()) {
    s3xyProbeClient->disconnect();
    vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
  }
  s3xyClearTarget();

  // Clear the peer side when possible before tearing down the encrypted link.
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (used[i]) s3xyBestEffortRemoteUnpair(i);
  }
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (clients[i] && clients[i]->isConnected()) clients[i]->disconnect();
  }
  vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));

  // Remove both registry-known bonds and any stale orphan bonds left by old
  // pairing attempts. The all-bonds pass is available on Bluedroid builds.
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (addresses[i][0]) s3xyBestEffortRemoveBond(addresses[i]);
  }
  const uint16_t removedBonds = s3xyRemoveAllLocalBonds();

  Preferences reg;
  if (reg.begin("s3xyreg", false)) { reg.clear(); reg.end(); }
  Preferences legacy;
  if (legacy.begin("s3xy", false)) {
    // Preserve bt + auto; only obsolete/saved-peer compatibility data is cache.
    legacy.remove("addr");
    legacy.end();
  }

  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) s3xyRegistryClearSlotRuntime(i, false);
  portENTER_CRITICAL(&s3xyMux);
  memset(s3xyDiscovered, 0, sizeof(s3xyDiscovered));
  s3xyDiscoveredCount = 0;
  s3xyDiscoveryScanning = false;
  s3xyDiscoveryError[0] = '\0';
#if S3XY_DIAGNOSTICS_ENABLED
  s3xyLogHead = 0;
  s3xyLogCount = 0;
  s3xyLogDropped = 0;
#endif
  portEXIT_CRITICAL(&s3xyMux);

  Serial.printf("S3XY: Bluetooth data reset complete; removed bonds=%u; rebooting\n", (unsigned)removedBonds);
  vTaskDelay(pdMS_TO_TICKS(500));
  ESP.restart();
}

static bool s3xyQueueCommand(uint8_t type, int8_t slot = -1, const char *address = nullptr) {
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyCommand.type != S3XY_CMD_NONE) {
    portEXIT_CRITICAL(&s3xyMux);
    return false;
  }
  s3xyCommand.type = type;
  s3xyCommand.slot = slot;
  s3xyCommand.address[0] = '\0';
  if (address) {
    strncpy(s3xyCommand.address, address, sizeof(s3xyCommand.address) - 1);
    s3xyCommand.address[sizeof(s3xyCommand.address)-1] = '\0';
  }
  portEXIT_CRITICAL(&s3xyMux);
  return true;
}

static bool s3xyTakeCommand(S3xyCommandRequest &out) {
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyCommand.type == S3XY_CMD_NONE) {
    portEXIT_CRITICAL(&s3xyMux);
    return false;
  }
  out.type = s3xyCommand.type;
  out.slot = s3xyCommand.slot;
  strncpy(out.address, s3xyCommand.address, sizeof(out.address)-1);
  out.address[sizeof(out.address)-1] = '\0';
  s3xyCommand.type = S3XY_CMD_NONE;
  s3xyCommand.slot = -1;
  s3xyCommand.address[0] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
  return true;
}

static uint8_t s3xyCountPendingAuto() {
  uint8_t count = 0;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyBluetoothEnabled && s3xyAutoEnabled && s3xyBleInitialized) {
    for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
      const S3xyDeviceSlot &d = s3xyDevices[i];
      if (d.used && d.autoConnect && !d.connected && !d.manualPaused && d.nextAttemptMs != 0) count++;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  return count;
}

static void s3xyApplyBatchAutoResult(uint8_t slot, bool ready) {
  if (slot >= S3XY_MAX_DEVICES) return;
  portENTER_CRITICAL(&s3xyMux);
  if (ready) {
    s3xyDevices[slot].autoReconnects++;
    s3xyDevices[slot].consecutiveFailures = 0;
    s3xyDevices[slot].nextAttemptMs = 0;
  } else if (s3xyBluetoothEnabled && s3xyDevices[slot].used && s3xyDevices[slot].autoConnect &&
             !s3xyDevices[slot].manualPaused) {
    s3xyDevices[slot].state = S3XY_MAP_RETRY;
    s3xyDevices[slot].autoFailures++;
    s3xyDevices[slot].consecutiveFailures++;
    s3xyAutoLinkFailures++;
    const bool repairingBootBond = s3xyBootBondRepairArmed;
    const uint32_t backoff = repairingBootBond ? S3XY_AUTO_DISCONNECT_RETRY_MS :
                             ((s3xyDevices[slot].consecutiveFailures <= 2) ?
                              S3XY_AUTO_RETRY_SHORT_MS : S3XY_AUTO_RETRY_LONG_MS);
    s3xyDevices[slot].nextAttemptMs = millis() + backoff;
  }
  portEXIT_CRITICAL(&s3xyMux);
}

static void s3xyRunBatchAutoReconnect() {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return;
  if (s3xyCountPendingAuto() < 2) return;
  bool pendingAtStart[S3XY_MAX_DEVICES] = {};
  bool exactSeen[S3XY_MAX_DEVICES] = {};
  bool attempted[S3XY_MAX_DEVICES] = {};

  // One active scan sees all advertising saved Buttons instead of paying the
  // four-second exact-scan timeout once per slot. Link/security/GATT remains
  // deliberately serialized to avoid shared BLE security-state races.
  const uint32_t batchStartedMs = millis();
  portENTER_CRITICAL(&s3xyMux);
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    S3xyDeviceSlot &d = s3xyDevices[i];
    if (d.used && d.autoConnect && !d.connected && !d.manualPaused && d.nextAttemptMs != 0) {
      pendingAtStart[i] = true;
#if S3XY_DIAGNOSTICS_ENABLED
      d.autoAttempts++;
      d.autoAttemptStartMs = batchStartedMs;
      d.lastDiscoverMs = 0;
      d.lastConnectMs = 0;
      d.lastReadyMs = 0;
      strncpy(d.lastAutoPath, "BATCH_EXACT", sizeof(d.lastAutoPath) - 1);
      d.lastAutoPath[sizeof(d.lastAutoPath) - 1] = '\0';
#endif
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  s3xySetAutoTrace("batch registry scan");
  const uint8_t exactFound = s3xyScanRegistryBatch(S3XY_AUTO_BATCH_SCAN_SECONDS);
  const uint32_t batchDiscoverElapsedMs = millis() - batchStartedMs;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) exactSeen[i] = s3xyBatchTargets[i] != nullptr;
  s3xyLogPush(S3XY_LOG_INFO, (String("batch registry scan exact=") + String((unsigned)exactFound)).c_str());

  for (uint8_t slot = 0; slot < S3XY_MAX_DEVICES; slot++) {
    if (!s3xyBatchTargets[slot]) continue;
    attempted[slot] = true;
    const uint32_t sinceLast = millis() - s3xyLastBleOperationMs;
    if (sinceLast < S3XY_BLE_OPERATION_GAP_MS)
      vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - sinceLast));

    s3xyClearTarget();
    s3xyTarget = s3xyBatchTargets[slot];
    s3xyBatchTargets[slot] = nullptr; // ownership transferred to s3xyTarget
    portENTER_CRITICAL(&s3xyMux);
    s3xyAutoExactHits++;
    const bool eligible = s3xyDevices[slot].used && s3xyDevices[slot].autoConnect &&
                          !s3xyDevices[slot].connected && !s3xyDevices[slot].manualPaused &&
                          s3xyBluetoothEnabled && s3xyAutoEnabled;
    portEXIT_CRITICAL(&s3xyMux);
    if (!eligible) { s3xyClearTarget(); continue; }

    s3xyAutoTimingDiscover(slot, batchDiscoverElapsedMs);
    s3xyAutoTimingPath(slot, "BATCH_EXACT");
    s3xySetAutoTrace(String("batch slot ") + String(slot + 1) + " HIT -> connect");
    const bool ready = s3xyConnectSlot(slot);
    if (ready) s3xyAutoTimingReady(slot);
    if (!ready) s3xyDisconnectFailedLink(slot, S3XY_MAP_RETRY);
    s3xyLastBleOperationMs = millis();
    s3xyApplyBatchAutoResult(slot, ready);
    s3xyLogPush(S3XY_LOG_INFO, ready ? "batch auto reconnect READY" : "batch auto reconnect link retry scheduled",
                nullptr, 0, -127, (int8_t)slot);
  }

  // If the same active scan saw a S3XY peer at a changed address, use the
  // protected 3D49 identity once instead of paying another per-slot 4 s miss.
  if (s3xyBatchIdentityTarget && s3xyCountPendingAuto() > 0) {
    const uint32_t sinceLast = millis() - s3xyLastBleOperationMs;
    if (sinceLast < S3XY_BLE_OPERATION_GAP_MS)
      vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - sinceLast));
    s3xyClearTarget();
    s3xyTarget = s3xyBatchIdentityTarget;
    s3xyBatchIdentityTarget = nullptr;
    const int matchedSlot = s3xyProbeTargetIdentity();
    s3xyLastBleOperationMs = millis();
    if (matchedSlot >= 0 && matchedSlot < (int)S3XY_MAX_DEVICES) {
      attempted[matchedSlot] = true;
      char reboundAddr[24] = {};
      portENTER_CRITICAL(&s3xyMux);
      if (s3xyDevices[matchedSlot].used) {
        strncpy(reboundAddr, s3xyDevices[matchedSlot].address, sizeof(reboundAddr) - 1);
        reboundAddr[sizeof(reboundAddr) - 1] = '\0';
      }
      portEXIT_CRITICAL(&s3xyMux);
      if (reboundAddr[0]) {
        const uint32_t gap = millis() - s3xyLastBleOperationMs;
        if (gap < S3XY_BLE_OPERATION_GAP_MS)
          vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - gap));
        const uint32_t reboundScanStartedMs = millis();
        const bool found = s3xyScanTarget(reboundAddr, S3XY_AUTO_WAKE_SCAN_SECONDS, (int8_t)matchedSlot);
        s3xyLastBleOperationMs = millis();
        if (found) {
          s3xyAutoTimingDiscover((uint8_t)matchedSlot, millis() - reboundScanStartedMs);
          s3xyAutoTimingPath((uint8_t)matchedSlot, "BATCH_3D49");
          const bool ready = s3xyConnectSlot((uint8_t)matchedSlot);
          if (ready) s3xyAutoTimingReady((uint8_t)matchedSlot);
          if (!ready) s3xyDisconnectFailedLink((uint8_t)matchedSlot, S3XY_MAP_RETRY);
          s3xyLastBleOperationMs = millis();
          s3xyApplyBatchAutoResult((uint8_t)matchedSlot, ready);
          s3xySetAutoTrace(String("batch slot ") + String(matchedSlot + 1) + (ready ? " 3D49 READY" : " 3D49 link FAIL"));
        }
      }
    }
  }

  // Count exact misses once for peers that were pending when the batch began.
  // Peers not attempted then enter the proven per-slot exact/identity path on
  // the next loop; link failures keep their normal security/backoff schedule.
  const uint32_t retryAt = millis() + S3XY_AUTO_WAKE_SCAN_GAP_MS;
  portENTER_CRITICAL(&s3xyMux);
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (pendingAtStart[i] && !exactSeen[i]) s3xyAutoExactMisses++;
    S3xyDeviceSlot &d = s3xyDevices[i];
    if (!attempted[i] && d.used && d.autoConnect && !d.connected && !d.manualPaused && d.nextAttemptMs != 0) {
      d.state = S3XY_MAP_IDLE;
      d.nextAttemptMs = retryAt;
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  s3xyClearBatchTargets();
}

static int s3xyFindDueAutoSlot(uint32_t now) {
  int chosen = -1;
  portENTER_CRITICAL(&s3xyMux);
  if (s3xyBluetoothEnabled && s3xyAutoEnabled && s3xyBleInitialized) {
    // Exact scans are per-slot and block for up to four seconds, so a
    // fixed 0..N scan order would let an offline Button 01 starve Button 02/03.
    // Round-robin preserves deterministic per-device reconnect without starvation.
    const uint8_t start = s3xyAutoRoundRobinCursor % S3XY_MAX_DEVICES;
    for (uint8_t off = 0; off < S3XY_MAX_DEVICES; off++) {
      const uint8_t i = (uint8_t)((start + off) % S3XY_MAX_DEVICES);
      S3xyDeviceSlot &d = s3xyDevices[i];
      if (!d.used || !d.autoConnect || d.connected || d.manualPaused || d.nextAttemptMs == 0) continue;
      if ((int32_t)(now - d.nextAttemptMs) >= 0) {
        chosen = (int)i;
        s3xyAutoRoundRobinCursor = (uint8_t)((i + 1) % S3XY_MAX_DEVICES);
        break;
      }
    }
  }
  portEXIT_CRITICAL(&s3xyMux);
  return chosen;
}

static void s3xyRunAutoAttempt(uint8_t triggerSlot) {
  if (!s3xyBluetoothMasterIsEnabled() || !s3xyBleInitialized) return;
  if (triggerSlot >= S3XY_MAX_DEVICES) return;

  char address[24] = {};
  bool eligible = false;
  bool hasStableIdentity = false;
  portENTER_CRITICAL(&s3xyMux);
  S3xyDeviceSlot &d0 = s3xyDevices[triggerSlot];
  eligible = s3xyBluetoothEnabled && s3xyAutoEnabled && d0.used && d0.autoConnect &&
             !d0.connected && !d0.manualPaused && d0.address[0];
  if (eligible) {
    strncpy(address, d0.address, sizeof(address) - 1);
    address[sizeof(address) - 1] = '\0';
    hasStableIdentity = (d0.peerIdLen > 0);
#if S3XY_DIAGNOSTICS_ENABLED
    d0.autoAttempts++;
#endif
  }
  portEXIT_CRITICAL(&s3xyMux);
  if (!eligible || !address[0]) return;
  s3xyAutoTimingBegin(triggerSlot, "EXACT");

  // Reconnect policy:
  //   1) known-good Manual Connect path: exact saved address
  //   2) if the address is stale after reboot, ACTIVE registry scan
  //   3) encrypted 3D49 stable-ID match -> safe address/type rebind
  //   4) fresh exact scan -> normal secure connect/notify/B6 pipeline
  const uint32_t sinceLast = millis() - s3xyLastBleOperationMs;
  if (sinceLast < S3XY_BLE_OPERATION_GAP_MS)
    vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - sinceLast));

#if S3XY_DIAGNOSTICS_ENABLED
  String trace = String("slot ") + String(triggerSlot + 1) + " exact scan " + address;
  s3xySetAutoTrace(trace);
#endif
  s3xyLogPush(S3XY_LOG_INFO, (String("auto exact scan addr=") + address).c_str(), nullptr, 0, -127, (int8_t)triggerSlot);

  const uint32_t exactScanStartedMs = millis();
  bool found = s3xyScanTargetPassive(address, S3XY_AUTO_SCAN_SECONDS, (int8_t)triggerSlot);
  s3xyLastBleOperationMs = millis();
  if (found) s3xyAutoTimingDiscover(triggerSlot, millis() - exactScanStartedMs);
  uint8_t connectSlot = triggerSlot;

  if (found) {
    portENTER_CRITICAL(&s3xyMux);
    s3xyAutoExactHits++;
    portEXIT_CRITICAL(&s3xyMux);
    s3xySetAutoTrace(String("slot ") + String(triggerSlot + 1) + " exact HIT -> connect");
  } else {
    portENTER_CRITICAL(&s3xyMux);
    s3xyAutoExactMisses++;
    portEXIT_CRITICAL(&s3xyMux);
    s3xyAutoTimingPath(triggerSlot, "IDENTITY_RECOVERY");
    s3xySetAutoTrace(String("slot ") + String(triggerSlot + 1) + " exact MISS -> identity recovery");
    s3xyLogPush(S3XY_LOG_INFO, hasStableIdentity ?
                "auto exact miss; trying registry/3D49 recovery" :
                "auto exact miss; no saved 3D49 identity", nullptr, 0, -127, (int8_t)triggerSlot);

    if (!hasStableIdentity) {
      portENTER_CRITICAL(&s3xyMux);
      if (s3xyDevices[triggerSlot].used && s3xyDevices[triggerSlot].autoConnect &&
          !s3xyDevices[triggerSlot].connected && !s3xyDevices[triggerSlot].manualPaused) {
        s3xyDevices[triggerSlot].state = S3XY_MAP_IDLE;
        s3xyDevices[triggerSlot].nextAttemptMs = millis() + S3XY_AUTO_WAKE_SCAN_GAP_MS;
      }
      portEXIT_CRITICAL(&s3xyMux);
      return;
    }

    const uint32_t gapAfterExact = millis() - s3xyLastBleOperationMs;
    if (gapAfterExact < S3XY_BLE_OPERATION_GAP_MS)
      vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - gapAfterExact));

    int8_t wakeSlot = -1;
    const uint32_t registryScanStartedMs = millis();
    const bool wakeFound = s3xyScanRegisteredWake(S3XY_AUTO_WAKE_SCAN_SECONDS, wakeSlot);
    const uint32_t registryScanElapsedMs = millis() - registryScanStartedMs;
    s3xyLastBleOperationMs = millis();

    if (!wakeFound) {
      portENTER_CRITICAL(&s3xyMux);
      if (s3xyDevices[triggerSlot].used && s3xyDevices[triggerSlot].autoConnect &&
          !s3xyDevices[triggerSlot].connected && !s3xyDevices[triggerSlot].manualPaused) {
        s3xyDevices[triggerSlot].state = S3XY_MAP_IDLE;
        s3xyDevices[triggerSlot].nextAttemptMs = millis() + S3XY_AUTO_WAKE_SCAN_GAP_MS;
      }
      portEXIT_CRITICAL(&s3xyMux);
      s3xySetAutoTrace(String("slot ") + String(triggerSlot + 1) + " registry wake MISS");
      s3xyLogPush(S3XY_LOG_INFO, "auto registry wake miss; short retry", nullptr, 0, -127, (int8_t)triggerSlot);
      return;
    }

    if (wakeSlot == -2) {
      // Changed-address candidate. The registry callback intentionally does not
      // choose a slot until protected 3D49 proves which saved button this is.
      const int matchedSlot = s3xyProbeTargetIdentity();
      s3xyLastBleOperationMs = millis();
      if (matchedSlot < 0 || matchedSlot >= (int)S3XY_MAX_DEVICES) {
        portENTER_CRITICAL(&s3xyMux);
        if (s3xyDevices[triggerSlot].used && s3xyDevices[triggerSlot].autoConnect &&
            !s3xyDevices[triggerSlot].connected && !s3xyDevices[triggerSlot].manualPaused) {
          s3xyDevices[triggerSlot].state = S3XY_MAP_IDLE;
          s3xyDevices[triggerSlot].nextAttemptMs = millis() + S3XY_AUTO_RETRY_SHORT_MS;
        }
        portEXIT_CRITICAL(&s3xyMux);
        s3xySetAutoTrace(String("slot ") + String(triggerSlot + 1) + " 3D49 no match/retry");
        return;
      }

      connectSlot = (uint8_t)matchedSlot;
      char reboundAddr[24] = {};
      portENTER_CRITICAL(&s3xyMux);
      if (s3xyDevices[connectSlot].used) {
        strncpy(reboundAddr, s3xyDevices[connectSlot].address, sizeof(reboundAddr) - 1);
        reboundAddr[sizeof(reboundAddr) - 1] = '\0';
      }
      portEXIT_CRITICAL(&s3xyMux);
      if (!reboundAddr[0]) return;

      const uint32_t gapAfterProbe = millis() - s3xyLastBleOperationMs;
      if (gapAfterProbe < S3XY_BLE_OPERATION_GAP_MS)
        vTaskDelay(pdMS_TO_TICKS(S3XY_BLE_OPERATION_GAP_MS - gapAfterProbe));

      s3xySetAutoTrace(String("slot ") + String(connectSlot + 1) + " 3D49 MATCH -> fresh scan");
      s3xyAutoTimingAdopt(triggerSlot, connectSlot, "3D49_REBIND");
      const uint32_t reboundScanStartedMs = millis();
      found = s3xyScanTarget(reboundAddr, S3XY_AUTO_WAKE_SCAN_SECONDS + 1, (int8_t)connectSlot);
      s3xyLastBleOperationMs = millis();
      if (found) s3xyAutoTimingDiscover(connectSlot, millis() - reboundScanStartedMs);
      if (!found) {
        portENTER_CRITICAL(&s3xyMux);
        if (s3xyDevices[connectSlot].used && s3xyDevices[connectSlot].autoConnect &&
            !s3xyDevices[connectSlot].connected && !s3xyDevices[connectSlot].manualPaused) {
          s3xyDevices[connectSlot].state = S3XY_MAP_IDLE;
          s3xyDevices[connectSlot].nextAttemptMs = millis() + S3XY_AUTO_WAKE_SCAN_GAP_MS;
        }
        portEXIT_CRITICAL(&s3xyMux);
        s3xySetAutoTrace(String("slot ") + String(connectSlot + 1) + " rebound fresh scan MISS");
        return;
      }
    } else if (wakeSlot >= 0 && wakeSlot < (int8_t)S3XY_MAX_DEVICES) {
      // Registry scan found a saved address directly. s3xyTarget already holds
      // the fresh advertisement, so it can enter the normal connect pipeline.
      connectSlot = (uint8_t)wakeSlot;
      s3xyAutoTimingAdopt(triggerSlot, connectSlot, "REGISTRY_EXACT");
      s3xyAutoTimingDiscover(connectSlot, registryScanElapsedMs);
      s3xySetAutoTrace(String("slot ") + String(connectSlot + 1) + " registry exact HIT -> connect");
    } else {
      s3xyClearTarget();
      return;
    }
  }

  // Guard against settings/state changing while the blocking scan/probe ran.
  portENTER_CRITICAL(&s3xyMux);
  eligible = connectSlot < S3XY_MAX_DEVICES && s3xyDevices[connectSlot].used &&
             s3xyDevices[connectSlot].autoConnect && !s3xyDevices[connectSlot].connected &&
             !s3xyDevices[connectSlot].manualPaused && s3xyBluetoothEnabled && s3xyAutoEnabled;
  portEXIT_CRITICAL(&s3xyMux);
  if (!eligible) {
    s3xyClearTarget();
    return;
  }

  const bool ready = s3xyConnectSlot(connectSlot);
  if (ready) s3xyAutoTimingReady(connectSlot);
  if (!ready) s3xyDisconnectFailedLink(connectSlot, S3XY_MAP_RETRY);
  s3xyLastBleOperationMs = millis();

  portENTER_CRITICAL(&s3xyMux);
  if (ready) {
    s3xyDevices[connectSlot].autoReconnects++;
    s3xyDevices[connectSlot].consecutiveFailures = 0;
    s3xyDevices[connectSlot].nextAttemptMs = 0;
    // Give every other disconnected auto-connect slot a deterministic turn.
    for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
      if (i == connectSlot) continue;
      S3xyDeviceSlot &d = s3xyDevices[i];
      if (d.used && d.autoConnect && !d.connected && !d.manualPaused && d.nextAttemptMs == 0)
        d.nextAttemptMs = millis() + S3XY_BLE_OPERATION_GAP_MS + (uint32_t)i * 100U;
    }
  } else if (s3xyBluetoothEnabled && s3xyDevices[connectSlot].used &&
             s3xyDevices[connectSlot].autoConnect && !s3xyDevices[connectSlot].manualPaused) {
    s3xyDevices[connectSlot].state = S3XY_MAP_RETRY;
    s3xyDevices[connectSlot].autoFailures++;
    s3xyDevices[connectSlot].consecutiveFailures++;
    s3xyAutoLinkFailures++;
    // When the application registry survived reboot but the controller bond
    // store did not, do not wait 5/15 seconds between repair attempts. The
    // button can still be advertising with the same address while expecting a
    // fresh SMP exchange. Keep the registry/mappings intact and retry the
    // secure-link pipeline quickly until READY proves the relationship again.
    const bool repairingBootBond = s3xyBootBondRepairArmed;
    const uint32_t backoff = repairingBootBond ?
                             S3XY_AUTO_DISCONNECT_RETRY_MS :
                             ((s3xyDevices[connectSlot].consecutiveFailures <= 2) ?
                              S3XY_AUTO_RETRY_SHORT_MS : S3XY_AUTO_RETRY_LONG_MS);
    s3xyDevices[connectSlot].nextAttemptMs = millis() + backoff;
  }
  portEXIT_CRITICAL(&s3xyMux);

  s3xySetAutoTrace(String("slot ") + String(connectSlot + 1) + (ready ? " READY" : " link FAIL/backoff"));
  s3xyLogPush(S3XY_LOG_INFO, ready ? "auto reconnect READY" : "auto reconnect link retry scheduled",
              nullptr, 0, -127, (int8_t)connectSlot);
}

static void s3xyMapperTask(void *arg) {
  (void)arg;
  for (;;) {
    if (s3xyRuntimeStopIsPending()) {
      s3xyRuntimeShutdown();
      vTaskDelete(nullptr);
      return;
    }
    S3xyCommandRequest cmd = {};
    const bool haveCmd = s3xyTakeCommand(cmd);
    const bool masterEnabled = s3xyBluetoothMasterIsEnabled();
    // Master ON means the BLE host/controller should really be ON, even with
    // an empty registry. Master OFF means BLEDevice::init() is never called.
    const bool needBle = masterEnabled;
    if (needBle && !s3xyBleInitialized) {
      s3xyBleInitialized = s3xyMapperInit();
      if (!s3xyBleInitialized) {
        s3xyLogPush(S3XY_LOG_ERROR, "BLE initialization failed; retry in task loop");
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }

    if (haveCmd && !masterEnabled) {
      s3xyLogPush(S3XY_LOG_ERROR, "BLE command rejected: Bluetooth Master OFF");
    } else if (haveCmd && s3xyBleInitialized) {
      if (cmd.type == S3XY_CMD_RESET_ALL_BLUETOOTH) {
        s3xyResetAllBluetoothData();
      } else if (cmd.type == S3XY_CMD_DISCOVERY_SCAN) {
        s3xyDiscoveryScan();
      } else if (cmd.type == S3XY_CMD_PAIR_ADDRESS) {
        s3xyPairAddress(cmd.address);
      } else if (cmd.slot >= 0 && cmd.slot < (int8_t)S3XY_MAX_DEVICES) {
        const uint8_t slot = (uint8_t)cmd.slot;
        if (cmd.type == S3XY_CMD_CONNECT_SLOT) {
          char addr[24];
          portENTER_CRITICAL(&s3xyMux);
          if (s3xyDevices[slot].used) {
            s3xyDevices[slot].manualPaused = false;
            strncpy(addr, s3xyDevices[slot].address, sizeof(addr)-1); addr[sizeof(addr)-1] = '\0';
          } else addr[0] = '\0';
          portEXIT_CRITICAL(&s3xyMux);
          if (addr[0]) {
            const bool found = s3xyScanTarget(addr, S3XY_AUTO_SCAN_SECONDS, (int8_t)slot);
            const bool ready = found && s3xyConnectSlot(slot);
            if (!ready) s3xyDisconnectFailedLink(slot, S3XY_MAP_ERROR);
            s3xyLastBleOperationMs = millis();
          }
        } else if (cmd.type == S3XY_CMD_DISCONNECT_SLOT) {
          BLEClient *client;
          portENTER_CRITICAL(&s3xyMux);
          s3xyDevices[slot].manualPaused = true;
          s3xyDevices[slot].nextAttemptMs = 0;
          client = s3xyDevices[slot].client;
          portEXIT_CRITICAL(&s3xyMux);
          if (client && client->isConnected()) {
            client->disconnect();
            vTaskDelay(pdMS_TO_TICKS(S3XY_DISCONNECT_SETTLE_MS));
          }
          s3xyResetLinkRuntime(slot, S3XY_MAP_IDLE);
          s3xyLastBleOperationMs = millis();
        } else if (cmd.type == S3XY_CMD_FORGET_SLOT) {
          s3xyForgetSlot(slot);
        } else if (cmd.type == S3XY_CMD_HANDSHAKE_SLOT) {
          s3xyWriteHandshake(slot);
        }
      }
    } else if (s3xyBleInitialized) {
      const int due = s3xyFindDueAutoSlot(millis());
      if (due >= 0) {
        if (s3xyCountPendingAuto() >= 2) s3xyRunBatchAutoReconnect();
        else s3xyRunAutoAttempt((uint8_t)due);
      }
    }

    bool runAction = false, runAccel = false, runResearchCapture = false, runResearchReset = false;
    uint8_t researchCaptureLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
    portENTER_CRITICAL(&s3xyMux);
    if (s3xyActionPending > 0) { s3xyActionPending--; runAction = true; }
    if (s3xyAccelActionPending > 0) { s3xyAccelActionPending--; runAccel = true; }
    if (s3xyResearchCaptureAPending > 0) {
      s3xyResearchCaptureAPending--; runResearchCapture = true; researchCaptureLabelSlot = RESEARCH_CAPTURE_LABEL_A;
    } else if (s3xyResearchCaptureBPending > 0) {
      s3xyResearchCaptureBPending--; runResearchCapture = true; researchCaptureLabelSlot = RESEARCH_CAPTURE_LABEL_B;
    } else if (s3xyResearchCaptureCPending > 0) {
      s3xyResearchCaptureCPending--; runResearchCapture = true; researchCaptureLabelSlot = RESEARCH_CAPTURE_LABEL_C;
    } else if (s3xyResearchCaptureDPending > 0) {
      s3xyResearchCaptureDPending--; runResearchCapture = true; researchCaptureLabelSlot = RESEARCH_CAPTURE_LABEL_D;
    }
    if (s3xyResearchCaptureResetPending > 0) {
      s3xyResearchCaptureResetPending--; runResearchReset = true;
    }
    portEXIT_CRITICAL(&s3xyMux);
    if (runAction) handleS3xySingleAction();
    if (runAccel) requestPedalMapToggleFromButton();
    if (runResearchReset) {
      researchCaptureReset();
      s3xyLogPush(S3XY_LOG_INFO, "Research Capture Reset · DONE");
    }
    if (runResearchCapture) {
      const bool accepted = researchCaptureRequest(researchCaptureLabelSlot);
      String msg = String("Research Capture ") + researchCaptureLabelSlotName(researchCaptureLabelSlot)
                 + " · " + (accepted ? "STARTED" : "IGNORED");
      s3xyLogPush(S3XY_LOG_INFO, msg.c_str());
    }

    static uint32_t lastRssiMs = 0;
    if (s3xyBleInitialized && millis() - lastRssiMs >= 2000) {
      lastRssiMs = millis();
      for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
        BLEClient *client;
        bool used, connected;
        portENTER_CRITICAL(&s3xyMux);
        used = s3xyDevices[i].used;
        connected = s3xyDevices[i].connected;
        client = s3xyDevices[i].client;
        portEXIT_CRITICAL(&s3xyMux);
        if (used && connected && client && client->isConnected()) {
          const int16_t rssi = (int16_t)client->getRssi();
          portENTER_CRITICAL(&s3xyMux); s3xyDevices[i].rssi = rssi; portEXIT_CRITICAL(&s3xyMux);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static String s3xyStatsToJson() {
  const uint32_t now = millis();
  bool autoEnabled, bluetoothEnabled, bleInitialized, scanning;
  uint8_t discoveredCount;
  char discoveryError[64];
#if S3XY_DIAGNOSTICS_ENABLED
  uint32_t logDropped;
  uint32_t identityProbeAttempts, identityProbeMatches, identityProbeMismatches, identityRebinds;
  uint32_t autoExactHits, autoExactMisses, autoLinkFailures;
  int16_t bootLocalBondCount;
  bool bondRepairArmed;
  uint32_t bondRepairAttempts, bondRepairReady, bondPersistStillMissing;
  char bleStack[16];
  char autoTrace[112];
  uint32_t totalAttempts = 0, totalReconnects = 0, totalFailures = 0;
#endif

  portENTER_CRITICAL(&s3xyMux);
  autoEnabled = s3xyAutoEnabled;
  bluetoothEnabled = s3xyBluetoothEnabled;
  bleInitialized = s3xyBleInitialized;
  scanning = s3xyDiscoveryScanning;
  discoveredCount = s3xyDiscoveredCount;
  strncpy(discoveryError, s3xyDiscoveryError, sizeof(discoveryError)-1);
  discoveryError[sizeof(discoveryError)-1] = '\0';
#if S3XY_DIAGNOSTICS_ENABLED
  logDropped = s3xyLogDropped;
  identityProbeAttempts = s3xyIdentityProbeAttempts;
  identityProbeMatches = s3xyIdentityProbeMatches;
  identityProbeMismatches = s3xyIdentityProbeMismatches;
  identityRebinds = s3xyIdentityRebinds;
  autoExactHits = s3xyAutoExactHits;
  autoExactMisses = s3xyAutoExactMisses;
  autoLinkFailures = s3xyAutoLinkFailures;
  bootLocalBondCount = s3xyBootLocalBondCount;
  bondRepairArmed = s3xyBootBondRepairArmed;
  bondRepairAttempts = s3xyBondRepairAttempts;
  bondRepairReady = s3xyBondRepairReady;
  bondPersistStillMissing = s3xyBondPersistStillMissing;
  strncpy(bleStack, s3xyBleStackName, sizeof(bleStack) - 1);
  bleStack[sizeof(bleStack) - 1] = '\0';
  strncpy(autoTrace, s3xyAutoTrace, sizeof(autoTrace) - 1);
  autoTrace[sizeof(autoTrace) - 1] = '\0';
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    totalAttempts += s3xyDevices[i].autoAttempts;
    totalReconnects += s3xyDevices[i].autoReconnects;
    totalFailures += s3xyDevices[i].autoFailures;
  }
#endif
  portEXIT_CRITICAL(&s3xyMux);

  uint32_t ulcExpire, ulcAccepted, ulcBlocked, ulcTxOk, ulcTxFail, ulcLastAction;
  bool ulcPending;
  uint8_t ulcLastDir;
  char ulcResult[64];
  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcPending = ulcSnoozePending;
  ulcExpire = ulcSnoozeExpireMs;
  ulcAccepted = ulcSnoozeAccepted;
  ulcBlocked = ulcSnoozeBlocked;
  ulcTxOk = ulcSnoozeTxOk;
  ulcTxFail = ulcSnoozeTxFail;
  ulcLastAction = ulcSnoozeLastActionMs;
  ulcLastDir = ulcSnoozeLastDir;
  strncpy(ulcResult, ulcSnoozeLastResult, sizeof(ulcResult)-1);
  ulcResult[sizeof(ulcResult)-1] = '\0';
  portEXIT_CRITICAL(&ulcSnoozeMux);

  String j;
#if S3XY_DIAGNOSTICS_ENABLED
  j.reserve(4096);
#else
  j.reserve(3072);
#endif
  j = "{";
  j += "\"bluetoothEnabled\":" + String(bluetoothEnabled ? "true" : "false");
  j += ",\"bleInitialized\":" + String(bleInitialized ? "true" : "false");
  j += ",\"autoEnabled\":" + String(autoEnabled ? "true" : "false");
  j += ",\"diagnosticsEnabled\":" + String(S3XY_DIAGNOSTICS_ENABLED ? "true" : "false");
  j += ",\"maxDevices\":" + String(S3XY_MAX_DEVICES);
  j += ",\"pairedCount\":" + String(s3xyRegisteredCount());
  j += ",\"connectedCount\":" + String(s3xyConnectedCount());
#if S3XY_DIAGNOSTICS_ENABLED
  j += ",\"localBondCount\":" + String(s3xyLocalBondCount());
  j += ",\"bleStack\":\"" + s3xyJsonEscape(bleStack) + "\"";
#ifdef ESP_ARDUINO_VERSION_STR
  j += ",\"arduinoCore\":\"" + String(ESP_ARDUINO_VERSION_STR) + "\"";
#else
  j += ",\"arduinoCore\":\"unknown\"";
#endif
  j += ",\"bootLocalBondCount\":" + String((int)bootLocalBondCount);
  j += ",\"bondRepairArmed\":" + String(bondRepairArmed ? "true" : "false");
  j += ",\"bondRepairAttempts\":" + String(bondRepairAttempts);
  j += ",\"bondRepairReady\":" + String(bondRepairReady);
  j += ",\"bondPersistStillMissing\":" + String(bondPersistStillMissing);
  j += ",\"identityProbeAttempts\":" + String(identityProbeAttempts);
  j += ",\"identityProbeMatches\":" + String(identityProbeMatches);
  j += ",\"identityProbeMismatches\":" + String(identityProbeMismatches);
  j += ",\"identityRebinds\":" + String(identityRebinds);
  j += ",\"autoExactHits\":" + String(autoExactHits);
  j += ",\"autoExactMisses\":" + String(autoExactMisses);
  j += ",\"autoLinkFailures\":" + String(autoLinkFailures);
  j += ",\"autoTrace\":\"" + s3xyJsonEscape(autoTrace) + "\"";
  j += ",\"autoAttempts\":" + String(totalAttempts);
  j += ",\"autoReconnects\":" + String(totalReconnects);
  j += ",\"autoFailures\":" + String(totalFailures);
#endif
  j += ",\"scanning\":" + String(scanning ? "true" : "false");
  j += ",\"scanError\":\"" + s3xyJsonEscape(discoveryError) + "\"";
  j += ",\"devices\":[";

  bool first = true;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    bool used, connected, paused, deviceAutoConnect;
    uint8_t state, singleAction, doubleAction, longAction;
    int16_t rssi;
    char addr[24], name[28];
#if S3XY_DIAGNOSTICS_ENABLED
    bool subscribed, secure, addressTypeKnown, identityPersistVerified;
    uint8_t addressType, lastLen, peerIdLen;
    uint32_t notifyCount, unparsedCount, singleCount, doubleCount, longCount, ackCount, lastMs;
    uint32_t nextAttempt, attempts, reconnects, failures, consecutive;
    uint32_t lastDiscoverMs, lastConnectMs, lastReadyMs;
    char err[64], idHex[64], lastAutoPath[24];
    uint8_t last[S3XY_LOG_DATA_MAX] = {};
#endif
    portENTER_CRITICAL(&s3xyMux);
    used = s3xyDevices[i].used;
    if (used) {
      connected = s3xyDevices[i].connected;
      paused = s3xyDevices[i].manualPaused;
      deviceAutoConnect = s3xyDevices[i].autoConnect;
      state = s3xyDevices[i].state;
      rssi = s3xyDevices[i].rssi;
      singleAction = s3xyDevices[i].singleAction;
      doubleAction = s3xyDevices[i].doubleAction;
      longAction = s3xyDevices[i].longAction;
      strncpy(addr, s3xyDevices[i].address, sizeof(addr)-1);
      addr[sizeof(addr)-1] = '\0';
      strncpy(name, s3xyDevices[i].name, sizeof(name)-1);
      name[sizeof(name)-1] = '\0';
#if S3XY_DIAGNOSTICS_ENABLED
      subscribed = s3xyDevices[i].subscribed;
      secure = s3xyDevices[i].secureOk;
      addressType = s3xyDevices[i].addressType;
      addressTypeKnown = s3xyDevices[i].addressTypeKnown;
      identityPersistVerified = s3xyDevices[i].identityPersistVerified;
      peerIdLen = s3xyDevices[i].peerIdLen;
      notifyCount = s3xyDevices[i].notifyCount;
      unparsedCount = s3xyDevices[i].unparsedCount;
      singleCount = s3xyDevices[i].singleCount;
      doubleCount = s3xyDevices[i].doubleCount;
      longCount = s3xyDevices[i].longCount;
      ackCount = s3xyDevices[i].handshakeAckCount;
      lastMs = s3xyDevices[i].lastNotifyMs;
      lastLen = s3xyDevices[i].lastNotifyLen;
      memcpy(last, s3xyDevices[i].lastNotify, lastLen);
      nextAttempt = s3xyDevices[i].nextAttemptMs;
      attempts = s3xyDevices[i].autoAttempts;
      reconnects = s3xyDevices[i].autoReconnects;
      failures = s3xyDevices[i].autoFailures;
      consecutive = s3xyDevices[i].consecutiveFailures;
      lastDiscoverMs = s3xyDevices[i].lastDiscoverMs;
      lastConnectMs = s3xyDevices[i].lastConnectMs;
      lastReadyMs = s3xyDevices[i].lastReadyMs;
      strncpy(lastAutoPath, s3xyDevices[i].lastAutoPath, sizeof(lastAutoPath)-1);
      lastAutoPath[sizeof(lastAutoPath)-1] = '\0';
      strncpy(err, s3xyDevices[i].lastError, sizeof(err)-1);
      err[sizeof(err)-1] = '\0';
      strncpy(idHex, s3xyDevices[i].idHex, sizeof(idHex)-1);
      idHex[sizeof(idHex)-1] = '\0';
#endif
    }
    portEXIT_CRITICAL(&s3xyMux);
    if (!used) continue;

    if (!first) j += ",";
    first = false;
    j += "{";
    j += "\"id\":" + String((unsigned)(i + 1));
    j += ",\"name\":\"" + s3xyJsonEscape(name) + "\"";
    j += ",\"address\":\"" + s3xyJsonEscape(addr) + "\"";
    j += ",\"state\":\"" + String(s3xyStateName(state)) + "\"";
    j += ",\"connected\":" + String(connected ? "true" : "false");
    j += ",\"paused\":" + String(paused ? "true" : "false");
    j += ",\"autoConnect\":" + String(deviceAutoConnect ? "true" : "false");
    j += ",\"rssi\":" + String((int)rssi);
    j += ",\"singleAction\":\"" + String(s3xyActionCode(singleAction)) + "\"";
    j += ",\"doubleAction\":\"" + String(s3xyActionCode(doubleAction)) + "\"";
    j += ",\"longAction\":\"" + String(s3xyActionCode(longAction)) + "\"";
    j += ",\"singleLabel\":\"" + String(s3xyActionLabel(singleAction)) + "\"";
    j += ",\"doubleLabel\":\"" + String(s3xyActionLabel(doubleAction)) + "\"";
    j += ",\"longLabel\":\"" + String(s3xyActionLabel(longAction)) + "\"";
#if S3XY_DIAGNOSTICS_ENABLED
    char lastHex[80] = {};
    s3xyBytesToHex(last, lastLen, lastHex, sizeof(lastHex));
    const uint32_t retryMs = (nextAttempt && (int32_t)(nextAttempt - now) > 0) ? (nextAttempt - now) : 0;
    j += ",\"addressTypeKnown\":" + String(addressTypeKnown ? "true" : "false");
    j += ",\"addressType\":" + String((unsigned)addressType);
    j += ",\"subscribed\":" + String(subscribed ? "true" : "false");
    j += ",\"secure\":" + String(secure ? "true" : "false");
    j += ",\"notifyCount\":" + String(notifyCount);
    j += ",\"unparsedCount\":" + String(unparsedCount);
    j += ",\"singleCount\":" + String(singleCount);
    j += ",\"doubleCount\":" + String(doubleCount);
    j += ",\"longCount\":" + String(longCount);
    j += ",\"handshakeAckCount\":" + String(ackCount);
    j += ",\"lastNotifyMs\":" + String(lastMs);
    j += ",\"lastNotify\":\"" + String(lastHex) + "\"";
    j += ",\"idHex\":\"" + s3xyJsonEscape(idHex) + "\"";
    j += ",\"peerIdLen\":" + String((unsigned)peerIdLen);
    j += ",\"identityPersistVerified\":" + String(identityPersistVerified ? "true" : "false");
    j += ",\"autoAttempts\":" + String(attempts);
    j += ",\"autoReconnects\":" + String(reconnects);
    j += ",\"autoFailures\":" + String(failures);
    j += ",\"consecutiveFailures\":" + String(consecutive);
    j += ",\"lastDiscoverMs\":" + String(lastDiscoverMs);
    j += ",\"lastConnectMs\":" + String(lastConnectMs);
    j += ",\"lastReadyMs\":" + String(lastReadyMs);
    j += ",\"lastAutoPath\":\"" + s3xyJsonEscape(lastAutoPath) + "\"";
    j += ",\"retryInMs\":" + String(retryMs);
    j += ",\"error\":\"" + s3xyJsonEscape(err) + "\"";
#endif
    j += "}";
  }
  j += "]";

  j += ",\"scanResults\":[";
  for (uint8_t i = 0; i < discoveredCount; i++) {
    S3xyDiscoveredDevice d;
    portENTER_CRITICAL(&s3xyMux);
    d = s3xyDiscovered[i];
    portEXIT_CRITICAL(&s3xyMux);
    if (i) j += ",";
    j += "{\"address\":\"" + s3xyJsonEscape(d.address) + "\"";
    j += ",\"name\":\"" + s3xyJsonEscape(d.name) + "\"";
    j += ",\"rssi\":" + String((int)d.rssi);
    j += ",\"registered\":" + String(d.registered ? "true" : "false") + "}";
  }
  j += "]";

  const bool ulcRequestPending = ulcPending && (int32_t)(ulcExpire - now) > 0;
  j += ",\"ulcRequestPending\":" + String(ulcRequestPending ? "true" : "false");
  j += ",\"ulcAccepted\":" + String(ulcAccepted);
  j += ",\"ulcBlocked\":" + String(ulcBlocked);
  j += ",\"ulcTxOk\":" + String(ulcTxOk);
  j += ",\"ulcTxFail\":" + String(ulcTxFail);
  j += ",\"ulcLastActionMs\":" + String(ulcLastAction);
  j += ",\"ulcLastDir\":" + String((unsigned)ulcLastDir);
  j += ",\"ulcResult\":\"" + s3xyJsonEscape(ulcResult) + "\"";
#if S3XY_DIAGNOSTICS_ENABLED
  j += ",\"logDropped\":" + String((unsigned long)logDropped);
#endif
  j += "}";
  return j;
}

static void csvEscapeField(const char *src, char *dst, size_t dstLen) {
  if (!dst || dstLen == 0) return;
  size_t w = 0;
  if (!src) src = "";
  for (size_t r = 0; src[r] && w + 1 < dstLen; r++) {
    char c = src[r];
    if (c == '\r') continue;
    if (c == '\n') c = ' ';
    if (c == '"') {
      if (w + 2 >= dstLen) break;
      dst[w++] = '"';
      dst[w++] = '"';
    } else {
      dst[w++] = c;
    }
  }
  dst[w] = '\0';
}

static void s3xyClearLog() {
#if S3XY_DIAGNOSTICS_ENABLED
  portENTER_CRITICAL(&s3xyMux);
  s3xyLogHead = 0; s3xyLogCount = 0; s3xyLogDropped = 0;
  s3xyActionPending = 0; s3xyAccelActionPending = 0;
  s3xyResearchCaptureAPending = 0; s3xyResearchCaptureBPending = 0;
  s3xyResearchCaptureCPending = 0; s3xyResearchCaptureDPending = 0;
  s3xyResearchCaptureResetPending = 0;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    s3xyDevices[i].notifyCount = 0;
    s3xyDevices[i].unparsedCount = 0;
    s3xyDevices[i].singleCount = 0;
    s3xyDevices[i].doubleCount = 0;
    s3xyDevices[i].longCount = 0;
    s3xyDevices[i].handshakeAckCount = 0;
    s3xyDevices[i].autoAttempts = 0;
    s3xyDevices[i].autoReconnects = 0;
    s3xyDevices[i].autoFailures = 0;
    // Preserve consecutiveFailures because it participates in reconnect backoff.
    s3xyDevices[i].lastNotifyMs = 0;
    s3xyDevices[i].lastNotifyLen = 0;
    memset(s3xyDevices[i].lastNotify, 0, sizeof(s3xyDevices[i].lastNotify));
  }
  portEXIT_CRITICAL(&s3xyMux);
  s3xyLogPush(S3XY_LOG_INFO, "multi-device/action log cleared");
#endif
}


