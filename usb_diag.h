#pragma once

// Passive 0x3F8 observations, kept distinct by physical connector since boot.
static struct UsbUlcRx { uint32_t count[2], ms[2]; uint8_t raw[8]; } usbUlcRx[3] = {};
static portMUX_TYPE usbUlcRxMux = portMUX_INITIALIZER_UNLOCKED;
// Latest planner/settings frames; keep configuration pages and physical buses separate.
static constexpr struct { uint16_t id; int16_t page; } USB_DAS_IDS[] = {
  {0x24A, -1}, {0x3E9, -1}, {0x293, -1}, {0x238, -1}, {0x7FF, 1}, {0x7FF, 3}
};
static constexpr size_t USB_DAS_COUNT = sizeof(USB_DAS_IDS) / sizeof(USB_DAS_IDS[0]);
static struct UsbDasRx { uint32_t count, ms; uint8_t raw[8]; } usbDasRx[3][USB_DAS_COUNT] = {};
static void usbDiagUlcObserve(uint8_t bus, uint32_t id, uint8_t dlc, const uint8_t *data) {
  if (bus >= 3 || dlc != 8 || !data) return;
  for (uint8_t i = 0; i < USB_DAS_COUNT; ++i) if (id == USB_DAS_IDS[i].id &&
      (USB_DAS_IDS[i].page < 0 || data[0] == USB_DAS_IDS[i].page)) {
    const uint32_t now = (uint32_t)millis();
    portENTER_CRITICAL(&usbUlcRxMux);
    UsbDasRx &rx = usbDasRx[bus][i];
    rx.count++; rx.ms = now; memcpy(rx.raw, data, 8);
    portEXIT_CRITICAL(&usbUlcRxMux);
    return;
  }
  if (id != 0x3F8) return;
  const uint8_t bit = (data[0] >> 1) & 1;
  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&usbUlcRxMux);
  usbUlcRx[bus].count[bit]++;
  usbUlcRx[bus].ms[bit] = now;
  memcpy(usbUlcRx[bus].raw, data, 8);
  portEXIT_CRITICAL(&usbUlcRxMux);
}

static String usbDiagDasBusRx() {
  UsbDasRx rx[3][USB_DAS_COUNT];
  portENTER_CRITICAL(&usbUlcRxMux);
  memcpy(rx, usbDasRx, sizeof(rx));
  portEXIT_CRITICAL(&usbUlcRxMux);
  const uint32_t now = (uint32_t)millis();
  String s = "{\"scope\":\"LATEST_RX_PER_PHYSICAL_BUS\",\"frames\":[";
  for (uint8_t b = 0; b < 3; ++b) for (uint8_t i = 0; i < USB_DAS_COUNT; ++i) {
    const auto &e = rx[b][i];
    char raw[17] = {}, line[192];
    if (e.count) for (uint8_t j = 0; j < 8; ++j) snprintf(raw + j * 2, 3, "%02X", e.raw[j]);
    snprintf(line, sizeof(line), "%s{\"bus\":\"%c\",\"id\":%u,\"rx\":%lu,\"ms\":%lu,\"ageMs\":%lu,\"raw\":\"%s\",\"page\":%d}",
             b || i ? "," : "", 'A' + b, (unsigned)USB_DAS_IDS[i].id,
             (unsigned long)e.count, (unsigned long)e.ms,
             (unsigned long)(e.count ? now - e.ms : 999999), raw, (int)USB_DAS_IDS[i].page);
    s += line;
  }
  s += "]}";
  return s;
}

static String usbDiagUlcBusRx() {
  UsbUlcRx rx[3];
  portENTER_CRITICAL(&usbUlcRxMux);
  memcpy(rx, usbUlcRx, sizeof(rx));
  portEXIT_CRITICAL(&usbUlcRxMux);
  const uint32_t now = (uint32_t)millis();
  String s = "{\"scope\":\"RX_SINCE_BOOT\",\"id\":1016,\"buses\":[";
  for (uint8_t i = 0; i < 3; ++i) {
    char raw[17] = {}, line[240];
    if (rx[i].count[0] || rx[i].count[1])
      for (uint8_t j = 0; j < 8; ++j) snprintf(raw + j * 2, 3, "%02X", rx[i].raw[j]);
    snprintf(line, sizeof(line), "%s{\"bus\":\"%c\",\"zeroRx\":%lu,\"oneRx\":%lu,\"zeroAgeMs\":%lu,\"oneAgeMs\":%lu,\"raw\":\"%s\"}",
             i ? "," : "", 'A' + i, (unsigned long)rx[i].count[0], (unsigned long)rx[i].count[1],
             (unsigned long)(rx[i].count[0] ? now - rx[i].ms[0] : 999999),
             (unsigned long)(rx[i].count[1] ? now - rx[i].ms[1] : 999999), raw);
    s += line;
  }
  s += "]}";
  return s;
}

