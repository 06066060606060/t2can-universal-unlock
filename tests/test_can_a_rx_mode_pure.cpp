#include <cassert>
#include <type_traits>
#include "runtime_gate_pure.h"
static_assert(std::is_same<decltype(&canARxReadBatchBudgetPure), uint8_t (*)()>::value,
              "RX policy must not accept retired persisted settings");
static_assert(canARxReadBatchBudgetPure() == 1u, "each frame is processed before the next read");
int main() { assert(canARxReadBatchBudgetPure() == CAN_A_RX_IMMEDIATE); }
