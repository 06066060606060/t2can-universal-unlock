#pragma once

// CAN TASKS / RECOVERY SUPERVISOR
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// CAN TASKS
// ═══════════════════════════════════════════════════════════════

// Recovery invalidates only observations owned by the controller that reset.
// A new global epoch still cancels in-flight TX, while freshness for an
// unaffected, physically-live bus may cross the local recovery boundary.
static uint8_t canPhysicalFreshMaskNow(uint32_t now) {
  uint8_t mask = 0;
  const uint32_t lastA = lastCanAFrameMs;
  const uint32_t lastB = lastCanBFrameMs;
  const uint32_t lastC = lastCanCFrameMs;
  if (lastC != 0 && (uint32_t)(now - lastC) <= RECOVERY_BUS_FRESH_MS)
    mask = (uint8_t)(mask | CAN_TX_FRESH_PARTY);
  if (lastB != 0 && (uint32_t)(now - lastB) <= RECOVERY_BUS_FRESH_MS)
    mask = (uint8_t)(mask | CAN_TX_FRESH_CHASSIS);
  if (lastA != 0 && (uint32_t)(now - lastA) <= RECOVERY_BUS_FRESH_MS)
    mask = (uint8_t)(mask | CAN_TX_FRESH_BODY);
  return mask;
}

static void invalidateCanTxStateInternal(uint8_t invalidatedBusMask) {
  const uint32_t now = (uint32_t)millis();
  const uint8_t physicalFreshMask = canPhysicalFreshMaskNow(now);
  const bool barrierLocked = canTxBarrierMutex &&
                             xSemaphoreTake(canTxBarrierMutex, portMAX_DELAY) == pdTRUE;
  if (barrierLocked) {
    const uint8_t preserved = canTxPreservedFreshMaskPure(
        canTxBarrierState.freshMask, invalidatedBusMask, physicalFreshMask);
    canTxBarrierInvalidatePreservePure(canTxBarrierState, preserved);
    __atomic_store_n(&canTxFreshMaskFast, preserved, __ATOMIC_RELEASE);
  }

  const SummonRoutePure route = activeSummonRoute();
  const SummonInvalidationPure inv =
      summonInvalidationPure(route, invalidatedBusMask);

  portENTER_CRITICAL(&stateMux);
  if (inv.invalidateDas || inv.invalidateTemplate) {
    r79ApGateSession = {};
    __atomic_add_fetch(&r79ApGateGeneration, 1u, __ATOMIC_ACQ_REL);
  }
  if (inv.invalidateDas) {
    gateAPActive = false;
    gateNOAActive = false;
    dasAutopilotStateValid = false;
    dasAutopilotState4 = 0xFF;
    dasAutoLaneChangeState = 0xFF;
    dasAutoLaneChangeStateValid = false;
    lastDASStatusMillis = 0;
  }
  if (inv.invalidateGear) {
    // Recovery clears fresh gear evidence. v3.6 R79 remains default-on until
    // a new valid D/R + manual state positively proves manual driving.
    gateParked = true;
    lastAca = false;
    acaValid = false;
    lastAcaMillis = 0;
    last280Millis = 0;
    gear118State = -1;
    gear186State = -1;
    gear118Raw = TESLA_GEAR_INVALID;
    gear186Raw = TESLA_GEAR_INVALID;
    gear118Ms = 0;
    gear186Ms = 0;
    summonGearSource = SUMMON_GEAR_NONE;
    summonGearObservedMs = 0;
  }
  if (inv.invalidateSpr) {
    sprSeen = false;
    sprValid = false;
    lastSprRaw = 0;
    lastSprMillis = 0;
  }
  if (inv.invalidateGear || inv.invalidateSpr) {
    gateSummoning = false;
  }
  if (inv.invalidateGear || inv.invalidateDas) {
    // Recovery of the signals that prove manual driving fails R79 back ON.
    r79ManualSuppression = {};
  }
  // Keep Summon monitor and the R79 manual latch coherent after source reset.
  refreshSummonDerivedStateLocked(now, barrierLocked);
  portEXIT_CRITICAL(&stateMux);

  if (inv.invalidateDas) {
    portENTER_CRITICAL(&nagTsl9Mux);
    nagTsl9State = {};
    portEXIT_CRITICAL(&nagTsl9Mux);
  }

  if (inv.invalidateTemplate) {
    portENTER_CRITICAL(&r79LabMux);
    r79LabStockValid = false;
    r79LabLastStockMs = 0;
    r79FixedQuietState = {};
    r79Mode2DelayedState = {};
    r79RetryPending = false;
    r79RetryIndex = 0;
    r79RetryOriginKind = R79LAB_TX_NONE;
    r79RetryDueMs = 0;
    r79RetryGeneration = 0u;
    r79LabLastTxValid = false;
    memset(r79LabLastStockRaw, 0, sizeof(r79LabLastStockRaw));
    portEXIT_CRITICAL(&r79LabMux);
  }
  // Keep an owned CENTER cleanup pending across controller recovery. The
  // scheduler will retry only after a fresh route-local MUX1 template arrives.
  tsl9InputRequestCancel(TSL9_INPUT_FAILURE_CAN_UNAVAILABLE_PURE, true);
  driverWindowLabResetRuntimeUnderTxBarrier();

  // These transient feature requests are inexpensive to restart and are cleared
  // at either controller recovery so they cannot cross a changed CAN epoch.
  portENTER_CRITICAL(&blinkAMux);
  autoBlinkerNoaSessionResetPure(autoBlinkerNoaSessionState);
  autoBlinkerCancelPauseResetPure(autoBlinkerCancelPauseState);
  autoBlinkerClearPendingLocked();
  oneShotTurn = STALK_IDLE;
  oneShotDirect = false;
  oneShotSource = BLINKER_TX_SOURCE_NONE_PURE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  blinkerTxRequestState = {};
  activeTurn = STALK_IDLE;
  lastReqDir = 0;
  autoRequestLastSeenMs = 0;
  autoRetryCount = 0;
  visualBehaviorType = 0;
  visualDebugLastMs = 0;
  seen249 = false;
  realDlc = 0;
  cksumSelfTest = false;
  memset(realRaw249, 0, sizeof(realRaw249));
  portEXIT_CRITICAL(&blinkAMux);

  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcSnoozePending = false;
  ulcSnoozeExpireMs = 0;
  portEXIT_CRITICAL(&ulcSnoozeMux);

  portENTER_CRITICAL(&s3xyMux);
  s3xyActionPending = 0;
  s3xyAccelActionPending = 0;
  s3xyPerformanceActionPending = 0;
  s3xyResearchCaptureAPending = 0;
  s3xyResearchCaptureBPending = 0;
  s3xyResearchCaptureCPending = 0;
  s3xyResearchCaptureDPending = 0;
  s3xyResearchCaptureResetPending = 0;
  s3xyTlsscActionPending = 0;
  s3xyAutoBlinkerActionPending = 0;
  portEXIT_CRITICAL(&s3xyMux);

  portENTER_CRITICAL(&pedalMapMux);
  pedalMapStockValid = false;
  pedalMapStockDataValid = false;
  pedalMapStockRaw = PEDAL_MAP_RAW_STOCK;
  pedalMapStockMs = 0;
  memset(pedalMapStockData, 0, sizeof(pedalMapStockData));
  // Keep the user-selected drive-session target across a transient CAN
  // recovery. The next fresh 0x334 re-establishes the template; confirmed
  // Park, explicit STOCK, or reboot owns session release.
  portEXIT_CRITICAL(&pedalMapMux);

  if ((invalidatedBusMask & SUMMON_BUS_A) != 0) {
    portENTER_CRITICAL(&autoLc293Mux);
    uiAutoLaneChangeStockAValid = false;
    uiAutoLaneChangeStockAMs = 0;
    portEXIT_CRITICAL(&autoLc293Mux);
  }
  if ((invalidatedBusMask & SUMMON_BUS_B) != 0) {
    portENTER_CRITICAL(&autoLc293Mux);
    uiAutoLaneChangeStockBValid = false;
    uiAutoLaneChangeStockBMs = 0;
    portEXIT_CRITICAL(&autoLc293Mux);
    portENTER_CRITICAL(&lab3f8Mux);
    uiDriverAssistLastRxMs = 0;
    uiUlcBlindSpotConfig = 0;
    uiAlcOffHighwayEnable = false;
    portEXIT_CRITICAL(&lab3f8Mux);
  }

  if (barrierLocked) xSemaphoreGive(canTxBarrierMutex);
}

