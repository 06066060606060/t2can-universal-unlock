#pragma once
#include "vision_speed_control_pure.h"

// Existing isolated Lane Graph/R79 harnesses run with the new option OFF.
// New vision3fd integration tests exercise the full enabled runtime separately.
[[maybe_unused]] static bool visionControlApplyFinal(uint8_t *data, uint8_t) {
  return visionControlApplyPure(data, 8u, false);
}
[[maybe_unused]] static void visionControlRecordTx(bool, uint8_t, bool) {}
[[maybe_unused]] static void visionControlCacheStock(uint8_t, const uint8_t *, uint32_t, uint32_t) {}
