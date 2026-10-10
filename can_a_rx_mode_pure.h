#pragma once
#include <stdint.h>

static constexpr uint8_t CAN_A_RX_IMMEDIATE = 1u;
static inline uint8_t canARxReadBatchBudgetPure(bool = false, uint8_t = CAN_A_RX_IMMEDIATE) {
  return 1u;
}
