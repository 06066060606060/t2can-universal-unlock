#pragma once

// CAN Research Capture
// RX-only CAN A + CAN B recorder for general CAN research.
// SNAPSHOT mode keeps the v2.7b3 full-state snapshot workflow.
// RAW modes use an independent rolling 5 s RX PRE ring plus an append-only archive.
// Manual RAW copies up to 5 s PRE; AUTO ALC events copy 2 s PRE and record 2 s POST.
// In RAW_AUTO_ALC mode, manual C/D captures remain available between AUTO episodes
// and use the full 5 s manual PRE window; A/B stay reserved for automatic OPEN/BLOCKED labels.
// Completed RAW segments auto re-arm without Reset, up to 8 archived segments.
// Qualified LEFT availability events use semantic state classes:
// OPEN = 6/8, BLOCKED = every other valid DAS_autoLaneChangeState. A transition
// must persist for 300 ms. Events inside an active AUTO episode extend POST instead
// of consuming another segment. OPEN events require a fresh LEFT 0x239 lane with
// FUSED line usage; BLOCKED events intentionally do not, so the recorder can capture
// the very lane/topology loss that may be causing the block. The RIGHT lane is not required.
// Existing 0x3F8 VH and 0x399 Party overlay attempts are observable metadata;
// this module never transmits or replays CAN.

static constexpr uint32_t RESEARCH_CAPTURE_PRE_DEFAULT_MS = 2000;
static constexpr uint32_t RESEARCH_CAPTURE_POST_DEFAULT_MS = 5000;
static constexpr uint32_t RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS = 2000;
static constexpr uint32_t RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS = 500;
static constexpr uint8_t  RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT = 5;
static constexpr uint8_t  RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT = 5;
static constexpr uint8_t  RESEARCH_CAPTURE_PRE_SLOT_COUNT = RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT + RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT;
static constexpr uint8_t  RESEARCH_CAPTURE_POST_MAX_COUNT = 5;
static constexpr uint16_t RESEARCH_CAPTURE_POST_TARGETS[RESEARCH_CAPTURE_POST_MAX_COUNT] = {500, 1000, 2000, 5000, 10000};
static constexpr uint32_t RESEARCH_CAPTURE_CAPACITY = 131072;
static constexpr uint16_t RESEARCH_CAPTURE_IDS_PER_BUS = 2048;
static constexpr uint16_t RESEARCH_CAPTURE_STATE_COUNT = RESEARCH_CAPTURE_IDS_PER_BUS * 2;
static constexpr uint8_t  RESEARCH_CAPTURE_MAX_SEGMENTS = 32;
static constexpr uint8_t  RESEARCH_CAPTURE_LABEL_SLOTS = 4;
static constexpr size_t   RESEARCH_CAPTURE_LABEL_BYTES = 64;
static constexpr uint8_t  RESEARCH_CAPTURE_BUS_PARTY = 0;
static constexpr uint8_t  RESEARCH_CAPTURE_BUS_VH = 1;
static constexpr uint8_t  RESEARCH_CAPTURE_BUS_PARTY_TX_OK = 2;
static constexpr uint8_t  RESEARCH_CAPTURE_BUS_PARTY_TX_FAIL = 3;
static constexpr uint8_t  RESEARCH_CAPTURE_FLAG_TX = 0x01;
static constexpr uint8_t  RESEARCH_CAPTURE_FLAG_TX_OK = 0x02;
static constexpr uint32_t RESEARCH_CAPTURE_RAW_MANUAL_PRE_MS = 5000;
static constexpr uint32_t RESEARCH_CAPTURE_RAW_AUTO_PRE_MS = 2000;
static constexpr uint32_t RESEARCH_CAPTURE_RAW_POST_MS = 2000;
static constexpr uint32_t RESEARCH_CAPTURE_AUTO_LANE_FRESH_MS = 1000;
static constexpr uint32_t RESEARCH_CAPTURE_AUTO_PERSIST_MS = 300;
static constexpr uint32_t RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY = 360448;
static constexpr uint32_t RESEARCH_CAPTURE_RAW_PRE_CAPACITY = 32768;
static constexpr uint8_t  RESEARCH_CAPTURE_RAW_MAX_SEGMENTS = 8;

enum ResearchCaptureMode : uint8_t {
  RESEARCH_CAPTURE_MODE_SNAPSHOT = 0,
  RESEARCH_CAPTURE_MODE_RAW_TRANSITION = 1,
  RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC = 2
};

static constexpr uint8_t RESEARCH_CAPTURE_LABEL_A = 0;
static constexpr uint8_t RESEARCH_CAPTURE_LABEL_B = 1;
static constexpr uint8_t RESEARCH_CAPTURE_LABEL_C = 2;
static constexpr uint8_t RESEARCH_CAPTURE_LABEL_D = 3;
static constexpr uint8_t RESEARCH_CAPTURE_LABEL_NONE = 0xFF;

enum ResearchCaptureState : uint8_t {
  RESEARCH_CAPTURE_READY = 0,
  RESEARCH_CAPTURE_CAPTURING,
  RESEARCH_CAPTURE_PAUSED,
  RESEARCH_CAPTURE_FULL,
  RESEARCH_CAPTURE_ERROR
};

struct __attribute__((packed)) ResearchCaptureLatest {
  uint32_t lastSeenMs;
  uint8_t dlc;
  uint8_t valid;
  uint8_t data[8];
};
static_assert(sizeof(ResearchCaptureLatest) == 14, "research latest-state entry size changed");

struct __attribute__((packed)) ResearchCapturePreState {
  uint16_t stateIndex;
  uint16_t frameAgeMs;
  uint8_t dlc;
  uint8_t data[8];
};
static_assert(sizeof(ResearchCapturePreState) == 13, "research pre-state entry size changed");

struct __attribute__((packed)) ResearchCaptureEntry {
  uint16_t segment;
  int16_t relativeMs;
  uint16_t id;
  uint16_t frameAgeMs;
  uint8_t bus;
  uint8_t dlc;
  uint8_t labelSlot;
  uint8_t flags;
  uint8_t data[8];
};
static_assert(sizeof(ResearchCaptureEntry) == 20, "research capture entry size changed");

struct __attribute__((packed)) ResearchCaptureRawEntry {
  uint32_t timestampMs;
  uint16_t id;
  uint8_t bus;
  uint8_t dlc;
  uint8_t data[8];
};
static_assert(sizeof(ResearchCaptureRawEntry) == 16, "research raw entry size changed");

static constexpr size_t RESEARCH_CAPTURE_SNAPSHOT_MAIN_BYTES = sizeof(ResearchCaptureEntry) * RESEARCH_CAPTURE_CAPACITY;
static constexpr size_t RESEARCH_CAPTURE_RAW_MAIN_BYTES = sizeof(ResearchCaptureRawEntry) * RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY;
static constexpr size_t RESEARCH_CAPTURE_SNAPSHOT_AUX_BYTES = sizeof(ResearchCapturePreState) * RESEARCH_CAPTURE_STATE_COUNT * RESEARCH_CAPTURE_PRE_SLOT_COUNT;
static constexpr size_t RESEARCH_CAPTURE_RAW_AUX_BYTES = sizeof(ResearchCaptureRawEntry) * RESEARCH_CAPTURE_RAW_PRE_CAPACITY;
static constexpr size_t RESEARCH_CAPTURE_LATEST_BYTES = sizeof(ResearchCaptureLatest) * RESEARCH_CAPTURE_STATE_COUNT;
static constexpr size_t RESEARCH_CAPTURE_KNOWN_BYTES = sizeof(uint16_t) * RESEARCH_CAPTURE_STATE_COUNT;

// v3.3 optimization: allocate the large archive/pre-history blocks for the
// selected capture mode instead of permanently reserving the RAW maximum.
// Snapshot capacity and RAW capacity are unchanged.
static inline size_t researchCaptureMainBytesForMode(uint8_t mode) {
  return (mode == RESEARCH_CAPTURE_MODE_RAW_TRANSITION || mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC) ? RESEARCH_CAPTURE_RAW_MAIN_BYTES : RESEARCH_CAPTURE_SNAPSHOT_MAIN_BYTES;
}

static inline size_t researchCaptureAuxBytesForMode(uint8_t mode) {
  return (mode == RESEARCH_CAPTURE_MODE_RAW_TRANSITION || mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC) ? RESEARCH_CAPTURE_RAW_AUX_BYTES : RESEARCH_CAPTURE_SNAPSHOT_AUX_BYTES;
}

struct __attribute__((packed)) ResearchCapturePreSlot {
  uint32_t snapshotMs;
  uint16_t count;
  uint16_t txAgeMs;
  uint8_t valid;
  uint8_t txValid;
  uint8_t txDlc;
  uint8_t txOk;
  uint8_t txData[8];
};

