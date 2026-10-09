from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text()
vehicle = (root / 'vehicle_logic.h').read_text()
runtime = (root / 'can_runtime.h').read_text()
core = (root / 'can_core.h').read_text()
api = (root / 'web_api.h').read_text()
dash = (root / 'dashboard_source.html').read_text()
fixed = (root / 'r79_fixed_policy_pure.h').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
assert 'r79FixedQuietStepPure' in vehicle and 'r79FixedObserveStock' in vehicle
dispatch = runtime.index('r79ProcessStockFrame(f, timingMux, r79FrameNowMs)')
tick = runtime.index('r79TransportTick();')
assert dispatch < tick
for token in ('canTaskHeartbeatLastCause', 'canTaskHeartbeatLastAgeAms',
              'canTaskHeartbeatLastAgeBms', 'canTaskHeartbeatTimeoutCountA',
              'canTaskHeartbeatTimeoutCountB', 'canTaskHeartbeatTimeoutCountBoth'):
    assert token in core and token in api
for key in ('quietArm', 'quietFire', 'quietGuardSkip'):
    assert f'"{key}"' in api
for dom in ('r79FixedQuietValue', 'sysHeartbeatCause', 'sysHeartbeatAge',
            'sysHeartbeatCounts'):
    assert dom in dash
assert 'overflow-anchor:none' in dash.replace(' ', '')
assert 'min-height:100svh' in dash.replace(' ', '')
assert 'visualViewport' in dash and 'restoreViewportScroll' in dash
print('v3.7 fixed R79 / heartbeat preservation contract: PASS')
