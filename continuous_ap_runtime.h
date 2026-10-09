#pragma once
#include "continuous_ap_fsm_pure.h"
#include "continuous_ap_frames_pure.h"
static portMUX_TYPE continuousApMux=portMUX_INITIALIZER_UNLOCKED;
static ContApConfig continuousApConfig;
static uint32_t continuousApGeneration=1;
static ContinuousApFsmPure continuousApFsm;
static ContApObservation continuousApAp,continuousApTurn,continuousApBrake,continuousApGear118,continuousApGear186;
static ContApObservation continuousApTorqueS,continuousApTorqueP,continuousApTorqueParty,continuousApStalkControl,continuousApScrollControl;
static ContApStockFrame continuousApStalkStock,continuousApScrollStock,continuousApLastTx;
static bool continuousApManualSeen=false,continuousApCancelLatched=false,continuousApEchoAmbiguous=false;
static uint32_t continuousApManualMs=0,continuousApLastTxMs=0;
static uint8_t continuousApStalkCounter=0;
static inline bool continuousApNagBusy() {
  portENTER_CRITICAL(&tsl9InputMux);
  const auto s=tsl9InputScheduler.snapshot((uint32_t)millis());
  portEXIT_CRITICAL(&tsl9InputMux);
  return s.active||s.cleanupPending;
}
static inline ContApConfig continuousApControlSnapshot(){portENTER_CRITICAL(&continuousApMux);auto c=continuousApConfig;portEXIT_CRITICAL(&continuousApMux);return c;}
static inline ContApStatus continuousApSnapshot(){portENTER_CRITICAL(&continuousApMux);auto s=continuousApFsm.status();portEXIT_CRITICAL(&continuousApMux);return s;}
static inline ContApInputs continuousApInputsLocked(uint32_t now,uint32_t e,bool nagBusy){
  ContApInputs i;i.nowMs=now;i.epoch=e;i.generation=continuousApGeneration;i.profile=activeVehicleProfile;i.topology=activeVehicleTopology;i.config=continuousApConfig;
  i.ap=continuousApAp;i.turn=continuousApTurn;i.brake=continuousApBrake;
  i.gear=continuousApFreshPure(continuousApGear118,i,1000)?continuousApGear118:continuousApGear186;
  if(i.topology==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS){
    const bool s=continuousApFreshPure(continuousApTorqueS,i,250),p=continuousApFreshPure(continuousApTorqueP,i,250);
    i.torque=s?continuousApTorqueS:continuousApTorqueP;
    if(s&&p&&((continuousApTorqueP.value<0?-continuousApTorqueP.value:continuousApTorqueP.value)>(i.torque.value<0?-i.torque.value:i.torque.value)))i.torque=continuousApTorqueP;
    if((continuousApTorqueS.seen&&!continuousApTorqueS.valid&&continuousApTorqueS.epoch==e&&(uint32_t)(now-continuousApTorqueS.rxMs)<=250)||(continuousApTorqueP.seen&&!continuousApTorqueP.valid&&continuousApTorqueP.epoch==e&&(uint32_t)(now-continuousApTorqueP.rxMs)<=250))i.torque.valid=false;
  } else i.torque=continuousApTorqueParty;
  const auto status=continuousApFsm.status();
  const auto method=status.releaseOwed?continuousApFsm.inputMethod():i.config.method;
  const auto &stock=method==ContApMethod::StalkDouble?continuousApStalkStock:continuousApScrollStock;
  i.control=method==ContApMethod::StalkDouble?continuousApStalkControl:continuousApScrollControl;
  i.templateFresh=stock.valid&&stock.epoch==e&&(uint32_t)(now-stock.rxMs)<=(method==ContApMethod::StalkDouble?500u:250u);
  i.transportAllowed=!canTxAdministrativeHold&&!canUsbTxHeld()&&mcpReady&&twaiReady;
  i.ownershipAllowed=!nagBusy;
  i.ambiguous=continuousApEchoAmbiguous;
  return i;
}
static inline void continuousApObserveStock(uint8_t bus,uint16_t id,const uint8_t *data,uint8_t dlc,uint32_t e,uint32_t now){
  if(!data||dlc>8||activeVehicleProfile==VEHICLE_MODEL_YL||!vehicleProfileTopologyValid(activeVehicleProfile,activeVehicleTopology))return;
  ContApStockFrame f;f.route=bus==1?ContApRoute::ChassisB:(activeVehicleTopology==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS?ContApRoute::BodyA:ContApRoute::PartyA);
  f.valid=true;f.id=id;f.dlc=dlc;f.epoch=e;f.rxMs=now;memcpy(f.data,data,dlc);
  // Controls are accepted only on the gesture's selected route.
  if(id==0x3c2&&f.route!=continuousApGestureRoutePure(activeVehicleProfile,activeVehicleTopology,ContApMethod::ScrollSingle))return;
  ContApDecodedInputs d;if(!continuousApDecodeFramePure(f,d))return;
  portENTER_CRITICAL(&continuousApMux);
  const bool echo=continuousApLastTx.valid&&continuousApLastTx.epoch==e&&continuousApLastTx.route==f.route&&continuousApLastTx.id==id&&continuousApLastTx.dlc==dlc&&(uint32_t)(now-continuousApLastTxMs)<=100&&memcmp(continuousApLastTx.data,data,dlc)==0;
  if(echo&&(d.signal==ContApSignal::Stalk||d.signal==ContApSignal::Scroll)){
    // Neutral equality is not confirmation of release. Press equality is ambiguous.
    if(!d.idle)continuousApEchoAmbiguous=true;
    portEXIT_CRITICAL(&continuousApMux);return;
  }
  ContApObservation sample{true,d.valid,d.value,now,e};
  switch(d.signal){
    case ContApSignal::Ap:continuousApAp=sample;break;
    case ContApSignal::Turn:continuousApTurn=sample;break;
    case ContApSignal::Brake:continuousApBrake=sample;break;
    case ContApSignal::Gear:if(id==0x118)continuousApGear118=sample;else continuousApGear186=sample;break;
    case ContApSignal::Torque:if(f.route==ContApRoute::PartyA)continuousApTorqueParty=sample;else if(id==0x372)continuousApTorqueS=sample;else continuousApTorqueP=sample;break;
    case ContApSignal::Stalk:sample.value=d.idle?0:1;continuousApStalkControl=sample;continuousApStalkStock=f;continuousApStalkStock.valid=d.valid&&d.idle;continuousApEchoAmbiguous=false;break;
    case ContApSignal::Scroll:sample.value=d.idle?0:1;continuousApScrollControl=sample;continuousApScrollStock=f;continuousApScrollStock.valid=d.valid&&d.idle;continuousApEchoAmbiguous=false;break;
    default:break;
  }
  const bool manual=(d.signal==ContApSignal::Brake&&(!d.valid||d.value!=1))||((d.signal==ContApSignal::Stalk||d.signal==ContApSignal::Scroll)&&(!d.valid||!d.idle));
  const auto state=continuousApFsm.status().state;
  const bool inEpisode=state!=ContApState::Idle;
  const bool watch=inEpisode||continuousApFsm.hasArmedActive();
  const bool apStop=d.signal==ContApSignal::Ap&&(!d.valid||d.value<2||d.value>6||(inEpisode&&d.value>=3));
  const bool gearStop=d.signal==ContApSignal::Gear&&(!d.valid||d.value!=4);
  const bool turnStop=d.signal==ContApSignal::Turn&&(!d.valid||(d.value&0xf0)!=0||((d.value&3)!=0&&(d.value&12)!=0)||(inEpisode&&state!=ContApState::WaitSignalOff&&d.value!=0));
  const bool torqueStop=d.signal==ContApSignal::Torque&&!d.valid;
  if(manual){continuousApManualSeen=true;continuousApManualMs=now;}
  // Preserve stop events across a drained RX batch; later stock cannot undo them.
  if(watch&&(manual||apStop||gearStop||turnStop||torqueStop))continuousApCancelLatched=true;
  portEXIT_CRITICAL(&continuousApMux);
}
static inline void continuousApResetForEpoch(uint32_t e){
  portENTER_CRITICAL(&continuousApMux);
  ++continuousApGeneration;continuousApFsm.reset(e);
  continuousApAp={};continuousApTurn={};continuousApBrake={};continuousApGear118={};continuousApGear186={};
  continuousApTorqueS={};continuousApTorqueP={};continuousApTorqueParty={};continuousApStalkControl={};continuousApScrollControl={};
  continuousApStalkStock={};continuousApScrollStock={};continuousApLastTx={};continuousApCancelLatched=continuousApEchoAmbiguous=continuousApManualSeen=false;
  portEXIT_CRITICAL(&continuousApMux);
}
struct ContinuousApTxContext {ContApCommand command;ContApStockFrame frame;};
static inline bool continuousApComposeValidated(ContinuousApTxContext &ctx){
  const auto now=(uint32_t)millis();const bool busy=continuousApNagBusy();
  portENTER_CRITICAL(&continuousApMux);
  const auto i=continuousApInputsLocked(now,canTxBarrierState.epoch,busy);
  bool ok=(!continuousApCancelLatched||ctx.command.kind==ContApCommandKind::Release)&&continuousApFsm.allowsCommand(ctx.command,i);
  if(ok&&ctx.command.method==ContApMethod::StalkDouble){
    const uint8_t counter=ctx.command.phase==0&&ctx.command.kind==ContApCommandKind::Press?(continuousApStalkStock.data[1]+1)&15:continuousApStalkCounter;
    ok=continuousApStalkCrcValidPure(continuousApStalkStock)&&continuousApBuildStalkPure(ctx.command.kind==ContApCommandKind::Press?4:0,counter,ctx.frame);
  } else if(ok)ok=continuousApBuildClickPure(continuousApScrollStock,ctx.command.kind==ContApCommandKind::Press?2:1,ctx.frame);
  if(ok){ctx.frame.epoch=ctx.command.epoch;ctx.frame.rxMs=now;ctx.frame.route=ctx.command.route;}
  portEXIT_CRITICAL(&continuousApMux);return ok;
}
static inline bool continuousApValidateMcp(struct can_frame *out,void *arg){auto &ctx=*(ContinuousApTxContext*)arg;if(!continuousApComposeValidated(ctx))return false;out->can_id=ctx.frame.id;out->can_dlc=ctx.frame.dlc;memcpy(out->data,ctx.frame.data,8);return true;}
static inline bool continuousApValidateTwai(twai_message_t *out,void *arg){auto &ctx=*(ContinuousApTxContext*)arg;if(!continuousApComposeValidated(ctx))return false;out->identifier=ctx.frame.id;out->data_length_code=ctx.frame.dlc;out->flags=0;memcpy(out->data,ctx.frame.data,8);return true;}
static inline bool continuousApOwnsInputRoute(ContApRoute route){
  if(continuousApNagBusy())return false;
  portENTER_CRITICAL(&continuousApMux);const bool owns=continuousApFsm.ownedRoute()==route;portEXIT_CRITICAL(&continuousApMux);return owns;
}
// Shared 0x3C2 composition runs under the TX barrier. Once Continuous AP has
// used this route, Nag cannot replay an independently cached click payload.
static inline bool continuousApComposeNag(ContApRoute route,uint8_t *data,void *arg){
  const uint32_t now=(uint32_t)millis(),e=canTxBarrierState.epoch;
  portENTER_CRITICAL(&continuousApMux);
  const bool selected=continuousApGestureRoutePure(activeVehicleProfile,activeVehicleTopology,ContApMethod::ScrollSingle)==route;
  const bool protectedInput=selected&&((continuousApConfig.enabled&&continuousApConfig.method!=ContApMethod::StalkDouble)||
      (continuousApLastTx.valid&&continuousApLastTx.route==route&&continuousApLastTx.id==0x3c2));
  bool ok=true;
  if(protectedInput){
    const auto &stock=continuousApScrollStock;const auto &control=continuousApScrollControl;
    ok=continuousApFsm.ownedRoute()==ContApRoute::None&&!continuousApEchoAmbiguous&&stock.valid&&stock.route==route&&stock.epoch==e&&
       (uint32_t)(now-stock.rxMs)<=250&&control.seen&&control.valid&&control.value==0&&control.epoch==e&&
       (!continuousApLastTx.valid||(int32_t)(control.rxMs-continuousApLastTxMs)>0)&&!canTxAdministrativeHold&&!canUsbTxHeld()&&mcpReady&&twaiReady;
    if(ok){memcpy(data,stock.data,8);tsl9InputApplyCommandPure(*(Tsl9InputCommandPure*)arg,data);}
  }
  portEXIT_CRITICAL(&continuousApMux);return ok;
}
static inline bool continuousApValidateNagCanA(struct can_frame *out,void *arg){return continuousApComposeNag(ContApRoute::BodyA,out->data,arg);}
static inline bool continuousApValidateNagCanB(twai_message_t *out,void *arg){return continuousApComposeNag(ContApRoute::ChassisB,out->data,arg);}