struct ResearchCaptureSegmentMeta {
  uint32_t triggerMs;
  uint32_t rawStartIndex;
  uint32_t rawFrameCount;
  uint16_t rawEventCount;
  uint8_t labelSlot;
  // Trigger-time summary for AUTO A/B research segments. 0xFF means unavailable/manual.
  uint8_t triggerAlcFrom;
  uint8_t triggerAlcTo;
  uint8_t triggerLeftLaneExists;
  uint8_t triggerLeftLineUsage;
  uint8_t triggerRightLaneExists;
  uint8_t triggerDasState;
  uint8_t triggerRoadClass;
  uint8_t triggerGpsRoadMatch;
  uint8_t triggerNavRouteActive;
  uint8_t triggerControlledAccess;
  uint8_t triggerLeftOffRamp;
  uint8_t triggerRightOffRamp;
  uint32_t triggerRoadAgeMs;
  char label[RESEARCH_CAPTURE_LABEL_BYTES];
};

static portMUX_TYPE researchCaptureMux = portMUX_INITIALIZER_UNLOCKED;
static ResearchCaptureEntry *researchCaptureEntries = nullptr;
static ResearchCaptureLatest *researchCaptureLatest = nullptr;
static ResearchCapturePreState *researchCapturePreStates = nullptr;
static uint16_t *researchCaptureKnownIndices = nullptr;
static volatile size_t researchCaptureAllocatedMainBytes = 0;
static volatile size_t researchCaptureAllocatedAuxBytes = 0;
static ResearchCapturePreSlot researchCapturePreSlots[RESEARCH_CAPTURE_PRE_SLOT_COUNT] = {};
static ResearchCaptureSegmentMeta researchCaptureSegments[RESEARCH_CAPTURE_MAX_SEGMENTS] = {};
static char researchCaptureLabels[RESEARCH_CAPTURE_LABEL_SLOTS][RESEARCH_CAPTURE_LABEL_BYTES] = {
  "Label A", "Label B", "Label C", "Label D"
};

static volatile uint32_t researchCaptureCount = 0;
static volatile uint16_t researchCaptureSegment = 0;
static volatile uint16_t researchCaptureCompletedSegments = 0;
static volatile uint16_t researchCaptureKnownIds = 0;
static volatile uint8_t researchCaptureState = RESEARCH_CAPTURE_READY;
static volatile uint8_t researchCaptureCurrentLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
static volatile uint8_t researchCaptureLastLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
static volatile uint8_t researchCapturePreHead = 0; // dense tier head
static volatile uint8_t researchCapturePreValidCount = 0; // dense tier valid count
static volatile uint8_t researchCapturePreExtHead = 0;
static volatile uint8_t researchCapturePreExtValidCount = 0;
static volatile uint8_t researchCaptureNextPostIndex = 0;
static volatile uint8_t researchCaptureLastPreSnapshotsCopied = 0;
static volatile uint8_t researchCaptureLastPostSnapshotsCopied = 0;
static volatile uint16_t researchCaptureLastTriggerRows = 0;
static volatile uint32_t researchCaptureStartMs = 0;
static volatile uint32_t researchCaptureLastPreSnapshotMs = 0; // dense tier timestamp
static volatile uint32_t researchCaptureLastPreExtSnapshotMs = 0;
static volatile uint32_t researchCapturePreWindowMs = RESEARCH_CAPTURE_PRE_DEFAULT_MS;
static volatile uint32_t researchCapturePostWindowMs = RESEARCH_CAPTURE_POST_DEFAULT_MS;
static volatile uint8_t researchCaptureMode = RESEARCH_CAPTURE_MODE_SNAPSHOT;
static volatile bool researchCaptureConfigUpdating = false;
static volatile bool researchCaptureExporting = false;
// RAW modes use mode-sized main/aux PSRAM blocks as a compact 16-byte archive
// plus an independent rolling PRE ring. This preserves completed segments while PRE
// recording continues for the next automatic or manual trigger.
static volatile uint32_t researchCaptureRawArchiveCount = 0;
static volatile uint32_t researchCaptureRawPreStartIndex = 0;
static volatile uint32_t researchCaptureRawPreFrameCount = 0;
static volatile uint32_t researchCaptureRawTriggerMs = 0;
static volatile uint32_t researchCaptureRawPostDeadlineMs = 0;
static volatile uint32_t researchCaptureRawEvicted = 0;
static volatile bool researchCaptureRawTriggered = false;
static volatile uint8_t researchCaptureRawLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
static volatile uint32_t researchCaptureRawFirstFrameMs = 0;
static ResearchAlcPersistenceState researchCaptureAutoPersistence = researchAlcPersistenceInitialPure();
static volatile uint8_t researchCaptureAutoLastEvent = 0; // 1=AUTO_CLOSE, 2=AUTO_OPEN
static volatile uint8_t researchCaptureAutoLastFrom = 0xFF;
static volatile uint8_t researchCaptureAutoLastTo = 0xFF;
static volatile uint32_t researchCaptureAutoQualifiedTransitions = 0;
static volatile uint32_t researchCaptureAutoTriggerCount = 0;
static volatile uint32_t researchCaptureAutoRejectLane = 0;
static volatile uint32_t researchCaptureAutoRejectWarmup = 0;
static volatile uint32_t researchCaptureAutoRejectBusy = 0;
static volatile uint32_t researchCaptureAutoMergedEvents = 0;
static volatile uint32_t researchCaptureAutoOpenCount = 0;
static volatile uint32_t researchCaptureAutoBlockedCount = 0;
static volatile uint32_t researchCaptureAutoLastTriggerMs = 0;
static volatile uint32_t researchCaptureRequests = 0;
static volatile uint32_t researchCaptureIgnored = 0;
static volatile uint32_t researchCaptureDropped = 0;
static volatile bool researchCaptureUsingPsram = false;

// Latest metadata for the existing T-2CAN VH 0x3F8 overlay TX attempt.
static volatile uint32_t researchCaptureTx3f8Ms = 0;
static volatile uint8_t researchCaptureTx3f8Dlc = 0;
static volatile uint8_t researchCaptureTx3f8Ok = 0;
static volatile uint8_t researchCaptureTx3f8Valid = 0;
static uint8_t researchCaptureTx3f8Data[8] = {};

static inline bool researchCaptureModeIsRaw(uint8_t mode) {
  return mode == RESEARCH_CAPTURE_MODE_RAW_TRANSITION || mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC;
}

static const char *researchCaptureModeName(uint8_t mode) {
  if (mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC) return "RAW_AUTO_ALC";
  if (mode == RESEARCH_CAPTURE_MODE_RAW_TRANSITION) return "RAW_TRANSITION";
  return "SNAPSHOT";
}

static const char *researchCaptureAutoEventName(uint8_t event) {
  if (event == 1) return "AUTO_CLOSE";
  if (event == 2) return "AUTO_OPEN";
  return "NONE";
}

