"""Host characterization of BLE registry edits, legacy cleanup and pending scopes."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASELINE = os.environ.get("T2CAN_BLE_BASELINE")

def extract(source, name):
    m = re.search(r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", source, re.M)
    assert m, name
    end = source.index("{", m.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[m.start():end] + "\n"

def production():
    root = Path(BASELINE) if BASELINE else ROOT
    ble = (root / "s3xy_ble.h").read_text()
    if not BASELINE:
        scope = re.search(r"enum S3xyPendingClearScope[^}]*};", ble, re.S)
        assert scope
        return scope[0] + "\n" + "\n".join(extract(ble, n) for n in (
            "s3xyClearPendingActionsLocked", "s3xyRegistryRename", "s3xyRegistrySetAction",
            "s3xyRegistrySetAutoConnect", "s3xyFinalizeLegacyKeys"))
    # Optional characterization of the immutable pre-extraction source. Normal
    # runs always require current production helpers, never a fixture fallback.
    web = (root / "web_api.h").read_text()
    rename = extract(web, "httpS3xyDeviceRename")
    rename = rename[rename.index("  portENTER_CRITICAL"):rename.index('  server.send(200')]
    action = extract(web, "httpS3xyDeviceAction")
    action = action[action.index("  bool validGesture"):action.index("  if (!validGesture)")]
    auto = extract(web, "httpS3xyDeviceAuto")
    auto = auto[auto.index("  const uint32_t now"):auto.index('  server.send(200')]
    logic = (root / "vehicle_logic.h").read_text()
    cleanup = logic[logic.index('  // s3xy/addr is a legacy'):logic.index('  if (!ok) {', logic.index('  // s3xy/addr is a legacy'))]
    can = (root / "can_runtime.h").read_text()
    def clear(src):
        start = src.index("  s3xyActionPending = 0;")
        end = src.index("  portEXIT_CRITICAL(&s3xyMux);", start)
        text = src[start:end]
        return text[:text.index("  for (")] if "  for (" in text else text
    full = clear(extract(ble, "s3xyRequestRuntimeStop"))
    recovery = clear(can)
    return "enum S3xyPendingClearScope { S3XY_PENDING_CAN_RECOVERY, S3XY_PENDING_ALL };\n" + (
        "static void s3xyClearPendingActionsLocked(S3xyPendingClearScope scope) { if(scope == S3XY_PENDING_ALL) {" + full + "} else {" + recovery + "}}\n"
        "static void s3xyRegistryRename(uint8_t slot, const String &name) {" + rename + "}\n"
        "static bool s3xyRegistrySetAction(uint8_t slot, const String &gesture, uint8_t action) {" + action + "if (!validGesture) return false; s3xyRegistrySaveSlot(slot); return true;}\n"
        "static void s3xyRegistrySetAutoConnect(uint8_t slot, bool enabled) {" + auto + "}\n"
        "static bool s3xyFinalizeLegacyKeys() {bool ok=true; Preferences p;" + cleanup + "return ok;}\n")

PREAMBLE = r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
struct String : std::string {
  using std::string::string;
  String(std::string s):std::string(s){}
  String(unsigned char n):std::string(std::to_string(n)){}
};
static int lockDepth=0, s3xyMux=0;
#define portENTER_CRITICAL(x) do { (void)(x); assert(lockDepth++==0); } while(0)
#define portEXIT_CRITICAL(x) do { (void)(x); assert(--lockDepth==0); } while(0)
struct Device { char name[28]; uint8_t singleAction,doubleAction,longAction; bool autoConnect,manualPaused,connected; uint32_t nextAttemptMs; };
static Device s3xyDevices[3] = {};
static bool s3xyBluetoothEnabled=false,s3xyAutoEnabled=false;
static uint32_t clockNow=0;
static uint32_t millis(){return clockNow;}
static std::vector<std::string> events;
static std::vector<Device> saved;
static void s3xyRegistrySaveSlot(uint8_t slot){assert(lockDepth==0);events.push_back("save"+std::to_string(slot));saved.push_back(s3xyDevices[slot]);}
static constexpr int S3XY_LOG_INFO=1;
static void s3xyLogPush(int,const char* msg,void*,int,int,int8_t slot){assert(lockDepth==0);events.push_back(std::string(msg)+std::to_string(slot));}
static constexpr uint8_t S3XY_MAX_DEVICES=3, S3XY_PEER_ID_MAX=8;
static unsigned registeredCount=0;
static unsigned s3xyRegisteredCount(){return registeredCount;}
static std::map<std::string,bool> opens;
static std::map<std::string,size_t> sizes,reads;
struct Preferences {
  std::string ns;
  bool begin(const char* name,bool ro){assert(!ro);ns=name;events.push_back("begin:"+ns);return opens[ns];}
  void end(){events.push_back("end:"+ns);}
  bool remove(const char* key){events.push_back("remove:"+ns+":"+key);return false;}
  size_t getBytesLength(const char* key){events.push_back("size:"+ns+":"+key);return sizes[key];}
  size_t getBytes(const char* key,void*,size_t len){assert(len==S3XY_PEER_ID_MAX);events.push_back("read:"+ns+":"+key);return reads[key];}
};
static volatile uint32_t s3xyActionPending,s3xyAccelActionPending,s3xyPerformanceActionPending;
static volatile uint32_t s3xyResearchCaptureAPending,s3xyResearchCaptureBPending,s3xyResearchCaptureCPending,s3xyResearchCaptureDPending,s3xyResearchCaptureResetPending;
static volatile uint32_t s3xyTlsscActionPending,s3xyAutoBlinkerActionPending,s3xyLeftBlinkerActionPending,s3xyRightBlinkerActionPending;
"""
MAIN = r"""
int main(){
  s3xyRegistryRename(1,"New button"); assert(std::string(s3xyDevices[1].name)=="New button");assert(saved.back().name[0]=='N');assert(events==std::vector<std::string>{"save1"});
  events.clear();s3xyRegistryRename(1,std::string(60,'x'));assert(strlen(s3xyDevices[1].name)==27);assert(saved.back().name[27]==0);
  for(const char* gesture:{"single","double","long","invalid","Single",""}){
    events.clear();s3xyDevices[1].singleAction=1;s3xyDevices[1].doubleAction=2;s3xyDevices[1].longAction=3;
    const bool valid=std::string(gesture)=="single"||std::string(gesture)=="double"||std::string(gesture)=="long";
    assert(s3xyRegistrySetAction(1,gesture,42)==valid);
    assert(s3xyDevices[1].singleAction==(std::string(gesture)=="single"?42:1));
    assert(s3xyDevices[1].doubleAction==(std::string(gesture)=="double"?42:2));
    assert(s3xyDevices[1].longAction==(std::string(gesture)=="long"?42:3));
    assert(events.size()==(valid?1u:0u));
  }
  for(unsigned mask=0;mask<32;++mask)for(uint32_t now:{0u,123u,UINT32_MAX-100u}){
    const bool enabled=mask&1,bt=mask&2,global=mask&4,connected=mask&8,paused=mask&16;
    events.clear();clockNow=now;s3xyBluetoothEnabled=bt;s3xyAutoEnabled=global;
    s3xyDevices[1].connected=connected;s3xyDevices[1].manualPaused=paused;s3xyDevices[1].nextAttemptMs=555;
    s3xyRegistrySetAutoConnect(1,enabled);
    assert(s3xyDevices[1].autoConnect==enabled);
    assert(s3xyDevices[1].manualPaused==(enabled?false:paused));
    assert(s3xyDevices[1].nextAttemptMs==(enabled&&bt&&global&&!connected?uint32_t(now+250U):0));
    assert(events.size()==2&&events[0]=="save1");
    assert(saved.back().nextAttemptMs==s3xyDevices[1].nextAttemptMs);
    assert(events[1]==std::string("device auto-connect ")+(enabled?"enabled1":"disabled1"));
  }
  volatile uint32_t *pending[]={&s3xyActionPending,&s3xyAccelActionPending,&s3xyPerformanceActionPending,&s3xyResearchCaptureAPending,&s3xyResearchCaptureBPending,&s3xyResearchCaptureCPending,&s3xyResearchCaptureDPending,&s3xyResearchCaptureResetPending,&s3xyTlsscActionPending,&s3xyAutoBlinkerActionPending,&s3xyLeftBlinkerActionPending,&s3xyRightBlinkerActionPending};
  for(auto scope:{S3XY_PENDING_CAN_RECOVERY,S3XY_PENDING_ALL}){
    for(auto p:pending)*p=7;portENTER_CRITICAL(&s3xyMux);s3xyClearPendingActionsLocked(scope);assert(lockDepth==1);portEXIT_CRITICAL(&s3xyMux);
    for(unsigned i=0;i<12;++i)assert(*pending[i]==(scope==S3XY_PENDING_CAN_RECOVERY&&i>=10?7u:0u));
  }
  for(unsigned mask=0;mask<8;++mask)for(size_t rawLen:{size_t(0),size_t(1),size_t(8),size_t(9)})for(bool complete:{false,true}){
    events.clear();opens["s3xy"]=mask&1;opens["s3xyreg"]=mask&2;registeredCount=(mask&4)?1:0;
    sizes.clear();reads.clear();sizes["b0"]=rawLen;reads["b0"]=complete?rawLen:0;
    assert(s3xyFinalizeLegacyKeys()==bool((mask&1)&&(mask&2)));
    std::vector<std::string> expected={"begin:s3xy"};
    if(mask&1){if(mask&4)expected.push_back("remove:s3xy:addr");expected.push_back("end:s3xy");}
    expected.push_back("begin:s3xyreg");
    if(mask&2){
      expected.push_back("size:s3xyreg:b0");
      if(rawLen&&rawLen<=8){expected.push_back("read:s3xyreg:b0");if(complete)expected.push_back("remove:s3xyreg:i0");}
      expected.push_back("size:s3xyreg:b1");expected.push_back("size:s3xyreg:b2");expected.push_back("end:s3xyreg");
    }
    assert(events==expected);
  }
}
"""

