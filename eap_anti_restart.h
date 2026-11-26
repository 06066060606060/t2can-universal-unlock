#pragma once

#include <Arduino.h>

#define EAP_NR_NS             "features"
#define EAP_NR_KEY_FRAME      "eapFrame"
#define EAP_NR_WINDOW_MS      60000u
#define EAP_NR_PERIOD_MS      100u
#define EAP_NR_MAX_FRAMES     600u
#define EAP_NR_STOCK_FRESH_MS 2000u

static uint8_t eapNrFrame[8] = {};
static volatile uint8_t eapNrFrameLen = 0;
static volatile bool eapNrFrameValid = false;
static volatile bool eapNrPersisted = false;
static volatile uint32_t eapNrStockMs = 0;
static volatile uint32_t eapNrReplayOk = 0;
static volatile uint32_t eapNrReplayFail = 0;
static volatile uint32_t eapNrReplayBusy = 0;
static volatile uint32_t eapNrReplaySkip = 0;
static volatile uint32_t eapNrLastReplayMs = 0;
static volatile int16_t eapNrLastErr = 0;
static volatile uint32_t eapNrArmedMs = 0;
static volatile uint16_t eapNrBurst = 0;
static bool eapNrWasEnabled = false;

static void eapNrLoad() {
  Preferences p;
  if (!p.begin(EAP_NR_NS, true)) return;
  const String hex = p.getString(EAP_NR_KEY_FRAME, "");
  p.end();
  if (hex.length() != 16) return;

  auto nib = [](char c) -> int8_t {
    if (c >= '0' && c <= '9') return (int8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
    return -1;
  };
  uint8_t decoded[8];
  for (uint8_t i = 0; i < 8; i++) {
    const int8_t a = nib(hex[i * 2]);
    const int8_t b = nib(hex[i * 2 + 1]);
    if (a < 0 || b < 0) return;
    decoded[i] = (uint8_t)((a << 4) | b);
  }
  memcpy(eapNrFrame, decoded, sizeof(decoded));
  eapNrFrameLen = 8;
  eapNrFrameValid = true;
  eapNrPersisted = true;
}

static void eapNrPersistFrame() {
  if (eapNrPersisted || !eapNrFrameValid) return;
  char hex[17];
  for (uint8_t i = 0; i < 8; i++) snprintf(hex + i * 2, 3, "%02X", eapNrFrame[i]);
  hex[16] = '\0';
  Preferences p;
  if (!p.begin(EAP_NR_NS, false)) return;
  const size_t n = p.putString(EAP_NR_KEY_FRAME, hex);
  p.end();
  if (n > 0) eapNrPersisted = true;
}

static void eapNrObserve331(const twai_message_t &f, uint32_t ms) {
  if (f.extd || f.rtr || f.data_length_code != 8u ||
      !activeProfileTlsscRestoreSupported() || activeProfileUsesYlTopology() ||
      !activeCanBIsChassis()) return;
  eapNrStockMs = ms ? ms : 1u;
  uint8_t patched[8];
  memcpy(patched, f.data, sizeof(patched));
  patched[0] = (uint8_t)((patched[0] & 0xC0u) | 0x1Bu);
  bool changed = !eapNrFrameValid;
  for (uint8_t i = 0; i < 8 && !changed; i++) changed = eapNrFrame[i] != patched[i];
  if (!changed) return;
  memcpy(eapNrFrame, patched, sizeof(patched));
  eapNrFrameLen = 8;
  eapNrFrameValid = true;
  if (tlsscRestoreEnabled) eapNrPersistFrame();
}

static void eapNrTick() {
  const bool enabled = tlsscRestoreEnabled;
  if (enabled && !eapNrWasEnabled) {
    eapNrArmedMs = 0;
    eapNrBurst = 0;
    eapNrLastReplayMs = 0;
  }
  eapNrWasEnabled = enabled;
  if (!enabled || !activeProfileTlsscRestoreSupported() || activeProfileUsesYlTopology() ||
      !activeCanBIsChassis() || !twaiReady || canTxAdministrativeHold ||
      canUsbPassive() || !twaiNonSummonAdmissionOpen()) return;

  const uint32_t now = (uint32_t)millis();
  if (eapNrArmedMs == 0) eapNrArmedMs = now ? now : 1u;
  if ((uint32_t)(now - eapNrArmedMs) > EAP_NR_WINDOW_MS) return;
  if (eapNrBurst >= EAP_NR_MAX_FRAMES) return;
  if (!eapNrFrameValid) { eapNrReplaySkip++; return; }
  eapNrPersistFrame();
  if (eapNrStockMs && (uint32_t)(now - eapNrStockMs) < EAP_NR_STOCK_FRESH_MS) return;
  if (eapNrLastReplayMs && (uint32_t)(now - eapNrLastReplayMs) < EAP_NR_PERIOD_MS) return;

  twai_message_t out = {};
  out.identifier = 0x331u;
  out.extd = 0;
  out.rtr = 0;
  out.data_length_code = eapNrFrameLen;
  memcpy(out.data, eapNrFrame, eapNrFrameLen);
  const esp_err_t err = canTxTwaiTransmit(&out, canTxEpochSnapshot());
  eapNrLastErr = (int16_t)err;
  if (err == ESP_ERR_TIMEOUT) {
    eapNrReplayBusy++;
    return;
  }
  eapNrLastReplayMs = now ? now : 1u;
  eapNrBurst++;
  if (err == ESP_OK) eapNrReplayOk++;
  else eapNrReplayFail++;
}

static void eapNrStatsAppend(String &json) {
  json += ",\"eap\":{\"cacheValid\":";
  json += eapNrFrameValid ? "true" : "false";
  json += ",\"cacheLen\":"; json += String((unsigned)eapNrFrameLen);
  json += ",\"cacheHex\":\"";
  if (eapNrFrameValid) {
    for (uint8_t i = 0; i < eapNrFrameLen; i++) {
      char b[3];
      snprintf(b, sizeof(b), "%02X", eapNrFrame[i]);
      json += b;
    }
  }
  json += "\",\"replayOk\":"; json += String((unsigned long)eapNrReplayOk);
  json += ",\"replayFail\":"; json += String((unsigned long)eapNrReplayFail);
  json += ",\"replayBusy\":"; json += String((unsigned long)eapNrReplayBusy);
  json += ",\"replaySkip\":"; json += String((unsigned long)eapNrReplaySkip);
  json += ",\"burst\":"; json += String((unsigned)eapNrBurst);
  json += ",\"lastErr\":"; json += String((int)eapNrLastErr);
  json += ",\"lastErrName\":\"";
  switch ((uint16_t)eapNrLastErr) {
    case 0: json += "OK"; break;
    case 0x107: json += "TX-busy"; break;
    case 0x103: json += "gate-closed"; break;
    case 0x10B: json += "send-failed"; break;
    default: json += "other"; break;
  }
  json += "\",\"epoch\":\""; json += canTxEpochSnapshot() ? "live" : "boot";
  json += "\"}";
}
