"""Country and Map host integration, with independent runtime/API fixtures."""
import unittest


class CountryOverrideHost(unittest.TestCase):

    def test_country_guarded_transport(self):
        """Retained from test_country_override_runtime_host.py."""
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
        core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
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
        code += "bool featureCfgPersistLab(bool){return true;}\nstatic bool countryOverrideSetLabEnabledWithBarrier(bool,bool persist=false);\n"
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

    def test_independent_country_map_runtime(self):
        """Retained from test_v315_country_map_runtime_host.py."""
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
        core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
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
        code += "bool featureCfgPersistLab(bool){return true;}\nstatic bool countryOverrideSetLabEnabledWithBarrier(bool,bool persist=false);\n"
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

    def test_country_map_api_persistence(self):
        """Retained from test_country_override_nz_api_host.py."""
        """Execute production independent-country/map HTTP, atomic NVS and migration functions."""
        from pathlib import Path
        import os, re, subprocess, tempfile
        ROOT = Path(__file__).resolve().parents[1]
        def extract(file, name):
            text = (ROOT / file).read_text()
            match = re.search(r'^static\s+[^;{}]*?\b' + name + r'\([^;{}]*?\)\s*\{', text, re.M)
            assert match, name
            end = text.index('{', match.start()) + 1
            depth = 1
            while depth:
                depth += (text[end] == '{') - (text[end] == '}')
                end += 1
            return text[match.start():end] + '\n'
        code = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <map>
#include "country_override_pure.h"
struct String : std::string {using std::string::string; int toInt() const {return std::stoi(*this);}};
struct Server {
 std::map<std::string,String> args; int status=0;
 bool hasArg(const char*k){return args.count(k);}
 String arg(const char*k){return args[k];}
 void send(int s,const char*,const String&){status=s;}
} server;
std::map<std::string,uint16_t> nvs;
bool nvsReadOpen=true,nvsWriteOpen=true,nvsWrite=true,lockOk=true,held=false;
int writes=0,countryOverrideMux=0,canTxBarrierMutex=1;
constexpr int portMAX_DELAY=1000,pdTRUE=1;
uint8_t activeVehicleProfile=2,activeVehicleTopology=3;
volatile uint8_t countryOverrideMode=0,countryOverrideMapMode=0;
volatile uint32_t countryOverrideCancelGeneration=1;
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
int xSemaphoreTake(int,int){if(!lockOk)return 0;assert(!held);held=true;return pdTRUE;}
void xSemaphoreGive(int){assert(held);held=false;}
struct Preferences {
 bool begin(const char*ns,bool read){assert(std::string(ns)=="countrylab");return read?nvsReadOpen:nvsWriteOpen;}
 bool isKey(const char*k){return nvs.count(k);}
 uint16_t getUShort(const char*k,uint16_t fallback){return nvs.count(k)?nvs[k]:fallback;}
 uint8_t getUChar(const char*k,uint8_t fallback){return nvs.count(k)?uint8_t(nvs[k]):fallback;}
 size_t putUShort(const char*k,uint16_t value){++writes;if(!nvsWrite)return 0;nvs[k]=value;return 2;}
 void end(){}
};
String countryOverrideStatsToJson(){return "{}";}
'''
        for name in ('countryOverrideModeSnapshot', 'countryOverrideMapModeSnapshot',
                     'countryOverrideSetModeWithBarrier', 'countryOverrideApplySelection',
                     'countryOverrideCfgLoad'):
            code += extract('vehicle_logic.h', name)
        code += extract('web_api.h', 'httpCountryOverrideUpdate')
        code += r'''
