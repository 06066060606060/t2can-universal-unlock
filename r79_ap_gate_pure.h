#pragma once

#include <stdint.h>

enum R79ApGateModePure : uint8_t {
  R79_AP_GATE_BLOCK_PURE = 0,
  R79_AP_GATE_DELAY_PURE = 1
};

enum R79ApGateReasonPure : uint8_t {
  R79_AP_GATE_BYPASS_PURE = 0,
  R79_AP_GATE_NON_AP_PURE = 1,
  R79_AP_GATE_BLOCKED_PURE = 2,
  R79_AP_GATE_WAITING_PURE = 3,
  R79_AP_GATE_READY_PURE = 4,
  R79_AP_GATE_UNKNOWN_PURE = 5
};

struct R79ApGateConfigPure {
  bool enabled;
  uint8_t mode;
  uint8_t delaySeconds;
  bool allowManualDriving = false;
};

struct R79ApGateSessionPure {
  bool active;
  uint32_t startedMs;
  bool released;
};

struct R79ApGateDecisionPure {
  bool allowed;
  uint8_t reason;
  uint32_t remainingMs;
};

static inline bool r79ApGateConfigValidPure(const R79ApGateConfigPure &config) {
  return config.mode <= R79_AP_GATE_DELAY_PURE &&
         config.delaySeconds >= 2u && config.delaySeconds <= 10u;
}

// Caller supplies validated, fresh DAS state (AP states 3..6 all map to true).
// Reset the session on configuration/recovery and observe invalid before accepting
// a new frame after a freshness gap. No timestamp value is used as a sentinel.
static inline void r79ApGateObservePure(
    R79ApGateSessionPure &session, const R79ApGateConfigPure &config,
    uint32_t nowMs, bool dasFreshValid, bool apActive) {
  if (!config.enabled || !r79ApGateConfigValidPure(config) ||
      !dasFreshValid || !apActive) {
    session = {};
    return;
  }
  if (!session.active) {
    session.active = true;
    session.startedMs = nowMs;
    session.released = false;
  }
  if (config.mode == R79_AP_GATE_DELAY_PURE &&
      (uint32_t)(nowMs - session.startedMs) >=
          (uint32_t)config.delaySeconds * 1000u) {
    session.released = true;
  }
}

// A supplementary permission only: the caller must AND it with legacy R79
// authorization. Snapshot evaluation never mutates the CAN-owned session.
static inline R79ApGateDecisionPure r79ApGateDecisionPure(
    const R79ApGateConfigPure &config, const R79ApGateSessionPure &session,
    uint32_t nowMs, bool dasFreshValid, bool apActive) {
  if (!config.enabled) return {true, R79_AP_GATE_BYPASS_PURE, 0u};
  if (!r79ApGateConfigValidPure(config) || !dasFreshValid)
    return {false, R79_AP_GATE_UNKNOWN_PURE, 0u};
  if (!apActive) return {true, R79_AP_GATE_NON_AP_PURE, 0u};
  if (config.mode == R79_AP_GATE_BLOCK_PURE)
    return {false, R79_AP_GATE_BLOCKED_PURE, 0u};
  if (!session.active) return {false, R79_AP_GATE_UNKNOWN_PURE, 0u};
  const uint32_t delayMs = (uint32_t)config.delaySeconds * 1000u;
  const uint32_t elapsedMs = (uint32_t)(nowMs - session.startedMs);
  if (session.released || elapsedMs >= delayMs)
    return {true, R79_AP_GATE_READY_PURE, 0u};
  return {false, R79_AP_GATE_WAITING_PURE, delayMs - elapsedMs};
}
