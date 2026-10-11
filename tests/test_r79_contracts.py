"""R79 contracts; original checks remain executable Python.

Each method has its own locals and helper definitions. See COVERAGE.md
for the former file mapping and retained assertion counts.
"""
import unittest
from contract_test_support import run_contract_cases


class R79Contracts(unittest.TestCase):

    def test_r79_dashboard(self):
        """Retained from test_r79_dashboard_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        api = (root / 'web_api.h').read_text()
        html = (root / 'dashboard_source.html').read_text()
        logic = (root / 'vehicle_logic.h').read_text()

        for key in ('fixedPolicy', 'transport', 'periodic', 'bit18Mode', 'bit18ModeName',
                    'runtimeState', 'stockTemplateValid', 'stockMux1Rx', 'txOk', 'txFail',
                    'fastAttempts', 'fastTxOk', 'fastTxFail', 'periodicTxOk',
                    'periodicTxFail', 'quietArm', 'quietFire', 'quietGuardSkip'):
            assert f'"{key}"' in api, f'missing R79 API field {key}'

        assert '/api/r79/stats' in api and '/api/r79/update' in api
        assert 'R79 Status' in html and 'Read-only production status' in html
        assert "jget('/api/r79/stats')" in html
        for dom in ('r79FixedPolicyValue', 'r79FixedTransportValue', 'r79FixedPeriodicValue',
                    'r79FixedBit18Value', 'r79FixedRuntimeValue', 'r79FixedTotalsValue',
                    'r79FixedFastValue', 'r79FixedPeriodicTotalsValue', 'r79FixedQuietValue'):
            assert f'id="{dom}"' in html

        panel = html[html.index('<section class="panel" id="panelLabR79"'):html.index('<section class="panel" id="panelNagModeH"')]
        for interactive in ('<input', '<select', '<button class="btn"'):
            assert interactive not in panel, f'R79 status card must remain read-only: {interactive}'
        for retired in ('POST-MUX2 TX', 'PRE-MUX1 TX', 'R79 Transport', 'ROAMING Mirror Ratio'):
            assert retired not in panel

        assert 'r79FixedFastEcho' in logic and 'r79FixedTick' in logic
        print('R79 v3.7 dashboard/API static checks passed')

    def test_r79_mode2_runtime(self):
        """Retained from test_r79_mode2_runtime_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        ino = next(root.glob('*.ino')).read_text()
        logic = (root / 'vehicle_logic.h').read_text()
        runtime = (root / 'can_runtime.h').read_text()
        api = (root / 'web_api.h').read_text()
        html = (root / 'dashboard_source.html').read_text()

        assert '#include "r79_fixed_policy_pure.h"' in ino
        for token in (
            'r79TransportMode', 'r79Mode1TxWaitMode', 'r79Mode1ReinjectEnabled',
            'r79Mode1DelayMs',
            'r79Mode2ReinjectEnabled', 'r79Mode2DelayMs',
            'R79Mode2DelayedStatePure r79Mode2DelayedState',
        ):
            assert token in logic

        # One dispatcher selects exactly one transport for each received stock frame.
        assert 'static bool r79ProcessStockFrame(' in logic
        dispatch_start = logic.rindex('static bool r79ProcessStockFrame(')
        dispatch_end = logic.index('\n}', dispatch_start) + 2
        dispatch = logic[dispatch_start:dispatch_end]
        assert 'R79_MODE_2_PURE' in dispatch
        assert 'r79Mode2FastEcho' in dispatch
        assert 'r79FixedFastEcho' in dispatch
        assert 'r79Mode2ObserveStock' in dispatch
        assert 'r79FixedObserveStock' in dispatch
        assert 'r79ProcessStockFrame(f' in runtime
        assert 'r79FixedFastEcho(f)' not in runtime

        # Mode 2 is a single non-blocking attempt. Queue flush and retry scheduling
        # remain Mode-1-only and cannot occur from either Mode 2 sender.
        m2_fast_start = logic.rindex('static bool r79Mode2FastEcho(')
        m2_fast_end = logic.index('static void r79Mode2ObserveStock(', m2_fast_start)
        m2_fast = logic[m2_fast_start:m2_fast_end]
        assert 'r79Mode2ApplyBitsPure' in m2_fast
        assert 'r79ApGateTransmitGuarded(&out, 0u, apGeneration)' in m2_fast
        assert 'r79FastReactiveGateOpen()' in m2_fast
        assert 'twai_clear_transmit_queue' not in m2_fast
        assert 'r79RetrySchedule' not in m2_fast
        guard_start = logic.index('static esp_err_t r79ApGateTransmitGuarded(')
        guard_end = logic.index('static esp_err_t r79LabDirectTwaiTransmit(', guard_start)
        assert 'r79DmsApplyFinal' in logic[guard_start:guard_end]
        dms_start = logic.index('static bool r79DmsControlActive()')
        dms_end = logic.index('static inline bool r79DmsApplyFinal', dms_start)
        assert 'driverMonitoringControlSnapshot()' in logic[dms_start:dms_end]
        assert 'nagCfg.' not in logic[dms_start:dms_end]
        assert 'R79_MODE_1_PURE' not in logic[dms_start:dms_end]

        m2_tick_start = logic.rindex('static void r79Mode2Tick()')
        m2_tick_end = logic.index('static void r79TransportTick()', m2_tick_start)
        m2_tick = logic[m2_tick_start:m2_tick_end]
        assert 'r79Mode2DelayedStepPure' in m2_tick
        assert 'r79RuntimeStatusSnapshot' in m2_tick  # common manual D/R suppression
        assert 'r79ApGateTransmitGuarded(&out, 0u, apGeneration)' in m2_tick
        assert 'r79RetrySchedule' not in m2_tick
        assert 'twai_clear_transmit_queue' not in m2_tick
        assert 'r79TransportTick();' in runtime
        assert 'r79FixedTick();' not in runtime

        # Persistent settings and status/API surface.
        for token in ('"mode"', '"m1wait"', '"m1re"', '"m1delay"', '"m2re"', '"m2delay"'):
            assert token in logic
        for key in ('"mode"', '"modeName"', '"mode1TxWaitMode"',
                    '"mode1ReinjectEnabled"', '"mode1DelayMs"', '"mode2ReinjectEnabled"',
                    '"mode2DelayMs"'):
            assert key in api
        for arg in ('server.hasArg("mode")', 'server.hasArg("mode1TxWaitMode")',
                    'server.hasArg("mode1Reinject")', 'server.hasArg("mode1DelayMs")',
                    'server.hasArg("mode2Reinject")',
                    'server.hasArg("mode2DelayMs")'):
            assert arg in api

        for dom in ('r79Mode', 'r79Mode1TxWait', 'r79Mode1Reinject', 'r79Mode1DelayMs',
                    'r79Mode1Controls', 'r79Mode2Reinject', 'r79Mode2DelayMs',
                    'r79Mode2Controls'):
            assert f'id="{dom}"' in html
        assert 'updateR79Settings' in html

        print('R79 Mode 2 runtime/dashboard contract OK')

    def test_r79_runtime(self):
        """Retained from test_r79_runtime_static.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        vl = (root / 'vehicle_logic.h').read_text()
        cr = (root / 'can_runtime.h').read_text()
        pure = (root / 'summon_state_pure.h').read_text()

        assert 'r79TxDecisionPure(' in vl
        assert 'gear118Raw' in vl and 'gear186Raw' in vl
        assert 'static uint8_t r79LabGateReason(' not in vl
        assert 'canTxTwaiTransmitWithMaskWait(&out' not in vl
        assert 'canTxAdministrativeHold' in vl and 'twaiReady' in vl
        assert 'r79LabPending' not in vl and 'r79LabPending' not in cr
        assert 'R79PendingPure' not in pure and 'r79Pending' not in pure
        assert 'R79_REASSERT_RETRY_MS' not in vl
        assert 'r79LabFreshMaskWaitCount' not in vl

        start = vl.index('static void r79FixedTick() {')
        end = vl.index('static bool setTlsscEnabled', start)
        periodic = vl[start:end]
        assert 'if (!summoning) return;' not in periodic
        assert 'r79FixedQuietStepPure' in periodic
        assert 'r79QuietFireCount++' in periodic
        assert 'r79FixedQuietHardDeadlinePure(r79FixedQuietState)' in periodic
        assert '(void)r79LabTransmitShadow(' in periodic
        assert 'stock, R79LAB_TX_PERIODIC, hardDeadlineMs' in periodic
        assert 'r79TransportStrategy' not in periodic
        assert 'r79LabSchedulerMode' not in periodic

        shadow_start = vl.rindex('static bool r79LabTransmitShadow(')
        shadow_end = vl.index('static void r79LabRetryTick()', shadow_start)
        shadow = vl[shadow_start:shadow_end]
        assert 'r79FixedDeadlineExpiredPure(now, hardDeadlineMs)' in shadow
        assert 'const uint32_t now = (uint32_t)millis();' in shadow
        assert 'r79FixedDeadlineWaitBudgetPure(' in shadow
        assert 'r79LabTransmitOnce(' in shadow
        assert '&r79Mode1PostMux2Generation, expectedGeneration' in shadow
        assert 'r79LabDirectTwaiTransmitGuarded' in vl

        retry_start = vl.rindex('static void r79LabRetryTick()')
        retry_end = vl.index('static bool r79FixedFastEcho(', retry_start)
        retry = vl[retry_start:retry_end]
        assert 'r79FixedDeadlineExpiredPure(attemptNow, hardDeadlineMs)' in retry
        assert 'r79LabTransmitOnce(' in retry and 'waitMs' in retry

        assert 'r79ProcessStockFrame(f' in cr
        fast_call = cr.index('r79ProcessStockFrame(f')
        for later in ('bootCaptureObserveVhFrame', 'researchCaptureObserveVh',
                      'driverMonitorCaptureObserve'):
            assert fast_call < cr.index(later, fast_call)
        fixed_start = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
        fixed_end = vl.index('static void r79FixedObserveStock(', fixed_start)
        fast = vl[fixed_start:fixed_end]
        assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
        assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast

        print('R79 v3.7 runtime static checks passed')

    def test_fixed_ulc_route_and_driver_monitor(self):
        """Retained from test_v36b12_static.py."""
        from pathlib import Path
        ROOT = Path(__file__).resolve().parents[1]
        core = (ROOT/'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        logic = (ROOT/'vehicle_logic.h').read_text()
        runtime = (ROOT/'can_runtime.h').read_text()
        web = (ROOT/'web_api.h').read_text()
        dash = (ROOT/'dashboard_source.html').read_text()
        dm = (ROOT/'driver_monitor_capture.h').read_text()
        pure = (ROOT/'driver_monitor_capture_pure.h').read_text()

        # Mode H engine-specific behavior moved to the current b19 contract test.

        # 0x3F8 now has one CAN-B compositor and a fixed Confirm-Free route.
        assert 'ulcCompose3f8Pure' in logic
        assert 'lab3f8ObserveCanA' not in logic
        assert 'ulcNoConfirmTargetBus' not in logic
        assert 'ulcNoConfirmInjectCanA' not in logic
        assert 'injectDriverAssistControl' in runtime
        for token in ['driverAssistRxB', 'driverAssistRawB', 'stalkConfirmRoute']:
            assert token in web, token
        assert 'driverAssistRxA' not in web
        assert 'stalkConfirmTargetBus' not in web

        # Driver Monitoring now includes DAS_status and EPAS_sysStatus on YL Party CAN.
        assert '0x399' in pure and '0x370' in pure
        assert 'DRIVER_MONITOR_ID_DAS_STATUS' in dm
        assert 'DRIVER_MONITOR_ID_EPAS_STATUS' in dm
        assert 'das_hands_on_raw' in web
        assert 'epas_hands_on_raw' in web
        assert 'epas_torque_nm' in web

        print('fixed CAN-B ULC compositor and driver monitor contract passed')

    def test_r79_fast_path(self):
        """Retained from test_v36d2_r79_fast_path_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob('*.ino')).read_text()
        vl = (ROOT / 'vehicle_logic.h').read_text()
        cr = (ROOT / 'can_runtime.h').read_text()
        web = (ROOT / 'web_api.h').read_text()
        dash = (ROOT / 'dashboard_source.html').read_text()

        assert '#include <esp_timer.h>' not in ino

        assert 'esp_timer_get_time()' not in cr + vl
        call = cr.index('r79ProcessStockFrame(f')
        for marker in ('const uint32_t frameNow', 'canRxObserve(CAN_RX_BUS_VH',
                       'bootCaptureObserveVhFrame', 'researchCaptureObserveVh',
                       'driverMonitorCaptureObserve'):
            assert call < cr.index(marker, call), f'fast echo must precede {marker}'

        a = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
        fast = vl[a:vl.index('static void r79FixedObserveStock(', a)]
        assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
        assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast
        assert 'summonPriorityAllowsR79FlushPure' in fast
        assert 'r79ImmediateClearTransmitQueueGuarded(apGeneration)' in fast
        assert 'twai_clear_transmit_queue()' not in fast
        assert 'r79RetrySchedule(R79LAB_TX_IMMEDIATE' in fast

        flush = vl[vl.index('static bool r79ImmediateClearTransmitQueueGuarded'):
                   vl.index('static bool r79Mode1PostMux2ClearTransmitQueueGuarded')]
        assert 'xSemaphoreTake(canTxBarrierMutex, 0)' in flush
        assert 'r79ApGateGenerationSnapshot() == expectedApGeneration' in flush
        assert 'r79FastReactiveGateOpen()' in flush
        assert 'twai_clear_transmit_queue()' in flush

        gate = vl[vl.index('static bool r79FastReactiveGateOpen'):vl.index('static void r79FastReactiveRecordTxState')]
        assert 'r79ManualSuppression' in gate and 'r79TxDecisionPure' in gate
        assert 'r79FreshGearRawLocked' not in gate and 'gearValid' not in gate

        assert 'static volatile uint8_t r79Bit18Policy = R79_BIT18_DEFAULT_PURE;' in vl
        cfg = vl[vl.index('static void r79CfgLoad'):vl.index('static bool r79CfgSave')]
        assert 'prefs.getUChar("bit18", R79_BIT18_DEFAULT_PURE)' in cfg
        assert 'r79Bit18PolicySanitizePure(stored)' in cfg

        for key in ('fixedPolicy', 'transport', 'periodic', 'bit18Mode', 'bit18ModeName',
                    'fastAttempts', 'fastTxOk', 'fastTxFail'):
            assert f'"{key}"' in web, key
        for dom in ('r79FixedPolicyValue', 'r79FixedTransportValue', 'r79FixedFastValue'):
            assert f'id="{dom}"' in dash, dom

        print('v3.7 R79 fixed fast path static contract: PASS')

    def test_r79_followup_and_can_read_budget(self):
        """Retained from test_v36d3_followup_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob('*.ino')).read_text()
        vl = (ROOT / 'vehicle_logic.h').read_text()
        cc = (ROOT / 'can_core.h').read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        cr = (ROOT / 'can_runtime.h').read_text()
        web = (ROOT / 'web_api.h').read_text()
        dash = (ROOT / 'dashboard_source.html').read_text()
        fixed = (ROOT / 'r79_fixed_policy_pure.h').read_text()

        assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
        assert 'R79_FIXED_QUIET_DELAY_MS_PURE = 150u' in fixed
        assert 'D9_MUX1_2MS_WAIT' in web
        assert 'D9_MUX1_FAST_ECHO_0MS' in web
        assert 'String periodic = "MUX2_PLUS_"' in web

        # Select arrows retain their background image when the current skin changes
        # only background-color. The original form rule owns repeat and placement.
        assert '.selectArrow{background-image:url(' in dash
        assert 'background-repeat:no-repeat!important' in dash
        assert 'body.t2-2027 .selectArrow,body.t2-2027 .attach,body.t2-2027 .labSelect' in dash
        assert 'background-color:transparent!important' in dash

        # Generic CSV download must clear pause state in finally and resume polling.
        fn_start = dash.index('async function downloadFile(')
        fn_end = dash.index('\n', fn_start)
        dlfn = dash[fn_start:fn_end]
        assert 'await fetch(' in dlfn and 'new Blob(' in dlfn and 'saveDownloadedBlob(' in dlfn
        assert 'finally{' in dlfn and 'downloadInProgress=false' in dlfn and 'downloadManaged=false' in dlfn
        assert 'forcePoll()===' not in dlfn
        assert 'setTimeout(()=>forcePoll(),0)' in dlfn

        assert 'Session avg/min · bus / arb / BUS OFF' in dash
        assert 'canDiagRatePrev' not in dash

        assert 'MCP_PREFETCH_CAPACITY = canARxReadBatchBudgetPure()' in cr
        pref = cr[cr.index('MCP_PREFETCH_CAPACITY = canARxReadBatchBudgetPure()'):cr.index('// ── STATUS CHECK / RECOVERY', cr.index('MCP_PREFETCH_CAPACITY = canARxReadBatchBudgetPure()'))]
        assert 'Can_A.readMessage(&prefetched[batch])' in pref
        assert 'processed < MCP_RX_BUDGET' in pref
        assert 'batch < readBatchBudget' in pref
        assert 'canARxReadBatchBudgetPure()' in pref
        assert pref.index('Can_A.readMessage(&prefetched[batch])') < pref.index('canRxObserve(CAN_RX_BUS_PARTY')

        assert 'TWAI_ALERT_TX_SUCCESS' not in cr
        assert 'r79FastReactiveObserveTxSuccessAlert' not in cr + vl
        assert 'canBTxAcceptedSerial' not in cc and 'canBTxPipelineIdle' not in cc

        reset = web[web.index('static bool resetRuntimeStats'):web.index('static void httpResetRuntimeStats', web.index('static bool resetRuntimeStats'))]
        for name in ('r79FastEchoAttempts', 'r79FastEchoTxOk', 'r79FastEchoTxFail'):
            assert f'{name} = 0' in reset, name

        print('v3.7 follow-up preservation contract: PASS')

        rx_policy = (Path(__file__).resolve().parents[1] / "runtime_gate_pure.h").read_text()
        assert "CAN_A_RX_IMMEDIATE = 1u" in rx_policy
        assert "return CAN_A_RX_IMMEDIATE;" in rx_policy

    def test_post_mux1_mode(self):
        """Retained from test_v36d9a2_post_mux1_mode_static.py."""
        from pathlib import Path

        R = Path(__file__).resolve().parents[1]
        ino = next(R.glob('*.ino')).read_text()
        vl = (R / 'vehicle_logic.h').read_text()
        api = (R / 'web_api.h').read_text()
        dash = (R / 'dashboard_source.html').read_text()
        fixed = (R / 'r79_fixed_policy_pure.h').read_text()

        assert 'R79_FIXED_FAST_WAIT_MS_PURE = 2u' in fixed
        a = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
        fast = vl[a:vl.index('static void r79FixedObserveStock(', a)]
        assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
        assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast
        assert 'twai_transmit(&out, 0)' not in fast
        assert 'r79FixedApplyBitsPure(out.data, bit18Policy, r79Hw3Active())' in fast
        assert 'r79RetrySchedule(R79LAB_TX_IMMEDIATE' in fast
        for key in ('transport', 'bit18Mode', 'bit18ModeName', 'fastAttempts',
                    'fastTxOk', 'fastTxFail'):
            assert f'"{key}"' in api, key
        assert 'D9_MUX1_2MS_WAIT' in api and 'D9_MUX1_FAST_ECHO_0MS' in api
        settings = dash[dash.index('<section class="panel" id="panelR79"'):dash.index('<section class="panel" id="panelTlssc"')]
        assert 'FAST ECHO · 0 ms' in settings and '2 ms WAIT' in settings
        panel = dash[dash.index('<section class="panel" id="panelLabR79"'):dash.index('<section class="panel" id="panelNagModeH"')]
        assert 'R79 Status' in panel
        assert 'FAST ECHO · 0 ms' not in panel and 'V2.6 STYLE' not in panel
        print('v3.7 fixed post-mux1 enqueue contract: PASS')

    def test_r79_fixed_runtime(self):
        """Retained from test_v37_r79_fixed_runtime_static.py."""
        from pathlib import Path

        ROOT = Path(__file__).resolve().parents[1]
        ino = next(ROOT.glob("*.ino")).read_text()
        vl = (ROOT / "vehicle_logic.h").read_text()
        cr = (ROOT / "can_runtime.h").read_text()
        web = (ROOT / "web_api.h").read_text()

        assert '#include "r79_fixed_policy_pure.h"' in ino
        assert 'static volatile uint8_t r79Bit18Policy = R79_BIT18_DEFAULT_PURE;' in vl
        assert 'static volatile bool r79Mode1ReinjectEnabled = true;' in vl
        assert 'r79Mode1PostMux2Generation' in vl

        # The live CAN-B path has one mutually-exclusive Mode 1/Mode 2 dispatcher and
        # one transport tick. Mode 1 retains its payload and recovery policy while
        # exposing the historical initial enqueue-wait comparison.
        assert cr.count('r79ProcessStockFrame(f, timingMux, r79FrameNowMs)') == 1
        assert cr.count('r79TransportTick();') == 1
        assert 'r79TransportHandleMux1(f, frameRxDequeueUs)' not in cr

        fast_start = vl.index('static bool r79FixedFastEcho(const twai_message_t &src) {')
        fast_end = vl.index('static void r79FixedObserveStock(', fast_start)
        fast = vl[fast_start:fast_end]
        assert 'r79FixedApplyBitsPure(out.data, bit18Policy, r79Hw3Active())' in fast
        assert 'r79Mode1TxWaitMsPure(waitMode)' in fast
        assert 'r79ApGateTransmitGuarded(&out, waitMs, apGeneration)' in fast
        for retired in ('r79TransportStrategy', 'r79PostMux1TxMode',
                        'r79PeriodicSchedulerMode', 'r79PeriodicAnchorMux',
                        'r79PeriodicShotsPerCycle'):
            assert retired not in fast

        tick_start = vl.index('static void r79FixedTick() {')
        tick_end = vl.index('static bool setTlsscEnabled', tick_start)
        tick = vl[tick_start:tick_end]
        tick_flat = ' '.join(tick.split())
        assert 'r79FixedQuietStepPure' in tick
        assert 'r79LabRetryTick();' in tick
        assert 'canTxCancellationGenerationSnapshot( &r79Mode1PostMux2Generation)' in tick_flat
        assert 'r79LabTransmitShadow(' in tick
        for retired in ('r79TransportStrategy', 'r79PostMux1TxMode',
                        'r79PeriodicSchedulerMode', 'r79PeriodicAnchorMux',
                        'r79PeriodicShotsPerCycle'):
            assert retired not in tick

        # Production API exposes the transport selector plus the existing Mode-1
        # STOCK/FORCE 0 policy and is not LAB-enabled gated.
        assert 'static void httpR79Stats()' in web
        assert 'static void httpR79Update()' in web
        assert 'server.on("/api/r79/stats", HTTP_GET, httpR79Stats);' in web
        assert 'server.on("/api/r79/update", HTTP_POST, httpR79Update);' in web
        update_start = web.index('static void httpR79Update()')
        update_end = web.index('\nstatic void ', update_start + 1)
        update = web[update_start:update_end]
        assert 'bit18Mode' in update
        assert 'server.hasArg("mode")' in update
        assert 'server.hasArg("mode1TxWaitMode")' in update
        assert 'server.hasArg("mode1Reinject")' in update
        assert 'server.hasArg("mode1DelayMs")' in update
        assert 'server.hasArg("mode2Reinject")' in update
        assert 'server.hasArg("mode2DelayMs")' in update
        assert '400' in update
        assert 'labMenuEnabled' not in update
        assert 'canTxCancellationGenerationAdvance(&r79Mode1PostMux2Generation)' in update
        assert 'r79Mode1DisableCancelsRetryPure' in update
        assert 'if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(true);' in update
        assert 'if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(false);' in update

        print('v3.7 fixed R79 runtime static contract: PASS')

    def test_r79_retired_absent(self):
        """Retained from test_v37_r79_retired_absent.py."""
        from pathlib import Path

        root = Path(__file__).resolve().parents[1]
        product = "\n".join(
            p.read_text(encoding="utf-8")
            for p in root.iterdir()
            if p.suffix in {".h", ".cpp", ".ino", ".html"} and p.name != "index_html.h"
        )

        for token in (
            "R79_TRANSPORT_ROAMING_MIRROR",
            "R79_TRANSPORT_V26_LEGACY",
            "R79_TRANSPORT_MUX0_COLLISION",
            "R79LAB_TX_PRE_MUX1",
            "R79LAB_TX_POST_MUX2",
            "R79LAB_TX_LEGACY_MIRROR",
            "R79LAB_TX_V26_PERIODIC",
            "R79LAB_TX_MUX0_COLLISION",
            "r79PostMux2",
            "r79PreMux1",
            "R79_REFRESH_SUMMON_ONLY",
            "R79_REFRESH_OFF",
            "R79_SCHED_GUARDED_487_PHASE_WALK",
            "r79PhasedScheduler",
            "r79RoamingBurst",
            "r79TimingCapture",
            "/api/r79timing/",
        ):
            assert token not in product, token

        for removed in (
            "r79_phase_walk_pure.h",
            "r79_phased_scheduler_pure.h",
            "r79_periodic_scheduler_pure.h",
            "r79_quiet_window_pure.h",
            "r79_pre_mux1_pure.h",
            "r79_post_mux2_pure.h",
            "r79_roaming_burst_pure.h",
            "r79_mux0_collision_pure.h",
            "r79_timing_capture.h",
            "r79_timing_capture.cpp",
        ):
            assert not (root / removed).exists(), removed

        print("v3.7 retired R79 experiments absent: PASS")


if __name__ == "__main__":
    unittest.main()
