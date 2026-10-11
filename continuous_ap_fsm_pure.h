#pragma once
#include <stdint.h>
#include "vehicle_profile.h"

// continuous ap types
enum class ContApMethod : uint8_t { Unset, StalkDouble, ScrollSingle, ScrollDouble };
// Selected-input RX lifetime; independent of press/release waveform timing.
static constexpr uint32_t CONT_AP_SCROLL_FRESH_MS = 500u;
static inline uint32_t continuousApInputFreshLimitPure(ContApMethod method) {
  return method==ContApMethod::StalkDouble?500u:CONT_AP_SCROLL_FRESH_MS;
}
enum class ContApRoute : uint8_t { None, BodyA, ChassisB, PartyA };
struct ContApConfig { bool enabled = false; ContApMethod method = ContApMethod::Unset; };
struct ContApStockFrame {
  ContApRoute route=ContApRoute::None; uint16_t id=0; uint8_t dlc=0;
  uint8_t data[8]={}; uint32_t epoch=0, rxMs=0; bool valid=false;
};
enum class ContApSignal : uint8_t { None, Ap, Turn, Brake, Gear, Torque, Stalk, Scroll };
struct ContApDecodedInputs {
  ContApSignal signal=ContApSignal::None; bool valid=false, idle=false;
  int32_t value=0; uint8_t left=0,right=0;
};
struct ContApObservation { bool seen=false,valid=false; int32_t value=0; uint32_t rxMs=0,epoch=0; uint16_t sourceId=0; ContApRoute sourceRoute=ContApRoute::None; };
enum class ContApState : uint8_t { Idle, WaitSignalOff, WaitDelayAndReady, SendGesture, WaitResult, RetryWait, CleanupPending };
enum class ContApReason : uint8_t { None, Disabled, Unsupported, WaitingActive, WaitingSignal, WaitingReady, Stale, DriverInput, Brake, Gear, ApState, Torque, Deadline, Changed, Transport, Complete, CleanupUnconfirmed, InputUnavailable, WaitingApRelease, WaitingInput };
enum class ContApCommandKind : uint8_t { None, Press, Release };
struct ContApInputs {
  uint32_t nowMs=0,epoch=0,generation=0; uint8_t profile=0,topology=0; ContApConfig config;
  ContApObservation ap,turn,brake,gear,torque,control;
  bool templateFresh=false,transportAllowed=false,ownershipAllowed=false,ambiguous=false;
};
struct ContApCommand {
  ContApCommandKind kind=ContApCommandKind::None; ContApMethod method=ContApMethod::Unset;
  ContApRoute route=ContApRoute::None; uint8_t phase=0;
  uint32_t generation=0,epoch=0,token=0,preparedAtMs=0;
};
struct ContApStatus {
  ContApState state=ContApState::Idle; ContApReason reason=ContApReason::WaitingActive;
  ContApReason lastCancelReason=ContApReason::None;
  uint32_t attempts=0,txOk=0,txFail=0,cleanupSends=0;
  bool releaseOwed=false,cleanupUnconfirmed=false;
};

static inline const char *continuousApStateName(ContApState state){
  static const char *const names[]={"Idle","Waiting for signal off","Waiting for readiness","Sending gesture","Waiting for AP","Retry wait","Release pending"};
  const unsigned n=(unsigned)state;return n<7?names[n]:"Unknown";
}
static inline const char *continuousApReasonName(ContApReason reason){
  static const char *const names[]={"Ready","Disabled","Unsupported method for this vehicle connection","Waiting for AP active","Waiting for signal off","Waiting for readiness","Waiting for fresh vehicle state","Driver input","Brake applied","Drive gear required","AP unavailable","Steering torque","Episode limit","Settings or connection changed","CAN unavailable","Complete","Release unconfirmed","Input signal invalid","Waiting for AP release","Waiting for controls to settle"};
  const unsigned n=(unsigned)reason;return n<20?names[n]:"Unknown";
}

