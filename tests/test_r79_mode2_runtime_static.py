from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text()
logic = (root / 'vehicle_logic.h').read_text()
runtime = (root / 'can_runtime.h').read_text()
api = (root / 'web_api.h').read_text()
html = (root / 'dashboard_source.html').read_text()

assert '#include "r79_mode2_pure.h"' in ino
for token in (
    'r79TransportMode', 'r79Mode1TxWaitMode', 'r79Mode1ReinjectEnabled',
    'r79Mode1DelayMs',
    'r79Mode2ReinjectEnabled', 'r79Mode2DelayMs',
    'R79Mode2DelayedStatePure r79Mode2DelayedState',
):
    assert token in logic

# One dispatcher selects exactly one transport for each received stock frame.
assert 'static bool r79ProcessStockFrame(' in logic
dispatch_start = logic.rindex('static bool r79ProcessStockFrame(')
dispatch_end = logic.index('\n}', dispatch_start) + 2
dispatch = logic[dispatch_start:dispatch_end]
assert 'R79_MODE_2_PURE' in dispatch
assert 'r79Mode2FastEcho' in dispatch
assert 'r79FixedFastEcho' in dispatch
assert 'r79Mode2ObserveStock' in dispatch
assert 'r79FixedObserveStock' in dispatch
assert 'r79ProcessStockFrame(f' in runtime
assert 'r79FixedFastEcho(f)' not in runtime

# Mode 2 is a single non-blocking attempt. Queue flush and retry scheduling
# remain Mode-1-only and cannot occur from either Mode 2 sender.
m2_fast_start = logic.rindex('static bool r79Mode2FastEcho(')
m2_fast_end = logic.index('static void r79Mode2ObserveStock(', m2_fast_start)
m2_fast = logic[m2_fast_start:m2_fast_end]
assert 'r79Mode2ApplyBitsPure' in m2_fast
assert 'r79ApGateTransmitGuarded(&out, 0u, apGeneration)' in m2_fast
assert 'r79FastReactiveGateOpen()' in m2_fast
assert 'twai_clear_transmit_queue' not in m2_fast
assert 'r79RetrySchedule' not in m2_fast
guard_start = logic.index('static esp_err_t r79ApGateTransmitGuarded(')
guard_end = logic.index('static esp_err_t r79LabDirectTwaiTransmit(', guard_start)
assert 'r79DmsApplyFinal' in logic[guard_start:guard_end]
dms_start = logic.index('static bool r79DmsControlActive()')
dms_end = logic.index('static inline bool r79DmsApplyFinal', dms_start)
assert 'driverMonitoringControlSnapshot()' in logic[dms_start:dms_end]
assert 'nagCfg.' not in logic[dms_start:dms_end]
assert 'R79_MODE_1_PURE' not in logic[dms_start:dms_end]

m2_tick_start = logic.rindex('static void r79Mode2Tick()')
m2_tick_end = logic.index('static void r79TransportTick()', m2_tick_start)
m2_tick = logic[m2_tick_start:m2_tick_end]
assert 'r79Mode2DelayedStepPure' in m2_tick
assert 'r79RuntimeStatusSnapshot' in m2_tick  # common manual D/R suppression
assert 'r79ApGateTransmitGuarded(&out, 0u, apGeneration)' in m2_tick
assert 'r79RetrySchedule' not in m2_tick
assert 'twai_clear_transmit_queue' not in m2_tick
assert 'r79TransportTick();' in runtime
assert 'r79FixedTick();' not in runtime

# Persistent settings and status/API surface.
for token in ('"mode"', '"m1wait"', '"m1re"', '"m1delay"', '"m2re"', '"m2delay"'):
    assert token in logic
for key in ('"mode"', '"modeName"', '"mode1TxWaitMode"',
            '"mode1ReinjectEnabled"', '"mode1DelayMs"', '"mode2ReinjectEnabled"',
            '"mode2DelayMs"'):
    assert key in api
for arg in ('server.hasArg("mode")', 'server.hasArg("mode1TxWaitMode")',
            'server.hasArg("mode1Reinject")', 'server.hasArg("mode1DelayMs")',
            'server.hasArg("mode2Reinject")',
            'server.hasArg("mode2DelayMs")'):
    assert arg in api

for dom in ('r79Mode', 'r79Mode1TxWait', 'r79Mode1Reinject', 'r79Mode1DelayMs',
            'r79Mode1Controls', 'r79Mode2Reinject', 'r79Mode2DelayMs',
            'r79Mode2Controls'):
    assert f'id="{dom}"' in html
assert 'updateR79Settings' in html

print('R79 Mode 2 runtime/dashboard contract OK')
