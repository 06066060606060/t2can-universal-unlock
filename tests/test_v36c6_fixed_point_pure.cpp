#include <cassert>
#include <cstdint>
#include <cstring>
#include "fixed_point_pure.h"

int main() {
  char buf[24] = {};

  assert(formatCentiPure(0, buf, sizeof(buf)));
  assert(std::strcmp(buf, "0.00") == 0);
  assert(formatCentiPure(165, buf, sizeof(buf)));
  assert(std::strcmp(buf, "1.65") == 0);
  assert(formatCentiPure(-2050, buf, sizeof(buf)));
  assert(std::strcmp(buf, "-20.50") == 0);
  uint16_t centi = 0;
  assert(parseUnsignedCentiPure("0", centi, 10000) && centi == 0);
  assert(parseUnsignedCentiPure("1", centi, 10000) && centi == 100);
  assert(parseUnsignedCentiPure("1.6", centi, 10000) && centi == 160);
  assert(parseUnsignedCentiPure("1.60", centi, 10000) && centi == 160);
  assert(parseUnsignedCentiPure("1.605", centi, 10000) && centi == 161); // nearest centi
  assert(parseUnsignedCentiPure("+1", centi, 10000) && centi == 100);
  assert(parseUnsignedCentiPure("1e0", centi, 10000) && centi == 100);
  assert(parseUnsignedCentiPure("1.5e1", centi, 10000) && centi == 1500);
  assert(parseUnsignedCentiPure("5e-3", centi, 10000) && centi == 1);
  assert(parseUnsignedCentiPure("100.00", centi, 10000) && centi == 10000);
  assert(!parseUnsignedCentiPure("100.01", centi, 10000));
  assert(!parseUnsignedCentiPure("-1", centi, 10000));
  assert(!parseUnsignedCentiPure("1.2x", centi, 10000));
  assert(!parseUnsignedCentiPure("", centi, 10000));

  return 0;
}
