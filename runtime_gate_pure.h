#pragma once
#include <stdint.h>

// Host-testable AP state validity helpers. AP/NOA/manual state remains latched
// until the active topology receives a newer DAS_status frame or the relevant
// CAN recovery boundary invalidates that authorization.
static inline bool dasStateApActivePure(bool valid, uint8_t state4) {
  if (!valid) return false;
  return state4 == 3 || state4 == 4 || state4 == 5 || state4 == 6;
}

static inline bool dasStateNoaPure(bool valid, uint8_t state4) {
  return valid && state4 == 5;
}

static inline bool dasStateAutosteerPure(bool valid, uint8_t state4) {
  return valid && state4 == 3;
}

static inline bool dasStateManualPure(bool valid, uint8_t state4) {
  if (!valid) return false;
  return state4 == 0 || state4 == 1 || state4 == 8 || state4 == 9 || state4 == 14;
}

enum NagSkipReasonPure : uint8_t {
  NAG_SKIP_NONE = 0,
  NAG_SKIP_DISABLED = 1,
  NAG_SKIP_BOOT_DELAY = 2,
  NAG_SKIP_WARMUP = 3,
  NAG_SKIP_SELF_FRAME = 4,
  NAG_SKIP_HANDS_ON = 5,
  NAG_SKIP_AP_INVALID = 6,
  NAG_SKIP_AP_INACTIVE = 7,
  NAG_SKIP_DECISION = 8,
  NAG_SKIP_STOPPED = 9
};

static inline NagSkipReasonPure nagEligibilityReasonPure(
    bool enabled, bool bootDelayPassed, bool canSeen, bool isOurs,
    uint8_t handsOnState, bool apStateValid, bool apActive) {
  if (!enabled) return NAG_SKIP_DISABLED;
  if (!bootDelayPassed) return NAG_SKIP_BOOT_DELAY;
  if (!canSeen) return NAG_SKIP_WARMUP;
  if (isOurs) return NAG_SKIP_SELF_FRAME;
  if (handsOnState > 1) return NAG_SKIP_HANDS_ON;
  if (!apStateValid) return NAG_SKIP_AP_INVALID;
  if (!apActive) return NAG_SKIP_AP_INACTIVE;
  return NAG_SKIP_NONE;
}

static inline uint32_t nagGapMaxUpdatePure(uint32_t previousTxMs, uint32_t nowMs,
                                           uint32_t currentMaxMs) {
  if (previousTxMs == 0) return currentMaxMs;
  const uint32_t gap = (uint32_t)(nowMs - previousTxMs);
  return gap > currentMaxMs ? gap : currentMaxMs;
}

// Tesla Party CAN 0x257 DI_vehicleSpeed: Intel bit 12, length 12,
// factor 0.08 km/h, offset -40 km/h. Raw 4095 is SNA.
static inline bool nagDecodePartySpeedRawPure(const uint8_t *data, uint8_t dlc, uint16_t &rawOut) {
  if (!data || dlc < 3) return false;
  const uint16_t raw = (uint16_t)(((uint16_t)(data[1] >> 4) & 0x0Fu) | ((uint16_t)data[2] << 4));
  if (raw == 0x0FFFu) return false;
  rawOut = raw;
  return true;
}

static inline int32_t nagPartySpeedKphX100Pure(uint16_t raw) {
  // (raw * 0.08 - 40.0) * 100 = raw * 8 - 4000.
  return (int32_t)raw * 8 - 4000;
}

static inline bool nagPauseAtZeroBlocksPure(bool optionEnabled, bool speedValid,
                                             bool speedFresh, uint16_t raw) {
  return optionEnabled && speedValid && speedFresh && raw == 500u;
}


enum McpTxResultReason : uint8_t {
  MCP_TX_OK = 0,
  MCP_TX_INVALID_MSG = 1,
  MCP_TX_MUTEX_BUSY = 2,
  MCP_TX_MCP_NOT_READY = 3,
  MCP_TX_EPOCH_MISMATCH = 4,
  MCP_TX_FRESH_MASK = 5,
  MCP_TX_SEND_ERROR = 6
};

static inline McpTxResultReason mcpTxResultReasonPure(
    bool msgValid, bool mutexAcquired, bool mcpReady, uint32_t currentEpoch,
    uint8_t freshMask, uint32_t expectedEpoch, bool sendOk) {
  if (!msgValid) return MCP_TX_INVALID_MSG;
  if (!mutexAcquired) return MCP_TX_MUTEX_BUSY;
  if (!mcpReady) return MCP_TX_MCP_NOT_READY;
  if (currentEpoch != expectedEpoch) return MCP_TX_EPOCH_MISMATCH;
  if (freshMask != 0x03u) return MCP_TX_FRESH_MASK;
  if (!sendOk) return MCP_TX_SEND_ERROR;
  return MCP_TX_OK;
}
