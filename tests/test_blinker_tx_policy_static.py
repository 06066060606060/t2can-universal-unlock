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
