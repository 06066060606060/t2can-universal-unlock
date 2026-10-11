// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include "driver_monitor_capture_pure.h"
#include "../vehicle_profile.h"
#include <assert.h>
#include <stdint.h>
#include "../r79_dms_composition_pure.h"

// Coverage origin: test_v36b21_driver_monitor_all_profiles_pure.cpp
namespace case_driver_monitor_all_profiles_pure {
int run() {
  using namespace driver_monitor_capture_pure;
  const uint16_t ids[] = {0x389, 0x5D9, 0x247, 0x399, 0x370};
  for (uint16_t id : ids) {
    assert(targetFrame(BUS_A, id));
    assert(targetFrame(BUS_B, id));
  }
  assert(!targetFrame(BUS_A, 0x118));
  assert(!targetFrame(BUS_B, 0x118));
  assert(!targetFrame(2, 0x389));
  return 0;
  return 0;
}
} // namespace case_driver_monitor_all_profiles_pure

// Coverage origin: test_v372_dms_nag_profiles_pure.cpp
namespace case_dms_nag_profiles_pure {
int run() {
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
  return 0;
}
} // namespace case_dms_nag_profiles_pure

// Coverage origin: test_v390_r79_dms_composition_pure.cpp
namespace case_r79_dms_composition_pure {
int run() {
  static_assert(R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE == 43u,
                "production DMS policy owns bit43");
  assert(driverMonitoringDisablePolicyActivePure(true, true, true, true));
  assert(!driverMonitoringDisablePolicyActivePure(false, true, true, true));
  assert(!driverMonitoringDisablePolicyActivePure(true, false, true, true));
  assert(!driverMonitoringDisablePolicyActivePure(true, true, false, false));
  assert(!driverMonitoringDisablePolicyActivePure(true, true, true, false));

  uint8_t mux1[8] = {1u, 0u, 0u, 0u, 0u, 0x08u, 0u, 0u};
  assert(r79DmsApplyFinalOverlayPure(mux1, true));
  assert((mux1[5] & 0x08u) == 0u);
  assert(!r79DmsApplyFinalOverlayPure(mux1, true));
  uint8_t mux2[8] = {2u, 0u, 0u, 0u, 0u, 0x08u, 0u, 0u};
  assert(!r79DmsApplyFinalOverlayPure(mux2, true));
  assert((mux2[5] & 0x08u) != 0u);

  assert(r79DmsStockDispositionPure(true, true, true, false) ==
         R79_DMS_STOCK_R79_OWNED_PURE);
  assert(r79DmsStockDispositionPure(true, true, false, false) ==
         R79_DMS_STOCK_DMS_ONLY_PURE);
  assert(r79DmsStockDispositionPure(true, true, false, true) ==
         R79_DMS_STOCK_NONE_PURE);
  assert(r79DmsStockDispositionPure(true, false, false, false) ==
         R79_DMS_STOCK_NONE_PURE);
  assert(r79DmsStockDispositionPure(false, true, false, false) ==
         R79_DMS_STOCK_NONE_PURE);
  return 0;
  return 0;
}
} // namespace case_r79_dms_composition_pure

int main() {
  if (case_driver_monitor_all_profiles_pure::run()) return 1;
  if (case_dms_nag_profiles_pure::run()) return 1;
  if (case_r79_dms_composition_pure::run()) return 1;
  return 0;
}
