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
static volatile uint8_t canTxFreshMaskFast = 0;

static void mcpRxOverflowObserve(uint32_t now, uint8_t flags) {
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
// operation with respect to invalidateCanTxState().
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
static constexpr uint8_t CAN_B_TX_TRACE_CAPACITY = 64;
struct CanBTxTraceEntry {
  uint32_t seq;
  uint32_t capturedMs;
  uint16_t id;
  uint8_t dlc;
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

static void canBTraceRecordTx(const twai_message_t *msg, esp_err_t result) {
  if (!msg) return;
  CanBTxTraceEntry e = {};
  e.seq = __atomic_add_fetch(&canBTxTraceSeq, 1U, __ATOMIC_RELAXED);
  e.capturedMs = (uint32_t)millis();
  e.id = (uint16_t)(msg->identifier & 0x7FFU);
  e.dlc = (uint8_t)min((uint8_t)8, (uint8_t)msg->data_length_code);
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

static volatile bool canTxAdministrativeHold = false;

static esp_err_t canTxTwaiTransmit(const twai_message_t *msg, uint32_t expectedEpoch) {
  if (!msg) return ESP_ERR_INVALID_ARG;
  if (canTxAdministrativeHold) {
    canBTraceRecordTx(msg, ESP_ERR_INVALID_STATE);
    return ESP_ERR_INVALID_STATE;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    canBTraceRecordTx(msg, ESP_ERR_TIMEOUT);
    return ESP_ERR_TIMEOUT;
  }
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (!canTxAdministrativeHold && twaiReady && canTxBarrierAllowsPure(canTxBarrierState, expectedEpoch))
    err = twai_transmit(msg, 0);
  xSemaphoreGive(canTxBarrierMutex);
  canBTraceRecordTx(msg, err);
  return err;
}

static esp_err_t canTxTwaiClearQueue(uint32_t expectedEpoch) {
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE)
    return ESP_ERR_TIMEOUT;
  esp_err_t err = ESP_ERR_INVALID_STATE;
  if (twaiReady && canTxBarrierAllowsPure(canTxBarrierState, expectedEpoch))
    err = twai_clear_transmit_queue();
  xSemaphoreGive(canTxBarrierMutex);
  return err;
}

static bool canTxMcpSend(const struct can_frame *msg, uint32_t expectedEpoch,
                         MCP2515::ERROR &errOut, McpTxResultReason *reasonOut) {
  if (reasonOut) *reasonOut = MCP_TX_INVALID_MSG;
  if (!msg) return false;
  if (canTxAdministrativeHold) {
    if (reasonOut) *reasonOut = MCP_TX_EPOCH_MISMATCH;
    return false;
  }
  if (!canTxBarrierMutex || xSemaphoreTake(canTxBarrierMutex, 0) != pdTRUE) {
    if (reasonOut) *reasonOut = MCP_TX_MUTEX_BUSY;
    return false;
  }

  McpTxResultReason reason = canTxAdministrativeHold ? MCP_TX_EPOCH_MISMATCH : mcpTxResultReasonPure(
      true, true, mcpReady, canTxBarrierState.epoch, canTxBarrierState.freshMask,
      expectedEpoch, true);
  const bool allowed = reason == MCP_TX_OK;

  if (allowed) {
    errOut = Can_A.sendMessage(msg);
    reason = mcpTxResultReasonPure(
        true, true, true, canTxBarrierState.epoch, canTxBarrierState.freshMask,
        expectedEpoch, errOut == MCP2515::ERROR_OK);
  }
  if (reasonOut) *reasonOut = reason;
  xSemaphoreGive(canTxBarrierMutex);
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

enum NagMode : uint8_t { MODE_A = 0, MODE_B = 1, MODE_CUSTOM = 2, MODE_C = 3 };

struct NagConfig {
  bool     enabled;
  bool     pauseAtZeroSpeed;
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
  float    steeringScale;
  float    steeringOffset;
};

static NagConfig nagCfg;
static portMUX_TYPE nagCfgMux = portMUX_INITIALIZER_UNLOCKED;

struct NagContext {
  uint8_t  apState;
  uint8_t  handsOnState;
  uint8_t  prevHandsOnState;
  float    steeringAngleDeg;
  unsigned long lastApStateMs;
  unsigned long lastSteeringMs;
  unsigned long state2EnterMs;
  unsigned long state3EnterMs;
  uint16_t walkSeed;
  uint16_t modeCRaw;
  float    lastModeCTorqueNm;
  uint16_t vehicleSpeedRaw;
  bool     vehicleSpeedValid;
  unsigned long lastVehicleSpeedMs;
};

static NagContext nagCtx;
static portMUX_TYPE nagCtxMux = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t nagRxFrames    = 0;
static volatile uint32_t nagEchoCount   = 0;
static volatile uint32_t nagEchoLatUs   = 0;
static volatile uint8_t  nagRealHo      = 0;
static volatile float    nagRealTorque  = 0;
static volatile uint8_t  nagLastInjectedHo = 0;
static volatile float    nagLastInjectedNm = 0;
static unsigned long nagLastTxFailLog = 0;

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
// Mode B burst/pause timing starts from AP engagement rather than boot uptime.
// This is intentionally independent of the Original mcpRxCount > 1000 warmup check.
static volatile uint32_t nagModeBPhaseStartMs = 0;

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
    default: break;
  }
  // Routine self-echo and startup/disabled skips would otherwise mask the last
  // driving-relevant interruption. Preserve them in counters, not Last Skip.
  if (reason == NAG_SKIP_HANDS_ON || reason == NAG_SKIP_AP_INVALID ||
      reason == NAG_SKIP_AP_INACTIVE || reason == NAG_SKIP_DECISION ||
      reason == NAG_SKIP_STOPPED) {
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
  c.enabled        = true;
  c.pauseAtZeroSpeed = false;
  c.burstMs        = 1000;
  c.pauseMs        = 1500;
  c.apStateId      = 0x399;
  c.apStateByte    = 0;
  c.apStateShift   = 4;
  c.apStateMask    = 0x0F;
  c.handsOnByte    = 0;
  c.handsOnShift   = 0;
  c.handsOnMask    = 0x0F;
  c.steeringId     = 0x129;
  c.steeringByteHi = 1;
  c.steeringByteLo = 0;
  c.steeringScale  = 0.1f;
  c.steeringOffset = 0.0f;
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


static void nagCfgClampAll(NagConfig& c) {
  if (c.torqueCount < 1) c.torqueCount = 1;
  if (c.torqueCount > NAG_MAX_TORQUE_ENTRIES) c.torqueCount = NAG_MAX_TORQUE_ENTRIES;
  if (c.hoRatePct > 100) c.hoRatePct = 100;
  if (c.burstMs < 50)    c.burstMs   = 50;
  if (c.burstMs > 10000) c.burstMs   = 10000;
  if (c.pauseMs > 10000) c.pauseMs   = 10000;
  for (uint8_t i = 0; i < c.torqueCount; i++) nagClampTorque(c.torqueB2[i], c.torqueB3[i]);
}

static void nagCfgLoad() {
  Serial.println("NVS: Loading nag config...");
  if (!prefs.begin("nag", true)) {
    Serial.println("NVS: No existing nag config, using defaults");
    nagCfgDefaultsModeA(nagCfg);
    return;
  }
  if (!prefs.isKey("v")) {
    prefs.end();
    nagCfgDefaultsModeA(nagCfg);
    return;
  }
  nagCfgSetCommonDefaults(nagCfg);
  nagCfg.enabled        = prefs.getBool("en", true);
  nagCfg.pauseAtZeroSpeed = prefs.getBool("p0", false);
  nagCfg.mode           = prefs.getUChar("mode", 0);
  if (nagCfg.mode > MODE_C) nagCfg.mode = MODE_A;
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
  prefs.end();
  nagCfgClampAll(nagCfg);
  Serial.println("NVS: Nag config loaded OK");
}

static void nagCfgSave() {
  NagConfig snapshot;
  portENTER_CRITICAL(&nagCfgMux);
  snapshot = nagCfg;
  portEXIT_CRITICAL(&nagCfgMux);
  nagCfgClampAll(snapshot);
  if (!prefs.begin("nag", false)) {
    Serial.println("NVS: Nag save failed - could not open");
    return;
  }
  prefs.putBool("en",     snapshot.enabled);
  prefs.putBool("p0",     snapshot.pauseAtZeroSpeed);
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
  prefs.putUChar("v",     3);
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
      nagCtx.lastModeCTorqueNm = 1.65f;
      portEXIT_CRITICAL(&nagCtxMux);
    }
  }

  if (mode == MODE_A || mode == MODE_CUSTOM) {
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
      nagCtx.lastModeCTorqueNm = (float)nagCtx.modeCRaw * 0.01f - 20.5f;
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

static void nagUpdateApState(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t apb, apsh, apmask, hob, hosh, homask;
  portENTER_CRITICAL(&nagCfgMux);
  apb = nagCfg.apStateByte; apsh = nagCfg.apStateShift; apmask = nagCfg.apStateMask;
  hob = nagCfg.handsOnByte; hosh = nagCfg.handsOnShift; homask = nagCfg.handsOnMask;
  portEXIT_CRITICAL(&nagCfgMux);
  if (apb >= dlc || hob >= dlc) return;
  uint8_t ap = (data[apb] >> apsh) & apmask;
  uint8_t ho = (data[hob] >> hosh) & homask;
  unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.apState = ap;
  nagCtx.lastApStateMs = now;
  if (ho != nagCtx.handsOnState) {
    nagCtx.prevHandsOnState = nagCtx.handsOnState;
    nagCtx.handsOnState = ho;
    if (ho == 2 && nagCtx.state2EnterMs == 0) nagCtx.state2EnterMs = now;
    if (ho != 2) nagCtx.state2EnterMs = 0;
    if (ho == 3 && nagCtx.state3EnterMs == 0) nagCtx.state3EnterMs = now;
    if (ho != 3) nagCtx.state3EnterMs = 0;
  }
  portEXIT_CRITICAL(&nagCtxMux);
}

static void nagUpdateVehicleSpeed(const uint8_t* data, uint8_t dlc) {
  uint16_t raw = 0;
  const bool valid = nagDecodePartySpeedRawPure(data, dlc, raw);
  const unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.vehicleSpeedValid = valid;
  if (valid) nagCtx.vehicleSpeedRaw = raw;
  nagCtx.lastVehicleSpeedMs = now;
  portEXIT_CRITICAL(&nagCtxMux);
}

static void nagUpdateSteering(const uint8_t* data, uint8_t dlc) {
  if (dlc < 8) return;
  uint8_t bh, bl; float scale, offs;
  portENTER_CRITICAL(&nagCfgMux);
  bh = nagCfg.steeringByteHi; bl = nagCfg.steeringByteLo;
  scale = nagCfg.steeringScale; offs = nagCfg.steeringOffset;
  portEXIT_CRITICAL(&nagCfgMux);
  if (bh >= dlc || bl >= dlc) return;
  int16_t raw = (int16_t)(((uint16_t)data[bh] << 8) | data[bl]);
  float deg = raw * scale + offs;
  unsigned long now = millis();
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.steeringAngleDeg = deg;
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

// ── Nag process frame from MCP2515 ──

static void nagProcessMcpFrame(const struct can_frame& rxf) {
  // Defense in depth: never let extended or RTR frames alias a standard
  // Tesla ID and reach status parsing or an actuation path.
  if ((rxf.can_id & 0xC0000000UL) != 0) return;
  uint16_t id = rxf.can_id & 0x7FF;
  uint8_t dlc = rxf.can_dlc;
  if (dlc < 1) return;

  // Model Y L reads DAS/Summon status on CAN A / Party. Standard 3/Y
  // receives the corresponding status from Chassis CAN B regardless of whether
  // CAN A is configured as Body or Party, so only YL runs these handlers here.
  if (activeProfileIsYl()) {
    switch (id) {
      case 280: if (dlc >= 7) handle280(rxf.data); break;
      case 390: if (dlc >= 8) handle390(rxf.data); break;
      case 921: if (dlc >= 1) handle921(rxf.data, dlc); break;
      default: break;
    }
  }

  // Nag Killer requires CAN A to be Party. YL-specific Party parsing above remains model-gated.
  if (!activeProfileNagSupported()) return;

  uint16_t targetId, apStateId, steeringId;
  bool en;
  portENTER_CRITICAL(&nagCfgMux);
  targetId   = nagCfg.targetId;
  apStateId  = nagCfg.apStateId;
  steeringId = nagCfg.steeringId;
  en         = nagCfg.enabled;
  portEXIT_CRITICAL(&nagCfgMux);

  if (id == NAG_SPEED_ID) nagUpdateVehicleSpeed(rxf.data, dlc);
  // YL's proven Party layout exposes the legacy 0x399 context used by these
  // diagnostics. Standard Party+Chassis authorizes NAG from Chassis CAN B
  // 0x399 instead; do not misparse a Party frame with the same numeric ID.
  if (activeProfileIsYl() && id == apStateId) nagUpdateApState(rxf.data, dlc);
  if (id == steeringId) nagUpdateSteering(rxf.data, dlc);

  if (id != targetId) return;
  nagRxFrames++;

  if (dlc < 5) return;
  uint8_t ho = (rxf.data[4] >> 6) & 0x03;
  uint16_t tRaw = ((rxf.data[2] & 0x0F) << 8) | rxf.data[3];
  nagRealHo     = ho;
  nagRealTorque = tRaw * 0.01f - 20.5f;

  bool isOurs = false;
  if (ho == 1) {
    uint8_t modeNow;
    portENTER_CRITICAL(&nagCfgMux);
    modeNow = nagCfg.mode;
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
      en, bootDelayPassed, canSeen, isOurs, ho, apValid, apActiveForNag);
  if (eligibility != NAG_SKIP_NONE) {
    nagDiagRecordSkip(eligibility, nowMs);
    return;
  }

  bool pauseAtZero = false;
  portENTER_CRITICAL(&nagCfgMux);
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  portEXIT_CRITICAL(&nagCfgMux);
  uint16_t speedRaw = 0;
  bool speedValid = false;
  uint32_t speedLastMs = 0;
  portENTER_CRITICAL(&nagCtxMux);
  speedRaw = nagCtx.vehicleSpeedRaw;
  speedValid = nagCtx.vehicleSpeedValid;
  speedLastMs = nagCtx.lastVehicleSpeedMs;
  portEXIT_CRITICAL(&nagCtxMux);
  const bool speedFresh = speedLastMs != 0 && (uint32_t)(nowMs - speedLastMs) <= NAG_SPEED_FRESH_MS;
  if (nagPauseAtZeroBlocksPure(pauseAtZero, speedValid, speedFresh, speedRaw)) {
    nagDiagRecordSkip(NAG_SKIP_STOPPED, nowMs);
    return;
  }

  const uint32_t txEpoch = canTxEpochSnapshot();
  if (txEpoch == 0) {
    nagDiagRecordTxBlock(MCP_TX_MUTEX_BUSY, nowMs);
    return;
  }
  {
    uint8_t b2 = 0, b3 = 0; bool setHo = false;
    if (nagDecideInjection(dlc, b2, b3, setHo)) {
      struct can_frame txf;
      txf.can_id = rxf.can_id;
      txf.can_dlc = rxf.can_dlc;
      memcpy(txf.data, rxf.data, 8);
      txf.data[2] = (txf.data[2] & 0xF0) | (b2 & 0x0F);
      txf.data[3] = b3;
      txf.data[4] = setHo ? (txf.data[4] | 0x40) : txf.data[4];
      txf.data[6] = (txf.data[6] & 0xF0) | (((txf.data[6] & 0x0F) + 1) & 0x0F);
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
        nagLastInjectedHo = setHo ? 1 : 0;
        uint16_t raw = ((b2 & 0x0F) << 8) | b3;
        nagLastInjectedNm = raw * 0.01f - 20.5f;
        portENTER_CRITICAL(&nagDiagMux);
        nagTxOk++;
        nagMaxTxGapMs = nagGapMaxUpdatePure(nagLastTxOkMs, nowMs, nagMaxTxGapMs);
        nagLastTxOkMs = nowMs;
        nagSessionTxOk++;
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
        unsigned long now = millis();
        if (now - nagLastTxFailLog >= 2000) {
          nagLastTxFailLog = now;
          Serial.printf("[NAG TX FAIL] MCP err=%d nag=%lu mcp=%lu\n",
                        (int)err, (unsigned long)nagTxFail, (unsigned long)mcpTxFail);
        }
      }
    } else {
      nagDiagRecordSkip(NAG_SKIP_DECISION, nowMs);
    }
  }
}
