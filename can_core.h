#pragma once

// BOOT CAPTURE / CAN CORE / NAG
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// BOOT / FIRST-CAN TIMING CAPTURE
//
// Passive diagnostics only. These timestamps never participate in feature
// gating, CAN recovery decisions, or TX/injection. Capture starts
// automatically on every ESP32 boot and can be exported as CSV from:
//   /api/system/boot-capture.csv
// All timestamps are milliseconds from setup() entry (bootTime).
// ═══════════════════════════════════════════════════════════════

static constexpr uint32_t BOOT_CAPTURE_UNSET = 0xFFFFFFFFUL;
static portMUX_TYPE bootCaptureMux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t bootCapCanInitDoneMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapCanTasksStartedMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapWifiReadyMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstCanAMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstCanBMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirst370Ms = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirst370TorqueMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirst399Ms = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstParty24AMs = BOOT_CAPTURE_UNSET;
static volatile uint32_t bootCapFirstVh249Ms = BOOT_CAPTURE_UNSET;
static volatile uint16_t bootCapFirst370Raw = 0xFFFF;
static volatile uint16_t bootCapFirst370TorqueRaw = 0xFFFF;

struct BootHardReinitEvent {
  uint32_t startMs;
  uint32_t endMs;
  uint8_t reason;
  int8_t success; // -1=in progress, 0=failed, 1=success
};

static constexpr uint8_t BOOT_CAPTURE_HARD_MAX = 8;
static BootHardReinitEvent bootCapHard[BOOT_CAPTURE_HARD_MAX] = {};
static volatile uint8_t bootCapHardCount = 0;
static volatile uint32_t bootCapHardDropped = 0;

static inline uint32_t bootCaptureNowMs() {
  return (uint32_t)(millis() - bootTime);
}

static void bootCaptureMarkOnce(volatile uint32_t *slot) {
  // Fast path after the first event: no critical section on normal CAN traffic.
  if (*slot != BOOT_CAPTURE_UNSET) return;
  const uint32_t t = bootCaptureNowMs();
  portENTER_CRITICAL(&bootCaptureMux);
  if (*slot == BOOT_CAPTURE_UNSET) *slot = t;
  portEXIT_CRITICAL(&bootCaptureMux);
}

static void bootCaptureObservePartyFrame(uint16_t id, uint8_t dlc, const uint8_t *data) {
  bootCaptureMarkOnce(&bootCapFirstCanAMs);

  if (id == 0x399 && dlc >= 1)
    bootCaptureMarkOnce(&bootCapFirst399Ms);

  if (id == 0x24A && dlc >= 8)
    bootCaptureMarkOnce(&bootCapFirstParty24AMs);

  if (id == 0x370 && dlc >= 4 &&
      (bootCapFirst370Ms == BOOT_CAPTURE_UNSET ||
       bootCapFirst370TorqueMs == BOOT_CAPTURE_UNSET)) {
    const uint16_t raw = (uint16_t)(((data[2] & 0x0F) << 8) | data[3]);
    const uint32_t t = bootCaptureNowMs();
    const int32_t d = (int32_t)raw - 2050;
    portENTER_CRITICAL(&bootCaptureMux);
    if (bootCapFirst370Ms == BOOT_CAPTURE_UNSET) {
      bootCapFirst370Ms = t;
      bootCapFirst370Raw = raw;
    }
    // TORQUE (REAL) uses raw*0.01 - 20.5, so raw=2050 is 0.00 Nm.
    // Record the first frame at |torque| >= 0.10 Nm to distinguish
    // "0x370 arrived" from "a visibly non-zero torque arrived".
    if (bootCapFirst370TorqueMs == BOOT_CAPTURE_UNSET && (d >= 10 || d <= -10)) {
      bootCapFirst370TorqueMs = t;
      bootCapFirst370TorqueRaw = raw;
    }
    portEXIT_CRITICAL(&bootCaptureMux);
  }
}

static void bootCaptureObserveVhFrame(uint32_t id, uint8_t dlc) {
  bootCaptureMarkOnce(&bootCapFirstCanBMs);
  if (id == 0x249 && dlc >= 4)
    bootCaptureMarkOnce(&bootCapFirstVh249Ms);
}

static int8_t bootCaptureHardStart(uint8_t reason) {
  const uint32_t t = bootCaptureNowMs();
  int8_t idx = -1;
  portENTER_CRITICAL(&bootCaptureMux);
  if (bootCapHardCount < BOOT_CAPTURE_HARD_MAX) {
    idx = (int8_t)bootCapHardCount++;
    bootCapHard[idx].startMs = t;
    bootCapHard[idx].endMs = BOOT_CAPTURE_UNSET;
    bootCapHard[idx].reason = reason;
    bootCapHard[idx].success = -1;
  } else {
    bootCapHardDropped++;
  }
  portEXIT_CRITICAL(&bootCaptureMux);
  return idx;
}

static void bootCaptureHardFinish(int8_t idx, bool success) {
  if (idx < 0 || idx >= (int8_t)BOOT_CAPTURE_HARD_MAX) return;
  const uint32_t t = bootCaptureNowMs();
  portENTER_CRITICAL(&bootCaptureMux);
  bootCapHard[(uint8_t)idx].endMs = t;
  bootCapHard[(uint8_t)idx].success = success ? 1 : 0;
  portEXIT_CRITICAL(&bootCaptureMux);
}

static const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXTERNAL_RESET";
    case ESP_RST_SW:        return "SOFTWARE_RESET";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

// ═══════════════════════════════════════════════════════════════
// MCP2515 GLOBALS
// ═══════════════════════════════════════════════════════════════

static constexpr CAN_CLOCK MCP_CLOCK = MCP_16MHZ;

static constexpr uint32_t MCP_SPI_HZ = 10000000;

static constexpr uint8_t MCP_RX_BUDGET = 32;

static MCP2515 Can_A(MCP2515_CS, MCP_SPI_HZ, &SPI);
static volatile uint8_t  mcpState = 0;      // 0=OK, 1=WARN, 2=BUS-OFF
static volatile uint32_t mcpTxOk = 0;
static volatile uint32_t mcpTxFail = 0;
static volatile uint8_t  mcpTxFailConsecutive = 0;
static volatile uint32_t mcpRxCount = 0;
static volatile uint32_t mcpRxOverflowCount = 0;
static volatile uint32_t mcpRxOverflowLastMs = 0;
static volatile uint8_t  mcpRxOverflowLastFlags = 0;
static portMUX_TYPE mcpRxOverflowMux = portMUX_INITIALIZER_UNLOCKED;
static unsigned long lastMcpStatusMs = 0;
static unsigned long lastMcpRecoverMs = 0;
static volatile uint8_t mcpLastErrorFlags = 0;
static volatile uint32_t mcpLastErrorFlagsMs = 0;
static volatile uint32_t canAMcpBusOffCount = 0;
static volatile uint8_t canTxFreshMaskFast = 0;

static void mcpRxOverflowObserve(uint32_t now, uint8_t flags) {
  __atomic_fetch_add(&canUsbMcpOverflowTotal, 1U, __ATOMIC_RELAXED);
  portENTER_CRITICAL(&mcpRxOverflowMux);
  mcpRxOverflowCount++;
  mcpRxOverflowLastMs = now;
  mcpRxOverflowLastFlags = flags;
  portEXIT_CRITICAL(&mcpRxOverflowMux);
}

static void mcpRxOverflowSnapshot(uint32_t &count, uint32_t &lastMs, uint8_t &flags) {
  portENTER_CRITICAL(&mcpRxOverflowMux);
  count = mcpRxOverflowCount;
  lastMs = mcpRxOverflowLastMs;
  flags = mcpRxOverflowLastFlags;
  portEXIT_CRITICAL(&mcpRxOverflowMux);
}

static void mcpRxOverflowReset() {
  portENTER_CRITICAL(&mcpRxOverflowMux);
  mcpRxOverflowCount = 0;
  mcpRxOverflowLastMs = 0;
  mcpRxOverflowLastFlags = 0;
  portEXIT_CRITICAL(&mcpRxOverflowMux);
}

// All feature TX reaches the hardware through these two functions. The caller
// passes the recovery epoch captured before its authorization decision. The
// mutex makes epoch validation and the nonblocking hardware enqueue one atomic
// operation with respect to recovery-state invalidation.
static uint32_t canTxEpochSnapshot() {
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) return 0;
  const uint32_t epoch = canTxBarrierState.epoch;
  xSemaphoreGive(canTxBarrierMutex);
  return epoch;
}

static void canTxMarkFresh(uint8_t busBit) {
  // After a bus has marked the current recovery epoch fresh, normal traffic
  // avoids thousands of redundant semaphore operations per second.
  if ((__atomic_load_n(&canTxFreshMaskFast, __ATOMIC_ACQUIRE) & busBit) == busBit) return;
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) return;
  canTxBarrierMarkFreshPure(canTxBarrierState, busBit);
  __atomic_store_n(&canTxFreshMaskFast, canTxBarrierState.freshMask, __ATOMIC_RELEASE);
  xSemaphoreGive(canTxBarrierMutex);
}

// CAN B BUS-OFF diagnostic trace. Keep this deliberately small: every CAN B
// application TX attempt passes this single point, while the most recent window
// is frozen only when BUS_OFF is observed. No CAN RX frames are copied here.
enum CanTxTraceSource : uint8_t {
  CAN_TX_TRACE_SOURCE_DEFAULT = 0,
  CAN_TX_TRACE_SOURCE_AUTO_BLINKER = 1,
  CAN_TX_TRACE_SOURCE_S3XY_BUTTON = 2,
  CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB = 3,
  CAN_TX_TRACE_SOURCE_TLSSC = 4,
  CAN_TX_TRACE_SOURCE_TLSSC_CLEAR = 5,
  CAN_TX_TRACE_SOURCE_R79 = 6,
  CAN_TX_TRACE_SOURCE_R79_REINJECT = 7,
  CAN_TX_TRACE_SOURCE_DMS = 8,
  CAN_TX_TRACE_SOURCE_DISPLAY = 9
};

static constexpr uint8_t CAN_B_TX_TRACE_CAPACITY = 64;
struct CanBTxTraceEntry {
  uint32_t seq;
  uint32_t capturedMs;
  uint16_t id;
  uint8_t dlc;
  uint8_t source;
  int32_t result;
  uint8_t data[8];
};
static portMUX_TYPE canBTxTraceMux = portMUX_INITIALIZER_UNLOCKED;
static CanBTxTraceEntry canBTxTraceLive[CAN_B_TX_TRACE_CAPACITY] = {};
static CanBTxTraceEntry canBTxTraceFrozen[CAN_B_TX_TRACE_CAPACITY] = {};
static volatile uint8_t canBTxTraceLiveHead = 0;
static volatile uint8_t canBTxTraceLiveCount = 0;
static volatile uint8_t canBTxTraceFrozenCount = 0;
static volatile uint32_t canBTxTraceSeq = 0;
static volatile uint32_t canBTxTraceFrozenMs = 0;
static volatile uint32_t canBTxTraceFrozenBusOffOrdinal = 0;

static void canBTraceRecordTx(const twai_message_t *msg, esp_err_t result,
                              uint8_t traceSource = CAN_TX_TRACE_SOURCE_DEFAULT) {
  if (!msg) return;
  CanBTxTraceEntry e = {};
  e.seq = __atomic_add_fetch(&canBTxTraceSeq, 1U, __ATOMIC_RELAXED);
  e.capturedMs = (uint32_t)millis();
  e.id = (uint16_t)(msg->identifier & 0x7FFU);
  e.dlc = (uint8_t)min((uint8_t)8, (uint8_t)msg->data_length_code);
  e.source = traceSource;
  e.result = (int32_t)result;
  if (e.dlc) memcpy(e.data, msg->data, e.dlc);
  portENTER_CRITICAL(&canBTxTraceMux);
  canBTxTraceLive[canBTxTraceLiveHead] = e;
  canBTxTraceLiveHead = (uint8_t)((canBTxTraceLiveHead + 1U) % CAN_B_TX_TRACE_CAPACITY);
  if (canBTxTraceLiveCount < CAN_B_TX_TRACE_CAPACITY) canBTxTraceLiveCount++;
  portEXIT_CRITICAL(&canBTxTraceMux);
}

static void canBTraceReset() {
  portENTER_CRITICAL(&canBTxTraceMux);
  memset(canBTxTraceLive, 0, sizeof(canBTxTraceLive));
  memset(canBTxTraceFrozen, 0, sizeof(canBTxTraceFrozen));
  canBTxTraceLiveHead = 0;
  canBTxTraceLiveCount = 0;
  canBTxTraceFrozenCount = 0;
  canBTxTraceFrozenMs = 0;
  canBTxTraceFrozenBusOffOrdinal = 0;
  portEXIT_CRITICAL(&canBTxTraceMux);
}

// CAN A forensic trace mirrors the CAN B BUS-OFF trace but records only
// application TX attempts that pass through canTxMcpSend(). It does not copy
// CAN A RX traffic and has no effect on TX admission or recovery decisions.
static constexpr uint8_t CAN_A_TX_TRACE_CAPACITY = 64;
struct CanATxTraceEntry {
  uint32_t seq;
  uint32_t capturedMs;
  uint16_t id;
  uint8_t dlc;
  uint8_t source;
  uint8_t reason;
  int32_t result;
  uint8_t data[8];
};
struct CanABusOffSnapshot {
  bool valid;
  uint32_t capturedMs;
  uint8_t eflg;
  uint8_t txFailConsecutive;
  uint32_t rxAgeMs;
  uint32_t rxOverflowCount;
  uint32_t txOk;
  uint32_t txFail;
};
static portMUX_TYPE canATxTraceMux = portMUX_INITIALIZER_UNLOCKED;
static CanATxTraceEntry canATxTraceLive[CAN_A_TX_TRACE_CAPACITY] = {};
static CanATxTraceEntry canATxTraceFrozen[CAN_A_TX_TRACE_CAPACITY] = {};
static CanABusOffSnapshot canALastBusOffSnapshot = {};
static volatile uint8_t canATxTraceLiveHead = 0;
static volatile uint8_t canATxTraceLiveCount = 0;
static volatile uint8_t canATxTraceFrozenCount = 0;
static volatile uint32_t canATxTraceSeq = 0;
static volatile uint32_t canATxTraceFrozenMs = 0;
static volatile uint32_t canATxTraceFrozenBusOffOrdinal = 0;

