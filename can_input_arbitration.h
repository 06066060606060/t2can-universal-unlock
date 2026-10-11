#pragma once

// Shared input arbitration. Included after both runtime state owners.
static inline bool continuousApOwnsInputRoute(ContApRoute route){
  if(continuousApNagBusy())return false;
  portENTER_CRITICAL(&continuousApMux);const bool owns=continuousApFsm.ownedRoute()==route;portEXIT_CRITICAL(&continuousApMux);return owns;
}
// Shared 0x3C2 composition runs under the TX barrier. Once Continuous AP has
// used this route, Nag cannot replay an independently cached click payload.
static inline bool continuousApComposeNag(ContApRoute route,uint8_t *data,void *arg){
  const uint32_t now=(uint32_t)millis(),e=canTxBarrierState.epoch;
  const auto &command=*(Tsl9InputCommandPure*)arg;
  portENTER_CRITICAL(&tsl9InputMux);
  const auto nag=tsl9InputScheduler.snapshot(now);
  portEXIT_CRITICAL(&tsl9InputMux);
  const bool pendingCenter=command.kind==TSL9_INPUT_COMMAND_CENTER_PURE&&
      nag.cleanupPending&&!nag.active&&command.mode==nag.mode&&
      command.leftTick==0&&command.rightTick==0;
  portENTER_CRITICAL(&continuousApMux);
  const bool selected=continuousApGestureRoutePure(activeVehicleProfile,activeVehicleTopology,ContApMethod::ScrollSingle)==route;
  const bool protectedInput=selected&&((continuousApConfig.enabled&&continuousApConfig.method!=ContApMethod::StalkDouble)||
      (continuousApLastTx.valid&&continuousApLastTx.route==route&&continuousApLastTx.id==0x3c2));
  bool ok=true;
  if(protectedInput){
    const auto &stock=continuousApScrollStock;const auto &control=continuousApScrollControl;
    const auto owned=continuousApFsm.ownedRoute();
    // Drain NAG's already-owed CENTER before a waiting Continuous AP gesture.
    // New NAG steps and cleanup over a prepared/accepted AP press stay blocked.
    const bool routeAvailable=owned==ContApRoute::None||
        (owned==route&&pendingCenter&&!continuousApFsm.hasPendingPress()&&!continuousApFsm.status().releaseOwed);
    ok=routeAvailable&&!continuousApEchoAmbiguous&&stock.valid&&stock.route==route&&stock.epoch==e&&
       (uint32_t)(now-stock.rxMs)<=CONT_AP_SCROLL_FRESH_MS&&control.seen&&control.valid&&control.value==0&&control.epoch==e&&
       (!continuousApLastTx.valid||(int32_t)(control.rxMs-continuousApLastTxMs)>0)&&!canTxAdministrativeHold&&!canUsbTxHeld()&&mcpReady&&twaiReady;
    if(ok){memcpy(data,stock.data,8);tsl9InputApplyCommandPure(command,data);}
  }
  portEXIT_CRITICAL(&continuousApMux);return ok;
}
static inline bool continuousApValidateNagCanA(struct can_frame *out,void *arg){return continuousApComposeNag(ContApRoute::BodyA,out->data,arg);}
static inline bool continuousApValidateNagCanB(twai_message_t *out,void *arg){return continuousApComposeNag(ContApRoute::ChassisB,out->data,arg);}