// continuous ap frames pure
static inline uint8_t continuousApStalkCrcPure(uint8_t pos,uint8_t counter) {
  static const uint8_t neutral[16]={0x46,0x44,0x52,0x6d,0x43,0x41,0xdd,0xf9,0x4c,0xa5,0xf6,0x8c,0x49,0x2f,0x31,0x3b};
  static const uint8_t delta[5]={0,0xe0,0xef,0x0f,0xf1};
  return neutral[counter&15]^delta[pos];
}
static inline bool continuousApStalkCrcValidPure(const ContApStockFrame &f) {
  const uint8_t pos=f.data[1]>>4;
  return f.valid&&f.id==0x229&&f.dlc==3&&f.route==ContApRoute::BodyA&&pos<=4&&f.data[2]==0
      &&f.data[0]==continuousApStalkCrcPure(pos,f.data[1]&15);
}
static inline bool continuousApScrollIdlePure(const ContApStockFrame &f) {
  const uint8_t *d=f.data;
  return f.valid&&f.id==0x3c2&&f.dlc==8&&(d[0]&3)==1&&(d[6]&0x10)
      &&((d[1]>>4)&3)==1&&(d[1]&3)==1&&((d[1]>>2)&3)==1
      &&((d[1]>>6)&3)==1&&((d[0]>>3)&3)==1&&((d[0]>>5)&3)==1
      &&(d[2]&63)==0&&(d[3]&63)==0&&(d[5]&6)==0;
}
static inline bool continuousApScrollFieldsValidPure(const uint8_t *d) {
  const uint8_t fields[]={uint8_t((d[1]>>4)&3),uint8_t(d[1]&3),uint8_t((d[1]>>2)&3),uint8_t((d[1]>>6)&3),uint8_t((d[0]>>3)&3),uint8_t((d[0]>>5)&3)};
  for(uint8_t value:fields)if(value!=1&&value!=2)return false;
  return (d[6]&0x10)!=0;
}
static inline bool continuousApDecodeFramePure(const ContApStockFrame &f,ContApDecodedInputs &o) {
  o={}; const uint8_t *d=f.data; const bool b=f.route==ContApRoute::ChassisB;
  if(b&&f.id==0x399) o.signal=ContApSignal::Ap;
  else if(b&&f.id==0x3f5) o.signal=ContApSignal::Turn;
  else if((b&&f.id==0x39d)||
          ((b||f.route==ContApRoute::BodyA||f.route==ContApRoute::PartyA)&&f.id==0x145)) o.signal=ContApSignal::Brake;
  else if(b&&(f.id==0x118||f.id==0x186)) o.signal=ContApSignal::Gear;
  else if((f.route==ContApRoute::BodyA&&(f.id==0x372||f.id==0x373))||(f.route==ContApRoute::PartyA&&f.id==0x370)) o.signal=ContApSignal::Torque;
  else if(f.route==ContApRoute::BodyA&&f.id==0x229) o.signal=ContApSignal::Stalk;
  else if((b||f.route==ContApRoute::BodyA)&&f.id==0x3c2) { if(f.dlc==8&&(d[0]&3)!=1)return false; o.signal=ContApSignal::Scroll; }
  else return false;
  if(!f.valid||f.dlc>8) return true;
  if(o.signal==ContApSignal::Brake) {
    if(f.id==0x145){
      if(f.dlc!=8)return true;
      o.value=(d[3]>>5)&3;
    } else {
      if(f.dlc!=3&&f.dlc!=5)return true;
      o.value=d[2]&3;
    }
    o.valid=o.value==1||o.value==2;return true;
  }
  if(o.signal==ContApSignal::Stalk) {
    o.valid=continuousApStalkCrcValidPure(f);o.value=d[1]>>4;o.idle=o.valid&&o.value==0;return true;
  }
  if(f.dlc!=8)return true;
  o.valid=true;
  switch(o.signal){
    case ContApSignal::Ap:o.value=d[0]&15;o.valid=o.value<=6||o.value==8||o.value==9||o.value==14;break;
    case ContApSignal::Turn:o.left=d[0]&3;o.right=(d[0]>>2)&3;o.value=d[0];o.valid=o.left<3&&o.right<3;break;
    case ContApSignal::Gear:o.value=(d[2]>>5)&7;o.valid=o.value>=1&&o.value<=4;break;
    case ContApSignal::Torque:{uint16_t raw=((uint16_t)(d[2]&15)<<8)|d[3];o.value=(int32_t)raw-2050;o.valid=raw<4094&&(d[0]&4)==0;break;}
    case ContApSignal::Scroll:o.value=(d[1]>>4)&3;o.valid=continuousApScrollFieldsValidPure(d);o.idle=o.valid&&continuousApScrollIdlePure(f);break;
    default:o.valid=false;break;
  }
  return true;
}
static inline bool continuousApBuildStalkPure(uint8_t pos,uint8_t counter,ContApStockFrame &out) {
  if(pos!=0&&pos!=4)return false;
  out={};out.valid=true;out.route=ContApRoute::BodyA;out.id=0x229;out.dlc=3;
  out.data[1]=(pos<<4)|(counter&15);out.data[0]=continuousApStalkCrcPure(pos,counter);return true;
}
static inline bool continuousApBuildClickPure(const ContApStockFrame &stock,uint8_t pressed,ContApStockFrame &out) {
  if((pressed!=1&&pressed!=2)||!continuousApScrollIdlePure(stock))return false;
  out=stock;out.data[1]=(stock.data[1]&0xcf)|(pressed<<4);return true;
}

