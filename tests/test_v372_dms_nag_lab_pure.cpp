#include <cassert>
#include <cstdint>
#include <cstring>

#include "../r79_dms_nag_lab_pure.h"

int main() {
  static_assert(R79_DMS_ENABLE_CABIN_CAMERA_BIT_PURE == 43u, "bit43 contract");
  const uint8_t stock[8] = {0x01u, 0x00u, 0x0Eu, 0x00u, 0x00u, 0x88u, 0x1Au, 0x82u};
  uint8_t out[8];

  std::memcpy(out, stock, sizeof(out));
  r79DmsNagLabApplyPure(out, false);
  assert(std::memcmp(out, stock, sizeof(out)) == 0);

  std::memcpy(out, stock, sizeof(out));
  r79DmsNagLabApplyPure(out, true);
  assert(((out[5] >> 3) & 1u) == 0u);
  for (uint8_t i = 0; i < 8u; ++i) {
    const uint8_t allowed = i == 5u ? (uint8_t)(1u << 3) : 0u;
    assert((((uint8_t)(stock[i] ^ out[i])) & (uint8_t)~allowed) == 0u);
  }
  return 0;
}
