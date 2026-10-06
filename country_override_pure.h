#pragma once

#include <stddef.h>
#include <stdint.h>
#include "vehicle_profile.h"

static constexpr uint8_t COUNTRY_OVERRIDE_STOCK_PURE = 0;
static constexpr uint8_t COUNTRY_OVERRIDE_US_PURE = 1;
static constexpr uint8_t COUNTRY_OVERRIDE_KR_PURE = 2;
static constexpr uint8_t COUNTRY_OVERRIDE_NZ_PURE = 3;

static inline bool countryOverrideModeValidPure(uint8_t mode) {
  return mode == COUNTRY_OVERRIDE_STOCK_PURE || mode == COUNTRY_OVERRIDE_US_PURE ||
         mode == COUNTRY_OVERRIDE_KR_PURE || mode == COUNTRY_OVERRIDE_NZ_PURE;
}

static constexpr uint8_t MAP_REGION_STOCK_PURE = 0u;
static constexpr uint8_t MAP_REGION_US_PURE = 1u;
static constexpr uint8_t MAP_REGION_KR_PURE = 2u;
static inline bool mapRegionModeValidPure(uint8_t mode) { return mode <= 2u; }
static inline uint8_t mapRegionFromLegacyCountryPure(uint8_t country) {
  return country == COUNTRY_OVERRIDE_US_PURE ? MAP_REGION_US_PURE :
      country == COUNTRY_OVERRIDE_KR_PURE ? MAP_REGION_KR_PURE : MAP_REGION_STOCK_PURE;
}

static inline bool countryOverrideGateOpenPure(uint8_t mode,
                                               bool validProfileTopology,
                                               bool r79StateActive) {
  return countryOverrideModeValidPure(mode) &&
         mode != COUNTRY_OVERRIDE_STOCK_PURE && validProfileTopology && r79StateActive;
}

// CAN A = 0, CAN B = 1. Only transform a stock frame received on this route.
// UI_driverAssistMapData (0x238) uses Body/Chassis/VH routes, not Party.
// GTW_carConfig (0x7FF) has route-local templates on either connected bus.
static inline bool countryOverrideRouteAllowedPure(uint8_t profile,
                                                   uint8_t topology,
                                                   uint8_t bus,
                                                   uint32_t id) {
  if (bus > 1u || !vehicleProfileTopologyValid(profile, topology)) return false;
  if (id == 0x7FFu) return true;
  if (id == 0x238u) return bus == 1u || vehicleProfileCanAIsBody(profile, topology);
  return false;
}

static inline uint16_t countryOverrideRead238CountryPure(const uint8_t *data, uint8_t len) {
  if (data == nullptr || len < 4u) return 0u;
  return (uint16_t)data[2] | ((uint16_t)(data[3] & 0x03u) << 8);
}

static inline uint8_t countryOverride238ChecksumPure(const uint8_t *data) {
  if (data == nullptr) return 0u;
  uint16_t sum = 0x38u + 0x02u;
  for (uint8_t i = 0; i < 7u; ++i) sum += data[i];
  return (uint8_t)sum;
}

static inline bool countryOverrideApply238Pure(uint8_t *data, uint8_t len,
                                              uint8_t mode) {
  if (data == nullptr || len != 8u || !countryOverrideModeValidPure(mode) ||
      mode == COUNTRY_OVERRIDE_STOCK_PURE) return false;
  if (data[7] != countryOverride238ChecksumPure(data)) return false;
  const uint16_t source = countryOverrideRead238CountryPure(data, len);
  const uint16_t target = mode == COUNTRY_OVERRIDE_US_PURE ? 840u :
      mode == COUNTRY_OVERRIDE_NZ_PURE ? 554u : 410u;
  // Validate the three-digit wire domain, not ISO membership. Reject UNKNOWN
  // (0), SNA (1023), and values outside that domain before modifying any byte.
  if (source == 0u || source > 999u || source == target) return false;

  data[2] = (uint8_t)target;
  data[3] = (uint8_t)((data[3] & 0xFCu) | (target >> 8));

  const uint8_t nextCounter = (uint8_t)((((data[6] >> 4) & 0x0Fu) + 1u) & 0x0Fu);
  data[6] = (uint8_t)((data[6] & 0x0Fu) | (nextCounter << 4));
  data[7] = countryOverride238ChecksumPure(data);
  return true;
}

static inline bool countryOverrideApply7ffPure(uint8_t *data, uint8_t len,
                                              uint8_t mode, uint8_t mapMode = 0xFFu) {
  // Default preserves historical pure callers; production always passes the
  // independent map selection explicitly.
  if (mapMode == 0xFFu) mapMode = mapRegionFromLegacyCountryPure(mode);
  if (data == nullptr || len != 8u || !countryOverrideModeValidPure(mode) ||
      !mapRegionModeValidPure(mapMode)) return false;

  if (data[0] == 0x01u) {
    if (mode == COUNTRY_OVERRIDE_STOCK_PURE) return false;
    // Little-endian alpha-2 representation. Check ASCII format; this does not
    // claim full ISO membership validation. Unknown/SNA encodings fail closed.
    if (data[2] < 'A' || data[2] > 'Z' || data[3] < 'A' || data[3] > 'Z') return false;
    const uint8_t low = mode == COUNTRY_OVERRIDE_US_PURE ? 0x53u :
        mode == COUNTRY_OVERRIDE_NZ_PURE ? 0x5Au : 0x52u;
    const uint8_t high = mode == COUNTRY_OVERRIDE_US_PURE ? 0x55u :
        mode == COUNTRY_OVERRIDE_NZ_PURE ? 0x4Eu : 0x4Bu;
    if (data[2] == low && data[3] == high) return false;
    data[2] = low;
    data[3] = high;
    return true;
  }

  if (data[0] == 0x03u) {
    if (mapMode == MAP_REGION_STOCK_PURE) return false;
    const uint8_t source = data[1] & 0x0Fu;
    const uint8_t target = mapMode == MAP_REGION_US_PURE ? 0u : 7u;
    // GTW_mapRegion's DBC domain is 0..10; 11..15 are reserved/SNA.
    if (source > 10u || source == target) return false;
    data[1] = (uint8_t)((data[1] & 0xF0u) | target);
    return true;
  }

  return false;
}
