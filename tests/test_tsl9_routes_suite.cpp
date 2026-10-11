// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include "../vehicle_profile.h"
#include <cstdint>
#include "../tsl9_hands_on_0x399_pure.h"
#include <assert.h>
#include <stdint.h>

// Coverage origin: test_legacy_y_body_scroll_route.cpp
namespace case_legacy_y_body_scroll_route {
int run() {
  const uint8_t standardProfiles[] = {
      VEHICLE_MODEL_Y_JUNIPER, VEHICLE_MODEL_Y_LEGACY,
      VEHICLE_MODEL_3_HIGHLAND, VEHICLE_MODEL_3_LEGACY};
  for (const uint8_t id : standardProfiles) {
    assert(vehicleProfileTsl9InputOnBodyCanA(
        id, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(!vehicleProfileTsl9InputOnBodyCanA(
        id, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
  }
  assert(!vehicleProfileTsl9InputOnBodyCanA(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  return 0;
}
} // namespace case_legacy_y_body_scroll_route

// Coverage origin: test_tsl9_legacy_body_route_pure.cpp
namespace case_tsl9_legacy_body_route_pure {
int run() {
  assert(vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_Y_LEGACY, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  assert(vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_3_LEGACY, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));

  // Working 0x399 profiles must not be moved to the Legacy Body route.
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_Y_JUNIPER, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_3_HIGHLAND, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_Y_LEGACY, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
  assert(!vehicleProfileNagTsl9OnBody39B(
      VEHICLE_MODEL_3_LEGACY, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));

  // Captured 2024 Legacy Model Y Body 0x39B AP-active/Hands-On=2 sample.
  uint8_t bodyFrame[8] = {
      0x05u, 0x1Au, 0xDFu, 0x80u, 0xB0u, 0xC8u, 0x71u, 0x05u};
  Tsl9HandsOnStatePure bodyState = {};
  const Tsl9HandsOnResultPure bodyResult =
      tsl9ApplyHandsOnDowngradeForCanIdPure(
          bodyState, true, TSL9_SEQUENCE_EXTENDED_PURE,
          TSL9_DOWNGRADE_AP_SESSION_PURE, 0x39Bu,
          bodyFrame, 8u, 1000u);
  assert(bodyResult.modified);
  assert(bodyFrame[5] == 0xC4u);
  assert(bodyFrame[6] == 0x81u);
  assert(bodyFrame[7] == 0x11u);
  assert(bodyFrame[7] == tsl9ChecksumForCanIdPure(0x39Bu, bodyFrame));

  // The identical payload needs a different checksum on source ID 0x399.
  assert(tsl9ChecksumForCanIdPure(0x399u, bodyFrame) == 0x0Fu);
  return 0;
  return 0;
}
} // namespace case_tsl9_legacy_body_route_pure

// Coverage origin: test_tsl9_profile_capability_pure.cpp
namespace case_tsl9_profile_capability_pure {
int run() {
  assert(vehicleProfileNagTorqueSupported(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  assert(vehicleProfileNagTsl9Supported(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));

  const uint8_t standardProfiles[] = {
      VEHICLE_MODEL_Y_JUNIPER,
      VEHICLE_MODEL_Y_LEGACY,
      VEHICLE_MODEL_3_HIGHLAND,
      VEHICLE_MODEL_3_LEGACY,
  };
  for (const uint8_t id : standardProfiles) {
    assert(vehicleProfileNagTorqueSupported(
        id, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
    assert(!vehicleProfileNagTsl9Supported(
        id, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
    assert(vehicleProfileNagSupported(
        id, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));

    assert(!vehicleProfileNagTorqueSupported(
        id, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(vehicleProfileNagTsl9Supported(
        id, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(vehicleProfileNagSupported(
        id, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  }

  assert(!vehicleProfileNagSupported(
      VEHICLE_PROFILE_NONE, VEHICLE_TOPOLOGY_NONE));
  return 0;
  return 0;
}
} // namespace case_tsl9_profile_capability_pure

// Coverage origin: test_v390_tsl9_legacy_route_pure.cpp
namespace case_tsl9_legacy_route_pure {
int run() {
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
  return 0;
}
} // namespace case_tsl9_legacy_route_pure

int main() {
  if (case_legacy_y_body_scroll_route::run()) return 1;
  if (case_tsl9_legacy_body_route_pure::run()) return 1;
  if (case_tsl9_profile_capability_pure::run()) return 1;
  if (case_tsl9_legacy_route_pure::run()) return 1;
  return 0;
}