static void invalidateNagPartySpeedState() {
  portENTER_CRITICAL(&nagCtxMux);
  nagCtx.vehicleSpeedRaw = 0;
  nagCtx.vehicleSpeedValid = false;
  nagCtx.lastVehicleSpeedMs = 0;
  portEXIT_CRITICAL(&nagCtxMux);
}

static void invalidateCanTxStateForCanARecovery() {
  invalidateNagPartySpeedState();
  invalidateCanTxStateInternal(SUMMON_BUS_A);
  if (nagTsl9Body39BSelected()) {
    portENTER_CRITICAL(&nagTsl9Mux);
    nagTsl9State = {};
    portEXIT_CRITICAL(&nagTsl9Mux);
  }
  nagExactEchoReset();
  nagHumanRuntimeReset(true);
}

static void invalidateCanTxStateForCanBRecovery() {
  const bool invalidatesNagGate = activeProfileNagGateDependsOnCanB();
  invalidateCanTxStateInternal(SUMMON_BUS_B);
  if (invalidatesNagGate) {
    nagExactEchoReset();
    nagHumanRuntimeReset(true);
  }
}

static void invalidateCanTxStateForFullRecovery() {
  invalidateNagPartySpeedState();
  invalidateCanTxStateInternal(SUMMON_BUS_BOTH);
  nagExactEchoReset();
  nagHumanRuntimeReset(true);
}

static void recordChassisBusOffSnapshot(const McpChassisStatus &st, uint32_t now) {
  CanChassisRecoverySnapshot snap = {};
  snap.valid = true;
  snap.state = (uint8_t)st.state;
  snap.capturedMs = now;
  const uint32_t lastB = lastCanBFrameMs;
  snap.rxGapMs = lastB ? (uint32_t)(now - lastB) : 0;
  snap.msgsToTx = st.msgs_to_tx;
  snap.msgsToRx = st.msgs_to_rx;
  snap.txErrorCounter = st.tx_error_counter;
  snap.rxErrorCounter = st.rx_error_counter;
  snap.txFailedCount = st.tx_failed_count;
  snap.rxMissedCount = st.rx_missed_count;
  snap.rxOverrunCount = st.rx_overrun_count;
  snap.arbLostCount = st.arb_lost_count;
  snap.busErrorCount = st.bus_error_count;
  portENTER_CRITICAL(&canRecoveryMux);
  canChassisLastBusOffSnapshot = snap;
  portEXIT_CRITICAL(&canRecoveryMux);
}

static void canBTraceFreezeBusOff(uint32_t now) {
  portENTER_CRITICAL(&canBTxTraceMux);
  const uint8_t count = canBTxTraceLiveCount;
  const uint8_t start = (uint8_t)((canBTxTraceLiveHead + CAN_B_TX_TRACE_CAPACITY - count) % CAN_B_TX_TRACE_CAPACITY);
  for (uint8_t i = 0; i < count; i++)
    canBTxTraceFrozen[i] = canBTxTraceLive[(uint8_t)((start + i) % CAN_B_TX_TRACE_CAPACITY)];
  for (uint8_t i = count; i < CAN_B_TX_TRACE_CAPACITY; i++) canBTxTraceFrozen[i] = {};
  canBTxTraceFrozenCount = count;
  canBTxTraceFrozenMs = now;
  canBTxTraceFrozenBusOffOrdinal = canChassisBusOffCount + 1U;
  portEXIT_CRITICAL(&canBTxTraceMux);
}

static void canATraceFreezeBusOff(uint32_t now, uint8_t eflg, uint8_t txFailConsecutive) {
  uint32_t overflowCount = 0, overflowLastMs = 0;
  uint8_t overflowFlags = 0;
  mcpRxOverflowSnapshot(overflowCount, overflowLastMs, overflowFlags);
  (void)overflowLastMs;
  (void)overflowFlags;
  const uint32_t ordinal = __atomic_add_fetch(&canAMcpBusOffCount, 1U, __ATOMIC_RELAXED);
  const uint32_t lastA = lastCanAFrameMs;

  portENTER_CRITICAL(&canATxTraceMux);
  const uint8_t count = canATxTraceLiveCount;
  const uint8_t start = (uint8_t)((canATxTraceLiveHead + CAN_A_TX_TRACE_CAPACITY - count) % CAN_A_TX_TRACE_CAPACITY);
  for (uint8_t i = 0; i < count; i++)
    canATxTraceFrozen[i] = canATxTraceLive[(uint8_t)((start + i) % CAN_A_TX_TRACE_CAPACITY)];
  for (uint8_t i = count; i < CAN_A_TX_TRACE_CAPACITY; i++) canATxTraceFrozen[i] = {};
  canATxTraceFrozenCount = count;
  canATxTraceFrozenMs = now;
  canATxTraceFrozenBusOffOrdinal = ordinal;
  canALastBusOffSnapshot.valid = true;
  canALastBusOffSnapshot.capturedMs = now;
  canALastBusOffSnapshot.eflg = eflg;
  canALastBusOffSnapshot.txFailConsecutive = txFailConsecutive;
  canALastBusOffSnapshot.rxAgeMs = lastA ? (uint32_t)(now - lastA) : 0;
  canALastBusOffSnapshot.rxOverflowCount = overflowCount;
  canALastBusOffSnapshot.txOk = mcpTxOk;
  canALastBusOffSnapshot.txFail = mcpTxFail;
  portEXIT_CRITICAL(&canATxTraceMux);
  canBusOffPersistenceMarkDirty(CAN_BUS_OFF_BUS_A_PURE);
}

// Initialize the MCP2515 and publish readiness only after every stage succeeds.
// All setup/recovery paths use the same checked sequence and MCP_CLOCK.
static bool tmrMcpInitController(MCP2515 &can, volatile bool &ready, volatile uint8_t &state,
                                  const char *name) {
  ready = false;

  const MCP2515::ERROR resetErr = can.reset(); // SPI RESET command; never toggles GPIO9.
  delay(2);
  const MCP2515::ERROR rateErr = (resetErr == MCP2515::ERROR_OK)
                                   ? can.setBitrate(CAN_500KBPS, MCP_CLOCK)
                                   : resetErr;
  const MCP2515::ERROR modeErr = (rateErr == MCP2515::ERROR_OK)
                                   ? can.setNormalMode()
                                   : rateErr;

  // A successful SPI command sequence is not enough: verify that the MCP2515
  // did not remain in BUS-OFF after returning to normal mode.
  const uint8_t eflg = (modeErr == MCP2515::ERROR_OK) ? can.getErrorFlags() : 0xFFu;
  const bool busOff = (eflg & MCP2515::EFLG_TXBO) != 0;
  const bool ok = resetErr == MCP2515::ERROR_OK &&
                  rateErr == MCP2515::ERROR_OK &&
                  modeErr == MCP2515::ERROR_OK &&
                  !busOff;

  ready = ok;
  state = ok ? 0 : 2;
  if (!ok) {
    TMR_SERIAL_PRINTF("[%s] MCP2515 init failed: reset=%d bitrate=%d mode=%d eflg=0x%02X\n",
                        name, (int)resetErr, (int)rateErr, (int)modeErr, (unsigned)eflg);
  }
  return ok;
}

