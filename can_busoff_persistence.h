#pragma once

enum CanBusOffRecordBoot : uint8_t {
  CAN_BUS_OFF_BOOT_NONE = 0,
  CAN_BUS_OFF_BOOT_CURRENT = 1,
  CAN_BUS_OFF_BOOT_PREVIOUS = 2
};

enum CanBusOffPersistState : uint8_t {
  CAN_BUS_OFF_PERSIST_EMPTY = 0,
  CAN_BUS_OFF_PERSIST_LOADED = 1,
  CAN_BUS_OFF_PERSIST_DIRTY = 2,
  CAN_BUS_OFF_PERSIST_SAVED = 3,
  CAN_BUS_OFF_PERSIST_ERROR = 4
};

enum CanBusOffPersistError : uint8_t {
  CAN_BUS_OFF_PERSIST_ERROR_NONE = 0,
  CAN_BUS_OFF_PERSIST_ERROR_BUSY = 1,
  CAN_BUS_OFF_PERSIST_ERROR_OPEN = 2,
  CAN_BUS_OFF_PERSIST_ERROR_ENCODE = 3,
  CAN_BUS_OFF_PERSIST_ERROR_WRITE = 4,
  CAN_BUS_OFF_PERSIST_ERROR_VERIFY = 5,
  CAN_BUS_OFF_PERSIST_ERROR_CLEAR = 6
};

static constexpr uint8_t CAN_BUS_OFF_DIRTY_A = 0x01u;
static constexpr uint8_t CAN_BUS_OFF_DIRTY_B = 0x02u;
static constexpr uint32_t CAN_BUS_OFF_PERSIST_RETRY_MS = 5000u;

static portMUX_TYPE canBusOffPersistenceMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t canBusOffPersistenceDirtyMask = 0;
static volatile uint32_t canBusOffPersistenceDirtyEpoch[2] = {};
static volatile uint8_t canBusOffPersistenceBoot[2] = {};
static volatile uint8_t canBusOffPersistenceState[2] = {};
static volatile uint8_t canBusOffPersistenceError[2] = {};
static volatile uint32_t canBusOffPersistenceRetryAtMs[2] = {};
static volatile bool canBusOffPersistenceBusy = false;
static CanBusOffSlotMetaPure canBusOffPersistenceSlots[2][2] = {};
static uint8_t canBusOffPersistenceActiveSlot[2] = {0xFFu, 0xFFu};
static CanBusOffRecordPure canBusOffPersistenceDecodeSlots[2] = {};
static CanBusOffRecordPure canBusOffPersistenceRecord = {};
static uint8_t canBusOffPersistenceEncoded[CAN_BUS_OFF_RECORD_MAX_BYTES_PURE] = {};

static inline uint8_t canBusOffPersistenceIndex(uint8_t bus) {
  return bus == CAN_BUS_OFF_BUS_A_PURE ? 0u : 1u;
}

static inline uint8_t canBusOffPersistenceMask(uint8_t bus) {
  return bus == CAN_BUS_OFF_BUS_A_PURE ? CAN_BUS_OFF_DIRTY_A : CAN_BUS_OFF_DIRTY_B;
}

static inline const char *canBusOffRecordBootName(uint8_t bus) {
  const uint8_t value = canBusOffPersistenceBoot[canBusOffPersistenceIndex(bus)];
  if (value == CAN_BUS_OFF_BOOT_CURRENT) return "CURRENT_BOOT";
  if (value == CAN_BUS_OFF_BOOT_PREVIOUS) return "PREVIOUS_BOOT";
  return "NONE";
}

static inline const char *canBusOffPersistStateName(uint8_t bus) {
  switch (canBusOffPersistenceState[canBusOffPersistenceIndex(bus)]) {
    case CAN_BUS_OFF_PERSIST_LOADED: return "LOADED";
    case CAN_BUS_OFF_PERSIST_DIRTY: return "DIRTY";
    case CAN_BUS_OFF_PERSIST_SAVED: return "SAVED";
    case CAN_BUS_OFF_PERSIST_ERROR: return "ERROR";
    default: return "EMPTY";
  }
}

