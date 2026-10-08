#include <cassert>

#include "../ap_right_scroll_pure.h"

int main() {
  assert(!tsl9ScrollWarningStatePure(2u));
  assert(tsl9ScrollWarningStatePure(3u));
  assert(tsl9ScrollWarningStatePure(4u));
  assert(tsl9ScrollWarningStatePure(5u));
  assert(tsl9ScrollWarningStatePure(6u));
  assert(!tsl9ScrollWarningStatePure(7u));
  assert(!tsl9ScrollWarningStatePure(8u));
  assert(tsl9ScrollWarningStatePure(9u));
  assert(tsl9ScrollWarningStatePure(10u));
  assert(!tsl9ScrollWarningStatePure(11u));
  return 0;
}
