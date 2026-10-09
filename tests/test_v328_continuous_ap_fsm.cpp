#include <cassert>
#include <initializer_list>
#include "../continuous_ap_fsm_pure.h"
static void fresh(ContApInputs &i,uint32_t t){i.nowMs=t;for(auto *s:{&i.ap,&i.turn,&i.brake,&i.gear,&i.torque,&i.control}){s->seen=s->valid=true;s->rxMs=t;s->epoch=i.epoch;}}
static ContApInputs base(ContApMethod m){ContApInputs i;i.epoch=1;i.generation=1;i.profile=m==ContApMethod::StalkDouble?3:4;i.topology=2;i.config={true,m};i.transportAllowed=i.ownershipAllowed=i.templateFresh=true;i.ap.value=3;i.turn.value=1;i.brake.value=1;i.gear.value=4;fresh(i,100);return i;}
static ContApCommand start(ContinuousApFsmPure &f,ContApInputs &i,uint32_t t=100){fresh(i,t);f.service(i);i.ap.value=2;fresh(i,t+1);f.service(i);i.turn.value=0;fresh(i,t+2);f.service(i);fresh(i,t+1001);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,t+1002);return f.service(i);}
int main(){
  for(ContApMethod m:{ContApMethod::StalkDouble,ContApMethod::ScrollSingle,ContApMethod::ScrollDouble}){
    ContinuousApFsmPure f;auto i=base(m);auto c=start(f,i);
    assert(c.kind==ContApCommandKind::Press&&c.method==m&&f.allowsCommand(c,i));
    auto changed=i;changed.config.enabled=false;assert(!f.allowsCommand(c,changed));
    changed=i;changed.brake.value=2;assert(!f.allowsCommand(c,changed));
    const uint32_t first=i.nowMs;f.onTxResult(c,true,first);
    fresh(i,first+(m==ContApMethod::StalkDouble?40:250));c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,true,i.nowMs);
    if(m!=ContApMethod::ScrollSingle){fresh(i,i.nowMs+(m==ContApMethod::StalkDouble?40:150));c=f.service(i);assert(c.kind==ContApCommandKind::Press);f.onTxResult(c,true,i.nowMs);fresh(i,i.nowMs+(m==ContApMethod::StalkDouble?40:250));c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,true,i.nowMs);}
    i.ap.value=3;fresh(i,i.nowMs+1);f.service(i);assert(!f.status().releaseOwed);
    for(int n=0;n<10;n++){fresh(i,i.nowMs+100);assert(f.service(i).kind==ContApCommandKind::None);}
  }
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollDouble);auto c=start(f,i,0xfffffc00u);assert(c.kind==ContApCommandKind::Press);f.onTxResult(c,true,i.nowMs);i.ap.value=3;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);assert(f.allowsCommand(c,i));f.onTxResult(c,true,i.nowMs);fresh(i,i.nowMs+1);f.service(i);assert(!f.status().releaseOwed);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollDouble);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.generation=2;i.config.enabled=false;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release&&c.generation==2&&c.method==ContApMethod::ScrollDouble);f.onTxResult(c,true,i.nowMs);fresh(i,i.nowMs+1);f.service(i);assert(!f.status().releaseOwed);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,false,i.nowMs);assert(f.status().attempts==1);for(int n=0;n<3;n++){fresh(i,i.nowMs+10000);assert(f.service(i).kind==ContApCommandKind::None);}assert(!f.status().releaseOwed);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.control.valid=false;fresh(i,i.nowMs+1);i.control.valid=false;assert(f.service(i).kind==ContApCommandKind::None);fresh(i,i.nowMs+501);i.control.valid=false;assert(f.service(i).kind==ContApCommandKind::None);assert(f.status().cleanupUnconfirmed);}
  for(int bad=0;bad<6;bad++){ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);if(bad==0)i.ap.value=2;if(bad==1)i.turn.value=5;if(bad==2)i.brake.value=2;if(bad==3)i.gear.value=1;if(bad==4)i.config.enabled=false;if(bad==5)i.torque.value=300;auto c=start(f,i);assert(c.kind==ContApCommandKind::None);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);auto stale=i;stale.nowMs+=251;assert(!f.allowsCommand(c,stale));f.cancel(2,1,i.nowMs);assert(!f.allowsCommand(c,i));f.onTxResult(c,true,i.nowMs);i.generation=2;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);for(int attempt=1;attempt<=3;attempt++){
    assert(c.kind==ContApCommandKind::Press&&f.status().attempts==(unsigned)attempt);f.onTxResult(c,true,i.nowMs);
    fresh(i,i.nowMs+250);c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,true,i.nowMs);
    fresh(i,i.nowMs+1);f.service(i);fresh(i,i.nowMs+699);assert(f.service(i).kind==ContApCommandKind::None);
    fresh(i,i.nowMs+299);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,i.nowMs+1);c=f.service(i);
  }assert(c.kind==ContApCommandKind::None);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);f.service(i);i.ap.value=2;fresh(i,101);f.service(i);fresh(i,6101);assert(f.service(i).kind==ContApCommandKind::None);assert(f.status().state==ContApState::Idle);i.turn.value=0;fresh(i,7101);assert(f.service(i).kind==ContApCommandKind::None);}
  for(int fault:{0,1,8,9,14,15}){ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);i.ap.value=fault;fresh(i,i.nowMs+1);assert(!f.allowsCommand(c,i));assert(f.service(i).kind==ContApCommandKind::None);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollDouble);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.epoch=2;fresh(i,i.nowMs+1);assert(f.service(i).kind==ContApCommandKind::None);assert(f.status().cleanupUnconfirmed);}

  // A stale AP-active observation cannot qualify a later available edge, even
  // when there was no scheduler tick during the gap (including rollover).
  for(uint32_t t:{100u,0xfffffc00u}){ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);fresh(i,t);f.service(i);i.ap.value=2;i.turn.value=1;fresh(i,t+1500);f.service(i);i.turn.value=0;fresh(i,t+1501);f.service(i);fresh(i,t+2501);assert(f.service(i).kind==ContApCommandKind::None);}

  // Continuous high torque pauses and terminates after 5 s, including wrap.
  for(uint32_t t:{100u,0xfffffc00u}){ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);fresh(i,t);f.service(i);i.ap.value=2;fresh(i,t+1);f.service(i);i.turn.value=0;i.torque.value=250;fresh(i,t+2);f.service(i);fresh(i,t+4999);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,t+5002);assert(f.service(i).kind==ContApCommandKind::None&&f.status().state==ContApState::Idle);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollDouble);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.config.enabled=false;++i.generation;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,false,i.nowMs);fresh(i,i.nowMs+40);c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,false,i.nowMs);fresh(i,i.nowMs+40);assert(f.service(i).kind==ContApCommandKind::None&&f.status().cleanupSends==2);fresh(i,i.nowMs+500);f.service(i);assert(f.status().cleanupUnconfirmed);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.transportAllowed=false;fresh(i,i.nowMs+1);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,i.nowMs+500);f.service(i);assert(f.status().cleanupUnconfirmed);i.transportAllowed=true;fresh(i,i.nowMs+1);assert(f.service(i).kind==ContApCommandKind::None);}

  // Final admission must also enforce time limits if a task was preempted
  // after preparing a command while new RX kept every input fresh.
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);fresh(i,i.nowMs+10000);assert(!f.allowsCommand(c,i));}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.config.enabled=false;++i.generation;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);fresh(i,i.nowMs+500);assert(!f.allowsCommand(c,i));}
}
