#include <cassert>
#include <initializer_list>
#include "summon_state_pure.h"

int main() {
  {
    const auto r = summonRoutePure(VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH);
    assert(r.valid);
    assert(r.gearBusMask == SUMMON_BUS_A);
    assert(r.dasBusMask == SUMMON_BUS_A);
    assert(r.sprBusMask == SUMMON_BUS_B);
    assert(r.transportBusMask == SUMMON_BUS_B);
    assert(r.requiredTxFreshMask == SUMMON_BUS_B);
    assert(!r.allow186Fallback);
  }
  for (uint8_t topology : {VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS,
                           VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS}) {
    const auto r = summonRoutePure(VEHICLE_MODEL_Y_JUNIPER, topology);
    assert(r.valid);
    assert(r.gearBusMask == SUMMON_BUS_B);
    assert(r.dasBusMask == SUMMON_BUS_B);
    assert(r.sprBusMask == SUMMON_BUS_B);
    assert(r.transportBusMask == SUMMON_BUS_B);
    assert(r.requiredTxFreshMask == SUMMON_BUS_B);
    assert(r.allow186Fallback);
  }
  return 0;
}