// continuous ap policy pure
static inline uint32_t continuousApConfigEncodePure(ContApConfig c) {
  const uint8_t m=(uint8_t)c.method;
  if(m>3 || (c.enabled&&m==0)) return 0x100;
  return 0x100u|m|(c.enabled?4u:0u);
}
static inline bool continuousApConfigDecodePure(uint32_t raw, ContApConfig &out) {
  out={};
  if((raw&~7u)!=0x100u || (raw&7u)==4u) return false;
  out.enabled=(raw&4u)!=0; out.method=(ContApMethod)(raw&3u); return true;
}
static inline ContApRoute continuousApGestureRoutePure(uint8_t p, uint8_t t, ContApMethod m) {
  if(!vehicleProfileCanAIsBody(p,t)) return ContApRoute::None;
  const bool legacy=p==VEHICLE_MODEL_Y_LEGACY||p==VEHICLE_MODEL_3_LEGACY;
  if(legacy) return t==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS && m==ContApMethod::StalkDouble
      ? ContApRoute::BodyA:ContApRoute::None;
  if(m!=ContApMethod::ScrollSingle&&m!=ContApMethod::ScrollDouble) return ContApRoute::None;
  return ContApRoute::BodyA;
}
static inline bool vehicleProfileContinuousApMethodSupported(uint8_t p,uint8_t t,uint8_t m) {
  return continuousApGestureRoutePure(p,t,(ContApMethod)m)!=ContApRoute::None;
}

// RX readiness describes observations, never authorization to engage AP.
static inline const char *continuousApRxStatusPure(const ContApObservation &s,uint32_t now,uint32_t epoch,uint32_t limit) {
  if(!s.seen)return "Missing";
  if(!epoch||s.epoch!=epoch||(uint32_t)(now-s.rxMs)>limit)return "Stale";
  return s.valid?"Fresh":"Invalid";
}
static inline const char *continuousApSignalReadinessPure(const ContApInputs &i) {
  if(continuousApGestureRoutePure(i.profile,i.topology,i.config.method)==ContApRoute::None)return "Unsupported";
  const ContApObservation observations[]={i.ap,i.turn,i.brake,i.gear,i.torque,i.control};
  const uint32_t limits[]={1000,350,250,1000,250,continuousApInputFreshLimitPure(i.config.method)};
  bool missing=false,bad=false;
  for(unsigned n=0;n<6;++n){
    const char *state=continuousApRxStatusPure(observations[n],i.nowMs,i.epoch,limits[n]);
    missing|=state[0]=='M';bad|=state[0]!='F';
  }
  return missing?"Waiting for signals":bad?"Stale / Invalid":"Available";
}

// Independent brake sources cannot clear each other's applied/invalid state.
// A source must report release itself (or reset with the CAN epoch). Keeping an
// unsafe source after it becomes stale makes the normal freshness gate block.
static inline ContApObservation continuousApSelectBrakePure(
    const ContApObservation &ibst,const ContApObservation &espA,
    const ContApObservation &espB,uint32_t now,uint32_t epoch) {
  const ContApObservation *sources[]={&ibst,&espA,&espB};
  ContApObservation selected,observed;
  bool unsafeSelected=false;
  for(const auto *source:sources){
    if(!source->seen)continue;
    if(!observed.seen||(uint32_t)(now-source->rxMs)<(uint32_t)(now-observed.rxMs))observed=*source;
    if(!epoch||source->epoch!=epoch)continue;
    const bool unsafe=!source->valid||source->value!=1;
    if(!selected.seen||(unsafe&&!unsafeSelected)||
       (unsafe==unsafeSelected&&(uint32_t)(now-source->rxMs)<(uint32_t)(now-selected.rxMs))){
      selected=*source;unsafeSelected=unsafe;
    }
  }
  return selected.seen?selected:observed;
}

