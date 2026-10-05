#pragma once
#include <stdint.h>
#include <string.h>

struct ConfirmCountryPure { uint16_t numeric; const char *alpha2; };
// ISO country codes: https://unstats.un.org/unsd/methodology/m49/
static constexpr ConfirmCountryPure CONFIRM_COUNTRIES_PURE[] = {
  {124, "CA"}, {156, "CN"}, {250, "FR"}, {276, "DE"},
  {392, "JP"}, {410, "KR"}, {826, "GB"}, {840, "US"}
};
static inline const ConfirmCountryPure *confirmCountryPure(uint16_t numeric) {
  for (const auto &country : CONFIRM_COUNTRIES_PURE)
    if (country.numeric == numeric) return &country;
  return nullptr;
}
static inline bool confirmCountryValidPure(uint16_t numeric) {
  return numeric == 0 || confirmCountryPure(numeric);
}
static inline uint16_t confirmCountryLoadPure(bool hasCountry, uint16_t country, bool legacyKorea) {
  return hasCountry ? (confirmCountryValidPure(country) ? country : 0) : (legacyKorea ? 410 : 0);
}

static inline bool confirmCountryPatchPure(uint32_t id, uint8_t dlc, uint8_t *data, uint16_t numeric) {
  if (dlc != 8 || !data) return false;
  const auto *country = confirmCountryPure(numeric);
  if (!country) return false;
  uint8_t before[8]; memcpy(before, data, 8);
  if (id == 0x7FF && data[0] == 1) {
    // Native FR is bytes 52 46: retain the observed packed-country order.
    data[2] = country->alpha2[1]; data[3] = country->alpha2[0];
  } else if (id == 0x238) {
    if (((data[2] | ((uint16_t)data[3] << 8)) & 1023) == numeric) return false;
    data[2] = (uint8_t)numeric; data[3] = (uint8_t)((data[3] & 0xFC) | (numeric >> 8));
    data[6] = (uint8_t)((data[6] & 0x0F) | (((data[6] + 0x10) & 0xF0)));
    uint8_t sum = 0x38 + 2;
    for (uint8_t i = 0; i < 7; ++i) sum = (uint8_t)(sum + data[i]);
    data[7] = sum;
  }
  return memcmp(before, data, 8) != 0;
}
