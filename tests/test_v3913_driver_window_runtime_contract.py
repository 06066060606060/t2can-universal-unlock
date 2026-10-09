from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "T2CAN-Universal-v3.28.0-LP_YL.ino").read_text()
CORE = (ROOT / "can_core.h").read_text()
RUNTIME = (ROOT / "can_runtime.h").read_text()
LOGIC = (ROOT / "vehicle_logic.h").read_text()
WEB = (ROOT / "web_api.h").read_text()

assert '#include "driver_window_lab_pure.h"' in INO
assert "CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB" in CORE

assert "static void handleDriverWindowLab3C2CanB(" in LOGIC
profile_gate = LOGIC.split("static bool driverWindowLabProfileSupported()", 1)[1].split("\n}", 1)[0]
assert "driverWindowLabProfileSupportedPure" in profile_gate
assert "activeCanBIsChassis()" not in profile_gate
handler = LOGIC.split("static void handleDriverWindowLab3C2CanB(", 1)[1]
handler = handler.split("\n}\n", 1)[0]
assert "driverWindowLabConsumeStockPure" in handler
assert "CAN_TX_FRESH_VH" in handler
assert "CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB" in handler
assert "canTxTwaiTransmitWithMaskTaggedGuarded" in handler
assert "driverWindowLabArmContextSnapshot" in handler
assert "twai_transmit(" not in handler
assert "DRIVER_WINDOW_CONSUME_TX_FAILED" in handler
assert "driverWindowLabLastConsumeReason = DRIVER_WINDOW_CONSUME_TX_FAILED" in handler

case = RUNTIME.split("case VCLEFT_SWITCH_ID:", 1)[1].split("break;", 1)[0]
assert "activeProfileIsYl()" in case
assert "handleDriverWindowLab3C2CanB(f);" in case
assert "driverWindowLabResetRuntimeUnderTxBarrier();" in RUNTIME

assert "static void httpDriverWindowLabStats()" in WEB
assert "static void httpDriverWindowLabOpen()" in WEB
open_handler = WEB.split("static void httpDriverWindowLabOpen()", 1)[1]
open_handler = open_handler.split("\n}\n", 1)[0]
assert "driverWindowLabRequestOpen" in open_handler
assert "twai_transmit(" not in open_handler
assert "canTxTwaiTransmit" not in open_handler
assert 'server.on("/api/lab/driver-window/stats", HTTP_GET, httpDriverWindowLabStats);' in WEB
assert 'server.on("/api/lab/driver-window/open", HTTP_POST, httpDriverWindowLabOpen);' in WEB
assert "driverWindowLabResetRuntime();" in WEB.split("static void httpFeatureLab()", 1)[1].split("static void httpFeatureDoorCancel", 1)[0]
assert "canTxCancellationGenerationAdvance" in LOGIC
assert "driverWindowLabServiceTick();" in RUNTIME
assert "driverWindowLabResetRuntimeUnderTxBarrier();" in RUNTIME
assert "driverWindowLabLastResult" in LOGIC
assert "DRIVER_WINDOW_RESULT_COMPLETED" in LOGIC
stats = WEB.split("static String driverWindowLabStatsToJson()", 1)[1].split("static void httpDriverWindowLabStats", 1)[0]
assert "driverWindowLabLastResult" in stats
assert "completed != 0u" not in stats

for field in (
    '"supported"', '"available"', '"pending"', '"stockAgeMs"',
    '"requests"', '"completed"', '"txOk"', '"txFail"',
    '"blocked"', '"lastResult"', '"stockRaw"', '"lastTxRaw"'
):
    assert field in WEB

print("PASS v3.28.0 driver-window LAB runtime/API contract")
