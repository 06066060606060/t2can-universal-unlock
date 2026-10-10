from continuous_ap_host_fixture import run,ROOT
import re
source=(ROOT/'vehicle_logic.h').read_text()
def function(name):
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',source,re.M);assert m,name
 end=source.index('{',m.start())+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]+'\n'
run(r"""
#include "tsl9_input_scheduler_pure.h"
// Existing transport entrypoints for the unmodified Nag send functions.
[[maybe_unused]] constexpr uint8_t CAN_TX_FRESH_VH=2;
bool canTxMcpSend(const can_frame*m,uint32_t e,MCP2515::ERROR&r,void*){can_frame out=*m;auto v=[](can_frame*,void*){return true;};return canTxMcpSendValidated(&out,e,v,nullptr,r);}
esp_err_t canTxTwaiTransmitWithMask(const twai_message_t*m,uint32_t e,uint8_t mask){twai_message_t out=*m;auto v=[](twai_message_t*,void*){return true;};return canTxTwaiTransmitValidated(&out,e,mask,v,nullptr);}
void researchCaptureObserveTxVh(uint16_t,uint8_t,const uint8_t*,bool){}
"""+function('tsl9InputSendCanA')+function('tsl9InputSendCanB')+r"""
int main(){
 continuousApConfig={true,ContApMethod::ScrollDouble};
 feed(100,3,1);continuousApService(ContApRoute::BodyA);feed(101,2,1);continuousApService(ContApRoute::BodyA);feed(102,2,0);continuousApService(ContApRoute::BodyA);feed(1102,2,0);continuousApService(ContApRoute::BodyA);assert(sent==1);
 can_frame pressed={};pressed.can_id=0x3c2;pressed.can_dlc=8;memcpy(pressed.data,sentData,8);
 clockMs=1103;continuousApObserveStock(0,0x3c2,pressed.data,8,epoch,clockMs);continuousApService(ContApRoute::BodyA);
 clockMs=1604;continuousApService(ContApRoute::BodyA);assert(continuousApSnapshot().cleanupUnconfirmed);int sends=sent;
 Tsl9InputCommandPure command={};command.kind=TSL9_INPUT_COMMAND_STEP_PURE;command.mode=TSL9_INPUT_MODE_RIGHT_SPEED_PURE;command.rightTick=1;
 assert(!tsl9InputSendCanA(pressed,command)&&sent==sends); // Never replay accepted click from stale Nag template.
 feed(1605,3,0);assert(tsl9InputSendCanA(pressed,command)&&sent==sends+1&&(sentData[1]&0x30)==0x10&&sentData[3]==1); // Compose from latest released stock.
 // Repeat on Chassis B for Party+Chassis, and reject physical press before enqueue.
 continuousApResetForEpoch(++epoch);activeVehicleTopology=3;feed(1700,3,0);
 twai_message_t old={};old.identifier=0x3c2;old.data_length_code=8;memcpy(old.data,pressed.data,8);
 assert(tsl9InputSendCanB(old,command)&&(sentData[1]&0x30)==0x10);
 clockMs=1701;continuousApObserveStock(1,0x3c2,pressed.data,8,epoch,clockMs);sends=sent;assert(!tsl9InputSendCanB(old,command)&&sent==sends);
}
""")
print('Shared production Nag sends do not replay Continuous AP clicks PASS')
