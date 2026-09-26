#pragma once

#include <stdint.h>

static constexpr uint8_t ULC_COMPOSITE_STOCK_PURE = 0xFFu;

struct UlcCompositeSelectionPure {
  bool alcOffHighwayEnabled;
  uint8_t ulcOffHighwayMode;
  uint8_t blindSpotMode;
  bool confirmFreeEnabled;
};

struct UlcCompositeGatesPure {
  bool alcAutosteerOpen;
  bool ulcApOpen;
  bool confirmFreeOpen;
};

struct UlcCompositeResultPure {
  bool changed;
  bool alcOffHighwayChanged;
  bool ulcOffHighwayChanged;
  bool blindSpotChanged;
  bool confirmFreeChanged;
};

static inline UlcCompositeResultPure ulcCompose3f8Pure(
    uint8_t *data, uint8_t dlc, const UlcCompositeSelectionPure &selected,
    const UlcCompositeGatesPure &gates) {
  UlcCompositeResultPure result = {};
  if (!data || dlc < 8 ||
      (selected.ulcOffHighwayMode != ULC_COMPOSITE_STOCK_PURE &&
       selected.ulcOffHighwayMode > 1u) ||
      (selected.blindSpotMode != ULC_COMPOSITE_STOCK_PURE &&
       selected.blindSpotMode > 2u)) {
    return result;
  }

  if (selected.alcOffHighwayEnabled && gates.alcAutosteerOpen &&
      (data[7] & 0x01u) == 0) {
    data[7] = (uint8_t)(data[7] | 0x01u);
    result.alcOffHighwayChanged = true;
  }

  if (selected.ulcOffHighwayMode != ULC_COMPOSITE_STOCK_PURE &&
      gates.ulcApOpen) {
    const uint8_t stock = (uint8_t)((data[1] >> 7) & 0x01u);
    if (stock != selected.ulcOffHighwayMode) {
      if (selected.ulcOffHighwayMode != 0)
        data[1] = (uint8_t)(data[1] | 0x80u);
      else
        data[1] = (uint8_t)(data[1] & (uint8_t)~0x80u);
      result.ulcOffHighwayChanged = true;
    }
  }

  if (selected.blindSpotMode != ULC_COMPOSITE_STOCK_PURE && gates.ulcApOpen) {
    const uint8_t stock = (uint8_t)((data[6] >> 4) & 0x03u);
    if (stock != selected.blindSpotMode) {
      data[6] = (uint8_t)((data[6] & (uint8_t)~0x30u) |
                          (uint8_t)(selected.blindSpotMode << 4));
      result.blindSpotChanged = true;
    }
  }

  if (selected.confirmFreeEnabled && gates.confirmFreeOpen &&
      (data[0] & 0x02u) != 0) {
    data[0] = (uint8_t)(data[0] & (uint8_t)~0x02u);
    result.confirmFreeChanged = true;
  }

  result.changed = result.alcOffHighwayChanged ||
      result.ulcOffHighwayChanged || result.blindSpotChanged ||
      result.confirmFreeChanged;
  return result;
}
