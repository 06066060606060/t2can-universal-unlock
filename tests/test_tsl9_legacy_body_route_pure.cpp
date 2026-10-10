#include <cassert>
#include <cstdint>

#include "../tsl9_hands_on_0x399_pure.h"
#include "../vehicle_profile.h"

int main() {
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
}
