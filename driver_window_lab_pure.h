#pragma once

#include <stdint.h>
#include <string.h>
#include "vehicle_profile.h"

static constexpr uint32_t DRIVER_WINDOW_STOCK_FRESH_MS = 250u;
static constexpr uint32_t DRIVER_WINDOW_REQUEST_TIMEOUT_MS = 1000u;
static constexpr uint8_t DRIVER_WINDOW_PRESS_FRAMES = 2u;
static constexpr uint8_t DRIVER_WINDOW_AUTO_DOWN_LF_MASK = 0x0Cu;
static constexpr uint8_t DRIVER_WINDOW_AUTO_DOWN_LF_IDLE = 0x04u;
static constexpr uint8_t DRIVER_WINDOW_AUTO_DOWN_LF_ACTIVE = 0x08u;

static inline bool driverWindowLabProfileSupportedPure(
    uint8_t profileId, uint8_t topology) {
  return profileId == VEHICLE_MODEL_YL &&
         topology == VEHICLE_TOPOLOGY_YL_PARTY_VH;
}

enum DriverWindowArmResultPure : uint8_t {
  DRIVER_WINDOW_ARM_OK = 0,
  DRIVER_WINDOW_ARM_LAB_DISABLED,
  DRIVER_WINDOW_ARM_UNSUPPORTED_PROFILE,
  DRIVER_WINDOW_ARM_GEAR_STALE,
  DRIVER_WINDOW_ARM_NOT_PARKED,
  DRIVER_WINDOW_ARM_STOCK_MISSING,
  DRIVER_WINDOW_ARM_STOCK_STALE,
  DRIVER_WINDOW_ARM_PHYSICAL_INPUT,
  DRIVER_WINDOW_ARM_CAN_UNAVAILABLE,
  DRIVER_WINDOW_ARM_BUSY,
};

enum DriverWindowConsumeReasonPure : uint8_t {
  DRIVER_WINDOW_CONSUME_NONE = 0,
  DRIVER_WINDOW_CONSUME_PHYSICAL_INPUT,
  DRIVER_WINDOW_CONSUME_EPOCH_CHANGED,
  DRIVER_WINDOW_CONSUME_TIMEOUT,
  DRIVER_WINDOW_CONSUME_LAB_DISABLED,
  DRIVER_WINDOW_CONSUME_UNSUPPORTED_PROFILE,
  DRIVER_WINDOW_CONSUME_GEAR_STALE,
  DRIVER_WINDOW_CONSUME_NOT_PARKED,
  DRIVER_WINDOW_CONSUME_STOCK_STALE,
  DRIVER_WINDOW_CONSUME_CAN_UNAVAILABLE,
  DRIVER_WINDOW_CONSUME_TX_FAILED,
};

struct DriverWindowArmContextPure {
  bool labEnabled;
  bool modelYlPartyVh;
  bool gearFresh;
  bool parked;
  bool stockValid;
  uint32_t stockAgeMs;
  bool stockInputIdle;
  bool canTxAllowed;
  uint32_t guardGeneration;
};

struct DriverWindowLabStatePure {
  bool pending;
  uint8_t framesSent;
  uint32_t requestedMs;
  uint32_t txEpoch;
  uint32_t guardGeneration;
};

struct DriverWindowConsumeResultPure {
  bool transmit;
  bool completed;
  bool blocked;
  uint8_t reason;
  uint32_t guardGeneration;
  uint8_t data[8];
};

static inline bool driverWindowSetAutoDownLfPure(uint8_t data[8], bool enabled) {
  if (data == nullptr) return false;
  data[6] = (uint8_t)((data[6] & (uint8_t)~DRIVER_WINDOW_AUTO_DOWN_LF_MASK) |
                      (enabled ? DRIVER_WINDOW_AUTO_DOWN_LF_ACTIVE
                               : DRIVER_WINDOW_AUTO_DOWN_LF_IDLE));
  return true;
}

