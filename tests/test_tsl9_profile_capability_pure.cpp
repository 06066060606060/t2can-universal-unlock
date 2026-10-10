#include <cassert>
#include <cstdint>

#include "../vehicle_profile.h"

int main() {
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
}