static const char *researchCaptureStateName(uint8_t state) {
  switch (state) {
    case RESEARCH_CAPTURE_READY: return "READY";
    case RESEARCH_CAPTURE_CAPTURING: return "CAPTURING";
    case RESEARCH_CAPTURE_PAUSED: return "PAUSED";
    case RESEARCH_CAPTURE_FULL: return "FULL";
    case RESEARCH_CAPTURE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

static const char *researchCaptureLabelSlotName(uint8_t slot) {
  switch (slot) {
    case RESEARCH_CAPTURE_LABEL_A: return "A";
    case RESEARCH_CAPTURE_LABEL_B: return "B";
    case RESEARCH_CAPTURE_LABEL_C: return "C";
    case RESEARCH_CAPTURE_LABEL_D: return "D";
    default: return "NONE";
  }
}

static void researchCaptureLoadLabels() {
  static const char *defaults[RESEARCH_CAPTURE_LABEL_SLOTS] = {"Label A", "Label B", "Label C", "Label D"};
  static const char *keys[RESEARCH_CAPTURE_LABEL_SLOTS] = {"la", "lb", "lc", "ld"};
  Preferences p;
  const bool opened = p.begin("researchcap", true);
  for (uint8_t i = 0; i < RESEARCH_CAPTURE_LABEL_SLOTS; i++) {
    String value = opened ? p.getString(keys[i], defaults[i]) : String(defaults[i]);
    if (value.length() == 0 || value.length() >= RESEARCH_CAPTURE_LABEL_BYTES) value = defaults[i];
    strncpy(researchCaptureLabels[i], value.c_str(), RESEARCH_CAPTURE_LABEL_BYTES - 1);
    researchCaptureLabels[i][RESEARCH_CAPTURE_LABEL_BYTES - 1] = '\0';
  }
  if (opened) p.end();
}

static bool researchCaptureSetLabel(uint8_t slot, const String &valueIn) {
  if (slot >= RESEARCH_CAPTURE_LABEL_SLOTS) return false;
  String value = valueIn;
  value.trim();
  if (value.length() == 0 || value.length() >= RESEARCH_CAPTURE_LABEL_BYTES) return false;
  char copy[RESEARCH_CAPTURE_LABEL_BYTES] = {};
  memcpy(copy, value.c_str(), value.length());
  copy[value.length()] = '\0';
  static const char *keys[RESEARCH_CAPTURE_LABEL_SLOTS] = {"la", "lb", "lc", "ld"};
  Preferences p;
  if (!p.begin("researchcap", false)) return false;
  const size_t written = p.putString(keys[slot], copy);
  p.end();
  if (written == 0) return false;
  portENTER_CRITICAL(&researchCaptureMux);
  memcpy(researchCaptureLabels[slot], copy, sizeof(copy));
  portEXIT_CRITICAL(&researchCaptureMux);
  return true;
}


static bool researchCapturePreWindowSupported(uint32_t ms) {
  return ms == 2000U || ms == 5000U || ms == 10000U || ms == 15000U;
}

static bool researchCapturePostWindowSupported(uint32_t ms) {
  return ms == 2000U || ms == 5000U || ms == 10000U;
}

static uint8_t researchCapturePostCountForWindow(uint32_t ms) {
  if (ms <= 2000U) return 3;
  if (ms <= 5000U) return 4;
  return RESEARCH_CAPTURE_POST_MAX_COUNT;
}

static uint32_t researchCaptureExtendedIntervalMs(uint32_t preMs) {
  if (preMs <= RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS || RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT < 2) return 0;
  const uint32_t divisor = (uint32_t)RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT - 1U;
  return (preMs + divisor - 1U) / divisor;
}

static void researchCaptureLoadConfig() {
  uint32_t preMs = RESEARCH_CAPTURE_PRE_DEFAULT_MS;
  uint32_t postMs = RESEARCH_CAPTURE_POST_DEFAULT_MS;
  uint8_t mode = RESEARCH_CAPTURE_MODE_SNAPSHOT;
  Preferences p;
  if (p.begin("researchcap", true)) {
    preMs = p.getUInt("prems", RESEARCH_CAPTURE_PRE_DEFAULT_MS);
    postMs = p.getUInt("postms", RESEARCH_CAPTURE_POST_DEFAULT_MS);
    mode = p.getUChar("mode", RESEARCH_CAPTURE_MODE_SNAPSHOT);
    p.end();
  }
  if (!researchCapturePreWindowSupported(preMs)) preMs = RESEARCH_CAPTURE_PRE_DEFAULT_MS;
  if (!researchCapturePostWindowSupported(postMs)) postMs = RESEARCH_CAPTURE_POST_DEFAULT_MS;
  if (mode != RESEARCH_CAPTURE_MODE_SNAPSHOT && mode != RESEARCH_CAPTURE_MODE_RAW_TRANSITION &&
      mode != RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC) mode = RESEARCH_CAPTURE_MODE_SNAPSHOT;
  // RAW AUTO ALC decodes YL-specific CAN A (Party) 0x239/0x399 semantics.
  // If a stored YL mode follows a later profile change, fail closed at runtime.
  if (mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC && !activeProfileIsYl())
    mode = RESEARCH_CAPTURE_MODE_SNAPSHOT;
  researchCapturePreWindowMs = preMs;
  researchCapturePostWindowMs = postMs;
  researchCaptureMode = mode;
}

static bool researchCaptureSetConfig(uint32_t preMs, uint32_t postMs) {
  if (!researchCapturePreWindowSupported(preMs) || !researchCapturePostWindowSupported(postMs)) return false;
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING || researchCaptureConfigUpdating || researchCaptureExporting) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }
  if (preMs == researchCapturePreWindowMs && postMs == researchCapturePostWindowMs) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return true;
  }
  researchCaptureConfigUpdating = true;
  portEXIT_CRITICAL(&researchCaptureMux);

  bool saved = false;
  Preferences p;
  if (p.begin("researchcap", false)) {
    const size_t a = p.putUInt("prems", preMs);
    const size_t b = p.putUInt("postms", postMs);
    saved = (a == sizeof(uint32_t) && b == sizeof(uint32_t));
    p.end();
  }

  portENTER_CRITICAL(&researchCaptureMux);
  if (saved) {
    researchCapturePreWindowMs = preMs;
    researchCapturePostWindowMs = postMs;
    researchCapturePreHead = 0;
    researchCapturePreValidCount = 0;
    researchCapturePreExtHead = 0;
    researchCapturePreExtValidCount = 0;
    researchCaptureLastPreSnapshotMs = 0;
    researchCaptureLastPreExtSnapshotMs = 0;
    memset(researchCapturePreSlots, 0, sizeof(researchCapturePreSlots));
  }
  researchCaptureConfigUpdating = false;
  portEXIT_CRITICAL(&researchCaptureMux);
  return saved;
}

static void researchCaptureReset();
static void researchCaptureReleaseModeBuffers();
static bool researchCaptureEnsureBuffer();

static bool researchCaptureSetMode(uint8_t mode) {
  if (mode != RESEARCH_CAPTURE_MODE_SNAPSHOT && mode != RESEARCH_CAPTURE_MODE_RAW_TRANSITION &&
      mode != RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC) return false;
  uint8_t oldMode;
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING || researchCaptureConfigUpdating || researchCaptureExporting) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }
  oldMode = researchCaptureMode;
  if (mode == oldMode) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return true;
  }
  researchCaptureConfigUpdating = true;
  researchCaptureMode = mode;
  portEXIT_CRITICAL(&researchCaptureMux);

  // Resize the mode-specific buffers first. NVS is updated only after the new
  // allocation succeeds, so an allocation failure cannot persist a mode that
  // would fail again on the next boot.
  researchCaptureReleaseModeBuffers();
  const bool bufferOk = researchCaptureEnsureBuffer();
  bool saved = false;
  if (bufferOk) {
    Preferences p;
    if (p.begin("researchcap", false)) {
      saved = p.putUChar("mode", mode) == sizeof(uint8_t);
      p.end();
    }
  }

  if (!bufferOk || !saved) {
    portENTER_CRITICAL(&researchCaptureMux);
    researchCaptureMode = oldMode;
    portEXIT_CRITICAL(&researchCaptureMux);
    researchCaptureReleaseModeBuffers();
    (void)researchCaptureEnsureBuffer();
  }

  researchCaptureReset();
  portENTER_CRITICAL(&researchCaptureMux);
  researchCaptureConfigUpdating = false;
  if ((!bufferOk || !saved) && !(researchCaptureEntries && researchCapturePreStates))
    researchCaptureState = RESEARCH_CAPTURE_ERROR;
  portEXIT_CRITICAL(&researchCaptureMux);
  return bufferOk && saved;
}

static inline uint16_t researchCaptureStateIndex(uint8_t bus, uint16_t id) {
  return (uint16_t)((bus == RESEARCH_CAPTURE_BUS_VH ? RESEARCH_CAPTURE_IDS_PER_BUS : 0U) + id);
}

static inline uint16_t researchCaptureAge16(uint32_t now, uint32_t then) {
  if (then == 0) return 65535U;
  const uint32_t age = (uint32_t)(now - then);
  return (uint16_t)(age > 65535U ? 65535U : age);
}

static void researchCaptureReleaseModeBuffers() {
  ResearchCaptureEntry *entries = nullptr;
  ResearchCapturePreState *pre = nullptr;
  portENTER_CRITICAL(&researchCaptureMux);
  entries = researchCaptureEntries;
  pre = researchCapturePreStates;
  researchCaptureEntries = nullptr;
  researchCapturePreStates = nullptr;
  researchCaptureAllocatedMainBytes = 0;
  researchCaptureAllocatedAuxBytes = 0;
  researchCaptureUsingPsram = false;
  portEXIT_CRITICAL(&researchCaptureMux);
  if (entries) heap_caps_free(entries);
  if (pre) heap_caps_free(pre);
}

