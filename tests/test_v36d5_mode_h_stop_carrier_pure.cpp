#include <cassert>
#include "../nag_mode_h_variant_pure.h"

int main() {
  assert(nagModeHStopBehaviorValidPure(H_STOP_HARD_PAUSE));
  assert(nagModeHStopBehaviorValidPure(H_STOP_STOCK_CARRIER));
  assert(!nagModeHStopBehaviorValidPure(2));
  assert(!nagModeHUseStopCarrierPure(H_STOP_HARD_PAUSE, true));
  assert(!nagModeHUseStopCarrierPure(H_STOP_STOCK_CARRIER, false));
  assert(nagModeHUseStopCarrierPure(H_STOP_STOCK_CARRIER, true));
  return 0;
}
