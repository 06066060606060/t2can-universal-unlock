#include <cassert>

#include "../summon_state_pure.h"

int main() {
  assert(summonTxPriorityStatePure(false, TESLA_GEAR_INVALID, false, false) ==
         SUMMON_PRIORITY_NORMAL);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_D, false, false) ==
         SUMMON_PRIORITY_NORMAL);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_P, false, false) ==
         SUMMON_PRIORITY_PARK_STANDBY);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_P, true, false) ==
         SUMMON_PRIORITY_READY);
  assert(summonTxPriorityStatePure(false, TESLA_GEAR_INVALID, false, true) ==
         SUMMON_PRIORITY_ACTIVE);

  assert(!summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_NORMAL));
  assert(!summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_PARK_STANDBY));
  assert(summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_READY));
  assert(summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_ACTIVE));

  assert(summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_PARK_STANDBY, 13u, 14u, 6u));
  assert(!summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_PARK_STANDBY, 14u, 14u, 6u));
  assert(summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_READY, 5u, 14u, 6u));
  assert(!summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_READY, 6u, 14u, 6u));

  assert(r79RetryDelayMsPure(0u) == 5u);
  assert(r79RetryDelayMsPure(1u) == 15u);
  assert(r79RetryDelayMsPure(2u) == 30u);
  assert(r79RetryDelayMsPure(3u) == 0u);
  return 0;
}