static bool researchCaptureEnsureBuffer() {
  uint8_t mode;
  ResearchCaptureEntry *currentEntries;
  ResearchCapturePreState *currentPre;
  ResearchCaptureLatest *currentLatest;
  uint16_t *currentKnown;
  size_t currentMainBytes, currentAuxBytes;
  portENTER_CRITICAL(&researchCaptureMux);
  mode = researchCaptureMode;
  currentEntries = researchCaptureEntries;
  currentPre = researchCapturePreStates;
  currentLatest = researchCaptureLatest;
  currentKnown = researchCaptureKnownIndices;
  currentMainBytes = researchCaptureAllocatedMainBytes;
  currentAuxBytes = researchCaptureAllocatedAuxBytes;
  portEXIT_CRITICAL(&researchCaptureMux);

  const size_t requiredMainBytes = researchCaptureMainBytesForMode(mode);
  const size_t requiredAuxBytes = researchCaptureAuxBytesForMode(mode);
  if (currentEntries && currentPre && currentLatest && currentKnown &&
      currentMainBytes == requiredMainBytes && currentAuxBytes == requiredAuxBytes) return true;

  // A mode change may require a differently-sized archive/history pair. Detach
  // the old pair under the capture lock, then free it outside the critical section.
  if (currentEntries || currentPre) researchCaptureReleaseModeBuffers();

  ResearchCaptureEntry *entries = (ResearchCaptureEntry *)heap_caps_malloc(
      requiredMainBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  ResearchCapturePreState *pre = (ResearchCapturePreState *)heap_caps_malloc(
      requiredAuxBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  ResearchCaptureLatest *latest = nullptr;
  uint16_t *known = nullptr;
  const bool allocatedLatest = currentLatest == nullptr;
  const bool allocatedKnown = currentKnown == nullptr;
  if (allocatedLatest) latest = (ResearchCaptureLatest *)heap_caps_malloc(
      RESEARCH_CAPTURE_LATEST_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (allocatedKnown) known = (uint16_t *)heap_caps_malloc(
      RESEARCH_CAPTURE_KNOWN_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!entries || !pre || (allocatedLatest && !latest) || (allocatedKnown && !known)) {
    if (entries) heap_caps_free(entries);
    if (pre) heap_caps_free(pre);
    if (latest) heap_caps_free(latest);
    if (known) heap_caps_free(known);
    portENTER_CRITICAL(&researchCaptureMux);
    researchCaptureState = RESEARCH_CAPTURE_ERROR;
    researchCaptureUsingPsram = false;
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }

  memset(entries, 0, requiredMainBytes);
  memset(pre, 0, requiredAuxBytes);
  if (allocatedLatest) memset(latest, 0, RESEARCH_CAPTURE_LATEST_BYTES);
  if (allocatedKnown) memset(known, 0, RESEARCH_CAPTURE_KNOWN_BYTES);

  portENTER_CRITICAL(&researchCaptureMux);
  if (!researchCaptureEntries && !researchCapturePreStates) {
    researchCaptureEntries = entries;
    researchCapturePreStates = pre;
    researchCaptureAllocatedMainBytes = requiredMainBytes;
    researchCaptureAllocatedAuxBytes = requiredAuxBytes;
    entries = nullptr;
    pre = nullptr;
  }
  if (!researchCaptureLatest) {
    researchCaptureLatest = latest;
    latest = nullptr;
  }
  if (!researchCaptureKnownIndices) {
    researchCaptureKnownIndices = known;
    known = nullptr;
  }
  const bool ok = researchCaptureEntries && researchCaptureLatest &&
                  researchCapturePreStates && researchCaptureKnownIndices &&
                  researchCaptureAllocatedMainBytes == requiredMainBytes &&
                  researchCaptureAllocatedAuxBytes == requiredAuxBytes;
  if (ok) {
    researchCaptureUsingPsram = true;
    if (researchCaptureState == RESEARCH_CAPTURE_ERROR) researchCaptureState = RESEARCH_CAPTURE_READY;
  }
  portEXIT_CRITICAL(&researchCaptureMux);

  if (entries) heap_caps_free(entries);
  if (pre) heap_caps_free(pre);
  if (latest) heap_caps_free(latest);
  if (known) heap_caps_free(known);
  return ok;
}

static bool researchCaptureInit() {
  researchCaptureLoadLabels();
  researchCaptureLoadConfig();
  const bool ok = researchCaptureEnsureBuffer();
  uint8_t mode; size_t mainBytes, auxBytes;
  portENTER_CRITICAL(&researchCaptureMux);
  mode = researchCaptureMode;
  mainBytes = researchCaptureAllocatedMainBytes;
  auxBytes = researchCaptureAllocatedAuxBytes;
  portEXIT_CRITICAL(&researchCaptureMux);
  Serial.printf("CAN Research Capture: %s · %s · main=%u KiB · aux=%u KiB · state=%u KiB · snapshot=%lu rows · raw=%lu frames/%u segments\n",
                ok ? "PSRAM READY" : "DISABLED", researchCaptureModeName(mode),
                (unsigned)(mainBytes / 1024U), (unsigned)(auxBytes / 1024U),
                (unsigned)((RESEARCH_CAPTURE_LATEST_BYTES + RESEARCH_CAPTURE_KNOWN_BYTES) / 1024U),
                (unsigned long)RESEARCH_CAPTURE_CAPACITY,
                (unsigned long)RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY, (unsigned)RESEARCH_CAPTURE_RAW_MAX_SEGMENTS);
  return ok;
}

static inline void researchCaptureAppendLocked(uint16_t segment, int16_t relativeMs,
                                           uint16_t id, uint16_t frameAgeMs,
                                           uint8_t bus, uint8_t dlc, uint8_t labelSlot,
                                           uint8_t flags, const uint8_t *data) {
  if (!researchCaptureEntries || researchCaptureCount >= RESEARCH_CAPTURE_CAPACITY) {
    researchCaptureDropped++;
    return;
  }
  ResearchCaptureEntry &e = researchCaptureEntries[researchCaptureCount++];
  e.segment = segment;
  e.relativeMs = relativeMs;
  e.id = id;
  e.frameAgeMs = frameAgeMs;
  e.bus = bus;
  e.dlc = dlc > 8 ? 8 : dlc;
  e.labelSlot = labelSlot;
  e.flags = flags;
  memset(e.data, 0, sizeof(e.data));
  if (data && e.dlc) memcpy(e.data, data, e.dlc);
}

static inline ResearchCapturePreState *researchCapturePreSlotBase(uint8_t slot) {
  return researchCapturePreStates + ((size_t)slot * RESEARCH_CAPTURE_STATE_COUNT);
}

static void researchCaptureFillPreSlotLocked(uint8_t slot, uint32_t now) {
  if (slot >= RESEARCH_CAPTURE_PRE_SLOT_COUNT || !researchCapturePreStates || !researchCaptureLatest || !researchCaptureKnownIndices) return;
  ResearchCapturePreSlot &meta = researchCapturePreSlots[slot];
  ResearchCapturePreState *dst = researchCapturePreSlotBase(slot);
  uint16_t copied = 0;
  const uint16_t knownCount = researchCaptureKnownIds;
  for (uint16_t i = 0; i < knownCount && copied < RESEARCH_CAPTURE_STATE_COUNT; i++) {
    const uint16_t stateIdx = researchCaptureKnownIndices[i];
    if (stateIdx >= RESEARCH_CAPTURE_STATE_COUNT) continue;
    const ResearchCaptureLatest &s = researchCaptureLatest[stateIdx];
    if (!s.valid) continue;
    ResearchCapturePreState &p = dst[copied++];
    p.stateIndex = stateIdx;
    p.frameAgeMs = researchCaptureAge16(now, s.lastSeenMs);
    p.dlc = s.dlc;
    memcpy(p.data, s.data, sizeof(p.data));
  }
  meta.snapshotMs = now;
  meta.count = copied;
  meta.valid = 1;
  meta.txValid = researchCaptureTx3f8Valid;
  meta.txDlc = researchCaptureTx3f8Dlc;
  meta.txOk = researchCaptureTx3f8Ok;
  meta.txAgeMs = researchCaptureTx3f8Valid ? researchCaptureAge16(now, researchCaptureTx3f8Ms) : 65535U;
  memcpy(meta.txData, researchCaptureTx3f8Data, sizeof(meta.txData));
}

static void researchCaptureTakeDensePreSnapshotLocked(uint32_t now) {
  const uint8_t slot = researchCapturePreHead;
  researchCaptureFillPreSlotLocked(slot, now);
  researchCapturePreHead = (uint8_t)((researchCapturePreHead + 1U) % RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT);
  if (researchCapturePreValidCount < RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT) researchCapturePreValidCount++;
  researchCaptureLastPreSnapshotMs = now;
}

static void researchCaptureTakeExtendedPreSnapshotLocked(uint32_t now) {
  const uint8_t slot = (uint8_t)(RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT + researchCapturePreExtHead);
  researchCaptureFillPreSlotLocked(slot, now);
  researchCapturePreExtHead = (uint8_t)((researchCapturePreExtHead + 1U) % RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT);
  if (researchCapturePreExtValidCount < RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT) researchCapturePreExtValidCount++;
  researchCaptureLastPreExtSnapshotMs = now;
}

static uint16_t researchCaptureAppendCurrentSnapshotLocked(uint32_t now, int16_t relativeMs,
     uint16_t segment, uint8_t labelSlot) {
  uint16_t copied = 0;
  const uint16_t knownCount = researchCaptureKnownIds;
  for (uint16_t i = 0; i < knownCount; i++) {
    const uint16_t stateIdx = researchCaptureKnownIndices[i];
    if (stateIdx >= RESEARCH_CAPTURE_STATE_COUNT) continue;
    const ResearchCaptureLatest &s = researchCaptureLatest[stateIdx];
    if (!s.valid) continue;
    const uint8_t bus = stateIdx >= RESEARCH_CAPTURE_IDS_PER_BUS ? RESEARCH_CAPTURE_BUS_VH : RESEARCH_CAPTURE_BUS_PARTY;
    const uint16_t id = (uint16_t)(stateIdx % RESEARCH_CAPTURE_IDS_PER_BUS);
    researchCaptureAppendLocked(segment, relativeMs, id, researchCaptureAge16(now, s.lastSeenMs),
       bus, s.dlc, labelSlot, 0, s.data);
    copied++;
  }
  if (researchCaptureTx3f8Valid) {
    const uint8_t flags = (uint8_t)(RESEARCH_CAPTURE_FLAG_TX | (researchCaptureTx3f8Ok ? RESEARCH_CAPTURE_FLAG_TX_OK : 0));
    researchCaptureAppendLocked(segment, relativeMs, 0x3F8, researchCaptureAge16(now, researchCaptureTx3f8Ms),
       RESEARCH_CAPTURE_BUS_VH, researchCaptureTx3f8Dlc, labelSlot, flags, researchCaptureTx3f8Data);
    copied++;
  }
  return copied;
}

static uint16_t researchCaptureAppendPreSlotLocked(uint8_t slot, uint32_t triggerNow,
      uint16_t segment, uint8_t labelSlot) {
  if (slot >= RESEARCH_CAPTURE_PRE_SLOT_COUNT || !researchCapturePreSlots[slot].valid) return 0;
  const ResearchCapturePreSlot &meta = researchCapturePreSlots[slot];
  const uint32_t age = (uint32_t)(triggerNow - meta.snapshotMs);
  const int16_t relativeMs = -(int16_t)(age > 32767U ? 32767U : age);
  const ResearchCapturePreState *src = researchCapturePreSlotBase(slot);
  uint16_t copied = 0;
  for (uint16_t i = 0; i < meta.count; i++) {
    const ResearchCapturePreState &p = src[i];
    const uint8_t bus = p.stateIndex >= RESEARCH_CAPTURE_IDS_PER_BUS ? RESEARCH_CAPTURE_BUS_VH : RESEARCH_CAPTURE_BUS_PARTY;
    const uint16_t id = (uint16_t)(p.stateIndex % RESEARCH_CAPTURE_IDS_PER_BUS);
    researchCaptureAppendLocked(segment, relativeMs, id, p.frameAgeMs, bus, p.dlc, labelSlot, 0, p.data);
    copied++;
  }
  if (meta.txValid) {
    const uint8_t flags = (uint8_t)(RESEARCH_CAPTURE_FLAG_TX | (meta.txOk ? RESEARCH_CAPTURE_FLAG_TX_OK : 0));
    researchCaptureAppendLocked(segment, relativeMs, 0x3F8, meta.txAgeMs,
       RESEARCH_CAPTURE_BUS_VH, meta.txDlc, labelSlot, flags, meta.txData);
    copied++;
  }
  return copied;
}

static inline ResearchCaptureRawEntry *researchCaptureRawArchiveBase() {
  return reinterpret_cast<ResearchCaptureRawEntry *>(researchCaptureEntries);
}

static inline ResearchCaptureRawEntry *researchCaptureRawPreBase() {
  return reinterpret_cast<ResearchCaptureRawEntry *>(researchCapturePreStates);
}

static bool researchCaptureRawRequestAtLocked(uint8_t labelSlot, uint32_t triggerNow,
                                                   uint32_t captureNow, uint32_t preWindowMs);
static bool researchCaptureRawRequestLocked(uint8_t labelSlot, uint32_t triggerNow, uint32_t preWindowMs);

static inline uint8_t researchCaptureDecodeAlc399(const uint8_t *data, uint8_t dlc) {
  return das399ReadAlcPure(data, dlc);
}

static inline uint32_t researchCaptureRawPreCoverageMsLocked(uint32_t now) {
  if (researchCaptureRawPreFrameCount == 0 || !researchCaptureRawPreBase()) return 0;
  const ResearchCaptureRawEntry &oldest = researchCaptureRawPreBase()[researchCaptureRawPreStartIndex];
  return (uint32_t)(now - oldest.timestampMs);
}

static void researchCaptureRawPreAppendLocked(uint8_t bus, uint16_t id, uint8_t dlc,
                                              const uint8_t *data, uint32_t now) {
  ResearchCaptureRawEntry *pre = researchCaptureRawPreBase();
  if (!pre) return;

  while (researchCaptureRawPreFrameCount > 0) {
    const ResearchCaptureRawEntry &oldest = pre[researchCaptureRawPreStartIndex];
    if ((uint32_t)(now - oldest.timestampMs) <= RESEARCH_CAPTURE_RAW_MANUAL_PRE_MS) break;
    researchCaptureRawPreStartIndex = (researchCaptureRawPreStartIndex + 1U) % RESEARCH_CAPTURE_RAW_PRE_CAPACITY;
    researchCaptureRawPreFrameCount--;
    researchCaptureRawEvicted++;
  }
  if (researchCaptureRawPreFrameCount >= RESEARCH_CAPTURE_RAW_PRE_CAPACITY) {
    researchCaptureRawPreStartIndex = (researchCaptureRawPreStartIndex + 1U) % RESEARCH_CAPTURE_RAW_PRE_CAPACITY;
    researchCaptureRawPreFrameCount--;
    researchCaptureRawEvicted++;
  }

  const uint32_t idx = (researchCaptureRawPreStartIndex + researchCaptureRawPreFrameCount) % RESEARCH_CAPTURE_RAW_PRE_CAPACITY;
  ResearchCaptureRawEntry &e = pre[idx];
  e.timestampMs = now;
  e.id = id;
  e.bus = bus;
  e.dlc = dlc > 8 ? 8 : dlc;
  memset(e.data, 0, sizeof(e.data));
  if (data && e.dlc) memcpy(e.data, data, e.dlc);
  researchCaptureRawPreFrameCount++;

  if (researchCaptureRawPreFrameCount > 0) {
    researchCaptureRawFirstFrameMs = pre[researchCaptureRawPreStartIndex].timestampMs;
  }
}

static bool researchCaptureRawArchiveAppendLocked(const ResearchCaptureRawEntry &src) {
  ResearchCaptureRawEntry *archive = researchCaptureRawArchiveBase();
  if (!archive || researchCaptureRawArchiveCount >= RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) {
    researchCaptureDropped++;
    researchCaptureState = RESEARCH_CAPTURE_FULL;
    return false;
  }
  archive[researchCaptureRawArchiveCount++] = src;
  researchCaptureCount = researchCaptureRawArchiveCount;
  if (researchCaptureSegment > 0 && researchCaptureSegment <= RESEARCH_CAPTURE_MAX_SEGMENTS) {
    researchCaptureSegments[researchCaptureSegment - 1U].rawFrameCount++;
  }
  return true;
}

static uint32_t researchCaptureRawCopyPreLocked(uint32_t triggerNow, uint32_t preWindowMs) {
  ResearchCaptureRawEntry *pre = researchCaptureRawPreBase();
  ResearchCaptureRawEntry *archive = researchCaptureRawArchiveBase();
  if (!pre || researchCaptureRawPreFrameCount == 0) return 0;
  if (!archive) {
    researchCaptureDropped++;
    researchCaptureState = RESEARCH_CAPTURE_ERROR;
    return 0;
  }

  const uint32_t available = researchCaptureRawArchiveCount < RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY
      ? RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY - researchCaptureRawArchiveCount : 0;
  const ResearchRawCopyPlan plan = researchRawCopyPlanPure(
      pre, RESEARCH_CAPTURE_RAW_PRE_CAPACITY, researchCaptureRawPreStartIndex,
      researchCaptureRawPreFrameCount, triggerNow, preWindowMs, available);
  if (plan.count == 0) {
    if (plan.truncated) {
      researchCaptureDropped++;
      researchCaptureState = RESEARCH_CAPTURE_FULL;
    }
    return 0;
  }

  const uint32_t contiguous = RESEARCH_CAPTURE_RAW_PRE_CAPACITY - plan.ringStart;
  const uint32_t firstCount = plan.count < contiguous ? plan.count : contiguous;
  memcpy(archive + researchCaptureRawArchiveCount, pre + plan.ringStart,
         firstCount * sizeof(ResearchCaptureRawEntry));
  const uint32_t secondCount = plan.count - firstCount;
  if (secondCount) {
    memcpy(archive + researchCaptureRawArchiveCount + firstCount, pre,
           secondCount * sizeof(ResearchCaptureRawEntry));
  }
  researchCaptureRawArchiveCount += plan.count;
  researchCaptureCount = researchCaptureRawArchiveCount;
  if (researchCaptureSegment > 0 && researchCaptureSegment <= RESEARCH_CAPTURE_MAX_SEGMENTS) {
    researchCaptureSegments[researchCaptureSegment - 1U].rawFrameCount += plan.count;
  }
  if (plan.truncated) {
    researchCaptureDropped++;
    researchCaptureState = RESEARCH_CAPTURE_FULL;
  }
  return plan.count;
}

static bool researchCaptureAutoLaneQualifiedLocked(uint32_t now) {
  if (!researchCaptureLatest) return false;
  const ResearchCaptureLatest &lane = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x239)];
  if (!lane.valid || lane.dlc < 7 || lane.lastSeenMs == 0 ||
      (uint32_t)(now - lane.lastSeenMs) > RESEARCH_CAPTURE_AUTO_LANE_FRESH_MS) return false;
  const DasLane239Decoded decoded = dasLane239DecodePure(lane.data, lane.dlc);
  return dasLeftResearchLaneQualifiedPure(decoded);
}

static void researchCaptureAutoMaybeTriggerLocked(uint8_t bus, uint16_t id, uint8_t dlc,
                                                   const uint8_t *data, uint32_t now) {
  if (researchCaptureMode != RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC || bus != RESEARCH_CAPTURE_BUS_PARTY ||
      id != 0x399 || !data || dlc < 7) return;
  if (researchCaptureExporting) {
    researchCaptureAutoRejectBusy++;
    return;
  }

  const uint8_t alc = researchCaptureDecodeAlc399(data, dlc);
  const uint8_t previousStableAlc = researchCaptureAutoPersistence.stableAlc;
  const ResearchAlcPersistenceResult persistence = researchAlcPersistenceStepPure(
      researchCaptureAutoPersistence, alc, now, RESEARCH_CAPTURE_AUTO_PERSIST_MS);
  const ResearchAlcTransitionKind transition = persistence.kind;
  if (transition == RESEARCH_ALC_NONE) return;
  const uint32_t transitionMs = persistence.transitionMs;
  const uint32_t confirmationDelayMs = (uint32_t)(now - transitionMs);
  const bool closeEvent = transition == RESEARCH_ALC_CLOSE;

  // OPEN must be corroborated by the current 0x239 LEFT lane. BLOCKED events
  // deliberately bypass this current-lane qualification: if 0x239 drops or changes
  // line usage at the same moment as the ALC block, rejecting here would hide the
  // transition we are trying to diagnose. The previous stable ALC state being 6/8
  // already proves LEFT was planner-available before a CLOSE event.
  if (!closeEvent && !researchCaptureAutoLaneQualifiedLocked(now)) {
    researchCaptureAutoRejectLane++;
    return;
  }
  researchCaptureAutoQualifiedTransitions++;
  researchCaptureAutoLastEvent = closeEvent ? 1 : 2;
  researchCaptureAutoLastFrom = previousStableAlc;
  researchCaptureAutoLastTo = alc;

  // A second qualified LEFT open/close event inside the same episode extends POST instead
  // of consuming another segment. The raw 0x399 frame itself is the event marker.
  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING && researchCaptureRawTriggered) {
    if (researchCaptureRawLabelSlot < RESEARCH_CAPTURE_LABEL_C) {
      const uint32_t candidateDeadline = transitionMs + RESEARCH_CAPTURE_RAW_POST_MS;
      if ((int32_t)(candidateDeadline - researchCaptureRawPostDeadlineMs) > 0) {
        researchCaptureRawPostDeadlineMs = candidateDeadline;
      }
      researchCaptureAutoMergedEvents++;
      if (researchCaptureSegment > 0 && researchCaptureSegment <= RESEARCH_CAPTURE_MAX_SEGMENTS) {
        researchCaptureSegments[researchCaptureSegment - 1U].rawEventCount++;
      }
    } else {
      // A manual C/D marker is occupying the single raw archive writer. Do not
      // relabel/extend that segment as an AUTO episode; record this as busy.
      researchCaptureAutoRejectBusy++;
    }
    return;
  }

  const uint32_t requiredAutoCoverageMs = RESEARCH_CAPTURE_RAW_AUTO_PRE_MS + confirmationDelayMs;
  if (researchCaptureRawPreCoverageMsLocked(now) < requiredAutoCoverageMs) {
    researchCaptureAutoRejectWarmup++;
    return;
  }
  if (researchCaptureState != RESEARCH_CAPTURE_READY || researchCaptureRawTriggered) {
    researchCaptureAutoRejectBusy++;
    return;
  }
  if (researchCaptureSegment >= RESEARCH_CAPTURE_RAW_MAX_SEGMENTS ||
      researchCaptureRawArchiveCount >= RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) {
    researchCaptureState = RESEARCH_CAPTURE_FULL;
    researchCaptureAutoRejectBusy++;
    return;
  }

  const uint8_t labelSlot = closeEvent ? RESEARCH_CAPTURE_LABEL_B : RESEARCH_CAPTURE_LABEL_A;
  researchCaptureRequests++;
  if (researchCaptureRawRequestAtLocked(labelSlot, transitionMs, now, RESEARCH_CAPTURE_RAW_AUTO_PRE_MS)) {
    researchCaptureAutoTriggerCount++;
    researchCaptureAutoLastTriggerMs = transitionMs;
    if (closeEvent) researchCaptureAutoBlockedCount++;
    else researchCaptureAutoOpenCount++;
    if (researchCaptureSegment > 0 && researchCaptureSegment <= RESEARCH_CAPTURE_MAX_SEGMENTS) {
      ResearchCaptureSegmentMeta &meta = researchCaptureSegments[researchCaptureSegment - 1U];
      meta.triggerAlcFrom = previousStableAlc;
      meta.triggerAlcTo = alc;
      meta.triggerDasState = readDASState4(data);
      if (researchCaptureLatest) {
        const ResearchCaptureLatest &lane = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x239)];
        if (lane.valid && lane.dlc >= 7) {
          const DasLane239Decoded decoded = dasLane239DecodePure(lane.data, lane.dlc);
          meta.triggerLeftLaneExists = decoded.leftLaneExists ? 1U : 0U;
          meta.triggerLeftLineUsage = decoded.leftLineUsage;
          meta.triggerRightLaneExists = decoded.rightLaneExists ? 1U : 0U;
        }
        const ResearchCaptureLatest &road = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_VH, 0x238)];
        if (road.valid && road.dlc >= 5 && road.lastSeenMs != 0) {
          const RoadContext238Pure decodedRoad = decodeRoadContext238Pure(road.data, road.dlc);
          if (decodedRoad.valid) {
            meta.triggerRoadClass = decodedRoad.roadClass;
            meta.triggerGpsRoadMatch = decodedRoad.gpsRoadMatch ? 1U : 0U;
            meta.triggerNavRouteActive = decodedRoad.navRouteActive ? 1U : 0U;
            meta.triggerControlledAccess = decodedRoad.controlledAccess ? 1U : 0U;
            meta.triggerLeftOffRamp = decodedRoad.nextBranchLeftOffRamp ? 1U : 0U;
            meta.triggerRightOffRamp = decodedRoad.nextBranchRightOffRamp ? 1U : 0U;
            meta.triggerRoadAgeMs = (uint32_t)(now - road.lastSeenMs);
          }
        }
      }
    }
  } else {
    researchCaptureIgnored++;
    researchCaptureAutoRejectBusy++;
  }
}