static inline bool driverWindowMux0Pure(const uint8_t data[8]) {
  return data != nullptr && (data[0] & 0x03u) == 0u;
}

static inline bool driverWindowPhysicalInputIdlePure(const uint8_t data[8]) {
  return data != nullptr && data[4] == 0u && data[5] == 0u &&
         (data[6] & DRIVER_WINDOW_AUTO_DOWN_LF_MASK) ==
             DRIVER_WINDOW_AUTO_DOWN_LF_IDLE;
}

static inline void driverWindowLabResetPure(DriverWindowLabStatePure &state) {
  state = {};
}

static inline DriverWindowArmResultPure driverWindowLabArmPure(
    DriverWindowLabStatePure &state, uint32_t now, uint32_t txEpoch,
    const DriverWindowArmContextPure &context) {
  if (!context.labEnabled) return DRIVER_WINDOW_ARM_LAB_DISABLED;
  if (!context.modelYlPartyVh) return DRIVER_WINDOW_ARM_UNSUPPORTED_PROFILE;
  if (!context.gearFresh) return DRIVER_WINDOW_ARM_GEAR_STALE;
  if (!context.parked) return DRIVER_WINDOW_ARM_NOT_PARKED;
  if (!context.stockValid) return DRIVER_WINDOW_ARM_STOCK_MISSING;
  if (context.stockAgeMs > DRIVER_WINDOW_STOCK_FRESH_MS)
    return DRIVER_WINDOW_ARM_STOCK_STALE;
  if (!context.stockInputIdle) return DRIVER_WINDOW_ARM_PHYSICAL_INPUT;
  if (!context.canTxAllowed) return DRIVER_WINDOW_ARM_CAN_UNAVAILABLE;
  if (state.pending) return DRIVER_WINDOW_ARM_BUSY;
  state.pending = true;
  state.framesSent = 0u;
  state.requestedMs = now;
  state.txEpoch = txEpoch;
  state.guardGeneration = context.guardGeneration;
  return DRIVER_WINDOW_ARM_OK;
}

static inline DriverWindowConsumeResultPure driverWindowLabConsumeStockPure(
    DriverWindowLabStatePure &state, uint32_t now, uint32_t txEpoch,
    const DriverWindowArmContextPure &context,
    const uint8_t *stock, uint8_t dlc) {
  DriverWindowConsumeResultPure result = {};
  if (!state.pending) return result;
  if (txEpoch != state.txEpoch) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_EPOCH_CHANGED;
    return result;
  }
  if ((uint32_t)(now - state.requestedMs) > DRIVER_WINDOW_REQUEST_TIMEOUT_MS) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_TIMEOUT;
    return result;
  }
  if (!context.labEnabled) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_LAB_DISABLED;
    return result;
  }
  if (!context.modelYlPartyVh) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_UNSUPPORTED_PROFILE;
    return result;
  }
  if (!context.gearFresh) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_GEAR_STALE;
    return result;
  }
  if (!context.parked) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_NOT_PARKED;
    return result;
  }
  if (!context.stockValid || context.stockAgeMs > DRIVER_WINDOW_STOCK_FRESH_MS) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_STOCK_STALE;
    return result;
  }
  if (!context.canTxAllowed) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_CAN_UNAVAILABLE;
    return result;
  }
  if (stock == nullptr || dlc < 8u || !driverWindowMux0Pure(stock))
    return result;
  if (!driverWindowPhysicalInputIdlePure(stock)) {
    driverWindowLabResetPure(state);
    result.blocked = true;
    result.reason = DRIVER_WINDOW_CONSUME_PHYSICAL_INPUT;
    return result;
  }

  memcpy(result.data, stock, sizeof(result.data));
  driverWindowSetAutoDownLfPure(result.data, true);
  result.transmit = true;
  result.guardGeneration = state.guardGeneration;
  if (state.framesSent < UINT8_MAX) state.framesSent++;
  if (state.framesSent >= DRIVER_WINDOW_PRESS_FRAMES) {
    state.pending = false;
    result.completed = true;
  }
  return result;
}
