#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "driver_monitor_capture_pure.h"

static constexpr uint16_t DRIVER_MONITOR_ID_STATUS2 = 0x389;
static constexpr uint16_t DRIVER_MONITOR_ID_CARLOG = 0x5D9;
static constexpr uint16_t DRIVER_MONITOR_ID_AUTOPILOT_DEBUG = 0x247;
static constexpr uint16_t DRIVER_MONITOR_ID_DAS_STATUS = 0x399;
static constexpr uint16_t DRIVER_MONITOR_ID_EPAS_STATUS = 0x370;
static constexpr uint32_t DRIVER_MONITOR_PRE_MS = 2000;
static constexpr uint32_t DRIVER_MONITOR_POST_MS = 5000;
static constexpr uint16_t DRIVER_MONITOR_PRE_CAPACITY = 512;
static constexpr uint16_t DRIVER_MONITOR_ARCHIVE_CAPACITY = 2048;
static constexpr uint8_t DRIVER_MONITOR_MAX_SEGMENTS = 12;
static constexpr uint8_t DRIVER_MONITOR_LABEL_COUNT = 6;
static constexpr uint8_t DRIVER_MONITOR_LABEL_NONE = 0xFF;
static constexpr uint8_t DRIVER_MONITOR_BUS_A = driver_monitor_capture_pure::BUS_A;
static constexpr uint8_t DRIVER_MONITOR_BUS_B = driver_monitor_capture_pure::BUS_B;
static constexpr uint8_t DRIVER_MONITOR_BUS_UNKNOWN = 0xFF;

struct DriverMonitorSample {
  uint32_t ms;
  uint16_t id;
  uint8_t bus;
  uint8_t dlc;
  uint8_t data[8];
};

struct DriverMonitorRecord {
  DriverMonitorSample sample;
  int32_t relativeMs;
  uint8_t label;
  uint8_t segment;
};

struct DriverMonitorSegment {
  uint32_t triggerMs;
  uint16_t startIndex;
  uint16_t count;
  uint8_t label;
  uint8_t number;
  uint8_t complete;
  uint8_t aborted;
};

static portMUX_TYPE driverMonitorCaptureMux = portMUX_INITIALIZER_UNLOCKED;
static DriverMonitorSample *driverMonitorPreRing = nullptr;
static DriverMonitorRecord *driverMonitorArchive = nullptr;
static DriverMonitorSegment driverMonitorSegments[DRIVER_MONITOR_MAX_SEGMENTS] = {};
static volatile uint16_t driverMonitorPreHead = 0;
static volatile uint16_t driverMonitorPreCount = 0;
static volatile uint16_t driverMonitorArchiveCount = 0;
static volatile uint8_t driverMonitorSegmentCount = 0;
static volatile uint8_t driverMonitorCurrentSegment = 0xFF;
static volatile uint8_t driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
static volatile bool driverMonitorCapturing = false;
static volatile bool driverMonitorExporting = false;
static volatile bool driverMonitorInitError = false;
static volatile bool driverMonitorUsingPsram = false;
static volatile uint32_t driverMonitorTriggerMs = 0;
static volatile uint32_t driverMonitorPostDeadlineMs = 0;
static volatile uint32_t driverMonitorRequests = 0;
static volatile uint32_t driverMonitorIgnored = 0;
static volatile uint32_t driverMonitorDropped = 0;