static void researchCaptureRawObserveLocked(uint8_t bus, uint16_t id, uint8_t dlc,
                                            const uint8_t *data, uint32_t now) {
  if (!researchCaptureModeIsRaw(researchCaptureMode) || !researchCaptureEntries || !researchCapturePreStates) return;
  if (researchCaptureState == RESEARCH_CAPTURE_ERROR || researchCaptureState == RESEARCH_CAPTURE_FULL) return;

  // Inspect the current 0x399 before adding it to PRE. AUTO persistence can
  // confirm a transition after t=0; the delayed request copies the intervening
  // rolling-PRE traffic and preserves the original transition timestamp.
  researchCaptureAutoMaybeTriggerLocked(bus, id, dlc, data, now);
  researchCaptureRawPreAppendLocked(bus, id, dlc, data, now);

  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING && researchCaptureRawTriggered) {
    ResearchCaptureRawEntry e = {};
    e.timestampMs = now;
    e.id = id;
    e.bus = bus;
    e.dlc = dlc > 8 ? 8 : dlc;
    if (data && e.dlc) memcpy(e.data, data, e.dlc);
    researchCaptureRawArchiveAppendLocked(e);
  }
}

static bool researchCaptureRawRequestAtLocked(uint8_t labelSlot, uint32_t triggerNow,
                                                   uint32_t captureNow, uint32_t preWindowMs) {
  if (researchCaptureExporting || researchCaptureState != RESEARCH_CAPTURE_READY || researchCaptureRawTriggered) return false;
  if (labelSlot >= RESEARCH_CAPTURE_LABEL_SLOTS) return false;
  if (researchCaptureSegment >= RESEARCH_CAPTURE_RAW_MAX_SEGMENTS ||
      researchCaptureRawArchiveCount >= RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) {
    researchCaptureState = RESEARCH_CAPTURE_FULL;
    return false;
  }

  researchCaptureSegment++;
  const uint16_t segment = researchCaptureSegment;
  ResearchCaptureSegmentMeta &meta = researchCaptureSegments[segment - 1U];
  memset(&meta, 0, sizeof(meta));
  meta.triggerAlcFrom = 0xFF;
  meta.triggerAlcTo = 0xFF;
  meta.triggerLeftLaneExists = 0xFF;
  meta.triggerLeftLineUsage = 0xFF;
  meta.triggerRightLaneExists = 0xFF;
  meta.triggerDasState = 0xFF;
  meta.triggerRoadClass = 0xFF;
  meta.triggerGpsRoadMatch = 0xFF;
  meta.triggerNavRouteActive = 0xFF;
  meta.triggerControlledAccess = 0xFF;
  meta.triggerLeftOffRamp = 0xFF;
  meta.triggerRightOffRamp = 0xFF;
  meta.triggerRoadAgeMs = 0xFFFFFFFFUL;
  meta.triggerMs = triggerNow;
  meta.rawStartIndex = researchCaptureRawArchiveCount;
  meta.rawEventCount = 1;
  meta.labelSlot = labelSlot;
  strncpy(meta.label, researchCaptureLabels[labelSlot], RESEARCH_CAPTURE_LABEL_BYTES - 1);
  meta.label[RESEARCH_CAPTURE_LABEL_BYTES - 1] = '\0';

  researchCaptureCurrentLabelSlot = labelSlot;
  researchCaptureLastLabelSlot = labelSlot;
  researchCaptureStartMs = triggerNow;
  researchCaptureRawTriggerMs = triggerNow;
  researchCaptureRawPostDeadlineMs = triggerNow + RESEARCH_CAPTURE_RAW_POST_MS;
  researchCaptureRawTriggered = true;
  researchCaptureRawLabelSlot = labelSlot;
  researchCaptureState = RESEARCH_CAPTURE_CAPTURING;

  // AUTO persistence confirms the event after t0. Copy PRE through confirmation
  // while widening the copy window by the same delay, preserving exactly the
  // requested preWindowMs before the original transition and the first 300 ms
  // of post-transition traffic that accumulated in the rolling PRE ring.
  const uint32_t confirmationDelayMs = (uint32_t)(captureNow - triggerNow);
  researchCaptureRawCopyPreLocked(captureNow, preWindowMs + confirmationDelayMs);
  if (researchCaptureState == RESEARCH_CAPTURE_FULL) return false;
  return true;
}

