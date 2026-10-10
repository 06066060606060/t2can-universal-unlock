#pragma once

#include <stdint.h>

static constexpr uint8_t ULC_COMPOSITE_STOCK_PURE = 0xFFu;
static constexpr uint32_t LAB3F8_FRESH_MS_PURE = 3000u;
static inline bool lab3f8FrameValidPure(uint32_t id, uint8_t dlc, bool extended, bool remote) {
  return id == 0x3F8u && dlc == 8u && !extended && !remote;
}

struct UlcCompositeSelectionPure {
  bool alcOffHighwayEnabled;
  uint8_t ulcOffHighwayMode;
  uint8_t blindSpotMode;
  bool confirmFreeEnabled;
  bool summonHeartbeatOverrideEnabled;
  uint8_t summonHeartbeatValue;
};

struct UlcCompositeGatesPure {
  bool alcAutosteerOpen;
  bool ulcApOpen;
  bool confirmFreeOpen;
  bool summonHeartbeatOverrideOpen;
};

struct UlcCompositeResultPure {
  bool changed;
  bool alcOffHighwayChanged;
  bool ulcOffHighwayChanged;
  bool blindSpotChanged;
  bool confirmFreeChanged;
  bool summonHeartbeatApplied;
};

static inline UlcCompositeResultPure ulcCompose3f8Pure(
    uint8_t *data, uint8_t dlc, const UlcCompositeSelectionPure &selected,
    const UlcCompositeGatesPure &gates) {
  UlcCompositeResultPure result = {};
  if (!data || dlc < 8 ||
      (selected.ulcOffHighwayMode != ULC_COMPOSITE_STOCK_PURE &&
       selected.ulcOffHighwayMode > 1u) ||
      (selected.blindSpotMode != ULC_COMPOSITE_STOCK_PURE &&
       selected.blindSpotMode > 2u) ||
      (selected.summonHeartbeatOverrideEnabled &&
       selected.summonHeartbeatValue > 3u)) {
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

  // UI_summonHeartbeat is a two-bit raw field at bits 2-3. Mark it applied
  // whenever its session override gate is open so the caller mirrors every
  // valid stock 0x3F8 frame, even when the selected raw value already matches.
  if (selected.summonHeartbeatOverrideEnabled &&
      gates.summonHeartbeatOverrideOpen) {
    data[0] = (uint8_t)((data[0] & 0xF3u) |
        (uint8_t)(selected.summonHeartbeatValue << 2));
    result.summonHeartbeatApplied = true;
  }

  result.changed = result.alcOffHighwayChanged ||
      result.ulcOffHighwayChanged || result.blindSpotChanged ||
      result.confirmFreeChanged || result.summonHeartbeatApplied;
  return result;
}
