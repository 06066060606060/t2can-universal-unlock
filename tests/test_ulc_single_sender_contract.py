from pathlib import Path


root = Path(__file__).resolve().parents[1]
logic = (root / "vehicle_logic.h").read_text()
runtime = (root / "can_runtime.h").read_text()
stalk = (root / "ulc_stalk_confirm_pure.h").read_text()
api = (root / "web_api.h").read_text()
sources = logic + runtime + stalk + api


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


inject = function_body(
    logic, "static void injectDriverAssistControl(const twai_message_t &src)"
)
assert inject.count("canTxTwaiTransmitValidated(") == 1
assert "ulcCompose3f8Pure(" in inject
assert "ulcNoConfirmInjectCanA" not in runtime
for removed in [
    "ulcNoConfirmTargetBus",
    "ULC_NO_CONFIRM_BUS_A_PURE",
    "ulcNoConfirmEffectiveTargetBus",
    "ulcNoConfirmTxAOk",
    "ulcNoConfirmTxAFail",
]:
    assert removed not in sources, f"old target-bus symbol remains: {removed}"

print("single CAN-B 0x3F8 compositor contract: PASS")
