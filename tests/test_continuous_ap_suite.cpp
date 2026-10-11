// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstring>
#include <initializer_list>
#include "../continuous_ap_fsm_pure.h"

// Coverage origin: test_v328_continuous_ap_frames.cpp
namespace case_continuous_ap_frames {
int run(){
  ContApStockFrame f; f.valid=true; f.route=ContApRoute::ChassisB; f.id=0x399; f.dlc=8;
  ContApDecodedInputs d;
  assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==0);
  for(int a=0;a<16;a++){f.data[0]=a;continuousApDecodeFramePure(f,d);assert(d.value==a);}
  f.id=0x3f5;f.data[0]=8;assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.right==2&&d.left==0);
  f.data[0]=3;continuousApDecodeFramePure(f,d);assert(!d.valid);
  f.id=0x39d;f.dlc=3;f.data[2]=1;assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==1);
  f.data[2]=3;continuousApDecodeFramePure(f,d);assert(!d.valid);
  // ESP_status driverBrakeApply is byte3 bits5..6 on either physical CAN port.
  for(auto route:{ContApRoute::BodyA,ContApRoute::PartyA,ContApRoute::ChassisB}) {
    f={};f.valid=true;f.route=route;f.id=0x145;f.dlc=8;
    for(uint8_t raw=0;raw<4;++raw){
      f.data[3]=uint8_t(0x9f|(raw<<5));
      assert(continuousApDecodeFramePure(f,d)&&d.signal==ContApSignal::Brake);
      assert(d.value==raw&&d.valid==(raw==1||raw==2));
    }
    for(uint8_t dlc:{uint8_t(0),uint8_t(3),uint8_t(5),uint8_t(7)}){
      f.dlc=dlc;assert(continuousApDecodeFramePure(f,d)&&!d.valid);
    }
  }
  f={};f.valid=true;f.route=ContApRoute::None;f.id=0x145;f.dlc=8;
  assert(!continuousApDecodeFramePure(f,d));
  f.id=0x372;f.route=ContApRoute::BodyA;f.dlc=8;f.data[0]=0;f.data[2]=8;f.data[3]=2;
  assert(continuousApDecodeFramePure(f,d)&&d.valid&&d.value==0);
  f.data[2]=8;f.data[3]=0xfc;continuousApDecodeFramePure(f,d);assert(d.valid&&d.value==250);
  f.data[2]=7;f.data[3]=8;continuousApDecodeFramePure(f,d);assert(d.valid&&d.value==-250);
  f.data[2]=15;f.data[3]=254;continuousApDecodeFramePure(f,d);assert(!d.valid);
  f.route=ContApRoute::ChassisB;assert(!continuousApDecodeFramePure(f,d));
  const uint8_t crc[16]={0x46,0x44,0x52,0x6d,0x43,0x41,0xdd,0xf9,0x4c,0xa5,0xf6,0x8c,0x49,0x2f,0x31,0x3b};
  for(int c=0;c<16;c++){
    assert(continuousApBuildStalkPure(0,c,f)); assert(f.data[0]==crc[c]&&f.data[1]==c&&f.data[2]==0);
    assert(continuousApStalkCrcValidPure(f));
    assert(continuousApBuildStalkPure(4,c,f));assert(f.data[0]==(crc[c]^0xf1));
    assert(continuousApStalkCrcValidPure(f));f.data[2]=1;assert(!continuousApStalkCrcValidPure(f));
  }
  f={};f.valid=true;f.route=ContApRoute::ChassisB;f.id=0x3c2;f.dlc=8;
  f.data[0]=0x29;f.data[1]=0x55;f.data[6]=0x10;f.data[7]=0xa6;
  ContApStockFrame out;
  assert(continuousApBuildClickPure(f,2,out)); assert(out.data[1]==0x65&&f.data[1]==0x55);
  for(int i=0;i<8;i++)if(i!=1)assert(out.data[i]==f.data[i]);
  assert(continuousApBuildClickPure(f,1,out)&&out.data[1]==0x55);
  for(int raw:{0,3})assert(!continuousApBuildClickPure(f,raw,out));
  f.data[3]=1;assert(!continuousApBuildClickPure(f,2,out));f.data[3]=0;
  f.data[6]=0;assert(!continuousApBuildClickPure(f,2,out));f.data[6]=0x10;
  f.data[0]&=~3;assert(!continuousApBuildClickPure(f,2,out));
  return 0;
}
} // namespace case_continuous_ap_frames