static inline const char *canBusOffPersistErrorName(uint8_t bus) {
  switch (canBusOffPersistenceError[canBusOffPersistenceIndex(bus)]) {
    case CAN_BUS_OFF_PERSIST_ERROR_BUSY: return "BUSY";
    case CAN_BUS_OFF_PERSIST_ERROR_OPEN: return "NVS_OPEN";
    case CAN_BUS_OFF_PERSIST_ERROR_ENCODE: return "ENCODE";
    case CAN_BUS_OFF_PERSIST_ERROR_WRITE: return "WRITE";
    case CAN_BUS_OFF_PERSIST_ERROR_VERIFY: return "VERIFY";
    case CAN_BUS_OFF_PERSIST_ERROR_CLEAR: return "CLEAR_VERIFY";
    default: return "NONE";
  }
}

static bool canBusOffPersistenceTryAcquire() {
  bool acquired = false;
  portENTER_CRITICAL(&canBusOffPersistenceMux);
  if (!canBusOffPersistenceBusy) {
    canBusOffPersistenceBusy = true;
    acquired = true;
  }
  portEXIT_CRITICAL(&canBusOffPersistenceMux);
  return acquired;
}

static void canBusOffPersistenceRelease() {
  portENTER_CRITICAL(&canBusOffPersistenceMux);
  canBusOffPersistenceBusy = false;
  portEXIT_CRITICAL(&canBusOffPersistenceMux);
}

static bool canBusOffPersistenceReadSlot(Preferences &p, const char *key,
                                         uint8_t bus,
                                         CanBusOffRecordPure &record) {
  const size_t length = p.getBytesLength(key);
  if (length == 0 || length > sizeof(canBusOffPersistenceEncoded)) return false;
  if (p.getBytes(key, canBusOffPersistenceEncoded, length) != length) return false;
  return decodeCanBusOffRecordPure(canBusOffPersistenceEncoded, length, bus, record);
}

static void canBusOffPersistenceRestoreRecord(const CanBusOffRecordPure &record) {
  uint32_t maxSeq = 0;
  if (record.bus == CAN_BUS_OFF_BUS_A_PURE) {
    portENTER_CRITICAL(&canATxTraceMux);
    canAMcpBusOffCount = record.count;
    canATxTraceFrozenCount = record.traceCount;
    canATxTraceFrozenMs = record.eventUptimeMs;
    canATxTraceFrozenBusOffOrdinal = record.count;
    canALastBusOffSnapshot.valid = record.snapshot.valid;
    canALastBusOffSnapshot.capturedMs = record.eventUptimeMs;
    canALastBusOffSnapshot.eflg = record.snapshot.errorFlags;
    canALastBusOffSnapshot.txFailConsecutive = record.snapshot.txFailConsecutive;
    canALastBusOffSnapshot.rxAgeMs = record.snapshot.rxAgeOrGapMs;
    canALastBusOffSnapshot.rxOverflowCount = record.snapshot.rxOverflowCount;
    canALastBusOffSnapshot.txOk = record.snapshot.txOk;
    canALastBusOffSnapshot.txFail = record.snapshot.txFail;
    for (uint8_t i = 0; i < record.traceCount; ++i) {
      const CanBusOffTraceEntryPure &src = record.trace[i];
      CanATxTraceEntry &dst = canATxTraceFrozen[i];
      dst = {};
      dst.seq = src.seq; dst.capturedMs = src.capturedMs; dst.id = src.id;
      dst.dlc = src.dlc; dst.source = src.source; dst.reason = src.reason;
      dst.result = src.result; memcpy(dst.data, src.data, sizeof(dst.data));
      if (src.seq > maxSeq) maxSeq = src.seq;
    }
    for (uint8_t i = record.traceCount; i < CAN_A_TX_TRACE_CAPACITY; ++i)
      canATxTraceFrozen[i] = {};
    portEXIT_CRITICAL(&canATxTraceMux);
    __atomic_store_n(&canATxTraceSeq, maxSeq, __ATOMIC_RELEASE);
  } else {
    portENTER_CRITICAL(&canRecoveryMux);
    canTwaiBusOffCount = record.count;
    canTwaiLastBusOffSnapshot.valid = record.snapshot.valid;
    canTwaiLastBusOffSnapshot.state = record.snapshot.controllerState;
    canTwaiLastBusOffSnapshot.capturedMs = record.eventUptimeMs;
    canTwaiLastBusOffSnapshot.rxGapMs = record.snapshot.rxAgeOrGapMs;
    canTwaiLastBusOffSnapshot.msgsToTx = record.snapshot.msgsToTx;
    canTwaiLastBusOffSnapshot.msgsToRx = record.snapshot.msgsToRx;
    canTwaiLastBusOffSnapshot.txErrorCounter = record.snapshot.txErrorCounter;
    canTwaiLastBusOffSnapshot.rxErrorCounter = record.snapshot.rxErrorCounter;
    canTwaiLastBusOffSnapshot.txFailedCount = record.snapshot.txFailedCount;
    canTwaiLastBusOffSnapshot.rxMissedCount = record.snapshot.rxMissedCount;
    canTwaiLastBusOffSnapshot.rxOverrunCount = record.snapshot.rxOverrunCount;
    canTwaiLastBusOffSnapshot.arbLostCount = record.snapshot.arbLostCount;
    canTwaiLastBusOffSnapshot.busErrorCount = record.snapshot.busErrorCount;
    portEXIT_CRITICAL(&canRecoveryMux);
    portENTER_CRITICAL(&canBTxTraceMux);
    canBTxTraceFrozenCount = record.traceCount;
    canBTxTraceFrozenMs = record.eventUptimeMs;
    canBTxTraceFrozenBusOffOrdinal = record.count;
    for (uint8_t i = 0; i < record.traceCount; ++i) {
      const CanBusOffTraceEntryPure &src = record.trace[i];
      CanBTxTraceEntry &dst = canBTxTraceFrozen[i];
      dst = {};
      dst.seq = src.seq; dst.capturedMs = src.capturedMs; dst.id = src.id;
      dst.dlc = src.dlc; dst.source = src.source; dst.result = src.result;
      memcpy(dst.data, src.data, sizeof(dst.data));
      if (src.seq > maxSeq) maxSeq = src.seq;
    }
    for (uint8_t i = record.traceCount; i < CAN_B_TX_TRACE_CAPACITY; ++i)
      canBTxTraceFrozen[i] = {};
    portEXIT_CRITICAL(&canBTxTraceMux);
    __atomic_store_n(&canBTxTraceSeq, maxSeq, __ATOMIC_RELEASE);
  }
}

