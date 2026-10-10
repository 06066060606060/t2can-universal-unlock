#include <cassert>
#include <cstdint>
#include "../vehicle_profile.h"

int main() {
  // YL fixed Party+VH route.
  assert(vehicleProfileDmsNagSupported(VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));

  const uint8_t standardProfiles[] = {
      VEHICLE_MODEL_Y_JUNIPER,
      VEHICLE_MODEL_Y_LEGACY,
      VEHICLE_MODEL_3_HIGHLAND,
      VEHICLE_MODEL_3_LEGACY,
  };
  for (uint8_t id : standardProfiles) {
    assert(vehicleProfileDmsNagSupported(id, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(vehicleProfileDmsNagSupported(id, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
  }

  // Invalid/non-routed combinations stay blocked.
  assert(!vehicleProfileDmsNagSupported(VEHICLE_PROFILE_NONE, VEHICLE_TOPOLOGY_NONE));
  assert(!vehicleProfileDmsNagSupported(VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
  assert(!vehicleProfileDmsNagSupported(VEHICLE_MODEL_Y_JUNIPER, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  return 0;
}