static volatile uint32_t driverMonitor389Ms = 0;
static volatile uint32_t driverMonitor5d9Ms = 0;
static volatile uint32_t driverMonitor247Ms = 0;
static volatile uint32_t driverMonitor399Ms = 0;
static volatile uint32_t driverMonitor370Ms = 0;
static volatile uint32_t driverMonitor389Count = 0;
static volatile uint32_t driverMonitor5d9Count = 0;
static volatile uint32_t driverMonitor247Count = 0;
static volatile uint32_t driverMonitor399Count = 0;
static volatile uint32_t driverMonitor370Count = 0;
static volatile uint8_t driverMonitor389Dlc = 0;
static volatile uint8_t driverMonitor5d9Dlc = 0;
static volatile uint8_t driverMonitor247Dlc = 0;
static volatile uint8_t driverMonitor399Dlc = 0;
static volatile uint8_t driverMonitor370Dlc = 0;
static volatile uint8_t driverMonitor389Bus = DRIVER_MONITOR_BUS_UNKNOWN;
static volatile uint8_t driverMonitor5d9Bus = DRIVER_MONITOR_BUS_UNKNOWN;
static volatile uint8_t driverMonitor247Bus = DRIVER_MONITOR_BUS_UNKNOWN;
static volatile uint8_t driverMonitor399Bus = DRIVER_MONITOR_BUS_UNKNOWN;
static volatile uint8_t driverMonitor370Bus = DRIVER_MONITOR_BUS_UNKNOWN;
static uint8_t driverMonitor389Data[8] = {};
static uint8_t driverMonitor5d9Data[8] = {};
static uint8_t driverMonitor247Data[8] = {};
static uint8_t driverMonitor399Data[8] = {};
static uint8_t driverMonitor370Data[8] = {};
static volatile uint8_t driverMonitorDasHandsOn = 0xFF;
static volatile uint8_t driverMonitorEpasHandsOn = 0xFF;
static volatile int16_t driverMonitorEpasTorqueCentiNm = INT16_MIN;
static volatile uint8_t driverMonitorInteraction = 0xFF;
static volatile uint8_t driverMonitorPreviousInteraction = 0xFF;
static volatile uint32_t driverMonitorInteractionChangeMs = 0;
static volatile uint32_t driverMonitorInteractionChanges = 0;

static const char *driverMonitorLabelSlotName(uint8_t slot) {
  static const char *names[DRIVER_MONITOR_LABEL_COUNT] = {"A", "B", "C", "D", "E", "F"};
  return slot < DRIVER_MONITOR_LABEL_COUNT ? names[slot] : "NONE";
}

static const char *driverMonitorLabelName(uint8_t slot) {
  static const char *names[DRIVER_MONITOR_LABEL_COUNT] = {
    "FRONT_NORMAL", "SCREEN_GLANCE", "SIDE_LOOK", "LOOK_DOWN",
    "TORQUE_LOOK_AWAY", "FRONT_NO_TORQUE"
  };
  return slot < DRIVER_MONITOR_LABEL_COUNT ? names[slot] : "NONE";
}

static const char *driverMonitorInteractionName(uint8_t v) {
  switch (v) {
    case 0: return "DRIVER_INTERACTING";
    case 1: return "DRIVER_NOT_INTERACTING";
    case 2: return "CONTINUED_DRIVER_NOT_INTERACTING";
    case 3: return "RESERVED_3";
    default: return "UNKNOWN";
  }
}

static const char *driverMonitorPhysicalBusName(uint8_t bus) {
  return bus == DRIVER_MONITOR_BUS_A ? "CAN A" : bus == DRIVER_MONITOR_BUS_B ? "CAN B" : "UNKNOWN";
}

static const char *driverMonitorProfileBusName(uint8_t bus) {
  return bus == DRIVER_MONITOR_BUS_A ? activeProfileCanAName() :
         bus == DRIVER_MONITOR_BUS_B ? activeProfileCanBName() : "UNKNOWN";
}

static inline uint32_t driverMonitorAge(uint32_t now, uint32_t ts) {
  return ts ? (uint32_t)(now - ts) : 0xFFFFFFFFUL;
}

static void driverMonitorFreeBuffers() {
  if (driverMonitorPreRing) free(driverMonitorPreRing);
  if (driverMonitorArchive) free(driverMonitorArchive);
  driverMonitorPreRing = nullptr;
  driverMonitorArchive = nullptr;
}

