#pragma once

#include <stdint.h>

static constexpr uint8_t R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE = 43u;

// LAB experiment: when enabled, clear only 0x3FD mux1 bit43.
// The caller is responsible for mux/profile/LAB gating; current firmware
// enables it on every Universal profile with the existing 0x3FD/R79 route.
static inline void r79DmsNagLabApplyPure(uint8_t data[8], bool enabled) {
  if (!data || !enabled) return;
  data[R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE / 8u] = (uint8_t)(
      data[R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE / 8u] &
      (uint8_t)~(1u << (R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE % 8u)));
}