static bool canBusOffPersistenceLoadBus(Preferences &p, uint8_t bus,
                                        const char *key0, const char *key1) {
  const uint8_t index = canBusOffPersistenceIndex(bus);
  const bool valid0 = canBusOffPersistenceReadSlot(
      p, key0, bus, canBusOffPersistenceDecodeSlots[0]);
  const bool valid1 = canBusOffPersistenceReadSlot(
      p, key1, bus, canBusOffPersistenceDecodeSlots[1]);
  canBusOffPersistenceSlots[index][0] = {
      valid0, valid0 ? canBusOffPersistenceDecodeSlots[0].generation : 0u};
  canBusOffPersistenceSlots[index][1] = {
      valid1, valid1 ? canBusOffPersistenceDecodeSlots[1].generation : 0u};
  const uint8_t newest = canBusOffNewestSlotPure(
      canBusOffPersistenceSlots[index][0], canBusOffPersistenceSlots[index][1]);
  canBusOffPersistenceActiveSlot[index] = newest;
  if (newest == 0xFFu) return true;
  const CanBusOffRecordPure &record = canBusOffPersistenceDecodeSlots[newest];
  canBusOffPersistenceRestoreRecord(record);
  canBusOffPersistenceBoot[index] = CAN_BUS_OFF_BOOT_PREVIOUS;
  canBusOffPersistenceState[index] = CAN_BUS_OFF_PERSIST_LOADED;
  return true;
}