static void canATraceRecordTx(const struct can_frame *msg, McpTxResultReason reason,
                              MCP2515::ERROR result,
                              uint8_t traceSource = CAN_TX_TRACE_SOURCE_DEFAULT) {
  if (!msg) return;
  CanATxTraceEntry e = {};
  e.seq = __atomic_add_fetch(&canATxTraceSeq, 1U, __ATOMIC_RELAXED);
  e.capturedMs = (uint32_t)millis();
  e.id = (uint16_t)(msg->can_id & 0x7FFU);
  e.dlc = (uint8_t)min((uint8_t)8, (uint8_t)msg->can_dlc);
  e.source = traceSource;
  e.reason = (uint8_t)reason;
  e.result = (int32_t)result;
  if (e.dlc) memcpy(e.data, msg->data, e.dlc);
  portENTER_CRITICAL(&canATxTraceMux);
  canATxTraceLive[canATxTraceLiveHead] = e;
  canATxTraceLiveHead = (uint8_t)((canATxTraceLiveHead + 1U) % CAN_A_TX_TRACE_CAPACITY);
  if (canATxTraceLiveCount < CAN_A_TX_TRACE_CAPACITY) canATxTraceLiveCount++;
  portEXIT_CRITICAL(&canATxTraceMux);
}

static void canATraceReset() {
  portENTER_CRITICAL(&canATxTraceMux);
  memset(canATxTraceLive, 0, sizeof(canATxTraceLive));
  memset(canATxTraceFrozen, 0, sizeof(canATxTraceFrozen));
  canATxTraceLiveHead = 0;
  canATxTraceLiveCount = 0;
  canATxTraceFrozenCount = 0;
  canATxTraceFrozenMs = 0;
  canATxTraceFrozenBusOffOrdinal = 0;
  canALastBusOffSnapshot = {};
  portEXIT_CRITICAL(&canATxTraceMux);
  canAMcpBusOffCount = 0;
}

static volatile bool canTxAdministrativeHold = false;
static bool canTxAdministrativeRequested = false;
// Sticky until reboot: configuration handlers cannot release maintenance hold.
static bool canMaintenanceRequested = false;
static bool canMaintenanceSupervisorParked = false;
static bool canMaintenancePreparing = false;
static bool canMaintenanceStopped = false;
// Owned by canMaintenancePreparing; sticky until reboot, including failed retries.
static bool canMaintenanceObservedCanRuntime = false;
static bool canMaintenanceActive() {
  return __atomic_load_n(&canMaintenanceRequested, __ATOMIC_ACQUIRE);
}

// Administrative TX hold used by profile/reset/feature transitions.
// Take the same barrier mutex used by all application TX paths so enabling
// the hold waits for any in-flight application send to leave the critical
// section before the caller mutates persistent/runtime state. Senders also
// re-check the hold after acquiring the mutex, so no new application TX can
// cross an active hold. Disabling resumes the existing epoch/freshness state;
// callers that require invalidation explicitly invoke the recovery barrier.
static void setCanTxAdministrativeHold(bool hold) {
  if (!canTxBarrierMutex) {
    canTxAdministrativeRequested = hold;
    canTxAdministrativeHold = hold || canMaintenanceActive() || canUsbTxHeld();
    return;
  }
  if (xSemaphoreTake(canTxBarrierMutex, portMAX_DELAY) == pdTRUE) {
    canTxAdministrativeRequested = hold;
    canTxAdministrativeHold = hold || canMaintenanceActive() || canUsbTxHeld();
    xSemaphoreGive(canTxBarrierMutex);
  }
}

static esp_err_t canTxTwaiTransmitWithMaskTagged(
    const twai_message_t *msg, uint32_t expectedEpoch, uint8_t requiredFreshMask,
    uint8_t traceSource) {
  if (!msg) return ESP_ERR_INVALID_ARG;
  if (canTxAdministrativeHold) {
    canBTraceRecordTx(msg, ESP_ERR_INVALID_STATE, traceSource);
    return ESP_ERR_INVALID_STATE;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    canBTraceRecordTx(msg, ESP_ERR_TIMEOUT, traceSource);
    return ESP_ERR_TIMEOUT;
  }
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (!canTxAdministrativeHold && twaiReady &&
      canTxBarrierAllowsMaskedPure(canTxBarrierState, expectedEpoch, requiredFreshMask))
    err = twai_transmit(msg, 0);
  xSemaphoreGive(canTxBarrierMutex);
  canBTraceRecordTx(msg, err, traceSource);
  return err;
}

// Feature-local cancellation guard. Advancing a generation takes the same
// barrier as enqueue, so after cancellation returns no decision prepared under
// the previous generation can reach TWAI.
static uint32_t canTxCancellationGenerationSnapshot(
    const volatile uint32_t *generation) {
  return generation
      ? __atomic_load_n(generation, __ATOMIC_ACQUIRE)
      : 0u;
}

static void canTxCancellationGenerationAdvance(
    volatile uint32_t *generation) {
  if (!generation) return;
  if (!canTxBarrierMutex) {
    __atomic_add_fetch(generation, 1u, __ATOMIC_ACQ_REL);
    return;
  }
  if (xSemaphoreTake(canTxBarrierMutex, portMAX_DELAY) == pdTRUE) {
    __atomic_add_fetch(generation, 1u, __ATOMIC_ACQ_REL);
    xSemaphoreGive(canTxBarrierMutex);
  }
}

static esp_err_t canTxTwaiTransmitWithMaskTaggedGuarded(
    const twai_message_t *msg, uint32_t expectedEpoch,
    uint8_t requiredFreshMask, uint8_t traceSource,
    const volatile uint32_t *generation, uint32_t expectedGeneration) {
  if (!msg || !generation) return ESP_ERR_INVALID_ARG;
  if (canTxAdministrativeHold) {
    canBTraceRecordTx(msg, ESP_ERR_INVALID_STATE, traceSource);
    return ESP_ERR_INVALID_STATE;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    canBTraceRecordTx(msg, ESP_ERR_TIMEOUT, traceSource);
    return ESP_ERR_TIMEOUT;
  }
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (!canTxAdministrativeHold && twaiReady &&
      canTxCancellationGenerationSnapshot(generation) == expectedGeneration &&
      canTxBarrierAllowsMaskedPure(
          canTxBarrierState, expectedEpoch, requiredFreshMask))
    err = twai_transmit(msg, 0);
  xSemaphoreGive(canTxBarrierMutex);
  canBTraceRecordTx(msg, err, traceSource);
  return err;
}

static esp_err_t canTxTwaiTransmitWithMask(
    const twai_message_t *msg, uint32_t expectedEpoch, uint8_t requiredFreshMask) {
  return canTxTwaiTransmitWithMaskTagged(
      msg, expectedEpoch, requiredFreshMask, CAN_TX_TRACE_SOURCE_DEFAULT);
}

static esp_err_t canTxTwaiTransmitTagged(
    const twai_message_t *msg, uint32_t expectedEpoch, uint8_t traceSource) {
  return canTxTwaiTransmitWithMaskTagged(
      msg, expectedEpoch, CAN_TX_FRESH_BOTH, traceSource);
}

static esp_err_t canTxTwaiTransmit(
    const twai_message_t *msg, uint32_t expectedEpoch) {
  return canTxTwaiTransmitWithMask(msg, expectedEpoch, CAN_TX_FRESH_BOTH);
}

// Final feature admission and payload composition share the enqueue barrier.
static esp_err_t canTxTwaiTransmitValidated(
    twai_message_t *msg, uint32_t expectedEpoch, uint8_t requiredFreshMask,
    bool (*validate)(twai_message_t *, void *), void *context) {
  if (!msg || !validate) return ESP_ERR_INVALID_ARG;
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    canBTraceRecordTx(msg, ESP_ERR_TIMEOUT, CAN_TX_TRACE_SOURCE_DEFAULT);
    return ESP_ERR_TIMEOUT;
  }
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (!canTxAdministrativeHold && twaiReady &&
      canTxBarrierAllowsMaskedPure(canTxBarrierState, expectedEpoch, requiredFreshMask) &&
      validate(msg, context))
    err = twai_transmit(msg, 0);
  xSemaphoreGive(canTxBarrierMutex);
  canBTraceRecordTx(msg, err, CAN_TX_TRACE_SOURCE_DEFAULT);
  return err;
}

static bool canTxMcpSendTagged(const struct can_frame *msg,
                               uint32_t expectedEpoch, uint8_t traceSource,
                               MCP2515::ERROR &errOut,
                               McpTxResultReason *reasonOut) {
  if (reasonOut) *reasonOut = MCP_TX_INVALID_MSG;
  if (!msg) return false;
  if (canTxAdministrativeHold) {
    if (reasonOut) *reasonOut = MCP_TX_EPOCH_MISMATCH;
    canATraceRecordTx(msg, MCP_TX_EPOCH_MISMATCH, MCP2515::ERROR_FAIL,
                      traceSource);
    return false;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    if (reasonOut) *reasonOut = MCP_TX_MUTEX_BUSY;
    canATraceRecordTx(msg, MCP_TX_MUTEX_BUSY, MCP2515::ERROR_FAIL,
                      traceSource);
    return false;
  }

  McpTxResultReason reason = canTxAdministrativeHold ? MCP_TX_EPOCH_MISMATCH : mcpTxResultReasonPure(
      true, true, mcpReady, canTxBarrierState.epoch, canTxBarrierState.freshMask,
      expectedEpoch, true);
  const bool allowed = reason == MCP_TX_OK;
  MCP2515::ERROR traceResult = MCP2515::ERROR_FAIL;

  if (allowed) {
    errOut = Can_A.sendMessage(msg);
    traceResult = errOut;
    reason = mcpTxResultReasonPure(
        true, true, true, canTxBarrierState.epoch, canTxBarrierState.freshMask,
        expectedEpoch, errOut == MCP2515::ERROR_OK);
  }
  if (reasonOut) *reasonOut = reason;
  xSemaphoreGive(canTxBarrierMutex);
  canATraceRecordTx(msg, reason, traceResult, traceSource);
  return allowed;
}

static bool canTxMcpSend(const struct can_frame *msg, uint32_t expectedEpoch,
                         MCP2515::ERROR &errOut, McpTxResultReason *reasonOut) {
  return canTxMcpSendTagged(msg, expectedEpoch, CAN_TX_TRACE_SOURCE_DEFAULT,
                            errOut, reasonOut);
}

static bool canTxMcpSendTaggedGuarded(
    const struct can_frame *msg, uint32_t expectedEpoch, uint8_t traceSource,
    const volatile uint32_t *generation, uint32_t expectedGeneration,
    MCP2515::ERROR &errOut, McpTxResultReason *reasonOut) {
  if (reasonOut) *reasonOut = MCP_TX_INVALID_MSG;
  if (!msg || !generation) return false;
  if (canTxAdministrativeHold) {
    if (reasonOut) *reasonOut = MCP_TX_EPOCH_MISMATCH;
    canATraceRecordTx(msg, MCP_TX_EPOCH_MISMATCH, MCP2515::ERROR_FAIL,
                      traceSource);
    return false;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    if (reasonOut) *reasonOut = MCP_TX_MUTEX_BUSY;
    canATraceRecordTx(msg, MCP_TX_MUTEX_BUSY, MCP2515::ERROR_FAIL,
                      traceSource);
    return false;
  }

  McpTxResultReason reason = MCP_TX_EPOCH_MISMATCH;
  const bool generationValid =
      canTxCancellationGenerationSnapshot(generation) == expectedGeneration;
  if (generationValid) {
    reason = canTxAdministrativeHold ? MCP_TX_EPOCH_MISMATCH : mcpTxResultReasonPure(
        true, true, mcpReady && twaiReady, canTxBarrierState.epoch,
        canTxBarrierState.freshMask, expectedEpoch, true);
  }
  const bool allowed = generationValid && reason == MCP_TX_OK;
  MCP2515::ERROR traceResult = MCP2515::ERROR_FAIL;
  if (allowed) {
    errOut = Can_A.sendMessage(msg);
    traceResult = errOut;
    reason = mcpTxResultReasonPure(
        true, true, true, canTxBarrierState.epoch,
        canTxBarrierState.freshMask, expectedEpoch,
        errOut == MCP2515::ERROR_OK);
  }
  if (reasonOut) *reasonOut = reason;
  xSemaphoreGive(canTxBarrierMutex);
  canATraceRecordTx(msg, reason, traceResult, traceSource);
  return allowed;
}

// ═══════════════════════════════════════════════════════════════
// CAN RECOVERY SUPERVISOR
//
// IMPORTANT: this block does NOT participate in NAG / Summon / TLSSC / Advanced EAP /
// Auto Blinker feature logic remains authoritative for its gating.
// It only monitors CAN controller/task liveness and recreates the CAN
// subsystem when acquisition or wake recovery fails.
// ═══════════════════════════════════════════════════════════════