class BleOwnershipHostTest(unittest.TestCase):
    def test_production_consumers_delegate_to_ble_owner(self):
        if BASELINE:
            self.skipTest("pre-extraction characterization")
        ble = (ROOT / "s3xy_ble.h").read_text()
        web = (ROOT / "web_api.h").read_text()
        for handler, helper in (("httpS3xyDeviceRename", "s3xyRegistryRename"),
                                ("httpS3xyDeviceAction", "s3xyRegistrySetAction"),
                                ("httpS3xyDeviceAuto", "s3xyRegistrySetAutoConnect")):
            body = extract(web, handler)
            self.assertIn(helper + "(", body)
            self.assertNotIn("s3xyDevices[", body)
            self.assertNotIn("s3xyRegistrySaveSlot", body)
        self.assertIn("s3xyFinalizeLegacyKeys()", extract((ROOT / "vehicle_logic.h").read_text(), "nvsSchemaFinalize"))
        self.assertIn("s3xyClearPendingActionsLocked(S3XY_PENDING_CAN_RECOVERY)", (ROOT / "can_runtime.h").read_text())
        for name in ("s3xyRequestRuntimeStop", "s3xyResetAllBluetoothData", "s3xyClearLog"):
            self.assertIn("s3xyClearPendingActionsLocked(S3XY_PENDING_ALL)", extract(ble, name))
        notify = extract(ble, "s3xyNotifyCallbackForSlot")
        self.assertIn("s3xyParseNotifyTokensPure(data, len)", notify)
        self.assertNotIn("s3xyFindToken", notify)

    def test_registry_cleanup_pending_semantics(self):
        code = PREAMBLE + production() + MAIN
        with tempfile.TemporaryDirectory(prefix="v3285-ble-owner-") as tmp:
            source = Path(tmp) / "probe.cpp"
            binary = Path(tmp) / "probe"
            source.write_text(code)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == "__main__":
    unittest.main()
