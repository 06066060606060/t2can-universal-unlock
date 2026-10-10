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
