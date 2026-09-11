// T2CAN Universal v3.2 hotfix - Model 3/Y firmware

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>
#include <Update.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLESecurity.h>
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <esp_gap_ble_api.h>
#elif defined(CONFIG_NIMBLE_ENABLED)
#include <host/ble_gap.h>
#include <host/ble_store.h>
#endif
#include "driver/twai.h"
#include "pin_config.h"
#include <mcp2515.h>
#include <SPI.h>
#include "index_html.h"
#include "vehicle_profile.h"
#include "auto_blinker_pure.h"
#include "can_research_capture_pure.h"
#include "runtime_gate_pure.h"

#define FW_VERSION "v3.2 hotfix"

#include "t2can_core_state.h"
#include "t2can_forward.h"
#include "can_research_capture.h"
#include "s3xy_ble.h"
#include "can_core.h"
#include "vehicle_logic.h"
#include "web_api.h"
#include "can_runtime.h"

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  delay(100); // Serial settle only; CAN startup is not held here

  rtcBootCount++;
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.printf("\n=== T2CAN Unified BOOT ===\n");
  Serial.printf("Reset reason: %d (%s)\n", reset_reason, resetReasonName(reset_reason));
  Serial.printf("RTC boot count: %lu\n", (unsigned long)rtcBootCount);
  if (reset_reason == ESP_RST_BROWNOUT) {
    Serial.println("WARNING: Brownout detected!");
  }
  Serial.printf("IDF version: %s\n", esp_get_idf_version());

  // NVS init. Universal v3.x never auto-erases a configured profile because of an
  // initialization error. An explicit first-Universal migration or Factory
  // Reset is the only path that erases the full NVS partition.
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    vehicleProfileNvsError = true;
    vehicleProfileSetupMode = true;
    Serial.printf("NVS: Init failed %d (%s) -> SAFE SETUP MODE, no CAN/BLE\n",
                  (int)err, esp_err_to_name(err));
  } else {
    Preferences profileProbe;
    bool universalInitialized = false;
    uint8_t storedProfile = VEHICLE_PROFILE_NONE;
    if (profileProbe.begin(VEHICLE_PROFILE_NAMESPACE, true)) {
      universalInitialized = profileProbe.getBool(VEHICLE_UNIVERSAL_INIT_KEY, false);
      storedProfile = profileProbe.getUChar(VEHICLE_PROFILE_KEY, VEHICLE_PROFILE_NONE);
      vehicleProfileMigrationNotice = profileProbe.getBool(VEHICLE_MIGRATION_NOTICE_KEY, false);
      profileProbe.end();
    }

    if (!vehicleProfileValid(storedProfile)) {
      if (!universalInitialized) {
        Serial.println("Universal first boot: erasing previous NVS configuration...");
        if (nvs_flash_erase() == ESP_OK && nvs_flash_init() == ESP_OK &&
            vehicleProfileWriteBootstrapMarker(true)) {
          vehicleProfileMigrationNotice = true;
        } else {
          vehicleProfileNvsError = true;
          Serial.println("Universal migration erase/re-init failed -> SAFE SETUP MODE");
        }
      }
      vehicleProfileSetupMode = true;
    } else if (!vehicleProfileLoadFromNvs()) {
      vehicleProfileSetupMode = true;
      Serial.println("Vehicle profile invalid/incomplete -> SAFE SETUP MODE");
    }
  }

  canTxBarrierMutex = xSemaphoreCreateMutex();
  if (!canTxBarrierMutex) {
    Serial.println("CAN TX recovery barrier allocation failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // No valid profile means fail-closed setup mode: Wi-Fi/Web/OTA/profile
  // selection only. CAN controllers, CAN tasks, recovery supervisor and BLE
  // are not initialized.
  if (vehicleProfileSetupMode || vehicleProfileNvsError) {
    Serial.printf("PROFILE SETUP MODE profile=%u nvsError=%s migrationNotice=%s\n",
                  (unsigned)activeVehicleProfile, vehicleProfileNvsError ? "YES" : "NO",
                  vehicleProfileMigrationNotice ? "YES" : "NO");
    BaseType_t retWeb = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, &webTaskHandle, 0);
    if (retWeb != pdPASS) {
      Serial.printf("Web task creation failed in setup mode: %d\n", retWeb);
      delay(3000);
      ESP.restart();
    }
    Serial.println("BOOT SAFE SETUP MODE");
    return;
  }

  Serial.printf("Vehicle Profile=%s CAN A=%s CAN B=%s Turn=%s\n",
                vehicleProfileName(activeVehicleProfile),
                activeProfileCanAName(),
                activeProfileCanBName(),
                turnSignalVariantName(activeTurnSignalVariant));

  // Load configs. Universal v3.x starts from a full fresh NVS on first Universal boot,
  // so only the Universal schema/default interpretation is required here.
  nvsSchemaRead();
  featureCfgLoad();
  nagCfgLoad();
  summonCfgLoad();
  lab3f8CfgLoad();
  r79LabCfgLoad();
  s3xyAutoLoadConfig();
  nvsSchemaFinalize();
  if (labMenuEnabled) researchCaptureInit();
  else Serial.println("CAN Research Capture: LAB OFF · resources not allocated");

  Serial.printf("Nag mode=%u id=0x%03X torqueCount=%u enabled=%u\n",
    nagCfg.mode, nagCfg.targetId, nagCfg.torqueCount, nagCfg.enabled);
  Serial.println("Summon Monitor=ALWAYS_ON (state/capture/priority only)");
  Serial.printf("TLSSC enabled=%s highwayGate=%s (0x3FD mux0 bit38/39)\n",
                tlsscEnabled ? "true" : "false",
                tlsscHighwayGateEnabled ? "true" : "false");
  Serial.printf("LAB enabled=%s 0x3F8 alc=%u ulcBlind=%u acc=%u gate=AUTOSTEER_ONLY\n",
                labMenuEnabled ? "ON" : "OFF",
                (unsigned)lab3f8AlcMode, (unsigned)lab3f8UlcBlindMode,
                (unsigned)lab3f8AccFollowRaw);
  Serial.printf("R79 policy bit19=0 bit47=1 bit18Mode=%u period=%ums scheduler=INDEPENDENT park=ALWAYS_ON gate=AP_OR_SUMMON_OR_PARK\n",
                (unsigned)r79LabSmartMode, (unsigned)r79LabPeriodMs);
  Serial.printf("S3XY bluetooth=%s auto-connect=%s registry=%u/%u\n",
                s3xyBluetoothEnabled ? "ON" : "OFF",
                s3xyAutoEnabled ? "true" : "false",
                (unsigned)s3xyRegisteredCount(), (unsigned)S3XY_MAX_DEVICES);

  // Board power-on is treated as the wake signal for RX.
  // Existing per-feature validity gates still control every injection/TX path.
  Serial.println("Driver-wake power detected. Starting CAN init immediately...");

  // ══ Init CAN A (MCP2515) ══
  Serial.println("[CAN A] Initializing MCP2515...");
  pinMode(MCP2515_RST, OUTPUT);
  digitalWrite(MCP2515_RST, HIGH);
  delay(1);
  digitalWrite(MCP2515_RST, LOW);
  delay(2);
  digitalWrite(MCP2515_RST, HIGH);
  delay(2);

  SPI.begin(MCP2515_SCLK, MCP2515_MISO, MCP2515_MOSI, MCP2515_CS);
  mcpSpiStarted = true;

  if (!mcpInitChecked()) {
    Serial.println("[CAN A] MCP2515 init failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }
  Serial.printf("[CAN A] MCP2515 ready (500 kbps, clk=%s)\n",
                (MCP_CLOCK == MCP_16MHZ) ? "16MHz" :
                (MCP_CLOCK == MCP_8MHZ)  ? "8MHz" : "20MHz");

  // ══ Init CAN B (TWAI) ══
  Serial.println("[CAN B] Initializing TWAI...");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 256;
  g.tx_queue_len = TWAI_TX_QUEUE_LEN;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err1 = twai_driver_install(&g, &t, &f);
  esp_err_t err2 = twai_start();
  Serial.printf("[CAN B] TWAI: %s / %s\n", esp_err_to_name(err1), esp_err_to_name(err2));

  if (err1 != ESP_OK || err2 != ESP_OK) {
    Serial.println("[CAN B] TWAI init failed! Rebooting...");
    delay(3000);
    ESP.restart();
  }

  uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                              TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                              TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF |
                              TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL;
  if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
    Serial.println("[CAN B] TWAI alerts configured");
  }

  canInitTime = millis();
  twaiReady = true;
  bootCaptureMarkOnce(&bootCapCanInitDoneMs);

  // Start CAN tasks immediately after both controllers are ready/running.
  BaseType_t retMcp = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, &canTaskMcpHandle, 1);
  if (retMcp != pdPASS) {
    Serial.printf("CAN A task creation failed: %d\n", retMcp);
    delay(3000);
    ESP.restart();
  }

  BaseType_t retTwai = xTaskCreatePinnedToCore(canTaskTwai, "canB", 8192, nullptr, 4, &canTaskTwaiHandle, 1);
  if (retTwai != pdPASS) {
    Serial.printf("CAN B task creation failed: %d\n", retTwai);
    delay(3000);
    ESP.restart();
  }
  bootCaptureMarkOnce(&bootCapCanTasksStartedMs);

  BaseType_t retSup = xTaskCreatePinnedToCore(canSupervisorTask, "canSup", 6144, nullptr, 3, &canSupervisorHandle, 0);
  if (retSup != pdPASS) {
    Serial.printf("CAN supervisor task creation failed: %d\n", retSup);
    delay(3000);
    ESP.restart();
  }

  Serial.printf("[BOOT] CAN RX tasks started at %lu ms\n", (unsigned long)(millis() - bootTime));

  // Start Wi-Fi/web after the CAN receive path is live.
  BaseType_t retWeb = xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, &webTaskHandle, 0);
  if (retWeb != pdPASS) {
    Serial.printf("Web task creation failed: %d\n", retWeb);
    delay(3000);
    ESP.restart();
  }

  // S3XY BLE task runs independently on core 0. If Bluetooth Master is OFF it
  // never initializes BLEDevice at all. If ON, the mapper remains lazy with no
  // saved target and auto-initializes for saved targets or explicit commands.
  if (s3xyBluetoothEnabled) {
    BaseType_t retS3xy = xTaskCreatePinnedToCore(s3xyMapperTask, "s3xyMap", 8192, nullptr, 1, &s3xyTaskHandle, 0);
    if (retS3xy != pdPASS) {
      Serial.printf("S3XY mapper task creation failed: %d\n", retS3xy);
      // Mapping is diagnostic only; do not reboot or disturb proven CAN logic.
    }
  } else {
    s3xyTaskHandle = nullptr;
    Serial.println("S3XY: OFF · BLE task not started");
  }

  Serial.println("BOOT OK");
}

void loop() {
  static unsigned long lastBeatLog = 0;
  static uint32_t loopBeat = 0;
  loopBeat++;
  unsigned long now = millis();

  if (now - lastBeatLog >= 5000) {
    lastBeatLog = now;
    unsigned long canAgeMs = (lastCanFrameMs == 0) ? 999999 : (now - lastCanFrameMs);
    Serial.printf(
      "[BEAT] uptime=%lu loop=%lu canBeat=%lu canRxTotal=%lu webBeat=%lu canFrames=%lu canAgeMs=%lu mcpTxOk=%lu mcpTxFail=%lu sumTxOk=%lu sumTxFail=%lu heap=%u\n",
      now / 1000,
      (unsigned long)loopBeat,
      (unsigned long)canBeat,
      (unsigned long)canRxTotal(),
      (unsigned long)webBeat,
      (unsigned long)canRxTotal(),
      canAgeMs,
      (unsigned long)mcpTxOk,
      (unsigned long)mcpTxFail,
      (unsigned long)sumTxOk,
      (unsigned long)sumTxFail,
      ESP.getFreeHeap()
    );
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}