static bool canBusOffPersistenceLoad() {
  if (!canBusOffPersistenceTryAcquire()) return false;
  Preferences p;
  const bool opened = p.begin("canDiag", false);
  bool ok = opened;
  if (opened) {
    ok = canBusOffPersistenceLoadBus(
        p, CAN_BUS_OFF_BUS_A_PURE, "a0", "a1") &&
        canBusOffPersistenceLoadBus(
            p, CAN_BUS_OFF_BUS_B_PURE, "b0", "b1");
    p.end();
  }
  if (!ok) {
    canBusOffPersistenceState[0] = canBusOffPersistenceState[1] =
        CAN_BUS_OFF_PERSIST_ERROR;
    canBusOffPersistenceError[0] = canBusOffPersistenceError[1] =
        CAN_BUS_OFF_PERSIST_ERROR_OPEN;
  }
  canBusOffPersistenceRelease();
  return ok;
}

static void canBusOffPersistenceMarkDirty(uint8_t bus) {
  if (bus != CAN_BUS_OFF_BUS_A_PURE && bus != CAN_BUS_OFF_BUS_B_PURE) return;
  const uint8_t index = canBusOffPersistenceIndex(bus);
  portENTER_CRITICAL(&canBusOffPersistenceMux);
  canBusOffPersistenceDirtyEpoch[index]++;
  canBusOffPersistenceDirtyMask |= canBusOffPersistenceMask(bus);
  canBusOffPersistenceBoot[index] = CAN_BUS_OFF_BOOT_CURRENT;
  canBusOffPersistenceState[index] = CAN_BUS_OFF_PERSIST_DIRTY;
  canBusOffPersistenceError[index] = CAN_BUS_OFF_PERSIST_ERROR_NONE;
  canBusOffPersistenceRetryAtMs[index] = 0;
  portEXIT_CRITICAL(&canBusOffPersistenceMux);
}

static void canBusOffPersistenceSnapshot(uint8_t bus,
                                         CanBusOffRecordPure &record) {
  record = {};
  record.bus = bus;
  if (bus == CAN_BUS_OFF_BUS_A_PURE) {
    portENTER_CRITICAL(&canATxTraceMux);
    record.count = canAMcpBusOffCount;
    record.eventUptimeMs = canATxTraceFrozenMs;
    record.snapshot.valid = canALastBusOffSnapshot.valid;
    record.snapshot.errorFlags = canALastBusOffSnapshot.eflg;
    record.snapshot.txFailConsecutive = canALastBusOffSnapshot.txFailConsecutive;
    record.snapshot.rxAgeOrGapMs = canALastBusOffSnapshot.rxAgeMs;
    record.snapshot.rxOverflowCount = canALastBusOffSnapshot.rxOverflowCount;
    record.snapshot.txOk = canALastBusOffSnapshot.txOk;
    record.snapshot.txFail = canALastBusOffSnapshot.txFail;
    record.traceCount = canATxTraceFrozenCount > CAN_BUS_OFF_TRACE_CAPACITY_PURE
        ? CAN_BUS_OFF_TRACE_CAPACITY_PURE : canATxTraceFrozenCount;
    for (uint8_t i = 0; i < record.traceCount; ++i) {
      const CanATxTraceEntry &src = canATxTraceFrozen[i];
      CanBusOffTraceEntryPure &dst = record.trace[i];
      dst.seq = src.seq; dst.capturedMs = src.capturedMs; dst.id = src.id;
      dst.dlc = src.dlc; dst.source = src.source; dst.reason = src.reason;
      dst.result = src.result; memcpy(dst.data, src.data, sizeof(dst.data));
    }
    portEXIT_CRITICAL(&canATxTraceMux);
  } else {
    portENTER_CRITICAL(&canRecoveryMux);
    record.count = canTwaiBusOffCount;
    record.eventUptimeMs = canTwaiLastBusOffSnapshot.capturedMs;
    record.snapshot.valid = canTwaiLastBusOffSnapshot.valid;
    record.snapshot.controllerState = canTwaiLastBusOffSnapshot.state;
    record.snapshot.rxAgeOrGapMs = canTwaiLastBusOffSnapshot.rxGapMs;
    record.snapshot.msgsToTx = canTwaiLastBusOffSnapshot.msgsToTx;
    record.snapshot.msgsToRx = canTwaiLastBusOffSnapshot.msgsToRx;
    record.snapshot.txErrorCounter = canTwaiLastBusOffSnapshot.txErrorCounter;
    record.snapshot.rxErrorCounter = canTwaiLastBusOffSnapshot.rxErrorCounter;
    record.snapshot.txFailedCount = canTwaiLastBusOffSnapshot.txFailedCount;
    record.snapshot.rxMissedCount = canTwaiLastBusOffSnapshot.rxMissedCount;
    record.snapshot.rxOverrunCount = canTwaiLastBusOffSnapshot.rxOverrunCount;
    record.snapshot.arbLostCount = canTwaiLastBusOffSnapshot.arbLostCount;
    record.snapshot.busErrorCount = canTwaiLastBusOffSnapshot.busErrorCount;
    portEXIT_CRITICAL(&canRecoveryMux);
    portENTER_CRITICAL(&canBTxTraceMux);
    record.traceCount = canBTxTraceFrozenCount > CAN_BUS_OFF_TRACE_CAPACITY_PURE
        ? CAN_BUS_OFF_TRACE_CAPACITY_PURE : canBTxTraceFrozenCount;
    for (uint8_t i = 0; i < record.traceCount; ++i) {
      const CanBTxTraceEntry &src = canBTxTraceFrozen[i];
      CanBusOffTraceEntryPure &dst = record.trace[i];
      dst.seq = src.seq; dst.capturedMs = src.capturedMs; dst.id = src.id;
      dst.dlc = src.dlc; dst.source = src.source; dst.result = src.result;
      memcpy(dst.data, src.data, sizeof(dst.data));
    }
    portEXIT_CRITICAL(&canBTxTraceMux);
  }
}

