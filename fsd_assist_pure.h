#pragma once
#include <stddef.h>
#include <stdint.h>

// Stock country values that published FSD Supervised lists treated as
// approved around 2026-11-26. The car broadcasts the numeric code on
// 0x238 and the alpha-2 letters on 0x7FF mux 1. Read those stock frames.
// UI_fsdAllowedInLocalizedRegion is already 1 on a non-FSD car, so it is
// not an FSD test. A hardcoded list goes stale when Tesla adds a country.
static constexpr uint16_t FSD_COUNTRY_CODES_PURE[] = {
    36u, 56u, 124u, 156u, 191u, 203u, 208u, 233u, 246u, 380u,
    410u, 428u, 440u, 484u, 528u, 554u, 630u, 703u, 705u, 840u};
static constexpr size_t FSD_COUNTRY_CODE_COUNT_PURE =
    sizeof(FSD_COUNTRY_CODES_PURE) / sizeof(FSD_COUNTRY_CODES_PURE[0]);

static constexpr uint16_t fsdCountryPackPure(char a, char b) {
  return (uint16_t)(uint8_t)a | ((uint16_t)(uint8_t)b << 8);
}

// AU BE CA CN HR CZ DK EE FI IT KR LV LT MX NL NZ PR SK SI US.
static constexpr uint16_t FSD_COUNTRY_LETTERS_PURE[] = {
    fsdCountryPackPure('A', 'U'), fsdCountryPackPure('B', 'E'),
    fsdCountryPackPure('C', 'A'), fsdCountryPackPure('C', 'N'),
    fsdCountryPackPure('C', 'Z'), fsdCountryPackPure('D', 'K'),
    fsdCountryPackPure('E', 'E'), fsdCountryPackPure('F', 'I'),
    fsdCountryPackPure('H', 'R'), fsdCountryPackPure('I', 'T'),
    fsdCountryPackPure('K', 'R'), fsdCountryPackPure('L', 'T'),
    fsdCountryPackPure('L', 'V'), fsdCountryPackPure('M', 'X'),
    fsdCountryPackPure('N', 'L'), fsdCountryPackPure('N', 'Z'),
    fsdCountryPackPure('P', 'R'), fsdCountryPackPure('S', 'I'),
    fsdCountryPackPure('S', 'K'), fsdCountryPackPure('U', 'S')};
static constexpr size_t FSD_COUNTRY_LETTER_COUNT_PURE =
    sizeof(FSD_COUNTRY_LETTERS_PURE) / sizeof(FSD_COUNTRY_LETTERS_PURE[0]);

static constexpr uint8_t FSD_COUNTRY_UNKNOWN_PURE = 0u;
static constexpr uint8_t FSD_COUNTRY_NOT_APPROVED_PURE = 1u;
static constexpr uint8_t FSD_COUNTRY_APPROVED_PURE = 2u;

static inline bool fsdCountryCodeApprovedPure(uint16_t code) {
  for (size_t i = 0; i < FSD_COUNTRY_CODE_COUNT_PURE; ++i) {
    if (FSD_COUNTRY_CODES_PURE[i] == code) return true;
  }
  return false;
}

static inline bool fsdCountryLettersApprovedPure(uint8_t a, uint8_t b) {
  if (a < (uint8_t)'A' || a > (uint8_t)'Z' || b < (uint8_t)'A' || b > (uint8_t)'Z')
    return false;
  const uint16_t packed = fsdCountryPackPure((char)a, (char)b);
  for (size_t i = 0; i < FSD_COUNTRY_LETTER_COUNT_PURE; ++i) {
    if (FSD_COUNTRY_LETTERS_PURE[i] == packed) return true;
  }
  return false;
}

// Unknown until either stock source has been seen. If the two sources
// disagree, approved wins so a country rewrite is refused.
static inline uint8_t fsdCountryVerdictPure(bool numericSeen, uint16_t numeric,
                                           bool lettersSeen, uint8_t a, uint8_t b) {
  const bool numericApproved = numericSeen && fsdCountryCodeApprovedPure(numeric);
  const bool lettersApproved = lettersSeen && fsdCountryLettersApprovedPure(a, b);
  if (numericApproved || lettersApproved) return FSD_COUNTRY_APPROVED_PURE;
  if (numericSeen || lettersSeen) return FSD_COUNTRY_NOT_APPROVED_PURE;
  return FSD_COUNTRY_UNKNOWN_PURE;
}

// UI_applyADDWWarnings on UI_autopilotControl 0x3FD mux 0 bit 62.
// 0xFF leaves the car's bit. 0 clears it. 1 sets it.
// Returns false when the bit already matches. Callers still transmit that
// frame: a matching bit is not a reason to leave the car's frame as the
// newest one. Bit 60 (DCAS) and bits 38/39 (TLSSC) are left untouched.
static constexpr uint8_t ADDW_WARNINGS_STOCK_PURE = 0xFFu;
// The car sends mux 0 about every 500 ms. Repeat the composed clone faster
// than that, and stop when no stock mux 0 has arrived for 1.5 s.
static constexpr uint32_t ADDW_REPEAT_MS_PURE = 50u;
static constexpr uint32_t ADDW_REPEAT_STALE_MS_PURE = 1500u;

static inline int8_t addwWarningsReadPure(const uint8_t *data) {
  if (!data || (data[0] & 7u) != 0u) return -1;
  return (int8_t)((data[7] >> 6) & 1u);
}

static inline bool addwWarningsApplyPure(uint8_t *data, uint8_t dlc, uint8_t mode) {
  if (!data || dlc != 8u || (data[0] & 7u) != 0u) return false;
  if (mode != 0u && mode != 1u) return false;
  const uint8_t current = (uint8_t)((data[7] >> 6) & 1u);
  if (current == mode) return false;
  if (mode == 1u) data[7] = (uint8_t)(data[7] | (uint8_t)0x40u);
  else data[7] = (uint8_t)(data[7] & (uint8_t)~0x40u);
  return true;
}

// True when a remembered mux 0 clone should be sent again. lastTxMs 0 means
// it has not been sent yet. Unsigned subtraction handles millis wrap.
static inline bool addwRepeatDuePure(bool haveTemplate, uint32_t now,
                                    uint32_t stockMs, uint32_t lastTxMs) {
  if (!haveTemplate) return false;
  if ((uint32_t)(now - stockMs) > ADDW_REPEAT_STALE_MS_PURE) return false;
  if (lastTxMs != 0u && (uint32_t)(now - lastTxMs) < ADDW_REPEAT_MS_PURE) return false;
  return true;
}

// APP_driverMonitorStatus 0x3AB. The Autopilot computer sends this.
// Captures on this car are 4 bytes. Bit 32 is absent on that short frame.
struct AppDmStatusPure {
  uint8_t addwStatus;
  uint8_t attnOn;
  uint8_t handsOn;
  uint8_t handsSource;
  uint8_t handsNotReady;
  uint8_t handsInputRequired;
};

static inline bool appDmStatusDecodePure(const uint8_t *data, uint8_t dlc,
                                         AppDmStatusPure *out) {
  if (!data || !out || dlc < 4u) return false;
  const uint32_t word = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
      ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
  out->attnOn = (uint8_t)((word >> 4) & 1u);
  out->handsOn = (uint8_t)((word >> 5) & 1u);
  out->handsSource = (uint8_t)((word >> 8) & 7u);
  out->handsNotReady = (uint8_t)((word >> 23) & 1u);
  out->handsInputRequired = (uint8_t)((word >> 25) & 1u);
  out->addwStatus = (uint8_t)((word >> 28) & 3u);
  return true;
}
