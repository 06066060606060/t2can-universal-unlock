"""Ble contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class BleContracts(unittest.TestCase):

    def test_s3xy_stock_synced_stalk(self):
        """Retained from test_s3xy_stock_synced_stalk_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        vehicle = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')

        request_fn = vehicle.split(
            'static bool requestTurnSignalPulseFromButton(uint8_t dir) {', 1
        )[1].split('\n}', 1)[0]
        assert 'static BlinkerTxConsumeResultPure observe249AndTakeBlinkerRequest(' in vehicle, (
            'stock-synchronized 0x249 observer is missing'
        )
        observe_fn = vehicle.split(
            'static BlinkerTxConsumeResultPure observe249AndTakeBlinkerRequest(', 1
        )[1].split('\n}', 1)[0]
        handle_b = vehicle.split(
            'static void handle249OnCanB(const uint8_t *data, uint8_t dlc) {', 1
        )[1].split('\n}', 1)[0]
        handle_a = vehicle.split(
            'static void handle249OnCanA(const uint8_t *data, uint8_t dlc) {', 1
        )[1].split('\n}', 1)[0]

        assert '#define BLINKA_S3XY_STOCK_TIMEOUT_MS 250' in vehicle
        assert 'BlinkerTxRequestStatePure blinkerTxRequestState = {};' in vehicle
        assert 'blinkerTxConsumeStockPure(' in observe_fn
        assert 'turn == STALK_IDLE' in observe_fn

        assert 'observe249AndTakeBlinkerRequest(data, dlc)' in handle_b
        assert 'sendStalkFrameCanB(dirToTurn(request.dir)' in handle_b
        assert 'observe249AndTakeBlinkerRequest(data, dlc)' in handle_a
        assert 'sendStalkFrameCanA(dirToTurn(request.dir)' in handle_a

        assert 'activeTurnSignalVariant == TURN_SIGNAL_UNSET' in request_fn
        assert 'requestBlinkerTx(' in request_fn
        assert 'oneShotUntil = now + BLINKA_PULSE_MS;' not in request_fn

        assert 'blinkerTxRequestState = {};' in runtime

        print('PASS S3XY stalk action is stock-synchronized and single-shot')

    def test_s3xy_direct_blinker(self):
        """Retained from test_v36e2_s3xy_direct_blinker_static.py."""
        from pathlib import Path
        R=Path(__file__).resolve().parents[1]
        s=(R/'s3xy_ble.h').read_text()
        v=(R/'vehicle_logic.h').read_text()
        f=(R/'t2can_forward.h').read_text()
        d=(R/'dashboard_source.html').read_text()
        assert 'oneShotDirect' in v
        assert 'static bool requestTurnSignalPulseFromButton(uint8_t dir)' in v
        fn=v.split('static bool requestTurnSignalPulseFromButton(uint8_t dir) {',1)[1].split('\n}',1)[0]
        assert 'activeProfileAdvancedEapSupported()' not in fn
        assert 'ulcNoConfirmEnabledSnapshot()' not in fn
        assert 'activeTurnSignalVariant == TURN_SIGNAL_UNSET' in fn
        assert 'requestBlinkerTx(dir, BLINKER_TX_SOURCE_S3XY_PURE' in fn
        assert 'sendStalkFrameCanB(uint8_t turn, uint32_t txEpoch, bool directUser)' in v
        assert 'sendStalkFrameCanA(uint8_t turn, uint32_t txEpoch, bool directUser)' in v
        assert 'if (!directUser && ulcNoConfirmEnabledSnapshot()) return;' in v
        assert 'if (!oneShotDirect)' in v
        support=s.split('static bool s3xyActionSupportedForCurrentProfile(uint8_t action) {',1)[1].split('\n}',1)[0]
        assert 'activeTurnSignalVariant != TURN_SIGNAL_UNSET' in support
        assert 'activeProfileAdvancedEapSupported() && activeTurnSignalVariant' not in support
        assert "v!=='auto_blinker_toggle'||!!window.__t2AdvancedEapSupported" in d
        assert "['left_blinker','right_blinker'].includes(v)||!!window.__t2TurnSignalSupported" in d
        assert 'window.__t2TurnSignalSupported=Number(s.turn)!==0' in d
        print('PASS v3.6f1 direct S3XY blinker policy split')

    def test_s3xy_action_ids(self):
        """Retained from test_v37_s3xy_action_ids_static.py."""
        from pathlib import Path
        import re

        ROOT = Path(__file__).resolve().parents[1]
        s3xy = (ROOT / 's3xy_ble.h').read_text()

        expected = [
            ('S3XY_ACTION_NONE', 0, 'none'),
            ('S3XY_ACTION_NOA_CANCEL', 1, 'noa_cancel'),
            ('S3XY_ACTION_ACCEL_MODE_TOGGLE', 2, 'accel_mode_toggle'),
            ('S3XY_ACTION_RESEARCH_CAPTURE_C', 3, 'research_capture_c'),
            ('S3XY_ACTION_RESEARCH_CAPTURE_A', 4, 'research_capture_a'),
            ('S3XY_ACTION_RESEARCH_CAPTURE_B', 5, 'research_capture_b'),
            ('S3XY_ACTION_RESEARCH_CAPTURE_D', 6, 'research_capture_d'),
            ('S3XY_ACTION_RESEARCH_CAPTURE_RESET', 7, 'research_capture_reset'),
            ('S3XY_ACTION_TLSSC_TOGGLE', 8, 'tlssc_toggle'),
            ('S3XY_ACTION_AUTO_BLINKER_TOGGLE', 9, 'auto_blinker_toggle'),
            ('S3XY_ACTION_PERFORMANCE_MODE', 10, 'performance_mode'),
            ('S3XY_ACTION_LEFT_BLINKER', 11, 'left_blinker'),
            ('S3XY_ACTION_RIGHT_BLINKER', 12, 'right_blinker'),
            ('S3XY_ACTION_BATTERY_PREHEAT', 13, 'battery_preheat'),
        ]
        for name, value, code in expected:
            assert re.search(rf'\b{name}\s*=\s*{value}\b', s3xy), (name, value)
            assert f'return "{code}";' in s3xy, code

        assert 'S3XY_ACTION_NAG' not in s3xy
        assert 'nag_killer' not in s3xy.lower()
        print('v3.7 S3XY action IDs remain stable: PASS')

    def test_s3xy_blinker_disabled_ui(self):
        """Retained from test_v36d9a6_s3xy_blinker_disabled_ui_static.py."""
        from pathlib import Path
        R=Path(__file__).resolve().parents[1]
        s=(R/'s3xy_ble.h').read_text()
        v=(R/'vehicle_logic.h').read_text()
        f=(R/'t2can_forward.h').read_text()
        d=(R/'dashboard_source.html').read_text()
        ino=next(R.glob('*.ino')).read_text()
        # Existing persisted IDs remain stable; new actions append only.
        assert 'S3XY_ACTION_PERFORMANCE_MODE = 10' in s
        assert 'S3XY_ACTION_LEFT_BLINKER = 11' in s
        assert 'S3XY_ACTION_RIGHT_BLINKER = 12' in s
        assert 'return "left_blinker"' in s and 'return "right_blinker"' in s
        assert 'Left Blinker' in s and 'Right Blinker' in s
        assert 'requestTurnSignalPulseFromButton(1)' in s
        assert 'requestTurnSignalPulseFromButton(2)' in s
        assert 'static bool requestTurnSignalPulseFromButton(uint8_t dir);' in f
        # Planner-driven Auto Blinker keeps its legacy pulse lifetime. Direct S3XY
        # stalk actions are covered separately and must not reuse this timed burst.
        assert 'requestBlinkerTx(fireRequestDir, BLINKER_TX_SOURCE_AUTO_PURE' in v
        assert 'oneShotReleaseAt = now + BLINKA_STALKLESS_PRESS_MS;' in v
        assert 'activeTurnSignalVariant == TURN_SIGNAL_UNSET' in v
        assert "['left_blinker','Left Blinker']" in d
        assert "['right_blinker','Right Blinker']" in d
        assert '.settingToggleRow:has(:disabled)' in d
        assert 'input:disabled,select:disabled' in d
        assert 'button:disabled' in d
        assert '-webkit-text-fill-color:currentColor' in d
        print('v3.6f1 S3XY blinker + disabled UI static contract: PASS')


if __name__ == "__main__":
    unittest.main()
