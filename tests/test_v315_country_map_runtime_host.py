"""Execute production RX handlers and guarded transport with fake hardware.

The mutex/critical-section hooks inject an update after the handler reads mode.
Only hardware I/O and RTOS primitives are substituted; transforms, handlers,
generation checks and recovery-epoch validation come from production sources.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
logic = (ROOT / "vehicle_logic.h").read_text()
core = (ROOT / "can_core.h").read_text()
state = (ROOT / "t2can_core_state.h").read_text()


def function(source, name):
    # Locate the signature itself, not a preceding helper or declaration.
    name_pos = source.index(name + "(")
    start = source.rfind("static ", 0, name_pos)
    brace = source.index("{", name_pos)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "country_override_pure.h"
#include "runtime_gate_pure.h"
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
void (*exitHook)() = nullptr;
void portENTER_CRITICAL(int*) {}
void portEXIT_CRITICAL(int*) {
  if (exitHook) { auto hook = exitHook; exitHook = nullptr; hook(); }
}
constexpr int pdTRUE = 1;
constexpr int portMAX_DELAY = 1000;
int canTxBarrierMutex = 1;
bool mutexBusy = false;
void (*takeHook)() = nullptr;
int xSemaphoreTake(int, int) {
  if (takeHook) { auto hook=takeHook; takeHook=nullptr; hook(); }
  return mutexBusy ? 0 : 1;
}
void xSemaphoreGive(int) {}
using esp_err_t = int;
constexpr int ESP_OK=0, ESP_ERR_INVALID_ARG=1, ESP_ERR_INVALID_STATE=2, ESP_ERR_TIMEOUT=3;
constexpr uint8_t CAN_TX_TRACE_SOURCE_DEFAULT = 0;
struct can_frame { uint32_t can_id; uint8_t can_dlc; uint8_t data[8]; };
struct twai_message_t {
  uint32_t identifier; bool extd; bool rtr; uint8_t data_length_code; uint8_t data[8];
};
int sendsA=0, sendsB=0;
can_frame sentA{};
twai_message_t sentB{};
struct MCP2515 {
  enum ERROR { ERROR_OK, ERROR_FAIL };
  ERROR sendMessage(const can_frame* f) { ++sendsA; sentA=*f; return ERROR_OK; }
} Can_A;
esp_err_t twai_transmit(const twai_message_t* f, int) { ++sendsB; sentB=*f; return ESP_OK; }
esp_err_t can3fdTimingTransmit(const twai_message_t* f, int wait, uint8_t) { return twai_transmit(f, wait); }
void canATraceRecordTx(const can_frame*, McpTxResultReason, MCP2515::ERROR, uint8_t) {}
void canBTraceRecordTx(const twai_message_t*, esp_err_t, uint8_t) {}
void researchCaptureObserveTxVh(uint16_t, uint8_t, const uint8_t*, bool) {}
bool canTxAdministrativeHold=false, mcpReady=true, twaiReady=true;
bool labMenuEnabled=true, r79Active=true, admission=true, summonHeartbeatOverrideEnabled=false;
int lab3f8Mux=0;
volatile uint32_t lab3f8Generation=1;
uint8_t activeVehicleProfile=1, activeVehicleTopology=1;
uint32_t mcpTxOk=0, mcpTxFail=0;
uint8_t mcpTxFailConsecutive=0;
uint32_t millis() { return 100; }
bool twaiNonSummonAdmissionOpen() { return admission; }
void mux1CancelPendingLocked() {}
constexpr int R79_TX_STATE_ACTIVE=1;
struct R79RuntimeStatus { int state; };
R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t) { return {r79Active ? 1 : 0}; }
'''
code += state[state.index("static constexpr uint8_t CAN_TX_FRESH_PARTY"):
              state.index("// BEGIN HOST-TESTABLE LOGIC")]
for name in ("canTxBarrierAllowsMaskedPure", "canTxBarrierInvalidatePreservePure"):
    code += function(state, name)
code += "\nCanTxBarrierState canTxBarrierState{1, 3};\n"
for name in ("canTxCancellationGenerationSnapshot", "canTxTwaiTransmitWithMaskTaggedGuarded",
             "canTxMcpSendTaggedGuarded"):
    code += function(core, name)
code += logic[logic.index("static portMUX_TYPE countryOverrideMux"):
              logic.index("static bool countryOverrideSetModeWithBarrier")]
for name in ("countryOverrideSetModeWithBarrier", "countryOverrideSetLabEnabledWithBarrier",
             "countryOverrideModeSnapshot", "countryOverrideMapModeSnapshot", "countryOverrideGateOpen", "countryOverrideRecordResult",
             "countryOverrideObserve238CanA", "countryOverrideObserve238CanB",
             "countryOverrideObserve7ffCanA", "countryOverrideObserve7ffCanB"):
    code += function(logic, name)
code += r'''

int main(){
 (void)countryOverrideR79AuthorizationOpen;
 summonHeartbeatOverrideEnabled=true;
 assert(countryOverrideSetLabEnabledWithBarrier(false));
 assert(lab3f8Generation == 2u);
 assert(!summonHeartbeatOverrideEnabled);
 canTxBarrierInvalidatePreservePure(canTxBarrierState,0);canTxBarrierState={1,3};
 for(uint8_t country=0;country<=3;++country)for(uint8_t map=0;map<=2;++map){
  assert(countryOverrideSetModeWithBarrier(country,map));
  for(uint8_t page:{1,3}){
   sendsA=sendsB=0;
   can_frame a{0x7FF,8,{page,0xA6,'E','D',3,4,5,6}};
   twai_message_t b{0x7FF,false,false,8,{page,0xA6,'E','D',3,4,5,6}};
   countryOverrideObserve7ffCanA(a,1);countryOverrideObserve7ffCanB(b,1);
   bool enabled=page==1?country!=0:map!=0;
   assert(sendsA==enabled&&sendsB==enabled);
   if(enabled){
    const uint8_t low=country==1?'S':country==2?'R':'Z';
    const uint8_t high=country==1?'U':country==2?'K':'N';
    if(page==1)assert(sentA.data[2]==low&&sentA.data[3]==high);
    else assert((sentA.data[1]&15)==(map==1?0:7));
    assert(memcmp(sentA.data,sentB.data,8)==0);
    for(int i=0;i<8;++i)if(page==1?(i!=2&&i!=3):i!=1)assert(sentA.data[i]==a.data[i]);
    if(page==3)assert((sentA.data[1]&0xF0)==(a.data[1]&0xF0));
   }
  }
  sendsA=sendsB=0;
  can_frame a{0x238,8,{0,0xA6,0x14,0xAD,3,4,0xF5,0}};
  a.data[7]=0xFF& (0x38+2+0+0xA6+0x14+0xAD+3+4+0xF5);
  twai_message_t b{0x238,false,false,8,{}};memcpy(b.data,a.data,8);
  countryOverrideObserve238CanA(a,1);countryOverrideObserve238CanB(b,1);
  assert(sendsB==(country!=0));
  assert(sendsA==0); // Y L CAN A is Party; 238 is received on CAN B.
 }
 // Country STOCK still permits Map US; closed R79 authorization still blocks it.
 assert(countryOverrideSetModeWithBarrier(0,1));r79Active=false;sendsA=0;
 can_frame a{0x7FF,8,{3,0xA7,1,2,3,4,5,6}};
 countryOverrideObserve7ffCanA(a,1);assert(sendsA==0);
}

'''

with tempfile.TemporaryDirectory() as td:
 source=Path(td)/'country.cpp';exe=Path(td)/'country';source.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Independent Country / Map production RX PASS')
