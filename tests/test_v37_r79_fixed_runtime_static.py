from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob("*.ino")).read_text()
vl = (ROOT / "vehicle_logic.h").read_text()
cr = (ROOT / "can_runtime.h").read_text()
web = (ROOT / "web_api.h").read_text()

assert '#include "r79_fixed_policy_pure.h"' in ino
assert 'static volatile uint8_t r79Bit18Policy = R79_BIT18_DEFAULT_PURE;' in vl
assert 'static volatile bool r79Mode1ReinjectEnabled = true;' in vl
assert 'r79Mode1PostMux2Generation' in vl

# The live CAN-B path has one mutually-exclusive Mode 1/Mode 2 dispatcher and
# one transport tick. Mode 1 retains its payload and recovery policy while
# exposing the historical initial enqueue-wait comparison.
assert cr.count('r79ProcessStockFrame(f, timingMux, r79FrameNowMs)') == 1
assert cr.count('r79TransportTick();') == 1
assert 'r79TransportHandleMux1(f, frameRxDequeueUs)' not in cr

fast_start = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
fast_end = vl.index('static void r79FixedObserveStock(', fast_start)
fast = vl[fast_start:fast_end]
assert 'r79FixedApplyBitsPure(out.data, bit18Policy, r79Hw3Active())' in fast
assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast
for retired in ('r79TransportStrategy', 'r79PostMux1TxMode',
                'r79PeriodicSchedulerMode', 'r79PeriodicAnchorMux',
                'r79PeriodicShotsPerCycle'):
    assert retired not in fast

tick_start = vl.index('static void r79FixedTick() {')
tick_end = vl.index('static bool setTlsscEnabled', tick_start)
tick = vl[tick_start:tick_end]
tick_flat = ' '.join(tick.split())
assert 'r79FixedQuietStepPure' in tick
assert 'r79LabRetryTick();' in tick
assert 'canTxCancellationGenerationSnapshot( &r79Mode1PostMux2Generation)' in tick_flat
assert 'r79LabTransmitShadow(' in tick
for retired in ('r79TransportStrategy', 'r79PostMux1TxMode',
                'r79PeriodicSchedulerMode', 'r79PeriodicAnchorMux',
                'r79PeriodicShotsPerCycle'):
    assert retired not in tick

# Production API exposes the transport selector plus the existing Mode-1
# STOCK/FORCE 0 policy and is not LAB-enabled gated.
assert 'static void httpR79Stats()' in web
assert 'static void httpR79Update()' in web
assert 'server.on("/api/r79/stats", HTTP_GET, httpR79Stats);' in web
assert 'server.on("/api/r79/update", HTTP_POST, httpR79Update);' in web
update_start = web.index('static void httpR79Update()')
update_end = web.index('\nstatic void ', update_start + 1)
update = web[update_start:update_end]
assert 'bit18Mode' in update
assert 'server.hasArg("mode")' in update
assert 'server.hasArg("mode1TxWaitMode")' in update
assert 'server.hasArg("mode1Reinject")' in update
assert 'server.hasArg("mode1DelayMs")' in update
assert 'server.hasArg("mode2Reinject")' in update
assert 'server.hasArg("mode2DelayMs")' in update
assert '400' in update
assert 'labMenuEnabled' not in update
assert 'canTxCancellationGenerationAdvance(&r79Mode1PostMux2Generation)' in update
assert 'r79Mode1DisableCancelsRetryPure' in update
assert 'if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(true);' in update
assert 'if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(false);' in update

print('v3.7 fixed R79 runtime static contract: PASS')
