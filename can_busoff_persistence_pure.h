#pragma once

#include <stddef.h>
#include <stdint.h>

static constexpr uint32_t CAN_BUS_OFF_RECORD_MAGIC_PURE = 0x5442434Fu;
static constexpr uint16_t CAN_BUS_OFF_RECORD_VERSION_V1_PURE = 1u;
static constexpr uint16_t CAN_BUS_OFF_RECORD_VERSION_PURE = 2u;
static constexpr uint8_t CAN_BUS_OFF_BUS_A_PURE = 1u;
static constexpr uint8_t CAN_BUS_OFF_BUS_B_PURE = 2u;
static constexpr uint8_t CAN_BUS_OFF_TRACE_CAPACITY_PURE = 64u;
static constexpr size_t CAN_BUS_OFF_RECORD_HEADER_V1_BYTES_PURE = 84u;
static constexpr size_t CAN_BUS_OFF_RECORD_HEADER_BYTES_PURE = 104u;
static constexpr size_t CAN_BUS_OFF_TRACE_ENTRY_BYTES_PURE = 28u;
static constexpr size_t CAN_BUS_OFF_RECORD_MAX_BYTES_PURE =
    CAN_BUS_OFF_RECORD_HEADER_BYTES_PURE +
    CAN_BUS_OFF_TRACE_CAPACITY_PURE * CAN_BUS_OFF_TRACE_ENTRY_BYTES_PURE + 4u;

struct CanBusOffTraceEntryPure {
  uint32_t seq;
  uint32_t capturedMs;
  uint16_t id;
  uint8_t dlc;
  uint8_t source;
  uint8_t reason;
  int32_t result;
  uint8_t data[8];
};

struct CanBusOffSnapshotPure {
  bool valid;
  uint8_t controllerState;
  uint8_t errorFlags;
  uint8_t txFailConsecutive;
  uint32_t rxAgeOrGapMs;
  uint32_t rxOverflowCount;
  uint32_t txOk;
  uint32_t txFail;
  uint32_t msgsToTx;
  uint32_t msgsToRx;
  uint32_t txErrorCounter;
  uint32_t rxErrorCounter;
  uint32_t txFailedCount;
  uint32_t rxMissedCount;
  uint32_t rxOverrunCount;
  uint32_t arbLostCount;
  uint32_t busErrorCount;
  uint32_t alertSeenMask;
  uint32_t alertBatchMask;
  uint32_t txFailedAlertAgeMs;
  uint32_t errPassAlertAgeMs;
  uint32_t busErrorAlertAgeMs;
};

struct CanBusOffRecordPure {
  uint8_t bus;
  uint32_t generation;
  uint32_t count;
  uint32_t eventUptimeMs;
  CanBusOffSnapshotPure snapshot;
  uint8_t traceCount;
  CanBusOffTraceEntryPure trace[CAN_BUS_OFF_TRACE_CAPACITY_PURE];
};

struct CanBusOffSlotMetaPure {
  bool valid;
  uint32_t generation;
};

static inline void canBusOffWrite16Pure(uint8_t *out, size_t &offset,
                                        uint16_t value) {
  out[offset++] = (uint8_t)value;
  out[offset++] = (uint8_t)(value >> 8);
}

static inline void canBusOffWrite32Pure(uint8_t *out, size_t &offset,
                                        uint32_t value) {
  out[offset++] = (uint8_t)value;
  out[offset++] = (uint8_t)(value >> 8);
  out[offset++] = (uint8_t)(value >> 16);
  out[offset++] = (uint8_t)(value >> 24);
}

static inline uint16_t canBusOffRead16Pure(const uint8_t *in, size_t &offset) {
  const uint16_t value = (uint16_t)in[offset] |
      (uint16_t)((uint16_t)in[offset + 1u] << 8);
  offset += 2u;
  return value;
}

static inline uint32_t canBusOffRead32Pure(const uint8_t *in, size_t &offset) {
  const uint32_t value = (uint32_t)in[offset] |
      ((uint32_t)in[offset + 1u] << 8) |
      ((uint32_t)in[offset + 2u] << 16) |
      ((uint32_t)in[offset + 3u] << 24);
  offset += 4u;
  return value;
}

static inline uint32_t canBusOffCrc32Pure(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
  }
  return ~crc;
}

static inline bool canBusOffGenerationNewerPure(uint32_t lhs, uint32_t rhs) {
  return (int32_t)(lhs - rhs) > 0;
}

