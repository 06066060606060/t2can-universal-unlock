from pathlib import Path
import os,subprocess,tempfile
P=Path(__file__).resolve().parents[1]
s=(P/'can_core.h').read_text();start=s.index('static bool canTxMcpSendValidated(');end=s.index('static bool canTxMcpSendTagged(',start)
code=r"""
#include <cassert>
#include <cstdint>
#include "runtime_gate_pure.h"
namespace MCP2515{enum ERROR{ERROR_OK,ERROR_FAIL};}
struct can_frame{uint32_t can_id;uint8_t can_dlc,data[8];};
int canTxBarrierMutex=1;bool locked=false,canTxAdministrativeHold=false,mcpReady=true;
constexpr int pdTRUE=1;constexpr uint8_t CAN_TX_FRESH_BOTH=3,CAN_TX_TRACE_SOURCE_DEFAULT=0;
struct Barrier{uint32_t epoch=0;uint8_t freshMask=0;}canTxBarrierState;
bool canTxBarrierAllowsMaskedPure(const Barrier &b,uint32_t e,uint8_t m){return b.epoch==e&&(b.freshMask&m)==m;}
int xSemaphoreTake(int,int){if(locked)return 0;locked=true;return 1;}void xSemaphoreGive(int){assert(locked);locked=false;}
void canATraceRecordTx(const can_frame*,McpTxResultReason,MCP2515::ERROR,uint8_t){}
struct Controller{int sends=0;bool ok=true;MCP2515::ERROR sendMessage(const can_frame*){assert(locked);++sends;return ok?MCP2515::ERROR_OK:MCP2515::ERROR_FAIL;}}Can_A;
bool eligible=true;int validated=0;
bool validate(can_frame *f,void*){assert(locked);++validated;f->can_id=0x3c2;return eligible;}
"""+s[start:end]+r"""
int main(){
 canTxBarrierState.epoch=9;canTxBarrierState.freshMask=3;can_frame f={};MCP2515::ERROR e;
 assert(canTxMcpSendValidated(&f,9,validate,nullptr,e)&&e==MCP2515::ERROR_OK&&Can_A.sends==1&&f.can_id==0x3c2);
 eligible=false;assert(!canTxMcpSendValidated(&f,9,validate,nullptr,e)&&Can_A.sends==1);
 eligible=true;int calls=validated;assert(!canTxMcpSendValidated(&f,8,validate,nullptr,e)&&validated==calls);
 canTxAdministrativeHold=true;assert(!canTxMcpSendValidated(&f,9,validate,nullptr,e)&&validated==calls);
 canTxAdministrativeHold=false;mcpReady=false;assert(!canTxMcpSendValidated(&f,9,validate,nullptr,e)&&validated==calls);
 mcpReady=true;canTxBarrierState.freshMask=1;assert(!canTxMcpSendValidated(&f,9,validate,nullptr,e)&&validated==calls);
 canTxBarrierState.freshMask=3;locked=true;assert(!canTxMcpSendValidated(&f,9,validate,nullptr,e)&&validated==calls);locked=false;
 Can_A.ok=false;assert(canTxMcpSendValidated(&f,9,validate,nullptr,e)&&e==MCP2515::ERROR_FAIL&&Can_A.sends==2);
}
"""
with tempfile.TemporaryDirectory() as td:
 src=Path(td)/'host.cpp';out=Path(td)/'host';src.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(P),str(src),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
print('MCP production final enqueue barrier PASS')
