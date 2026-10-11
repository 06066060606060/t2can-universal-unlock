"""Diagnostics contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class DiagnosticsContracts(unittest.TestCase):

    def test_can_busoff_persistence(self):
        """Retained from test_can_busoff_persistence_contract.py."""
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

    def test_can_usb_logger(self):
        """Retained from test_can_usb_logger_contract.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        header = root / 'can_usb_logger_pure.h'
        assert header.exists(), 'USB full-frame logger and bounded queue are not implemented'
        ino = next(root.glob('*.ino')).read_text()
        runtime = (root / 'can_runtime.h').read_text()
        assert ino.index('#include "can_usb_logger.h"') < ino.index('#include "can_core.h"')
        assert 'Serial.begin(115200);' in ino and 'canUsbStartTask();' in ino
        assert runtime.index('canUsbObserve(0,') < runtime.index('if ((rxf.can_id & 0xC0000000UL)')
        twai = runtime.split('static void canTaskTwai(', 1)[1]
        assert twai.index('canUsbObserve(1,') < twai.index('r79ProcessStockFrame(')
        assert 'if (canUsbPassive())' in runtime.split('static void canSupervisorTask(', 1)[1]
        print('PASS USB raw frame hooks and survey single-bus integration')

    def test_dashboard_diagnostics_visibility(self):
        """Retained from test_dashboard_diagnostics_visibility_static.py."""
        from pathlib import Path
        import re


        root = Path(__file__).resolve().parents[1]
        dashboard = (root / "dashboard_source.html").read_text()

        assert 'id="diagBleLabel"' in dashboard
        render = re.search(r"function renderDevices\(s\)\{(.*?)\nfunction ", dashboard, re.S)
        assert render
        body = render.group(1)
        assert "diagBleLabel" in body and "diagBleCard" in body
        assert "diagBleLabel.style.display=diagDisplay" in body
        assert "diagBleCard.style.display=diagDisplay" in body

        assert "PREVIOUS BOOT" in dashboard
        assert "busOffEventUptimeMs" in dashboard
        assert "busOffRecordBoot" in dashboard
        assert "if(!r.ok)" in dashboard and "bus-off-persistence-clear-failed" in dashboard
        assert "BUS OFF evidence" in dashboard
        print("dashboard diagnostics visibility/persistence contract: PASS")

    def test_diagnostics(self):
        """Retained from test_v36b6_diagnostics_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        core = (root / 'can_core.h').read_text(encoding='utf-8').replace('#include "mode_h_runtime.h"', (root / "mode_h_runtime.h").read_text())
        runtime = (root / 'can_runtime.h').read_text(encoding='utf-8')
        api = (root / 'web_api.h').read_text(encoding='utf-8')
        logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
        capture = (root / 'can_research_capture.h').read_text(encoding='utf-8')
        forward = (root / 't2can_forward.h').read_text(encoding='utf-8')
        pins = (root / 'pin_config.h').read_text(encoding='utf-8')
        html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
        ino_files = list(root.glob('*.ino'))
        ino = ino_files[0].read_text(encoding='utf-8')

        def has_json_key(text: str, key: str) -> bool:
            return f'\\"{key}\\"' in text or f'"{key}"' in text

        # Version target.

        # CAN A forensic ring must observe every MCP application TX through the single hardware gate.
        assert 'CAN_A_TX_TRACE_CAPACITY = 64' in core
        assert 'struct CanATxTraceEntry' in core
        assert 'static void canATraceRecordTx(' in core
        tagged_start = core.index('static bool canTxMcpSendTagged(')
        mcp_start = core.index('static bool canTxMcpSend(', tagged_start)
        mcp_end = core.index('// ═══════════════════════════════════════════════════════════════\n// CAN RECOVERY SUPERVISOR', mcp_start)
        mcp_tagged_fn = core[tagged_start:mcp_start]
        mcp_fn = core[mcp_start:mcp_end]
        assert 'canATraceRecordTx(' in mcp_tagged_fn
        assert 'canTxMcpSendTagged(' in mcp_fn

        # BUS-OFF detection must freeze CAN A forensic data before MCP reinit; normal recovery conditions stay intact.
        assert 'static void canATraceFreezeBusOff(' in runtime
        status_start = runtime.index('// ── STATUS CHECK / RECOVERY (1 Hz) ──')
        status_end = runtime.index('vTaskDelay(1);', status_start)
        status = runtime[status_start:status_end]
        assert 'bool busOff = (eflg & MCP2515::EFLG_TXBO) || (consecutive > 5);' in status
        assert 'canATraceFreezeBusOff(' in status
        assert status.index('canATraceFreezeBusOff(') < status.index('mcpReinit()')

        # CAN A diagnostics are exposed without removing the existing CAN B forensic path.
        for key in (
            'mcpErrorFlags','mcpTxFailConsecutive','mcpBusOffCount','mcpBusOffSnapshotValid',
            'mcpBusOffSnapshotAgeMs','mcpBusOffSnapshotEflg','mcpBusOffSnapshotTxFailSeq',
            'mcpBusOffSnapshotRxAgeMs','canATxTraceCount','canATxTraceAgeMs','canATxTraceBusOffOrdinal',
        ):
            assert has_json_key(api, key), f'missing system diagnostic key {key}'
        assert 'static void httpCanATxTraceCsv()' in api
        assert 'server.on("/api/system/cana-tx.csv"' in api
        assert 'server.on("/api/system/canb-tx.csv"' in api
        assert 'CAN A TX CSV' in html
        assert 'id="diagCanATxCsv"' in html
        assert 'id="diagCanBTxCsv"' in html
        assert 'canATraceReset();' in api
        assert 'canBTraceReset();' in api

        # CAN Research CSV progress: server advertises row count and UI reads stream progress.
        research_start = api.index('static void httpResearchCaptureCsv() {')
        research_end = api.index('static void httpSystemStats()', research_start)
        research_fn = api[research_start:research_end]
        assert 'X-T2CAN-CSV-Rows' in research_fn
        assert 'researchCaptureCsvExportRows' in research_fn
        assert 'async function downloadResearchCaptureCsv()' in html
        assert "response.body.getReader()" in html
        assert 'new Blob(parts' in html
        assert 'downloadProgress' in html
        assert "Downloading… ${downloadProgress}%" in html
        assert "$('researchCapDownload').onclick=downloadResearchCaptureCsv" in html
        assert 'if(otaUploading||downloadInProgress||document.hidden)return;' in html
        assert "downloadFile('/api/researchcapture/log.csv'" not in html

        # Final source-level dead-code cleanup: remove only unreachable wrappers/branches.
        for symbol in (
            'autoBlinkerEligibleRequestDir', 'researchCaptureObserveTxParty',
            'canTxFreshMaskSnapshot', 'static inline uint8_t sccm249Crc8(',
        ):
            assert symbol not in (core + runtime + logic + capture + forward), f'dead symbol remains: {symbol}'
        for macro in ('MCP2515_INT', 'T_2Can_Fd', 'MCP2518_CS', 'MCP2518_INT', 'ESP_BOOT'):
            assert macro not in pins, f'dead pin/config macro remains: {macro}'
        # Keep the actually-used pure SCCM checksum implementation.
        assert 'sccm249Crc8Pure' in (root / 'auto_blinker_pure.h').read_text(encoding='utf-8')
        # labLite used to compute a separate laneValid local that was never consumed.
        lab_start = api.index('static String labLiteSnapshotToJson() {')
        lab_end = api.index('// Lightweight HOME snapshot.', lab_start)
        assert 'const bool laneValid =' not in api[lab_start:lab_end]

        print('v3.6b6 diagnostics/progress/dead-code contract OK')

    def test_can_traffic_snapshot(self):
        """Retained from test_v36d9a1_can_traffic_snapshot_static.py."""
        from pathlib import Path
        ROOT=Path(__file__).resolve().parents[1]
        web=(ROOT/"web_api.h").read_text()
        ino=next(ROOT.glob("*.ino")).read_text()
        const=web.index('static constexpr uint32_t CAN_TRAFFIC_UI_FRESH_MS = 1500;')
        struct=web.index('struct CanTrafficUiSnapshot')
        helper=web.index('static CanTrafficUiSnapshot canTrafficUiSnapshot()')
        first_use=web.index('static String canTrafficStatsToJson()')
        assert const < struct < helper < first_use
        for fn in ('canTrafficStatsToJson','systemStatsToJson','homeFastSnapshotToJson','homeSnapshotToJson'):
            pos=web.index('static String '+fn+'()')
            assert struct < pos and helper < pos
        assert web.count('struct CanTrafficUiSnapshot') == 1
        assert web.count('CAN_TRAFFIC_UI_FRESH_MS = 1500') == 1
        print('v3.6d9a2 CAN traffic UI snapshot compile-order contract: PASS')

    def test_diagnostic(self):
        """Retained from test_v3911_diagnostic_contract.py."""
        from pathlib import Path
        import re


        ROOT = Path(__file__).resolve().parents[1]
        INO = (ROOT / (ROOT.name + ".ino")).read_text()
        CORE = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        RUNTIME = (ROOT / "can_runtime.h").read_text()
        API = (ROOT / "web_api.h").read_text()

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

        print("PASS v3.29.1 CAN task and CAN A RX diagnostic integration contract")

    def test_twai_alert_diagnostics(self):
        """Retained from test_v3211_twai_alert_diagnostics_static.py."""
        from pathlib import Path


        ROOT = Path(__file__).resolve().parents[1]
        CORE = (ROOT / "can_core.h").read_text(encoding="utf-8").replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
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

        print("v3.29.1 TWAI error-alert diagnostics contract: PASS")

    def test_research_capture_mode(self):
        """Retained from test_v3914_research_capture_mode_contract.py."""
        from pathlib import Path
        import re


        ROOT = Path(__file__).resolve().parents[1]
        INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
        CAPTURE = (ROOT / "can_research_capture.h").read_text(encoding="utf-8")
        API = (ROOT / "web_api.h").read_text(encoding="utf-8")
        HTML = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


        # SNAPSHOT and RAW TRANSITION share one fixed PSRAM allocation. A mode change
        # must not free the working buffers and attempt a larger fragmented allocation.
        assert "RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY = 163840" in CAPTURE
        assert "RESEARCH_CAPTURE_FIXED_MAIN_BYTES" in CAPTURE
        assert "RESEARCH_CAPTURE_FIXED_AUX_BYTES" in CAPTURE
        set_mode = CAPTURE[
            CAPTURE.index("static bool researchCaptureSetMode(uint8_t mode)"):
            CAPTURE.index("static inline uint16_t researchCaptureStateIndex", CAPTURE.index("static bool researchCaptureSetMode(uint8_t mode)"))
        ]
        assert "researchCaptureReleaseModeBuffers();" not in set_mode

        # Only the two supported user modes remain selectable and accepted by the API.
        select = re.search(r'<select[^>]+id="researchCapMode"[^>]*>(.*?)</select>', HTML, re.S)
        assert select
        options = re.findall(r'<option[^>]+value="([^"]+)"', select.group(1))
        assert options == ["SNAPSHOT", "RAW_TRANSITION"]

        mode_handler = API[
            API.index("static void httpResearchCaptureMode()"):
            API.index("static void httpResearchCaptureReset()", API.index("static void httpResearchCaptureMode()"))
        ]
        assert "RAW_AUTO_ALC" not in mode_handler
        assert "ULC_CONFIRM" not in mode_handler
        assert "mode must be SNAPSHOT or RAW_TRANSITION" in mode_handler

        # A rejected mode change must be surfaced instead of silently snapping the UI
        # back to the previous server value.
        save_mode = HTML[
            HTML.index("async function saveResearchMode()"):
            HTML.index("async function saveAllResearchLabels()")
        ]
        assert "capture mode change failed" in save_mode
        assert "alert(" in save_mode

        print("PASS v3.29.1 stable two-mode research capture contract")


if __name__ == "__main__":
    unittest.main()
