#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include "../confirm_country_pure.h"
using String = std::string;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
static uint32_t now;
static uint32_t millis() { return now; }
static unsigned snapshots;
#define STATUS(name) static String name() { ++snapshots; return "{\"source\":\"" #name "\"}"; }
STATUS(vehicleProfileStatusJson) STATUS(v3FeaturePolicyJson) STATUS(blinkAStatsToJson)
STATUS(dasTelemetryStatsToJson) STATUS(ulcStatsToJson) STATUS(systemStatsToJson)
STATUS(researchCaptureStatsToJson) STATUS(r79StatsToJson)
static constexpr uint8_t CAN_B_TX_TRACE_CAPACITY = 64;
struct CanBTxTraceEntry { uint32_t seq, capturedMs; uint16_t id; uint8_t dlc, source; int32_t result; uint8_t data[8]; };
static CanBTxTraceEntry canBTxTraceLive[CAN_B_TX_TRACE_CAPACITY] = {};
static uint8_t canBTxTraceLiveCount, canBTxTraceLiveHead;
static uint8_t ulcNoConfirmTimingMode;
static bool saveOk = true;
static bool ulcCfgSave() { return saveOk; }
static bool countrySupported = true;
static uint16_t savedCountry;
static bool confirmCountrySupported() { return countrySupported; }
static bool confirmCountrySave(uint16_t country) { if (!saveOk) return false; savedCountry = country; return true; }
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
static struct {
  bool connected = true;
  int capacity = 512;
  size_t shortWrite = 512;
  String input, output;
  operator bool() const { return connected; }
  int available() { return input.size(); }
  int read() { char c = input.front(); input.erase(0, 1); return c; }
  int availableForWrite() { return capacity; }
  size_t write(const uint8_t *p, size_t count) {
    assert(count <= (size_t)capacity && count <= 256);
    count = std::min(count, shortWrite);
    output.append((const char *)p, count); return count;
  }
} Serial;
#include "../usb_diag.h"
static void ticks(unsigned n = 100) { while (n--) usbDiagTick(); }
static String request(const String &s) {
  Serial.output.clear(); Serial.input = s; ticks(); return Serial.output;
}
int main() {
  ticks(); assert(snapshots == 0 && Serial.output.empty());
  for (unsigned country : {0u, 124u, 156u, 250u, 276u, 392u, 410u, 826u, 840u}) {
    assert(request("POST /api/ulc/update?country=" + std::to_string(country) + "\n").find("ulcStatsToJson") != String::npos);
    assert(savedCountry == country);
  }
  for (const char *invalid : {"999", "0410", "-1", "410&confirm=1", "410junk"})
    assert(request(String("POST /api/ulc/update?country=") + invalid + "\n").find("error") != String::npos);
  countrySupported = false;
  assert(request("POST /api/ulc/update?country=410\n").find("unsupported") != String::npos);
  assert(savedCountry == 840); // T2CAN and unsupported profiles do not write preferences.
  countrySupported = true; saveOk = false;
  assert(request("POST /api/ulc/update?country=410\n").find("NVS write failed") != String::npos);
  assert(savedCountry == 840);
  saveOk = true;
  assert(request("GET /api/profile/status\r\n").find("vehicleProfileStatusJson") != String::npos);
  assert(request("GET /api/lab/auto-lane-change/stats\n").find("ulcStatsToJson") != String::npos);
  assert(request("GET /api/blinkA/").empty());
  assert(request("stats\n").find("blinkAStatsToJson") != String::npos);
  const unsigned before = snapshots;
  assert(request("POST /api/blinkA/enable\n").find("error") != String::npos);
  assert(request(String(80, 'x') + "GET /api/blinkA/stats\n").find("error") != String::npos);
  assert(request(String("GET /api/profile/status\0bad\n", 28)).find("error") != String::npos);
  assert(snapshots == before);
  assert(request("GET /api/ulc/update?timing=1\n").find("error") != String::npos);
  assert(ulcNoConfirmTimingMode == 0);
  assert(request("POST /api/ulc/update?timing=1\n").find("ulcStatsToJson") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  saveOk = false;
  assert(request("POST /api/ulc/update?timing=0\n").find("NVS write failed") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  saveOk = true;
  assert(request("POST /api/ulc/update?timing=2\n").find("error") != String::npos);
  assert(ulcNoConfirmTimingMode == 1);
  request("POST /api/ulc/update?timing=0\n"); assert(ulcNoConfirmTimingMode == 0);
  assert(request("GET /api/das/stats\n").find("dasTelemetryStatsToJson") != String::npos);
  assert(request("GET /api/r79/stats\n").find("r79StatsToJson") != String::npos);
  assert(request("POST /api/r79/update\n").find("error") != String::npos);
  assert(request("GET /api/ulc/bus-rx\n").find("\"raw\":\"\"") != String::npos);
  uint8_t stock[8] = {0x83, 0x28, 8, 0, 0x59, 0xDD, 0x9F, 0xA0};
  now = 0xFFFFFFF0u;
  usbDiagUlcObserve(2, 0x3F8, 8, stock);
  usbDiagUlcObserve(1, 0x3F8, 8, stock);
  stock[0] = 0x81;
  now = 0xFFFFFFFAu;
  usbDiagUlcObserve(2, 0x3F8, 8, stock);
  usbDiagUlcObserve(0, 0x294, 8, stock); // Unrelated or malformed frames ignored.
  usbDiagUlcObserve(0, 0x3F8, 7, stock);
  usbDiagUlcObserve(3, 0x3F8, 8, stock);
  usbDiagUlcObserve(0, 0x3F8, 8, nullptr);
  now = 10;
  const String buses = request("GET /api/ulc/bus-rx\n");
  assert(buses.find("\"bus\":\"A\",\"zeroRx\":0,\"oneRx\":0") != String::npos);
  assert(buses.find("\"bus\":\"B\",\"zeroRx\":0,\"oneRx\":1") != String::npos);
  assert(buses.find("\"bus\":\"C\",\"zeroRx\":1,\"oneRx\":1,\"zeroAgeMs\":16,\"oneAgeMs\":26,\"raw\":\"8128080059DD9FA0\"") != String::npos);
  assert(request("POST /api/ulc/bus-rx\n").find("error") != String::npos);
  // Same-ID frames on Body/Party must remain separate; reject short/invalid RX.
  assert(request("GET /api/das/bus-rx\n").find("\"rx\":0,\"ms\":0,\"ageMs\":999999,\"raw\":\"\"") != String::npos);
  const uint8_t fork[8] = {0, 2, 12, 0, 0, 0, 0, 0};
  const uint8_t passing[8] = {0, 1, 8, 0, 0, 0, 0, 0};
  now = 0xFFFFFFF0u;
  usbDiagUlcObserve(0, 0x3E9, 8, fork);
  usbDiagUlcObserve(2, 0x3E9, 8, passing);
  usbDiagUlcObserve(1, 0x24A, 8, stock);
  usbDiagUlcObserve(1, 0x3E9, 4, fork);
  usbDiagUlcObserve(1, 0x3E9, 9, fork);
  usbDiagUlcObserve(1, 0x3E9, 8, nullptr);
  usbDiagUlcObserve(3, 0x24A, 8, stock);
  now = 10;
  const String dasBuses = request("GET /api/das/bus-rx\n");
  assert(dasBuses.find("\"bus\":\"A\",\"id\":1001,\"rx\":1,\"ms\":4294967280,\"ageMs\":26,\"raw\":\"00020C0000000000\"") != String::npos);
  assert(dasBuses.find("\"bus\":\"C\",\"id\":1001,\"rx\":1,\"ms\":4294967280,\"ageMs\":26,\"raw\":\"0001080000000000\"") != String::npos);
  assert(dasBuses.find("\"bus\":\"B\",\"id\":1001,\"rx\":0") != String::npos);
  assert(dasBuses.find("\"bus\":\"B\",\"id\":586,\"rx\":1") != String::npos);
  assert(request("POST /api/das/bus-rx\n").find("error") != String::npos);
  // A changing 0x7FF page must not overwrite the other region/configuration page.
  const uint8_t country[8] = {1, 2, 0x46, 0x52, 0, 4, 0, 0};
  const uint8_t region[8] = {3, 0xB1, 0, 0, 0, 0, 0, 0};
  const uint8_t otherPage[8] = {2, 0xFF, 0, 0, 0, 0, 0, 0};
  usbDiagUlcObserve(2, 0x293, 8, stock);
  usbDiagUlcObserve(1, 0x238, 8, passing);
  usbDiagUlcObserve(2, 0x7FF, 8, country);
  usbDiagUlcObserve(2, 0x7FF, 8, region);
  usbDiagUlcObserve(2, 0x7FF, 8, otherPage);
  usbDiagUlcObserve(2, 0x7FF, 7, country);
  usbDiagUlcObserve(3, 0x7FF, 8, region);
  usbDiagUlcObserve(2, 0x7FF, 8, nullptr);
  const String config = request("GET /api/das/bus-rx\n");
  assert(config.find("\"bus\":\"C\",\"id\":659,\"rx\":1") != String::npos);
  assert(config.find("\"bus\":\"B\",\"id\":568,\"rx\":1") != String::npos);
  assert(config.find("\"bus\":\"C\",\"id\":2047,\"rx\":1,\"ms\":10,\"ageMs\":0,\"raw\":\"0102465200040000\",\"page\":1") != String::npos);
  assert(config.find("\"bus\":\"C\",\"id\":2047,\"rx\":1,\"ms\":10,\"ageMs\":0,\"raw\":\"03B1000000000000\",\"page\":3") != String::npos);
  assert(config.find("\"bus\":\"A\",\"id\":2047,\"rx\":0") != String::npos);
  assert(request("GET /api/canb/txtrace\n").find("\"frames\":[]") != String::npos);
  canBTxTraceLive[63] = {11, 100, 0x3F8, 8, 0, 0, {0x81, 0x28, 8, 0, 0x59, 0xDD, 0x9F, 0xA1}};
  canBTxTraceLive[0] = {12, 101, 0x293, 1, 0, -1, {0xFF}};
  canBTxTraceLiveCount = 2; canBTxTraceLiveHead = 1;
  const String trace = request("GET /api/canb/txtrace\n");
  assert(trace.find("\"seq\":11") < trace.find("\"seq\":12"));
  assert(trace.find("8128080059DD9FA1") != String::npos);
  assert(trace.find("\"result\":-1,\"raw\":\"FF\"") != String::npos);
  canBTxTraceLiveCount = CAN_B_TX_TRACE_CAPACITY;
  const String fullTrace = request("GET /api/canb/txtrace\n");
  assert(std::count(fullTrace.begin(), fullTrace.end(), '{') == CAN_B_TX_TRACE_CAPACITY + 2);
  assert(fullTrace.find("\"seq\":11") < fullTrace.find("\"seq\":12"));
  // A large trace may take >2 s while USB still makes steady progress.
  Serial.output.clear(); Serial.input = "GET /api/canb/txtrace\n"; Serial.shortWrite = 32;
  usbDiagTick();
  for (unsigned i = 0; i < 200; ++i) { now += 30; usbDiagTick(); }
  assert(Serial.output == fullTrace);
  Serial.shortWrite = 512;
  Serial.capacity = 0;
  assert(request("GET /api/system/stats\n").empty());
  const unsigned paused = snapshots;
  ticks(); assert(snapshots == paused && Serial.output.empty());
  Serial.capacity = 512; Serial.shortWrite = 3; ticks();
  assert(Serial.output == "{\"path\":\"/api/system/stats\",\"data\":{\"source\":\"systemStatsToJson\"}}\n");
  Serial.capacity = 0; request("GET /api/system/stats\n"); now += 2001; ticks();
  Serial.capacity = 512;
  assert(request("GET /api/features/status\n").find("v3FeaturePolicyJson") != String::npos);
  request("GET /api/profile/"); Serial.connected = false; ticks(); Serial.connected = true;
  assert(request("status\n").find("error") != String::npos);
  Serial.shortWrite = 512;
  const String replies = request("GET /api/das/stats\nGET /api/blinkA/stats\n");
  assert(std::count(replies.begin(), replies.end(), '\n') == 2);
  std::cout << "USB diagnostics: framing, routes, timing persistence/rollback, partial writes, backpressure, timeout and reconnect PASS\n";
}