static inline uint8_t canBusOffNewestSlotPure(CanBusOffSlotMetaPure slot0,
                                               CanBusOffSlotMetaPure slot1) {
  if (!slot0.valid) return slot1.valid ? 1u : 0xFFu;
  if (!slot1.valid) return 0u;
  return canBusOffGenerationNewerPure(slot1.generation, slot0.generation)
      ? 1u : 0u;
}

static inline uint8_t canBusOffNextWriteSlotPure(CanBusOffSlotMetaPure slot0,
                                                  CanBusOffSlotMetaPure slot1) {
  if (!slot0.valid) return 0u;
  if (!slot1.valid) return 1u;
  return canBusOffGenerationNewerPure(slot1.generation, slot0.generation)
      ? 0u : 1u;
}

static inline size_t encodeCanBusOffRecordPure(const CanBusOffRecordPure &record,
                                                uint8_t *out, size_t capacity) {
  if (!out || (record.bus != CAN_BUS_OFF_BUS_A_PURE &&
               record.bus != CAN_BUS_OFF_BUS_B_PURE) ||
      record.traceCount > CAN_BUS_OFF_TRACE_CAPACITY_PURE) return 0u;
  const size_t length = CAN_BUS_OFF_RECORD_HEADER_BYTES_PURE +
      (size_t)record.traceCount * CAN_BUS_OFF_TRACE_ENTRY_BYTES_PURE + 4u;
  if (capacity < length || length > 0xFFFFu) return 0u;
  size_t o = 0;
  canBusOffWrite32Pure(out, o, CAN_BUS_OFF_RECORD_MAGIC_PURE);
  canBusOffWrite16Pure(out, o, CAN_BUS_OFF_RECORD_VERSION_PURE);
  canBusOffWrite16Pure(out, o, (uint16_t)length);
  out[o++] = record.bus;
  out[o++] = 0; out[o++] = 0; out[o++] = 0;
  canBusOffWrite32Pure(out, o, record.generation);
  canBusOffWrite32Pure(out, o, record.count);
  canBusOffWrite32Pure(out, o, record.eventUptimeMs);
  out[o++] = record.snapshot.valid ? 1u : 0u;
  out[o++] = record.snapshot.controllerState;
  out[o++] = record.snapshot.errorFlags;
  out[o++] = record.snapshot.txFailConsecutive;
  const uint32_t snapshotValues[] = {
      record.snapshot.rxAgeOrGapMs, record.snapshot.rxOverflowCount,
      record.snapshot.txOk, record.snapshot.txFail,
      record.snapshot.msgsToTx, record.snapshot.msgsToRx,
      record.snapshot.txErrorCounter, record.snapshot.rxErrorCounter,
      record.snapshot.txFailedCount, record.snapshot.rxMissedCount,
      record.snapshot.rxOverrunCount, record.snapshot.arbLostCount,
      record.snapshot.busErrorCount};
  for (uint8_t i = 0; i < 13; ++i)
    canBusOffWrite32Pure(out, o, snapshotValues[i]);
  const uint32_t alertValues[] = {
      record.snapshot.alertSeenMask, record.snapshot.alertBatchMask,
      record.snapshot.txFailedAlertAgeMs,
      record.snapshot.errPassAlertAgeMs,
      record.snapshot.busErrorAlertAgeMs};
  for (uint8_t i = 0; i < 5; ++i)
    canBusOffWrite32Pure(out, o, alertValues[i]);
  out[o++] = record.traceCount;
  out[o++] = 0; out[o++] = 0; out[o++] = 0;
  for (uint8_t i = 0; i < record.traceCount; ++i) {
    const CanBusOffTraceEntryPure &entry = record.trace[i];
    canBusOffWrite32Pure(out, o, entry.seq);
    canBusOffWrite32Pure(out, o, entry.capturedMs);
    canBusOffWrite16Pure(out, o, entry.id);
    out[o++] = entry.dlc <= 8u ? entry.dlc : 8u;
    out[o++] = entry.source;
    out[o++] = entry.reason;
    out[o++] = 0; out[o++] = 0; out[o++] = 0;
    canBusOffWrite32Pure(out, o, (uint32_t)entry.result);
    for (uint8_t j = 0; j < 8; ++j) out[o++] = entry.data[j];
  }
  const uint32_t crc = canBusOffCrc32Pure(out, o);
  canBusOffWrite32Pure(out, o, crc);
  return o == length ? length : 0u;
}