enum CanSupervisorCommand : uint8_t {
  CAN_SUP_NONE = 0,
  CAN_SUP_HARD_ACQUIRE = 1,
  CAN_SUP_HARD_STALE = 2,
  CAN_SUP_HARD_MANUAL = 3
};

enum CanRecoveryDiagnosticReason : uint8_t {
  CAN_REC_NONE = 0,
  CAN_REC_TWAI_BUS_OFF = 1,
  CAN_REC_TWAI_RECOVERY_FAIL = 2,
  CAN_REC_TWAI_STOPPED = 3,
  CAN_REC_TWAI_RESTART_FAIL = 4,
  CAN_REC_ONE_BUS_STALE = 5,
  CAN_REC_WAKE_ACQUIRE_TIMEOUT = 6,
  CAN_REC_COLD_ACQUIRE_TIMEOUT = 7,
  CAN_REC_TASK_HEARTBEAT_TIMEOUT = 8,
  CAN_REC_MANUAL = 9,
  CAN_REC_MCP_REINIT_FAIL = 10
};

static inline const char *canRecoveryDiagnosticReasonName(uint8_t reason) {
  switch (reason) {
    case CAN_REC_TWAI_BUS_OFF: return "TWAI_BUS_OFF";
    case CAN_REC_TWAI_RECOVERY_FAIL: return "TWAI_RECOVERY_FAIL";
    case CAN_REC_TWAI_STOPPED: return "TWAI_STOPPED";
    case CAN_REC_TWAI_RESTART_FAIL: return "TWAI_RESTART_FAIL";
    case CAN_REC_ONE_BUS_STALE: return "ONE_BUS_STALE";
    case CAN_REC_WAKE_ACQUIRE_TIMEOUT: return "WAKE_ACQUIRE_TIMEOUT";
    case CAN_REC_COLD_ACQUIRE_TIMEOUT: return "COLD_ACQUIRE_TIMEOUT";
    case CAN_REC_TASK_HEARTBEAT_TIMEOUT: return "TASK_HEARTBEAT_TIMEOUT";
    case CAN_REC_MANUAL: return "MANUAL";
    case CAN_REC_MCP_REINIT_FAIL: return "MCP_REINIT_FAIL";
    default: return "NONE";
  }
}

struct CanTwaiRecoverySnapshot {
  bool valid;
  uint8_t state;
  uint32_t capturedMs;
  uint32_t rxGapMs;
  uint32_t msgsToTx;
  uint32_t msgsToRx;
  uint32_t txErrorCounter;
  uint32_t rxErrorCounter;
  uint32_t txFailedCount;
  uint32_t rxMissedCount;
  uint32_t rxOverrunCount;
  uint32_t arbLostCount;
  uint32_t busErrorCount;
  uint32_t alertSeenMask;
  uint32_t alertBatchMask;
  uint32_t txFailedAlertAgeMs;
  uint32_t errPassAlertAgeMs;
  uint32_t busErrorAlertAgeMs;
};

struct CanTwaiErrorAlertWindow {
  uint32_t seenMask;
  uint32_t lastTxFailedMs;
  uint32_t lastErrPassMs;
  uint32_t lastBusErrorMs;
};

struct CanTwaiBusOffAlertEvidence {
  bool valid;
  uint32_t seenMask;
  uint32_t batchMask;
  uint32_t txFailedAgeMs;
  uint32_t errPassAgeMs;
  uint32_t busErrorAgeMs;
};

static portMUX_TYPE canRecoveryMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t canSupervisorCommand = CAN_SUP_NONE;
static volatile bool canSubsystemBusy = false;
static volatile bool canTasksStopping = false;
static volatile bool canTaskMcpQuiesced = false;
static volatile bool canTaskTwaiQuiesced = false;
static TaskHandle_t canTaskMcpHandle = nullptr;
static TaskHandle_t canTaskTwaiHandle = nullptr;
static TaskHandle_t canSupervisorHandle = nullptr;
static TaskHandle_t webTaskHandle = nullptr;

static volatile uint32_t canTaskMcpHeartbeatMs = 0;
static volatile uint32_t canTaskTwaiHeartbeatMs = 0;
enum CanTaskHeartbeatTimeoutCause : uint8_t {
  CAN_TASK_HEARTBEAT_NONE = 0,
  CAN_TASK_HEARTBEAT_A = 1,
  CAN_TASK_HEARTBEAT_B = 2,
  CAN_TASK_HEARTBEAT_BOTH = 3
};
static inline const char *canTaskHeartbeatTimeoutCauseName(uint8_t cause) {
  switch (cause) {
    case CAN_TASK_HEARTBEAT_A: return "CAN_A";
    case CAN_TASK_HEARTBEAT_B: return "CAN_B";
    case CAN_TASK_HEARTBEAT_BOTH: return "BOTH";
    default: return "NONE";
  }
}
static volatile uint8_t canTaskHeartbeatLastCause = CAN_TASK_HEARTBEAT_NONE;
static volatile uint32_t canTaskHeartbeatLastAgeAms = 0;
static volatile uint32_t canTaskHeartbeatLastAgeBms = 0;
static volatile uint32_t canTaskHeartbeatTimeoutCountA = 0;
static volatile uint32_t canTaskHeartbeatTimeoutCountB = 0;
static volatile uint32_t canTaskHeartbeatTimeoutCountBoth = 0;
static volatile CanTaskLiveDiagnosticsPure canTaskMcpDiagnostics = {};
static volatile CanTaskLiveDiagnosticsPure canTaskTwaiDiagnostics = {};
static CanTaskTimeoutSnapshotPure canTaskHeartbeatLastSnapshotA = {};
static CanTaskTimeoutSnapshotPure canTaskHeartbeatLastSnapshotB = {};
static volatile CanARxDiagnosticsPure canARxDiagnostics = {};
static inline uint8_t canTaskStateCode(TaskHandle_t handle) {
  return handle ? (uint8_t)eTaskGetState(handle) : (uint8_t)eInvalid;
}
static inline const char *canTaskStateName(uint8_t state) {
  switch ((eTaskState)state) {
    case eRunning: return "RUNNING";
    case eReady: return "READY";
    case eBlocked: return "BLOCKED";
    case eSuspended: return "SUSPENDED";
    case eDeleted: return "DELETED";
    default: return "INVALID";
  }
}
static volatile uint32_t lastCanAFrameMs = 0;
static volatile uint32_t lastCanBFrameMs = 0;
static volatile uint32_t canHardReinitCount = 0;
static volatile uint32_t canHardReinitFailCount = 0;
static volatile uint8_t  canLastHardReinitReason = CAN_SUP_NONE; // legacy supervisor command
static volatile uint8_t  canPendingHardDiagReason = CAN_REC_NONE;
static volatile uint8_t  canLastHardDiagReason = CAN_REC_NONE;
static volatile uint32_t canRecoverySleepCount = 0;
static volatile uint32_t canRecoveryWakeCount = 0;

static volatile uint32_t canTwaiBusOffCount = 0;
static volatile uint32_t canTwaiStoppedCount = 0;
static volatile uint32_t canTwaiLocalRecoveryStartCount = 0;
static volatile uint32_t canTwaiRecoveryStartFailCount = 0;
static volatile uint32_t canTwaiRestartOkCount = 0;
static volatile uint32_t canTwaiRestartFailCount = 0;
static volatile uint8_t  canTwaiLastEventReason = CAN_REC_NONE;
static volatile uint32_t canTwaiLastEventMs = 0;
static volatile uint32_t canBLastRxGapMs = 0;
static volatile uint32_t canBMaxRxGapMs = 0;
static CanTwaiRecoverySnapshot canTwaiLastBusOffSnapshot = {};
static CanTwaiErrorAlertWindow canTwaiErrorAlertWindow = {};
static CanTwaiBusOffAlertEvidence canTwaiBusOffAlertEvidence = {};

static bool mcpSpiStarted = false;
static bool recoveryEverBothActive = false;
static bool recoverySleeping = false;
static uint32_t recoveryWakeAcquireStartMs = 0;
static uint32_t recoveryOneBusStaleStartMs = 0;
static uint32_t recoveryLastHardRequestMs = 0;
static uint32_t recoveryLastBothActiveMs = 0;
static uint8_t recoveryColdRetryCount = 0;
static bool recoveryColdRetriesExhausted = false;

static constexpr uint32_t RECOVERY_BUS_FRESH_MS = 3000;
static constexpr uint32_t RECOVERY_SLEEP_QUIET_MS = 5000;
static constexpr uint32_t RECOVERY_WAKE_ACQUIRE_MS = 5000;
static constexpr uint32_t RECOVERY_ONE_BUS_STALE_MS = 4000;
// Fast cold acquisition: CAN RX starts as soon as controllers are ready.
// Keep task-heartbeat startup grace separate so fast acquisition does not make
// the task watchdog unnecessarily aggressive.
static constexpr uint32_t RECOVERY_COLD_FIRST_ACQUIRE_MS = 2000;
static constexpr uint32_t RECOVERY_TASK_START_GRACE_MS = 5000;
static constexpr uint32_t RECOVERY_HARD_COOLDOWN_MS = 10000;
static constexpr uint32_t RECOVERY_COLD_RETRY_INTERVAL_MS = 15000;
static constexpr uint8_t  RECOVERY_COLD_MAX_RETRIES = 3;
static constexpr uint32_t RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS = 3000;
static constexpr uint32_t RECOVERY_TASK_STOP_SETTLE_MS = 50;
static constexpr uint32_t RECOVERY_TWAI_WAIT_MS = 1800;

static void requestCanSubsystemRestart(uint8_t reason, uint8_t diagReason);

// ═══════════════════════════════════════════════════════════════
// NAG ECHO (CAN A - MCP2515)
// ═══════════════════════════════════════════════════════════════

static const uint16_t NAG_TORQUE_RAW_MAX = 0x8B6;
static const uint16_t NAG_TORQUE_RAW_MIN = 0x74E;
static const uint8_t  NAG_MAX_TORQUE_ENTRIES = 8;
static const unsigned long NAG_INJECTION_DELAY_MS = 15000;
static constexpr uint16_t NAG_MODE_C_RAW_MIN = 0x898;
static constexpr uint16_t NAG_MODE_C_RAW_MAX = 0x8B6;
static constexpr uint32_t NAG_MODE_C_STEP_MS = 200;
static constexpr uint16_t NAG_SPEED_ID = 0x257;
static constexpr uint32_t NAG_SPEED_FRESH_MS = 1000;

enum NagMode : uint8_t {
  MODE_A = 0, MODE_B = 1,
  // IDs 2 and 4..6 were removed legacy/experimental modes. Keep the numeric
  // gaps so persisted/public IDs for Mode A/B/C/H remain unchanged.
  MODE_C = 3, MODE_H = 7
};

struct NagConfig {
  bool     enabled;
  bool     ignoreApState;
  uint8_t  method;
  uint8_t  tsl9Sequence;
  uint8_t  tsl9DowngradeWindow;
  uint8_t  tsl9InputMode;
  uint8_t  tsl9LegacyRoute;
  bool     pauseAtZeroSpeed;
  uint8_t  modeHStopBehavior;
  uint8_t  mode;
  uint16_t targetId;
  uint8_t  torqueCount;
  uint8_t  torqueB2[NAG_MAX_TORQUE_ENTRIES];
  uint8_t  torqueB3[NAG_MAX_TORQUE_ENTRIES];
  uint8_t  hoRatePct;
  uint16_t burstMs;
  uint16_t pauseMs;
  uint16_t apStateId;
  uint8_t  apStateByte;
  uint8_t  apStateShift;
  uint8_t  apStateMask;
  uint8_t  handsOnByte;
  uint8_t  handsOnShift;
  uint8_t  handsOnMask;
  uint16_t steeringId;
  uint8_t  steeringByteHi;
  uint8_t  steeringByteLo;
};

static NagConfig nagCfg;
static portMUX_TYPE nagCfgMux = portMUX_INITIALIZER_UNLOCKED;

// RX selector cache: Party CAN carries many unrelated frames. Keep the three
// configurable NAG selector IDs in a tiny lock-free read cache so unrelated RX
// frames can return before taking nagCfgMux. Writers publish under nagCfgMux;
// the sequence guard prevents readers from consuming a partially-updated set.
static uint32_t nagRxSelectorSeq = 0;
static uint16_t nagRxTargetId = 0x370;
static uint16_t nagRxApStateId = 0x399;
static uint16_t nagRxSteeringId = 0x129;

static inline void nagRxSelectorsPublishLocked(const NagConfig &c) {
  // GCC atomics avoid a C++ data race between the Core-0 web writer and the
  // Core-1 CAN reader. The odd/even sequence publishes the three IDs as one
  // coherent snapshot without putting the common RX path behind nagCfgMux.
  __atomic_add_fetch(&nagRxSelectorSeq, 1U, __ATOMIC_ACQ_REL);  // odd = writer active
  __atomic_store_n(&nagRxTargetId, c.targetId, __ATOMIC_RELAXED);
  __atomic_store_n(&nagRxApStateId, c.apStateId, __ATOMIC_RELAXED);
  __atomic_store_n(&nagRxSteeringId, c.steeringId, __ATOMIC_RELAXED);
  __atomic_add_fetch(&nagRxSelectorSeq, 1U, __ATOMIC_RELEASE);  // even = published
}

