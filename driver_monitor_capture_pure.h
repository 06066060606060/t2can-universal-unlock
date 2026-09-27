#pragma once
#include <stdint.h>

namespace driver_monitor_capture_pure {

enum : uint8_t {
  BUS_A = 0,
  BUS_B = 1,
};

static inline bool targetFrame(uint8_t bus, uint16_t id) {
  if (bus != BUS_A && bus != BUS_B) return false;
  return id == 0x389 || id == 0x5D9 || id == 0x247 || id == 0x399 || id == 0x370;
}

// DAS_status2.DAS_driverInteractionLevel = start bit 38, length 2, little endian.
static inline uint8_t driverMonitorInteractionLevelPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 5) return 0xFF;
  return (uint8_t)((data[4] >> 6) & 0x03u);
}

// DAS_status.DAS_autopilotHandsOnState = start bit 42, length 4, little endian.
static inline uint8_t driverMonitorDasHandsOnPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 6) return 0xFF;
  return (uint8_t)((data[5] >> 2) & 0x0Fu);
}

// EPAS_sysStatus handsOnLevel = bits 38..39; torsion torque raw = bits 16..27.
static inline uint8_t driverMonitorEpasHandsOnPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 5) return 0xFF;
  return (uint8_t)((data[4] >> 6) & 0x03u);
}

static inline uint16_t driverMonitorEpasTorqueRawPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 4) return 0xFFFFu;
  return (uint16_t)((((uint16_t)data[2] & 0x0Fu) << 8) | data[3]);
}

static inline int16_t driverMonitorEpasTorqueCentiNmPure(const uint8_t *data, uint8_t dlc) {
  const uint16_t raw = driverMonitorEpasTorqueRawPure(data, dlc);
  if (raw == 0xFFFFu) return INT16_MIN;
  return (int16_t)((int32_t)raw - 2050);
}

} // namespace driver_monitor_capture_pure