// Publish physical cancellation under the same barrier as enqueue admission.
static inline bool continuousApCommitCancel(uint32_t now){
  portENTER_CRITICAL(&continuousApMux);const bool pending=continuousApCancelLatched;portEXIT_CRITICAL(&continuousApMux);
  if(!pending)return true;
  if(!canTxBarrierMutex||xSemaphoreTake(canTxBarrierMutex,0)!=pdTRUE)return false;
  portENTER_CRITICAL(&continuousApMux);
  if(continuousApCancelLatched){++continuousApGeneration;continuousApFsm.cancel(continuousApGeneration,canTxBarrierState.epoch,now);continuousApCancelLatched=false;}
  portEXIT_CRITICAL(&continuousApMux);xSemaphoreGive(canTxBarrierMutex);return true;
}
static inline void continuousApService(ContApRoute route){
  const uint32_t now=(uint32_t)millis(),e=canTxEpochSnapshot();const bool busy=continuousApNagBusy();
  if(!e||!continuousApCommitCancel(now))return;
  portENTER_CRITICAL(&continuousApMux);
  auto wanted=continuousApFsm.ownedRoute();if(wanted==ContApRoute::None)wanted=continuousApGestureRoutePure(activeVehicleProfile,activeVehicleTopology,continuousApConfig.method);
  if(wanted==ContApRoute::None)wanted=ContApRoute::ChassisB;
  if(wanted!=route){portEXIT_CRITICAL(&continuousApMux);return;}
  auto in=continuousApInputsLocked(now,e,busy);
  if(continuousApManualSeen&&(uint32_t)(now-continuousApManualMs)<=750&&continuousApFsm.status().state==ContApState::Idle)in.ambiguous=true;
  const auto command=continuousApFsm.service(in);
  portEXIT_CRITICAL(&continuousApMux);
  if(command.kind==ContApCommandKind::None)return;
  ContinuousApTxContext ctx;ctx.command=command;bool accepted=false;
  if(command.route==ContApRoute::BodyA){struct can_frame out={};MCP2515::ERROR err=MCP2515::ERROR_FAIL;accepted=canTxMcpSendValidated(&out,e,continuousApValidateMcp,&ctx,err)&&err==MCP2515::ERROR_OK;}
  else if(command.route==ContApRoute::ChassisB){twai_message_t out={};accepted=canTxTwaiTransmitValidated(&out,e,CAN_TX_FRESH_BOTH,continuousApValidateTwai,&ctx)==ESP_OK;}
  portENTER_CRITICAL(&continuousApMux);
  if(accepted){continuousApLastTx=ctx.frame;continuousApLastTxMs=(uint32_t)millis();if(command.method==ContApMethod::StalkDouble)continuousApStalkCounter=(ctx.frame.data[1]+1)&15;}
  continuousApFsm.onTxResult(command,accepted,(uint32_t)millis());
  portEXIT_CRITICAL(&continuousApMux);
}

