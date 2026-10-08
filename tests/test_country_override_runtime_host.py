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
void reset() {
  (void)countryOverrideR79AuthorizationOpen;
  sendsA=sendsB=0; countryOverrideMapMode=1; countryOverrideMode=1; countryOverrideCancelGeneration=1;
  lab3f8Generation=1;
  canTxBarrierState={1,3}; labMenuEnabled=true; r79Active=true; admission=true;summonHeartbeatOverrideEnabled=false;
  canTxAdministrativeHold=false; mcpReady=twaiReady=true; mutexBusy=false;
  exitHook=nullptr; takeHook=nullptr;
}
void switchKorea() { assert(countryOverrideSetModeWithBarrier(2)); }
void recoverB() { canTxBarrierInvalidatePreservePure(canTxBarrierState, 1); }
void disableLab() {
  const uint32_t generation = countryOverrideCancelGeneration;
  summonHeartbeatOverrideEnabled=true;
  assert(countryOverrideSetLabEnabledWithBarrier(false));
  assert(lab3f8Generation == 2u);
  assert(!summonHeartbeatOverrideEnabled);
  assert(countryOverrideCancelGeneration == generation);
}
void canBOffline() { twaiReady=false; }

int main() {
  // Actual 0x238 country 250 (France), checksum 0x34; untouched high bits,
  // road flags and low counter nibble make payload preservation observable.
  const uint8_t mapData[8]={0x01,0x02,0xFA,0x80,0x04,0x05,0x74,0x34};
  const uint8_t carConfig[8]={1,0xAA,'R','F',4,5,6,7};
  for (uint8_t profile=1; profile<=5; ++profile) {
    for (uint8_t topology=1; topology<=3; ++topology) {
      bool valid=profile==1 ? topology==1 : topology!=1;
      for (int message=0; message<2; ++message) {
        for (int bus=0; bus<2; ++bus) {
          const uint32_t id=message ? 0x7FF : 0x238;
          const bool route=valid && (message || bus==1 || topology==2);
          auto onA=message ? countryOverrideObserve7ffCanA : countryOverrideObserve238CanA;
          auto onB=message ? countryOverrideObserve7ffCanB : countryOverrideObserve238CanB;
          can_frame a{id,8,{}};
          twai_message_t b{id,false,false,8,{}};
          memcpy(a.data,message ? carConfig : mapData,8);
          memcpy(b.data,a.data,8);
          activeVehicleProfile=profile; activeVehicleTopology=topology;
          for (int scenario=0; scenario<14; ++scenario) {
            reset();
            if(scenario==1) exitHook=switchKorea;
            if(scenario==2) exitHook=recoverB;
            if(scenario==3) exitHook=disableLab;
            if(scenario==4) {countryOverrideMode=0;countryOverrideMapMode=0;}
            if(scenario==5) r79Active=false;
            if(scenario==6) canTxAdministrativeHold=true;
            if(scenario==7) mutexBusy=true;
            if(scenario==8) canTxBarrierState.epoch=2; // received before recovery
            if(scenario==9) labMenuEnabled=false;
            if(scenario==10) {countryOverrideMode=2;countryOverrideMapMode=2;}
            if(scenario==11) takeHook=canBOffline; // B goes offline after R79 gate
            if(scenario==12) {countryOverrideMode=3;countryOverrideMapMode=0;}
            if(scenario==13) countryOverrideMode=4;
            if(bus==0) onA(a,1); else onB(b,1);
            const bool expected=route && (scenario==0 || scenario==3 || scenario==9 || scenario==10 || scenario==12);
            assert(sendsA==(expected && bus==0 ? 1:0));
            assert(sendsB==(expected && bus==1 ? 1:0));
            if(expected) {
              const uint8_t* payload=bus==0 ? sentA.data : sentB.data;
              if(message) {
                assert(payload[2]==(scenario==12 ? 'Z':scenario==10 ? 'R':'S'));
                assert(payload[3]==(scenario==12 ? 'N':scenario==10 ? 'K':'U'));
                assert(payload[1]==0xAA && payload[4]==4);
              } else {
                assert(payload[2]==(scenario==12 ? 0x2A:scenario==10 ? 0x9A:0x48));
                assert(payload[3]==(scenario==12 ? 0x82:scenario==10 ? 0x81:0x83));
                assert(payload[6]==0x84);
                assert(payload[7]==(scenario==12 ? 0x76:scenario==10 ? 0xE5:0x95));
              }
            }
          }
          // NZ has no confirmed map-region target: never emit a MUX3 override.
          if (message) {
            reset(); countryOverrideMode=3;countryOverrideMapMode=0;
            a.data[0]=3; b.data[0]=3;
            if(bus==0) onA(a,1); else onB(b,1);
            assert(sendsA+sendsB==0);
            a.data[0]=1; b.data[0]=1;
          }
          reset(); a.can_dlc=7; b.data_length_code=7;
          if(bus==0) onA(a,1); else onB(b,1);
          assert(sendsA+sendsB==0);
          reset(); a.can_dlc=8; b.data_length_code=8;
          a.can_id|=0x80000000; b.extd=true;
          if(bus==0) onA(a,1); else onB(b,1);
          assert(sendsA+sendsB==0);
          reset(); a.can_id=id|0x40000000; b.extd=false; b.rtr=true;
          if(bus==0) onA(a,1); else onB(b,1);
          assert(sendsA+sendsB==0);
        }
      }
    }
  }
}
'''
with tempfile.TemporaryDirectory() as td:
    source = Path(td) / "country_runtime.cpp"
    exe = Path(td) / "country_runtime"
    source.write_text(code)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT), str(source), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS production country handlers/guarded transport: 840 mode/race/route cases + 180 malformed cases")
