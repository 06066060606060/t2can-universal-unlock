"""Execute surviving TLSSC/DMS production functions with ordinary AP state."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
logic=(ROOT/'vehicle_logic.h').read_text()
def function(name):
    match=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',logic,re.M)
    assert match,name
    end=logic.index('{',match.start())+1;depth=1
    while depth:
        depth+=(logic[end]=='{')-(logic[end]=='}');end+=1
    return logic[match.start():end]+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include "r79_dms_composition_pure.h"
#include "tsl9_hands_on_0x399_pure.h"
int stateMux=0,driverMonitoringControlMux=0,roadContextMux=0;
void portENTER_CRITICAL(int*){}void portEXIT_CRITICAL(int*){}
bool driverMonitoringDisableEnabled=true;
bool profileSupported=true,apValid=true,apActive=false;
bool activeProfileDmsNagSupported(){return profileSupported;}
void nagApGateSnapshot(bool& valid,bool& active){valid=apValid;active=apActive;}
uint32_t millis(){return 100u;}
struct twai_message_t {uint8_t data[8]={0};int flags=0;};
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_INVALID_STATE=1,pdTRUE=1,CAN_TX_FRESH_BOTH=3;
int canTxBarrierMutex=1,canTxBarrierState=0;
bool held=false,busy=false,canTxAdministrativeHold=false,twaiReady=true,fresh=true,admission=true;
int xSemaphoreTake(int,int){if(busy)return 0;assert(!held);held=true;return 1;}
void xSemaphoreGive(int){assert(held);held=false;}
bool canTxBarrierAllowsMaskedPure(int,uint32_t,int){return fresh;}
bool twaiNonSummonAdmissionOpen(){return admission;}
uint32_t canTxEpochSnapshot(){return 1u;}
unsigned sends=0;twai_message_t sent;
int twai_transmit(const twai_message_t*f,int wait){assert(held&&wait==0);++sends;sent=*f;return ESP_OK;}
void canBTraceRecordTx(const twai_message_t*,int){}
bool tlsscEnabled=true,gateAPActive=false,gateNOAActive=false,tlsscBlockInNoa=false;
bool tlsscInjectedActive=false,tlsscClearPending=false,highwayBlocked=false;
bool tlsscHighwayGateBlocked(uint32_t){return highwayBlocked;}
unsigned tlsscHighwayGateBlockedCount=0,sumTxFail=0,sumTxOk=0,tlsscClearTxOk=0,tlsscClearTxFail=0;
bool getBit(const uint8_t*d,int b){return (d[b/8]>>(b%8))&1u;}
void setBit(uint8_t*d,int b,bool v){if(v)d[b/8]|=1u<<(b%8);else d[b/8]&=~(1u<<(b%8));}
'''
for name in ('driverMonitoringControlSnapshot','r79DmsControlActive','tlsscTransmitGuarded','injectTLSSC'):
    code+=function(name)
code+=r'''
void reset(){sends=0;tlsscEnabled=true;gateAPActive=gateNOAActive=tlsscBlockInNoa=false;
 tlsscInjectedActive=tlsscClearPending=highwayBlocked=canTxAdministrativeHold=busy=false;
 twaiReady=fresh=admission=true;}
int main(){
 // A parked/inactive AP state cannot activate either removed test's local bypass.
 assert(!r79DmsControlActive());
 apActive=true;assert(r79DmsControlActive());
 apValid=false;assert(!r79DmsControlActive());apValid=true;
 driverMonitoringDisableEnabled=false;assert(!r79DmsControlActive());driverMonitoringDisableEnabled=true;
 profileSupported=false;assert(!r79DmsControlActive());profileSupported=true;
 twai_message_t stock;stock.data[0]=0;stock.data[2]=0xA5;
 reset();injectTLSSC(stock);assert(sends==0&&!tlsscInjectedActive);
 gateAPActive=true;injectTLSSC(stock);
 assert(sends==1&&getBit(sent.data,38)&&getBit(sent.data,39)&&sent.data[2]==0xA5);
 assert(stock.data[4]==0&&tlsscInjectedActive);
 // Normal AP loss still clears a previously owned TLSSC overlay once.
 gateAPActive=false;injectTLSSC(stock);
 assert(sends==2&&!getBit(sent.data,38)&&!getBit(sent.data,39)&&!tlsscInjectedActive);
 injectTLSSC(stock);assert(sends==2);
 for(unsigned reason=0;reason<8;++reason){reset();gateAPActive=true;
  if(reason==0)canTxAdministrativeHold=true;if(reason==1)twaiReady=false;
  if(reason==2)fresh=false;if(reason==3)admission=false;if(reason==4)busy=true;
  if(reason==5)highwayBlocked=true;if(reason==6){gateNOAActive=true;tlsscBlockInNoa=true;}
  if(reason==7)tlsscEnabled=false;
  injectTLSSC(stock);assert(sends==0&&!held);}
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'ap.cpp';exe=Path(d)/'ap';src.write_text(code)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(ROOT),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Surviving TLSSC/DMS ordinary AP gates and TLSSC clear/admission PASS')