// TMR CAN B / CHASSIS uses a native MCP2515 compatibility shim. Do not use
// mcpChassisStart() for recovery: that function historically only flips the
// software-ready flag and does NOT reinitialize the MCP2515 registers.
static esp_err_t tmrChassisRecoverChecked(const char *reason) {
  mcpChassisReady = false;

  const MCP2515::ERROR resetErr = Can_B.reset();
  delay(2);
  const MCP2515::ERROR rateErr = (resetErr == MCP2515::ERROR_OK)
                                   ? Can_B.setBitrate(CAN_500KBPS, MCP_CLOCK)
                                   : resetErr;
  const MCP2515::ERROR modeErr = (rateErr == MCP2515::ERROR_OK)
                                   ? Can_B.setNormalMode()
                                   : rateErr;
  const uint8_t eflg = (modeErr == MCP2515::ERROR_OK) ? Can_B.getErrorFlags() : 0xFFu;
  const bool busOff = (eflg & MCP2515::EFLG_TXBO) != 0;
  const bool ok = resetErr == MCP2515::ERROR_OK &&
                  rateErr == MCP2515::ERROR_OK &&
                  modeErr == MCP2515::ERROR_OK &&
                  !busOff;

  mcpChassisReady = ok;
  if (!ok) {
    TMR_SERIAL_PRINTF("[CAN B] CHASSIS %s failed: reset=%d bitrate=%d mode=%d eflg=0x%02X\n",
                      reason ? reason : "recovery",
                      (int)resetErr, (int)rateErr, (int)modeErr, (unsigned)eflg);
    return ESP_FAIL;
  }

  TMR_SERIAL_PRINTF("[CAN B] CHASSIS %s OK\n", reason ? reason : "recovery");
  return ESP_OK;
}

static bool mcpInitChecked() {
  const bool ok = tmrMcpInitController(Can_A, mcpReady, mcpState, "CAN A / BODY");
  if (ok) {
    mcpTxFailConsecutive = 0;
    mcpState = 0;
  }
  return ok;
}

static bool mcpPartyInitChecked() {
  return tmrMcpInitController(Can_C, mcpPartyReady, mcpPartyState, "CAN C / PARTY");
}

static bool mcpReinit() {
  return mcpInitChecked();
}

static bool mcpPartyReinit() {
  return mcpPartyInitChecked();
}

static void canChassisHandleAlerts() {
  uint32_t chassisAlerts = 0;
  if (mcpChassisReadAlerts(&chassisAlerts, 0) != ESP_OK || chassisAlerts == 0) return;
  if (chassisAlerts & MCP_CHASSIS_ALERT_BUS_OFF) {
    const uint32_t alertNow = (uint32_t)millis();
    McpChassisStatus alertSt = {};
    canBTraceFreezeBusOff(alertNow);
    if (mcpChassisGetStatus(&alertSt) == ESP_OK) recordChassisBusOffSnapshot(alertSt, alertNow);
  }
}


static void tmrProcessPartyFrame(const struct can_frame &rxf, uint32_t frameNow,
                                 uint32_t rxEpoch) {
  if ((rxf.can_id & 0xC0000000UL) != 0) return;
  const uint16_t id = (uint16_t)(rxf.can_id & 0x7FFu);
  canRxObserve(CAN_RX_BUS_PARTY, frameNow);
  canTxMarkFresh(CAN_TX_FRESH_PARTY);
  __atomic_add_fetch(&mcpPartyRxCount, 1U, __ATOMIC_RELAXED);
  // mcpRxCount is the historical NAG warm-up counter; it now follows J4 PARTY.
  __atomic_add_fetch(&mcpRxCount, 1U, __ATOMIC_RELAXED);
  bootCaptureObservePartyFrame(id, rxf.can_dlc, rxf.data);
  researchCaptureObserveParty(id, rxf.can_dlc, rxf.data);
  driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_A, id, rxf.can_dlc, rxf.data);

  if (id == 0x7FFu && rxf.can_dlc == 8u)
    countryOverrideObserve7ffCanA(rxf, rxEpoch);
  if (id == 0x238u && rxf.can_dlc == 8u)
    countryOverrideObserve238CanA(rxf, rxEpoch);

  nagProcessMcpFrame(rxf);
  if (id == VISUAL_DEBUG_ID && rxf.can_dlc >= 8u) {
    visualBehaviorType = (uint8_t)readBitsLE(rxf.data, 56, 2);
    visualDebugRxCount++;
    visualDebugLastMs = frameNow;
    evaluateAutoBlinker();
  }
}

