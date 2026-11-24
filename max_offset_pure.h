#pragma once
#include <stdint.h>
#include "tesla_can_fields_pure.h"

// UI_fsdMaxSpeedOffsetPercentage on UI_autopilotControl 0x3FD mux 2.
// Byte 1 bits 0-5. The manual screen steps only changed this byte:
// 10% = 82 0A 1E 38 10 00 00 90, 5% = 82 05 ..., 0% = 82 00 ....
// 10% is byte 1 = 0x0A. 20% does not take effect, so it is not offered.
// The other bytes stay as the car sent them.
static constexpr uint32_t MAX_OFFSET_FRESH_MS_PURE = 3000u;
static constexpr uint8_t MAX_OFFSET_DEFAULT_PERCENT_PURE = 10u;

static inline bool maxOffsetPercentAllowedPure(uint8_t percent) {
  switch (percent) {
    case 0u: case 5u: case 10u: return true;
    default: return false;
  }
}

static inline bool maxOffsetFramePure(uint32_t id, uint8_t dlc, bool extended,
                                      bool remote, const uint8_t *data) {
  return id == 0x3FDu && dlc == 8u && !extended && !remote && data &&
      (data[0] & 7u) == 2u;
}

static inline int8_t maxOffsetReadPure(const uint8_t *data) {
  return (int8_t)(data[1] & 0x3Fu);
}

static inline bool maxOffsetActivePure(bool enabled, bool apValid, uint8_t apState,
                                       uint32_t now, uint32_t apMs) {
  return enabled && apValid && teslaApStateActivePure(apState) &&
      (uint32_t)(now - apMs) <= MAX_OFFSET_FRESH_MS_PURE;
}

// Clearing UI_applyDCASBehavior before Autopilot is engaged keeps Autopilot
// from turning on. The clear is allowed only while Autopilot is already active.
static inline bool dcasClearActivePure(bool enabled, bool apValid, uint8_t apState,
                                      uint32_t now, uint32_t apMs) {
  return enabled && apValid && teslaApStateActivePure(apState) &&
      (uint32_t)(now - apMs) <= MAX_OFFSET_FRESH_MS_PURE;
}

// Writes the selected percent into mux 2 byte 1. Returns true when the
// clone should be transmitted, including when the stock percent already
// matches, so the car's own frame does not remain the latest value.
static inline bool maxOffsetApplyPure(uint8_t *data, uint8_t dlc, bool active,
                                      uint8_t percent) {
  if (!active || !data || dlc != 8u || (data[0] & 7u) != 2u) return false;
  if (!maxOffsetPercentAllowedPure(percent)) return false;
  data[1] = (uint8_t)((data[1] & (uint8_t)~0x3Fu) | percent);
  return true;
}

// UI_autopilotDrivingProfile on mux 2, bits 60-62 (byte 7 bits 4-6).
// 0 Chill, 1 Normal, 2 Assertive, 3 Mad Max, 4 Conservative.
// 0xFF means leave the car's value. Bit 63 (mux valid) stays as received.
static constexpr uint8_t DRIVING_PROFILE_STOCK_PURE = 0xFFu;
static constexpr uint8_t DRIVING_PROFILE_MAD_MAX_PURE = 3u;

static inline bool drivingProfileAllowedPure(uint8_t profile) {
  return profile <= 4u;
}

static inline uint8_t drivingProfileReadPure(const uint8_t *data) {
  return (uint8_t)((data[7] >> 4) & 7u);
}

static inline bool drivingProfileApplyPure(uint8_t *data, uint8_t dlc, uint8_t profile) {
  if (!data || dlc != 8u || (data[0] & 7u) != 2u) return false;
  if (!drivingProfileAllowedPure(profile)) return false;
  data[7] = (uint8_t)((data[7] & (uint8_t)0x8Fu) | (uint8_t)(profile << 4));
  return true;
}

// UI_applyDCASBehavior on mux 0 bit 60. The 2026.26.6.5 layout names this
// flag; the catalogue marks that name as plausible. Clearing it asks the
// Autopilot stack to stop applying the European DCAS ruleset.
// Returns false when the bit is already 0, so an echo is not sent again.
static inline int8_t dcasBehaviorReadPure(const uint8_t *data) {
  if (!data || (data[0] & 7u) != 0u) return -1;
  return (int8_t)((data[7] >> 4) & 1u);
}

static inline bool dcasBehaviorClearPure(uint8_t *data, uint8_t dlc) {
  if (!data || dlc != 8u || (data[0] & 7u) != 0u) return false;
  if (((data[7] >> 4) & 1u) == 0u) return false;
  data[7] = (uint8_t)(data[7] & (uint8_t)~0x10u);
  return true;
}

struct MaxOffsetStockPure {
  bool valid;
  uint8_t raw[8];
  uint32_t lastMs, epoch, rx;
  uint8_t profile, topology;
};
