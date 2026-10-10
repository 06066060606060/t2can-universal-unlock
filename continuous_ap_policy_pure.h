#pragma once
#include "continuous_ap_types.h"
#include "vehicle_profile.h"
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
  if(!vehicleProfileTopologyValid(p,t)||p==VEHICLE_MODEL_YL) return ContApRoute::None;
  const bool legacy=p==VEHICLE_MODEL_Y_LEGACY||p==VEHICLE_MODEL_3_LEGACY;
  if(legacy) return t==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS && m==ContApMethod::StalkDouble
      ? ContApRoute::BodyA:ContApRoute::None;
  if(m!=ContApMethod::ScrollSingle&&m!=ContApMethod::ScrollDouble) return ContApRoute::None;
  return t==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS?ContApRoute::BodyA:ContApRoute::ChassisB;
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
  const uint32_t limits[]={1000,350,250,1000,250,i.config.method==ContApMethod::StalkDouble?500u:250u};
  bool missing=false,bad=false;
  for(unsigned n=0;n<6;++n){
    const char *state=continuousApRxStatusPure(observations[n],i.nowMs,i.epoch,limits[n]);
    missing|=state[0]=='M';bad|=state[0]!='F';
  }
  return missing?"Waiting for signals":bad?"Stale / Invalid":"Available";
}
