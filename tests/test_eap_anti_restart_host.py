"""Exercise cached 0x331 replay against mocked NVS and CAN boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using esp_err_t = int;
constexpr int ESP_OK = 0;
constexpr int ESP_ERR_INVALID_STATE = 0x103;
constexpr int ESP_ERR_TIMEOUT = 0x107;
constexpr int ESP_FAIL = 0x10B;

class String {
 public:
  String() = default;
  String(const char *v) : value_(v) {}
  String(const std::string &v) : value_(v) {}
  template <typename T> String(T v) {
    std::ostringstream out;
    out << v;
    value_ = out.str();
  }
  String &operator+=(const char *v) { value_ += v; return *this; }
  String &operator+=(const String &v) { value_ += v.value_; return *this; }
  size_t length() const { return value_.length(); }
  char operator[](size_t index) const { return value_[index]; }
  const std::string &value() const { return value_; }
 private:
  std::string value_;
};

struct Preferences {
  static std::map<std::string, std::string> store;
  bool begin(const char *, bool) { return true; }
  String getString(const char *key, const char *fallback) {
    const auto it = store.find(key);
    return it == store.end() ? String(fallback) : String(it->second);
  }
  size_t putString(const char *key, const String &value) {
    store[key] = value.value();
    return value.value().size();
  }
  void end() {}
};
std::map<std::string, std::string> Preferences::store;

struct twai_message_t {
  uint32_t identifier;
  uint8_t extd;
  uint8_t rtr;
  uint8_t data_length_code;
  uint8_t data[8];
};

bool profileSupported = true;
bool usesYlTopology = false;
bool chassis = true;
bool tlsscRestoreEnabled = false;
bool twaiReady = true;
bool canTxAdministrativeHold = false;
bool admissionOpen = true;
bool usbPassive = false;
uint32_t now = 0;
uint32_t txEpoch = 1;
esp_err_t txResult = ESP_OK;
std::vector<twai_message_t> sent;
std::vector<uint32_t> epochs;

uint32_t millis() { return now; }
bool activeProfileTlsscRestoreSupported() { return profileSupported; }
bool activeProfileUsesYlTopology() { return usesYlTopology; }
bool activeCanBIsChassis() { return chassis; }
bool canUsbPassive() { return usbPassive; }
bool twaiNonSummonAdmissionOpen() { return admissionOpen; }
uint32_t canTxEpochSnapshot() { return txEpoch; }
esp_err_t canTxTwaiTransmit(const twai_message_t *msg, uint32_t epoch) {
  sent.push_back(*msg);
  epochs.push_back(epoch);
  return txResult;
}

#include "eap_anti_restart.h"

void resetState() {
  Preferences::store.clear();
  profileSupported = true;
  usesYlTopology = false;
  chassis = true;
  tlsscRestoreEnabled = false;
  twaiReady = true;
  canTxAdministrativeHold = false;
  admissionOpen = true;
  usbPassive = false;
  now = 0;
  txEpoch = 1;
  txResult = ESP_OK;
  sent.clear();
  epochs.clear();
  memset(eapNrFrame, 0, sizeof(eapNrFrame));
  eapNrFrameLen = 0;
  eapNrFrameValid = false;
  eapNrPersisted = false;
  eapNrStockMs = 0;
  eapNrReplayOk = 0;
  eapNrReplayFail = 0;
  eapNrReplayBusy = 0;
  eapNrReplaySkip = 0;
  eapNrLastReplayMs = 0;
  eapNrLastErr = 0;
  eapNrArmedMs = 0;
  eapNrBurst = 0;
  eapNrWasEnabled = false;
}

twai_message_t stockFrame(uint8_t first = 0x00) {
  twai_message_t f = {};
  f.identifier = 0x331;
  f.data_length_code = 8;
  for (uint8_t i = 0; i < 8; i++) f.data[i] = (uint8_t)(0x10 + i);
  f.data[0] = first;
  return f;
}

int main() {
  resetState();
  Preferences::store[EAP_NR_KEY_FRAME] = "not-a-valid-frame";
  eapNrLoad();
  assert(!eapNrFrameValid && eapNrFrameLen == 0);
  Preferences::store[EAP_NR_KEY_FRAME] = "1B10111213141516";
  eapNrLoad();
  assert(eapNrFrameValid && eapNrFrameLen == 8 && eapNrPersisted);
  assert(eapNrFrame[0] == 0x1B && eapNrFrame[7] == 0x16);

  resetState();
  twai_message_t stock = stockFrame(0x80);
  eapNrObserve331(stock, 100);
  assert(eapNrFrameValid && eapNrFrame[0] == 0x9B && eapNrStockMs == 100);
  assert(!eapNrPersisted && Preferences::store.empty());
  tlsscRestoreEnabled = true;
  now = 3000;
  eapNrTick();
  assert(sent.size() == 1 && sent[0].identifier == 0x331 && sent[0].data[0] == 0x9B);
  assert(epochs[0] == 1 && eapNrReplayOk == 1 && eapNrBurst == 1);
  assert(eapNrPersisted && Preferences::store[EAP_NR_KEY_FRAME] == "9B11121314151617");

  resetState();
  tlsscRestoreEnabled = true;
  eapNrFrameValid = true;
  eapNrFrameLen = 8;
  eapNrFrame[0] = 0x1B;
  eapNrPersisted = true;
  eapNrStockMs = 1000;
  now = 2999;
  eapNrTick();
  assert(sent.empty());
  now = 3000;
  eapNrTick();
  assert(sent.size() == 1);
  now = 3099;
  eapNrTick();
  assert(sent.size() == 1);
  txResult = ESP_ERR_TIMEOUT;
  now = 3100;
  eapNrTick();
  assert(sent.size() == 2 && eapNrReplayBusy == 1 && eapNrBurst == 1);
  assert(eapNrLastReplayMs == 3000 && eapNrLastErr == ESP_ERR_TIMEOUT);
  txResult = ESP_OK;
  now = 3199;
  eapNrTick();
  assert(sent.size() == 3 && eapNrBurst == 2 && eapNrReplayOk == 2);
  assert(eapNrLastReplayMs == 3199);
  now = 3200;
  eapNrTick();
  assert(sent.size() == 3);

  resetState();
  tlsscRestoreEnabled = true;
  eapNrFrameValid = true;
  eapNrFrameLen = 8;
  eapNrPersisted = true;
  eapNrBurst = EAP_NR_MAX_FRAMES;
  eapNrTick();
  assert(sent.empty());
  eapNrBurst = 0;
  eapNrArmedMs = 1;
  now = EAP_NR_WINDOW_MS + 2;
  eapNrTick();
  assert(sent.empty());

  resetState();
  tlsscRestoreEnabled = true;
  eapNrFrameValid = true;
  eapNrFrameLen = 8;
  eapNrPersisted = true;
  canTxAdministrativeHold = true;
  eapNrTick();
  assert(sent.empty());
  canTxAdministrativeHold = false;
  usbPassive = true;
  eapNrTick();
  assert(sent.empty());
  usbPassive = false;
  usesYlTopology = true;
  eapNrTick();
  assert(sent.empty());
  usesYlTopology = false;
  twaiReady = false;
  eapNrTick();
  assert(sent.empty());
  twaiReady = true;
  admissionOpen = false;
  eapNrTick();
  assert(sent.empty());

  resetState();
  tlsscRestoreEnabled = true;
  now = 1;
  eapNrTick();
  assert(sent.empty() && eapNrReplaySkip == 1);
  eapNrFrameValid = true;
  eapNrFrameLen = 8;
  eapNrPersisted = true;
  eapNrTick();
  assert(sent.size() == 1);
  tlsscRestoreEnabled = false;
  now = 200;
  eapNrTick();
  tlsscRestoreEnabled = true;
  now = 300;
  eapNrTick();
  assert(sent.size() == 2 && eapNrArmedMs == 300 && eapNrBurst == 1);

  resetState();
  tlsscRestoreEnabled = true;
  now = 1;
  eapNrFrameValid = true;
  eapNrFrameLen = 8;
  eapNrFrame[0] = 0x1B;
  eapNrPersisted = true;
  txResult = ESP_ERR_INVALID_STATE;
  eapNrTick();
  assert(eapNrReplayFail == 1 && eapNrBurst == 1 && eapNrLastErr == ESP_ERR_INVALID_STATE);
  String json = "{\"ok\":true";
  eapNrStatsAppend(json);
  json += "}";
  const std::string &text = json.value();
  assert(text.find("\"cacheValid\":true") != std::string::npos);
  assert(text.find("\"cacheHex\":\"1B00000000000000\"") != std::string::npos);
  assert(text.find("\"replayFail\":1") != std::string::npos);
  assert(text.find("\"lastErrName\":\"gate-closed\"") != std::string::npos);
}
'''

with tempfile.TemporaryDirectory(prefix='eap-anti-restart-') as directory:
    temp = Path(directory)
    (temp / 'Arduino.h').write_text('#pragma once\n', encoding='utf-8')
    source = temp / 'test.cpp'
    executable = temp / 'test.exe'
    source.write_text(code, encoding='utf-8')
    subprocess.run([
        os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I', str(temp), '-I', str(ROOT), str(source), '-o', str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)

ino = (ROOT / 'T2CAN-Universal-v3.30.3-LP_YL.ino').read_text(encoding='utf-8')
runtime = (ROOT / 'can_runtime.h').read_text(encoding='utf-8')
web = (ROOT / 'web_api.h').read_text(encoding='utf-8')
assert ino.index('featureCfgLoad();') < ino.index('eapNrLoad();')
assert 'eapNrObserve331(f, stockRxMs);' in runtime
assert 'tsl9InputServiceCanB();\n    eapNrTick();' in runtime
assert 'eapNrStatsAppend(json);' in web

print('eap anti-restart host: cache, replay, yield, limits, gates, diagnostics and hooks passed')