static bool researchCaptureRawRequestLocked(uint8_t labelSlot, uint32_t triggerNow, uint32_t preWindowMs) {
  return researchCaptureRawRequestAtLocked(labelSlot, triggerNow, triggerNow, preWindowMs);
}

static void researchCaptureTick(uint32_t now) {
  if (!researchCaptureEntries || !researchCaptureLatest || !researchCapturePreStates || !researchCaptureKnownIndices) return;
  portENTER_CRITICAL(&researchCaptureMux);
  // A mode change can detach/free the mode-specific buffers between the fast
  // pointer check above and this lock acquisition. Revalidate under the lock
  // before dereferencing either archive/history block.
  if (researchCaptureConfigUpdating || !researchCaptureEntries || !researchCaptureLatest ||
      !researchCapturePreStates || !researchCaptureKnownIndices) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return;
  }

  if (researchCaptureModeIsRaw(researchCaptureMode)) {
    if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING && researchCaptureRawTriggered &&
        (int32_t)(now - researchCaptureRawPostDeadlineMs) >= 0) {
      researchCaptureCompletedSegments++;
      researchCaptureRawTriggered = false;
      researchCaptureRawLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
      researchCaptureCurrentLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
      researchCaptureStartMs = 0;
      researchCaptureRawPostDeadlineMs = 0;
      if (researchCaptureSegment >= RESEARCH_CAPTURE_RAW_MAX_SEGMENTS ||
          researchCaptureRawArchiveCount >= RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) {
        researchCaptureState = RESEARCH_CAPTURE_FULL;
      } else {
        // Re-arm immediately. The independent PRE ring kept recording throughout
        // the completed segment, so the next transition does not require Reset.
        researchCaptureState = RESEARCH_CAPTURE_READY;
      }
    }
    portEXIT_CRITICAL(&researchCaptureMux);
    return;
  }

  if (researchCaptureLastPreSnapshotMs == 0 ||
      (uint32_t)(now - researchCaptureLastPreSnapshotMs) >= RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS) {
    researchCaptureTakeDensePreSnapshotLocked(now);
  }

  const uint32_t extInterval = researchCaptureExtendedIntervalMs(researchCapturePreWindowMs);
  if (extInterval != 0 && (researchCaptureLastPreExtSnapshotMs == 0 ||
      (uint32_t)(now - researchCaptureLastPreExtSnapshotMs) >= extInterval)) {
    researchCaptureTakeExtendedPreSnapshotLocked(now);
  }

  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING) {
    const uint32_t elapsed = (uint32_t)(now - researchCaptureStartMs);
    const uint8_t postCount = researchCapturePostCountForWindow(researchCapturePostWindowMs);
    while (researchCaptureNextPostIndex < postCount &&
 elapsed >= RESEARCH_CAPTURE_POST_TARGETS[researchCaptureNextPostIndex]) {
      const uint16_t target = RESEARCH_CAPTURE_POST_TARGETS[researchCaptureNextPostIndex];
      researchCaptureAppendCurrentSnapshotLocked(now, (int16_t)target,
    researchCaptureSegment, researchCaptureCurrentLabelSlot);
      researchCaptureLastPostSnapshotsCopied++;
      researchCaptureNextPostIndex++;
      if (researchCaptureCount >= RESEARCH_CAPTURE_CAPACITY) {
        researchCaptureDropped++;
        researchCaptureState = RESEARCH_CAPTURE_FULL;
        break;
      }
    }
    if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING && researchCaptureNextPostIndex >= postCount) {
      researchCaptureCompletedSegments++;
      researchCaptureState = RESEARCH_CAPTURE_PAUSED;
    }
  }

  portEXIT_CRITICAL(&researchCaptureMux);
}

