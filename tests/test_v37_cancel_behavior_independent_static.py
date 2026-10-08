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