static inline void nagRxSelectorsSnapshot(uint16_t &targetId, uint16_t &apStateId, uint16_t &steeringId) {
  const uint32_t seq1 = __atomic_load_n(&nagRxSelectorSeq, __ATOMIC_ACQUIRE);
  targetId = __atomic_load_n(&nagRxTargetId, __ATOMIC_RELAXED);
  apStateId = __atomic_load_n(&nagRxApStateId, __ATOMIC_RELAXED);
  steeringId = __atomic_load_n(&nagRxSteeringId, __ATOMIC_RELAXED);
  const uint32_t seq2 = __atomic_load_n(&nagRxSelectorSeq, __ATOMIC_ACQUIRE);
  if ((seq1 & 1U) || seq1 != seq2) {
    // Config updates are rare. Fall back to the canonical protected copy
    // instead of spinning inside the high-priority CAN RX task.
    portENTER_CRITICAL(&nagCfgMux);
    targetId = nagCfg.targetId;
    apStateId = nagCfg.apStateId;
    steeringId = nagCfg.steeringId;
    portEXIT_CRITICAL(&nagCfgMux);
  }
}

static void nagCfgCommit(const NagConfig &c) {
  NagConfig normalized = c;
  nagCfgApplyActiveProfilePolicy(normalized, false);
  portENTER_CRITICAL(&nagCfgMux);
  nagCfg = normalized;
  nagRxSelectorsPublishLocked(nagCfg);
  portEXIT_CRITICAL(&nagCfgMux);
}

static void nagRxSelectorsRefreshFromConfig() {
  portENTER_CRITICAL(&nagCfgMux);
  nagRxSelectorsPublishLocked(nagCfg);
  portEXIT_CRITICAL(&nagCfgMux);
}

static bool nagTsl9Body39BSelected() {
  uint8_t route;
  portENTER_CRITICAL(&nagCfgMux);
  route = tsl9LegacyRouteSanitizePure(nagCfg.tsl9LegacyRoute);
  portEXIT_CRITICAL(&nagCfgMux);
  return vehicleProfileNagTsl9OnBody39B(
      activeVehicleProfile, activeVehicleTopology, route);
}

static bool nagTsl9Chassis399Selected() {
  return !activeProfileIsYl() && activeCanBIsChassis() &&
      activeProfileNagTsl9Supported() && !nagTsl9Body39BSelected();
}

struct NagContext {
  uint8_t  apState;
  uint8_t  handsOnState;
  uint8_t  prevHandsOnState;
  uint32_t visualWarningEpoch;
  unsigned long visualWarningEnterMs;
  bool     visualWarningActive;
  uint32_t scrollWarningEpoch;
  bool     scrollWarningActive;
  int16_t  steeringAngleDeciDeg;
  unsigned long lastApStateMs;
  unsigned long lastSteeringMs;
  unsigned long state2EnterMs;
  unsigned long state3EnterMs;
  uint16_t walkSeed;
  uint16_t modeCRaw;
  uint16_t vehicleSpeedRaw;
  bool     vehicleSpeedValid;
  unsigned long lastVehicleSpeedMs;
};

static NagContext nagCtx;
static portMUX_TYPE nagCtxMux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t nagRxFrames    = 0;
static volatile uint32_t nagEchoCount   = 0;
static volatile uint32_t nagEchoLatUs   = 0;
static volatile uint8_t  nagRealHo = 0;
static volatile int16_t  nagRealTorqueCenti = 0;
static volatile uint8_t  nagLastInjectedHo = 0;
static volatile int16_t  nagLastInjectedCenti = 0;
#if T2CAN_SERIAL_DIAGNOSTICS
static unsigned long nagLastTxFailLog = 0;
#endif

// b18 NAG continuity diagnostics. Dedicated counters avoid contamination by
// other MCP/Party injections such as the 0x399 LEFT experiment.
static portMUX_TYPE nagDiagMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t nagTxOk = 0;
static volatile uint32_t nagTxFail = 0;
static volatile uint32_t nagSkipDisabled = 0;
static volatile uint32_t nagSkipBootDelay = 0;
static volatile uint32_t nagSkipWarmup = 0;
static volatile uint32_t nagSkipSelfFrame = 0;
static volatile uint32_t nagSkipHandsOn = 0;
static volatile uint32_t nagSkipApInvalid = 0;
static volatile uint32_t nagSkipApInactive = 0;
static volatile uint32_t nagSkipDecision = 0;
static volatile uint32_t nagSkipStopped = 0;
static volatile uint32_t nagSkipSpeedStale = 0;
static volatile uint32_t nagStopCarrierTxOk = 0;
static volatile uint32_t nagBlockMutex = 0;
static volatile uint32_t nagBlockMcpNotReady = 0;
static volatile uint32_t nagBlockEpoch = 0;
static volatile uint32_t nagBlockFreshMask = 0;
static volatile uint32_t nagBlockInvalidMsg = 0;
static volatile uint32_t nagSendError = 0;
static volatile uint32_t nagLastTxOkMs = 0;
static volatile uint32_t nagMaxTxGapMs = 0;
static volatile uint32_t nagSessionStartMs = 0;
static volatile uint32_t nagSessionTxOk = 0;
static volatile uint8_t nagLastSkipReason = NAG_SKIP_NONE;
static volatile uint32_t nagLastSkipMs = 0;
static volatile uint8_t nagLastTxBlockReason = MCP_TX_OK;
static volatile uint32_t nagLastTxBlockMs = 0;
static portMUX_TYPE nagTsl9Mux = portMUX_INITIALIZER_UNLOCKED;
static Tsl9HandsOnStatePure nagTsl9State = {};
static volatile uint32_t nagTsl9Rx = 0;
static volatile uint32_t nagTsl9Modified = 0;
static volatile uint32_t nagTsl9HandsOnModified = 0;
static volatile uint32_t nagTsl9TxOk = 0;
static volatile uint32_t nagTsl9TxFail = 0;

static void nagTsl9RuntimeReset() {
  portENTER_CRITICAL(&nagTsl9Mux);
  nagTsl9State = {};
  portEXIT_CRITICAL(&nagTsl9Mux);
}
// Mode B burst/pause timing starts from AP engagement rather than boot uptime.
// This is intentionally independent of the Original mcpRxCount > 1000 warmup check.
static volatile uint32_t nagModeBPhaseStartMs = 0;

// Mode H uses exact recent-TX payload matching because WAIT/REFRACTORY can
// preserve arbitrary OEM torque while forcing HO=1. This state is isolated
// from the existing A/B/C torque-table self-frame heuristic.
static portMUX_TYPE nagExactEchoMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool nagExactEchoLastTxValid = false;
static volatile uint32_t nagExactEchoLastTxMs = 0;
static uint8_t nagExactEchoLastTxRaw[8] = {0};
static uint8_t nagExactEchoPrevMode = 0xFF;

static void nagExactEchoReset() {
  portENTER_CRITICAL(&nagExactEchoMux);
  nagExactEchoLastTxValid = false;
  nagExactEchoLastTxMs = 0;
  memset(nagExactEchoLastTxRaw, 0, sizeof(nagExactEchoLastTxRaw));
  portEXIT_CRITICAL(&nagExactEchoMux);
}

// Mode H uses the established Rev.4 engine; persisted id 1 and h4* tuning remain stable.
static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();
static portMUX_TYPE nagHumanMux = portMUX_INITIALIZER_UNLOCKED;
static NagHumanV4ConfigPure nagHumanV4Config = nagHumanV4DefaultConfigPure();
static NagHumanV4StatePure nagHumanV4State = {};
static bool nagHumanV4Initialized = false;

static uint32_t nagHumanRuntimeSeed() {
  uint32_t seed = (uint32_t)esp_random() ^ (uint32_t)micros() ^ 0x484D4F44u;
  return nagHumanV1SanitizeSeedPure(seed);
}

static uint8_t nagHumanVariantSnapshot() {
  portENTER_CRITICAL(&nagHumanMux);
  const uint8_t v = nagModeHVariantValidPure(nagHumanVariant) ? nagHumanVariant : nagModeHDefaultVariantPure();
  portEXIT_CRITICAL(&nagHumanMux);
  return v;
}

static void nagHumanRuntimeReset(bool reseed) {
  const uint32_t seed = reseed ? nagHumanRuntimeSeed() : 0u;
  portENTER_CRITICAL(&nagHumanMux);
  if (!nagHumanV4Initialized) {
    nagHumanV4InitPure(nagHumanV4State, reseed ? seed ^ 0x52345634u : 0x52345634u);
    nagHumanV4Initialized = true;
  } else if (reseed) {
    nagHumanV4ReseedPure(nagHumanV4State, seed ^ 0x52345634u);
  } else {
    nagHumanV4ResetRuntimePure(nagHumanV4State, H1_IDLE);
  }
  portEXIT_CRITICAL(&nagHumanMux);
}

static void nagHumanRuntimeSetVariant(uint8_t variant) {
  const uint8_t normalized = nagModeHVariantValidPure(variant) ? variant : nagModeHDefaultVariantPure();
  portENTER_CRITICAL(&nagHumanMux);
  nagHumanVariant = normalized;
  portEXIT_CRITICAL(&nagHumanMux);
  nagExactEchoReset();
  nagHumanRuntimeReset(true);
  if (normalized == H_VARIANT_REV4) {
    uint32_t currentVisualWarningEpoch;
    portENTER_CRITICAL(&nagCtxMux);
    currentVisualWarningEpoch = nagCtx.visualWarningEpoch;
    portEXIT_CRITICAL(&nagCtxMux);
    portENTER_CRITICAL(&nagHumanMux);
    nagHumanV4State.visualWarningEpochSeen = currentVisualWarningEpoch;
    nagHumanV4State.visualRescuePending = false;
    nagHumanV4State.visualRescuePendingSinceMs = 0u;
    portEXIT_CRITICAL(&nagHumanMux);
  }
}

static NagHumanV1StepResultPure nagHumanRuntimeStep(uint32_t nowMs,
                                                     uint16_t sourceRaw,
                                                     bool runAllowed,
                                                     bool speedValid,
                                                     bool speedFresh,
                                                     uint16_t speedRaw,
                                                     uint32_t visualWarningEpoch,
                                                     uint32_t visualWarningEnterMs,
                                                     bool visualWarningActive) {
  NagHumanV1StepResultPure result = {};
  portENTER_CRITICAL(&nagHumanMux);
{
    if (!nagHumanV4Initialized) {
      nagHumanV4InitPure(nagHumanV4State, nagHumanRuntimeSeed() ^ 0x52345634u);
      nagHumanV4Initialized = true;
    }
    result = nagHumanV4StepPure(nagHumanV4State, nagHumanV4Config, nowMs,
                                sourceRaw, runAllowed, speedValid, speedFresh, speedRaw,
                                visualWarningEpoch, visualWarningEnterMs, visualWarningActive);
  }
  portEXIT_CRITICAL(&nagHumanMux);
  return result;
}

static NagHumanV4StatePure nagHumanV4RuntimeSnapshot() {
  NagHumanV4StatePure snapshot = {};
  portENTER_CRITICAL(&nagHumanMux);
  snapshot = nagHumanV4State;
  portEXIT_CRITICAL(&nagHumanMux);
  return snapshot;
}

static NagHumanV4ConfigPure nagHumanV4RuntimeConfigSnapshot() {
  NagHumanV4ConfigPure snapshot = {};
  portENTER_CRITICAL(&nagHumanMux);
  snapshot = nagHumanV4Config;
  portEXIT_CRITICAL(&nagHumanMux);
  return snapshot;
}

static bool nagHumanV4RuntimeSetLabTuning(
    uint16_t peakMinRaw, uint16_t peakMaxRaw,
    uint16_t waitMinMs, uint16_t waitMaxMs,
    uint16_t refractoryMinMs, uint16_t refractoryMaxMs,
    uint16_t carrierMinRaw, uint16_t carrierMaxRaw,
    uint8_t hoPolicy, uint16_t ho1ThresholdRaw, uint16_t ho2ThresholdRaw,
    bool visualRescueEnabled, uint16_t visualRescueDelayMs) {
  NagHumanV4ConfigPure next = nagHumanV4RuntimeConfigSnapshot();
  next.base.peakMinRaw = peakMinRaw;
  next.base.peakMaxRaw = peakMaxRaw;
  next.base.waitMinMs = waitMinMs;
  next.base.waitMaxMs = waitMaxMs;
  next.base.refractoryMinMs = refractoryMinMs;
  next.base.refractoryMaxMs = refractoryMaxMs;
  next.base.hoOverridePct = 0u;
  next.carrierMinRaw = carrierMinRaw;
  next.carrierMaxRaw = carrierMaxRaw;
  next.hoPolicy = hoPolicy;
  next.ho1ThresholdRaw = ho1ThresholdRaw;
  next.ho2ThresholdRaw = ho2ThresholdRaw;
  next.visualRescueEnabled = visualRescueEnabled;
  next.visualRescueDelayMs = visualRescueDelayMs;
  if (!nagHumanV4ConfigValidPure(next)) return false;
  portENTER_CRITICAL(&nagHumanMux);
  const uint32_t visualWarningEpochSeen = nagHumanV4State.visualWarningEpochSeen;
  const uint32_t visualRescueCount = nagHumanV4State.visualRescueCount;
  nagHumanV4Config = next;
  nagHumanV4State = NagHumanV4StatePure{};
  nagHumanV4State.visualWarningEpochSeen = visualWarningEpochSeen;
  nagHumanV4State.visualRescueCount = visualRescueCount;
  nagHumanV4Initialized = false;
  portEXIT_CRITICAL(&nagHumanMux);
  nagExactEchoReset();
  return true;
}

static void nagHumanRuntimeLoadDefaults() {
  portENTER_CRITICAL(&nagHumanMux);
  nagHumanVariant = nagModeHDefaultVariantPure();
  nagHumanV4Config = nagHumanV4DefaultConfigPure();
  nagHumanV4State = NagHumanV4StatePure{};
  nagHumanV4Initialized = false;
  portEXIT_CRITICAL(&nagHumanMux);
}

