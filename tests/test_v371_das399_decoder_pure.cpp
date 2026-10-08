#include <cassert>
#include <cstdint>

#include "das_status_pure.h"
#include "nag_human_v4_pure.h"
#include "runtime_gate_pure.h"

static DasStatus399Pure decode(uint8_t apState, uint8_t handsOnByte) {
  const uint8_t data[8] = {apState, 0x06, 0xFF, 0x80,
                           0xB0, handsOnByte, 0x60, 0x78};
  return dasStatus399DecodePure(data, 8u);
}

int main() {
  const uint8_t captured[8] = {0x03, 0x06, 0xFF, 0x80,
                               0xB0, 0x44, 0x60, 0x78};
  const DasStatus399Pure actual = dasStatus399DecodePure(captured, 8u);
  assert(actual.valid);
  assert(actual.apState == 3u);
  assert(actual.handsOnState == 1u);
  assert(dasStateApActivePure(actual.valid, actual.apState));
  assert(!nagHumanV4VisualWarningActivePure(actual.handsOnState));

  const DasStatus399Pure noa = decode(0x05u, 0x0Cu);
  assert(noa.valid && noa.apState == 5u && noa.handsOnState == 3u);
  assert(dasStateApActivePure(noa.valid, noa.apState));
  assert(dasStateNoaPure(noa.valid, noa.apState));
  assert(nagHumanV4VisualWarningEdgePure(1u, noa.handsOnState));

  const DasStatus399Pure fault = decode(0x0Eu, 0x04u);
  assert(fault.valid && fault.apState == 14u && fault.handsOnState == 1u);
  assert(!dasStateApActivePure(fault.valid, fault.apState));

  const DasStatus399Pure restricted = decode(0x06u, 0x14u);
  assert(restricted.valid && restricted.apState == 6u &&
         restricted.handsOnState == 5u);
  assert(dasStateApActivePure(restricted.valid, restricted.apState));
  assert(!nagHumanV4VisualWarningEdgePure(3u, 4u));
  assert(!nagHumanV4VisualWarningEdgePure(4u, 5u));

  const DasStatus399Pure shortFrame = dasStatus399DecodePure(captured, 5u);
  assert(!shortFrame.valid);
  assert(shortFrame.apState == 0xFFu);
  assert(shortFrame.handsOnState == 0xFFu);
  assert(!dasStateApActivePure(shortFrame.valid, shortFrame.apState));

  uint8_t selectedAp = 9u;
  uint8_t selectedHandsOn = 9u;
  assert(dasStatus399SelectorDecodePure(
      captured, 8u,
      DAS399_AP_STATE_BYTE_PURE, DAS399_AP_STATE_SHIFT_PURE,
      DAS399_AP_STATE_MASK_PURE, DAS399_HANDS_ON_BYTE_PURE,
      DAS399_HANDS_ON_SHIFT_PURE, DAS399_HANDS_ON_MASK_PURE,
      selectedAp, selectedHandsOn));
  assert(selectedAp == 3u);
  assert(selectedHandsOn == 1u);

  selectedAp = 9u;
  selectedHandsOn = 9u;
  assert(!dasStatus399SelectorDecodePure(
      captured, 5u,
      DAS399_AP_STATE_BYTE_PURE, DAS399_AP_STATE_SHIFT_PURE,
      DAS399_AP_STATE_MASK_PURE, DAS399_HANDS_ON_BYTE_PURE,
      DAS399_HANDS_ON_SHIFT_PURE, DAS399_HANDS_ON_MASK_PURE,
      selectedAp, selectedHandsOn));
  assert(selectedAp == 9u);
  assert(selectedHandsOn == 9u);

  return 0;
}
