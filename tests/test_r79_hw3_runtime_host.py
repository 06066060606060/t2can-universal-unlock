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
twai_message_t sent{};
esp_err_t sendResult=ESP_OK;
esp_err_t twai_transmit(const twai_message_t* f, int wait) {
 if(sendHook){auto hook=sendHook;sendHook=nullptr;hook();}
 if(r79ApGateConfig.enabled && r79ApGateConfig.mode==0 && dasAutopilotState4==3)
   ++policyViolations;
 ++sends; sent=*f; lastWait=wait; return sendResult;
}
esp_err_t twai_clear_transmit_queue() { ++flushes; return ESP_OK; }
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
constexpr int R79LAB_TX_NONE=0,R79LAB_TX_IMMEDIATE=1,R79LAB_TX_RETRY=2,R79LAB_TX_PERIODIC=3;
volatile uint32_t r79Mode1PostMux2Generation=1;
bool r79LabStockValid=true,r79RetryPending=false;
uint8_t r79LabLastStockRaw[8]{},r79RetryIndex=0,r79RetryOriginKind=0;
uint32_t r79RetryDueMs=0,r79RetryGeneration=0,r79RetryScheduled=0,r79RetryTxOk=0,r79RetryTxFail=0,r79RetryExhausted=0;
uint32_t r79LastRetryMs=0,r79QuietGuardSkip=0,r79QuietFireCount=0;
R79FixedQuietStatePure r79FixedQuietState{};
R79Mode2DelayedStatePure r79Mode2DelayedState{};
constexpr int R79_TX_STATE_ACTIVE=1;
struct R79RuntimeStatus { int state; };
R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t) { return {R79_TX_STATE_ACTIVE}; }

void r79LabRecordTxResult(bool,const twai_message_t&,int) {}
uint8_t priority=SUMMON_PRIORITY_ACTIVE;
uint8_t summonPriorityStateSnapshot(uint32_t) { return priority; }
uint32_t canTxCancellationGenerationSnapshot(const volatile uint32_t* g) { return *g; }
bool r79Hw3Enabled=false, hw3Supported=true;
bool activeProfileR79Hw3Supported() { return hw3Supported; }
static bool r79FastReactiveGateOpen();
'''
for name in ('r79Hw3Active', 'r79ApDasFreshLocked', 'r79ApGateDecisionLocked', 'r79ApGateObserveLocked',
             'r79ApGateGenerationSnapshot', 'r79FastReactiveGateOpen'):
    code += function(name)
# The old conditional helper can disappear once every call site is serialized.
if re.search(r'^static bool r79ApGateNeedsGuard\(', logic, re.MULTILINE):
    code += function('r79ApGateNeedsGuard')
for name in ('r79ApGateTransmitGuarded', 'r79LabDirectTwaiTransmit',
             'r79LabDirectTwaiTransmitGuarded', 'r79FastReactiveRecordTxState',
             'r79ImmediateClearTransmitQueueGuarded',
             'r79Mode1PostMux2GenerationCurrent', 'r79Mode1PostMux2ClearTransmitQueueGuarded',
             'r79RetryCancel', 'r79RetrySchedule', 'r79CopyLatestStock',
             'r79LabTransmitOnce', 'r79LabTransmitShadow', 'r79LabRetryTick',
             'r79FixedTick', 'r79Mode2Tick',
             'r79FixedFastEcho', 'r79Mode2FastEcho'):
    code += function(name)
code += r'''


