#pragma once
#include "confirm_country_pure.h"

static portMUX_TYPE confirmCountryMux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t confirmCountry = 0; // 0 preserves the native country.

static bool confirmCountrySupported() {
  return board == BOARD_TMR && boardTripleCan() && activeProfileUlcNoConfirmSupported();
}
static uint16_t confirmCountryValue() {
  portENTER_CRITICAL(&confirmCountryMux);
  const uint16_t country = confirmCountry;
  portEXIT_CRITICAL(&confirmCountryMux);
  return country;
}
static void confirmCountryLoad() {
  if (board != BOARD_TMR || !boardTripleCan()) return;
  Preferences p;
  if (p.begin("regionLab", true)) {
    confirmCountry = confirmCountryLoadPure(p.isKey("country"), p.getUShort("country", 0),
                                          p.getBool("countryKR", false));
    p.end();
  }
}

static bool confirmCountrySave(uint16_t country) {
  if (!confirmCountrySupported() || !confirmCountryValidPure(country)) return false;
  Preferences p;
  if (!p.begin("regionLab", false)) return false;
  const bool ok = p.putUShort("country", country) > 0 && p.getUShort("country", 65535) == country;
  p.end();
  if (ok) {
    portENTER_CRITICAL(&confirmCountryMux);
    confirmCountry = country;
    portEXIT_CRITICAL(&confirmCountryMux);
  }
  return ok;
}

static uint16_t confirmCountryActive() {
  portENTER_CRITICAL(&lab3f8Mux);
  const bool enabled = ulcNoConfirmEnabled;
  portEXIT_CRITICAL(&lab3f8Mux);
  return enabled && confirmCountrySupported() ? confirmCountryValue() : 0;
}

static void confirmCountryObserve(uint8_t bus, uint32_t id, uint8_t dlc, const uint8_t *data) {
  if (bus >= 3 || (id != 0x238 && id != 0x7FF) || dlc != 8 || !data ||
      board != BOARD_TMR || !boardTripleCan()) return;
  const uint16_t country = confirmCountryActive();
  if (!country) return;
  uint8_t raw[8]; memcpy(raw, data, 8);
  if (!confirmCountryPatchPure(id, dlc, raw, country)) return;
  const uint32_t epoch = canTxEpochSnapshot();
  if (confirmCountryActive() != country) return;
  if (bus == 1) {
    if (twaiNonSummonAdmissionOpen()) {
      twai_message_t out = {}; out.identifier = id; out.data_length_code = 8;
      memcpy(out.data, raw, 8);
      canTxTwaiTransmit(&out, epoch);
    }
  } else {
    struct can_frame out = {}; out.can_id = id; out.can_dlc = 8;
    memcpy(out.data, raw, 8);
    MCP2515::ERROR err = MCP2515::ERROR_FAIL;
    const bool attempted = canTxMcpSend(&out, epoch, err, nullptr, bus == 2);
    if (attempted) {
      if (err == MCP2515::ERROR_OK) { mcpTxOk++; mcpTxFailConsecutive = 0; }
      else { mcpTxFail++; if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++; }
    }
  }
}
