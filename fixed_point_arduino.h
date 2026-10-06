#pragma once
#include "fixed_point_pure.h"

static inline String fixedPointString(int32_t scaled, uint8_t decimals) {
  char buf[24] = {};
  if (!formatFixedPure(scaled, decimals, buf, sizeof(buf))) return String("0");
  return String(buf);
}

static inline String centiString(int32_t centi) {
  return fixedPointString(centi, 2u);
}