static void nagHumanRuntimeApTransition(bool apActive, bool wasApActive) {
  if (apActive == wasApActive) return;
  uint8_t mode;
  bool ignoreApState;
  portENTER_CRITICAL(&nagCfgMux);
  mode = nagCfg.mode;
  ignoreApState = nagCfg.ignoreApState;
  portEXIT_CRITICAL(&nagCfgMux);
  if (mode == MODE_H && !ignoreApState) {
    nagExactEchoReset();
    nagHumanRuntimeReset(true);
  }
}

// ── Nag helpers ──

static inline const char* nagSkipReasonName(uint8_t reason) {
  switch (reason) {
    case NAG_SKIP_DISABLED: return "DISABLED";
    case NAG_SKIP_BOOT_DELAY: return "BOOT_DELAY";
    case NAG_SKIP_WARMUP: return "MCP_WARMUP";
    case NAG_SKIP_SELF_FRAME: return "SELF_FRAME";
    case NAG_SKIP_HANDS_ON: return "HANDS_ON_GT1";
    case NAG_SKIP_AP_INVALID: return "AP_INVALID";
    case NAG_SKIP_AP_INACTIVE: return "AP_INACTIVE";
    case NAG_SKIP_DECISION: return "MODE_DECISION";
    case NAG_SKIP_STOPPED: return "STOPPED_0_KPH";
    case NAG_SKIP_SPEED_STALE: return "SPEED_STALE";
    default: return "NONE";
  }
}

static inline const char* nagTxBlockReasonName(uint8_t reason) {
  switch (reason) {
    case MCP_TX_INVALID_MSG: return "INVALID_MSG";
    case MCP_TX_MUTEX_BUSY: return "MUTEX_BUSY";
    case MCP_TX_MCP_NOT_READY: return "MCP_NOT_READY";
    case MCP_TX_EPOCH_MISMATCH: return "EPOCH_MISMATCH";
    case MCP_TX_FRESH_MASK: return "FRESH_MASK";
    case MCP_TX_SEND_ERROR: return "SEND_ERROR";
    default: return "NONE";
  }
}

static void nagDiagRecordSkip(NagSkipReasonPure reason, uint32_t now) {
  if (reason == NAG_SKIP_NONE) return;
  portENTER_CRITICAL(&nagDiagMux);
  switch (reason) {
    case NAG_SKIP_DISABLED: nagSkipDisabled++; break;
    case NAG_SKIP_BOOT_DELAY: nagSkipBootDelay++; break;
    case NAG_SKIP_WARMUP: nagSkipWarmup++; break;
    case NAG_SKIP_SELF_FRAME: nagSkipSelfFrame++; break;
    case NAG_SKIP_HANDS_ON: nagSkipHandsOn++; break;
    case NAG_SKIP_AP_INVALID: nagSkipApInvalid++; break;
    case NAG_SKIP_AP_INACTIVE: nagSkipApInactive++; break;
    case NAG_SKIP_DECISION: nagSkipDecision++; break;
    case NAG_SKIP_STOPPED: nagSkipStopped++; break;
    case NAG_SKIP_SPEED_STALE: nagSkipSpeedStale++; break;
    default: break;
  }
  // Routine self-echo and startup/disabled skips would otherwise mask the last
  // driving-relevant interruption. Preserve them in counters, not Last Skip.
  if (reason == NAG_SKIP_HANDS_ON || reason == NAG_SKIP_AP_INVALID ||
      reason == NAG_SKIP_AP_INACTIVE || reason == NAG_SKIP_DECISION ||
      reason == NAG_SKIP_STOPPED || reason == NAG_SKIP_SPEED_STALE) {
    nagLastSkipReason = (uint8_t)reason;
    nagLastSkipMs = now;
  }
  portEXIT_CRITICAL(&nagDiagMux);
}

static void nagDiagRecordTxBlock(McpTxResultReason reason, uint32_t now) {
  if (reason == MCP_TX_OK) return;
  portENTER_CRITICAL(&nagDiagMux);
  nagLastTxBlockReason = (uint8_t)reason;
  nagLastTxBlockMs = now;
  switch (reason) {
    case MCP_TX_INVALID_MSG: nagBlockInvalidMsg++; break;
    case MCP_TX_MUTEX_BUSY: nagBlockMutex++; break;
    case MCP_TX_MCP_NOT_READY: nagBlockMcpNotReady++; break;
    case MCP_TX_EPOCH_MISMATCH: nagBlockEpoch++; break;
    case MCP_TX_FRESH_MASK: nagBlockFreshMask++; break;
    case MCP_TX_SEND_ERROR: nagSendError++; break;
    default: break;
  }
  portEXIT_CRITICAL(&nagDiagMux);
}

static void nagDiagApTransition(bool ap, bool wasAp, uint32_t now) {
  if (ap == wasAp) return;
  portENTER_CRITICAL(&nagDiagMux);
  if (ap) {
    // New AP session: start continuity measurement from a clean baseline.
    nagLastTxOkMs = 0;
    nagMaxTxGapMs = 0;
    nagSessionTxOk = 0;
    nagSessionStartMs = now;
  } else {
    // Preserve the completed session's max gap/TX count for post-drive review.
    nagSessionStartMs = 0;
  }
  portEXIT_CRITICAL(&nagDiagMux);
}

static void nagClampTorque(uint8_t& b2, uint8_t& b3) {
  uint16_t raw = ((b2 & 0x0F) << 8) | b3;
  if (raw > NAG_TORQUE_RAW_MAX) raw = NAG_TORQUE_RAW_MAX;
  if (raw < NAG_TORQUE_RAW_MIN) raw = NAG_TORQUE_RAW_MIN;
  b2 = (b2 & 0xF0) | ((raw >> 8) & 0x0F);
  b3 = raw & 0xFF;
}

static void nagCfgSetCommonDefaults(NagConfig& c) {
  c.enabled        = false;
  c.ignoreApState  = false;
  c.method         = NAG_METHOD_DEFAULT_PURE;
  c.tsl9Sequence   = TSL9_SEQUENCE_DEFAULT_PURE;
  c.tsl9DowngradeWindow = TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE;
  c.tsl9InputMode  = TSL9_INPUT_MODE_DEFAULT_PURE;
  c.tsl9LegacyRoute = TSL9_LEGACY_ROUTE_DEFAULT_PURE;
  c.pauseAtZeroSpeed = false;
  c.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  c.burstMs        = 1000;
  c.pauseMs        = 1500;
  c.apStateId      = 0x399;
  c.apStateByte    = DAS399_AP_STATE_BYTE_PURE;
  c.apStateShift   = DAS399_AP_STATE_SHIFT_PURE;
  c.apStateMask    = DAS399_AP_STATE_MASK_PURE;
  c.handsOnByte    = DAS399_HANDS_ON_BYTE_PURE;
  c.handsOnShift   = DAS399_HANDS_ON_SHIFT_PURE;
  c.handsOnMask    = DAS399_HANDS_ON_MASK_PURE;
  c.steeringId     = 0x129;
  c.steeringByteHi = 1;
  c.steeringByteLo = 0;
}

static void nagCfgDefaultsModeA(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_A;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xB6;
  c.hoRatePct   = 100;
}
static void nagCfgDefaultsModeB(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_B;
  c.targetId    = 0x370;
  c.torqueCount = 4;
  c.torqueB2[0] = 0x08; c.torqueB3[0] = 0xB6;
  c.torqueB2[1] = 0x08; c.torqueB3[1] = 0x98;
  c.torqueB2[2] = 0x07; c.torqueB3[2] = 0x6C;
  c.torqueB2[3] = 0x07; c.torqueB3[3] = 0x4E;
  c.hoRatePct   = 100;
}

static void nagCfgDefaultsModeC(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_C;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0xA7; // +1.65 Nm midpoint; runtime value walks dynamically.
  c.hoRatePct   = 100;
  c.pauseMs     = 0;    // Mode C is continuous by default.
}

// Mode H — single event engine with the shared torque eligibility policy.
static void nagCfgDefaultsModeH(NagConfig& c) {
  nagCfgSetCommonDefaults(c);
  c.mode        = MODE_H;
  c.targetId    = 0x370;
  c.torqueCount = 1;
  c.torqueB2[0] = 0x08;
  c.torqueB3[0] = 0x02; // 0 Nm placeholder; runtime output comes from Mode H.
  c.hoRatePct   = 100;
}


static void nagCfgClampAll(NagConfig& c) {
  c.method = nagMethodSanitizePure(c.method);
  c.tsl9Sequence = tsl9SequenceSanitizePure(c.tsl9Sequence);
  c.tsl9DowngradeWindow =
      tsl9DowngradeWindowSanitizePure(c.tsl9DowngradeWindow);
  c.tsl9InputMode = tsl9InputModeSanitizePure(c.tsl9InputMode);
  c.tsl9LegacyRoute = tsl9LegacyRouteSanitizePure(c.tsl9LegacyRoute);
  if (!nagModeHStopBehaviorValidPure(c.modeHStopBehavior))
    c.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  if (c.mode == MODE_H) c.hoRatePct = 100;
  if (c.torqueCount < 1) c.torqueCount = 1;
  if (c.torqueCount > NAG_MAX_TORQUE_ENTRIES) c.torqueCount = NAG_MAX_TORQUE_ENTRIES;
  if (c.hoRatePct > 100) c.hoRatePct = 100;
  if (c.burstMs < 50)    c.burstMs   = 50;
  if (c.burstMs > 10000) c.burstMs   = 10000;
  if (c.pauseMs > 10000) c.pauseMs   = 10000;
  for (uint8_t i = 0; i < c.torqueCount; i++) nagClampTorque(c.torqueB2[i], c.torqueB3[i]);
}

static bool nagCfgApplyActiveProfilePolicy(NagConfig& c, bool disableIfFallback) {
  const bool torqueSupported = activeProfileNagTorqueSupported();
  const bool tsl9Supported = activeProfileNagTsl9Supported();
  const uint8_t previous = nagMethodSanitizePure(c.method);
  const uint8_t resolved = nagMethodResolveForCapabilitiesPure(
      previous, torqueSupported, tsl9Supported);
  const bool changed = resolved != previous;
  c.method = resolved;
  // A topology migration must never auto-arm a newly selected injection method.
  // Explicit user API changes pass disableIfFallback=false and retain the user's
  // requested enabled state after the method has been normalized.
  if ((!torqueSupported && !tsl9Supported) || (changed && disableIfFallback))
    c.enabled = false;
  return changed;
}

static inline bool nagModePersistedIdSupported(uint8_t mode) {
  return mode == MODE_A || mode == MODE_B || mode == MODE_C || mode == MODE_H;
}