static inline bool decodeCanBusOffRecordPure(const uint8_t *encoded,
                                              size_t length,
                                              uint8_t expectedBus,
                                              CanBusOffRecordPure &out) {
  if (!encoded || length < CAN_BUS_OFF_RECORD_HEADER_V1_BYTES_PURE + 4u ||
      length > CAN_BUS_OFF_RECORD_MAX_BYTES_PURE) return false;
  size_t o = 0;
  if (canBusOffRead32Pure(encoded, o) != CAN_BUS_OFF_RECORD_MAGIC_PURE)
    return false;
  const uint16_t version = canBusOffRead16Pure(encoded, o);
  if ((version != CAN_BUS_OFF_RECORD_VERSION_V1_PURE &&
       version != CAN_BUS_OFF_RECORD_VERSION_PURE) ||
      canBusOffRead16Pure(encoded, o) != length) return false;
  const size_t headerBytes = version == CAN_BUS_OFF_RECORD_VERSION_V1_PURE
      ? CAN_BUS_OFF_RECORD_HEADER_V1_BYTES_PURE
      : CAN_BUS_OFF_RECORD_HEADER_BYTES_PURE;
  if (length < headerBytes + 4u) return false;
  const uint8_t bus = encoded[o++];
  o += 3u;
  if (bus != expectedBus ||
      (bus != CAN_BUS_OFF_BUS_A_PURE && bus != CAN_BUS_OFF_BUS_B_PURE))
    return false;
  const uint32_t generation = canBusOffRead32Pure(encoded, o);
  const uint32_t count = canBusOffRead32Pure(encoded, o);
  const uint32_t eventUptimeMs = canBusOffRead32Pure(encoded, o);
  CanBusOffSnapshotPure snapshot = {};
  snapshot.valid = encoded[o++] != 0;
  snapshot.controllerState = encoded[o++];
  snapshot.errorFlags = encoded[o++];
  snapshot.txFailConsecutive = encoded[o++];
  uint32_t *snapshotValues[] = {
      &snapshot.rxAgeOrGapMs, &snapshot.rxOverflowCount,
      &snapshot.txOk, &snapshot.txFail, &snapshot.msgsToTx,
      &snapshot.msgsToRx, &snapshot.txErrorCounter,
      &snapshot.rxErrorCounter, &snapshot.txFailedCount,
      &snapshot.rxMissedCount, &snapshot.rxOverrunCount,
      &snapshot.arbLostCount, &snapshot.busErrorCount};
  for (uint8_t i = 0; i < 13; ++i)
    *snapshotValues[i] = canBusOffRead32Pure(encoded, o);
  if (version >= CAN_BUS_OFF_RECORD_VERSION_PURE) {
    uint32_t *alertValues[] = {
        &snapshot.alertSeenMask, &snapshot.alertBatchMask,
        &snapshot.txFailedAlertAgeMs, &snapshot.errPassAlertAgeMs,
        &snapshot.busErrorAlertAgeMs};
    for (uint8_t i = 0; i < 5; ++i)
      *alertValues[i] = canBusOffRead32Pure(encoded, o);
  }
  const uint8_t traceCount = encoded[o++];
  o += 3u;
  if (traceCount > CAN_BUS_OFF_TRACE_CAPACITY_PURE ||
      length != headerBytes +
          (size_t)traceCount * CAN_BUS_OFF_TRACE_ENTRY_BYTES_PURE + 4u)
    return false;
  const uint32_t expectedCrc = canBusOffCrc32Pure(encoded, length - 4u);
  size_t crcOffset = length - 4u;
  if (canBusOffRead32Pure(encoded, crcOffset) != expectedCrc) return false;

  out = {};
  out.bus = bus;
  out.generation = generation;
  out.count = count;
  out.eventUptimeMs = eventUptimeMs;
  out.snapshot = snapshot;
  out.traceCount = traceCount;
  for (uint8_t i = 0; i < traceCount; ++i) {
    CanBusOffTraceEntryPure &entry = out.trace[i];
    entry.seq = canBusOffRead32Pure(encoded, o);
    entry.capturedMs = canBusOffRead32Pure(encoded, o);
    entry.id = canBusOffRead16Pure(encoded, o);
    entry.dlc = encoded[o++];
    entry.source = encoded[o++];
    entry.reason = encoded[o++];
    o += 3u;
    entry.result = (int32_t)canBusOffRead32Pure(encoded, o);
    for (uint8_t j = 0; j < 8; ++j) entry.data[j] = encoded[o++];
  }
  return o == length - 4u;
}
