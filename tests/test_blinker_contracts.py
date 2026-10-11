"""Blinker contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class BlinkerContracts(unittest.TestCase):

    def test_blinker_tx_policy(self):
        """Retained from test_blinker_tx_policy_static.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        logic = (root / "vehicle_logic.h").read_text()
        runtime = (root / "can_runtime.h").read_text()


        def function_body(source: str, signature: str) -> str:
            start = source.index(signature)
            brace = source.index("{", start)
            depth = 0
            for pos in range(brace, len(source)):
                if source[pos] == "{":
                    depth += 1
                elif source[pos] == "}":
                    depth -= 1
                    if depth == 0:
                        return source[brace : pos + 1]
            raise AssertionError(f"unclosed function: {signature}")


        request = function_body(logic, "static bool requestBlinkerTx(")
        auto_tick = function_body(logic, "static void blinkATxTick()")
        s3xy = function_body(
            logic, "static bool requestTurnSignalPulseFromButton(uint8_t dir)"
        )
        stalkless = function_body(logic, "static void handle3C2OnCanA(")

        assert "blinkerTxArmPure(" in request
        assert "blinkerLegacyStartLocked(" in request
        assert "requestBlinkerTx(" in auto_tick
        assert "requestBlinkerTx(" in s3xy
        assert "oneShotUntil = now + BLINKA_PULSE_MS" not in request
        assert "blinkAEnabled" not in s3xy
        assert "blinkerTxRequestState = {};" in runtime
        assert "blinkerTxConsumeStockPure(" in stalkless
        assert "singleRequest.transmit" in stalkless
        assert "switchState = VCLEFT_SWITCH_ON" in stalkless
        assert "stalkFrameRecordResult(" in stalkless

        print("shared single-TX blinker policy contract: PASS")

    def test_blinker_tx_production(self):
        """Retained from test_blinker_tx_production_contract.py."""
        from pathlib import Path


        root = Path(__file__).resolve().parents[1]
        api = (root / "web_api.h").read_text()
        logic = (root / "vehicle_logic.h").read_text()
        forward = (root / "t2can_forward.h").read_text()
        ino = next(root.glob("*.ino")).read_text()
        dashboard = (root / "dashboard_source.html").read_text()


        def function_body(source: str, signature: str) -> str:
            start = source.index(signature)
            brace = source.index("{", start)
            depth = 0
            for pos in range(brace, len(source)):
                if source[pos] == "{":
                    depth += 1
                elif source[pos] == "}":
                    depth -= 1
                    if depth == 0:
                        return source[brace : pos + 1]
            raise AssertionError(f"unclosed function: {signature}")


        assert 'server.on("/api/blinkA/tx-mode", HTTP_POST, httpBlinkATxMode)' in api
        assert '/api/lab/blinker-tx/' not in api
        assert 'httpBlinkerTxLab' not in api + forward

        load = function_body(logic, "static void featureCfgLoad()")
        persist = function_body(logic, "static bool blinkerTxModePersist(uint8_t requested)")
        update = function_body(api, "static void httpBlinkATxMode()")
        feature_lab = function_body(api, "static void httpFeatureLab()")
        stats = function_body(api, "static String blinkAStatsToJson()")

        assert 'getUChar("blinkTx", 0xFFu)' in load
        assert "blinkerTxStoredModePure" in load
        assert 'begin("features", false)' in persist
        assert 'putUChar("blinkTx", requested)' in persist
        assert 'blinkerTxModePersist(requested)' in update
        assert 'blinkerTxSetMode(requested)' in update
        assert update.index('blinkerTxModePersist(requested)') < update.index('blinkerTxSetMode(requested)')
        assert "NVS write failed" in update
        assert "blinkerTxCancelLegacyLocked();" not in feature_lab
        assert "blinkerTxDefaultModePure(activeProfileIsYl())" not in ino

        for key in (
            "txMode",
            "txModeName",
            "txActiveSource",
            "txLastDirection",
            "txLastSource",
            "txLastResult",
            "txRequests",
            "txOk",
            "txFail",
            "txBlocked",
        ):
            assert f'"{key}"' in stats

        assert "AUTO_BLINKER" in api and "S3XY_BUTTON" in api

        blink_panel = dashboard[
            dashboard.index('id="panelBlink"') : dashboard.index('id="panelSummon"')
        ]
        assert 'id="blinkTxMode"' in blink_panel
        assert 'id="blinkTxSource"' in blink_panel
        assert 'id="blinkTxLast"' in blink_panel
        assert 'id="blinkTxRequests"' in blink_panel
        assert 'id="blinkTxCounts"' in blink_panel
        assert "S3XY blinker buttons" in blink_panel
        assert "/api/blinkA/tx-mode?mode=" in dashboard
        assert "txMode" in dashboard and "txActiveSource" in dashboard
        for retired in (
            "panelLabBlinkerTx",
            "labBlinkerTxMode",
            "fetchBlinkerTxLab",
            "updateBlinkerTxLab",
        ):
            assert retired not in dashboard
        print("production Auto Blinker TX policy contract: PASS")

    def test_auto_blinker_timers(self):
        """Retained from test_v37_auto_blinker_timers_static.py."""
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

    def test_cancel_behavior_independent(self):
        """Retained from test_v37_cancel_behavior_independent_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        LOGIC = (ROOT / "vehicle_logic.h").read_text(encoding="utf-8")
        S3XY = (ROOT / "s3xy_ble.h").read_text(encoding="utf-8")

        start = LOGIC.index("static AutoBlinkerCancelActionPure handleAutoBlinkerCancelToggle")
        end = LOGIC.index("static void handleS3xySingleAction", start)
        helper = LOGIC[start:end]

        # Manual cancel eligibility must be behavior-independent.
        for forbidden in (
            "visualBehaviorType",
            "behavior == 2",
            "behavior == 3",
            "dir != 0",
            "BLOCKED: no lane-change request",
            "ARMED: LEFT",
            "ARMED: RIGHT",
        ):
            assert forbidden not in helper, forbidden

        for required in (
            "autoBlinkerNOAGateOpen(now)",
            "visualDebugLastMs",
            "visualAge <= ULC_REQUEST_FRESH_MS",
            "ulcSnoozePending = true",
            "lastReqDir = 0",
            "autoRequestLastSeenMs = 0",
        ):
            assert required in helper, required

        door_start = LOGIC.index("static void handle102LaneChangeCancel")
        door_end = LOGIC.index("static void evaluateAutoBlinker", door_start)
        door = LOGIC[door_start:door_end]
        s3_start = LOGIC.index("static void handleS3xySingleAction")
        s3_end = LOGIC.index("// 0x3F8 UI_driverAssistControl", s3_start)
        s3 = LOGIC[s3_start:s3_end]
        assert "handleAutoBlinkerCancelToggle" in door
        assert "handleAutoBlinkerCancelToggle" in s3

        # Auto Blinker itself still keeps planner behaviorType decoding.
        auto_start = LOGIC.index("static uint8_t autoBlinkerCurrentRequestDir")
        auto_end = LOGIC.index("static constexpr uint32_t ULC_REQUEST_FRESH_MS", auto_start)
        auto = LOGIC[auto_start:auto_end]
        assert "visualBehaviorType" in auto
        assert "behavior == 2" in auto and "behavior == 3" in auto

        assert "DAS_behaviorType is intentionally not used by the manual cancel gate" in S3XY

        print("v3.7 manual cancel behavior-independent contract: PASS")

    def test_blinker_behavior_and_feature_controls(self):
        """Retained from test_v36b3_features_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        ble = (root / 's3xy_ble.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')

        # Version / package contract.

        # TLSSC disable/gate transitions must have an explicit one-shot clear path.
        for symbol in ('tlsscInjectedActive', 'tlsscClearPending', 'setTlsscEnabled', 'tlsscBlockInNoa'):
            assert symbol in logic or symbol in api, f'missing TLSSC b3 symbol: {symbol}'
        assert 'setBit(out.data, 38, false)' in logic
        assert 'setBit(out.data, 39, false)' in logic
        assert 'tlsscBlockInNoa' in api and 'tlsscBlockInNoa' in html

        # S3XY must expose TLSSC and Auto Blinker toggle actions.
        for token in ('S3XY_ACTION_TLSSC_TOGGLE', 'S3XY_ACTION_AUTO_BLINKER_TOGGLE'):
            assert token in ble, f'missing S3XY action {token}'
        for code in ('tlssc_toggle', 'auto_blinker_toggle'):
            assert code in ble and code in html, f'missing S3XY action code {code}'

        # AP Pedal / Regen Profile still forces CHILL on 0x334. Regen target/topology support is
        # covered by the newer v3.6b7 contract.
        assert 'writeBitsLE(outData,5,2,0)' in logic.replace(' ', ''), 'AP profile must force CHILL raw 0'
        assert 'AP Accel / Regen' in html
        assert 'apDriveProfileToggle' in html

        # HOME Injection State layout: no Summon Ready tile; R79 + CAN A/B remain.
        assert 'Summon Ready' not in html
        assert 'R79 TX' in html and 'homeCanAName' in html and 'homeCanBName' in html

        # Main-tab motion and its no-overflow behavior are exercised in the real
        # browser regression. Detail-panel motion remains part of this static contract.
        assert 'v38PanelForward' in html and 'v38PanelExitBack' in html
        assert 'animationend' in html, 'panel polling should be deferred until animation end'

        # Once firmware has asserted TLSSC, a later stock 1/1 frame must not silently
        # discard ownership before the requested one-shot OFF transition can be sent.
        stock_on = logic.index('if (!getBit(out.data, 38) || !getBit(out.data, 39))')
        stock_on_end = logic.index('    setBit(out.data, 38, true)', stock_on)
        assert 'tlsscInjectedActive = false' not in logic[stock_on:stock_on_end], 'TLSSC ownership must survive stock 1/1 after our ON injection'

        # Removed HOME Summon card must not leave live JS dereferences behind.
        for removed_id in ('gatePill','gateMode','sumHomeSession','gPark','gSpr','gAca','gQueue'):
            assert f"$('{removed_id}')" not in html, f'removed HOME Summon id still dereferenced: {removed_id}'

        print('v3.6b3 feature contract OK')


if __name__ == "__main__":
    unittest.main()
