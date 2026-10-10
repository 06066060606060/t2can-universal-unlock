"""Run production ISA transport, active-status and HTTP handlers with host doubles."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def functions(filename, name):
    text = (ROOT / filename).read_text(encoding="utf-8")
    matches = list(re.finditer(
        r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", text, re.M
    ))
    assert matches, name
    result = ""
    for match in matches:
        end = text.index("{", match.start()) + 1
        depth = 1
        while depth:
            depth += (text[end] == "{") - (text[end] == "}")
            end += 1
        result += text[match.start():end] + "\n"
    return result


code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include "tsl9_hands_on_0x399_pure.h"

struct NagConfig {
 bool enabled=false; uint8_t method=0,tsl9Sequence=1,tsl9DowngradeWindow=1;
} nagCfg;
int nagCfgMux=0,nagTsl9Mux=0,nagDiagMux=0,isaSuppressionControlMux=0;
void portENTER_CRITICAL(int*){} void portEXIT_CRITICAL(int*){}
uint32_t now=1000; uint32_t millis(){return now;}
bool yl=false,chassis=true,bodySelected=false,nagSupported=false,isaSupported=true;
bool activeProfileIsYl(){return yl;}
bool activeCanBIsChassis(){return chassis;}
bool activeProfileNagTsl9Supported(){return nagSupported;}
bool activeProfileIsaSuppressionSupported(){return isaSupported;}
bool nagTsl9Body39BSelected(){return bodySelected&&nagSupported&&!yl;}
bool nagTsl9Chassis399Selected(){return !yl&&chassis&&nagSupported&&!bodySelected;}
bool apValid=true,apActive=true;
void nagApGateSnapshot(bool&valid,bool&active){valid=apValid;active=apActive;}
bool nonSummonAdmission=true;
bool twaiNonSummonAdmissionOpen(){return nonSummonAdmission;}
bool isaSuppressionEnabled=true;
volatile uint32_t isaSuppressionGeneration=7;
uint32_t isaSuppressionModified=0,isaSuppressionTxOk=0,isaSuppressionTxFail=0;
Tsl9HandsOnStatePure nagTsl9State{};
uint32_t nagTsl9Rx=0,nagTsl9Modified=0,nagTsl9HandsOnModified=0;
uint32_t nagTsl9TxOk=0,nagTsl9TxFail=0,nagTxOk=0,nagTxFail=0;
uint32_t nagSessionTxOk=0,nagEchoCount=0,nagLastTxOkMs=0,nagMaxTxGapMs=0;
uint8_t nagLastInjectedHo=0;
uint32_t nagGapMaxUpdatePure(uint32_t previous,uint32_t current,uint32_t maximum){
 return current-previous>maximum?current-previous:maximum;
}
uint32_t mcpTxOk=0,mcpTxFail=0;uint8_t mcpTxFailConsecutive=0;
struct can_frame{uint32_t can_id=0x399;uint8_t can_dlc=8;uint8_t data[8]{};};
struct twai_message_t{
 uint32_t identifier=0x399,flags=0; bool extd=false,rtr=false;
 uint8_t data_length_code=8,data[8]{};
};
struct MCP2515{enum ERROR{ERROR_OK=0,ERROR_FAIL=1};};
enum McpTxResultReason{MCP_TX_INVALID_MSG,MCP_TX_SENT};
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_FAIL=1;
constexpr uint8_t CAN_TX_TRACE_SOURCE_DEFAULT=0,CAN_TX_FRESH_BOTH=3;
uint32_t epoch=2;uint32_t canTxEpochSnapshot(){return epoch;}
unsigned attemptsA=0,attemptsB=0,sendsA=0,sendsB=0;
can_frame sentA{};twai_message_t sentB{};
bool guardedFail=false,cancelDuringTx=false;
bool canTxMcpSendTaggedGuarded(const can_frame*f,uint32_t e,uint8_t source,
 const volatile uint32_t*generation,uint32_t snapshot,
 MCP2515::ERROR&error,McpTxResultReason*reason){
 assert(e==epoch&&source==CAN_TX_TRACE_SOURCE_DEFAULT);
 assert(generation==&isaSuppressionGeneration&&snapshot==7);
 ++attemptsA;if(cancelDuringTx)++isaSuppressionGeneration;
 if(guardedFail||*generation!=snapshot){error=MCP2515::ERROR_FAIL;return false;}
 ++sendsA;sentA=*f;error=MCP2515::ERROR_OK;*reason=MCP_TX_SENT;return true;
}
esp_err_t canTxTwaiTransmitWithMaskTaggedGuarded(const twai_message_t*f,
 uint32_t e,uint8_t mask,uint8_t source,
 const volatile uint32_t*generation,uint32_t snapshot){
 assert(e==epoch&&mask==CAN_TX_FRESH_BOTH&&source==CAN_TX_TRACE_SOURCE_DEFAULT);
 assert(generation==&isaSuppressionGeneration&&snapshot==7);
 ++attemptsB;if(cancelDuringTx)++isaSuppressionGeneration;
 if(guardedFail||*generation!=snapshot)return ESP_FAIL;
 ++sendsB;sentB=*f;return ESP_OK;
}
struct String:std::string{using std::string::string;String(const std::string&s):std::string(s){}};
struct JsonWriterArduino {
 String&out;bool first=true;explicit JsonWriterArduino(String&s):out(s){out="{";}
 void prefix(const char*k){if(!first)out+=",";first=false;out+="\"";out+=k;out+="\":";}
 void boolean(const char*k,bool b){prefix(k);out+=b?"true":"false";}
 void string(const char*k,const char*v){prefix(k);out+="\"";out+=v;out+="\"";}
 void u32(const char*k,uint32_t v){prefix(k);out+=std::to_string(v);}
 void finish(){out+="}";}
};
struct Server {
 std::map<std::string,String> parameters;int status=0;String body;
 unsigned args(){return parameters.size();}
 bool hasArg(const char*k){return parameters.count(k);}
 String arg(const char*k){return parameters[k];}
 void send(int s,const char*,const String&b){status=s;body=b;}
}server;
bool saveOk=true;unsigned applyCalls=0;
bool isaSuppressionControlApply(bool enabled){
 ++applyCalls;if(!saveOk)return false;isaSuppressionEnabled=enabled;return true;
}
'''

for name in ("isaSuppressionControlSnapshot", "isaSuppressionControlActive"):
    code += functions("vehicle_logic.h", name)
for name in ("nagTsl9RecordTx", "nagProcessTsl9Mcp", "nagProcessTsl9Twai399"):
    code += functions("can_core.h", name)
for name in ("httpBoolArg", "isaSuppressionControlJson",
             "httpIsaSuppressionControlConfig", "httpIsaSuppressionControlUpdate"):
    code += functions("web_api.h", name)

code += r'''
void reset(){
 nagCfg={};nagCfg.tsl9Sequence=TSL9_SEQUENCE_EXTENDED_PURE;
 nagCfg.tsl9DowngradeWindow=TSL9_DOWNGRADE_AP_SESSION_PURE;
 yl=bodySelected=nagSupported=false;chassis=isaSupported=apValid=apActive=true;
 isaSuppressionEnabled=true;isaSuppressionGeneration=7;nagTsl9State={};
 isaSuppressionModified=isaSuppressionTxOk=isaSuppressionTxFail=0;
 nagTsl9Rx=nagTsl9Modified=nagTsl9HandsOnModified=nagTsl9TxOk=nagTsl9TxFail=0;
 nagTxOk=nagTxFail=nagSessionTxOk=nagEchoCount=nagLastTxOkMs=nagMaxTxGapMs=0;
 mcpTxOk=mcpTxFail=mcpTxFailConsecutive=0;
 attemptsA=attemptsB=sendsA=sendsB=applyCalls=0;
 guardedFail=cancelDuringTx=false;nonSummonAdmission=true;saveOk=true;server={};sentA={};sentB={};
}
void fill(uint8_t*d,uint8_t ap=3,uint8_t ho=4){
 const uint8_t original[8]={0xB0,0x83,0x92,0x73,0x64,0xC3,0xFB,0x00};
 std::memcpy(d,original,8);d[0]=(d[0]&0xF0u)|ap;
 d[5]=(d[5]&0xC3u)|(ho<<2);d[7]=tsl9ChecksumForCanIdPure(0x399,d);
}
void verifyIsaOnly(const uint8_t*before,const uint8_t*after,uint16_t id){
 assert(after[1]==(before[1]|0x20u));
 assert(after[6]==(before[6]&0x0Fu)); // counter 15 wraps once to zero
 assert(after[7]==tsl9ChecksumForCanIdPure(id,after));
 for(unsigned i:{0u,2u,3u,4u,5u})assert(before[i]==after[i]);
}
int main(){
 // Party+Chassis rejects Nag TSL9 yet independently transmits ISA on B399.
 // Every valid Hands-On state behaves equally with Nag OFF or torque selected.
 for(bool enabled:{false,true})for(uint8_t ho:{0,1,2,3,4,5,15}){
  reset();nagCfg.enabled=enabled;nagCfg.method=NAG_METHOD_TORQUE_PURE;
  twai_message_t src;fill(src.data,3,ho);auto stock=src;
  assert(nagProcessTsl9Twai399(src));assert(sendsB==1&&sendsA==0);
  verifyIsaOnly(stock.data,sentB.data,0x399);assert(!std::memcmp(src.data,stock.data,8));
  assert(isaSuppressionModified==1&&isaSuppressionTxOk==1&&isaSuppressionTxFail==0);
  assert(nagTsl9Modified==0&&nagTsl9HandsOnModified==0&&nagSessionTxOk==0&&nagEchoCount==0);
 }
 // Legacy Body selection affects only Nag: ISA always uses Chassis B399.
 reset();nagSupported=bodySelected=true;nagCfg.enabled=true;nagCfg.method=NAG_METHOD_TSL9_PURE;
 can_frame body;body.can_id=0x39B;fill(body.data);auto bodyStock=body;
 assert(nagProcessTsl9Mcp(body));assert(sendsA==1&&isaSuppressionModified==0);
 assert(!(sentA.data[1]&0x20u));assert(((sentA.data[5]>>2)&15u)==1u);
 assert(sentA.data[7]==tsl9ChecksumForCanIdPure(0x39B,sentA.data));
 assert(!std::memcmp(body.data,bodyStock.data,8));
 nagTsl9State={false,777};
 twai_message_t src;fill(src.data);assert(nagProcessTsl9Twai399(src));
 assert(sendsB==1&&isaSuppressionModified==1&&nagTsl9HandsOnModified==1);
 assert(nagTsl9Rx==1&&nagTsl9Modified==1);
 assert(!nagTsl9State.apActive&&nagTsl9State.apActiveSinceMs==777);
 verifyIsaOnly(src.data,sentB.data,0x399);
 // Co-located transformations on Chassis and YL Party produce one TX,
 // advance the counter once and account for both owners independently.
 for(bool party:{false,true}){
  reset();yl=party;chassis=!party;nagSupported=true;
  nagCfg.enabled=true;nagCfg.method=NAG_METHOD_TSL9_PURE;
  if(party){can_frame f;fill(f.data);assert(nagProcessTsl9Mcp(f));assert(sendsA==1&&sendsB==0);
   assert((sentA.data[6]>>4)==0&&((sentA.data[5]>>2)&15u)==1u);
   assert(sentA.data[1]&0x20u);assert(sentA.data[7]==tsl9ChecksumForCanIdPure(0x399,sentA.data));
  }else{twai_message_t f;fill(f.data);assert(nagProcessTsl9Twai399(f));assert(sendsB==1&&sendsA==0);
   assert((sentB.data[6]>>4)==0&&((sentB.data[5]>>2)&15u)==1u);
   assert(sentB.data[1]&0x20u);assert(sentB.data[7]==tsl9ChecksumForCanIdPure(0x399,sentB.data));}
  assert(isaSuppressionModified==1&&isaSuppressionTxOk==1&&nagTsl9HandsOnModified==1);
  assert(nagTsl9TxOk==1&&nagSessionTxOk==1&&nagEchoCount==1);
 }
 // Disabled/already-suppressed/invalid AP and malformed frames remain stock.
 for(bool party:{false,true})for(unsigned reason=0;reason<9;++reason){
  reset();yl=party;chassis=!party;can_frame a;twai_message_t b;fill(a.data);fill(b.data);
  if(reason==0)isaSuppressionEnabled=false;
  if(reason==1){a.data[1]|=0x20;b.data[1]|=0x20;}
  if(reason==2){a.data[0]=(a.data[0]&0xF0u)|2;b.data[0]=(b.data[0]&0xF0u)|2;}
  if(reason==3){a.data[0]=(a.data[0]&0xF0u)|15;b.data[0]=(b.data[0]&0xF0u)|15;}
  if(reason==4){a.can_dlc=7;b.data_length_code=7;}
  if(reason==5){a.can_id=0x39B;b.identifier=0x39B;}
  if(reason==6){a.can_id|=0x80000000u;b.extd=true;}
  if(reason==7){a.can_id|=0x40000000u;b.rtr=true;}
  if(reason==8)isaSupported=false;
  if(party)assert(!nagProcessTsl9Mcp(a));else assert(!nagProcessTsl9Twai399(b));
  assert(attemptsA+attemptsB==0&&isaSuppressionModified==0);
 }
 // Shared guarded send receives the independent cancellation generation.
 for(bool party:{false,true})for(bool cancel:{false,true}){
  reset();yl=party;chassis=!party;cancelDuringTx=cancel;guardedFail=!cancel;
  can_frame a;twai_message_t b;fill(a.data);fill(b.data);
  if(party)assert(!nagProcessTsl9Mcp(a));else assert(!nagProcessTsl9Twai399(b));
  assert(attemptsA+attemptsB==1&&sendsA+sendsB==0);
  assert(isaSuppressionModified==1&&isaSuppressionTxFail==1&&isaSuppressionTxOk==0);
  assert(nagTsl9TxFail==0&&nagTxFail==0&&nagEchoCount==0);
 }
 reset();nonSummonAdmission=false;twai_message_t held;fill(held.data);
 assert(!nagProcessTsl9Twai399(held));assert(attemptsB==0&&sendsB==0);
 assert(isaSuppressionTxFail==1&&isaSuppressionTxOk==0);
 reset();nagSupported=true;nagCfg.enabled=true;nagCfg.method=NAG_METHOD_TSL9_PURE;
 isaSuppressionEnabled=false;nonSummonAdmission=false;fill(held.data);
 assert(nagProcessTsl9Twai399(held));assert(sendsB==1&&nagTsl9TxOk==1);
 // Active status and API capability do not borrow Nag TSL9 support.
 reset();assert(isaSuppressionControlActive());httpIsaSuppressionControlConfig();
 assert(server.status==200&&server.body.find("\"supported\":true")!=String::npos);
 assert(server.body.find("CHASSIS_0x399")!=String::npos);
 for(unsigned reason=0;reason<4;++reason){reset();
  if(reason==0)isaSuppressionEnabled=false;if(reason==1)isaSupported=false;
  if(reason==2)apValid=false;if(reason==3)apActive=false;
  assert(!isaSuppressionControlActive());}
 reset();bodySelected=true;httpIsaSuppressionControlConfig();
 assert(server.body.find("CHASSIS_0x399")!=String::npos&&server.body.find("BODY_0x39B")==String::npos);
 yl=true;httpIsaSuppressionControlConfig();assert(server.body.find("PARTY_0x399")!=String::npos);
 reset();isaSuppressionEnabled=false;server.parameters={{"enabled","1"}};
 httpIsaSuppressionControlUpdate();assert(server.status==200&&isaSuppressionEnabled&&applyCalls==1);
 for(const char*invalid:{"","2","TRUE"," 1","-1"}){
  reset();server.parameters={{"enabled",invalid}};httpIsaSuppressionControlUpdate();
  assert(server.status==400&&applyCalls==0);}
 reset();server.parameters={{"enabled","1"},{"extra","1"}};
 httpIsaSuppressionControlUpdate();assert(server.status==400&&applyCalls==0);
 reset();isaSupported=false;server.parameters={{"enabled","1"}};
 httpIsaSuppressionControlUpdate();assert(server.status==409&&applyCalls==0);
 server.parameters={{"enabled","0"}};httpIsaSuppressionControlUpdate();
 assert(server.status==200&&!isaSuppressionEnabled&&applyCalls==1);
 reset();isaSuppressionEnabled=false;saveOk=false;server.parameters={{"enabled","1"}};
 httpIsaSuppressionControlUpdate();assert(server.status==503&&!isaSuppressionEnabled);
}
'''

with tempfile.TemporaryDirectory(prefix="isa-v327-host-") as directory:
    source = Path(directory) / "test.cpp"
    executable = Path(directory) / "test"
    source.write_text(code, encoding="utf-8")
    subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra",
        "-Werror", "-Wno-unused-function", "-Wno-unused-const-variable",
        "-I", str(ROOT), str(source), "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)

print("PASS production ISA routes, transport isolation, composition, active gate and API")
