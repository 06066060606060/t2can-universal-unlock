#pragma once
#include <stdint.h>

static constexpr uint8_t SCCM249_CKSUM_CTR[16] = {
  0x9B, 0xE8, 0x2A, 0xD3, 0xD3, 0x83, 0x4C, 0x5E,
  0x3F, 0x5E, 0xE2, 0x28, 0x3A, 0x13, 0xAF, 0xCE
};

static inline uint8_t sccm249Crc8Pure(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x2F) : (uint8_t)(crc << 1);
  }
  return crc;
}

static inline uint8_t leftStalkChecksumPure(const uint8_t frame[4], uint8_t counter) {
  const uint8_t crcInput[4] = {(uint8_t)(frame[1] & 0xF0), frame[2], frame[3], 0x00};
  return (uint8_t)(sccm249Crc8Pure(crcInput, 4) ^ SCCM249_CKSUM_CTR[counter & 0x0F]);
}


static inline uint8_t vcleftMuxPure(const uint8_t data[8]) { return data[0] & 0x03u; }
static inline uint8_t vcleftLeftButtonPure(const uint8_t data[8]) { return (data[3] >> 6) & 0x03u; }
static inline uint8_t vcleftRightButtonPure(const uint8_t data[8]) { return (data[5] >> 4) & 0x03u; }
static inline void vcleftSetLeftButtonPure(uint8_t data[8], uint8_t state) {
  data[3] = (uint8_t)((data[3] & 0x3Fu) | ((state & 0x03u) << 6));
}
static inline void vcleftSetRightButtonPure(uint8_t data[8], uint8_t state) {
  data[5] = (uint8_t)((data[5] & 0xCFu) | ((state & 0x03u) << 4));
}
static inline bool doorOpenButtonPressedPure(const uint8_t *data, uint8_t dlc) {
  return data && dlc >= 4 && ((data[3] & 0x80u) != 0);
}

// v3.2 hotfix: common Advanced EAP lane-change eligibility helpers.
// reqDir: 1=LEFT, 2=RIGHT. ALC state 4 (EXITING_HIGHWAY) is accepted only
// when fresh map context confirms an active route and a direction-matching off-ramp.
static inline bool autoBlinkerAlcAllowsDirectionPure(
    uint8_t reqDir, uint8_t alcState, bool roadContextFresh, bool navRouteActive,
    bool leftOffRamp, bool rightOffRamp) {
  if (reqDir == 1 && (alcState == 6 || alcState == 8)) return true;
  if (reqDir == 2 && (alcState == 7 || alcState == 8)) return true;
  if (alcState != 4 || !roadContextFresh || !navRouteActive) return false;
  if (reqDir == 1) return leftOffRamp;
  if (reqDir == 2) return rightOffRamp;
  return false;
}

// v3.4b3 request-session model. The planner request is latched independently
// from temporary ALC eligibility. Once the delay expires, a blocked lane does
// not cancel the session; the caller keeps the request pending and retries at a
// bounded cadence until the lane opens or the request session really ends.
struct AutoBlinkerSessionDecisionPure {
  bool cancel;
  bool fire;
  bool retry;
};

static inline AutoBlinkerSessionDecisionPure autoBlinkerSessionDecisionPure(
    bool noaGateOpen,
    uint8_t pendingDir,
    uint8_t currentReqDir,
    bool requestSeenRecently,
    bool delayElapsed,
    bool pendingAlcAllowed,
    bool retryDue) {
  AutoBlinkerSessionDecisionPure out = {};
  if (!noaGateOpen || pendingDir == 0 || !requestSeenRecently ||
      (currentReqDir != 0 && currentReqDir != pendingDir)) {
    out.cancel = true;
    return out;
  }
  if (!delayElapsed || !retryDue) return out;
  if (pendingAlcAllowed) out.fire = true;
  else out.retry = true;
  return out;
}

static inline bool autoBlinkerShouldStartSessionPure(
    uint8_t currentReqDir, uint8_t lastReqDir, bool pending, bool pulseActive) {
  return currentReqDir != 0 && !pending && !pulseActive && currentReqDir != lastReqDir;
}
