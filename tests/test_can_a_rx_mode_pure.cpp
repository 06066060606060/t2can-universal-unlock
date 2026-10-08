#include <cassert>
#include "can_a_rx_mode_pure.h"

int main() {
  // LAB state and every legacy/corrupt persisted value cannot re-enable batching.
  for (unsigned mode = 0; mode < 256; ++mode) {
    assert(canARxReadBatchBudgetPure(false, mode) == 1);
    assert(canARxReadBatchBudgetPure(true, mode) == 1);
  }
}
