#pragma once
#include <stdint.h>
#include "das_status_pure.h"

// DAS_suppressSpeedWarning is bit 13, independent of Hands-On/Nag state.
// Keep the existing active-AP safety boundary and leave all other bits intact.
static inline bool isaSuppressSpeedWarningPure(uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 8u) return false;
  const uint8_t apState = dasStatus399ReadApStatePure(data);
  if (apState < 3u || apState > 6u || (data[1] & 0x20u) != 0u)
    return false;
  data[1] |= 0x20u;
  return true;
}
