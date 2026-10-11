"""Compare actual old/new route orchestration with event-recording host doubles."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'vehicle_logic.h').read_text()
def fn(name):
 m=re.search(r'^static [^\n]*\b'+name+r'\(',s,re.M);assert m,name
 a=m.start();b=s.index('{',m.end());d=1;e=b+1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 if name in ('tsl9InputObserveRoute','tsl9InputServiceRoute'):a=s.rfind('template <typename Frame>',0,a)
 return s[a:e]
prefix=r'''
#include <cstdint>
#include <cstdio>
struct can_frame{uint8_t data[8]={};uint8_t can_dlc=8;};
struct twai_message_t{uint8_t data[8]={};uint8_t data_length_code=8;};
uint32_t clockMs=1000;uint32_t millis(){return clockMs++;}
uint32_t esp_random(){puts("rng");return 123;}
int tsl9InputMux=0;void portENTER_CRITICAL(int*){puts("lock");}void portEXIT_CRITICAL(int*){puts("unlock");}
constexpr uint8_t TSL9_INPUT_MUX1_PURE=1,TSL9_INPUT_COMMAND_NONE_PURE=0,TSL9_INPUT_COMMAND_STEP_PURE=1,TSL9_INPUT_COMMAND_CENTER_PURE=2;
uint8_t tsl9InputMuxPure(const uint8_t*d){return d[0]&3;}
int8_t tsl9InputDecodeSignedSixBitPure(uint8_t d){return int8_t(d<<2)>>2;}
struct Tsl9InputCommandPure{uint8_t kind=0;};
struct Tsl9InputInputsPure{bool templateFresh=false;uint8_t mode=0;};
uint32_t tsl9InputTemplateMaxAgeMsPure(uint8_t){return 250;}
bool body=true,supported=true,owns=false,sendOk=true;int commandKind=1;
bool activeProfileTsl9InputOnBodyCanA(){return body;}bool activeProfileTsl9InputSupported(){return supported;}
enum class ContApRoute{BodyA,ChassisB};bool continuousApOwnsInputRoute(ContApRoute){return owns;}
Tsl9InputInputsPure tsl9InputInputsSnapshot(uint32_t now,bool a){printf("inputs %u %d\n",now,a);return {};}
struct Scheduler{
 void observeManualTicks(int8_t l,int8_t r,uint32_t now){printf("observe %d %d %u\n",l,r,now);}
 Tsl9InputCommandPure service(const Tsl9InputInputsPure&i){printf("service %d\n",i.templateFresh);return {uint8_t(commandKind)};}
 Tsl9InputCommandPure onCommandResult(Tsl9InputCommandPure c,bool ok,uint32_t now,uint32_t random){printf("result %u %d %u %u\n",c.kind,ok,now,random);return {uint8_t(c.kind==1?2:0)};}
}tsl9InputScheduler;
can_frame tsl9InputCanATemplate;twai_message_t tsl9InputCanBTemplate;
volatile bool tsl9InputCanATemplateValid=false,tsl9InputCanBTemplateValid=false;
volatile uint32_t tsl9InputCanATemplateMs=0,tsl9InputCanBTemplateMs=0;
uint32_t tsl9InputMux1Rx=0,tsl9InputTxOk=0,tsl9InputTxFail=0,tsl9InputCleanupTx=0;
bool tsl9InputSendCanA(const can_frame&f,const Tsl9InputCommandPure&c){printf("send A %u %u\n",f.data[3],c.kind);return sendOk;}
bool tsl9InputSendCanB(const twai_message_t&f,const Tsl9InputCommandPure&c){printf("send B %u %u\n",f.data[3],c.kind);return sendOk;}
'''
main=r'''
int main(){
 for(unsigned i=0;i<256;++i){
 body=i&1;supported=i&2;owns=i&4;sendOk=i&8;commandKind=(i>>4)%3;
 clockMs=1000;tsl9InputCanATemplateValid=tsl9InputCanBTemplateValid=i&64;
 tsl9InputCanATemplateMs=tsl9InputCanBTemplateMs=(i&128)?1000:0;
 can_frame a;twai_message_t b;a.data[0]=b.data[0]=(i&32)?1:0;a.data[2]=b.data[2]=63;a.data[3]=b.data[3]=2;
 tsl9InputObserveCanA(a);tsl9InputObserveCanB(b);tsl9InputServiceCanA();tsl9InputServiceCanB();
 printf("state %u %u %u %u %u %u %u\n",clockMs,tsl9InputMux1Rx,tsl9InputTxOk,tsl9InputTxFail,tsl9InputCleanupTx,unsigned(tsl9InputCanATemplateMs),unsigned(tsl9InputCanBTemplateMs));
 }
}
'''
current='\n'.join(fn(n) for n in ['tsl9InputObserveRoute','tsl9InputObserveCanA','tsl9InputObserveCanB','tsl9InputServiceRoute','tsl9InputServiceCanA','tsl9InputServiceCanB'])
with tempfile.TemporaryDirectory() as tmp:
 outputs=[]
 for label,code in [('baseline',(ROOT/'tests/tsl9_route_flow_baseline.h').read_text()),('current',current)]:
  source=Path(tmp)/(label+'.cpp');binary=Path(tmp)/label;source.write_text(prefix+code+main)
  subprocess.run([os.environ.get('CXX','g++'),'-std=c++17','-Wall','-Wextra','-Werror',str(source),'-o',str(binary)],check=True)
  outputs.append(subprocess.check_output([str(binary)]))
 assert outputs[0]==outputs[1], 'route observations, locking, timestamp/RNG, sends, cleanup or counters diverged'
 print('PASS: 256 old/new route scenarios match all event traces')