static bool researchCaptureRequest(uint8_t labelSlot) {
  if (!labMenuEnabled) return false;
  if (!researchCaptureEnsureBuffer()) return false;
  if (labelSlot >= RESEARCH_CAPTURE_LABEL_SLOTS) return false;
  const uint32_t triggerNow = (uint32_t)millis();
  researchCaptureTick(triggerNow);

  portENTER_CRITICAL(&researchCaptureMux);
  researchCaptureRequests++;
  if (researchCaptureExporting) {
    researchCaptureIgnored++;
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }
  if (researchCaptureModeIsRaw(researchCaptureMode)) {
    // RAW_AUTO_ALC owns A/B so manual markers cannot overwrite the semantic
    // meaning of automatic LEFT_OPEN / LEFT_BLOCKED segments. C/D remain
    // available as manual research markers and always use the 5 s manual PRE.
    if (researchCaptureMode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC && labelSlot < RESEARCH_CAPTURE_LABEL_C) {
      researchCaptureIgnored++;
      portEXIT_CRITICAL(&researchCaptureMux);
      return false;
    }
    if (researchCaptureConfigUpdating ||
        !researchCaptureRawRequestLocked(labelSlot, triggerNow, RESEARCH_CAPTURE_RAW_MANUAL_PRE_MS)) {
      researchCaptureIgnored++;
      portEXIT_CRITICAL(&researchCaptureMux);
      return false;
    }
    portEXIT_CRITICAL(&researchCaptureMux);
    return true;
  }
  if (researchCaptureState == RESEARCH_CAPTURE_CAPTURING || researchCaptureConfigUpdating) {
    researchCaptureIgnored++;
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }
  if (researchCaptureSegment >= RESEARCH_CAPTURE_MAX_SEGMENTS || researchCaptureCount >= RESEARCH_CAPTURE_CAPACITY) {
    researchCaptureState = RESEARCH_CAPTURE_FULL;
    researchCaptureIgnored++;
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }

  const uint8_t postCount = researchCapturePostCountForWindow(researchCapturePostWindowMs);
  const uint32_t rowsPerSnapshot = (uint32_t)researchCaptureKnownIds + 65U;
  const uint32_t preReady = (uint32_t)researchCapturePreValidCount + (uint32_t)researchCapturePreExtValidCount;
  const uint32_t required = rowsPerSnapshot * (preReady + 1U + postCount);
  if ((uint32_t)researchCaptureCount + required > RESEARCH_CAPTURE_CAPACITY) {
    researchCaptureState = RESEARCH_CAPTURE_FULL;
    researchCaptureIgnored++;
    portEXIT_CRITICAL(&researchCaptureMux);
    return false;
  }

  researchCaptureSegment++;
  const uint16_t segment = researchCaptureSegment;
  researchCaptureCurrentLabelSlot = labelSlot;
  researchCaptureLastLabelSlot = labelSlot;
  researchCaptureStartMs = triggerNow;
  researchCaptureNextPostIndex = 0;
  researchCaptureLastPreSnapshotsCopied = 0;
  researchCaptureLastPostSnapshotsCopied = 0;
  researchCaptureLastTriggerRows = 0;
  researchCaptureSegments[segment - 1].triggerMs = triggerNow;
  researchCaptureSegments[segment - 1].triggerAlcFrom = 0xFF;
  researchCaptureSegments[segment - 1].triggerAlcTo = 0xFF;
  researchCaptureSegments[segment - 1].triggerLeftLaneExists = 0xFF;
  researchCaptureSegments[segment - 1].triggerLeftLineUsage = 0xFF;
  researchCaptureSegments[segment - 1].triggerRightLaneExists = 0xFF;
  researchCaptureSegments[segment - 1].triggerDasState = 0xFF;
  researchCaptureSegments[segment - 1].triggerRoadClass = 0xFF;
  researchCaptureSegments[segment - 1].triggerGpsRoadMatch = 0xFF;
  researchCaptureSegments[segment - 1].triggerNavRouteActive = 0xFF;
  researchCaptureSegments[segment - 1].triggerControlledAccess = 0xFF;
  researchCaptureSegments[segment - 1].triggerLeftOffRamp = 0xFF;
  researchCaptureSegments[segment - 1].triggerRightOffRamp = 0xFF;
  researchCaptureSegments[segment - 1].triggerRoadAgeMs = 0xFFFFFFFFUL;
  researchCaptureSegments[segment - 1].labelSlot = labelSlot;
  strncpy(researchCaptureSegments[segment - 1].label, researchCaptureLabels[labelSlot], RESEARCH_CAPTURE_LABEL_BYTES - 1);
  researchCaptureSegments[segment - 1].label[RESEARCH_CAPTURE_LABEL_BYTES - 1] = '\0';

  const uint32_t preWindow = researchCapturePreWindowMs;
  if (preWindow > RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS && researchCapturePreExtValidCount) {
    const uint8_t validExt = researchCapturePreExtValidCount;
    const uint8_t oldestExt = (uint8_t)((researchCapturePreExtHead + RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT - validExt) % RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT);
    for (uint8_t i = 0; i < validExt; i++) {
      const uint8_t ring = (uint8_t)((oldestExt + i) % RESEARCH_CAPTURE_PRE_EXT_SLOT_COUNT);
      const uint8_t slot = (uint8_t)(RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT + ring);
      if (!researchCapturePreSlots[slot].valid) continue;
      const uint32_t age = (uint32_t)(triggerNow - researchCapturePreSlots[slot].snapshotMs);
      if (age == 0 || age <= RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS || age > preWindow + RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS) continue;
      researchCaptureAppendPreSlotLocked(slot, triggerNow, segment, labelSlot);
      researchCaptureLastPreSnapshotsCopied++;
    }
  }

  const uint8_t validDense = researchCapturePreValidCount;
  const uint8_t oldestDense = (uint8_t)((researchCapturePreHead + RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT - validDense) % RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT);
  const uint32_t denseLimit = preWindow < RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS ? preWindow : RESEARCH_CAPTURE_PRE_DENSE_HORIZON_MS;
  for (uint8_t i = 0; i < validDense; i++) {
    const uint8_t slot = (uint8_t)((oldestDense + i) % RESEARCH_CAPTURE_PRE_DENSE_SLOT_COUNT);
    if (!researchCapturePreSlots[slot].valid) continue;
    const uint32_t age = (uint32_t)(triggerNow - researchCapturePreSlots[slot].snapshotMs);
    if (age == 0 || age > denseLimit + RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS) continue;
    researchCaptureAppendPreSlotLocked(slot, triggerNow, segment, labelSlot);
    researchCaptureLastPreSnapshotsCopied++;
  }

  researchCaptureLastTriggerRows = researchCaptureAppendCurrentSnapshotLocked(triggerNow, 0, segment, labelSlot);
  researchCaptureState = researchCaptureCount >= RESEARCH_CAPTURE_CAPACITY ? RESEARCH_CAPTURE_FULL : RESEARCH_CAPTURE_CAPTURING;
  const bool accepted = researchCaptureState == RESEARCH_CAPTURE_CAPTURING;
  if (!accepted) researchCaptureIgnored++;
  portEXIT_CRITICAL(&researchCaptureMux);
  return accepted;
}

