#include <cassert>
#include <cstdint>
#include "driver_monitor_capture_pure.h"

int main() {
  using namespace driver_monitor_capture_pure;
  const uint16_t ids[] = {0x389, 0x5D9, 0x247, 0x399, 0x370};
  for (uint16_t id : ids) {
    assert(targetFrame(BUS_A, id));
    assert(targetFrame(BUS_B, id));
  }
  assert(!targetFrame(BUS_A, 0x118));
  assert(!targetFrame(BUS_B, 0x118));
  assert(!targetFrame(2, 0x389));
  return 0;
}