static bool canBusOffPersistenceWriteBus(uint8_t bus, uint32_t dirtyEpoch) {
  const uint8_t index = canBusOffPersistenceIndex(bus);
  canBusOffPersistenceSnapshot(bus, canBusOffPersistenceRecord);
  const uint8_t active = canBusOffPersistenceActiveSlot[index];
  canBusOffPersistenceRecord.generation = active == 0xFFu
      ? 0u : canBusOffPersistenceSlots[index][active].generation + 1u;
  const uint8_t slot = canBusOffNextWriteSlotPure(
      canBusOffPersistenceSlots[index][0], canBusOffPersistenceSlots[index][1]);
  const char *key = bus == CAN_BUS_OFF_BUS_A_PURE
      ? (slot == 0 ? "a0" : "a1") : (slot == 0 ? "b0" : "b1");
  const size_t length = encodeCanBusOffRecordPure(
      canBusOffPersistenceRecord, canBusOffPersistenceEncoded,
      sizeof(canBusOffPersistenceEncoded));
  if (length == 0) {
    canBusOffPersistenceError[index] = CAN_BUS_OFF_PERSIST_ERROR_ENCODE;
    return false;
  }
  Preferences p;
  if (!p.begin("canDiag", false)) {
    canBusOffPersistenceError[index] = CAN_BUS_OFF_PERSIST_ERROR_OPEN;
    return false;
  }
  const size_t written = p.putBytes(key, canBusOffPersistenceEncoded, length);
  bool verified = false;
  if (written == length && p.getBytesLength(key) == length &&
      p.getBytes(key, canBusOffPersistenceEncoded, length) == length) {
    CanBusOffRecordPure &decoded = canBusOffPersistenceDecodeSlots[0];
    verified = decodeCanBusOffRecordPure(
        canBusOffPersistenceEncoded, length, bus, decoded) &&
        decoded.generation == canBusOffPersistenceRecord.generation &&
        decoded.count == canBusOffPersistenceRecord.count;
  }
  p.end();
  if (!verified) {
    canBusOffPersistenceError[index] = written == length
        ? CAN_BUS_OFF_PERSIST_ERROR_VERIFY : CAN_BUS_OFF_PERSIST_ERROR_WRITE;
    return false;
  }
  canBusOffPersistenceSlots[index][slot] = {
      true, canBusOffPersistenceRecord.generation};
  canBusOffPersistenceActiveSlot[index] = slot;
  portENTER_CRITICAL(&canBusOffPersistenceMux);
  if (canBusOffPersistenceDirtyEpoch[index] == dirtyEpoch)
    canBusOffPersistenceDirtyMask &= (uint8_t)~canBusOffPersistenceMask(bus);
  canBusOffPersistenceState[index] = CAN_BUS_OFF_PERSIST_SAVED;
  canBusOffPersistenceError[index] = CAN_BUS_OFF_PERSIST_ERROR_NONE;
  canBusOffPersistenceRetryAtMs[index] = 0;
  portEXIT_CRITICAL(&canBusOffPersistenceMux);
  return true;
}