// Coverage origin: test_v328_continuous_ap_fsm.cpp
namespace case_continuous_ap_fsm {
static void fresh(ContApInputs &i,uint32_t t){i.nowMs=t;for(auto *s:{&i.ap,&i.turn,&i.brake,&i.gear,&i.torque,&i.control}){s->seen=s->valid=true;s->rxMs=t;s->epoch=i.epoch;}}
static ContApInputs base(ContApMethod m){ContApInputs i;i.epoch=1;i.generation=1;i.profile=m==ContApMethod::StalkDouble?3:4;i.topology=2;i.config={true,m};i.transportAllowed=i.ownershipAllowed=i.templateFresh=true;i.ap.value=3;i.turn.value=1;i.brake.value=1;i.gear.value=4;fresh(i,100);return i;}
static ContApCommand start(ContinuousApFsmPure &f,ContApInputs &i,uint32_t t=100){fresh(i,t);f.service(i);i.ap.value=2;fresh(i,t+1);f.service(i);i.turn.value=0;fresh(i,t+2);f.service(i);fresh(i,t+1001);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,t+1002);return f.service(i);}
int run(){
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

  // Continuous high torque pauses beyond 5 s and resumes within the episode, including wrap.
  for(uint32_t t:{100u,0xfffffc00u}){ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);fresh(i,t);f.service(i);i.ap.value=2;fresh(i,t+1);f.service(i);i.turn.value=0;i.torque.value=250;fresh(i,t+2);f.service(i);fresh(i,t+4999);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,t+5002);assert(f.service(i).kind==ContApCommandKind::None&&f.status().state==ContApState::WaitDelayAndReady);i.torque.value=0;fresh(i,t+6002);assert(f.service(i).kind==ContApCommandKind::Press);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollDouble);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.config.enabled=false;++i.generation;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,false,i.nowMs);fresh(i,i.nowMs+40);c=f.service(i);assert(c.kind==ContApCommandKind::Release);f.onTxResult(c,false,i.nowMs);fresh(i,i.nowMs+40);assert(f.service(i).kind==ContApCommandKind::None&&f.status().cleanupSends==2);fresh(i,i.nowMs+500);f.service(i);assert(f.status().cleanupUnconfirmed);}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.transportAllowed=false;fresh(i,i.nowMs+1);assert(f.service(i).kind==ContApCommandKind::None);fresh(i,i.nowMs+500);f.service(i);assert(f.status().cleanupUnconfirmed);i.transportAllowed=true;fresh(i,i.nowMs+1);assert(f.service(i).kind==ContApCommandKind::None);}

  // Only the selected input ages here: other vehicle observations remain fresh.
  // Changing the scroll limit to <500 or >500 must fail at the boundary, even at wrap.
  for(uint32_t startMs:{100u,0xfffffc00u})
    for(auto method:{ContApMethod::ScrollSingle,ContApMethod::ScrollDouble})
      for(uint32_t age:{250u,251u,316u,499u,500u,501u}) {
        ContinuousApFsmPure f;auto i=base(method);auto c=start(f,i,startMs);
        i.control.rxMs=i.nowMs-age;
        assert(f.allowsCommand(c,i)==(age<=500));
        assert(std::strcmp(continuousApSignalReadinessPure(i),age<=500?"Available":"Stale / Invalid")==0);
        if(age<=500){i.brake.rxMs=i.nowMs-251;assert(!f.allowsCommand(c,i));}
      }

  // Final admission must also enforce time limits if a task was preempted
  // after preparing a command while new RX kept every input fresh.
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);fresh(i,i.nowMs+10000);assert(!f.allowsCommand(c,i));}
  {ContinuousApFsmPure f;auto i=base(ContApMethod::ScrollSingle);auto c=start(f,i);f.onTxResult(c,true,i.nowMs);i.config.enabled=false;++i.generation;fresh(i,i.nowMs+1);c=f.service(i);assert(c.kind==ContApCommandKind::Release);fresh(i,i.nowMs+500);assert(!f.allowsCommand(c,i));}
  return 0;
}
} // namespace case_continuous_ap_fsm

// Coverage origin: test_v328_continuous_ap_policy.cpp
namespace case_continuous_ap_policy {
int run() {
  assert(continuousApGestureRoutePure(3,2,ContApMethod::StalkDouble)==ContApRoute::BodyA);
  for(uint8_t p=0;p<7;p++) for(uint8_t t=0;t<5;t++) for(uint8_t m=0;m<5;m++) {
    const bool legacy=p==3||p==5, refresh=p==2||p==4;
    const bool want=(t==2&&legacy&&m==1)||(t==2&&refresh&&(m==2||m==3));
    assert(vehicleProfileContinuousApMethodSupported(p,t,m)==want);
  }
  ContApConfig c;
  assert(continuousApConfigEncodePure({true,ContApMethod::StalkDouble})==0x105);
  assert(continuousApConfigEncodePure({true,ContApMethod::ScrollSingle})==0x106);
  assert(continuousApConfigEncodePure({true,ContApMethod::ScrollDouble})==0x107);
  for(uint32_t raw=0;raw<1024;raw++) {
    const bool want=raw>=0x100&&raw<=0x107&&raw!=0x104;
    assert(continuousApConfigDecodePure(raw,c)==want);
    if(want) assert(continuousApConfigEncodePure(c)==raw);
    else assert(!c.enabled&&c.method==ContApMethod::Unset);
  }
  return 0;
}
} // namespace case_continuous_ap_policy

int main() {
  if (case_continuous_ap_policy::run()) return 1;
  if (case_continuous_ap_frames::run()) return 1;
  if (case_continuous_ap_fsm::run()) return 1;
  return 0;
}