static void canTaskMcp(void* arg) {
  TMR_SERIAL_PRINTLN("[CAN A] MCP2515 task started");
  canTaskDiagnosticsResetPure(canTaskMcpDiagnostics);
  for (;;) {
    const uint32_t loopStartMs = (uint32_t)millis();
    canTaskMcpHeartbeatMs = loopStartMs;
    canTaskDiagnosticsHeartbeatPure(canTaskMcpDiagnostics, loopStartMs,
                                    (uint32_t)micros());
    if (canTasksStopping || canMaintenanceActive()) {
      canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                       CAN_TASK_STAGE_QUIESCE,
                                       (uint32_t)millis());
      canTaskMcpQuiesced = true;
      while (canTasksStopping || canMaintenanceActive()) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskMcpQuiesced = false;
      continue;
    }
    // ── BOUNDED READ LOOP ──
    // The default d3 path prefetches up to four MCP2515 frames before decoding.
    // LAB can select the c7-style path, which processes each frame immediately.
    // Both modes retain the 32-frame yield budget and task priority.
    static constexpr uint8_t MCP_PREFETCH_CAPACITY = 4;
    const uint8_t readBatchBudget = canARxReadBatchBudgetPure(labMenuEnabled, canARxSavedMode);
    struct can_frame prefetched[MCP_PREFETCH_CAPACITY];
    uint32_t lanePrefetchedRxMs[MCP_PREFETCH_CAPACITY];
    uint8_t processed = 0;
    bool noMoreFrames = false;
    canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                     CAN_TASK_STAGE_RECEIVE,
                                     (uint32_t)millis());
    while (processed < MCP_RX_BUDGET && !noMoreFrames) {
      const uint32_t countryRxEpoch = canTxEpochSnapshot();
      uint8_t batch = 0;
      while (batch < readBatchBudget &&
             processed + batch < MCP_RX_BUDGET &&
             Can_A.readMessage(&prefetched[batch]) == MCP2515::ERROR_OK) {
        lanePrefetchedRxMs[batch] = (uint32_t)millis();
        batch++;
      }
      if (batch == 0) break;
      noMoreFrames = batch < readBatchBudget;
      for (uint8_t bi = 0; bi < batch; ++bi) {
      canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                       CAN_TASK_STAGE_PROCESS,
                                       (uint32_t)millis());
      const struct can_frame &rxf = prefetched[bi];
      processed++;
      const uint32_t frameNow = (uint32_t)millis();
      lastCanAFrameMs = frameNow;
      mcpRxCount++;
      canRxObserve(CAN_RX_BUS_BODY, frameNow);
      canTxMarkFresh(CAN_TX_FRESH_BODY);
      __atomic_add_fetch(&mcpBodyRxCount, 1U, __ATOMIC_RELAXED);
      // All downstream Model YL decoders and TX decisions expect standard
      // 11-bit DATA frames. Aggregate telemetry still counts rejected frames.
      if ((rxf.can_id & 0xC0000000UL) != 0) continue;
      const uint16_t partyId = (uint16_t)(rxf.can_id & 0x7FF);
      bootCaptureObservePartyFrame(partyId, rxf.can_dlc, rxf.data);
      researchCaptureObserveParty(partyId, rxf.can_dlc, rxf.data);
      driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_A, partyId, rxf.can_dlc, rxf.data);
      if (partyId == UI_CHASSIS_CONTROL_ID && rxf.can_dlc >= 8)
        uiAutoLaneChangeObserveAndInjectCanA(rxf);
      if (partyId == 0x7FF && rxf.can_dlc == 8)
        countryOverrideObserve7ffCanA(rxf, countryRxEpoch);
      if (partyId == 0x238 && rxf.can_dlc == 8)
        countryOverrideObserve238CanA(rxf, countryRxEpoch);
      if (partyId == 0x3FD && rxf.can_dlc == 8)
        laneGraphObserveBody(rxf, countryRxEpoch, lanePrefetchedRxMs[bi]);
      if (activeCanAIsBody()) {
        // Standard 3/Y Body CAN: physical turn controls and front-interior
        // door-open switch used by the optional lane-change cancel action.
        if (nagTsl9Body39BSelected())
          (void)nagProcessTsl9Mcp(rxf);
        if (partyId == UI_POWERTRAIN_ID && rxf.can_dlc == 8 && activeProfilePedalMapSupported())
          handlePedalMap334OnCanA(rxf);
        if (partyId == LEFTSTALK_ID && rxf.can_dlc >= 3 && activeTurnSignalVariant == TURN_SIGNAL_STALK)
          handle249OnCanA(rxf.data, rxf.can_dlc);
        if (partyId == VCLEFT_SWITCH_ID && rxf.can_dlc >= 8 && activeTurnSignalVariant == TURN_SIGNAL_STALKLESS)
          handle3C2OnCanA(rxf);
        if (activeProfileIsYl() && partyId == VCLEFT_SWITCH_ID && rxf.can_dlc >= 8) {
          struct can_frame windowCompat = {};
          tmrCanFrameToChassis(rxf, windowCompat);
          handleDriverWindowLab3C2CanB(windowCompat);
        }
        if (partyId == VCLEFT_SWITCH_ID && rxf.can_dlc >= 8 &&
            activeProfileTsl9InputOnBodyCanA()) {
          tsl9InputObserveCanA(rxf);
        }
        if (partyId == DOOR_SWITCH_ID && rxf.can_dlc >= 4)
          handle102LaneChangeCancel(rxf.data, rxf.can_dlc);

        // TMR does not consume DAS_status 0x399 from BODY/J2. The NAG gate
        // is sourced exclusively from CHASSIS/J3 in the CAN-B task below.
        if (activeProfileIsYl() && partyId == 0x3FDu && rxf.can_dlc == 8 &&
            readMuxID(rxf.data) == 1u) {
          struct can_frame vhCompat = {};
          tmrCanFrameToChassis(rxf, vhCompat);
          (void)r79ProcessStockFrame(vhCompat, 1u, (uint32_t)millis());
        }
      }
      }
      canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                       CAN_TASK_STAGE_RECEIVE,
                                       (uint32_t)millis());
    }

    // ── J4 PARTY / MCP2515 ──
    // Party is independent from J2 BODY but shares the same SPI peripheral.
    // Keep the drain bounded so a saturated Party bus cannot starve Body.
    {
      uint8_t partyProcessed = 0;
      struct can_frame partyFrame = {};
      while (partyProcessed < MCP_RX_BUDGET &&
             Can_C.readMessage(&partyFrame) == MCP2515::ERROR_OK) {
        const uint32_t partyEpoch = canTxEpochSnapshot();
        const uint32_t partyNow = (uint32_t)millis();
        tmrProcessPartyFrame(partyFrame, partyNow, partyEpoch);
        partyProcessed++;
      }
      const uint8_t eflgParty = Can_C.getErrorFlags();
      if (eflgParty & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR))
        Can_C.clearRXnOVR();
      if (eflgParty & MCP2515::EFLG_TXBO) {
        mcpPartyState = 2;
        const unsigned long partyNowMs = millis();
        if (partyNowMs - lastMcpPartyRecoverMs >= 3000UL) {
          lastMcpPartyRecoverMs = partyNowMs;
          mcpPartyReady = false;
          if (mcpPartyReinit()) {
            mcpPartyState = 0;
            TMR_SERIAL_PRINTLN("[CAN C] PARTY BUS-OFF recovery complete -> RUNNING");
          } else {
            mcpPartyState = 2;
            TMR_SERIAL_PRINTLN("[CAN C] PARTY BUS-OFF recovery FAILED -> hard CAN recovery requested");
            requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_MCP_REINIT_FAIL);
          }
        }
      } else if (mcpPartyReady) {
        mcpPartyState = 0;
      }
    }

    canARxDiagnosticsCompleteLoopPure(canARxDiagnostics, processed,
                                       MCP_RX_BUDGET);
    canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                     CAN_TASK_STAGE_SERVICE,
                                     (uint32_t)millis());
    tsl9InputServiceCanA();

    // ── STATUS CHECK / RECOVERY (1 Hz) ──
    canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                     CAN_TASK_STAGE_STATUS,
                                     (uint32_t)millis());
    unsigned long now = millis();
    if (now - lastMcpStatusMs >= 1000) {
      lastMcpStatusMs = now;

      // Read the REAL MCP2515 error flags (EFLG register).
      uint8_t eflg = Can_A.getErrorFlags();
      mcpLastErrorFlags = eflg;
      mcpLastErrorFlagsMs = (uint32_t)now;

      // 1) RX overflow: MUST be cleared, otherwise the controller stops
      //    receiving in this buffer and the Nag Killer appears frozen.
      if (eflg & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR)) {
        mcpRxOverflowObserve((uint32_t)now, eflg);
        canARxDiagnosticsObserveOverflowPure(
            canARxDiagnostics, (uint32_t)now, eflg,
            (eflg & MCP2515::EFLG_RX0OVR) != 0,
            (eflg & MCP2515::EFLG_RX1OVR) != 0);
        Can_A.clearRXnOVR();
        TMR_SERIAL_PRINTLN("[CAN A] RX overflow flags cleared");
      }

      // 2) REAL bus-off via EFLG_TXBO only. Transient TX failures are not
      //    sufficient to declare BUS-OFF and must not trigger recovery.
      uint8_t consecutive = mcpTxFailConsecutive;
      bool busOff = (eflg & MCP2515::EFLG_TXBO);

      if (busOff) {
        const bool newlyObservedBusOff = mcpState != 2;
        if (newlyObservedBusOff) canATraceFreezeBusOff((uint32_t)now, eflg, consecutive);
        mcpState = 2; // BUS-OFF
        if (now - lastMcpRecoverMs > 3000) {
          lastMcpRecoverMs = now;
          TMR_SERIAL_PRINTF("[CAN A] MCP2515 bus-off (eflg=0x%02X txFailSeq=%u), reset...\n",
                        eflg, consecutive);
          invalidateCanTxStateForCanARecovery();
          if (!mcpReinit()) requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_MCP_REINIT_FAIL);
        }
      } else if (consecutive > 0 || (eflg & (MCP2515::EFLG_TXWAR | MCP2515::EFLG_RXWAR))) {
        mcpState = 1; // Warning
      } else {
        mcpState = 0; // OK
      }
    }

    canTaskDiagnosticsEnterStagePure(canTaskMcpDiagnostics,
                                     CAN_TASK_STAGE_DELAY,
                                     (uint32_t)millis());
    canTaskDiagnosticsFinishLoopPure(canTaskMcpDiagnostics,
                                     (uint32_t)micros());
    vTaskDelay(1);
  }
}

