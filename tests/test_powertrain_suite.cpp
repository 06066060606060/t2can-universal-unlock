// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include "pedal_map_session_pure.h"
#include "vehicle_profile.h"

// Coverage origin: test_ap_drive_profile_pure.cpp
namespace case_ap_drive_profile_pure {
int run() {
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
  return 0;
}
} // namespace case_ap_drive_profile_pure

// Coverage origin: test_pedal_map_session.cpp
namespace case_pedal_map_session {
int run() {
  PedalMapSessionPure s = pedalMapSessionInitialPure();
  assert(!s.active);
  assert(s.originRaw == PEDAL_MAP_RAW_STOCK);
  assert(s.targetRaw == PEDAL_MAP_RAW_STOCK);

  // A session remembers the pre-override Tesla value and keeps the requested
  // target latched until explicit STOCK/Park, even if stock later echoes target.
  assert(pedalMapSessionSetTargetPure(s, 1, 2));
  assert(s.active && s.originRaw == 1 && s.targetRaw == 2);
  pedalMapSessionObserveStockPure(s, 2);
  assert(s.active && s.originRaw == 1 && s.targetRaw == 2);

  // Existing Chill <-> Sport button behavior remains. Performance falls back
  // to Chill on the next legacy toggle, matching the previous implementation.
  assert(pedalMapToggleTargetPure(s, 2) == 0);
  assert(pedalMapSessionSetTargetPure(s, 2, 0));
  assert(s.active && s.originRaw == 1 && s.targetRaw == 0);
  assert(pedalMapToggleTargetPure(s, 2) == 1);

  // Stock observations do not silently release drive-session ownership. The
  // requested target remains armed until explicit STOCK or confirmed Park.
  pedalMapSessionObserveStockPure(s, 2);
  assert(s.active && s.targetRaw == 0);

  // PERFORMANCE can be selected again. Only an actual non-P/unknown -> P
  // transition clears the volatile session; repeated P frames do not. This
  // allows LAB selection while already parked, while still ending a drive
  // session when the vehicle enters Park.
  assert(pedalMapSessionSetTargetPure(s, 1, 2));
  pedalMapSessionOnGearTransitionPure(s, 1, 1);
  assert(s.active);
  pedalMapSessionOnGearTransitionPure(s, 0, 1);
  assert(!s.active);
  assert(s.originRaw == PEDAL_MAP_RAW_STOCK);
  assert(s.targetRaw == PEDAL_MAP_RAW_STOCK);

  // STOCK is an explicit immediate release command.
  assert(pedalMapSessionSetTargetPure(s, 0, 2));
  assert(pedalMapSessionSetTargetPure(s, 0, PEDAL_MAP_RAW_STOCK));
  assert(!s.active);

  assert(!pedalMapSessionSetTargetPure(s, 0, 3));
  return 0;
  return 0;
}
} // namespace case_pedal_map_session

int main() {
  if (case_ap_drive_profile_pure::run()) return 1;
  if (case_pedal_map_session::run()) return 1;
  return 0;
}
