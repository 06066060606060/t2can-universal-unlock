#pragma once
#include <stdint.h>

static constexpr uint8_t DAS399_AP_STATE_BYTE_PURE = 0u;
static constexpr uint8_t DAS399_AP_STATE_SHIFT_PURE = 0u;
static constexpr uint8_t DAS399_AP_STATE_MASK_PURE = 0x0Fu;
static constexpr uint8_t DAS399_HANDS_ON_BYTE_PURE = 5u;
static constexpr uint8_t DAS399_HANDS_ON_SHIFT_PURE = 2u;
static constexpr uint8_t DAS399_HANDS_ON_MASK_PURE = 0x0Fu;

struct DasStatus399Pure {
  bool valid;
  uint8_t apState;
  uint8_t handsOnState;
};

static inline uint8_t dasStatus399ReadApStatePure(const uint8_t *data) {
  return (uint8_t)((data[DAS399_AP_STATE_BYTE_PURE] >>
                    DAS399_AP_STATE_SHIFT_PURE) &
                   DAS399_AP_STATE_MASK_PURE);
}

static inline uint8_t dasStatus399ReadHandsOnStatePure(const uint8_t *data) {
  return (uint8_t)((data[DAS399_HANDS_ON_BYTE_PURE] >>
                    DAS399_HANDS_ON_SHIFT_PURE) &
                   DAS399_HANDS_ON_MASK_PURE);
}

static inline DasStatus399Pure dasStatus399DecodePure(const uint8_t *data,
                                                       uint8_t dlc) {
  DasStatus399Pure out = {false, 0xFFu, 0xFFu};
  if (!data || dlc <= DAS399_HANDS_ON_BYTE_PURE) return out;
  out.valid = true;
  out.apState = dasStatus399ReadApStatePure(data);
  out.handsOnState = dasStatus399ReadHandsOnStatePure(data);
  return out;
}

static inline bool dasStatus399SelectorDecodePure(
    const uint8_t *data, uint8_t dlc,
    uint8_t apByte, uint8_t apShift, uint8_t apMask,
    uint8_t handsOnByte, uint8_t handsOnShift, uint8_t handsOnMask,
    uint8_t &apOut, uint8_t &handsOnOut) {
  if (!data || apByte >= dlc || handsOnByte >= dlc ||
      apShift >= 8u || handsOnShift >= 8u) return false;
  const uint8_t ap = (uint8_t)((data[apByte] >> apShift) & apMask);
  const uint8_t handsOn =
      (uint8_t)((data[handsOnByte] >> handsOnShift) & handsOnMask);
  apOut = ap;
  handsOnOut = handsOn;
  return true;
}
