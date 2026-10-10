#pragma once

#include <stdint.h>

enum NvsKeepBleGuardBootActionPure : uint8_t {
  NVS_KEEP_BLE_GUARD_SAFE_ERROR_PURE = 0,
  NVS_KEEP_BLE_GUARD_NORMAL_BOOT_PURE = 1,
  NVS_KEEP_BLE_GUARD_RECOVER_PURE = 2,
};

static constexpr uint8_t NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE = 16u;

struct NvsKeepBleResetStatePure {
  bool guardActive;
  uint8_t namespacesCleared;
  bool bootstrapWritten;
};

static inline NvsKeepBleGuardBootActionPure
nvsKeepBleGuardBootActionPure(bool guardReadOk, bool guardActive) {
  if (!guardReadOk) return NVS_KEEP_BLE_GUARD_SAFE_ERROR_PURE;
  return guardActive ? NVS_KEEP_BLE_GUARD_RECOVER_PURE
                     : NVS_KEEP_BLE_GUARD_NORMAL_BOOT_PURE;
}

static inline NvsKeepBleResetStatePure nvsKeepBleResetStartPure(
    bool guardWriteSucceeded) {
  return {guardWriteSucceeded, 0u, false};
}

static inline void nvsKeepBleResetBeginRecoveryPure(
    NvsKeepBleResetStatePure &state) {
  if (!state.guardActive) return;
  // Boot recovery reruns the idempotent allowlist from the beginning and
  // rewrites the profile bootstrap before attempting to clear the guard.
  state.namespacesCleared = 0u;
  state.bootstrapWritten = false;
}

static inline void nvsKeepBleResetClearOnePure(
    NvsKeepBleResetStatePure &state, bool clearSucceeded) {
  if (!state.guardActive || !clearSucceeded ||
      state.namespacesCleared >=
          NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE)
    return;
  ++state.namespacesCleared;
}

static inline void nvsKeepBleResetWriteBootstrapPure(
    NvsKeepBleResetStatePure &state, bool writeSucceeded) {
  if (state.guardActive && writeSucceeded &&
      state.namespacesCleared ==
          NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE)
    state.bootstrapWritten = true;
}

static inline void nvsKeepBleResetClearGuardPure(
    NvsKeepBleResetStatePure &state, bool clearSucceeded) {
  if (state.guardActive && state.bootstrapWritten && clearSucceeded)
    state.guardActive = false;
}

static inline bool nvsKeepBleResetCompletePure(
    const NvsKeepBleResetStatePure &state) {
  return !state.guardActive && state.bootstrapWritten &&
      state.namespacesCleared ==
          NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE;
}
