#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../r79_ap_gate_pure.h"

static R79ApGateDecisionPure step(R79ApGateSessionPure &s, const R79ApGateConfigPure &c, uint32_t t, bool fresh = true, bool ap = true) {
  r79ApGateObservePure(s, c, t, fresh, ap);
  return r79ApGateDecisionPure(c, s, t, fresh, ap);
}

int main() {
  R79ApGateSessionPure s = {};
  R79ApGateConfigPure c = {true, 0, 2};
  assert(!step(s, c, 0).allowed); // Missing AP restriction must fail here.
  assert(step(s, c, 20000).reason == 2);
  assert(step(s, c, 20001, true, false).allowed);
  assert(step(s, c, 20002, true, false).reason == 1);
  assert(!step(s, c, 20003, false, false).allowed);
  assert(step(s, c, 20004, false, true).reason == 5);

  // Disabled means legacy behavior even with corrupt configuration or stale DAS.
  c = {false, 255, 0};
  auto d = step(s, c, 0, false, true);
  assert(d.allowed && d.reason == 0 && d.remainingMs == 0);

  // Zero is a real engagement timestamp. Exact boundary releases, never early.
  c = {true, 1, 2};
  s = {};
  d = step(s, c, 0);
  assert(!d.allowed && d.reason == 3 && d.remainingMs == 2000);
  d = step(s, c, 1999);
  assert(!d.allowed && d.remainingMs == 1);
  d = step(s, c, 2000);
  assert(d.allowed && d.reason == 4 && d.remainingMs == 0);
  // Released session stays released even after a whole millis epoch.
  assert(step(s, c, 0).allowed);

  // Disengagement and invalid evidence discard all accumulated waiting time.
  step(s, c, 2100, true, false);
  assert(!s.active);
  assert(step(s, c, 2200).remainingMs == 2000);
  step(s, c, 3000, false);
  assert(!s.active);
  assert(step(s, c, 3500).remainingMs == 2000);
  assert(!step(s, c, 5499).allowed);
  assert(step(s, c, 5500).allowed);
  s = {}; // Integration uses same reset on recovery/configuration changes.
  assert(!r79ApGateDecisionPure(c, s, 8000, true, true).allowed);

  // Unsigned elapsed time handles engagement immediately before wrap.
  s = {};
  assert(step(s, c, UINT32_MAX - 999u).remainingMs == 2000);
  assert(step(s, c, 999).remainingMs == 1);
  assert(step(s, c, 1000).allowed);

  // All supported integer delays and both bounds are enforced.
  for (uint8_t seconds = 2; seconds <= 10; ++seconds) {
    c = {true, 1, seconds}; s = {};
    step(s, c, 100);
    assert(!step(s, c, 100u + seconds * 1000u - 1u).allowed);
    assert(step(s, c, 100u + seconds * 1000u).allowed);
  }
  const R79ApGateConfigPure bad[] = {{true, 2, 2}, {true, 255, 2}, {true, 1, 0}, {true, 1, 1}, {true, 1, 11}, {true, 0, 255}};
  for (const auto &invalid : bad) {
    s = {};
    d = step(s, invalid, 0);
    assert(!d.allowed && d.reason == 5);
    assert(!step(s, invalid, 50000, true, false).allowed);
  }
  puts("R79 AP gate pure: 9 behavior groups passed");
}
