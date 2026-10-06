#pragma once
#include <stdint.h>
#include "vehicle_profile.h"

static constexpr uint32_t VISION_CONTROL_FRESH_MS_PURE = 3000u;
static constexpr uint8_t VISION_CONTROL_CHASSIS_PURE = 0u;
static constexpr uint8_t VISION_CONTROL_BODY_PURE = 1u;
static inline bool visionControlBusSupportedPure(uint8_t profile, uint8_t topology, uint8_t bus) {
  return bus <= 1u && vehicleProfileTopologyValid(profile, topology) &&
      (bus == 0u || vehicleProfileCanAIsBody(profile, topology));
}
static inline bool visionControlFramePure(uint32_t id, uint8_t dlc, bool extended,
                                         bool remote, const uint8_t *data) {
  return id == 0x3FDu && dlc == 8u && !extended && !remote && data && (data[0] & 7u) == 1u;
}
static inline bool visionControlGatePure(bool selected, bool lab, bool supported,
    bool transport, bool apValid, uint8_t apState, uint32_t now, uint32_t apMs,
    bool stockValid, uint32_t stockMs) {
  return selected && lab && supported && transport && apValid && apState >= 3u && apState <= 6u &&
      (uint32_t)(now - apMs) <= VISION_CONTROL_FRESH_MS_PURE && stockValid &&
      (uint32_t)(now - stockMs) <= VISION_CONTROL_FRESH_MS_PURE;
}
static inline bool visionControlApplyPure(uint8_t *data, uint8_t dlc, bool active) {
  if (!active || !data || dlc != 8u || (data[0] & 7u) != 1u || (data[6] & 0x02u) == 0u) return false;
  data[6] &= 0xFDu;
  return true;
}