// continuous ap fsm pure
static inline bool continuousApFreshPure(const ContApObservation &s,const ContApInputs &i,uint32_t ceiling) {
  return s.seen&&s.valid&&s.epoch==i.epoch&&(uint32_t)(i.nowMs-s.rxMs)<=ceiling;
}
class ContinuousApFsmPure {
  ContApStatus st_;
  uint32_t epoch_=0,generation_=0,episodeMs_=0,signalOffMs_=0,lastTxMs_=0,resultMs_=0;
  uint32_t cleanupMs_=0,releaseMs_=0,token_=0,blockedApRx_=0,activeRxMs_=0;
  uint8_t profile_=0,topology_=0,phase_=0,cleanupTries_=0;
  bool initialized_=false,armed_=false,lastActive_=false,highTorque_=false,counted_=false;
  bool issued_=false,planValid_=false,releaseAccepted_=false;
  ContApConfig config_;
  ContApCommand issuedCommand_,pressEvidence_;
  void initialize(const ContApInputs &i){
    if(!initialized_){epoch_=i.epoch;generation_=i.generation;profile_=i.profile;topology_=i.topology;config_=i.config;initialized_=true;}
  }
  static bool active(int32_t a){return a>=3&&a<=6;}
  static bool offTurn(int32_t v){return (v&255)==0;}
  static bool singleTurn(int32_t v){return (v&0xf0)==0&&(((v&3)>0)^(((v>>2)&3)>0));}
  static bool physicalIdle(const ContApInputs &i,ContApMethod m){return !i.ambiguous&&continuousApFreshPure(i.control,i,continuousApInputFreshLimitPure(m))&&i.control.value==0&&i.templateFresh;}
  ContApReason safety(const ContApInputs &i) const {
    if(!continuousApFreshPure(i.ap,i,1000)||!continuousApFreshPure(i.turn,i,350)||!continuousApFreshPure(i.brake,i,250)||!continuousApFreshPure(i.gear,i,1000)||!continuousApFreshPure(i.torque,i,250))return ContApReason::Stale;
    if(i.brake.value!=1)return ContApReason::Brake;
    if(i.gear.value!=4)return ContApReason::Gear;
    if(!continuousApFreshPure(i.control,i,continuousApInputFreshLimitPure(config_.method)))return ContApReason::InputUnavailable;
    if(i.control.value>=2)return ContApReason::DriverInput;
    // Waiting through a normal adjustment is allowed before pressing. Once a
    // press is accepted, interruption must use the bounded release cleanup.
    if(st_.releaseOwed&&i.control.value!=0)return ContApReason::DriverInput;
    if(i.ambiguous)return ContApReason::InputUnavailable;
    if(!i.transportAllowed)return ContApReason::Transport;
    return ContApReason::None;
  }
  void stop(ContApReason reason,uint32_t now){
    armed_=lastActive_=false;planValid_=false;st_.reason=reason;
    if(reason!=ContApReason::None&&reason!=ContApReason::Complete&&reason!=ContApReason::Disabled)st_.lastCancelReason=reason;
    if(st_.releaseOwed){if(st_.state!=ContApState::CleanupPending){cleanupMs_=now;cleanupTries_=0;}st_.state=ContApState::CleanupPending;}
    else st_.state=ContApState::Idle;
  }
  ContApCommand prepare(ContApCommandKind kind,ContApMethod method,ContApRoute route,uint32_t now){
    issuedCommand_={kind,method,route,phase_,generation_,epoch_,++token_,now};
    issued_=planValid_=true;return issuedCommand_;
  }
public:
  ContApStatus status() const{return st_;}
  bool hasArmedActive() const{return armed_;}
  bool hasPendingPress() const{return issued_&&issuedCommand_.kind==ContApCommandKind::Press;}
  ContApMethod inputMethod() const{return st_.releaseOwed?pressEvidence_.method:config_.method;}
  ContApRoute ownedRoute() const{return st_.releaseOwed?pressEvidence_.route:(st_.state!=ContApState::Idle?continuousApGestureRoutePure(profile_,topology_,config_.method):ContApRoute::None);}
  // RX batches may contain both the indicated active state and its loss.
  // Record only fully validated intent here; command preparation stays in service.
  void observeActiveIntent(const ContApInputs &i){
    initialize(i);
    if(st_.state!=ContApState::Idle||st_.releaseOwed||!i.epoch||
       i.epoch!=epoch_||i.generation!=generation_||i.profile!=profile_||i.topology!=topology_||
       i.config.enabled!=config_.enabled||i.config.method!=config_.method||!i.config.enabled||
       continuousApGestureRoutePure(i.profile,i.topology,i.config.method)==ContApRoute::None)return;
    if(safety(i)==ContApReason::None&&active(i.ap.value)&&singleTurn(i.turn.value)&&i.ap.rxMs!=blockedApRx_){
      armed_=lastActive_=true;activeRxMs_=i.ap.rxMs;st_.cleanupUnconfirmed=false;
      st_.reason=ContApReason::WaitingApRelease;
    }
  }
  void reset(uint32_t epoch){
    const uint32_t ok=st_.txOk,fail=st_.txFail,cleanup=st_.cleanupSends;
    const bool unconfirmed=st_.releaseOwed;
    *this=ContinuousApFsmPure();st_.lastCancelReason=ContApReason::Changed;epoch_=epoch;st_.txOk=ok;st_.txFail=fail;st_.cleanupSends=cleanup;st_.cleanupUnconfirmed=unconfirmed;
  }
  void cancel(uint32_t gen,uint32_t epoch,uint32_t now,ContApReason reason=ContApReason::Changed){
    generation_=gen;
    if(epoch!=epoch_){reset(epoch);generation_=gen;return;}
    stop(reason,now);
  }
  bool allowsCommand(const ContApCommand &c,const ContApInputs &i) const {
    if(!issued_||!planValid_||c.kind==ContApCommandKind::None||c.token!=issuedCommand_.token||c.generation!=generation_||c.generation!=i.generation||c.epoch!=epoch_||c.epoch!=i.epoch||!i.transportAllowed||!i.ownershipAllowed)return false;
    if(continuousApGestureRoutePure(i.profile,i.topology,c.method)!=c.route||!physicalIdle(i,c.method))return false;
    if(st_.state==ContApState::CleanupPending&&(uint32_t)(i.nowMs-cleanupMs_)>=500)return false;
    if(st_.state!=ContApState::CleanupPending&&(uint32_t)(i.nowMs-episodeMs_)>=10000)return false;
    if(c.kind==ContApCommandKind::Release)return st_.releaseOwed;
    return i.config.enabled&&i.config.method==c.method&&safety(i)==ContApReason::None&&i.ap.value==2&&offTurn(i.turn.value)&&i.torque.value<250&&i.torque.value>-250&&!st_.releaseOwed;
  }
  ContApCommand service(const ContApInputs &i){
    initialize(i);
    if(i.epoch!=epoch_){reset(i.epoch);generation_=i.generation;profile_=i.profile;topology_=i.topology;config_=i.config;initialized_=true;blockedApRx_=i.ap.rxMs;}
    if(i.profile!=profile_||i.topology!=topology_){
      if(st_.releaseOwed)st_.cleanupUnconfirmed=true;
      st_.releaseOwed=false;issued_=false;stop(ContApReason::Changed,i.nowMs);blockedApRx_=i.ap.rxMs;
      profile_=i.profile;topology_=i.topology;
    }
    if(i.generation!=generation_||i.config.enabled!=config_.enabled||i.config.method!=config_.method){cancel(i.generation,i.epoch,i.nowMs);config_=i.config;blockedApRx_=i.ap.rxMs;}
    if(st_.releaseOwed&&releaseAccepted_&&physicalIdle(i,pressEvidence_.method)&&(int32_t)(i.control.rxMs-releaseMs_)>0){st_.releaseOwed=false;releaseAccepted_=false;}
    if(st_.state==ContApState::CleanupPending){
      if(!st_.releaseOwed){st_.state=ContApState::Idle;st_.reason=ContApReason::Complete;}
      else {
        if((uint32_t)(i.nowMs-cleanupMs_)>=500){st_.cleanupUnconfirmed=true;st_.releaseOwed=false;st_.state=ContApState::Idle;st_.reason=ContApReason::CleanupUnconfirmed;blockedApRx_=i.ap.rxMs;planValid_=false;return {};}
        if(issued_||releaseAccepted_||cleanupTries_>=2||!i.transportAllowed||!i.ownershipAllowed||!physicalIdle(i,pressEvidence_.method)||continuousApGestureRoutePure(i.profile,i.topology,pressEvidence_.method)!=pressEvidence_.route)return {};
        if(cleanupTries_&&(uint32_t)(i.nowMs-lastTxMs_)<40)return {};
        ++cleanupTries_;++st_.cleanupSends;return prepare(ContApCommandKind::Release,pressEvidence_.method,pressEvidence_.route,i.nowMs);
      }
    }
    const ContApRoute route=continuousApGestureRoutePure(i.profile,i.topology,i.config.method);
    if(!i.config.enabled||route==ContApRoute::None){stop(!i.config.enabled?ContApReason::Disabled:ContApReason::Unsupported,i.nowMs);return {};}
    const bool apFresh=continuousApFreshPure(i.ap,i,1000),isActive=apFresh&&active(i.ap.value);
    if(st_.state==ContApState::Idle){
      const ContApReason unsafe=safety(i);
      if(!apFresh||(armed_&&(uint32_t)(i.nowMs-activeRxMs_)>1000)){armed_=lastActive_=false;}
      if(isActive){
        // A fresh, indicated AP-active observation establishes lane-change intent.
        // General wheel rotation/tilt may continue; cancellation clicks may not.
        if(unsafe==ContApReason::None&&!i.ambiguous&&i.ap.rxMs!=blockedApRx_&&singleTurn(i.turn.value)){
          armed_=lastActive_=true;activeRxMs_=i.ap.rxMs;st_.cleanupUnconfirmed=false;
          st_.reason=ContApReason::WaitingApRelease;
        } else if(armed_&&lastActive_&&unsafe==ContApReason::None&&offTurn(i.turn.value)){
          // Indicator OFF and AP loss arrive independently. Preserve witnessed
          // intent until the existing active-evidence deadline; OFF cannot
          // arm or refresh it, and unsafe observations still discard it.
          st_.reason=ContApReason::WaitingApRelease;
        } else {
          armed_=lastActive_=false;
          st_.reason=unsafe!=ContApReason::None?unsafe:i.ambiguous?ContApReason::DriverInput:ContApReason::WaitingSignal;
        }
      } else if(apFresh&&(i.ap.value==1||i.ap.value==2)&&armed_&&lastActive_){
        armed_=lastActive_=false;
        if(unsafe!=ContApReason::None||(!singleTurn(i.turn.value)&&!offTurn(i.turn.value))){
          stop(unsafe!=ContApReason::None?unsafe:ContApReason::DriverInput,i.nowMs);return {};
        }
        st_.state=ContApState::WaitSignalOff;st_.reason=ContApReason::WaitingSignal;st_.attempts=0;episodeMs_=i.nowMs;counted_=false;highTorque_=false;
        if(offTurn(i.turn.value)){signalOffMs_=i.nowMs;st_.state=ContApState::WaitDelayAndReady;st_.reason=ContApReason::WaitingReady;}
      } else {
        if(apFresh&&!isActive)armed_=lastActive_=false;
        st_.reason=unsafe!=ContApReason::None?unsafe:ContApReason::WaitingActive;
      }
      return {};
    }
    if(isActive){stop(ContApReason::Complete,i.nowMs);blockedApRx_=i.ap.rxMs;return service(i);}
    const ContApReason unsafe=safety(i);
    if(unsafe!=ContApReason::None){stop(unsafe,i.nowMs);blockedApRx_=i.ap.rxMs;return service(i);}
    if(i.ap.value!=2&&!(i.ap.value==1&&st_.attempts==0)){stop(ContApReason::ApState,i.nowMs);blockedApRx_=i.ap.rxMs;return service(i);}
    if((uint32_t)(i.nowMs-episodeMs_)>=10000){stop(ContApReason::Deadline,i.nowMs);blockedApRx_=i.ap.rxMs;return service(i);}
    highTorque_=i.torque.value>=250||i.torque.value<=-250;
    if(st_.state==ContApState::WaitSignalOff){
      if((uint32_t)(i.nowMs-episodeMs_)>=6000){stop(ContApReason::Deadline,i.nowMs);blockedApRx_=i.ap.rxMs;return {};}
      if(offTurn(i.turn.value)){signalOffMs_=i.nowMs;st_.state=ContApState::WaitDelayAndReady;st_.reason=ContApReason::WaitingReady;}
      else if(!singleTurn(i.turn.value)){stop(ContApReason::DriverInput,i.nowMs);blockedApRx_=i.ap.rxMs;}
      return {};
    }
    if(!offTurn(i.turn.value)){stop(ContApReason::DriverInput,i.nowMs);blockedApRx_=i.ap.rxMs;return service(i);}
    if(issued_)return {};
    if(st_.state==ContApState::WaitDelayAndReady){
      st_.reason=highTorque_?ContApReason::Torque:!physicalIdle(i,config_.method)?ContApReason::WaitingInput:ContApReason::WaitingReady;
      if((uint32_t)(i.nowMs-signalOffMs_)<1000||i.ap.value!=2||highTorque_||!i.ownershipAllowed||!physicalIdle(i,config_.method))return {};
      phase_=0;st_.state=ContApState::SendGesture;st_.reason=ContApReason::None;
    }
    if(st_.state==ContApState::WaitResult){
      if((uint32_t)(i.nowMs-resultMs_)<700)return {};
      st_.state=ContApState::RetryWait;lastTxMs_=i.nowMs;
    }
    if(st_.state==ContApState::RetryWait){
      if((uint32_t)(i.nowMs-lastTxMs_)<300||st_.releaseOwed||highTorque_||!i.ownershipAllowed)return {};
      if(st_.attempts>=3){stop(ContApReason::Deadline,i.nowMs);blockedApRx_=i.ap.rxMs;return {};}
      phase_=0;counted_=false;st_.state=ContApState::SendGesture;
    }
    if(st_.state!=ContApState::SendGesture||!i.ownershipAllowed)return {};
    const bool press=(phase_&1)==0;
    const uint32_t gap=config_.method==ContApMethod::StalkDouble?40:(press?150:250);
    if(phase_&&(uint32_t)(i.nowMs-lastTxMs_)<gap)return {};
    if(!physicalIdle(i,config_.method)){st_.reason=ContApReason::WaitingInput;return {};}
    if(press&&(highTorque_||st_.releaseOwed))return {};
    if(!counted_){++st_.attempts;counted_=true;}
    return prepare(press?ContApCommandKind::Press:ContApCommandKind::Release,config_.method,route,i.nowMs);
  }
  void onTxResult(const ContApCommand &c,bool accepted,uint32_t now){
    if(!issued_||c.token!=issuedCommand_.token)return;
    issued_=false;lastTxMs_=now;
    if(accepted)++st_.txOk;else ++st_.txFail;
    if(c.epoch!=epoch_){if(accepted&&c.kind==ContApCommandKind::Press)st_.cleanupUnconfirmed=true;return;}
    if(accepted&&c.kind==ContApCommandKind::Press){st_.releaseOwed=true;releaseAccepted_=false;pressEvidence_=c;}
    if(accepted&&c.kind==ContApCommandKind::Release){releaseAccepted_=true;releaseMs_=now;}
    if(!planValid_||c.generation!=generation_){if(st_.releaseOwed&&st_.state!=ContApState::CleanupPending){cleanupMs_=now;cleanupTries_=0;st_.state=ContApState::CleanupPending;}return;}
    if(st_.state==ContApState::CleanupPending)return;
    if(!accepted){if(st_.releaseOwed)stop(ContApReason::Transport,now);else st_.state=ContApState::RetryWait;return;}
    ++phase_;
    if(phase_==(config_.method==ContApMethod::ScrollSingle?2:4)){st_.state=ContApState::WaitResult;resultMs_=now;}
  }
};
