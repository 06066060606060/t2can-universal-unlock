#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <cstdio>

enum { BOARD_T2CAN, BOARD_TMR };
static int board = BOARD_TMR;
static bool triple = true, profile = true, ulcNoConfirmEnabled = true;
static bool boardTripleCan() { return triple; }
static bool activeProfileUlcNoConfirmSupported() { return profile; }
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
class Preferences {
 public:
  inline static std::map<std::string, uint16_t> values;
  inline static bool available = true, writeOk = true;
  inline static unsigned writes;
  bool begin(const char *space, bool) { assert(!strcmp(space, "regionLab")); return available; }
  bool isKey(const char *key) { return values.count(key); }
  uint16_t getUShort(const char *key, uint16_t fallback) { return isKey(key) ? values[key] : fallback; }
  bool getBool(const char *key, bool fallback) { return getUShort(key, fallback); }
  size_t putUShort(const char *key, uint16_t value) {
    ++writes;
    if (!writeOk) return 0;
    values[key] = value; return sizeof(value);
  }
  void end() {}
};
struct can_frame { uint32_t can_id; uint8_t can_dlc, data[8]; };
struct twai_message_t { uint32_t identifier; uint8_t data_length_code, data[8]; };
namespace MCP2515 { enum ERROR { ERROR_OK, ERROR_FAIL }; }
static bool admission = true, attempted = true, sendOk = true;
static unsigned sends, lastBus, mcpTxOk, mcpTxFail;
static uint8_t mcpTxFailConsecutive, lastPayload[8];
static uint32_t canTxEpochSnapshot() { return 7; }
static bool twaiNonSummonAdmissionOpen() { return admission; }
static int canTxTwaiTransmit(const twai_message_t *frame, uint32_t epoch) {
  assert(epoch == 7 && frame->identifier == 0x238 && frame->data_length_code == 8);
  sends++; lastBus = 1; memcpy(lastPayload, frame->data, 8); return 0;
}
static bool canTxMcpSend(const can_frame *frame, uint32_t epoch, MCP2515::ERROR &err,
                         void *, bool party) {
  assert(epoch == 7 && frame->can_id == 0x238 && frame->can_dlc == 8);
  if (!attempted) return false;
  sends++; lastBus = party ? 2 : 0; memcpy(lastPayload, frame->data, 8);
  err = sendOk ? MCP2515::ERROR_OK : MCP2515::ERROR_FAIL; return true;
}
#include "../confirm_country.h"

int main() {
  confirmCountryLoad(); assert(confirmCountryValue() == 0);
  Preferences::values["countryKR"] = 1;
  confirmCountryLoad(); assert(confirmCountryValue() == 410 && confirmCountryActive() == 410);
  assert(confirmCountrySave(0));
  confirmCountry = 410; confirmCountryLoad(); assert(confirmCountryValue() == 0);
  assert(confirmCountrySave(840));
  confirmCountry = 0; confirmCountryLoad(); assert(confirmCountryValue() == 840);
  ulcNoConfirmEnabled = false; assert(confirmCountryActive() == 0);
  assert(confirmCountrySave(410)); // Selection may be saved while Confirm-Free is off.
  assert(confirmCountryActive() == 0); ulcNoConfirmEnabled = true;
  Preferences::writeOk = false;
  assert(!confirmCountrySave(124) && confirmCountryValue() == 410);
  Preferences::writeOk = true; Preferences::available = false;
  assert(!confirmCountrySave(124) && confirmCountryValue() == 410);
  Preferences::available = true;
  const unsigned writes = Preferences::writes;
  assert(!confirmCountrySave(999));
  board = BOARD_T2CAN; assert(!confirmCountrySave(124));
  confirmCountryLoad(); assert(confirmCountryActive() == 0);
  board = BOARD_TMR; triple = false; assert(!confirmCountrySave(124)); triple = true;
  profile = false; assert(!confirmCountrySave(124)); profile = true;
  assert(Preferences::writes == writes);
  const uint8_t stock[8] = {0, 0, 250, 0xFC, 0, 0, 0xF6, 0};
  for (unsigned bus = 0; bus < 3; ++bus) {
    const unsigned before = sends;
    confirmCountryObserve(bus, 0x238, 8, stock);
    assert(sends == before + 1 && lastBus == bus);
    assert((lastPayload[2] | ((lastPayload[3] & 3) << 8)) == 410);
    assert(stock[2] == 250); // Preserve the received stock template.
  }
  const unsigned before = sends;
  admission = false; confirmCountryObserve(1, 0x238, 8, stock); admission = true;
  attempted = false; confirmCountryObserve(0, 0x238, 8, stock); attempted = true;
  ulcNoConfirmEnabled = false; confirmCountryObserve(2, 0x238, 8, stock); ulcNoConfirmEnabled = true;
  board = BOARD_T2CAN; confirmCountryObserve(2, 0x238, 8, stock); board = BOARD_TMR;
  profile = false; confirmCountryObserve(2, 0x238, 8, stock); profile = true;
  confirmCountryObserve(3, 0x238, 8, stock);
  confirmCountryObserve(0, 0x238, 7, stock);
  confirmCountryObserve(0, 0x238, 8, nullptr);
  assert(sends == before && mcpTxOk == 2 && mcpTxFail == 0);
  sendOk = false; confirmCountryObserve(2, 0x238, 8, stock);
  assert(mcpTxFail == 1 && mcpTxFailConsecutive == 1);
  Preferences::values["country"] = 999; confirmCountryLoad(); assert(confirmCountryActive() == 0);
  puts("Confirm-Free runtime: NVS reboot/migration, rollback, toggle/profile gates and same-bus TX PASS");
}
