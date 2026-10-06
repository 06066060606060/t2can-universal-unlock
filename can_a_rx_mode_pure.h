#pragma once
#include <stdint.h>

enum CanARxMode : uint8_t {
  CAN_A_RX_PREFETCH_4 = 0,
  CAN_A_RX_IMMEDIATE = 1,
};

static inline bool canARxModeValidPure(uint8_t mode) {
  return mode == CAN_A_RX_PREFETCH_4 || mode == CAN_A_RX_IMMEDIATE;
}

static inline uint8_t canARxReadBatchBudgetPure(bool labEnabled, uint8_t savedMode) {
  return labEnabled && savedMode == CAN_A_RX_IMMEDIATE ? 1u : 4u;
}
