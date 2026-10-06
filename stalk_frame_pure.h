#pragma once

#include <stdint.h>
#include <string.h>

#include "auto_blinker_pure.h"

static inline bool stalkFramePreparePure(const uint8_t *stock, uint8_t dlc,
                                         uint8_t counter, uint8_t turn,
                                         uint8_t out[4]) {
  if (!stock || dlc < 4u || !out) return false;
  memcpy(out, stock, 4u);
  out[1] = (uint8_t)((out[1] & 0xF0u) | (counter & 0x0Fu));
  out[2] = (uint8_t)((out[2] & 0xF0u) | (turn & 0x0Fu));
  out[0] = leftStalkChecksumPure(out, counter);
  return true;
}
