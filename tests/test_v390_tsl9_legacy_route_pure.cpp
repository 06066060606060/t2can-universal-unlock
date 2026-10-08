#include <assert.h>
#include <stdint.h>

#include "../tsl9_hands_on_0x399_pure.h"
#include "../vehicle_profile.h"

int main() {
  static_assert(TSL9_LEGACY_ROUTE_DEFAULT_PURE ==
                    TSL9_LEGACY_ROUTE_BODY_39B_PURE,
                "Legacy route defaults to Body 0x39B");
  assert(tsl9LegacyRouteSanitizePure(99u) ==
         TSL9_LEGACY_ROUTE_BODY_39B_PURE);

  const uint8_t legacyProfiles[] = {
      VEHICLE_MODEL_Y_LEGACY, VEHICLE_MODEL_3_LEGACY};
  for (uint8_t profile : legacyProfiles) {
    assert(vehicleProfileLegacyTsl9RouteSelectable(
        profile, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(vehicleProfileNagTsl9OnBody39B(
        profile, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS,
        TSL9_LEGACY_ROUTE_BODY_39B_PURE));
    assert(!vehicleProfileNagTsl9OnBody39B(
        profile, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS,
        TSL9_LEGACY_ROUTE_CHASSIS_399_PURE));
  }

  assert(!vehicleProfileLegacyTsl9RouteSelectable(
      VEHICLE_MODEL_Y_JUNIPER,
      VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_Y_JUNIPER,
      VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS,
      TSL9_LEGACY_ROUTE_BODY_39B_PURE));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH,
      TSL9_LEGACY_ROUTE_BODY_39B_PURE));
  return 0;
}
