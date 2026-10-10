#include <cassert>
#include <cstdint>
#include "can_research_capture_pure.h"

int main() {
  const uint16_t targets[] = {0x247, 0x3F8, 0x3E9, 0x24A, 0x3FD, 0x293};
  for (uint16_t id : targets) {
    assert(researchUlcConfirmTargetIdPure(id));
  }
  const uint16_t rejects[] = {0x000, 0x118, 0x239, 0x370, 0x389, 0x399, 0x5D9, 0x7FF};
  for (uint16_t id : rejects) {
    assert(!researchUlcConfirmTargetIdPure(id));
  }
  return 0;
}
