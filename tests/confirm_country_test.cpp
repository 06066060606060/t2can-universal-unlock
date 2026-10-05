#include <cassert>
#include <cstring>
#include <cstdio>
#include "../confirm_country_pure.h"
int main() {
  const uint8_t stockMap[8] = {3, 0xB1, 8, 0x6D, 0xE0, 0x40, 0x1E, 0};
  const uint8_t stockRoad[8] = {0xF0, 0x21, 0xFA, 0xD8, 0x1F, 0, 0xF6, 0x32};
  assert(confirmCountryLoadPure(false, 0, false) == 0);
  assert(confirmCountryLoadPure(false, 0, true) == 410); // Existing Korea preference survives upgrade.
  assert(confirmCountryLoadPure(true, 0, true) == 0); // Original takes priority over legacy Korea.
  assert(confirmCountryLoadPure(true, 999, true) == 0); // Invalid saved values fail closed.
  assert(confirmCountryLoadPure(true, 840, true) == 840);
  for (const auto &country : CONFIRM_COUNTRIES_PURE) {
    assert(confirmCountryValidPure(country.numeric));
    uint8_t malformed[8]; memcpy(malformed, stockRoad, 8);
    assert(!confirmCountryPatchPure(0x239, 8, malformed, country.numeric));
    assert(!confirmCountryPatchPure(0x238, 7, malformed, country.numeric));
    assert(!confirmCountryPatchPure(0x238, 9, malformed, country.numeric));
    assert(!confirmCountryPatchPure(0x238, 8, nullptr, country.numeric));
    uint8_t raw[8] = {1, 0xA5, 'Z', 'Z', 2, 0x7D, 0xE1, 0};
    assert(confirmCountryPatchPure(0x7FF, 8, raw, country.numeric));
    assert(raw[2] == country.alpha2[1] && raw[3] == country.alpha2[0]);
    assert(raw[0] == 1 && raw[1] == 0xA5 && raw[4] == 2 && raw[5] == 0x7D && raw[6] == 0xE1);
    assert(!confirmCountryPatchPure(0x7FF, 8, raw, country.numeric));
    for (unsigned counter = 0; counter < 16; ++counter) {
      memcpy(raw, stockRoad, 8); raw[2] = 1; raw[3] = 0xFC; raw[6] = (uint8_t)(counter << 4 | 6);
      assert(confirmCountryPatchPure(0x238, 8, raw, country.numeric));
      assert((raw[2] | ((raw[3] & 3) << 8)) == country.numeric);
      assert((raw[3] & 0xFC) == 0xFC && (raw[6] & 15) == 6);
      assert((raw[6] >> 4) == ((counter + 1) & 15));
      uint8_t checksum = 0x3A;
      for (unsigned i = 0; i < 7; ++i) checksum += raw[i];
      assert(raw[7] == checksum);
      assert(!confirmCountryPatchPure(0x238, 8, raw, country.numeric));
    }
    memcpy(raw, stockMap, 8);
    assert(!confirmCountryPatchPure(0x7FF, 8, raw, country.numeric));
    assert(!memcmp(raw, stockMap, 8)); // Country assist never changes map region.
  }
  const uint16_t invalidCountries[] = {0, 999, 65535};
  for (uint16_t invalid : invalidCountries) {
    uint8_t raw[8]; memcpy(raw, stockRoad, 8);
    assert(!confirmCountryPatchPure(0x238, 8, raw, invalid));
    assert(!memcmp(raw, stockRoad, 8));
  }
  puts("Confirm-Free country: migration, field preservation, checksum/counter, map isolation and malformed RX PASS");
}