static void canTaskChassis(void* arg) {
  TMR_SERIAL_PRINTLN("[CAN B] CHASSIS task started");
  unsigned long lastChassisStatusMs = 0;
  unsigned long lastNoCanWarn = 0;
  uint32_t lastQueueStatusMs = 0;

  canTaskDiagnosticsResetPure(canTaskChassisDiagnostics);
  for (;;) {
    const uint32_t loopStartMs = (uint32_t)millis();
    canTaskChassisHeartbeatMs = loopStartMs;
    canTaskDiagnosticsHeartbeatPure(canTaskChassisDiagnostics, loopStartMs,
                                    (uint32_t)micros());
    if (canTasksStopping || canMaintenanceActive()) {
      canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                       CAN_TASK_STAGE_QUIESCE,
                                       (uint32_t)millis());
      canTaskChassisQuiesced = true;
      while (canTasksStopping || canMaintenanceActive()) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskChassisQuiesced = false;
      continue;
    }
    // Drain completion/error alerts before the potentially busy RX batch so
    // TX_SUCCESS observation latency stays bounded by one task loop.
    canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                     CAN_TASK_STAGE_ALERTS,
                                     (uint32_t)millis());
    canChassisHandleAlerts();
    struct can_frame f;
    uint8_t rxBudget = 0;
    canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                     CAN_TASK_STAGE_RECEIVE,
                                     (uint32_t)millis());
    uint32_t countryRxEpoch = canTxEpochSnapshot();
    esp_err_t rxResult = mcpChassisReceive(&f, pdMS_TO_TICKS(2));
    uint32_t laneRxMs = (uint32_t)millis();
    while (rxBudget < MCP_CHASSIS_RX_DRAIN_BUDGET && rxResult == ESP_OK) {
      rxBudget++;
      canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                       CAN_TASK_STAGE_PROCESS,
                                       (uint32_t)millis());
      // v3.6d2 R79 fast path: this is deliberately before millis(), RX-gap
      // accounting, capture, and normal decoding. Preserve the existing d1
      // fail-open/manual-latch authorization policy inside the fast function.
      bool mux1StockClaimed = false;
      if (!activeProfileIsYl() && !((f.can_id & CAN_EFF_FLAG) != 0) && !((f.can_id & CAN_RTR_FLAG) != 0) && f.can_id == 0x3FD && f.can_dlc >= 8) {
        const uint8_t timingMux = readMuxID(f.data);
        if (f.can_dlc == 8u && timingMux == 1u)
          visionControlCacheStock(VISION_CONTROL_CHASSIS_PURE, f.data, countryRxEpoch, laneRxMs);
        const uint32_t r79FrameNowMs = (uint32_t)millis();
        mux1StockClaimed = r79ProcessStockFrame(f, timingMux, r79FrameNowMs);
      }
      // Keep the supervisor heartbeat alive even under sustained CAN B traffic.
      const uint32_t frameNow = (uint32_t)millis();
      canTaskChassisHeartbeatMs = frameNow;
      canTaskDiagnosticsPulsePure(canTaskChassisDiagnostics, frameNow);
      const uint32_t previousCanBFrameMs = lastCanBFrameMs;
      if (previousCanBFrameMs != 0) {
        const uint32_t rxGapMs = (uint32_t)(frameNow - previousCanBFrameMs);
        canBLastRxGapMs = rxGapMs;
        if (rxGapMs > canBMaxRxGapMs) canBMaxRxGapMs = rxGapMs;
      }
      lastCanBFrameMs = frameNow;
      canRxObserve(CAN_RX_BUS_VH, frameNow);
      // Only standard 11-bit DATA frames may reach Tesla decoders or TX paths.
      if (!((f.can_id & CAN_EFF_FLAG) != 0) && !((f.can_id & CAN_RTR_FLAG) != 0)) {
        canTxMarkFresh(CAN_TX_FRESH_VH);
        bootCaptureObserveVhFrame(f.can_id, f.can_dlc);
        researchCaptureObserveVh((uint16_t)f.can_id, f.can_dlc, f.data);
        driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_B, (uint16_t)f.can_id, f.can_dlc, f.data);
        switch (f.can_id) {
        case UI_CHASSIS_CONTROL_ID:
          uiAutoLaneChangeObserveAndInjectCanB(f);
          break;
        case UI_POWERTRAIN_ID:
          // Model Y L carries the pedal-map frame on VH / CAN B. Standard 3/Y
          // Body+Chassis uses the CAN-A Body copy instead and must ignore any
          // numeric 0x334 that happens to exist on Chassis CAN.
          if (!activeProfileIsYl() && activeProfilePedalMapSupported()) handlePedalMap334OnCanB(f);
          break;
        // TMR: 0x249 stalk state is read exclusively from BODY/J2 (CAN A).
        // Do not consume or transmit 0x249 on CHASSIS/J3.
        case VCLEFT_SWITCH_ID:
          if (activeProfileIsYl() && f.can_dlc >= 8)
            handleDriverWindowLab3C2CanB(f);
          if (activeProfileTsl9InputSupported() &&
              !activeProfileTsl9InputOnBodyCanA() && f.can_dlc >= 8) {
            tsl9InputObserveCanB(f);
          }
          break;
        case DOOR_SWITCH_ID:
          if (activeProfileIsYl() && f.can_dlc >= 4)
            handle102LaneChangeCancel(f.data, f.can_dlc);
          break;
        case VISUAL_DEBUG_ID:
          if (activeCanBIsChassis() && f.can_dlc >= 8) {
            visualBehaviorType = (uint8_t)readBitsLE(f.data, 56, 2);
            visualDebugRxCount++;
            visualDebugLastMs = frameNow;
            if (activeProfileAdvancedEapSupported()) evaluateAutoBlinker();
          }
          break;
        case 280:
          if (activeCanBIsChassis() && f.can_dlc >= 7) handle280(f.data);
          break;
        case 390:
          if (activeCanBIsChassis() && f.can_dlc >= 8) handle390(f.data);
          break;
        case 921:
          if (activeCanBIsChassis() && f.can_dlc >= 8) {
            // TMR NAG gate: DAS_status 0x399 is sourced from CHASSIS/J3.
            // Always decode it before the PARTY/J4 0x370 torque path.
            nagUpdateApState(f.data, f.can_dlc);
            (void)nagProcessTsl9Chassis399(f);
            handle921(f.data, f.can_dlc);
          }
          break;
        case 0x331:
          if (activeCanBIsChassis()) doInjectTlsscRestore(f);
          break;
        // Model YL/public-DBC reference: UI_driverAssistMapData road context.
        case 0x238:
          if (f.can_dlc >= 5) handleRoadContext238(f.data, f.can_dlc);
          countryOverrideObserve238CanB(f, countryRxEpoch);
          break;
        case 0x7FF:
          countryOverrideObserve7ffCanB(f, countryRxEpoch);
          break;

        // 1016 (SPR) is read on CAN B for both models.
        case DRIVER_ASSIST_ID:
          // Always retain stock telemetry before applying the production/research overlay.
          handle1016(f.data, f.can_dlc);
          lab3f8FrameRxMs = laneRxMs;
          lab3f8FrameRxEpoch = countryRxEpoch;
          injectDriverAssistControl(f);
          break;
        case 1021:
          if (f.can_dlc >= 8) {
            uint8_t mux = readMuxID(f.data);
            if (mux == 1) {
              r79LabObserve3fdMux1(f.data, f.can_dlc);
              const bool ulcCloneClaimed = injectUlcSnooze3fdMux1(f);
              laneGraphObserveStock(f, countryRxEpoch, mux1StockClaimed || ulcCloneClaimed, laneRxMs);
            } else if (mux == 0) injectTLSSC(f);
          }
          break;

        default:
          break;
        }
      }
      canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                       CAN_TASK_STAGE_RECEIVE,
                                       (uint32_t)millis());
      countryRxEpoch = canTxEpochSnapshot();
      rxResult = rxBudget < MCP_CHASSIS_RX_DRAIN_BUDGET ? mcpChassisReceive(&f, 0) : ESP_ERR_TIMEOUT;
      laneRxMs = (uint32_t)millis();
    }

    // Refresh Summon evidence before R79 retry/periodic servicing. The 5 s
    // V2.6 PARK fallback remains gate-compatible, but d1 queue priority uses
    // fresh real gear rather than gateParked.
    canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                     CAN_TASK_STAGE_SERVICE,
                                     (uint32_t)millis());
    refreshSummonState();
    driverWindowLabServiceTick();
    r79TransportTick();
    // R79 immediate/retry/periodic work always gets first access to the CAN-B
    // TX queue. TSL9 input assistance is intentionally lower priority.
    tsl9InputServiceCanB();
    const uint32_t captureNow = (uint32_t)millis();
    researchCaptureTick(captureNow);
    driverMonitorCaptureTick(captureNow);
    const uint32_t queueNow = (uint32_t)millis();
    if ((uint32_t)(queueNow - lastQueueStatusMs) >= MCP_CHASSIS_QUEUE_TELEMETRY_PERIOD_MS) {
      lastQueueStatusMs = queueNow;
      mcpChassisReadQueueStatus();
    }
    blinkATxTick();

    // Read alerts again after RX/tick work. BUS_OFF snapshot behavior is
    // preserved; d3 additionally consumes TX_IDLE/TX_SUCCESS for diagnostics.
    canChassisHandleAlerts();

    // CHASSIS status / recovery. Recovery ends in STOPPED, so explicitly
    // restart the driver instead of leaving CAN B silent after BUS_OFF.
    canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                     CAN_TASK_STAGE_STATUS,
                                     (uint32_t)millis());
    unsigned long now = millis();
    if (now - lastChassisStatusMs >= 1000) {
      lastChassisStatusMs = now;
      McpChassisStatus st = {};
      if (mcpChassisGetStatus(&st) == ESP_OK) {
        if (st.state == MCP_CHASSIS_STATE_RUNNING) {
          mcpChassisReady = true;
        } else if (st.state == MCP_CHASSIS_STATE_BUS_OFF) {
          mcpChassisReady = false;
          canChassisBusOffCount++;
          canChassisLastEventReason = CAN_REC_CHASSIS_BUS_OFF;
          canChassisLastEventMs = (uint32_t)now;
          uint32_t frozenOrdinal = 0;
          portENTER_CRITICAL(&canBTxTraceMux);
          frozenOrdinal = canBTxTraceFrozenBusOffOrdinal;
          portEXIT_CRITICAL(&canBTxTraceMux);
          if (frozenOrdinal != canChassisBusOffCount) {
            // Alert delivery is normally immediate. If it was missed, preserve
            // the old poll-based behavior as a fail-safe and freeze now.
            canBTraceFreezeBusOff((uint32_t)now);
            recordChassisBusOffSnapshot(st, (uint32_t)now);
          }
          canBusOffPersistenceMarkDirty(CAN_BUS_OFF_BUS_B_PURE);
          TMR_SERIAL_PRINTLN("[CAN B] CHASSIS bus-off -> recovery started");
          invalidateCanTxStateForCanBRecovery();
          const esp_err_t recoveryErr = tmrChassisRecoverChecked("BUS-OFF recovery");
          if (recoveryErr == ESP_OK) {
            canChassisLocalRecoveryStartCount++;
            canChassisLastEventReason = CAN_REC_NONE;
            canChassisLastEventMs = (uint32_t)now;
            TMR_SERIAL_PRINTLN("[CAN B] CHASSIS BUS-OFF recovery complete -> RUNNING");
          } else {
            canChassisRecoveryStartFailCount++;
            canChassisLastEventReason = CAN_REC_CHASSIS_RECOVERY_FAIL;
            canChassisLastEventMs = (uint32_t)now;
            TMR_SERIAL_PRINTF("[CAN B] CHASSIS recovery start failed: %s\n", esp_err_to_name(recoveryErr));
            requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_CHASSIS_RECOVERY_FAIL);
          }
        } else if (st.state == MCP_CHASSIS_STATE_STOPPED) {
          mcpChassisReady = false;
          canChassisStoppedCount++;
          if (canChassisLastEventReason == CAN_REC_NONE) {
            canChassisLastEventReason = CAN_REC_CHASSIS_STOPPED;
            canChassisLastEventMs = (uint32_t)now;
          }
          invalidateCanTxStateForCanBRecovery();
          esp_err_t rs = tmrChassisRecoverChecked("STOPPED recovery");
          if (rs == ESP_OK) {
            canChassisRestartOkCount++;
            mcpChassisReady = true;
            canChassisLastEventReason = CAN_REC_NONE;
            canChassisLastEventMs = (uint32_t)now;
            TMR_SERIAL_PRINTLN("[CAN B] CHASSIS STOPPED recovery complete -> RUNNING");
          } else {
            canChassisRestartFailCount++;
            canChassisLastEventReason = CAN_REC_CHASSIS_RESTART_FAIL;
            canChassisLastEventMs = (uint32_t)now;
            TMR_SERIAL_PRINTF("[CAN B] CHASSIS restart failed: %s\n", esp_err_to_name(rs));
            requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_CHASSIS_RESTART_FAIL);
          }
        } else {
          mcpChassisReady = false;
        }
      }
    }

    // No-CAN warning (shared counter)
    if ((millis() - bootTime) > 20000 && canRxTotal() == 0) {
      if (millis() - lastNoCanWarn > 5000) {
        TMR_SERIAL_PRINTLN("No CAN frames yet on either bus, staying alive.");
        lastNoCanWarn = millis();
      }
    }

    canTaskDiagnosticsEnterStagePure(canTaskChassisDiagnostics,
                                     CAN_TASK_STAGE_DELAY,
                                     (uint32_t)millis());
    canTaskDiagnosticsFinishLoopPure(canTaskChassisDiagnostics,
                                     (uint32_t)micros());
    vTaskDelay(1);
  }
}