static String usbDiagCanBTxTrace() {
  CanBTxTraceEntry entries[CAN_B_TX_TRACE_CAPACITY];
  portENTER_CRITICAL(&canBTxTraceMux);
  const uint8_t count = canBTxTraceLiveCount;
  const uint8_t head = canBTxTraceLiveHead;
  memcpy(entries, canBTxTraceLive, sizeof(entries));
  portEXIT_CRITICAL(&canBTxTraceMux);
  String s = "{\"scope\":\"CAN_B_ENQUEUE\",\"frames\":[";
  s.reserve(10000);
  for (uint8_t i = 0; i < count && i < CAN_B_TX_TRACE_CAPACITY; ++i) {
    const auto &e = entries[(head + CAN_B_TX_TRACE_CAPACITY - count + i) % CAN_B_TX_TRACE_CAPACITY];
    char raw[17] = {}, line[160];
    for (uint8_t j = 0; j < e.dlc && j < 8; ++j) snprintf(raw + j * 2, 3, "%02X", e.data[j]);
    snprintf(line, sizeof(line), "%s{\"seq\":%lu,\"ms\":%lu,\"id\":%u,\"dlc\":%u,\"result\":%ld,\"raw\":\"%s\"}",
             i ? "," : "", (unsigned long)e.seq, (unsigned long)e.capturedMs,
             (unsigned)e.id, (unsigned)e.dlc, (long)e.result, raw);
    s += line;
  }
  s += "]}";
  return s;
}

static String usbDiagConfirmTiming(uint8_t timing) {
  portENTER_CRITICAL(&lab3f8Mux);
  const uint8_t before = ulcNoConfirmTimingMode;
  ulcNoConfirmTimingMode = timing;
  portEXIT_CRITICAL(&lab3f8Mux);
  if (!ulcCfgSave()) {
    portENTER_CRITICAL(&lab3f8Mux);
    ulcNoConfirmTimingMode = before;
    portEXIT_CRITICAL(&lab3f8Mux);
    return "{\"error\":\"NVS write failed\"}";
  }
  return ulcStatsToJson();
}

static String usbDiagConfirmCountry(uint16_t country) {
  if (!confirmCountrySupported() || !confirmCountryValidPure(country))
    return "{\"error\":\"Country assist unsupported\"}";
  if (!confirmCountrySave(country)) return "{\"error\":\"NVS write failed\"}";
  return ulcStatsToJson();
}

// Dashboard snapshots and the existing Confirm-Free timing selector over USB.
// Independent of synchronous HTTP; only the exact requests below are accepted.
static void usbDiagTick() {
  static const struct { const char *request; String (*json)(); } routes[] = {
    {"GET /api/profile/status", vehicleProfileStatusJson},
    {"GET /api/features/status", v3FeaturePolicyJson},
    {"GET /api/blinkA/stats", blinkAStatsToJson},
    {"GET /api/das/stats", dasTelemetryStatsToJson},
    {"GET /api/das/bus-rx", usbDiagDasBusRx},
    {"GET /api/r79/stats", r79StatsToJson},
    {"GET /api/canb/txtrace", usbDiagCanBTxTrace},
    {"GET /api/ulc/bus-rx", usbDiagUlcBusRx},
    {"GET /api/ulc/stats", ulcStatsToJson},
    {"GET /api/lab/auto-lane-change/stats", ulcStatsToJson},
    {"GET /api/system/stats", systemStatsToJson},
    {"GET /api/researchcapture/stats", researchCaptureStatsToJson},
    {"POST /api/ulc/update?timing=0", []() { return usbDiagConfirmTiming(0); }},
    {"POST /api/ulc/update?timing=1", []() { return usbDiagConfirmTiming(1); }},
    {"POST /api/ulc/update?country=0", []() { return usbDiagConfirmCountry(0); }},
    {"POST /api/ulc/update?country=124", []() { return usbDiagConfirmCountry(124); }},
    {"POST /api/ulc/update?country=156", []() { return usbDiagConfirmCountry(156); }},
    {"POST /api/ulc/update?country=250", []() { return usbDiagConfirmCountry(250); }},
    {"POST /api/ulc/update?country=276", []() { return usbDiagConfirmCountry(276); }},
    {"POST /api/ulc/update?country=392", []() { return usbDiagConfirmCountry(392); }},
    {"POST /api/ulc/update?country=410", []() { return usbDiagConfirmCountry(410); }},
    {"POST /api/ulc/update?country=826", []() { return usbDiagConfirmCountry(826); }},
    {"POST /api/ulc/update?country=840", []() { return usbDiagConfirmCountry(840); }},
  };
  static char line[64];
  static size_t used = 0, sent = 0;
  static bool overflow = false;
  static String response;
  static uint32_t responseAt = 0;
  if (!Serial) {
    used = sent = 0; overflow = false; response = "";
    return;
  }
  if (response.length()) {
    // Expire stalled output, allowing large traces to progress without blocking CAN.
    if ((uint32_t)(millis() - responseAt) >= 2000) { response = ""; return; }
    const int free = Serial.availableForWrite();
    size_t count = response.length() - sent;
    if (free <= 0) return;
    if (count > (size_t)free) count = (size_t)free;
    if (count > 256) count = 256;
    const size_t written = Serial.write((const uint8_t *)response.c_str() + sent, count);
    sent += written;
    if (written) responseAt = millis();
    if (sent == response.length()) response = "";
    return;
  }
  for (unsigned budget = 0; budget < sizeof(line) && Serial.available(); ++budget) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (c < 32 || c > 126 || used == sizeof(line) - 1) overflow = true;
      else if (!overflow) line[used++] = c;
      continue;
    }
    if (!used && !overflow) continue;
    line[used] = 0;
    response = "{\"error\":\"unsupported USB request\"}\n";
    if (!overflow) {
      for (const auto &route : routes) if (strcmp(line, route.request) == 0) {
        response = String("{\"path\":\"") + (strchr(route.request, ' ') + 1) + "\",\"data\":" + route.json() + "}\n";
        break;
      }
    }
    used = sent = 0; overflow = false; responseAt = millis();
    return;
  }
}