void expect(uint8_t country,uint8_t map){assert(countryOverrideMode==country);assert(countryOverrideMapMode==map);}
void reboot(uint8_t country,uint8_t map){countryOverrideMode=255;countryOverrideMapMode=255;countryOverrideCfgLoad();expect(country,map);}
void request(const std::map<std::string,String>&args,int status){server.args=args;httpCountryOverrideUpdate();assert(server.status==status);}
int main(){
 // Missing keys and each released legacy preset keep their exact former effects.
 nvs.clear();reboot(0,0);
 assert(countryOverrideSetModeWithBarrier(0,0));expect(0,0);
 for(const auto& fixture: {std::pair<uint8_t,uint8_t>{0,0},{1,1},{2,2},{3,0},{4,0},{255,0}}){
  nvs={{"mode",fixture.first}};reboot(fixture.first<=3?fixture.first:0,fixture.second);
  assert(!nvs.count("selection"));
 }
 // Packed selection is authoritative over the older coupled preset.
 for(uint8_t country=0;country<4;++country)for(uint8_t map=0;map<3;++map){
  nvs={{"mode",2},{"selection",uint16_t(country|(uint16_t(map)<<8))}};reboot(country,map);
 }
 for(uint16_t bad: {uint16_t(4),uint16_t(255),uint16_t(0x0300),uint16_t(0x0104),uint16_t(0xFFFF)}){
  nvs={{"mode",2},{"selection",bad}};reboot(0,0);
 }
 nvsReadOpen=false;reboot(0,0);nvsReadOpen=true;
 // Exercise all independent valid HTTP combinations and actual NVS reload.
 for(int country=0;country<4;++country)for(int map=0;map<3;++map){
  request({{"countryMode",String(std::to_string(country).c_str())},{"mapMode",String(std::to_string(map).c_str())}},200);
  assert(nvs["selection"]==uint16_t(country|(map<<8)));reboot(country,map);
 }
 request({{"countryMode","0"}},200);expect(0,2);reboot(0,2);
 request({{"mapMode","1"}},200);expect(0,1);reboot(0,1);
 request({{"countryMode","3"}},200);expect(3,1);reboot(3,1);
 request({{"mapMode","0"}},200);expect(3,0);reboot(3,0);
 for(const auto*value:{"0","1","2","3"}){request({{"mode",value}},200);reboot(uint8_t(std::stoi(value)),0);}
 // Strict parser rejection must not write, publish or alter reboot state.
 const auto baseline=nvs;const int writesBefore=writes;
 for(const char*bad:{"4","255","-1","3x","3.0","03",""," 3","3 ","4294967296"}){
  request({{"countryMode",bad}},400);request({{"mode",bad}},400);expect(3,0);assert(nvs==baseline);
 }
 for(const char*bad:{"3","255","-1","2x","2.0","02",""," 2","2 ","4294967296"}){
  request({{"mapMode",bad}},400);expect(3,0);assert(nvs==baseline);
 }
 request({},400);request({{"countryMode","1"},{"mode","1"}},400);
 assert(writes==writesBefore);reboot(3,0);
 // NVS open/write failure retains both live selectors and old reboot selection.
 nvsWriteOpen=false;request({{"countryMode","1"},{"mapMode","2"}},503);expect(3,0);assert(nvs==baseline);reboot(3,0);
 nvsWriteOpen=true;nvsWrite=false;request({{"countryMode","1"},{"mapMode","2"}},503);expect(3,0);assert(nvs==baseline);reboot(3,0);nvsWrite=true;
 // Failed write before first migration also leaves the legacy reboot contract intact.
 nvs={{"mode",2}};reboot(2,2);nvsWrite=false;request({{"mapMode","0"}},503);expect(2,2);assert(!nvs.count("selection"));reboot(2,2);nvsWrite=true;
 const auto legacy=nvs;const int count=writes;
 // Real barrier-acquisition failure must refuse persistence as well as publication.
 const uint32_t beforeGeneration=countryOverrideCancelGeneration;lockOk=false;
 request({{"countryMode","1"},{"mapMode","0"}},503);expect(2,2);
 assert(nvs==legacy&&writes==count&&countryOverrideCancelGeneration==beforeGeneration&&!held);reboot(2,2);lockOk=true;
 canTxBarrierMutex=0;
 request({{"mapMode","0"}},503);expect(2,2);assert(nvs==legacy&&writes==count);reboot(2,2);canTxBarrierMutex=1;
 activeVehicleTopology=0;request({{"countryMode","1"}},409);assert(nvs==legacy&&writes==count);
 activeVehicleTopology=3;request({{"countryMode","1"},{"mapMode","0"}},200);expect(1,0);reboot(1,0);
 // Direct transaction boundary rejects invalid enums before persistent changes.
 const auto final=nvs;const int finalWrites=writes;
 assert(!countryOverrideApplySelection(4,0));assert(!countryOverrideApplySelection(0,3));
 assert(nvs==final&&writes==finalWrites);expect(1,0);
}
'''
        with tempfile.TemporaryDirectory(prefix='country-map-api-host-') as directory:
            source=Path(directory)/'test.cpp'; binary=Path(directory)/'test'
            source.write_text(code)
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
        print('Country/Map production HTTP/NVS: 12 independent combinations, legacy migration, strict parsing, packed fail-closed and NVS runtime/reboot rollback PASS')


if __name__ == "__main__":
    unittest.main()
