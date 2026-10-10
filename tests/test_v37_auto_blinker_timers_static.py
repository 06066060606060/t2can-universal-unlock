from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
logic = (ROOT / 'vehicle_logic.h').read_text()
runtime = (ROOT / 'can_runtime.h').read_text()
api = (ROOT / 'web_api.h').read_text()

for token in (
    'AutoBlinkerNoaSessionStatePure autoBlinkerNoaSessionState',
    'AutoBlinkerCancelPauseStatePure autoBlinkerCancelPauseState',
    'blinkANoaStabilizationSeconds', 'blinkACancelPauseSeconds',
    'autoBlinkerPlannerGateOpen', 'handleAutoBlinkerCancelToggle',
):
    assert token in logic, token

handle921 = logic[logic.index('static void handle921('):logic.index('static void handle1016(')]
assert 'autoBlinkerObserveNoaPure' in handle921

current = logic[logic.index('static uint8_t autoBlinkerCurrentRequestDir'):logic.index('static constexpr uint32_t ULC_REQUEST_FRESH_MS')]
assert 'autoBlinkerPlannerGateOpen(now)' in current
evaluate = logic[logic.index('static void evaluateAutoBlinker()'):logic.index('static void blinkATxTick()')]
assert 'autoBlinkerPlannerGateOpen(now)' in evaluate
prefire = logic[logic.index('static void blinkATxTick()'):logic.index('static void nagApGateSnapshot(')]
assert 'autoBlinkerPlannerGateOpen(now)' in prefire

direct = logic[logic.index('static bool requestTurnSignalPulseFromButton'):logic.index('static void injectTLSSC', logic.index('static bool requestTurnSignalPulseFromButton'))]
assert 'autoBlinkerPlannerGateOpen' not in direct

door = logic[logic.index('static void handle102LaneChangeCancel'):logic.index('static void evaluateAutoBlinker')]
s3xy = logic[logic.index('static void handleS3xySingleAction'):logic.index('// 0x3F8 UI_driverAssistControl')]
assert 'handleAutoBlinkerCancelToggle' in door
assert 'handleAutoBlinkerCancelToggle' in s3xy

assert 'autoBlinkerNoaSessionResetPure(autoBlinkerNoaSessionState)' in runtime
assert 'autoBlinkerCancelPauseResetPure(autoBlinkerCancelPauseState)' in runtime
for key in ('noaStabS', 'cancelPauseS'):
    assert f'"{key}"' in logic
for key in ('noaStabilizationSeconds', 'cancelPauseSeconds', 'noaStabilized',
            'noaStabilizationRemainingMs', 'noaSessionState',
            'noaSessionStateName', 'noaExitRemainingMs', 'cancelPaused',
            'cancelPauseRemainingMs'):
    assert f'"{key}"' in api, key
assert '/api/blinkA/timing' in api
assert 'BLINKA_NOA_STABILIZE_MIN_S_PURE' in api
assert 'BLINKA_NOA_STABILIZE_MAX_S_PURE' in api
assert 'BLINKA_CANCEL_PAUSE_MIN_S_PURE' in api
assert 'BLINKA_CANCEL_PAUSE_MAX_S_PURE' in api

timing = api[api.index('static void httpBlinkATiming()'):api.index('static void httpBlinkATxMode()')]
assert 'autoBlinkerNoaReconfigurePure' in timing
assert 'nextNoa != previousNoa' in timing

print('v3.7 Auto Blinker timers static contract: PASS')
