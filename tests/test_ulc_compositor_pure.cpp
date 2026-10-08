#include <cassert>
#include <cstdint>
#include <cstring>

#include "../ulc_compositor_pure.h"

int main() {
  uint8_t frame[8] = {0x02, 0x00, 0x44, 0x55, 0x66, 0x77, 0x00, 0x00};
  UlcCompositeSelectionPure selected = {};
  selected.alcOffHighwayEnabled = true;
  selected.ulcOffHighwayMode = 1;
  selected.blindSpotMode = 2;
  selected.confirmFreeEnabled = true;
  UlcCompositeGatesPure gates = {true, true, true, false};
  const UlcCompositeResultPure result =
      ulcCompose3f8Pure(frame, 8, selected, gates);
  assert(result.changed);
  assert(result.alcOffHighwayChanged);
  assert(result.ulcOffHighwayChanged);
  assert(result.blindSpotChanged);
  assert(result.confirmFreeChanged);
  assert((frame[7] & 0x01u) != 0);       // bit 56
  assert((frame[1] & 0x80u) != 0);       // bit 15
  assert((frame[6] & 0x30u) == 0x20u);   // bits 52-53
  assert((frame[0] & 0x02u) == 0);       // bit 1

  const UlcCompositeResultPure again =
      ulcCompose3f8Pure(frame, 8, selected, gates);
  assert(!again.changed);

  uint8_t invalidFrame[8] = {0x02, 0x00, 0x44, 0x55, 0x66, 0x77, 0x00, 0x00};
  uint8_t original[8] = {};
  std::memcpy(original, invalidFrame, sizeof(original));
  selected.blindSpotMode = 3;
  assert(!ulcCompose3f8Pure(invalidFrame, 8, selected, gates).changed);
  assert(std::memcmp(invalidFrame, original, sizeof(original)) == 0);
  assert(!ulcCompose3f8Pure(invalidFrame, 7, {}, gates).changed);
  assert(std::memcmp(invalidFrame, original, sizeof(original)) == 0);

  return 0;
}
