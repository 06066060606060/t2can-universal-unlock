#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../ulc_policy_pure.h"

int main() {
  uint8_t data[8] = {0x8B,0x28,0x88,0x00,0x19,0xBD,0x9D,0x24};
  assert(uiUlcOffHighwayReadRawPure(data, 8) == 0);

  assert(ulcPolicyApGateOpenPure(true, true, 3));
  assert(ulcPolicyApGateOpenPure(true, true, 4));
  assert(ulcPolicyApGateOpenPure(true, true, 5));
  assert(ulcPolicyApGateOpenPure(true, true, 6));
  assert(!ulcPolicyApGateOpenPure(true, true, 2));
  assert(!ulcPolicyApGateOpenPure(true, false, 5));
  assert(!ulcPolicyApGateOpenPure(false, true, 5));

  puts("ulc policy pure tests passed");
  return 0;
}
