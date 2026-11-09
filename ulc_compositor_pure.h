#pragma once
#include "tesla_can_fields_pure.h"
#include <stdint.h>
#include <stddef.h>

// ulc policy pure
static constexpr uint8_t UI_ULC_POLICY_STOCK_PURE = 0xFFu;

static inline uint8_t uiUlcOffHighwayReadRawPure(const uint8_t *data, uint8_t dlc, bool modelS = false) {
  if (!data || dlc < (modelS ? 8u : 2u)) return UI_ULC_POLICY_STOCK_PURE;
  return modelS ? (uint8_t)(data[7] & 1u) : (uint8_t)((data[1] >> 7) & 1u);
}

static inline bool ulcPolicyApGateOpenPure(bool selectionActive,
                                            bool dasStateValid, uint8_t dasState4) {
  if (!selectionActive || !dasStateValid) return false;
  return teslaApStateActivePure(dasState4);
}

// ulc stalk confirm pure
// Production UI_ulcStalkConfirm policy. The feature uses the fixed stock-follow
// CAN B / Chassis 0x3F8 path. Timing remains selectable without a LAB gate.
static constexpr uint8_t ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE = 0u;
static constexpr uint8_t ULC_NO_CONFIRM_TIMING_PRE_AP_PURE = 1u;

static inline bool ulcNoConfirmTimingValidPure(uint8_t timingMode) {
  return timingMode == ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE ||
         timingMode == ULC_NO_CONFIRM_TIMING_PRE_AP_PURE;
}

static inline bool ulcNoConfirmSupportedPure(bool profileAvailable) {
  return profileAvailable;
}

static inline bool ulcNoConfirmGateOpenWithTimingPure(bool profileAvailable,
                                                       bool featureEnabled,
                                                       uint8_t timingMode,
                                                       bool dasStateValid,
                                                       uint8_t dasState4) {
  if (!profileAvailable || !featureEnabled ||
      !ulcNoConfirmTimingValidPure(timingMode)) return false;
  if (timingMode == ULC_NO_CONFIRM_TIMING_PRE_AP_PURE) return true;
  if (!dasStateValid) return false;
  return teslaApStateActivePure(dasState4);
}

// ulc compositor pure
static constexpr uint8_t ULC_COMPOSITE_STOCK_PURE = 0xFFu;
static constexpr uint32_t LAB3F8_FRESH_MS_PURE = 3000u;
static inline bool lab3f8FrameValidPure(uint32_t id, uint8_t dlc, bool extended, bool remote, bool modelS = false) {
  return id == (modelS ? 0x3E8u : 0x3F8u) && dlc == 8u && !extended && !remote;
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
    const UlcCompositeGatesPure &gates, bool modelS = false) {
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

  // Model S 0x3E8 DBC: ALC bit54, ULC bit56, blind-spot bits50..51.
  // Stalk confirmation (bit1) and heartbeat (bits2..3) are unchanged.
  const uint8_t alcByte = modelS ? 6u : 7u;
  const uint8_t alcMask = modelS ? 0x40u : 0x01u;
  const uint8_t ulcByte = modelS ? 7u : 1u;
  const uint8_t ulcMask = modelS ? 0x01u : 0x80u;
  const uint8_t blindShift = modelS ? 2u : 4u;
  const uint8_t blindMask = (uint8_t)(3u << blindShift);

  if (selected.alcOffHighwayEnabled && gates.alcAutosteerOpen &&
      (data[alcByte] & alcMask) == 0) {
    data[alcByte] = (uint8_t)(data[alcByte] | alcMask);
    result.alcOffHighwayChanged = true;
  }

  if (selected.ulcOffHighwayMode != ULC_COMPOSITE_STOCK_PURE &&
      gates.ulcApOpen) {
    const uint8_t stock = (uint8_t)((data[ulcByte] & ulcMask) != 0u);
    if (stock != selected.ulcOffHighwayMode) {
      if (selected.ulcOffHighwayMode != 0)
        data[ulcByte] = (uint8_t)(data[ulcByte] | ulcMask);
      else
        data[ulcByte] = (uint8_t)(data[ulcByte] & (uint8_t)~ulcMask);
      result.ulcOffHighwayChanged = true;
    }
  }

  if (selected.blindSpotMode != ULC_COMPOSITE_STOCK_PURE && gates.ulcApOpen) {
    const uint8_t stock = (uint8_t)((data[6] >> blindShift) & 0x03u);
    if (stock != selected.blindSpotMode) {
      data[6] = (uint8_t)((data[6] & (uint8_t)~blindMask) |
                          (uint8_t)(selected.blindSpotMode << blindShift));
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
