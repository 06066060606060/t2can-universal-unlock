#include <cassert>
#include "../vehicle_profile.h"

int main() {
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
}
