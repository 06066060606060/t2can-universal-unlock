from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
owned = ['vehicle_logic.h', 'can_core.h', 'can_runtime.h', 't2can_core_state.h', 't2can_forward.h']
text = '\n'.join((ROOT / name).read_text() for name in owned)
for token in ('laneGraph', 'LaneGraph', 'parkedInjection', 'ParkedInjection', 'can3fdTiming', 'lab3f8LastTx', 'canARxSavedMode', 'canARxModePersist', 'CAN_A_RX_PREFETCH_4'):
    assert token not in text, f'retired runtime control remains: {token}'
for name in ('lane_graph_pure.h', 'parked_injection_pure.h', 'can3fd_timing_pure.h'):
    assert not (ROOT / name).exists(), f'retired resource remains: {name}'
logic = (ROOT / 'vehicle_logic.h').read_text()
assert '"canARxMode"' not in logic, 'legacy saved settings must not control receive scheduling'
runtime = (ROOT / 'can_runtime.h').read_text()
assert 'MCP_PREFETCH_CAPACITY = 1' in runtime
assert 'canARxReadBatchBudgetPure()' in runtime
# Normal BUS-OFF diagnostics and policy counters are independent of retired panels.
for token in ('canTwaiErrorAlertWindow', 'canBTraceFreezeBusOff', 'canBusOffPersistenceMarkDirty'):
    assert token in text, token
for token in ('lab3f8UlcBlindMode', 'uiUlcBlindSpotConfig', 'lab3f8TxOk', 'lab3f8TxFail'):
    assert token in logic, token
