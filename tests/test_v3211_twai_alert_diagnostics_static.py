from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
RUNTIME = (ROOT / "can_runtime.h").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")
PERSISTENCE = (ROOT / "can_busoff_persistence.h").read_text(encoding="utf-8")
PURE = (ROOT / "can_busoff_persistence_pure.h").read_text(encoding="utf-8")


assert "struct CanTwaiErrorAlertWindow" in CORE
assert "struct CanTwaiBusOffAlertEvidence" in CORE

handler_start = RUNTIME.index("static void canTwaiHandleAlerts()")
handler_end = RUNTIME.index("static void canTaskMcp(", handler_start)
handler = RUNTIME[handler_start:handler_end]
for alert in (
    "TWAI_ALERT_TX_FAILED",
    "TWAI_ALERT_ERR_PASS",
    "TWAI_ALERT_BUS_ERROR",
    "TWAI_ALERT_BUS_OFF",
):
    assert alert in handler
assert "canTwaiObserveErrorAlerts(" in handler
assert handler.count("twai_get_status_info(") == 1
assert "TWAI_ALERT_TX_SUCCESS" not in handler
assert "CAN_BUS_OFF_RECORD_VERSION_PURE = 2u" in PURE
assert "CAN_BUS_OFF_RECORD_VERSION_V1_PURE = 1u" in PURE
for field in (
    "alertSeenMask",
    "alertBatchMask",
    "txFailedAlertAgeMs",
    "errPassAlertAgeMs",
    "busErrorAlertAgeMs",
):
    assert f"record.snapshot.{field}" in PERSISTENCE
    assert f"canTwaiLastBusOffSnapshot.{field}" in PERSISTENCE

assert "canTwaiResetErrorAlertWindow();" in RUNTIME
assert "canTwaiObserveErrorAlerts(0u, true" in RUNTIME

poll_start = RUNTIME.index("if (st.state == TWAI_STATE_RUNNING)")
poll_end = RUNTIME.index("} else if (st.state == TWAI_STATE_STOPPED)", poll_start)
poll_bus_off = RUNTIME[poll_start:poll_end]
assert "snapshotCapturedMs" in poll_bus_off
assert "snapshotMatchesFrozen" in poll_bus_off
assert "if (!snapshotMatchesFrozen)" in poll_bus_off

for key in (
    "twaiBusOffSnapshotAlertSeenMask",
    "twaiBusOffSnapshotAlertBatchMask",
    "twaiBusOffSnapshotTxFailedAlertAgeMs",
    "twaiBusOffSnapshotErrPassAlertAgeMs",
    "twaiBusOffSnapshotBusErrorAlertAgeMs",
):
    assert f'"{key}"' in API, key
for alert in (
    "TWAI_ALERT_TX_FAILED",
    "TWAI_ALERT_ERR_PASS",
    "TWAI_ALERT_BUS_ERROR",
):
    assert f"busOffSnap.alertSeenMask & {alert}" in API

reset_start = API.index("static bool resetRuntimeStats()")
reset_end = API.index("static void httpResetRuntimeStats()", reset_start)
reset = API[reset_start:reset_end]
for symbol in (
    "canTwaiErrorAlertWindow = {}",
    "canTwaiBusOffAlertEvidence = {}",
):
    assert symbol in reset

print("v3.28.0 TWAI error-alert diagnostics contract: PASS")