static void canBusOffPersistenceService(uint32_t now) {
  uint8_t dirty;
  uint32_t epoch[2], retry[2];
  portENTER_CRITICAL(&canBusOffPersistenceMux);
  dirty = canBusOffPersistenceDirtyMask;
  epoch[0] = canBusOffPersistenceDirtyEpoch[0];
  epoch[1] = canBusOffPersistenceDirtyEpoch[1];
  retry[0] = canBusOffPersistenceRetryAtMs[0];
  retry[1] = canBusOffPersistenceRetryAtMs[1];
  portEXIT_CRITICAL(&canBusOffPersistenceMux);
  if (!dirty || !canBusOffPersistenceTryAcquire()) return;
  for (uint8_t index = 0; index < 2; ++index) {
    const uint8_t bus = index == 0 ? CAN_BUS_OFF_BUS_A_PURE : CAN_BUS_OFF_BUS_B_PURE;
    const uint8_t mask = canBusOffPersistenceMask(bus);
    if ((dirty & mask) == 0 || (retry[index] != 0 &&
        (int32_t)(now - retry[index]) < 0)) continue;
    if (!canBusOffPersistenceWriteBus(bus, epoch[index])) {
      portENTER_CRITICAL(&canBusOffPersistenceMux);
      canBusOffPersistenceState[index] = CAN_BUS_OFF_PERSIST_ERROR;
      canBusOffPersistenceRetryAtMs[index] = now + CAN_BUS_OFF_PERSIST_RETRY_MS;
      portEXIT_CRITICAL(&canBusOffPersistenceMux);
    }
  }
  canBusOffPersistenceRelease();
}

static bool canBusOffPersistenceClear() {
  if (!canBusOffPersistenceTryAcquire()) {
    canBusOffPersistenceError[0] = canBusOffPersistenceError[1] =
        CAN_BUS_OFF_PERSIST_ERROR_BUSY;
    return false;
  }
  Preferences p;
  bool ok = p.begin("canDiag", false);
  if (ok) {
    p.remove("a0"); p.remove("a1"); p.remove("b0"); p.remove("b1");
    ok = p.getBytesLength("a0") == 0 && p.getBytesLength("a1") == 0 &&
         p.getBytesLength("b0") == 0 && p.getBytesLength("b1") == 0;
    p.end();
  }
  if (ok) {
    portENTER_CRITICAL(&canBusOffPersistenceMux);
    canBusOffPersistenceDirtyMask = 0;
    canBusOffPersistenceDirtyEpoch[0] = canBusOffPersistenceDirtyEpoch[1] = 0;
    canBusOffPersistenceBoot[0] = canBusOffPersistenceBoot[1] = CAN_BUS_OFF_BOOT_NONE;
    canBusOffPersistenceState[0] = canBusOffPersistenceState[1] = CAN_BUS_OFF_PERSIST_EMPTY;
    canBusOffPersistenceError[0] = canBusOffPersistenceError[1] = CAN_BUS_OFF_PERSIST_ERROR_NONE;
    canBusOffPersistenceRetryAtMs[0] = canBusOffPersistenceRetryAtMs[1] = 0;
    canBusOffPersistenceSlots[0][0] = canBusOffPersistenceSlots[0][1] = {};
    canBusOffPersistenceSlots[1][0] = canBusOffPersistenceSlots[1][1] = {};
    canBusOffPersistenceActiveSlot[0] = canBusOffPersistenceActiveSlot[1] = 0xFFu;
    portEXIT_CRITICAL(&canBusOffPersistenceMux);
  } else {
    canBusOffPersistenceState[0] = canBusOffPersistenceState[1] =
        CAN_BUS_OFF_PERSIST_ERROR;
    canBusOffPersistenceError[0] = canBusOffPersistenceError[1] =
        CAN_BUS_OFF_PERSIST_ERROR_CLEAR;
  }
  canBusOffPersistenceRelease();
  return ok;
}
