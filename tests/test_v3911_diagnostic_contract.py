from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "T2CAN-Universal-v3.26.3-LP_YL.ino").read_text()
CORE = (ROOT / "can_core.h").read_text()
RUNTIME = (ROOT / "can_runtime.h").read_text()
API = (ROOT / "web_api.h").read_text()

assert INO.startswith("// T2CAN Universal v3.26.3")
assert '#define FW_VERSION "v3.26.3"' in INO
assert '#include "can_task_diagnostics_pure.h"' in INO

for symbol in (
    "canTaskMcpDiagnostics",
    "canTaskTwaiDiagnostics",
    "canTaskHeartbeatLastSnapshotA",
    "canTaskHeartbeatLastSnapshotB",
    "canARxDiagnostics",
):
    assert symbol in CORE

for call in (
    r"canTaskDiagnosticsHeartbeatPure\s*\(\s*canTaskMcpDiagnostics",
    r"canTaskDiagnosticsHeartbeatPure\s*\(\s*canTaskTwaiDiagnostics",
    r"canTaskDiagnosticsSnapshotPure\s*\(\s*canTaskMcpDiagnostics",
    r"canTaskDiagnosticsSnapshotPure\s*\(\s*canTaskTwaiDiagnostics",
    r"canARxDiagnosticsCompleteLoopPure\s*\(\s*canARxDiagnostics",
    r"canARxDiagnosticsObserveOverflowPure\s*\(\s*canARxDiagnostics",
):
    assert re.search(call, RUNTIME)

for key in (
    "canTaskSnapshotAStateName",
    "canTaskSnapshotAStageName",
    "canTaskSnapshotAStageAgeMs",
    "canTaskSnapshotALoopCount",
    "canTaskSnapshotAMaxLoopUs",
    "canTaskSnapshotBStateName",
    "canTaskSnapshotBStageName",
    "canTaskSnapshotBStageAgeMs",
    "canTaskSnapshotBLoopCount",
    "canTaskSnapshotBMaxLoopUs",
    "canARxFramesProcessed",
    "canARxMaxFramesPerLoop",
    "canARxBudgetExhaustedLoops",
    "mcpRx0OverflowObservations",
    "mcpRx1OverflowObservations",
    "mcpRxFramesAtLastOverflow",
    "mcpRxBudgetHitsAtLastOverflow",
):
    assert f'jw.' in API and f'"{key}"' in API

assert "canTaskDiagnosticsResetPure(canTaskMcpDiagnostics)" in API
assert "canTaskDiagnosticsResetPure(canTaskTwaiDiagnostics)" in API
assert "canARxDiagnosticsResetPure(canARxDiagnostics)" in API

supervisor = RUNTIME[
    RUNTIME.index("static void canSupervisorTask("):
    RUNTIME.index("// ═══════════════════════════════════════════════════════════════\n// SETUP / LOOP")
]
persistence = supervisor.index("canRecoverySupervisorTick(now);")
fresh_now = supervisor.index("now = (uint32_t)millis();", persistence)
heartbeat_check = supervisor.index("canTaskMcpHeartbeatMs", fresh_now)
assert persistence < fresh_now < heartbeat_check
assert supervisor.count("canTaskHeartbeatTimedOutPure(") == 2
assert "(uint32_t)(now - canTaskMcpHeartbeatMs)" not in supervisor
assert "(uint32_t)(now - canTaskTwaiHeartbeatMs)" not in supervisor

print("PASS v3.26.3 CAN task and CAN A RX diagnostic integration contract")
