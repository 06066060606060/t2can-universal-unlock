#include <cassert>
#include <cstdint>

#include "../stalk_frame_pure.h"

int main() {
  const uint8_t original[4] = {0xAA, 0xB3, 0xC4, 0x5D};
  const uint8_t stock[4] = {0xAA, 0xB3, 0xC4, 0x5D};
  uint8_t out[4] = {};

  assert(stalkFramePreparePure(stock, 4, 5, 6, out));
  const uint8_t expectedLeft[4] = {0xC8, 0xB5, 0xC6, 0x5D};
  for (uint8_t i = 0; i < 4; ++i) assert(out[i] == expectedLeft[i]);

  assert(stalkFramePreparePure(stock, 4, 5, 2, out));
  const uint8_t expectedRight[4] = {0xF0, 0xB5, 0xC2, 0x5D};
  for (uint8_t i = 0; i < 4; ++i) assert(out[i] == expectedRight[i]);

  assert(!stalkFramePreparePure(nullptr, 4, 5, 6, out));
  assert(!stalkFramePreparePure(stock, 3, 5, 6, out));
  assert(!stalkFramePreparePure(stock, 4, 5, 6, nullptr));
  for (uint8_t i = 0; i < 4; ++i) assert(stock[i] == original[i]);
  return 0;
}
