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