static inline bool continuousApControlApply(ContApConfig next){
  ContApConfig checked;
  if(!continuousApConfigDecodePure(continuousApConfigEncodePure(next),checked)||checked.enabled!=next.enabled||checked.method!=next.method)return false;
  if(next.enabled&&continuousApGestureRoutePure(activeVehicleProfile,activeVehicleTopology,next.method)==ContApRoute::None)return false;
  if(!canTxBarrierMutex||xSemaphoreTake(canTxBarrierMutex,portMAX_DELAY)!=pdTRUE)return false;
  Preferences prefs;bool ok=prefs.begin("features",false);
  if(ok){ok=prefs.putUInt("contApCfg",continuousApConfigEncodePure(next))==sizeof(uint32_t);prefs.end();}
  if(ok){
    portENTER_CRITICAL(&continuousApMux);
    continuousApConfig=next;++continuousApGeneration;
    continuousApFsm.cancel(continuousApGeneration,canTxBarrierState.epoch,(uint32_t)millis());
    portEXIT_CRITICAL(&continuousApMux);
  }
  xSemaphoreGive(canTxBarrierMutex);return ok;
}
// Called at boot before CAN tasks; the features namespace also owns factory reset.
static inline void continuousApControlLoad(){
  ContApConfig loaded;Preferences prefs;
  if(prefs.begin("features",true)){continuousApConfigDecodePure(prefs.getUInt("contApCfg",0x100u),loaded);prefs.end();}
  portENTER_CRITICAL(&continuousApMux);continuousApConfig=loaded;++continuousApGeneration;portEXIT_CRITICAL(&continuousApMux);
}
