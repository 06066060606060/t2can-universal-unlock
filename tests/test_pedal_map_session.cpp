#include <cassert>
#include <cstdint>
#include "pedal_map_session_pure.h"

int main() {
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
}
