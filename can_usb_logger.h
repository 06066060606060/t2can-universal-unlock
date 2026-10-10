#pragma once
#include "can_usb_logger_pure.h"
#include <esp_timer.h>

// Producers do no USB IO or allocation. All queue access is serialized below.
static CanUsbQueue<512> canUsbQueue;
static portMUX_TYPE canUsbMux = portMUX_INITIALIZER_UNLOCKED;
static bool canUsbCapturing = false;
static bool canUsbPassiveMode = false;
static bool canUsbTransitionHeld = false;
static bool canUsbModePending = false;
static bool canUsbModeResult = false;
static bool canUsbModeFinished = false;
static uint32_t canUsbControllerEpoch = 0;
static uint32_t canUsbMcpOverflowTotal = 0;
static TaskHandle_t canUsbTaskHandle = nullptr;

static bool canUsbPassive() {
  return __atomic_load_n(&canUsbPassiveMode, __ATOMIC_ACQUIRE);
}
static bool canUsbTxHeld() {
  return canUsbPassive() || __atomic_load_n(&canUsbTransitionHeld, __ATOMIC_ACQUIRE);
}
static bool canUsbSaveMode(bool passive) {
  Preferences prefs;
  if (!prefs.begin("usblog", false)) return false;
  const bool ok = prefs.putBool("passive", passive) == 1 &&
                  prefs.getBool("passive", !passive) == passive;
  prefs.end();
  return ok;
}
static void canUsbLoadMode() {
  Preferences prefs;
  // Failed configuration access chooses hardware listen-only until repaired.
  bool passive = true;
  if (prefs.begin("usblog", false)) {
    passive = prefs.isKey("passive") ? prefs.getBool("passive", true) : false;
    prefs.end();
  }
  __atomic_store_n(&canUsbPassiveMode, passive, __ATOMIC_RELEASE);
}

static void canUsbObserve(uint8_t bus, uint32_t id, uint8_t flags,
                          uint8_t dlc, const uint8_t *data) {
  if (!__atomic_load_n(&canUsbCapturing, __ATOMIC_ACQUIRE)) return;
  if (bus > 1 || flags > 3 || dlc > 8 ||
      id > ((flags & 1) ? 0x1FFFFFFFUL : 0x7FFUL)) return;
  CanUsbFrame frame = {};
  frame.bus = bus; frame.id = id; frame.flags = flags; frame.dlc = dlc;
  if (!(flags & 2) && data) memcpy(frame.data, data, dlc);
  portENTER_CRITICAL(&canUsbMux);
  // Timestamp and sequence share ordering across both receive tasks.
  frame.timestampUs = (uint64_t)esp_timer_get_time();
  canUsbQueue.observe(frame);
  portEXIT_CRITICAL(&canUsbMux);
}

static bool canUsbRequestMode(bool passive);
static void canUsbTask(void *);

static void canUsbStartTask() {
  if (!canUsbTaskHandle)
    xTaskCreatePinnedToCore(canUsbTask, "usbCAN", 4096, nullptr, 1,
                            &canUsbTaskHandle, 0);
}
