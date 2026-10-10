#include <cassert>
#include <cstdint>
#include <cstring>
#include "../can_busoff_persistence_pure.h"

int main() {
  CanBusOffRecordPure record = {};
  record.bus = CAN_BUS_OFF_BUS_A_PURE;
  record.generation = UINT32_MAX;
  record.count = 4;
  record.eventUptimeMs = 123456;
  record.snapshot.valid = true;
  record.snapshot.errorFlags = 0x20;
  record.snapshot.alertSeenMask = 0x1600;
  record.snapshot.alertBatchMask = 0x1200;
  record.snapshot.txFailedAlertAgeMs = 99;
  record.snapshot.errPassAlertAgeMs = 0;
  record.snapshot.busErrorAlertAgeMs = 7;
  record.traceCount = 1;
  record.trace[0].id = 0x249;
  record.trace[0].source = 2;
  record.trace[0].reason = 7;
  record.trace[0].result = -3;
  uint8_t encoded[CAN_BUS_OFF_RECORD_MAX_BYTES_PURE] = {};
  const size_t length = encodeCanBusOffRecordPure(record, encoded, sizeof(encoded));
  assert(length > 0);

  CanBusOffRecordPure decoded = {};
  assert(decodeCanBusOffRecordPure(encoded, length, CAN_BUS_OFF_BUS_A_PURE, decoded));
  assert(decoded.count == 4 && decoded.trace[0].source == 2);
  assert(decoded.snapshot.errorFlags == 0x20);
  assert(decoded.snapshot.alertSeenMask == 0x1600);
  assert(decoded.snapshot.alertBatchMask == 0x1200);
  assert(decoded.snapshot.txFailedAlertAgeMs == 99);
  assert(decoded.snapshot.errPassAlertAgeMs == 0);
  assert(decoded.snapshot.busErrorAlertAgeMs == 7);
  assert(decoded.trace[0].reason == 7 && decoded.trace[0].result == -3);

  // v2 added five alert-evidence words before traceCount. Old v1 records
  // remain readable and default the new evidence fields to zero/invalid.
  uint8_t v1[CAN_BUS_OFF_RECORD_MAX_BYTES_PURE] = {};
  const size_t v1Length = length - 20u;
  std::memcpy(v1, encoded, 80u);
  v1[4] = (uint8_t)CAN_BUS_OFF_RECORD_VERSION_V1_PURE;
  v1[5] = 0;
  v1[6] = (uint8_t)v1Length;
  v1[7] = (uint8_t)(v1Length >> 8);
  std::memcpy(v1 + 80u, encoded + 100u, length - 104u);
  size_t v1CrcOffset = v1Length - 4u;
  const uint32_t v1Crc = canBusOffCrc32Pure(v1, v1CrcOffset);
  canBusOffWrite32Pure(v1, v1CrcOffset, v1Crc);
  CanBusOffRecordPure decodedV1 = {};
  assert(decodeCanBusOffRecordPure(
      v1, v1Length, CAN_BUS_OFF_BUS_A_PURE, decodedV1));
  assert(decodedV1.snapshot.errorFlags == 0x20);
  assert(decodedV1.snapshot.alertSeenMask == 0u);
  assert(decodedV1.snapshot.alertBatchMask == 0u);

  encoded[length - 1] ^= 0x80;
  assert(!decodeCanBusOffRecordPure(encoded, length, CAN_BUS_OFF_BUS_A_PURE, decoded));
  encoded[length - 1] ^= 0x80;
  assert(!decodeCanBusOffRecordPure(encoded, length, CAN_BUS_OFF_BUS_B_PURE, decoded));

  CanBusOffSlotMetaPure oldSlot = {true, UINT32_MAX};
  CanBusOffSlotMetaPure wrapped = {true, 0};
  assert(canBusOffNewestSlotPure(oldSlot, wrapped) == 1);
  CanBusOffSlotMetaPure invalid = {false, 0};
  assert(canBusOffNewestSlotPure(oldSlot, invalid) == 0);
  assert(canBusOffNextWriteSlotPure(invalid, invalid) == 0);
  assert(canBusOffNextWriteSlotPure(wrapped, invalid) == 1);
  assert(canBusOffNextWriteSlotPure(oldSlot, wrapped) == 0);
}
