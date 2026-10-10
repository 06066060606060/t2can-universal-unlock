from pathlib import Path

root = Path(__file__).resolve().parents[1]
files = {name: (root / name).read_text(encoding='utf-8') for name in [
    'vehicle_logic.h','can_runtime.h','can_core.h','t2can_forward.h',
    't2can_core_state.h','summon_state_pure.h','web_api.h','dashboard_source.html'
]}
all_text = '\n'.join(files.values())

retired = [
    'SUMMON_GATE_DROPOUT_GRACE_MS','summonGateGraceUntilMs','summonGateGraceEnterCount',
    'summonGateGraceRecoverCount','summonGateGraceExpireCount','summonGateGraceActiveLocked',
    'summonAuthorization','summonAuthorizationSinceMs','summonAuthorizationTransitions',
    'forceMode','summonInjectionGateOpen','gateBlockReason',
    'SUMMON_PRIORITY_FULL',
    'summonPriorityStateSinceMs','summonPriorityTransitions','summonPriorityFullEnterCount',
    'summonPriorityFullExitCount','summonPriorityFullInactiveSinceMs',
    'SUMMON_PRIORITY_FULL_EXIT_GRACE_MS','twaiStandbyShed','twaiFullShed',
    'twaiSummonQueueFlush','twaiSummonRetryOk','twaiSummonRetryFail',
    'canTxTwaiTransmitWithMaskWait','canTxTwaiClearQueueWithMask','canTxTwaiClearQueue',
    'priorityGear280State','priorityGear390State','priorityGear280Raw','priorityGear390Raw',
    'priorityGear280Ms','priorityGear390Ms','injectSummon(',
    'summonAuthorizationPure','summonRequiredTxFreshMaskPure',
    'summonRemoteFallbackAllowedPure','summonParkEntryAllowedPure',
    'summonV26CompatInitialPure','manualDasStatePure','canTxBarrierInvalidatePure',
]
for symbol in retired:
    assert symbol not in all_text, f'retired v3.6b1 symbol remains: {symbol}'

# New names/structure must be present.
logic = files['vehicle_logic.h']
for symbol in ('gear118State','gear186State','gear118Raw','gear186Raw',
               'r79ManualSuppression','injectUlcSnooze3fdMux1'):
    assert symbol in logic, f'missing v3.6b2 cleanup symbol: {symbol}'
assert 'static bool summonLoadSheddingActive()' in logic
assert 'gateSummoning' in logic

# Snapshot is read-only: it must not refresh or mutate derived state.
start = logic.index('static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now) {')
end = logic.index('static void refreshSummonState', start)
snapshot = logic[start:end]
assert 'refreshSummonDerivedStateLocked' not in snapshot
assert 'r79ManualSuppressionUpdate' not in snapshot

# Old dashboard concepts should be gone.
html = files['dashboard_source.html']
for phrase in ('SUMMON_FULL','Gate grace','Authorization'):
    assert phrase not in html, f'legacy dashboard concept remains: {phrase}'

print('v3.6b2 cleanup contract OK (d1 transport priority exceptions acknowledged)')