static bool driverMonitorCaptureInit() {
  if (driverMonitorPreRing && driverMonitorArchive) return true;

  const size_t preBytes = sizeof(DriverMonitorSample) * DRIVER_MONITOR_PRE_CAPACITY;
  const size_t archiveBytes = sizeof(DriverMonitorRecord) * DRIVER_MONITOR_ARCHIVE_CAPACITY;
  driverMonitorFreeBuffers();

  DriverMonitorSample *pre = (DriverMonitorSample *)heap_caps_malloc(preBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  DriverMonitorRecord *archive = (DriverMonitorRecord *)heap_caps_malloc(archiveBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  bool psram = pre && archive;
  if (!psram) {
    if (pre) free(pre);
    if (archive) free(archive);
    pre = (DriverMonitorSample *)malloc(preBytes);
    archive = (DriverMonitorRecord *)malloc(archiveBytes);
  }
  if (!pre || !archive) {
    if (pre) free(pre);
    if (archive) free(archive);
    driverMonitorInitError = true;
    driverMonitorPreRing = nullptr;
    driverMonitorArchive = nullptr;
    return false;
  }

  memset(pre, 0, preBytes);
  memset(archive, 0, archiveBytes);
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  driverMonitorPreRing = pre;
  driverMonitorArchive = archive;
  driverMonitorUsingPsram = psram;
  driverMonitorInitError = false;
  driverMonitorPreHead = 0;
  driverMonitorPreCount = 0;
  driverMonitorArchiveCount = 0;
  driverMonitorSegmentCount = 0;
  driverMonitorCurrentSegment = 0xFF;
  driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
  driverMonitorCapturing = false;
  driverMonitorExporting = false;
  memset(driverMonitorSegments, 0, sizeof(driverMonitorSegments));
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
  T2CAN_SERIAL_PRINTF("Driver Monitoring Capture: allocated %u + %u bytes (%s)\n",
                (unsigned)preBytes, (unsigned)archiveBytes, psram ? "PSRAM" : "internal RAM");
  return true;
}

static inline bool driverMonitorTargetFrame(uint8_t bus, uint16_t id) {
  return driver_monitor_capture_pure::targetFrame(bus, id);
}

static void driverMonitorUpdateLiveLocked(const DriverMonitorSample &s) {
  volatile uint32_t *lastMs = nullptr;
  volatile uint32_t *count = nullptr;
  volatile uint8_t *lastDlc = nullptr;
  volatile uint8_t *lastBus = nullptr;
  uint8_t *lastData = nullptr;
  if (s.id == DRIVER_MONITOR_ID_STATUS2) {
    lastMs = &driverMonitor389Ms; count = &driverMonitor389Count; lastDlc = &driverMonitor389Dlc; lastBus = &driverMonitor389Bus; lastData = driverMonitor389Data;
  } else if (s.id == DRIVER_MONITOR_ID_CARLOG) {
    lastMs = &driverMonitor5d9Ms; count = &driverMonitor5d9Count; lastDlc = &driverMonitor5d9Dlc; lastBus = &driverMonitor5d9Bus; lastData = driverMonitor5d9Data;
  } else if (s.id == DRIVER_MONITOR_ID_AUTOPILOT_DEBUG) {
    lastMs = &driverMonitor247Ms; count = &driverMonitor247Count; lastDlc = &driverMonitor247Dlc; lastBus = &driverMonitor247Bus; lastData = driverMonitor247Data;
  } else if (s.id == DRIVER_MONITOR_ID_DAS_STATUS) {
    lastMs = &driverMonitor399Ms; count = &driverMonitor399Count; lastDlc = &driverMonitor399Dlc; lastBus = &driverMonitor399Bus; lastData = driverMonitor399Data;
  } else if (s.id == DRIVER_MONITOR_ID_EPAS_STATUS) {
    lastMs = &driverMonitor370Ms; count = &driverMonitor370Count; lastDlc = &driverMonitor370Dlc; lastBus = &driverMonitor370Bus; lastData = driverMonitor370Data;
  }
  if (!lastMs) return;
  *lastMs = s.ms;
  (*count)++;
  *lastDlc = s.dlc;
  *lastBus = s.bus;
  memcpy(lastData, s.data, sizeof(s.data));

  // Decode by CAN ID only. On non-YL profiles these are research/best-effort
  // interpretations and the raw frame + physical bus remain authoritative.
  if (s.id == DRIVER_MONITOR_ID_STATUS2) {
    const uint8_t level = driver_monitor_capture_pure::driverMonitorInteractionLevelPure(s.data, s.dlc);
    if (level != 0xFF) {
      if (driverMonitorInteraction != 0xFF && level != driverMonitorInteraction) {
        driverMonitorPreviousInteraction = driverMonitorInteraction;
        driverMonitorInteractionChangeMs = s.ms;
        driverMonitorInteractionChanges++;
      }
      driverMonitorInteraction = level;
    }
  }
  if (s.id == DRIVER_MONITOR_ID_DAS_STATUS) {
    driverMonitorDasHandsOn = driver_monitor_capture_pure::driverMonitorDasHandsOnPure(s.data, s.dlc);
  }
  if (s.id == DRIVER_MONITOR_ID_EPAS_STATUS) {
    driverMonitorEpasHandsOn = driver_monitor_capture_pure::driverMonitorEpasHandsOnPure(s.data, s.dlc);
    driverMonitorEpasTorqueCentiNm = driver_monitor_capture_pure::driverMonitorEpasTorqueCentiNmPure(s.data, s.dlc);
  }
}

static bool driverMonitorAppendRecordLocked(const DriverMonitorSample &s, uint8_t label, uint8_t segment, int32_t relativeMs) {
  if (!driverMonitorArchive || driverMonitorArchiveCount >= DRIVER_MONITOR_ARCHIVE_CAPACITY) {
    driverMonitorDropped++;
    driverMonitorCapturing = false;
    return false;
  }
  DriverMonitorRecord &r = driverMonitorArchive[driverMonitorArchiveCount++];
  r.sample = s;
  r.relativeMs = relativeMs;
  r.label = label;
  r.segment = segment;
  if (driverMonitorCurrentSegment < driverMonitorSegmentCount)
    driverMonitorSegments[driverMonitorCurrentSegment].count++;
  return true;
}

static inline void driverMonitorCaptureObserve(uint8_t bus, uint16_t id, uint8_t dlc, const uint8_t *data) {
  if (!labMenuEnabled || !data || !driverMonitorTargetFrame(bus, id)) return;
  if (!driverMonitorPreRing || !driverMonitorArchive) return;

  DriverMonitorSample s = {};
  s.ms = (uint32_t)millis();
  s.id = id;
  s.bus = bus;
  s.dlc = dlc > 8 ? 8 : dlc;
  if (s.dlc) memcpy(s.data, data, s.dlc);

  portENTER_CRITICAL(&driverMonitorCaptureMux);
  driverMonitorUpdateLiveLocked(s);
  driverMonitorPreRing[driverMonitorPreHead] = s;
  driverMonitorPreHead = (uint16_t)((driverMonitorPreHead + 1u) % DRIVER_MONITOR_PRE_CAPACITY);
  if (driverMonitorPreCount < DRIVER_MONITOR_PRE_CAPACITY) driverMonitorPreCount++;

  if (driverMonitorCapturing && driverMonitorCurrentSegment < driverMonitorSegmentCount) {
    const int32_t rel = (int32_t)(s.ms - driverMonitorTriggerMs);
    const bool stored = driverMonitorAppendRecordLocked(s, driverMonitorCurrentLabel,
                                    driverMonitorSegments[driverMonitorCurrentSegment].number, rel);
    if (!stored && driverMonitorCurrentSegment < driverMonitorSegmentCount) {
      driverMonitorSegments[driverMonitorCurrentSegment].aborted = 1;
      driverMonitorCurrentSegment = 0xFF;
      driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
    }
  }
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
}

static bool driverMonitorCaptureRequest(uint8_t labelSlot) {
  if (!labMenuEnabled || labelSlot >= DRIVER_MONITOR_LABEL_COUNT) return false;
  if (!driverMonitorCaptureInit()) return false;
  const uint32_t now = (uint32_t)millis();

  portENTER_CRITICAL(&driverMonitorCaptureMux);
  driverMonitorRequests++;
  if (driverMonitorCapturing || driverMonitorExporting || driverMonitorSegmentCount >= DRIVER_MONITOR_MAX_SEGMENTS ||
      driverMonitorArchiveCount >= DRIVER_MONITOR_ARCHIVE_CAPACITY) {
    driverMonitorIgnored++;
    portEXIT_CRITICAL(&driverMonitorCaptureMux);
    return false;
  }

  const uint8_t segmentIndex = driverMonitorSegmentCount;
  DriverMonitorSegment &seg = driverMonitorSegments[segmentIndex];
  memset(&seg, 0, sizeof(seg));
  seg.triggerMs = now;
  seg.startIndex = driverMonitorArchiveCount;
  seg.label = labelSlot;
  seg.number = (uint8_t)(segmentIndex + 1u);
  driverMonitorSegmentCount++;
  driverMonitorCurrentSegment = segmentIndex;
  driverMonitorCurrentLabel = labelSlot;

  const uint16_t valid = driverMonitorPreCount;
  const uint16_t start = (uint16_t)((driverMonitorPreHead + DRIVER_MONITOR_PRE_CAPACITY - valid) % DRIVER_MONITOR_PRE_CAPACITY);
  for (uint16_t i = 0; i < valid; i++) {
    const DriverMonitorSample &s = driverMonitorPreRing[(uint16_t)((start + i) % DRIVER_MONITOR_PRE_CAPACITY)];
    const uint32_t age = (uint32_t)(now - s.ms);
    if (age > DRIVER_MONITOR_PRE_MS) continue;
    if (!driverMonitorAppendRecordLocked(s, labelSlot, seg.number, -(int32_t)age)) break;
  }

  driverMonitorTriggerMs = now;
  driverMonitorPostDeadlineMs = now + DRIVER_MONITOR_POST_MS;
  driverMonitorCapturing = driverMonitorArchiveCount < DRIVER_MONITOR_ARCHIVE_CAPACITY;
  if (!driverMonitorCapturing) seg.aborted = 1;
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
  return driverMonitorCapturing;
}

static void driverMonitorCaptureTick(uint32_t now) {
  if (!labMenuEnabled || !driverMonitorArchive) return;
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (driverMonitorCapturing && (int32_t)(now - driverMonitorPostDeadlineMs) >= 0) {
    driverMonitorCapturing = false;
    if (driverMonitorCurrentSegment < driverMonitorSegmentCount)
      driverMonitorSegments[driverMonitorCurrentSegment].complete = 1;
    driverMonitorCurrentSegment = 0xFF;
    driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
  }
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
}

static void driverMonitorCaptureSuspend() {
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (driverMonitorCapturing && driverMonitorCurrentSegment < driverMonitorSegmentCount) {
    driverMonitorSegments[driverMonitorCurrentSegment].aborted = 1;
    driverMonitorCapturing = false;
  }
  driverMonitorCurrentSegment = 0xFF;
  driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
}

static void driverMonitorCaptureReset() {
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (driverMonitorExporting) {
    driverMonitorIgnored++;
    portEXIT_CRITICAL(&driverMonitorCaptureMux);
    return;
  }
  driverMonitorArchiveCount = 0;
  driverMonitorSegmentCount = 0;
  driverMonitorCurrentSegment = 0xFF;
  driverMonitorCurrentLabel = DRIVER_MONITOR_LABEL_NONE;
  driverMonitorCapturing = false;
  driverMonitorTriggerMs = 0;
  driverMonitorPostDeadlineMs = 0;
  driverMonitorRequests = 0;
  driverMonitorIgnored = 0;
  driverMonitorDropped = 0;
  memset(driverMonitorSegments, 0, sizeof(driverMonitorSegments));
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
}

static bool driverMonitorCaptureBeginExport(uint16_t *countOut) {
  if (!countOut) return false;
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (driverMonitorCapturing || driverMonitorExporting || !driverMonitorArchive) {
    portEXIT_CRITICAL(&driverMonitorCaptureMux);
    return false;
  }
  driverMonitorExporting = true;
  *countOut = driverMonitorArchiveCount;
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
  return true;
}

static void driverMonitorCaptureEndExport() {
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  driverMonitorExporting = false;
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
}

static bool driverMonitorCaptureCopyRecord(uint16_t index, DriverMonitorRecord *out) {
  if (!out) return false;
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (!driverMonitorArchive || index >= driverMonitorArchiveCount) {
    portEXIT_CRITICAL(&driverMonitorCaptureMux);
    return false;
  }
  *out = driverMonitorArchive[index];
  portEXIT_CRITICAL(&driverMonitorCaptureMux);
  return true;
}

static String driverMonitorRawHex(const uint8_t *data, uint8_t dlc) {
  String out;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  out.reserve(n ? n * 3 : 1);
  for (uint8_t i = 0; i < n; i++) {
    if (i) out += ' ';
    char b[4];
    snprintf(b, sizeof(b), "%02X", data[i]);
    out += b;
  }
  return out;
}

static String driverMonitorCaptureStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  bool capturing, exporting, initError, usingPsram;
  uint16_t archiveCount, preCount;
  uint8_t segmentCount, currentLabel, interaction, previousInteraction;
  uint32_t deadline, requests, ignored, dropped, ms389, ms5d9, ms247, ms399, ms370;
  uint32_t count389, count5d9, count247, count399, count370, changeMs, changes;
  uint8_t dlc389, dlc5d9, dlc247, dlc399, dlc370, dasHandsOn, epasHandsOn;
  uint8_t bus389, bus5d9, bus247, bus399, bus370;
  int16_t epasTorqueCentiNm;
  uint8_t d389[8], d5d9[8], d247[8], d399[8], d370[8];

  portENTER_CRITICAL(&driverMonitorCaptureMux);
  capturing = driverMonitorCapturing; exporting = driverMonitorExporting; initError = driverMonitorInitError; usingPsram = driverMonitorUsingPsram;
  archiveCount = driverMonitorArchiveCount; preCount = driverMonitorPreCount; segmentCount = driverMonitorSegmentCount; currentLabel = driverMonitorCurrentLabel;
  interaction = driverMonitorInteraction; previousInteraction = driverMonitorPreviousInteraction; deadline = driverMonitorPostDeadlineMs;
  requests = driverMonitorRequests; ignored = driverMonitorIgnored; dropped = driverMonitorDropped;
  ms389 = driverMonitor389Ms; ms5d9 = driverMonitor5d9Ms; ms247 = driverMonitor247Ms; ms399 = driverMonitor399Ms; ms370 = driverMonitor370Ms;
  count389 = driverMonitor389Count; count5d9 = driverMonitor5d9Count; count247 = driverMonitor247Count; count399 = driverMonitor399Count; count370 = driverMonitor370Count;
  changeMs = driverMonitorInteractionChangeMs; changes = driverMonitorInteractionChanges;
  dlc389 = driverMonitor389Dlc; dlc5d9 = driverMonitor5d9Dlc; dlc247 = driverMonitor247Dlc; dlc399 = driverMonitor399Dlc; dlc370 = driverMonitor370Dlc;
  bus389 = driverMonitor389Bus; bus5d9 = driverMonitor5d9Bus; bus247 = driverMonitor247Bus; bus399 = driverMonitor399Bus; bus370 = driverMonitor370Bus;
  dasHandsOn = driverMonitorDasHandsOn; epasHandsOn = driverMonitorEpasHandsOn; epasTorqueCentiNm = driverMonitorEpasTorqueCentiNm;
  memcpy(d389, driverMonitor389Data, 8); memcpy(d5d9, driverMonitor5d9Data, 8); memcpy(d247, driverMonitor247Data, 8); memcpy(d399, driverMonitor399Data, 8); memcpy(d370, driverMonitor370Data, 8);
  portEXIT_CRITICAL(&driverMonitorCaptureMux);

  uint32_t preCoverage = 0;
  portENTER_CRITICAL(&driverMonitorCaptureMux);
  if (driverMonitorPreRing && driverMonitorPreCount) {
    const uint16_t valid = driverMonitorPreCount;
    const uint16_t newestIndex = (uint16_t)((driverMonitorPreHead + DRIVER_MONITOR_PRE_CAPACITY - 1u) % DRIVER_MONITOR_PRE_CAPACITY);
    const uint32_t newestAge = (uint32_t)(now - driverMonitorPreRing[newestIndex].ms);
    if (newestAge <= 1000u) {
      const uint16_t start = (uint16_t)((driverMonitorPreHead + DRIVER_MONITOR_PRE_CAPACITY - valid) % DRIVER_MONITOR_PRE_CAPACITY);
      for (uint16_t i = 0; i < valid; i++) {
        const DriverMonitorSample &candidate = driverMonitorPreRing[(uint16_t)((start + i) % DRIVER_MONITOR_PRE_CAPACITY)];
        const uint32_t age = (uint32_t)(now - candidate.ms);
        if (age <= DRIVER_MONITOR_PRE_MS) { preCoverage = age; break; }
      }
    }
  }
  portEXIT_CRITICAL(&driverMonitorCaptureMux);

  const bool supported = true;
  const bool initialized = driverMonitorPreRing && driverMonitorArchive;
  const char *state = initError ? "ERROR" : capturing ? "CAPTURING" :
                      archiveCount >= DRIVER_MONITOR_ARCHIVE_CAPACITY ? "FULL" : "READY";
  const uint32_t remaining = capturing && (int32_t)(deadline - now) > 0 ? (uint32_t)(deadline - now) : 0;

  String j;
  j.reserve(1400);
  j = "{\"ok\":true";
  JsonWriterArduino jw(j, true);
  jw.boolean("supported", supported);
  jw.string("canAName", activeProfileCanAName());
  jw.string("canBName", activeProfileCanBName());
  jw.boolean("initialized", initialized);
  jw.string("state", state);
  jw.boolean("capturing", capturing);
  jw.boolean("exporting", exporting);
  jw.boolean("usingPsram", usingPsram);
  jw.u32("remainingMs", remaining);
  jw.u32("preWindowMs", DRIVER_MONITOR_PRE_MS);
  jw.u32("postWindowMs", DRIVER_MONITOR_POST_MS);
  jw.u32("preFrames", preCount);
  jw.u32("preCoverageMs", preCoverage);
  jw.u32("segments", segmentCount);
  jw.u32("maxSegments", DRIVER_MONITOR_MAX_SEGMENTS);
  jw.u32("samples", archiveCount);
  jw.u32("capacity", DRIVER_MONITOR_ARCHIVE_CAPACITY);
  jw.string("currentSlot", driverMonitorLabelSlotName(currentLabel));
  jw.string("currentLabel", driverMonitorLabelName(currentLabel));
  jw.u32("requests", requests);
  jw.u32("ignored", ignored);
  jw.u32("dropped", dropped);
  jw.u32("interactionRaw", interaction);
  jw.string("interactionName", driverMonitorInteractionName(interaction));
  jw.u32("previousInteractionRaw", previousInteraction);
  jw.string("previousInteractionName", driverMonitorInteractionName(previousInteraction));
  jw.u32("interactionChanges", changes);
  jw.u32("interactionChangeAgeMs", changeMs ? driverMonitorAge(now, changeMs) : 0xFFFFFFFFUL);
  jw.i32("bus389", bus389 == DRIVER_MONITOR_BUS_UNKNOWN ? -1 : (int32_t)bus389);
  jw.i32("bus5d9", bus5d9 == DRIVER_MONITOR_BUS_UNKNOWN ? -1 : (int32_t)bus5d9);
  jw.i32("bus247", bus247 == DRIVER_MONITOR_BUS_UNKNOWN ? -1 : (int32_t)bus247);
  jw.i32("bus399", bus399 == DRIVER_MONITOR_BUS_UNKNOWN ? -1 : (int32_t)bus399);
  jw.i32("bus370", bus370 == DRIVER_MONITOR_BUS_UNKNOWN ? -1 : (int32_t)bus370);
  jw.u32("age389Ms", driverMonitorAge(now, ms389));
  jw.u32("age5d9Ms", driverMonitorAge(now, ms5d9));
  jw.u32("age247Ms", driverMonitorAge(now, ms247));
  jw.u32("age399Ms", driverMonitorAge(now, ms399));
  jw.u32("age370Ms", driverMonitorAge(now, ms370));
  jw.u32("count389", count389);
  jw.u32("count5d9", count5d9);
  jw.u32("count247", count247);
  jw.u32("count399", count399);
  jw.u32("count370", count370);
  jw.string("raw389", driverMonitorRawHex(d389, dlc389));
  jw.string("raw5d9", driverMonitorRawHex(d5d9, dlc5d9));
  jw.string("raw247", driverMonitorRawHex(d247, dlc247));
  jw.string("raw399", driverMonitorRawHex(d399, dlc399));
  jw.string("raw370", driverMonitorRawHex(d370, dlc370));
  jw.i32("dasHandsOnRaw", dasHandsOn == 0xFF ? -1 : (int32_t)dasHandsOn);
  jw.i32("epasHandsOnRaw", epasHandsOn == 0xFF ? -1 : (int32_t)epasHandsOn);
  if (epasTorqueCentiNm == INT16_MIN) jw.raw("epasTorqueNm", "null");
  else jw.fixed("epasTorqueNm", (int32_t)epasTorqueCentiNm, 2u);
  jw.finish();
  return j;
}
