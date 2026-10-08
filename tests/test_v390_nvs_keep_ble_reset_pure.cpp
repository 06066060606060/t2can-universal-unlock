#include <assert.h>
#include <stdint.h>

#include "../nvs_keep_ble_reset_pure.h"

int main() {
  assert(nvsKeepBleGuardBootActionPure(false, false) ==
         NVS_KEEP_BLE_GUARD_SAFE_ERROR_PURE);
  assert(nvsKeepBleGuardBootActionPure(true, false) ==
         NVS_KEEP_BLE_GUARD_NORMAL_BOOT_PURE);

  // The new LAB namespace must be cleared before reset can finish. With
  // only the old 15 namespaces cleared, bootstrap/guard completion is unsafe.
  auto newLabPending = nvsKeepBleResetStartPure(true);
  for (uint8_t i = 0u; i < 15u; ++i)
    nvsKeepBleResetClearOnePure(newLabPending, true);
  nvsKeepBleResetWriteBootstrapPure(newLabPending, true);
  nvsKeepBleResetClearGuardPure(newLabPending, true);
  assert(newLabPending.guardActive && !newLabPending.bootstrapWritten);
  nvsKeepBleResetClearOnePure(newLabPending, true);
  nvsKeepBleResetWriteBootstrapPure(newLabPending, true);
  nvsKeepBleResetClearGuardPure(newLabPending, true);
  assert(nvsKeepBleResetCompletePure(newLabPending));
  assert(!nvsKeepBleResetStartPure(false).guardActive);

  // Inject a reboot after every namespace boundary. The persistent guard
  // selects recovery, then the idempotent transaction can finish.
  for (uint8_t boundary = 0u;
       boundary <= NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE;
       ++boundary) {
    NvsKeepBleResetStatePure state = nvsKeepBleResetStartPure(true);
    for (uint8_t i = 0u; i < boundary; ++i)
      nvsKeepBleResetClearOnePure(state, true);
    assert(state.guardActive);
    assert(nvsKeepBleGuardBootActionPure(true, state.guardActive) ==
           NVS_KEEP_BLE_GUARD_RECOVER_PURE);
    nvsKeepBleResetBeginRecoveryPure(state);
    while (state.namespacesCleared <
           NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE)
      nvsKeepBleResetClearOnePure(state, true);
    nvsKeepBleResetWriteBootstrapPure(state, true);
    nvsKeepBleResetClearGuardPure(state, true);
    assert(nvsKeepBleResetCompletePure(state));
  }

  // A failed namespace clear, bootstrap write, or final guard clear preserves
  // the guard and therefore can never select full-partition migration erase.
  NvsKeepBleResetStatePure failedClear = nvsKeepBleResetStartPure(true);
  nvsKeepBleResetClearOnePure(failedClear, false);
  assert(failedClear.namespacesCleared == 0u && failedClear.guardActive);
  assert(nvsKeepBleGuardBootActionPure(true, failedClear.guardActive) ==
         NVS_KEEP_BLE_GUARD_RECOVER_PURE);
  nvsKeepBleResetBeginRecoveryPure(failedClear);

  NvsKeepBleResetStatePure failedBootstrap = nvsKeepBleResetStartPure(true);
  for (uint8_t i = 0u;
       i < NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE; ++i)
    nvsKeepBleResetClearOnePure(failedBootstrap, true);
  nvsKeepBleResetWriteBootstrapPure(failedBootstrap, false);
  nvsKeepBleResetClearGuardPure(failedBootstrap, true);
  assert(failedBootstrap.guardActive && !failedBootstrap.bootstrapWritten);

  nvsKeepBleResetWriteBootstrapPure(failedBootstrap, true);
  nvsKeepBleResetClearGuardPure(failedBootstrap, false);
  assert(failedBootstrap.guardActive && failedBootstrap.bootstrapWritten);
  assert(nvsKeepBleGuardBootActionPure(true, failedBootstrap.guardActive) ==
         NVS_KEEP_BLE_GUARD_RECOVER_PURE);

  nvsKeepBleResetBeginRecoveryPure(failedBootstrap);
  for (uint8_t i = 0u;
       i < NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE; ++i)
    nvsKeepBleResetClearOnePure(failedBootstrap, true);
  nvsKeepBleResetWriteBootstrapPure(failedBootstrap, true);
  nvsKeepBleResetClearGuardPure(failedBootstrap, true);
  assert(nvsKeepBleResetCompletePure(failedBootstrap));
  return 0;
}