// ═══════════════════════════════════════════════════════════════
// RECOVERY-ONLY CAN SUBSYSTEM SUPERVISOR
// ═══════════════════════════════════════════════════════════════

static inline bool recoveryFresh(uint32_t now, uint32_t ts, uint32_t timeoutMs) {
  return ts != 0 && (uint32_t)(now - ts) <= timeoutMs;
}

static void recordCanTaskHeartbeatTimeout(uint32_t now, bool aDead, bool bDead) {
  const uint8_t cause = aDead
      ? (bDead ? CAN_TASK_HEARTBEAT_BOTH : CAN_TASK_HEARTBEAT_A)
      : (bDead ? CAN_TASK_HEARTBEAT_B : CAN_TASK_HEARTBEAT_NONE);
  if (cause == CAN_TASK_HEARTBEAT_NONE) return;
  const uint8_t stateA = canTaskStateCode(canTaskBodyHandle);
  const uint8_t stateB = canTaskStateCode(canTaskChassisHandle);
  const uint32_t stackA = canTaskBodyHandle
      ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskBodyHandle) : 0u;
  const uint32_t stackB = canTaskChassisHandle
      ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskChassisHandle) : 0u;
  const CanTaskTimeoutSnapshotPure snapshotA = canTaskDiagnosticsSnapshotPure(
      canTaskMcpDiagnostics, now, stateA, stackA);
  const CanTaskTimeoutSnapshotPure snapshotB = canTaskDiagnosticsSnapshotPure(
      canTaskChassisDiagnostics, now, stateB, stackB);
  portENTER_CRITICAL(&canRecoveryMux);
  // Snapshot once for the restart request that is about to be queued. The
  // supervisor may loop again before reinitialization starts.
  if (canSupervisorCommand < CAN_SUP_HARD_STALE) {
    canTaskHeartbeatLastCause = cause;
    canTaskHeartbeatLastAgeAms = snapshotA.heartbeatAgeMs;
    canTaskHeartbeatLastAgeBms = snapshotB.heartbeatAgeMs;
    canTaskHeartbeatLastSnapshotA = snapshotA;
    canTaskHeartbeatLastSnapshotB = snapshotB;
    if (cause == CAN_TASK_HEARTBEAT_A) canTaskHeartbeatTimeoutCountA++;
    else if (cause == CAN_TASK_HEARTBEAT_B) canTaskHeartbeatTimeoutCountB++;
    else canTaskHeartbeatTimeoutCountBoth++;
  }
  portEXIT_CRITICAL(&canRecoveryMux);
}

