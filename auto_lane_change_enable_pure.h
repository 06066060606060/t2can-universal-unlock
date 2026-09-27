#pragma once
#include <stdint.h>
#include <stddef.h>

static constexpr uint16_t UI_CHASSIS_CONTROL_ID_PURE = 0x293;
static constexpr uint8_t UI_AUTO_LANE_CHANGE_ON_PURE = 1;
static constexpr uint8_t UI_AUTO_LANE_CHANGE_SNA_PURE = 3;

// Independent physical-bus target for the experimental 0x293 overlay.
// BOTH preserves the pre-b22 behavior where each observed same-bus frame
// could be overlaid independently on CAN A and CAN B.
static constexpr uint8_t UI_AUTO_LC_BUS_A_PURE = 1;
static constexpr uint8_t UI_AUTO_LC_BUS_B_PURE = 2;
static constexpr uint8_t UI_AUTO_LC_BUS_BOTH_PURE = 3;

static inline bool uiAutoLaneChangeBusTargetValidPure(uint8_t targetBus) {
  return targetBus == UI_AUTO_LC_BUS_A_PURE ||
         targetBus == UI_AUTO_LC_BUS_B_PURE ||
         targetBus == UI_AUTO_LC_BUS_BOTH_PURE;
}

static inline bool uiAutoLaneChangeBusAllowedPure(uint8_t targetBus, uint8_t physicalBus) {
  if (!uiAutoLaneChangeBusTargetValidPure(targetBus)) return false;
  if (physicalBus != UI_AUTO_LC_BUS_A_PURE && physicalBus != UI_AUTO_LC_BUS_B_PURE) return false;
  return targetBus == UI_AUTO_LC_BUS_BOTH_PURE || targetBus == physicalBus;
}

static inline uint8_t uiAutoLaneChangeReadRawPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 4) return 0xFF;
  return (uint8_t)(data[3] & 0x03u);
}

static inline bool uiAutoLaneChangeGateOpenPure(bool labEnabled, bool featureEnabled,
                                                 bool dasStateValid, uint8_t dasState4) {
  if (!labEnabled || !featureEnabled || !dasStateValid) return false;
  return dasState4 == 3 || dasState4 == 4 || dasState4 == 5 || dasState4 == 6;
}

// Tesla 0x293 checksum: low CAN-ID byte + high CAN-ID byte + data[0..6], modulo 256.
static inline uint8_t ui293ChecksumPure(const uint8_t *data) {
  if (!data) return 0;
  uint16_t sum = (uint16_t)(UI_CHASSIS_CONTROL_ID_PURE & 0xFFu) +
                 (uint16_t)(UI_CHASSIS_CONTROL_ID_PURE >> 8);
  for (uint8_t i = 0; i < 7; ++i) sum = (uint16_t)(sum + data[i]);
  return (uint8_t)(sum & 0xFFu);
}

// Force UI_autoLaneChangeEnable (bits 24..25 = byte[3] bits 0..1) to ON,
// advance the 4-bit counter at bits 52..55, then refresh checksum byte[7].
static inline bool uiAutoLaneChangeFinalizePure(uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 8) return false;
  data[3] = (uint8_t)((data[3] & 0xFCu) | UI_AUTO_LANE_CHANGE_ON_PURE);
  const uint8_t counter = (uint8_t)(((data[6] >> 4) + 1u) & 0x0Fu);
  data[6] = (uint8_t)((data[6] & 0x0Fu) | (uint8_t)(counter << 4));
  data[7] = ui293ChecksumPure(data);
  return true;
}
