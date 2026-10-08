#include <cassert>
#include <cstdint>

#include "ap_display_hold_pure.h"

int main() {
  ApDisplayHoldPure hold = {};

  ApDisplayValuePure view = apDisplayResolvePure(hold, false, 0xFFu, false, 10u);
  assert(!view.valid);
  assert(view.state == 0xFFu);

  apDisplayObserveValidPure(hold, 3u);
  view = apDisplayResolvePure(hold, true, 3u, false, 100u);
  assert(view.valid);
  assert(view.state == 3u);

  apDisplayBeginHardInitPure(hold, 1000u);
  view = apDisplayResolvePure(hold, false, 0xFFu, true, 1001u);
  assert(view.valid);
  assert(view.state == 3u);

  view = apDisplayResolvePure(hold, false, 0xFFu, false, 9000u);
  assert(view.valid);
  assert(view.state == 3u);

  view = apDisplayResolvePure(hold, false, 0xFFu, false, 11001u);
  assert(!view.valid);
  assert(view.state == 0xFFu);

  apDisplayObserveValidPure(hold, 5u);
  view = apDisplayResolvePure(hold, true, 5u, false, 12000u);
  assert(view.valid);
  assert(view.state == 5u);

  view = apDisplayResolvePure(hold, false, 0xFFu, false, 12001u);
  assert(!view.valid);
  assert(view.state == 0xFFu);
  return 0;
}
