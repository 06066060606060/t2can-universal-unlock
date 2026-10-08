#include <cassert>
#include <cstdint>
#include <initializer_list>
#include "ap_drive_profile_pure.h"
#include "vehicle_profile.h"

int main() {
  assert(AP_DRIVE_REGEN_STANDARD_RAW == 20);
  assert(AP_DRIVE_REGEN_REDUCED_RAW == 10);
  assert(AP_DRIVE_REGEN_MINIMAL_RAW == 1);
  assert(apDriveRegenRawSelectablePure(AP_DRIVE_REGEN_STANDARD_RAW));
  assert(apDriveRegenRawSelectablePure(AP_DRIVE_REGEN_REDUCED_RAW));
  assert(apDriveRegenRawSelectablePure(AP_DRIVE_REGEN_MINIMAL_RAW));
  assert(!apDriveRegenRawSelectablePure(0));
  assert(!apDriveRegenRawSelectablePure(2));
  assert(!apDriveRegenRawSelectablePure(255));
  assert(apDriveRegenNamePure(20)[0] == 'S');
  assert(apDriveRegenNamePure(10)[0] == 'R');
  assert(apDriveRegenNamePure(1)[0] == 'M');

  assert(vehicleProfileApDriveProfileSupported(VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  for (uint8_t model : {uint8_t(VEHICLE_MODEL_Y_JUNIPER), uint8_t(VEHICLE_MODEL_Y_LEGACY),
                        uint8_t(VEHICLE_MODEL_3_HIGHLAND), uint8_t(VEHICLE_MODEL_3_LEGACY)}) {
    assert(vehicleProfileApDriveProfileSupported(model, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
    assert(!vehicleProfileApDriveProfileSupported(model, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
  }
  return 0;
}
