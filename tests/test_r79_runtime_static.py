from pathlib import Path

root = Path(__file__).resolve().parents[1]
vl = (root / 'vehicle_logic.h').read_text()
cr = (root / 'can_runtime.h').read_text()
pure = (root / 'summon_state_pure.h').read_text()

assert 'r79TxDecisionPure(' in vl
assert 'gear118Raw' in vl and 'gear186Raw' in vl
assert 'static uint8_t r79LabGateReason(' not in vl
assert 'canTxTwaiTransmitWithMaskWait(&out' not in vl
assert 'canTxAdministrativeHold' in vl and 'twaiReady' in vl
assert 'r79LabPending' not in vl and 'r79LabPending' not in cr
assert 'R79PendingPure' not in pure and 'r79Pending' not in pure
assert 'R79_REASSERT_RETRY_MS' not in vl
assert 'r79LabFreshMaskWaitCount' not in vl

start = vl.index('static void r79FixedTick() {')
end = vl.index('static bool setTlsscEnabled', start)
periodic = vl[start:end]
assert 'if (!summoning) return;' not in periodic
assert 'r79FixedQuietStepPure' in periodic
assert 'r79QuietFireCount++' in periodic
assert 'r79FixedQuietHardDeadlinePure(r79FixedQuietState)' in periodic
assert '(void)r79LabTransmitShadow(' in periodic
assert 'stock, R79LAB_TX_PERIODIC, hardDeadlineMs' in periodic
assert 'r79TransportStrategy' not in periodic
assert 'r79LabSchedulerMode' not in periodic

shadow_start = vl.rindex('static bool r79LabTransmitShadow(')
shadow_end = vl.index('static void r79LabRetryTick()', shadow_start)
shadow = vl[shadow_start:shadow_end]
assert 'r79FixedDeadlineExpiredPure(now, hardDeadlineMs)' in shadow
assert 'const uint32_t now = (uint32_t)millis();' in shadow
assert 'r79FixedDeadlineWaitBudgetPure(' in shadow
assert 'r79LabTransmitOnce(' in shadow
assert '&r79Mode1PostMux2Generation, expectedGeneration' in shadow
assert 'r79LabDirectTwaiTransmitGuarded' in vl

retry_start = vl.rindex('static void r79LabRetryTick()')
retry_end = vl.index('static bool r79FixedFastEcho(', retry_start)
retry = vl[retry_start:retry_end]
assert 'r79FixedDeadlineExpiredPure(attemptNow, hardDeadlineMs)' in retry
assert 'r79LabTransmitOnce(' in retry and 'waitMs' in retry

assert 'r79ProcessStockFrame(f' in cr
fast_call = cr.index('r79ProcessStockFrame(f')
for later in ('bootCaptureObserveVhFrame', 'researchCaptureObserveVh',
              'driverMonitorCaptureObserve'):
    assert fast_call < cr.index(later, fast_call)
fixed_start = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
fixed_end = vl.index('static void r79FixedObserveStock(', fixed_start)
fast = vl[fixed_start:fixed_end]
assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast

print('R79 v3.7 runtime static checks passed')
