"""Execute production AP transport guards and Mode 1/2 echoes with RTOS hooks.

Only hardware, logging and scheduling peripherals are substituted. Authorization,
DAS freshness, session observation, payload transforms and transport call sites
come from production. A hook models another task committing a configuration at
mutex admission or immediately before the hardware enqueue boundary.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
logic = (ROOT / 'vehicle_logic.h').read_text()


def function(name):
    definitions = list(re.finditer(
        r'^static\s+[^;{}]*?\b' + re.escape(name) + r'\([^;{}]*?\)\s*\{',
        logic, re.MULTILINE))
    assert definitions, f'Production definition missing: {name}'
    start = definitions[-1].start()
    brace = logic.index('{', definitions[-1].start())
    end, depth = brace + 1, 1
    while depth:
        depth += (logic[end] == '{') - (logic[end] == '}')
        end += 1
    return logic[start:end] + '\n'


# These producer functions must never reintroduce an unlocked hardware enqueue.
# Mode1 retry uses the same echo function; periodic LAB has its own combined
# generation guard. Dedicated DMS-only transport is intentionally outside scope.
for producer in ('r79FixedFastEcho', 'r79Mode2FastEcho', 'r79Mode2Tick',
                 'r79LabDirectTwaiTransmit'):
    body = function(producer)
    assert not re.search(r'\btwai_transmit\s*\(', body), producer + ' bypasses AP barrier'
    assert 'r79ApGateTransmitGuarded(' in body, producer + ' lacks guarded enqueue'


code = r'''
#include <cassert>
#include <cstdint>
#include "tests/vision_control_off_fixture.h"
#include <cstring>
#include <initializer_list>
#include "summon_state_pure.h"
#include "runtime_gate_pure.h"
#include "r79_ap_gate_pure.h"
#include "r79_fixed_policy_pure.h"
#include "r79_mode2_pure.h"
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_ERR_INVALID_ARG=1, ESP_ERR_INVALID_STATE=2, ESP_ERR_TIMEOUT=3;
constexpr int pdTRUE=1;
#define pdMS_TO_TICKS(x) (x)
struct twai_message_t { uint32_t identifier; uint8_t data_length_code; uint32_t flags; uint8_t data[8]; };
int stateMux=0, r79LabMux=0, canTxBarrierMutex=1;
bool held=false, busy=false;
void (*takeHook)()=nullptr;
void (*sendHook)()=nullptr;
void (*payloadHook)()=nullptr;
void portENTER_CRITICAL(int*) {}
void portEXIT_CRITICAL(int*) {}
int xSemaphoreTake(int, int) {
 if(takeHook){auto hook=takeHook;takeHook=nullptr;hook();}
 if(busy || held) return 0;
 held=true; return pdTRUE;
}
void xSemaphoreGive(int) { assert(held); held=false; }
bool canTxAdministrativeHold=false, twaiReady=true;
bool dasAutopilotStateValid=true, gateSummoning=false;
uint8_t dasAutopilotState4=3;
R79ManualSuppressionPure r79ManualSuppression{};
R79ApGateConfigPure r79ApGateConfig{false,0,2};
R79ApGateSessionPure r79ApGateSession{};
volatile uint32_t r79ApGateGeneration=1;
volatile bool r79ApControlEnabledFast=false;
constexpr uint32_t R79_AP_DAS_FRESH_MS=3000;
uint32_t lastDASStatusMillis=100, fakeNow=100;
uint32_t millis() { return fakeNow; }
int sends=0, flushes=0, lastWait=-1, policyViolations=0;
int unguardedFlushes=0;bool timeoutOnce=false;
twai_message_t sent{};
esp_err_t sendResult=ESP_OK;
esp_err_t twai_transmit(const twai_message_t* f, int wait) {
 if(sendHook){auto hook=sendHook;sendHook=nullptr;hook();}
 if(r79ApGateConfig.enabled && r79ApGateConfig.mode==0 && dasAutopilotState4==3)
   ++policyViolations;
 ++sends; sent=*f; lastWait=wait; return timeoutOnce&&sends==1?ESP_ERR_TIMEOUT:sendResult;
}
esp_err_t twai_clear_transmit_queue() { ++flushes;if(!held)++unguardedFlushes;return ESP_OK; }
esp_err_t can3fdTimingTransmit(const twai_message_t* f,int wait,uint8_t) { return twai_transmit(f,wait); }
void canBTraceRecordTx(const twai_message_t*, esp_err_t) {}
void r79DmsApplyFinal(uint8_t*) {
 if(payloadHook){auto hook=payloadHook;payloadHook=nullptr;hook();}
}
uint8_t readMuxID(const uint8_t* data) { return data[0]&7u; }
uint8_t r79Bit18Policy=0, r79Mode1TxWaitMode=1;
uint32_t r79FastEchoAttempts=0,r79FastEchoTxOk=0,r79FastEchoTxFail=0;
uint32_t r79EmergencyQueueFlushCount=0,r79FlushTriggeredRetryOk=0,r79FlushTriggeredRetryFail=0;
uint32_t r79LabLastTxMs=0;
constexpr int R79LAB_TX_IMMEDIATE=1, R79LAB_TX_RETRY=2;
void r79LabRecordTxResult(bool,const twai_message_t&,int) {}
void r79RetryCancel() {}
void r79RetrySchedule(uint8_t,uint32_t,uint32_t=0u) {}
uint8_t priorityState=SUMMON_PRIORITY_NORMAL;
uint8_t summonPriorityStateSnapshot(uint32_t) { return priorityState; }
uint32_t canTxCancellationGenerationSnapshot(const volatile uint32_t* g) { return *g; }
bool r79Hw3Active() { return false; }
static bool r79FastReactiveGateOpen();
'''
for name in ('r79ApDasFreshLocked', 'r79ApGateDecisionLocked', 'r79ApGateObserveLocked',
             'r79ApGateGenerationSnapshot', 'r79FastReactiveGateOpen'):
    code += function(name)
# The old conditional helper can disappear once every call site is serialized.
if re.search(r'^static bool r79ApGateNeedsGuard\(', logic, re.MULTILINE):
    code += function('r79ApGateNeedsGuard')
for name in ('r79ApGateTransmitGuarded', 'r79LabDirectTwaiTransmit',
             'r79LabDirectTwaiTransmitGuarded', 'r79FastReactiveRecordTxState',
             'r79ImmediateClearTransmitQueueGuarded', 'r79FixedFastEcho', 'r79Mode2FastEcho'):
    code += function(name)
code += r'''
void reset(bool enabled=false,uint8_t mode=0) {
 canTxAdministrativeHold=false;twaiReady=true;held=false;busy=false;canTxBarrierMutex=1;
 takeHook=nullptr;sendHook=nullptr;payloadHook=nullptr;
 r79ApGateConfig={enabled,mode,2};r79ApControlEnabledFast=enabled;
 r79ApGateSession={};r79ApGateGeneration=1;
 dasAutopilotStateValid=true;dasAutopilotState4=3;gateSummoning=false;r79ManualSuppression={};
 fakeNow=100;lastDASStatusMillis=100;sends=0;flushes=0;lastWait=-1;policyViolations=0;
 sendResult=ESP_OK;timeoutOnce=false;priorityState=SUMMON_PRIORITY_NORMAL;unguardedFlushes=0;r79ApGateObserveLocked(fakeNow);
}
void engageConfig() {
 assert(!held); // A real configuration writer takes the same transport barrier.
 r79ApGateConfig={true,0,2};r79ApControlEnabledFast=true;r79ApGateSession={};
 ++r79ApGateGeneration;
 r79ApGateObserveLocked(fakeNow);
}
void competingConfigBeforeEnqueue() {
 // With the barrier held, the competing writer must wait until after enqueue.
 // Without it, a commit now wins the race and this enqueue must be rejected.
 if(!held) engageConfig();
}
void generationFaultAtComposition() {
 // Fault injection within the final composer, not a real barrier-protected writer.
 r79ApGateConfig={true,0,2};++r79ApGateGeneration;r79ApGateObserveLocked(fakeNow);
}
void disableManualAtAdmission() {
 assert(!held);r79ApGateConfig.allowManualDriving=false;++r79ApGateGeneration;
}
void scheduleManualOffAfterTimeout() {
 assert(held);takeHook=disableManualAtAdmission;
}
void invalidateObservation() { assert(!held);dasAutopilotStateValid=false;r79ApGateObserveLocked(fakeNow); }
void observe(uint8_t ap,uint32_t now) {
 fakeNow=now;
 r79ApGateObserveLocked(now); // Incoming frame must invalidate any preceding gap.
 dasAutopilotState4=ap;dasAutopilotStateValid=true;lastDASStatusMillis=now;
 r79ApGateObserveLocked(now);
}
int main() {
 twai_message_t f{};f.identifier=0x3FD;f.data_length_code=8;f.data[0]=1;f.data[2]=0xFF;
 // Literal waits and payload bits protect the historical OFF behavior.
 for(int mode=1;mode<=2;++mode) {
  // Default remains suppressed in both manual directions; opt-in opens only
  // the manual clause while the current AP block/delay policy remains enabled.
  for(uint8_t gear:{TESLA_GEAR_D,TESLA_GEAR_R}){
   reset(true,0);dasAutopilotState4=2;r79ManualSuppression={true,gear};
   assert(!(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f)));assert(sends==0);
   r79ApGateConfig.allowManualDriving=true;
   assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));assert(sends==1&&sent.data[2]==0xF7);
   reset();dasAutopilotState4=2;r79ManualSuppression={true,gear};r79ApGateConfig.allowManualDriving=true;
   takeHook=disableManualAtAdmission;
   assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));assert(sends==0&&flushes==0);
  }
  reset();dasAutopilotStateValid=false;
  assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));
  assert(sends==1 && lastWait==(mode==1?2:0));
  assert(sent.data[2]==0xF7 && sent.data[5]==0x80 && flushes==0);
  reset();sendHook=competingConfigBeforeEnqueue;
  assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));
  assert(policyViolations==0 && "OFF to ON may not commit between guard and enqueue");
  reset();payloadHook=generationFaultAtComposition;
  (void)(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));
  assert(sends==0 && flushes==0);
  reset(true,0);gateSummoning=true;
  assert(!(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f)));
  assert(sends==0 && flushes==0);
  reset(true,1);observe(4,1000);observe(5,1500);observe(6,2099);
  assert(!(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f)));
  observe(3,2100);
  assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));assert(sends==1);
  observe(2,2200);observe(3,2300);
  assert(!(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f)));
  assert(sends==1);
 }
 // Configuration commit during semaphore acquisition rejects old generations,
 // even when final configuration would itself otherwise permit the frame.
 reset();auto generation=r79ApGateGenerationSnapshot();takeHook=engageConfig;
 assert(r79LabDirectTwaiTransmit(&f,2,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0 && flushes==0);
 reset();generation=r79ApGateGenerationSnapshot();sendHook=competingConfigBeforeEnqueue;
 assert(r79LabDirectTwaiTransmit(&f,2,generation)==ESP_OK);
 assert(policyViolations==0);
 reset(true,1);observe(3,2100);generation=r79ApGateGenerationSnapshot();takeHook=invalidateObservation;
 assert(r79ApGateTransmitGuarded(&f,2,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0 && flushes==0);
 reset(true,1);observe(3,2100);generation=r79ApGateGenerationSnapshot();
 // Advance beyond the current 2100ms evidence freshness without an observer tick.
 fakeNow=5101;
 assert(r79ApGateTransmitGuarded(&f,0,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0);
 observe(3,5200);assert(!r79FastReactiveGateOpen());
 observe(3,7199);assert(!r79FastReactiveGateOpen());
 observe(3,7200);assert(r79FastReactiveGateOpen());
 // A stop/start generation cannot replay after the new session finishes waiting.
 generation=r79ApGateGenerationSnapshot();observe(2,7300);observe(3,7400);observe(3,9400);
 assert(r79ApGateTransmitGuarded(&f,0,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0);
 // Reserved/unknown DAS encodings cannot masquerade as known AP-inactive.
 for(uint8_t raw : {7,10,11,12,13,15}) {
  reset(true,1);dasAutopilotState4=raw;
  assert(!r79FastReactiveGateOpen());
 }
 // The existing periodic generation barrier composes with AP generation.
 reset();volatile uint32_t periodic=4;generation=r79ApGateGenerationSnapshot();
 assert(r79LabDirectTwaiTransmitGuarded(&f,0,&periodic,3,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0);
 assert(r79LabDirectTwaiTransmitGuarded(&f,0,&periodic,4,generation)==ESP_OK);
 assert(sends==1);
 reset(true,1);observe(3,2100);generation=r79ApGateGenerationSnapshot();busy=true;
 assert(r79ApGateTransmitGuarded(&f,0,generation)==ESP_ERR_INVALID_STATE);
 assert(sends==0 && flushes==0);
 busy=false;canTxAdministrativeHold=true;
 assert(r79ApGateTransmitGuarded(&f,0,generation)==ESP_ERR_INVALID_STATE);
 assert(!held && sends==0);
 canTxAdministrativeHold=false;twaiReady=false;
 assert(r79ApGateTransmitGuarded(&f,0,generation)==ESP_ERR_INVALID_STATE);
 assert(!held && sends==0);
 assert(r79ApGateTransmitGuarded(nullptr,0,generation)==ESP_ERR_INVALID_ARG);
 // Periodic/retry guard uses the same manual permission and cancellation.
 reset();dasAutopilotState4=2;r79ManualSuppression={true,TESLA_GEAR_R};r79ApGateConfig.allowManualDriving=true;
 generation=r79ApGateGenerationSnapshot();volatile uint32_t manualPeriodic=9;
 assert(r79LabDirectTwaiTransmitGuarded(&f,0,&manualPeriodic,9,generation)==ESP_OK);assert(sends==1);
 takeHook=disableManualAtAdmission;
 assert(r79LabDirectTwaiTransmitGuarded(&f,0,&manualPeriodic,9,generation)==ESP_ERR_INVALID_STATE);assert(sends==1);
 for(int reason=0;reason<3;++reason){reset();dasAutopilotState4=2;r79ManualSuppression={true,TESLA_GEAR_D};r79ApGateConfig.allowManualDriving=true;
  if(reason==0)canTxAdministrativeHold=true;if(reason==1)twaiReady=false;if(reason==2)busy=true;
  assert(r79ApGateTransmitGuarded(&f,0,r79ApGateGenerationSnapshot())==ESP_ERR_INVALID_STATE);assert(!sends&&!held);
 }
 reset();dasAutopilotState4=2;r79ManualSuppression={true,TESLA_GEAR_D};r79ApGateConfig.allowManualDriving=true;
 auto shortStock=f;shortStock.data_length_code=7;assert(!r79FixedFastEcho(shortStock));assert(!r79Mode2FastEcho(shortStock));assert(!sends);
 // Timeout recovery must not flush unrelated traffic after manual permission
 // was cancelled between initial enqueue and the next barrier admission.
 reset();dasAutopilotState4=2;r79ManualSuppression={true,TESLA_GEAR_D};r79ApGateConfig.allowManualDriving=true;
 priorityState=SUMMON_PRIORITY_READY;sendResult=ESP_ERR_TIMEOUT;sendHook=scheduleManualOffAfterTimeout;
 assert(r79FixedFastEcho(f));assert(flushes==0&&sends==1&&unguardedFlushes==0);
 // An authorized READY timeout still flushes once and retries successfully.
 reset();dasAutopilotState4=2;r79ManualSuppression={true,TESLA_GEAR_R};r79ApGateConfig.allowManualDriving=true;
 priorityState=SUMMON_PRIORITY_READY;timeoutOnce=true;
 assert(r79FixedFastEcho(f));assert(flushes==1&&sends==2&&unguardedFlushes==0&&!held);
}
'''
with tempfile.TemporaryDirectory(prefix='r79-ap-transport-') as temp:
    source = Path(temp) / 'transport.cpp'
    binary = Path(temp) / 'transport'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Production AP transport: Mode1/2, configuration races, fresh loss, session epochs PASS')