static void nagCfgLoad() {
#if T2CAN_SERIAL_DIAGNOSTICS
  T2CAN_SERIAL_PRINTLN("NVS: Loading nag config...");
#endif
  if (!prefs.begin("nag", true)) {
#if T2CAN_SERIAL_DIAGNOSTICS
    T2CAN_SERIAL_PRINTLN("NVS: No existing nag config, using defaults");
#endif
    nagCfgDefaultsModeH(nagCfg);
    nagCfgApplyActiveProfilePolicy(nagCfg, true);
    nagHumanRuntimeLoadDefaults();
    nagRxSelectorsRefreshFromConfig();
    return;
  }
  if (!prefs.isKey("v")) {
    const bool savedEnabled = prefs.getBool("en", false);
    const bool savedIgnoreApState = prefs.getBool("iap", false);
    prefs.end();
    nagCfgDefaultsModeH(nagCfg);
    nagCfg.enabled = savedEnabled;
    nagCfg.ignoreApState = savedIgnoreApState;
    nagCfgApplyActiveProfilePolicy(nagCfg, true);
    nagHumanRuntimeLoadDefaults();
    nagRxSelectorsRefreshFromConfig();
    return;
  }
  const uint8_t nagCfgVersion = prefs.getUChar("v", 0u);
  nagCfgSetCommonDefaults(nagCfg);
  nagCfg.enabled        = prefs.getBool("en", false);
  nagCfg.ignoreApState  = prefs.getBool("iap", false);
  nagCfg.method         = nagMethodSanitizePure(
      prefs.getUChar("method", NAG_METHOD_DEFAULT_PURE));
  nagCfg.tsl9Sequence   = tsl9SequenceSanitizePure(
      prefs.getUChar("tsl9seq", TSL9_SEQUENCE_DEFAULT_PURE));
  nagCfg.tsl9DowngradeWindow = tsl9DowngradeWindowSanitizePure(
      prefs.getUChar("tsl9win", TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE));
  nagCfg.tsl9InputMode = tsl9InputModeSanitizePure(
      prefs.getUChar("tsl9in", TSL9_INPUT_MODE_DEFAULT_PURE));
  nagCfg.tsl9LegacyRoute = tsl9LegacyRouteSanitizePure(
      prefs.getUChar("tsl9rt", TSL9_LEGACY_ROUTE_DEFAULT_PURE));
  nagCfg.pauseAtZeroSpeed = prefs.getBool("p0", false);
  const uint8_t storedStopDefault = nagCfgVersion < 18u
      ? H_STOP_STOCK_CARRIER : nagModeHDefaultStopBehaviorPure();
  nagCfg.modeHStopBehavior = prefs.getUChar("hsb", storedStopDefault);
  if (!nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior))
    nagCfg.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  nagCfg.mode           = prefs.getUChar("mode", MODE_H);
  if (!nagModePersistedIdSupported(nagCfg.mode)) nagCfg.mode = MODE_H;
  nagCfg.targetId       = prefs.getUShort("id", 0x370);
  nagCfg.torqueCount    = prefs.getUChar("tc", 1);
  size_t n = prefs.getBytes("tb2", nagCfg.torqueB2, NAG_MAX_TORQUE_ENTRIES);
  if (n == 0) { nagCfg.torqueB2[0] = 0x08; }
  n = prefs.getBytes("tb3", nagCfg.torqueB3, NAG_MAX_TORQUE_ENTRIES);
  if (n == 0) { nagCfg.torqueB3[0] = 0xB6; }
  nagCfg.hoRatePct      = prefs.getUChar("ho", 100);
  nagCfg.burstMs        = prefs.getUShort("bms", 1000);
  nagCfg.pauseMs        = prefs.getUShort("pms", 1500);
  nagCfg.apStateId      = prefs.getUShort("apid", 0x399);
  nagCfg.steeringId     = prefs.getUShort("stid", 0x129);

  const NagHumanV4ConfigPure rev4Defaults = nagHumanV4DefaultConfigPure();
  uint8_t humanVariant = prefs.getUChar("hv", nagModeHDefaultVariantPure());
  if (!nagModeHVariantValidPure(humanVariant)) humanVariant = nagModeHDefaultVariantPure();

  // Rev.4 has owned the h4* namespace since schema v15. Legacy Rev.1 Plus h1*
  // keys are deliberately never reinterpreted as Rev.4 tuning.
  NagHumanV4ConfigPure rev4Cfg = rev4Defaults;
  rev4Cfg.base.peakMinRaw = prefs.getUShort("h4pmin", rev4Defaults.base.peakMinRaw);
  rev4Cfg.base.peakMaxRaw = prefs.getUShort("h4pmax", rev4Defaults.base.peakMaxRaw);
  rev4Cfg.base.waitMinMs = prefs.getUShort("h4wmin", rev4Defaults.base.waitMinMs);
  rev4Cfg.base.waitMaxMs = prefs.getUShort("h4wmax", rev4Defaults.base.waitMaxMs);
  rev4Cfg.base.refractoryMinMs = prefs.getUShort("h4rmin", rev4Defaults.base.refractoryMinMs);
  rev4Cfg.base.refractoryMaxMs = prefs.getUShort("h4rmax", rev4Defaults.base.refractoryMaxMs);
  rev4Cfg.base.hoOverridePct = 0u;
  rev4Cfg.carrierMinRaw = prefs.getUShort("h4cmin", rev4Defaults.carrierMinRaw);
  rev4Cfg.carrierMaxRaw = prefs.getUShort("h4cmax", rev4Defaults.carrierMaxRaw);
  rev4Cfg.hoPolicy = prefs.getUChar("h4hop", rev4Defaults.hoPolicy);
  rev4Cfg.ho1ThresholdRaw = prefs.getUShort("h4ho1", rev4Defaults.ho1ThresholdRaw);
  rev4Cfg.ho2ThresholdRaw = prefs.getUShort("h4ho2", rev4Defaults.ho2ThresholdRaw);
  rev4Cfg.visualRescueEnabled = prefs.getBool("h4vres", rev4Defaults.visualRescueEnabled);
  rev4Cfg.visualRescueDelayMs = prefs.getUShort("h4vdly", rev4Defaults.visualRescueDelayMs);
  prefs.end();

  // Schema v17 promotes only the exact v16 Rev.4 defaults. Custom Rev.4
  // tuning survives unchanged while all profiles gain the new HO defaults.
  const bool migratedRev4V17 = nagCfgVersion < 17u && nagHumanV4MigrateV16DefaultPure(rev4Cfg);
  const bool migratedRev4V18 = nagCfgVersion < 18u && nagHumanV4MigrateV17DefaultPure(rev4Cfg);
  if ((migratedRev4V17 || migratedRev4V18) &&
      nagCfg.modeHStopBehavior == H_STOP_STOCK_CARRIER) {
    nagCfg.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  }

  nagCfgClampAll(nagCfg);
  const bool profileMethodMigrated = nagCfgApplyActiveProfilePolicy(nagCfg, true);
  if (!nagHumanV4ConfigValidPure(rev4Cfg)) rev4Cfg = rev4Defaults;
  portENTER_CRITICAL(&nagHumanMux);
  nagHumanVariant = humanVariant;
  nagHumanV4Config = rev4Cfg;
  nagHumanV4State = NagHumanV4StatePure{};
  nagHumanV4Initialized = false;
  portEXIT_CRITICAL(&nagHumanMux);

  // Schema v18 updates only an exact prior Rev.4 default profile. Custom
  // tuning and an independently selected stop behavior survive unchanged.
  // Profile id 1 is intentionally retained, so an OTA installation previously
  // selecting Rev.1 Plus moves to Rev.4 without creating a fifth profile id.
  if ((nagCfgVersion < 20u || migratedRev4V17 || migratedRev4V18 ||
       profileMethodMigrated) && prefs.begin("nag", false)) {
    prefs.putBool("en", nagCfg.enabled);
    prefs.putBool("iap", nagCfg.ignoreApState);
    prefs.putUChar("method", nagMethodSanitizePure(nagCfg.method));
    prefs.putUChar("tsl9seq", tsl9SequenceSanitizePure(nagCfg.tsl9Sequence));
    prefs.putUChar("tsl9win", tsl9DowngradeWindowSanitizePure(
        nagCfg.tsl9DowngradeWindow));
    prefs.putUChar("tsl9in", tsl9InputModeSanitizePure(nagCfg.tsl9InputMode));
    prefs.putUChar("tsl9rt", tsl9LegacyRouteSanitizePure(
        nagCfg.tsl9LegacyRoute));
    prefs.putUChar("hv", humanVariant);
    prefs.putUChar("hsb", nagCfg.modeHStopBehavior);

    prefs.putUShort("h4pmin", rev4Cfg.base.peakMinRaw);
    prefs.putUShort("h4pmax", rev4Cfg.base.peakMaxRaw);
    prefs.putUShort("h4wmin", rev4Cfg.base.waitMinMs);
    prefs.putUShort("h4wmax", rev4Cfg.base.waitMaxMs);
    prefs.putUShort("h4rmin", rev4Cfg.base.refractoryMinMs);
    prefs.putUShort("h4rmax", rev4Cfg.base.refractoryMaxMs);
    prefs.putUShort("h4cmin", rev4Cfg.carrierMinRaw);
    prefs.putUShort("h4cmax", rev4Cfg.carrierMaxRaw);
    prefs.putUChar("h4hop", rev4Cfg.hoPolicy);
    prefs.putUShort("h4ho1", rev4Cfg.ho1ThresholdRaw);
    prefs.putUShort("h4ho2", rev4Cfg.ho2ThresholdRaw);
    prefs.putBool("h4vres", rev4Cfg.visualRescueEnabled);
    prefs.putUShort("h4vdly", rev4Cfg.visualRescueDelayMs);
    prefs.putUChar("v", 20u);
    prefs.end();
  }

  nagRxSelectorsRefreshFromConfig();
#if T2CAN_SERIAL_DIAGNOSTICS
  T2CAN_SERIAL_PRINTLN("NVS: Nag config loaded OK");
#endif
}

// Persist the torque-only opt-in before publishing it to the running configuration.
// Existing configuration writes retain their established format.
static bool nagIgnoreApStatePersist(bool value) {
  if (!prefs.begin("nag", false)) return false;
  const bool saved = prefs.putBool("iap", value) == 1u &&
      prefs.getBool("iap", !value) == value;
  prefs.end();
  return saved;
}

static void nagCfgSave() {
  NagConfig snapshot;
  NagHumanV4ConfigPure rev4Snapshot;
  uint8_t variantSnapshot;
  portENTER_CRITICAL(&nagCfgMux);
  snapshot = nagCfg;
  portEXIT_CRITICAL(&nagCfgMux);
  portENTER_CRITICAL(&nagHumanMux);
  rev4Snapshot = nagHumanV4Config;
  variantSnapshot = nagModeHVariantValidPure(nagHumanVariant) ? nagHumanVariant : nagModeHDefaultVariantPure();
  portEXIT_CRITICAL(&nagHumanMux);
  nagCfgClampAll(snapshot);
  nagCfgApplyActiveProfilePolicy(snapshot, false);
  if (!prefs.begin("nag", false)) {
#if T2CAN_SERIAL_DIAGNOSTICS
    T2CAN_SERIAL_PRINTLN("NVS: Nag save failed - could not open");
#endif
    return;
  }
  prefs.putBool("en",     snapshot.enabled);
  prefs.putBool("iap",    snapshot.ignoreApState);
  prefs.putUChar("method", nagMethodSanitizePure(snapshot.method));
  prefs.putUChar("tsl9seq", tsl9SequenceSanitizePure(snapshot.tsl9Sequence));
  prefs.putUChar("tsl9win", tsl9DowngradeWindowSanitizePure(
      snapshot.tsl9DowngradeWindow));
  prefs.putUChar("tsl9in", tsl9InputModeSanitizePure(snapshot.tsl9InputMode));
  prefs.putUChar("tsl9rt", tsl9LegacyRouteSanitizePure(
      snapshot.tsl9LegacyRoute));
  prefs.putBool("p0",     snapshot.pauseAtZeroSpeed);
  prefs.putUChar("hsb",   snapshot.modeHStopBehavior);
  prefs.putUChar("mode",  snapshot.mode);
  prefs.putUShort("id",   snapshot.targetId);
  prefs.putUChar("tc",    snapshot.torqueCount);
  prefs.putBytes("tb2",   snapshot.torqueB2, NAG_MAX_TORQUE_ENTRIES);
  prefs.putBytes("tb3",   snapshot.torqueB3, NAG_MAX_TORQUE_ENTRIES);
  prefs.putUChar("ho",    snapshot.hoRatePct);
  prefs.putUShort("bms",  snapshot.burstMs);
  prefs.putUShort("pms",  snapshot.pauseMs);
  prefs.putUShort("apid", snapshot.apStateId);
  prefs.putUShort("stid", snapshot.steeringId);
  prefs.putUChar("hv", variantSnapshot);

  prefs.putUShort("h4pmin", rev4Snapshot.base.peakMinRaw);
  prefs.putUShort("h4pmax", rev4Snapshot.base.peakMaxRaw);
  prefs.putUShort("h4wmin", rev4Snapshot.base.waitMinMs);
  prefs.putUShort("h4wmax", rev4Snapshot.base.waitMaxMs);
  prefs.putUShort("h4rmin", rev4Snapshot.base.refractoryMinMs);
  prefs.putUShort("h4rmax", rev4Snapshot.base.refractoryMaxMs);
  prefs.putUShort("h4cmin", rev4Snapshot.carrierMinRaw);
  prefs.putUShort("h4cmax", rev4Snapshot.carrierMaxRaw);
  prefs.putUChar("h4hop", rev4Snapshot.hoPolicy);
  prefs.putUShort("h4ho1", rev4Snapshot.ho1ThresholdRaw);
  prefs.putUShort("h4ho2", rev4Snapshot.ho2ThresholdRaw);
  prefs.putBool("h4vres", rev4Snapshot.visualRescueEnabled);
  prefs.putUShort("h4vdly", rev4Snapshot.visualRescueDelayMs);
  prefs.putUChar("v",     20u);
  prefs.end();
}

// ── Nag decide injection (raw data version) ──

static bool nagDecideInjection(uint8_t dlc,
                            uint8_t& out_b2, uint8_t& out_b3, bool& out_setHo) {
  if (dlc < 8) return false;
  unsigned long now = millis();

  uint8_t  mode, tCount, hoPct;
  uint16_t burstMs, pauseMs;
  uint8_t  tB2[NAG_MAX_TORQUE_ENTRIES], tB3[NAG_MAX_TORQUE_ENTRIES];

  portENTER_CRITICAL(&nagCfgMux);
  mode    = nagCfg.mode;
  tCount  = nagCfg.torqueCount;
  hoPct   = nagCfg.hoRatePct;
  burstMs = nagCfg.burstMs;
  pauseMs = nagCfg.pauseMs;
  for (uint8_t i = 0; i < tCount; i++) {
    tB2[i] = nagCfg.torqueB2[i];
    tB3[i] = nagCfg.torqueB3[i];
  }
  portEXIT_CRITICAL(&nagCfgMux);

  static uint8_t  tIdx = 0;
  static uint16_t hoSeq = 0;
  static uint32_t lastChangeMs = 0;
  static uint8_t  prevMode = 0xFF;

  if (mode != prevMode) {
    tIdx = 0; hoSeq = 0; lastChangeMs = now; prevMode = mode;
    if (mode == MODE_B) nagModeBPhaseStartMs = now;
    if (mode == MODE_C) {
      portENTER_CRITICAL(&nagCtxMux);
      nagCtx.walkSeed = (uint16_t)(esp_random() & 0xFFFFu);
      if (nagCtx.walkSeed == 0) nagCtx.walkSeed = 0xACE1u;
      nagCtx.modeCRaw = 0x8A7;
      portEXIT_CRITICAL(&nagCtxMux);
    }
  }

  if (mode == MODE_A) {
    out_b2 = tB2[tIdx % tCount];
    out_b3 = tB3[tIdx % tCount];
    tIdx++;
    bool setHo = ((hoSeq * 100u) / 65536u < (uint16_t)hoPct);
    hoSeq = (uint16_t)(hoSeq * 1103u + 12345u);
    out_setHo = setHo;
    return true;
  }

  if (mode == MODE_B) {
    uint32_t cycleMs = (uint32_t)burstMs + (uint32_t)pauseMs;
    if (cycleMs == 0) cycleMs = 1;
    uint32_t phaseStart = nagModeBPhaseStartMs;
    if (phaseStart == 0) {
      phaseStart = now;
      nagModeBPhaseStartMs = now;
    }
    uint32_t phase = (uint32_t)(now - phaseStart) % cycleMs;
    if (phase >= burstMs) return false;
    if (now - lastChangeMs >= 200) { tIdx = (tIdx + 1) % tCount; lastChangeMs = now; }
    out_b2 = tB2[tIdx];
    out_b3 = tB3[tIdx];
    out_setHo = true;
    return true;
  }

  if (mode == MODE_C) {
    uint16_t raw;
    portENTER_CRITICAL(&nagCtxMux);
    if (nagCtx.modeCRaw < NAG_MODE_C_RAW_MIN || nagCtx.modeCRaw > NAG_MODE_C_RAW_MAX)
      nagCtx.modeCRaw = 0x8A7;
    if (nagCtx.walkSeed == 0) nagCtx.walkSeed = 0xACE1u;
    if (now - lastChangeMs >= NAG_MODE_C_STEP_MS) {
      nagCtx.modeCRaw = nagModeCNextRawPure(nagCtx.modeCRaw, nagCtx.walkSeed);
      lastChangeMs = now;
    }
    raw = nagCtx.modeCRaw;
    portEXIT_CRITICAL(&nagCtxMux);
    out_b2 = (uint8_t)((raw >> 8) & 0x0F);
    out_b3 = (uint8_t)(raw & 0xFF);
    out_setHo = true;
    return true;
  }

  return false;
}