static void requestCanSubsystemRestart(uint8_t reason, uint8_t diagReason) {
  portENTER_CRITICAL(&canRecoveryMux);
  // Keep the first cause at a given supervisor priority. A higher-priority
  // request (for example MANUAL over STALE) replaces both command and cause.
  if (reason > canSupervisorCommand) {
    canSupervisorCommand = reason;
    canPendingHardDiagReason = diagReason;
  }
  portEXIT_CRITICAL(&canRecoveryMux);
}

static bool recoveryMcpColdInit() {
  // TMR uses one shared SPI bus. Never end/restart SPI around one controller,
  // and never toggle GPIO9: it is J3 CHASSIS chip-select.
  if (!mcpSpiStarted) {
    SPI.begin(MCP2515_SCLK, MCP2515_MISO, MCP2515_MOSI);
    mcpSpiStarted = true;
  }
  pinMode(MCP2515_BODY_CS, OUTPUT);
  pinMode(MCP2515_CHASSIS_CS, OUTPUT);
  pinMode(MCP2515_PARTY_CS, OUTPUT);
  digitalWrite(MCP2515_BODY_CS, HIGH);
  digitalWrite(MCP2515_CHASSIS_CS, HIGH);
  digitalWrite(MCP2515_PARTY_CS, HIGH);

  pinMode(MCP2515_BODY_STBY, OUTPUT);
  pinMode(MCP2515_CHASSIS_STBY, OUTPUT);
  pinMode(MCP2515_PARTY_STBY, OUTPUT);
  digitalWrite(MCP2515_BODY_STBY, LOW);
  digitalWrite(MCP2515_CHASSIS_STBY, LOW);
  digitalWrite(MCP2515_PARTY_STBY, LOW);
  delay(5);

  const bool bodyOk = mcpInitChecked();
  const bool partyOk = mcpPartyInitChecked();

  // Reuse the compatibility layer to initialize J3 CHASSIS.
  mcpChassisReady = false;
  const MCP2515::ERROR r = Can_B.reset();
  delay(2);
  const MCP2515::ERROR b = (r == MCP2515::ERROR_OK)
      ? Can_B.setBitrate(CAN_500KBPS, MCP_CLOCK) : r;
  const MCP2515::ERROR m = (b == MCP2515::ERROR_OK)
      ? Can_B.setNormalMode() : b;
  mcpChassisReady = r == MCP2515::ERROR_OK &&
              b == MCP2515::ERROR_OK &&
              m == MCP2515::ERROR_OK;
  return bodyOk && partyOk && mcpChassisReady;
}

static bool recoveryChassisInstallFresh() {
  return tmrChassisRecoverChecked("hard install");
}

static bool recoveryChassisFullReinit() {
  return tmrChassisRecoverChecked("hard reinit");
}