void reset(bool enabled,bool supported) {
 r79Hw3Enabled=enabled;hw3Supported=supported;sends=0;flushes=0;sendResult=ESP_OK;
 fakeNow=100;lastDASStatusMillis=100;r79ApGateConfig={false,0,2};r79ApGateSession={};
 r79ApGateGeneration=1;r79Mode1PostMux2Generation=1;r79FixedQuietState={};r79Mode2DelayedState={};
 r79ApGateObserveLocked(fakeNow);r79RetryCancel();priority=SUMMON_PRIORITY_ACTIVE;r79LabStockValid=true;
 payloadHook=nullptr;takeHook=nullptr;sendHook=nullptr;held=false;busy=false;
}
void assertPayload(uint8_t stock5,bool enabled,bool supported) {
 assert(sends>0);
 const uint8_t expected=enabled&&supported?stock5:(uint8_t)(stock5|0x80);
 assert(sent.data[5]==expected && "HW3 ON must preserve received stock bit47");
 assert(sent.data[2]==0xF7);
}
// Inject a generation fault during final composition; the final guard must reject it.
void invalidatePayload() { r79Hw3Enabled=!r79Hw3Enabled;++r79ApGateGeneration;++r79Mode1PostMux2Generation; }
int main() {
 for(bool enabled : {false,true}) for(bool supported : {false,true}) for(int bit47 : {0,1}) {
  twai_message_t f{};f.identifier=0x3FD;f.data_length_code=8;f.data[0]=1;f.data[2]=0xFF;f.data[5]=bit47?0xA5:0x25;
  for(int mode : {1,2}) {
   reset(enabled,supported);
   assert(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));assert(sends==1);
   assertPayload(f.data[5],enabled,supported);
   reset(enabled,supported);memcpy(r79LabLastStockRaw,f.data,8);
   if(mode==1){r79FixedQuietObserveStockPure(r79FixedQuietState,2,100,true,150);fakeNow=250;r79FixedTick();}
   else{r79Mode2ObserveStockPure(r79Mode2DelayedState,2,100,true,150);fakeNow=250;r79Mode2Tick();}
   assert(sends==1);assertPayload(f.data[5],enabled,supported);
  }
  // Mode1 timeout: immediate clone is reused for its emergency flush retry.
  reset(enabled,supported);sendResult=ESP_ERR_TIMEOUT;
  assert(r79FixedFastEcho(f));assert(sends==2&&flushes==1&&r79RetryPending);
  assertPayload(f.data[5],enabled,supported);
  // Scheduled immediate retry rebuilds from the NEWEST received raw stock.
  memcpy(r79LabLastStockRaw,f.data,8);r79LabLastStockRaw[5]^=0x80;
  fakeNow=105;sendResult=ESP_OK;r79LabRetryTick();assert(sends==3&&!r79RetryPending);
  assertPayload(r79LabLastStockRaw[5],enabled,supported);
  // Mode1 periodic flush and scheduled retries share production TransmitOnce.
  reset(enabled,supported);memcpy(r79LabLastStockRaw,f.data,8);sendResult=ESP_ERR_TIMEOUT;
  r79FixedQuietObserveStockPure(r79FixedQuietState,2,100,true,150);fakeNow=250;r79FixedTick();
  assert(sends==2&&flushes==1&&r79RetryPending&&r79RetryOriginKind==R79LAB_TX_PERIODIC);
  assertPayload(f.data[5],enabled,supported);
  fakeNow=255;sendResult=ESP_OK;r79LabRetryTick();assert(sends==3&&!r79RetryPending);
  assertPayload(f.data[5],enabled,supported);
 }
 // A committed payload-policy generation change discards prepared immediate and delayed frames.
 for(int mode : {1,2}) {
  twai_message_t f{};f.identifier=0x3FD;f.data_length_code=8;f.data[0]=1;
  reset(false,true);payloadHook=invalidatePayload;
  (void)(mode==1?r79FixedFastEcho(f):r79Mode2FastEcho(f));assert(sends==0);
  reset(false,true);memcpy(r79LabLastStockRaw,f.data,8);payloadHook=invalidatePayload;
  if(mode==1){r79FixedQuietObserveStockPure(r79FixedQuietState,2,100,true,150);fakeNow=250;r79FixedTick();}
  else{r79Mode2ObserveStockPure(r79Mode2DelayedState,2,100,true,150);fakeNow=250;r79Mode2Tick();}
  assert(sends==0);
 }
}

'''

with tempfile.TemporaryDirectory(prefix='r79-hw3-runtime-') as temp:
    source=Path(temp)/'runtime.cpp';binary=Path(temp)/'runtime'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('R79 HW3 production runtime: both bit47 values, OFF/ON, capability, both modes, all retries and payload races PASS')