// ── Nag context updates (raw data) ──

static void nagObserveHandsOnState(uint8_t ho, uint32_t now) {
  portENTER_CRITICAL(&nagCtxMux);
  if (ho != nagCtx.handsOnState) {
    const uint8_t previousHandsOnState = nagCtx.handsOnState;
    nagCtx.prevHandsOnState = previousHandsOnState;
    nagCtx.handsOnState = ho;
    const bool visualWarningNow = nagHumanV4VisualWarningActivePure(ho);
    const bool scrollWarningWas =
        tsl9InputWarningStatePure(previousHandsOnState);
    const bool scrollWarningNow = tsl9InputWarningStatePure(ho);
    if (nagHumanV4VisualWarningEdgePure(previousHandsOnState, ho)) {
      nagCtx.visualWarningEpoch++;
      if (nagCtx.visualWarningEpoch == 0u) nagCtx.visualWarningEpoch = 1u;
      nagCtx.visualWarningEnterMs = now;
    }
    nagCtx.visualWarningActive = visualWarningNow;
    if (!scrollWarningWas && scrollWarningNow) {
      nagCtx.scrollWarningEpoch++;
      if (nagCtx.scrollWarningEpoch == 0u) nagCtx.scrollWarningEpoch = 1u;
    }
    nagCtx.scrollWarningActive = scrollWarningNow;
    if (ho == 2 && nagCtx.state2EnterMs == 0) nagCtx.state2EnterMs = now;
    if (ho != 2) nagCtx.state2EnterMs = 0;
    if (ho == 3 && nagCtx.state3EnterMs == 0) nagCtx.state3EnterMs = now;
    if (ho != 3) nagCtx.state3EnterMs = 0;
  }
  portEXIT_CRITICAL(&nagCtxMux);
}

static void nagUpdateApState(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t apb, apsh, apmask, hob, hosh, homask;
  portENTER_CRITICAL(&nagCfgMux);
  apb = nagCfg.apStateByte; apsh = nagCfg.apStateShift; apmask = nagCfg.apStateMask;
  hob = nagCfg.handsOnByte; hosh = nagCfg.handsOnShift; homask = nagCfg.handsOnMask;
  portEXIT_CRITICAL(&nagCfgMux);
  uint8_t ap = 0xFFu;
  uint8_t ho = 0xFFu;
  if (!dasStatus399SelectorDecodePure(
          data, dlc, apb, apsh, apmask, hob, hosh, homask, ap, ho)) return;
  unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.apState = ap;
  nagCtx.lastApStateMs = now;
  portEXIT_CRITICAL(&nagCtxMux);
  nagObserveHandsOnState(ho, (uint32_t)now);
}

static void nagUpdateVehicleSpeed(const uint8_t* data, uint8_t dlc) {
  uint16_t raw = 0;
  const bool valid = nagDecodePartySpeedRawPure(data, dlc, raw);
  const unsigned long now = millis();
  const bool locked = canTxBarrierMutex &&
      xSemaphoreTake(canTxBarrierMutex, portMAX_DELAY) == pdTRUE;
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.vehicleSpeedValid = valid;
  if (valid) nagCtx.vehicleSpeedRaw = raw;
  nagCtx.lastVehicleSpeedMs = now;
  portEXIT_CRITICAL(&nagCtxMux);
  if (locked) xSemaphoreGive(canTxBarrierMutex);
}

static void nagUpdateSteering(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t bh, bl;
  portENTER_CRITICAL(&nagCfgMux);
  bh = nagCfg.steeringByteHi; bl = nagCfg.steeringByteLo;
  portEXIT_CRITICAL(&nagCfgMux);
  if (bh >= dlc || bl >= dlc) return;
  const int16_t raw = (int16_t)(((uint16_t)data[bh] << 8) | data[bl]);
  const unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  // Public Tesla DBC scale is fixed at 0.1 deg/raw with zero offset, so the
  // signed raw value is already steering angle in deci-degrees.
  nagCtx.steeringAngleDeciDeg = raw;
  nagCtx.lastSteeringMs = now;
  portEXIT_CRITICAL(&nagCtxMux);
}

// ── Forward declarations : summon status handlers (defined in CAN B section) ──
// On Model YL these are also called from CAN A (MCP2515).
static void handle280(const uint8_t *data);
static void handle390(const uint8_t *data);
static void handle921(const uint8_t *data, uint8_t dlc);
static void handle1016(const uint8_t *data, uint8_t dlc);
static bool nagApInjectionGateOpen();
static void nagApGateSnapshot(bool &validOut, bool &activeOut);
// Explicit prototypes required here because NAG TX is defined before later modular definitions.

static void nagTsl9RecordTx(bool ok, uint32_t nowMs, uint8_t injectedHo,
                            bool handsOnModified, bool isaModified) {
  if (handsOnModified) {
    portENTER_CRITICAL(&nagTsl9Mux);
    if (ok) nagTsl9TxOk++; else nagTsl9TxFail++;
    portEXIT_CRITICAL(&nagTsl9Mux);
    portENTER_CRITICAL(&nagDiagMux);
    if (ok) {
      nagTxOk++;
      nagMaxTxGapMs = nagGapMaxUpdatePure(nagLastTxOkMs, nowMs, nagMaxTxGapMs);
      nagLastTxOkMs = nowMs;
      nagSessionTxOk++;
    } else {
      nagTxFail++;
    }
    portEXIT_CRITICAL(&nagDiagMux);
    if (ok) {
      nagEchoCount++;
      nagLastInjectedHo = injectedHo;
    }
  }
  if (isaModified) {
    portENTER_CRITICAL(&isaSuppressionControlMux);
    if (ok) isaSuppressionTxOk++; else isaSuppressionTxFail++;
    portEXIT_CRITICAL(&isaSuppressionControlMux);
  }
}

// Model Y L uses DAS_status 0x399 on Party CAN A. Legacy Model 3/Y uses the
// gateway-translated DAS_status 0x39B on Body CAN A.
static bool nagProcessTsl9Mcp(const struct can_frame& rxf) {
  const bool yl399Route = activeProfileIsYl();
  const bool legacyBody39BRoute = nagTsl9Body39BSelected();
  if ((!yl399Route && !legacyBody39BRoute) ||
      !activeProfileNagTsl9Supported()) return false;
  const uint16_t expectedId = legacyBody39BRoute ? 0x39Bu : 0x399u;
  if ((rxf.can_id & 0xC0000000UL) != 0 ||
      (rxf.can_id & 0x7FFu) != expectedId || rxf.can_dlc < 8u) return false;

  bool enabled;
  uint8_t method;
  uint8_t sequence;
  uint8_t downgradeWindow;
  bool isaSuppression;
  uint32_t isaSuppressionGenerationSnapshot;
  portENTER_CRITICAL(&nagCfgMux);
  enabled = nagCfg.enabled;
  method = nagMethodSanitizePure(nagCfg.method);
  sequence = tsl9SequenceSanitizePure(nagCfg.tsl9Sequence);
  downgradeWindow = tsl9DowngradeWindowSanitizePure(
      nagCfg.tsl9DowngradeWindow);
  portEXIT_CRITICAL(&nagCfgMux);
  isaSuppressionControlSnapshot(
      isaSuppression, isaSuppressionGenerationSnapshot);

  struct can_frame out = rxf;
  const uint32_t nowMs = (uint32_t)millis();
  Tsl9DasTransformResultPure result;
  portENTER_CRITICAL(&nagTsl9Mux);
  if (method == NAG_METHOD_TSL9_PURE) nagTsl9Rx++;
  result = tsl9ApplyDasTransformForCanIdPure(
      nagTsl9State, enabled && method == NAG_METHOD_TSL9_PURE,
      sequence, downgradeWindow, isaSuppression, expectedId,
      out.data, out.can_dlc, nowMs);
  if (result.handsOnModified) nagTsl9Modified++;
  if (result.handsOnModified) nagTsl9HandsOnModified++;
  portEXIT_CRITICAL(&nagTsl9Mux);
  if (result.isaModified) {
    portENTER_CRITICAL(&isaSuppressionControlMux);
    isaSuppressionModified++;
    portEXIT_CRITICAL(&isaSuppressionControlMux);
  }
  if (!result.modified) return false;

  const uint32_t txEpoch = canTxEpochSnapshot();
  MCP2515::ERROR err = MCP2515::ERROR_FAIL;
  McpTxResultReason txReason = MCP_TX_INVALID_MSG;
  const bool attempted = canTxMcpSendTaggedGuarded(
      &out, txEpoch, CAN_TX_TRACE_SOURCE_DEFAULT,
      &isaSuppressionGeneration, isaSuppressionGenerationSnapshot,
      err, &txReason);
  const bool ok = attempted && err == MCP2515::ERROR_OK;
  if (ok) {
    mcpTxOk++;
    mcpTxFailConsecutive = 0;
  } else {
    mcpTxFail++;
    if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++;
  }
  nagTsl9RecordTx(ok, nowMs, (uint8_t)((out.data[5] >> 2) & 0x0Fu),
                  result.handsOnModified, result.isaModified);
  return ok;
}

// Standard profiles receive DAS_status 0x399 on Chassis CAN B. This transport
// is available with either Body+Chassis or Party+Chassis wiring.
static bool nagProcessTsl9Twai399(const twai_message_t &src) {
  if (!nagTsl9Chassis399Selected() ||
      !activeProfileNagTsl9Supported() ||
      !activeCanBIsChassis()) return false;
  if (src.extd || src.rtr || src.identifier != 0x399u ||
      src.data_length_code < 8u) return false;

  bool enabled;
  uint8_t method;
  uint8_t sequence;
  uint8_t downgradeWindow;
  bool isaSuppression;
  uint32_t isaSuppressionGenerationSnapshot;
  portENTER_CRITICAL(&nagCfgMux);
  enabled = nagCfg.enabled;
  method = nagMethodSanitizePure(nagCfg.method);
  sequence = tsl9SequenceSanitizePure(nagCfg.tsl9Sequence);
  downgradeWindow = tsl9DowngradeWindowSanitizePure(
      nagCfg.tsl9DowngradeWindow);
  portEXIT_CRITICAL(&nagCfgMux);
  isaSuppressionControlSnapshot(
      isaSuppression, isaSuppressionGenerationSnapshot);

  twai_message_t out = src;
  out.flags = 0;
  const uint32_t nowMs = (uint32_t)millis();
  Tsl9DasTransformResultPure result;
  portENTER_CRITICAL(&nagTsl9Mux);
  if (method == NAG_METHOD_TSL9_PURE) nagTsl9Rx++;
  result = tsl9ApplyDasTransformForCanIdPure(
      nagTsl9State, enabled && method == NAG_METHOD_TSL9_PURE,
      sequence, downgradeWindow, isaSuppression, 0x399u,
      out.data, out.data_length_code, nowMs);
  if (result.handsOnModified) nagTsl9Modified++;
  if (result.handsOnModified) nagTsl9HandsOnModified++;
  portEXIT_CRITICAL(&nagTsl9Mux);
  if (result.isaModified) {
    portENTER_CRITICAL(&isaSuppressionControlMux);
    isaSuppressionModified++;
    portEXIT_CRITICAL(&isaSuppressionControlMux);
  }
  if (!result.modified) return false;

  const uint32_t txEpoch = canTxEpochSnapshot();
  const esp_err_t err = canTxTwaiTransmitWithMaskTaggedGuarded(
      &out, txEpoch, CAN_TX_FRESH_BOTH, CAN_TX_TRACE_SOURCE_DEFAULT,
      &isaSuppressionGeneration, isaSuppressionGenerationSnapshot);
  const bool ok = err == ESP_OK;
  nagTsl9RecordTx(ok, nowMs, (uint8_t)((out.data[5] >> 2) & 0x0Fu),
                  result.handsOnModified, result.isaModified);
  return ok;
}

// ── Nag process frame from MCP2515 ──