// Maintenance never deletes tasks while they may own SPI/state locks and never
// starts a controller. OTA must not touch flash until every owner has parked.
static bool prepareCanForMaintenance() {
  __atomic_store_n(&canMaintenanceRequested, true, __ATOMIC_RELEASE);
  const bool supervisorSelf = canSupervisorHandle &&
      xTaskGetCurrentTaskHandle() == canSupervisorHandle;
  if (supervisorSelf)
    __atomic_store_n(&canMaintenanceSupervisorParked, true, __ATOMIC_RELEASE);
  bool expected = false;
  if (!__atomic_compare_exchange_n(&canMaintenancePreparing, &expected, true,
                                  false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return false;
  const auto finish = [](bool ok) {
    __atomic_store_n(&canMaintenancePreparing, false, __ATOMIC_RELEASE);
    return ok;
  };
  if (__atomic_load_n(&canMaintenanceStopped, __ATOMIC_ACQUIRE)) return finish(true);
  // Publish the hold first; then drain any sender that already owns the barrier.
  canTxAdministrativeHold = true;
  if (canTxBarrierMutex) {
    if (xSemaphoreTake(canTxBarrierMutex, pdMS_TO_TICKS(500)) != pdTRUE) return finish(false);
    canTxAdministrativeHold = true;
    xSemaphoreGive(canTxBarrierMutex);
  }
  uint32_t started = (uint32_t)millis();
  while (canSupervisorHandle &&
         !__atomic_load_n(&canMaintenanceSupervisorParked, __ATOMIC_ACQUIRE)) {
    if ((uint32_t)((uint32_t)millis() - started) >= 2000u) return finish(false);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  started = (uint32_t)millis();
  while ((canTaskBodyHandle && !canTaskMcpQuiesced) ||
         (canTaskChassisHandle && !canTaskChassisQuiesced)) {
    if ((uint32_t)((uint32_t)millis() - started) >= 500u) return finish(false);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  // All CAN owners are parked. Put all three TMR transceivers in standby.
  mcpReady = false;
  mcpPartyReady = false;
  mcpChassisReady = false;
  pinMode(MCP2515_BODY_STBY, OUTPUT);
  pinMode(MCP2515_CHASSIS_STBY, OUTPUT);
  pinMode(MCP2515_PARTY_STBY, OUTPUT);
  digitalWrite(MCP2515_BODY_STBY, HIGH);
  digitalWrite(MCP2515_CHASSIS_STBY, HIGH);
  digitalWrite(MCP2515_PARTY_STBY, HIGH);
  invalidateCanTxStateForFullRecovery();
  __atomic_store_n(&canMaintenanceStopped, true, __ATOMIC_RELEASE);
  return finish(true);
}

static void restartTmrCanSafely() {
  // A failed handshake never falls through to an uncoordinated reboot.
  while (!prepareCanForMaintenance()) vTaskDelay(pdMS_TO_TICKS(100));
  ESP.restart();
}

static void recoveryStopCanTasks() {
  canTasksStopping = true;
  canTaskMcpQuiesced = false;
  canTaskChassisQuiesced = false;

  uint32_t start = (uint32_t)millis();
  while ((!canTaskMcpQuiesced || !canTaskChassisQuiesced) &&
         (uint32_t)((uint32_t)millis() - start) < 300) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  TaskHandle_t a = canTaskBodyHandle;
  TaskHandle_t b = canTaskChassisHandle;
  canTaskBodyHandle = nullptr;
  canTaskChassisHandle = nullptr;
  if (a) vTaskDelete(a);
  if (b) vTaskDelete(b);

  canTasksStopping = false;
  canTaskMcpQuiesced = false;
  canTaskChassisQuiesced = false;
  canTaskMcpHeartbeatMs = 0;
  canTaskChassisHeartbeatMs = 0;
  canTaskDiagnosticsResetPure(canTaskMcpDiagnostics);
  canTaskDiagnosticsResetPure(canTaskChassisDiagnostics);
  vTaskDelay(pdMS_TO_TICKS(RECOVERY_TASK_STOP_SETTLE_MS));
}

static bool recoveryStartCanTasks() {
  BaseType_t a = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, &canTaskBodyHandle, 1);
  if (a != pdPASS) {
    canTaskBodyHandle = nullptr;
    return false;
  }
  BaseType_t b = xTaskCreatePinnedToCore(canTaskChassis, "canB", 8192, nullptr, 4, &canTaskChassisHandle, 1);
  if (b != pdPASS) {
    vTaskDelete(canTaskBodyHandle);
    canTaskBodyHandle = nullptr;
    canTaskChassisHandle = nullptr;
    return false;
  }
  return true;
}

static bool recoveryHardReinitialize(uint8_t reason, uint8_t diagReason) {
  if (canSubsystemBusy) return false;
  canSubsystemBusy = true;
  const int8_t bootCapHardIdx = bootCaptureHardStart(reason);
  canLastHardReinitReason = reason;
  canLastHardDiagReason = diagReason;
  canHardReinitCount++;
  TMR_SERIAL_PRINTF("[CAN SUP] hard CAN reinitialize #%lu reason=%u diag=%s\n",
                (unsigned long)canHardReinitCount, (unsigned)reason,
                canRecoveryDiagnosticReasonName(diagReason));

  // Preserve only the dashboard presentation. invalidateCanTxStateForFullRecovery()
  // still closes every functional AP/NOA gate before the controllers restart.
  portENTER_CRITICAL(&stateMux);
  apDisplayBeginHardInitPure(apDisplayHold, (uint32_t)millis());
  portEXIT_CRITICAL(&stateMux);

  recoveryStopCanTasks();
  invalidateCanTxStateForFullRecovery();
  bool aOk = recoveryMcpColdInit();
  bool bOk = recoveryChassisFullReinit();
  if (aOk) mcpState = 0;
  if (bOk) mcpChassisReady = true;
  if (mcpPartyReady) mcpPartyState = 0;
  bool tasksOk = aOk && bOk && recoveryStartCanTasks();

  lastCanAFrameMs = 0;
  lastCanBFrameMs = 0;
  canInitTime = millis();
  recoveryOneBusStaleStartMs = 0;
  recoveryWakeAcquireStartMs = (reason == CAN_SUP_HARD_ACQUIRE || recoveryEverBothActive)
                                 ? (uint32_t)millis() : 0;
  canSubsystemBusy = false;

  if (!(aOk && bOk && tasksOk)) {
    bootCaptureHardFinish(bootCapHardIdx, false);
    canHardReinitFailCount++;
    TMR_SERIAL_PRINTF("[CAN SUP] hard CAN reinitialize FAILED A=%u B=%u tasks=%u\n",
                  aOk ? 1 : 0, bOk ? 1 : 0, tasksOk ? 1 : 0);
    return false;
  }
  bootCaptureHardFinish(bootCapHardIdx, true);
  TMR_SERIAL_PRINTLN("[CAN SUP] hard CAN reinitialize complete");
  return true;
}

static void canRecoverySupervisorTick(uint32_t now) {
  canBusOffPersistenceService(now);
}

static void canSupervisorTask(void* arg) {
  TMR_SERIAL_PRINTLN("[CAN SUP] recovery-only supervisor started");
  for (;;) {
    uint32_t now = (uint32_t)millis();
    if (canMaintenanceActive()) {
      __atomic_store_n(&canMaintenanceSupervisorParked, true, __ATOMIC_RELEASE);
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    canRecoverySupervisorTick(now);

    if (!canSubsystemBusy) {
      // Independent task heartbeat: still advances while the vehicle is asleep.
      // Therefore silence on the CAN wires is not confused with a wedged task.
      bool graceDone = (uint32_t)(now - canInitTime) >= RECOVERY_TASK_START_GRACE_MS;
      bool aTaskDead = canTaskBodyHandle && graceDone &&
                       (canTaskMcpHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskMcpHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      bool bTaskDead = canTaskChassisHandle && graceDone &&
                       (canTaskChassisHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskChassisHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      if (aTaskDead || bTaskDead) {
        TMR_SERIAL_PRINTF("[CAN SUP] task heartbeat stale A=%u B=%u\n", aTaskDead ? 1 : 0, bTaskDead ? 1 : 0);
        recordCanTaskHeartbeatTimeout(now, aTaskDead, bTaskDead);
        requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_TASK_HEARTBEAT_TIMEOUT);
      }

      // IMPORTANT: RX silence is NOT a CAN fault.
      //
      // CAN A/B can legitimately become asymmetric or both can go quiet
      // (vehicle sleep, gateway scheduling, low-traffic periods, etc.).
      // Therefore traffic freshness is diagnostic-only and MUST NOT request
      // a hard CAN subsystem reinitialization.
      //
      // Real recovery remains driven by:
      //   - controller BUS_OFF / STOPPED handling in the CAN tasks;
      //   - failed local controller reinitialization;
      //   - CAN task heartbeat timeout.
      //
      // Keep the traffic state for diagnostics / sleep indication only.
      bool aFresh = recoveryFresh(now, lastCanAFrameMs, RECOVERY_BUS_FRESH_MS);
      bool bFresh = recoveryFresh(now, lastCanBFrameMs, RECOVERY_BUS_FRESH_MS);
      bool bothFresh = aFresh && bFresh;
      bool anyFresh = aFresh || bFresh;

      if (bothFresh) {
        if (!recoveryEverBothActive || recoverySleeping) {
          TMR_SERIAL_PRINTLN("[CAN SUP] CAN A+B active");
        }
        recoveryEverBothActive = true;
        recoverySleeping = false;
        recoveryWakeAcquireStartMs = 0;
        recoveryOneBusStaleStartMs = 0;
        recoveryLastBothActiveMs = now;
        recoveryColdRetryCount = 0;
        recoveryColdRetriesExhausted = false;
      } else if (recoveryEverBothActive) {
        if (!anyFresh) {
          recoveryOneBusStaleStartMs = 0;
          if (!recoverySleeping && recoveryLastBothActiveMs != 0 &&
              (uint32_t)(now - recoveryLastBothActiveMs) >= RECOVERY_SLEEP_QUIET_MS) {
            recoverySleeping = true;
            recoveryWakeAcquireStartMs = 0;
            canRecoverySleepCount++;
            TMR_SERIAL_PRINTF("[CAN SUP] vehicle CAN sleep #%lu -> passive wait\n",
                          (unsigned long)canRecoverySleepCount);
          }
        } else if (recoverySleeping) {
          recoverySleeping = false;
          recoveryWakeAcquireStartMs = now;
          canRecoveryWakeCount++;
          TMR_SERIAL_PRINTF("[CAN SUP] vehicle CAN wake #%lu -> passive monitoring\n",
                        (unsigned long)canRecoveryWakeCount);
        }
      } else {
        // Cold boot: wait passively for CAN traffic. Do NOT tear down and
        // reinitialize the controllers merely because no RX frame arrived.
        recoveryColdRetryCount = 0;
        recoveryColdRetriesExhausted = false;
        if (anyFresh) {
          recoveryEverBothActive = false;
          recoveryWakeAcquireStartMs = now;
        }
      }
    }

    uint8_t cmd = CAN_SUP_NONE;
    uint8_t diagReason = CAN_REC_NONE;
    portENTER_CRITICAL(&canRecoveryMux);
    cmd = canSupervisorCommand;
    diagReason = canPendingHardDiagReason;
    canSupervisorCommand = CAN_SUP_NONE;
    canPendingHardDiagReason = CAN_REC_NONE;
    portEXIT_CRITICAL(&canRecoveryMux);

    if (cmd != CAN_SUP_NONE && !canSubsystemBusy) {
      if (!recoveryHardReinitialize(cmd, diagReason)) {
        TMR_SERIAL_PRINTLN("[CAN SUP] subsystem recovery failed -> reboot T-2CAN");
        restartTmrCanSafely();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ═══════════════════════════════════════════════════════════════
// SETUP / LOOP
// ═══════════════════════════════════════════════════════════════
