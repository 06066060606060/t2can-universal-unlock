#include <assert.h>
#include <stdint.h>

#include "../r79_dms_composition_pure.h"

int main() {
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
}
