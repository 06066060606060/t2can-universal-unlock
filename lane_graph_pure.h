#pragma once
#include <stdint.h>
#include "vehicle_profile.h"

static constexpr uint8_t LANE_GRAPH_OFF_PURE = 0u;
static constexpr uint8_t LANE_GRAPH_AP_ACTIVE_PURE = 1u;
static constexpr uint8_t LANE_GRAPH_ALWAYS_PURE = 2u;
static constexpr uint32_t LANE_GRAPH_AP_FRESH_MS_PURE = 3000u;
static inline bool laneGraphModeValidPure(uint8_t mode) { return mode <= 2u; }
static inline bool laneGraphActivePure(uint8_t mode, bool lab, bool supported,
                                      bool transportReady, bool apValid,
                                      bool apActive, uint32_t apAgeMs) {
  if (!lab || !supported || !transportReady || !laneGraphModeValidPure(mode)) return false;
  return mode == LANE_GRAPH_ALWAYS_PURE ||
      (mode == LANE_GRAPH_AP_ACTIVE_PURE && apValid && apActive &&
       apAgeMs <= LANE_GRAPH_AP_FRESH_MS_PURE);
}
// OFF/inactive leaves the original stock bit intact. MUX0 bit45 is unrelated.
static inline bool laneGraphApplyPure(uint8_t *data, uint8_t length, bool active) {
  if (!data || length != 8u || (data[0] & 7u) != 1u || !active) return false;
  const bool changed = (data[5] & 0x20u) == 0u;
  data[5] |= 0x20u;
  return changed;
}

static constexpr uint8_t LANE_GRAPH_CHASSIS_PURE = 0u;
static constexpr uint8_t LANE_GRAPH_BODY_PURE = 1u;
static inline bool laneGraphBusValidPure(uint8_t bus) { return bus <= 1u; }
static inline bool laneGraphBusSupportedPure(uint8_t profile, uint8_t topology, uint8_t bus) {
  return laneGraphBusValidPure(bus) && vehicleProfileTopologyValid(profile, topology) &&
      (bus == LANE_GRAPH_CHASSIS_PURE || vehicleProfileCanAIsBody(profile, topology));
}
struct LaneGraphStockPure {
  bool valid;
  uint8_t raw[8];
  uint32_t lastMs, epoch, rx;
  uint8_t profile, topology;
};
static inline bool laneGraphStockFreshPure(const LaneGraphStockPure &stock,
                                          uint32_t now, uint32_t epoch,
                                          uint8_t profile, uint8_t topology) {
  return stock.valid && stock.epoch == epoch && stock.profile == profile &&
      stock.topology == topology && (uint32_t)(now - stock.lastMs) <= LANE_GRAPH_AP_FRESH_MS_PURE;
}
