#include <cassert>
#include <cstdint>
#include <cstring>

#include "../ulc_compositor_pure.h"

static void assertOnlyBits2And3Changed(const uint8_t *before,
                                      const uint8_t *after) {
  assert((uint8_t)(before[0] & 0xF3u) == (uint8_t)(after[0] & 0xF3u));
  assert(std::memcmp(before + 1, after + 1, 7) == 0);
}

int main() {
  for (uint8_t selectedValue = 0; selectedValue <= 3; ++selectedValue) {
    uint8_t frame[8] = {0xA3, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE};
    uint8_t original[8] = {};
    std::memcpy(original, frame, sizeof(frame));
    UlcCompositeSelectionPure selected = {};
    selected.summonHeartbeatOverrideEnabled = true;
    selected.summonHeartbeatValue = selectedValue;
    UlcCompositeGatesPure gates = {};
    gates.summonHeartbeatOverrideOpen = true;

    const auto result = ulcCompose3f8Pure(frame, 8, selected, gates);
    assert(result.changed);
    assert(result.summonHeartbeatApplied);
    assert(((frame[0] >> 2) & 0x03u) == selectedValue);
    assertOnlyBits2And3Changed(original, frame);
  }

  // An enabled override is applied to every valid stock frame, including when
  // its raw field already equals the selected value.
  uint8_t same[8] = {0xAC, 1, 2, 3, 4, 5, 6, 7};
  UlcCompositeSelectionPure selected = {};
  selected.summonHeartbeatOverrideEnabled = true;
  selected.summonHeartbeatValue = 3;
  UlcCompositeGatesPure gates = {};
  gates.summonHeartbeatOverrideOpen = true;
  const auto sameResult = ulcCompose3f8Pure(same, 8, selected, gates);
  assert(sameResult.changed);
  assert(sameResult.summonHeartbeatApplied);

  // Confirm-Free bit 1 and heartbeat bits 2-3 compose independently.
  uint8_t combined[8] = {0x0E, 0, 0, 0, 0, 0, 0, 0};
  selected.confirmFreeEnabled = true;
  selected.summonHeartbeatValue = 1;
  gates.confirmFreeOpen = true;
  const auto combinedResult = ulcCompose3f8Pure(combined, 8, selected, gates);
  assert(combinedResult.confirmFreeChanged);
  assert(combinedResult.summonHeartbeatApplied);
  assert((combined[0] & 0x02u) == 0u);
  assert(((combined[0] >> 2) & 0x03u) == 1u);

  // Disabled/closed/invalid requests leave all bytes untouched.
  uint8_t blocked[8] = {0x5A, 1, 2, 3, 4, 5, 6, 7};
  uint8_t original[8] = {};
  std::memcpy(original, blocked, sizeof(blocked));
  selected = {};
  selected.summonHeartbeatOverrideEnabled = true;
  selected.summonHeartbeatValue = 2;
  gates = {};
  assert(!ulcCompose3f8Pure(blocked, 8, selected, gates).changed);
  assert(std::memcmp(blocked, original, sizeof(blocked)) == 0);
  gates.summonHeartbeatOverrideOpen = true;
  selected.summonHeartbeatValue = 4;
  assert(!ulcCompose3f8Pure(blocked, 8, selected, gates).changed);
  assert(std::memcmp(blocked, original, sizeof(blocked)) == 0);
  selected.summonHeartbeatValue = 2;
  assert(!ulcCompose3f8Pure(blocked, 7, selected, gates).changed);
  assert(std::memcmp(blocked, original, sizeof(blocked)) == 0);
  return 0;
}
