#pragma once
#include "continuous_ap_policy_pure.h"
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
  static bool active(int32_t a){return a>=3&&a<=6;}
  static bool offTurn(int32_t v){return (v&255)==0;}
  static bool singleTurn(int32_t v){return (v&0xf0)==0&&(((v&3)>0)^(((v>>2)&3)>0));}
  static bool physicalIdle(const ContApInputs &i,ContApMethod m){return !i.ambiguous&&continuousApFreshPure(i.control,i,m==ContApMethod::StalkDouble?500:250)&&i.control.value==0&&i.templateFresh;}
  ContApReason safety(const ContApInputs &i) const {
    if(!continuousApFreshPure(i.ap,i,1000)||!continuousApFreshPure(i.turn,i,350)||!continuousApFreshPure(i.brake,i,250)||!continuousApFreshPure(i.gear,i,1000)||!continuousApFreshPure(i.torque,i,250))return ContApReason::Stale;
    if(i.brake.value!=1)return ContApReason::Brake;
    if(i.gear.value!=4)return ContApReason::Gear;
    if(!continuousApFreshPure(i.control,i,config_.method==ContApMethod::StalkDouble?500:250))return ContApReason::InputUnavailable;
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
    if(!initialized_){epoch_=i.epoch;generation_=i.generation;profile_=i.profile;topology_=i.topology;config_=i.config;initialized_=true;}
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
