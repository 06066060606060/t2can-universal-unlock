#pragma once

#include <stdint.h>

static constexpr uint8_t R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE = 43u;

enum R79DmsStockDispositionPure : uint8_t {
  R79_DMS_STOCK_NONE_PURE = 0,
  R79_DMS_STOCK_R79_OWNED_PURE = 1,
  R79_DMS_STOCK_DMS_ONLY_PURE = 2,
};

static inline bool r79DmsTorqueModeSupportedPure(uint8_t mode) {
  return mode == 0u || mode == 1u || mode == 3u || mode == 7u;
}

static inline bool r79DmsPolicyActivePure(
    bool nagMasterEnabled, bool dmsEnabled, bool profileSupported,
    bool apStateValid, bool apActive, uint8_t nagMethod,
    uint8_t torqueMode) {
  if (!nagMasterEnabled || !dmsEnabled || !profileSupported ||
      !apStateValid || !apActive) return false;
  if (nagMethod == 1u) return true;  // TSL9
  return nagMethod == 0u && r79DmsTorqueModeSupportedPure(torqueMode);
}

static inline bool r79DmsApplyFinalOverlayPure(uint8_t data[8], bool active) {
  if (!data || !active || (data[0] & 0x07u) != 1u) return false;
  const bool changed = (data[5] & 0x08u) != 0u;  // bit43
  data[5] &= (uint8_t)~0x08u;
  return changed;
}

static inline R79DmsStockDispositionPure r79DmsStockDispositionPure(
    bool mux1, bool dmsActive, bool r79Claimed, bool r79WorkPending) {
  if (!mux1) return R79_DMS_STOCK_NONE_PURE;
  if (r79Claimed) return R79_DMS_STOCK_R79_OWNED_PURE;
  if (dmsActive && !r79WorkPending) return R79_DMS_STOCK_DMS_ONLY_PURE;
  return R79_DMS_STOCK_NONE_PURE;
}
