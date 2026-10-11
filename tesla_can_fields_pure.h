#pragma once
#include <stdint.h>

// Byte-level primitives only; feature-specific validity and freshness remain
// in their callers. The caller owns the non-null seven-byte payload contract.
static inline uint8_t teslaAdditiveChecksumPure(uint16_t canId, const uint8_t *data) {
  uint16_t sum = (uint16_t)(canId >> 8) + (uint16_t)(canId & 0xFFu);
  for (uint8_t i = 0; i < 7u; ++i) sum += data[i];
  return (uint8_t)sum;
}

static inline bool teslaApStateActivePure(uint8_t state) {
  return state == 3u || state == 4u || state == 5u || state == 6u;
}