static void researchCaptureObserve(uint8_t bus, uint16_t id, uint8_t dlc, const uint8_t *data) {
  if (!labMenuEnabled) return;
  if (!data || id > 0x7FFU || bus > RESEARCH_CAPTURE_BUS_VH) return;
  if (!researchCaptureLatest || !researchCaptureKnownIndices) return;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  const uint16_t stateIdx = researchCaptureStateIndex(bus, id);

  portENTER_CRITICAL(&researchCaptureMux);
  // Mode changes resize/free the archive/history blocks outside the critical
  // section. Skip this frame while that transition is in progress and recheck
  // the pointers under the lock to close the fast-check race above.
  if (researchCaptureConfigUpdating || !researchCaptureEntries || !researchCapturePreStates ||
      !researchCaptureLatest || !researchCaptureKnownIndices) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return;
  }
  // Timestamp under the capture lock so Party/VH task preemption cannot append
  // out-of-order RAW entries and violate the bulk-copy planner's chronology.
  const uint32_t now = (uint32_t)millis();
  const bool researchCaptureRawTracksLatest = !researchCaptureModeIsRaw(researchCaptureMode) ||
      (bus == RESEARCH_CAPTURE_BUS_PARTY && (id == 0x239 || id == 0x399));
  if (researchCaptureRawTracksLatest) {
    ResearchCaptureLatest &s = researchCaptureLatest[stateIdx];
    if (!s.valid) {
      if (researchCaptureKnownIds < RESEARCH_CAPTURE_STATE_COUNT)
        researchCaptureKnownIndices[researchCaptureKnownIds++] = stateIdx;
    }
    s.lastSeenMs = now;
    s.dlc = n;
    s.valid = 1;
    if (n == sizeof(s.data)) {
      memcpy(s.data, data, sizeof(s.data));
    } else {
      memset(s.data, 0, sizeof(s.data));
      if (n) memcpy(s.data, data, n);
    }
  }
  researchCaptureRawObserveLocked(bus, id, n, data, now);
  portEXIT_CRITICAL(&researchCaptureMux);
}

static inline void researchCaptureObserveParty(uint16_t id, uint8_t dlc, const uint8_t *data) {
  researchCaptureObserve(RESEARCH_CAPTURE_BUS_PARTY, id, dlc, data);
}

static inline void researchCaptureObserveVh(uint16_t id, uint8_t dlc, const uint8_t *data) {
  researchCaptureObserve(RESEARCH_CAPTURE_BUS_VH, id, dlc, data);
}

static inline void researchCaptureObserveTxVh(uint16_t id, uint8_t dlc, const uint8_t *data, bool txOk) {
  if (!labMenuEnabled) return;
  if (id != 0x3F8 || !data) return;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  portENTER_CRITICAL(&researchCaptureMux);
  researchCaptureTx3f8Ms = (uint32_t)millis();
  researchCaptureTx3f8Dlc = n;
  researchCaptureTx3f8Ok = txOk ? 1 : 0;
  researchCaptureTx3f8Valid = 1;
  memset(researchCaptureTx3f8Data, 0, sizeof(researchCaptureTx3f8Data));
  if (n) memcpy(researchCaptureTx3f8Data, data, n);
  portEXIT_CRITICAL(&researchCaptureMux);
}

static inline void researchCaptureObserveTxParty(uint16_t id, uint8_t dlc,
                                                  const uint8_t *data, bool txOk) {
  if (!labMenuEnabled) return;
  if (id != 0x399 || !data) return;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureConfigUpdating || !researchCaptureEntries || !researchCapturePreStates) {
    portEXIT_CRITICAL(&researchCaptureMux);
    return;
  }
  const uint32_t now = (uint32_t)millis();
  researchCaptureRawObserveLocked(txOk ? RESEARCH_CAPTURE_BUS_PARTY_TX_OK
                                       : RESEARCH_CAPTURE_BUS_PARTY_TX_FAIL,
                                  id, n, data, now);
  portEXIT_CRITICAL(&researchCaptureMux);
}

static void researchCaptureReset() {
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureExporting) {
    researchCaptureIgnored++;
    portEXIT_CRITICAL(&researchCaptureMux);
    return;
  }
  researchCaptureCount = 0;
  researchCaptureSegment = 0;
  researchCaptureCompletedSegments = 0;
  researchCaptureCurrentLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
  researchCaptureLastLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
  researchCaptureStartMs = 0;
  researchCaptureNextPostIndex = 0;
  researchCaptureRequests = 0;
  researchCaptureIgnored = 0;
  researchCaptureDropped = 0;
  researchCaptureRawArchiveCount = 0;
  researchCaptureRawPreStartIndex = 0;
  researchCaptureRawPreFrameCount = 0;
  researchCaptureRawTriggerMs = 0;
  researchCaptureRawPostDeadlineMs = 0;
  researchCaptureRawEvicted = 0;
  researchCaptureRawTriggered = false;
  researchCaptureRawLabelSlot = RESEARCH_CAPTURE_LABEL_NONE;
  researchCaptureRawFirstFrameMs = 0;
  researchCaptureAutoPersistence = researchAlcPersistenceInitialPure();
  researchCaptureAutoLastEvent = 0;
  researchCaptureAutoLastFrom = 0xFF;
  researchCaptureAutoLastTo = 0xFF;
  researchCaptureAutoQualifiedTransitions = 0;
  researchCaptureAutoTriggerCount = 0;
  researchCaptureAutoRejectLane = 0;
  researchCaptureAutoRejectWarmup = 0;
  researchCaptureAutoRejectBusy = 0;
  researchCaptureAutoMergedEvents = 0;
  researchCaptureAutoOpenCount = 0;
  researchCaptureAutoBlockedCount = 0;
  researchCaptureAutoLastTriggerMs = 0;
  researchCapturePreHead = 0;
  researchCapturePreValidCount = 0;
  researchCapturePreExtHead = 0;
  researchCapturePreExtValidCount = 0;
  researchCaptureLastPreSnapshotMs = 0;
  researchCaptureLastPreExtSnapshotMs = 0;
  researchCaptureConfigUpdating = false;
  researchCaptureLastPreSnapshotsCopied = 0;
  researchCaptureLastPostSnapshotsCopied = 0;
  researchCaptureLastTriggerRows = 0;
  memset(researchCapturePreSlots, 0, sizeof(researchCapturePreSlots));
  memset(researchCaptureSegments, 0, sizeof(researchCaptureSegments));
  // Keep latest RX and latest 0x3F8 TX state warm; the rolling pre-snapshot
  // history starts fresh after reset.
  researchCaptureState = (researchCaptureEntries && researchCaptureLatest && researchCapturePreStates && researchCaptureKnownIndices)
                  ? RESEARCH_CAPTURE_READY : RESEARCH_CAPTURE_ERROR;
  portEXIT_CRITICAL(&researchCaptureMux);
}
