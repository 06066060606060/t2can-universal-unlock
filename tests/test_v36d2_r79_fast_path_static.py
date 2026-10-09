from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob('*.ino')).read_text()
vl = (ROOT / 'vehicle_logic.h').read_text()
cr = (ROOT / 'can_runtime.h').read_text()
web = (ROOT / 'web_api.h').read_text()
dash = (ROOT / 'dashboard_source.html').read_text()

assert '#define FW_VERSION "v3.28.0"' in ino
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
cfg = vl[vl.index('static void r79CfgLoad'):vl.index('static void r79CfgSave')]
assert 'prefs.getUChar("bit18", R79_BIT18_DEFAULT_PURE)' in cfg
assert 'r79Bit18PolicySanitizePure(stored)' in cfg

for key in ('fixedPolicy', 'transport', 'periodic', 'bit18Mode', 'bit18ModeName',
            'fastAttempts', 'fastTxOk', 'fastTxFail'):
    assert f'"{key}"' in web, key
for dom in ('r79FixedPolicyValue', 'r79FixedTransportValue', 'r79FixedFastValue'):
    assert f'id="{dom}"' in dash, dom

print('v3.7 R79 fixed fast path static contract: PASS')
