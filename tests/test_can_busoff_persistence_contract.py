from pathlib import Path


root = Path(__file__).resolve().parents[1]
adapter = (root / "can_busoff_persistence.h").read_text() if (root / "can_busoff_persistence.h").exists() else ""
runtime = (root / "can_runtime.h").read_text()
api = (root / "web_api.h").read_text()
ino = next(root.glob("*.ino")).read_text()
all_product = adapter + runtime + api + ino


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


for token in [
    'begin("canDiag"', '"a0"', '"a1"', '"b0"', '"b1"',
    "canBusOffPersistenceLoad()", "canBusOffPersistenceMarkDirty(",
    "canBusOffPersistenceService(", "canBusOffPersistenceClear()",
    '"busOffRecordBoot"', '"busOffEventUptimeMs"',
    '"busOffPersistState"', '"busOffPersistError"',
    "PREVIOUS_BOOT", "CURRENT_BOOT",
]:
    assert token in all_product, f"missing persistence contract: {token}"

for signature in [
    "static void recordTwaiBusOffSnapshot(",
    "static void canBTraceFreezeBusOff(",
    "static void canATraceFreezeBusOff(",
]:
    assert "Preferences" not in function_body(runtime, signature)

for signature in ["static void canTaskMcp(", "static void canTaskTwai("]:
    assert "Preferences" not in function_body(runtime, signature)

supervisor = function_body(runtime, "static void canRecoverySupervisorTick(")
assert "canBusOffPersistenceService(" in supervisor
reset = function_body(api, "static bool resetRuntimeStats()")
assert "canBusOffPersistenceClear()" in reset
handler = function_body(api, "static void httpResetRuntimeStats()")
assert "bus-off-persistence-clear-failed" in handler and "server.send(500" in handler

# Normal firmware-settings reset must not erase forensic records; factory reset
# remains explicitly destructive through whole-NVS erase.
assert 'clearNamespace("canDiag")' not in all_product
assert "nvs_flash_erase" in all_product

assert '#include "can_busoff_persistence_pure.h"' in ino
assert '#include "can_busoff_persistence.h"' in ino
assert ino.index('#include "can_core.h"') < ino.index('#include "can_busoff_persistence.h"')

print("dual-slot BUS OFF persistence contract: PASS")
