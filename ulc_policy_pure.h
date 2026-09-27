#pragma once
#include <stdint.h>
#include <stddef.h>

static constexpr uint8_t UI_ULC_POLICY_STOCK_PURE = 0xFFu;

static inline uint8_t uiUlcOffHighwayReadRawPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 2) return UI_ULC_POLICY_STOCK_PURE;
  return (uint8_t)((data[1] >> 7) & 0x01u);  // bit 15
}

static inline bool uiUlcOffHighwayApplyRawPure(uint8_t *data, uint8_t dlc, uint8_t raw) {
  if (!data || dlc < 2 || raw > 1u) return false;
  if (raw) data[1] = (uint8_t)(data[1] | 0x80u);
  else     data[1] = (uint8_t)(data[1] & (uint8_t)~0x80u);
  return true;
}

static inline bool ulcPolicyApGateOpenPure(bool selectionActive,
                                            bool dasStateValid, uint8_t dasState4) {
  if (!selectionActive || !dasStateValid) return false;
  return dasState4 == 3u || dasState4 == 4u || dasState4 == 5u || dasState4 == 6u;
}