static void nagProcessMcpFrame(const struct can_frame& rxf) {
  // Defense in depth: never let extended or RTR frames alias a standard
  // Tesla ID and reach status parsing or an actuation path.
  if ((rxf.can_id & 0xC0000000UL) != 0) return;
  const uint16_t id = rxf.can_id & 0x7FF;
  const uint8_t dlc = rxf.can_dlc;
  if (dlc < 1) return;

  (void)nagProcessTsl9Mcp(rxf);

  const bool ylProfile = activeProfileIsYl();
  // Model Y L reads DAS/Summon status on CAN A / Party. Standard 3/Y
  // receives the corresponding status from Chassis CAN B regardless of whether
  // CAN A is configured as Body or Party, so only YL runs these handlers here.
  if (ylProfile) {
    switch (id) {
      case 280: if (dlc >= 7) handle280(rxf.data); break;
      case 390: if (dlc >= 8) handle390(rxf.data); break;
      case 921:
        if (dlc >= 6) handle921(rxf.data, dlc);
        break;
      default: break;
    }
  }

  // The steering-torque method requires CAN A to be Party. TSL9 on standard
  // Body+Chassis is handled independently on Chassis CAN B above.
  if (!activeProfileNagTorqueSupported()) return;

  uint16_t targetId, apStateId, steeringId;
  nagRxSelectorsSnapshot(targetId, apStateId, steeringId);

  // Most Party frames are unrelated to NAG. Reject them before touching
  // nagCfgMux; only speed/context/target frames enter the NAG processing path.
  if (id != NAG_SPEED_ID && id != targetId && id != apStateId && id != steeringId) return;

  if (id == NAG_SPEED_ID) nagUpdateVehicleSpeed(rxf.data, dlc);
  // YL's proven Party layout exposes the legacy 0x399 context used by these
  // diagnostics. Standard Party+Chassis authorizes NAG from Chassis CAN B
  // 0x399 instead; do not misparse a Party frame with the same numeric ID.
  if (ylProfile && id == apStateId) nagUpdateApState(rxf.data, dlc);
  if (id == steeringId) nagUpdateSteering(rxf.data, dlc);

  uint8_t methodNow;
  portENTER_CRITICAL(&nagCfgMux);
  methodNow = nagMethodSanitizePure(nagCfg.method);
  portEXIT_CRITICAL(&nagCfgMux);
  if (methodNow != NAG_METHOD_TORQUE_PURE) return;

  if (id != targetId) return;

  bool en, ignoreApState;
  uint8_t modeNow;
  portENTER_CRITICAL(&nagCfgMux);
  // Revalidate the configurable target after taking the lock. A dashboard
  // config update can race the lock-free selector cache by at most this frame.
  if (id != nagCfg.targetId) {
    portEXIT_CRITICAL(&nagCfgMux);
    return;
  }
  en = nagCfg.enabled;
  ignoreApState = nagCfg.ignoreApState;
  modeNow = nagCfg.mode;
  portEXIT_CRITICAL(&nagCfgMux);

  nagRxFrames++;
  if (dlc < 5) return;
  uint8_t ho = (rxf.data[4] >> 6) & 0x03;
  uint16_t tRaw = ((rxf.data[2] & 0x0F) << 8) | rxf.data[3];
  nagRealHo = ho;
  nagRealTorqueCenti = (int16_t)tRaw - 2050;
  const bool exactEchoMode = modeNow == MODE_H;

  // Invalidate exact-echo state on every mode transition. Entering Mode H
  // starts a fresh human-interaction event session; leaving H clears it.
  if (modeNow != nagExactEchoPrevMode) {
    const uint8_t previousMode = nagExactEchoPrevMode;
    nagExactEchoReset();
    if (modeNow == MODE_H) nagHumanRuntimeReset(true);
    else if (previousMode == MODE_H) nagHumanRuntimeReset(false);
    nagExactEchoPrevMode = modeNow;
  }

  bool isOurs = false;
  if (exactEchoMode) {
    bool lastValid;
    uint32_t lastMs;
    uint8_t lastRaw[8];
    portENTER_CRITICAL(&nagExactEchoMux);
    lastValid = nagExactEchoLastTxValid;
    lastMs = nagExactEchoLastTxMs;
    memcpy(lastRaw, nagExactEchoLastTxRaw, 8);
    portEXIT_CRITICAL(&nagExactEchoMux);
    const uint32_t ageMs = lastMs ? (uint32_t)((uint32_t)millis() - lastMs) : 0xFFFFFFFFu;
    isOurs = nagExactRecentTxEchoPure(exactEchoMode, lastValid, ageMs, rxf.data, lastRaw, dlc);
  }

  // Preserve the v3.2 hotfix A/B/C self-frame heuristic unchanged.
  if (!isOurs && !exactEchoMode && ho == 1) {
    portENTER_CRITICAL(&nagCfgMux);
    if (modeNow != MODE_C) {
      for (uint8_t i = 0; i < nagCfg.torqueCount; i++) {
        uint16_t cfgRaw = ((nagCfg.torqueB2[i] & 0x0F) << 8) | nagCfg.torqueB3[i];
        if (tRaw == cfgRaw) { isOurs = true; break; }
      }
    }
    portEXIT_CRITICAL(&nagCfgMux);
    if (modeNow == MODE_C) {
      uint16_t modeCRaw;
      portENTER_CRITICAL(&nagCtxMux);
      modeCRaw = nagCtx.modeCRaw;
      portEXIT_CRITICAL(&nagCtxMux);
      isOurs = (tRaw == modeCRaw);
    }
  }

  const uint32_t nowMs = (uint32_t)millis();
  bool bootDelayPassed = (nowMs - canInitTime) >= NAG_INJECTION_DELAY_MS;
  bool canSeen = mcpRxCount > 1000;  // Retain the established MCP warmup threshold.
  bool apValid, apActiveForNag;
  nagApGateSnapshot(apValid, apActiveForNag);
  const NagSkipReasonPure eligibility = nagEligibilityReasonPure(
      en, bootDelayPassed, canSeen, isOurs, ho, apValid, apActiveForNag, ignoreApState);
  if (eligibility != NAG_SKIP_NONE) {
    // A real gate closure cancels a Mode H event. A local self-echo is merely
    // ignored and must not tear down the live event session.
    if (modeNow == MODE_H && eligibility != NAG_SKIP_SELF_FRAME)
      nagHumanRuntimeReset(false);
    nagDiagRecordSkip(eligibility, nowMs);
    return;
  }

  bool pauseAtZero = false;
  uint8_t modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  portENTER_CRITICAL(&nagCfgMux);
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  modeHStopBehavior = nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior)
      ? nagCfg.modeHStopBehavior : nagModeHDefaultStopBehaviorPure();
  portEXIT_CRITICAL(&nagCfgMux);
  uint16_t speedRaw = 0;
  bool speedValid = false;
  uint32_t speedLastMs = 0;
  uint32_t visualWarningEpoch = 0;
  uint32_t visualWarningEnterMs = 0;
  bool visualWarningActive = false;
  portENTER_CRITICAL(&nagCtxMux);
  speedRaw = nagCtx.vehicleSpeedRaw;
  speedValid = nagCtx.vehicleSpeedValid;
  speedLastMs = nagCtx.lastVehicleSpeedMs;
  visualWarningEpoch = nagCtx.visualWarningEpoch;
  visualWarningEnterMs = nagCtx.visualWarningEnterMs;
  visualWarningActive = nagCtx.visualWarningActive;
  portEXIT_CRITICAL(&nagCtxMux);
  const bool speedFresh = speedLastMs != 0 && (uint32_t)(nowMs - speedLastMs) <= NAG_SPEED_FRESH_MS;

  NagHumanV1StepResultPure humanDecision = {};
  bool modeHStopCarrier = false;
  if (modeNow == MODE_H) {
    humanDecision = nagHumanRuntimeStep(nowMs, tRaw, true, speedValid, speedFresh, speedRaw,
                                        visualWarningEpoch, visualWarningEnterMs, visualWarningActive);
    if (!humanDecision.tx) {
      const bool confirmedStopped = humanDecision.blockReason == H1_BLOCK_STOPPED;
      if (nagModeHUseStopCarrierPure(modeHStopBehavior, confirmedStopped)) {
        // d5 A/B experiment: keep Mode H's event engine paused at a confirmed
        // stop, but preserve transport continuity. Torque and the complete HO
        // field remain stock; only the rolling counter/checksum are advanced.
        humanDecision.tx = true;
        humanDecision.raw = tRaw;
        humanDecision.setHo = false;
        humanDecision.hoOverrideValid = false;
        humanDecision.hoLevel = 0u;
        modeHStopCarrier = true;
      } else {
        const NagSkipReasonPure reason = humanDecision.blockReason == H1_BLOCK_SPEED_STALE
            ? NAG_SKIP_SPEED_STALE
            : NAG_SKIP_STOPPED;
        nagDiagRecordSkip(reason, nowMs);
        return;
      }
    }
  } else if (nagPauseAtZeroBlocksPure(pauseAtZero, speedValid, speedFresh, speedRaw)) {
    nagDiagRecordSkip(NAG_SKIP_STOPPED, nowMs);
    return;
  }

  const uint8_t counterStep = 1u;

  const uint32_t txEpoch = canTxEpochSnapshot();
  if (txEpoch == 0) {
    nagDiagRecordTxBlock(MCP_TX_MUTEX_BUSY, nowMs);
    return;
  }
  {
    uint8_t b2 = 0, b3 = 0; bool setHo = false;
    bool hoOverrideValid = false;
    uint8_t hoLevel = 0u;
    bool shouldInject = false;
    if (modeNow == MODE_H) {
      b2 = (uint8_t)((humanDecision.raw >> 8) & 0x0Fu);
      b3 = (uint8_t)(humanDecision.raw & 0xFFu);
      setHo = humanDecision.setHo;
      hoOverrideValid = humanDecision.hoOverrideValid;
      hoLevel = humanDecision.hoLevel > 2u ? 2u : humanDecision.hoLevel;
      shouldInject = humanDecision.tx;
    } else {
      shouldInject = nagDecideInjection(dlc, b2, b3, setHo);
    }
    if (shouldInject) {
      struct can_frame txf;
      txf.can_id = rxf.can_id;
      txf.can_dlc = rxf.can_dlc;
      memcpy(txf.data, rxf.data, 8);
      txf.data[2] = (txf.data[2] & 0xF0) | (b2 & 0x0F);
      txf.data[3] = b3;
      if (modeNow == MODE_H && hoOverrideValid) {
        // Explicitly overwrite the complete 2-bit EPAS handsOnLevel field.
        // Mode H may request HO=2; HO=3 is never generated.
        txf.data[4] = (uint8_t)((txf.data[4] & 0x3Fu) | ((hoLevel & 0x03u) << 6));
      } else if (setHo) {
        txf.data[4] = (uint8_t)(txf.data[4] | 0x40u);
      }
      txf.data[6] = (txf.data[6] & 0xF0) |
                    (((txf.data[6] & 0x0F) + counterStep) & 0x0F);
      uint16_t s = txf.data[0] + txf.data[1] + txf.data[2] + txf.data[3]
                 + txf.data[4] + txf.data[5] + txf.data[6];
      txf.data[7] = (uint8_t)((s + 0x73) & 0xFF);

      unsigned long t0 = micros();
      MCP2515::ERROR err = MCP2515::ERROR_OK;
      McpTxResultReason txReason = MCP_TX_INVALID_MSG;
      const bool attempted = canTxMcpSend(&txf, txEpoch, err, &txReason);
      if (!attempted) {
        nagDiagRecordTxBlock(txReason, nowMs);
        return;
      }
      const bool txOk = err == MCP2515::ERROR_OK;
      nagEchoLatUs = micros() - t0;

      if (txOk) {
        mcpTxOk++; nagEchoCount++;
        mcpTxFailConsecutive = 0;
        nagLastInjectedHo = exactEchoMode
            ? (uint8_t)((txf.data[4] >> 6) & 0x03u)
            : (setHo ? 1 : 0);
        const uint16_t raw = ((b2 & 0x0F) << 8) | b3;
        nagLastInjectedCenti = (int16_t)raw - 2050;
        if (exactEchoMode) {
          portENTER_CRITICAL(&nagExactEchoMux);
          memcpy(nagExactEchoLastTxRaw, txf.data, 8);
          nagExactEchoLastTxMs = nowMs;
          nagExactEchoLastTxValid = true;
          portEXIT_CRITICAL(&nagExactEchoMux);
        }
        portENTER_CRITICAL(&nagDiagMux);
        nagTxOk++;
        nagMaxTxGapMs = nagGapMaxUpdatePure(nagLastTxOkMs, nowMs, nagMaxTxGapMs);
        nagLastTxOkMs = nowMs;
        nagSessionTxOk++;
        if (modeHStopCarrier) nagStopCarrierTxOk++;
        nagLastTxBlockReason = MCP_TX_OK;
        nagLastTxBlockMs = 0;
        portEXIT_CRITICAL(&nagDiagMux);
      } else {
        mcpTxFail++;
        if (mcpTxFailConsecutive < 255) mcpTxFailConsecutive++;
        portENTER_CRITICAL(&nagDiagMux);
        nagTxFail++;
        portEXIT_CRITICAL(&nagDiagMux);
        nagDiagRecordTxBlock(MCP_TX_SEND_ERROR, nowMs);
#if T2CAN_SERIAL_DIAGNOSTICS
        const unsigned long now = millis();
        if (now - nagLastTxFailLog >= 2000) {
          nagLastTxFailLog = now;
          T2CAN_SERIAL_PRINTF("[NAG TX FAIL] MCP err=%d nag=%lu mcp=%lu\n",
                        (int)err, (unsigned long)nagTxFail, (unsigned long)mcpTxFail);
        }
#endif
      }
    } else {
      nagDiagRecordSkip(NAG_SKIP_DECISION, nowMs);
    }
  }
}
