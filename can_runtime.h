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
  uint8_t mask = SUMMON_BUS_NONE;
  const uint32_t lastA = lastCanAFrameMs;
  const uint32_t lastB = lastCanBFrameMs;
  if (lastA != 0 && (uint32_t)(now - lastA) <= RECOVERY_BUS_FRESH_MS)
    mask = (uint8_t)(mask | SUMMON_BUS_A);
  if (lastB != 0 && (uint32_t)(now - lastB) <= RECOVERY_BUS_FRESH_MS)
    mask = (uint8_t)(mask | SUMMON_BUS_B);
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
  refreshSummonDerivedStateLocked(now);
  portEXIT_CRITICAL(&stateMux);

  if (inv.invalidateTemplate) {
    portENTER_CRITICAL(&r79LabMux);
    r79LabStockValid = false;
    r79LabLast3fdMs = 0;
    r79LabLastAttemptMs = 0;
    r79LabLastPeriodicRequestMs = 0;
    r79LabLastPeriodicTxMs = 0;
    r79FixedQuietState = {};
    r79RetryPending = false;
    r79RetryIndex = 0;
    r79RetryOriginKind = R79LAB_TX_NONE;
    r79RetryDueMs = 0;
    r79LabLastTxValid = false;
    r79LabLastBlockReason = R79LAB_BLOCK_TEMPLATE;
    r79LabLastBlockMs = now;
    memset(r79LabLastStockRaw, 0, sizeof(r79LabLastStockRaw));
    memset(r79LabLastEffectiveRaw, 0, sizeof(r79LabLastEffectiveRaw));
    portEXIT_CRITICAL(&r79LabMux);
  }
  apRightScrollRuntimeReset(false);

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

static void recordTwaiBusOffSnapshot(const twai_status_info_t &st, uint32_t now) {
  CanTwaiRecoverySnapshot snap = {};
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
  canTwaiLastBusOffSnapshot = snap;
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
  canBTxTraceFrozenBusOffOrdinal = canTwaiBusOffCount + 1U;
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
static bool mcpInitChecked() {
  mcpReady = false;
  const MCP2515::ERROR resetErr = Can_A.reset();
  delay(2); // conservative margin beyond MCP2515 128-cycle oscillator startup
  const MCP2515::ERROR rateErr = (resetErr == MCP2515::ERROR_OK)
                                   ? Can_A.setBitrate(CAN_500KBPS, MCP_CLOCK)
                                   : resetErr;
  const MCP2515::ERROR modeErr = (rateErr == MCP2515::ERROR_OK)
                                   ? Can_A.setNormalMode()
                                   : rateErr;
  const bool ok = resetErr == MCP2515::ERROR_OK &&
                  rateErr == MCP2515::ERROR_OK &&
                  modeErr == MCP2515::ERROR_OK;
  mcpReady = ok;
  if (ok) {
    mcpTxFailConsecutive = 0;
    mcpState = 0;
  } else {
    mcpState = 2;
    T2CAN_SERIAL_PRINTF("[CAN A] init failed: reset=%d bitrate=%d mode=%d\n",
                  (int)resetErr, (int)rateErr, (int)modeErr);
  }
  return ok;
}

static bool mcpReinit() {
  return mcpInitChecked();
}

static void canTwaiHandleAlerts() {
  uint32_t twaiAlerts = 0;
  if (twai_read_alerts(&twaiAlerts, 0) != ESP_OK || twaiAlerts == 0) return;
  const int64_t observedUs = esp_timer_get_time();
  if (twaiAlerts & TWAI_ALERT_TX_IDLE) canBTxObserveTxIdleAlert();
  if (twaiAlerts & TWAI_ALERT_TX_SUCCESS) {
    const uint32_t serialNow = canBTxAcceptedSerialSnapshot();
    r79FastReactiveObserveTxSuccessAlert(observedUs, serialNow);
  }
  if (twaiAlerts & TWAI_ALERT_BUS_OFF) {
    const uint32_t alertNow = (uint32_t)millis();
    twai_status_info_t alertSt = {};
    canBTraceFreezeBusOff(alertNow);
    if (twai_get_status_info(&alertSt) == ESP_OK) recordTwaiBusOffSnapshot(alertSt, alertNow);
  }
}

static void canTaskMcp(void* arg) {
  T2CAN_SERIAL_PRINTLN("[CAN A] MCP2515 task started");
  for (;;) {
    canTaskMcpHeartbeatMs = (uint32_t)millis();
    if (canTasksStopping) {
      canTaskMcpQuiesced = true;
      while (canTasksStopping) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskMcpQuiesced = false;
      continue;
    }
    // ── BOUNDED READ LOOP ──
    // d3 prefetches the tiny MCP2515 hardware RX buffers before running the
    // heavier observers/decoders. This shortens the time an unread frame sits
    // in RXB0/RXB1 during burst traffic without changing the total 32-frame
    // yield budget or task priority.
    static constexpr uint8_t MCP_PREFETCH_BUDGET = 4;
    struct can_frame prefetched[MCP_PREFETCH_BUDGET];
    bool partyFrames[MCP_PREFETCH_BUDGET];
    uint8_t processed = 0;
    bool noMoreFrames = false;
    while (processed < MCP_RX_BUDGET && !noMoreFrames) {
      uint8_t batch = 0;
      while (batch < MCP_PREFETCH_BUDGET &&
             processed + batch < MCP_RX_BUDGET &&
             Can_A.readMessage(&prefetched[batch], &partyFrames[batch]) == MCP2515::ERROR_OK) {
        batch++;
      }
      if (batch == 0) break;
      noMoreFrames = batch < MCP_PREFETCH_BUDGET;
      for (uint8_t bi = 0; bi < batch; ++bi) {
      const struct can_frame &rxf = prefetched[bi];
      processed++;
      const uint32_t frameNow = (uint32_t)millis();
      if (!(rxf.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG)))
        usbDiagUlcObserve(board == BOARD_TMR && partyFrames[bi] ? 2 : 0, rxf.can_id, rxf.can_dlc, rxf.data);
      if (boardTripleCan() && partyFrames[bi]) {
        boardPartyRx++;
        boardPartyLastRx = frameNow;
        if (!(rxf.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG)))
          nagProcessMcpFrame(rxf);
        continue; // Party IDs must never feed Body observers or freshness.
      }
      lastCanAFrameMs = frameNow;
      mcpRxCount++;
      canRxObserve(CAN_RX_BUS_PARTY, frameNow);
      // All downstream Model YL decoders and TX decisions expect standard
      // 11-bit DATA frames. Aggregate telemetry still counts rejected frames.
      if ((rxf.can_id & 0xC0000000UL) != 0) continue;
      const uint16_t partyId = (uint16_t)(rxf.can_id & 0x7FF);
      canTxMarkFresh(CAN_TX_FRESH_PARTY); // physical CAN A fresh in this recovery epoch
      if (partyId == 0x7FF) r79LabObserve7ff(T2CAN_BUS_PARTY, rxf.can_dlc, rxf.data);
      bootCaptureObservePartyFrame(partyId, rxf.can_dlc, rxf.data);
      researchCaptureObserveParty(partyId, rxf.can_dlc, rxf.data);
      driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_A, partyId, rxf.can_dlc, rxf.data);
      if (partyId == UI_CHASSIS_CONTROL_ID && rxf.can_dlc >= 8)
        uiAutoLaneChangeObserveAndInjectCanA(rxf);
      if (activeCanAIsParty()) {
        // Party CAN on CAN A: Nag Killer is topology-gated. Keep YL-only
        // DAS/Summon/visual-debug behavior explicitly model-gated so selecting
        // Party + Chassis on Standard 3/Y cannot accidentally enable YL paths.
        nagProcessMcpFrame(rxf);
        if (activeProfileIsYl() && partyId == VISUAL_DEBUG_ID && rxf.can_dlc >= 8) {
          visualBehaviorType = (uint8_t)readBitsLE(rxf.data, 56, 2);
          visualDebugRxCount++;
          visualDebugLastMs = frameNow;
          evaluateAutoBlinker();
        }
      } else if (activeCanAIsBody()) {
        // Standard 3/Y Body CAN: physical turn controls and front-interior
        // door-open switch used by the optional lane-change cancel action.
        if (partyId == UI_POWERTRAIN_ID && rxf.can_dlc == 8 && activeProfilePedalMapSupported())
          handlePedalMap334OnCanA(rxf);
        if (partyId == LEFTSTALK_ID && rxf.can_dlc >= 3 && activeTurnSignalVariant == TURN_SIGNAL_STALK)
          handle249OnCanA(rxf.data, rxf.can_dlc);
        if (partyId == VCLEFT_SWITCH_ID && rxf.can_dlc >= 8 && activeTurnSignalVariant == TURN_SIGNAL_STALKLESS)
          handle3C2OnCanA(rxf);
        if (partyId == DOOR_SWITCH_ID && rxf.can_dlc >= 4)
          handle102LaneChangeCancel(rxf.data, rxf.can_dlc);
      }
      }
    }

    // ── STATUS CHECK / RECOVERY (1 Hz) ──
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
        Can_A.clearRXnOVR();
        T2CAN_SERIAL_PRINTLN("[CAN A] RX overflow flags cleared");
      }

      // 2) REAL bus-off via EFLG_TXBO (not only TX failures).
      uint8_t consecutive = mcpTxFailConsecutive;
      bool busOff = (eflg & MCP2515::EFLG_TXBO) || (consecutive > 5);

      if (busOff) {
        const bool newlyObservedBusOff = mcpState != 2;
        if (newlyObservedBusOff) canATraceFreezeBusOff((uint32_t)now, eflg, consecutive);
        mcpState = 2; // BUS-OFF
        if (now - lastMcpRecoverMs > 3000) {
          lastMcpRecoverMs = now;
          T2CAN_SERIAL_PRINTF("[CAN A] MCP2515 bus-off (eflg=0x%02X txFailSeq=%u), reset...\n",
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

    vTaskDelay(1);
  }
}

static void canTaskTwai(void* arg) {
  T2CAN_SERIAL_PRINTLN("[CAN B] TWAI task started");
  unsigned long lastTwaiStatusMs = 0;
  unsigned long lastNoCanWarn = 0;
  uint32_t lastQueueStatusMs = 0;

  for (;;) {
    canTaskTwaiHeartbeatMs = (uint32_t)millis();
    if (canTasksStopping) {
      canTaskTwaiQuiesced = true;
      while (canTasksStopping) vTaskDelay(pdMS_TO_TICKS(5));
      canTaskTwaiQuiesced = false;
      continue;
    }
    // Drain completion/error alerts before the potentially busy RX batch so
    // TX_SUCCESS observation latency stays bounded by one task loop.
    canTwaiHandleAlerts();
    twai_message_t f;
    uint8_t rxBudget = 0;
    esp_err_t rxResult = twai_receive(&f, pdMS_TO_TICKS(2));
    while (rxBudget < TWAI_RX_DRAIN_BUDGET && rxResult == ESP_OK) {
      rxBudget++;
      // d2 latency origin: this is the earliest software timestamp available
      // after the TWAI driver dequeues the received frame. It is not a hardware
      // wire timestamp, but it lets us measure software dispatch -> TX request.
      const int64_t frameRxDequeueUs = esp_timer_get_time();
      // v3.6d2 R79 fast path: this is deliberately before millis(), RX-gap
      // accounting, capture, and normal decoding. Preserve the existing d1
      // fail-open/manual-latch authorization policy inside the fast function.
      if (!f.extd && !f.rtr && f.identifier == 0x3FD && f.data_length_code >= 8) {
        const uint8_t timingMux = readMuxID(f.data);
        const uint32_t r79FrameNowMs = (uint32_t)millis();
        if (timingMux == 1u) (void)r79FixedFastEcho(f, frameRxDequeueUs);
        r79FixedObserveStock(timingMux, r79FrameNowMs);
      }
      // Keep the supervisor heartbeat alive even under sustained CAN B traffic.
      const uint32_t frameNow = (uint32_t)millis();
      canTaskTwaiHeartbeatMs = frameNow;
      const uint32_t previousCanBFrameMs = lastCanBFrameMs;
      if (previousCanBFrameMs != 0) {
        const uint32_t rxGapMs = (uint32_t)(frameNow - previousCanBFrameMs);
        canBLastRxGapMs = rxGapMs;
        if (rxGapMs > canBMaxRxGapMs) canBMaxRxGapMs = rxGapMs;
      }
      lastCanBFrameMs = frameNow;
      canRxObserve(CAN_RX_BUS_VH, frameNow);
      // Only standard 11-bit DATA frames may reach Tesla decoders or TX paths.
      if (!f.extd && !f.rtr) {
        usbDiagUlcObserve(1, f.identifier, f.data_length_code, f.data);
        canTxMarkFresh(CAN_TX_FRESH_VH);
        if (f.identifier == 0x7FF) r79LabObserve7ff(T2CAN_BUS_VH, f.data_length_code, f.data);
        bootCaptureObserveVhFrame(f.identifier, f.data_length_code);
        researchCaptureObserveVh((uint16_t)f.identifier, f.data_length_code, f.data);
        driverMonitorCaptureObserve(DRIVER_MONITOR_BUS_B, (uint16_t)f.identifier, f.data_length_code, f.data);
        switch (f.identifier) {
        case UI_CHASSIS_CONTROL_ID:
          uiAutoLaneChangeObserveAndInjectCanB(f);
          break;
        case UI_POWERTRAIN_ID:
          // Model Y L carries the pedal-map frame on VH / CAN B. Standard 3/Y
          // Body+Chassis uses the CAN-A Body copy instead and must ignore any
          // numeric 0x334 that happens to exist on Chassis CAN.
          if (activeProfileIsYl() && activeProfilePedalMapSupported()) handlePedalMap334OnCanB(f);
          break;
        case LEFTSTALK_ID:
          if (activeProfileIsYl() && activeTurnSignalVariant == TURN_SIGNAL_STALK && f.data_length_code >= 3)
            handle249OnCanB(f.data, f.data_length_code);
          break;
        case VCLEFT_SWITCH_ID:
          if (activeProfileApRightScrollSupported() && f.data_length_code >= 8)
            handle3C2OnCanBRightScroll(f);
          break;
        case DOOR_SWITCH_ID:
          if (activeProfileIsYl() && f.data_length_code >= 4)
            handle102LaneChangeCancel(f.data, f.data_length_code);
          break;
        case VISUAL_DEBUG_ID:
          if (activeCanBIsChassis() && f.data_length_code >= 8) {
            visualBehaviorType = (uint8_t)readBitsLE(f.data, 56, 2);
            visualDebugRxCount++;
            visualDebugLastMs = frameNow;
            if (activeProfileAdvancedEapSupported()) evaluateAutoBlinker();
          }
          break;
        case 280:
          if (activeCanBIsChassis() && f.data_length_code >= 7) handle280(f.data);
          break;
        case 390:
          if (activeCanBIsChassis() && f.data_length_code >= 8) handle390(f.data);
          break;
        case 921:
          if (activeCanBIsChassis() && f.data_length_code >= 6) {
            handle921(f.data, f.data_length_code);
          }
          break;
        case 0x331:
          if (activeCanBIsChassis()) doInjectTlsscRestore(f);
          break;
        // Model YL/public-DBC reference: UI_driverAssistMapData road context.
        case 0x238:
          if (f.data_length_code >= 5) handleRoadContext238(f.data, f.data_length_code);
          break;

        // 1016 (SPR) is read on CAN B for both models.
        case DRIVER_ASSIST_ID:
          // Always retain stock telemetry before applying the production/research overlay.
          handle1016(f.data, f.data_length_code);
          injectDriverAssistControl(f);
          break;
        case 1021:
          if (f.data_length_code >= 8) {
            uint8_t mux = readMuxID(f.data);
            if (mux == 1) {
              r79LabObserve3fdMux1(f.data, f.data_length_code);
              injectUlcSnooze3fdMux1(f);
            } else if (mux == 0) injectTLSSC(f);
          }
          break;

        default:
          break;
        }
      }
      rxResult = rxBudget < TWAI_RX_DRAIN_BUDGET ? twai_receive(&f, 0) : ESP_ERR_TIMEOUT;
    }

    // Refresh Summon evidence before R79 retry/periodic servicing. The 5 s
    // V2.6 PARK fallback remains gate-compatible, but d1 queue priority uses
    // fresh real gear rather than gateParked.
    refreshSummonState();
    r79FixedTick();
    const uint32_t captureNow = (uint32_t)millis();
    researchCaptureTick(captureNow);
    driverMonitorCaptureTick(captureNow);
    const uint32_t queueNow = (uint32_t)millis();
    if ((uint32_t)(queueNow - lastQueueStatusMs) >= TWAI_QUEUE_TELEMETRY_PERIOD_MS) {
      lastQueueStatusMs = queueNow;
      twaiReadQueueStatus();
    }
    blinkATxTick();

    // Read alerts again after RX/tick work. BUS_OFF snapshot behavior is
    // preserved; d3 additionally consumes TX_IDLE/TX_SUCCESS for diagnostics.
    canTwaiHandleAlerts();

    // TWAI status / recovery. Recovery ends in STOPPED, so explicitly
    // restart the driver instead of leaving CAN B silent after BUS_OFF.
    unsigned long now = millis();
    if (now - lastTwaiStatusMs >= 1000) {
      lastTwaiStatusMs = now;
      twai_status_info_t st = {};
      if (twai_get_status_info(&st) == ESP_OK) {
        if (st.state == TWAI_STATE_RUNNING) {
          twaiReady = true;
        } else if (st.state == TWAI_STATE_BUS_OFF) {
          twaiReady = false;
          canTwaiBusOffCount++;
          canTwaiLastEventReason = CAN_REC_TWAI_BUS_OFF;
          canTwaiLastEventMs = (uint32_t)now;
          uint32_t frozenOrdinal = 0;
          portENTER_CRITICAL(&canBTxTraceMux);
          frozenOrdinal = canBTxTraceFrozenBusOffOrdinal;
          portEXIT_CRITICAL(&canBTxTraceMux);
          if (frozenOrdinal != canTwaiBusOffCount) {
            // Alert delivery is normally immediate. If it was missed, preserve
            // the old poll-based behavior as a fail-safe and freeze now.
            canBTraceFreezeBusOff((uint32_t)now);
            recordTwaiBusOffSnapshot(st, (uint32_t)now);
          }
          canBusOffPersistenceMarkDirty(CAN_BUS_OFF_BUS_B_PURE);
          T2CAN_SERIAL_PRINTLN("[CAN B] TWAI bus-off -> recovery started");
          invalidateCanTxStateForCanBRecovery();
          const esp_err_t recoveryErr = twai_initiate_recovery();
          if (recoveryErr == ESP_OK) {
            canTwaiLocalRecoveryStartCount++;
          } else {
            canTwaiRecoveryStartFailCount++;
            canTwaiLastEventReason = CAN_REC_TWAI_RECOVERY_FAIL;
            canTwaiLastEventMs = (uint32_t)now;
            T2CAN_SERIAL_PRINTF("[CAN B] TWAI recovery start failed: %s\n", esp_err_to_name(recoveryErr));
            requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_TWAI_RECOVERY_FAIL);
          }
        } else if (st.state == TWAI_STATE_STOPPED) {
          twaiReady = false;
          canTwaiStoppedCount++;
          if (canTwaiLastEventReason == CAN_REC_NONE) {
            canTwaiLastEventReason = CAN_REC_TWAI_STOPPED;
            canTwaiLastEventMs = (uint32_t)now;
          }
          invalidateCanTxStateForCanBRecovery();
          esp_err_t rs = twai_start();
          if (rs == ESP_OK) {
            canTwaiRestartOkCount++;
            twaiReady = true;
            canBTxObserveTxIdleAlert();
            T2CAN_SERIAL_PRINTLN("[CAN B] TWAI recovery complete -> restarted");
          } else {
            canTwaiRestartFailCount++;
            canTwaiLastEventReason = CAN_REC_TWAI_RESTART_FAIL;
            canTwaiLastEventMs = (uint32_t)now;
            T2CAN_SERIAL_PRINTF("[CAN B] TWAI restart failed: %s\n", esp_err_to_name(rs));
            requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_TWAI_RESTART_FAIL);
          }
        } else {
          twaiReady = false;
        }
      }
    }

    // No-CAN warning (shared counter)
    if ((millis() - bootTime) > 20000 && canRxTotal() == 0) {
      if (millis() - lastNoCanWarn > 5000) {
        T2CAN_SERIAL_PRINTLN("No CAN frames yet on either bus, staying alive.");
        lastNoCanWarn = millis();
      }
    }

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
  portENTER_CRITICAL(&canRecoveryMux);
  // Snapshot once for the restart request that is about to be queued. The
  // supervisor may loop again before reinitialization starts.
  if (canSupervisorCommand < CAN_SUP_HARD_STALE) {
    canTaskHeartbeatLastCause = cause;
    canTaskHeartbeatLastAgeAms = canTaskMcpHeartbeatMs
        ? (uint32_t)(now - canTaskMcpHeartbeatMs) : 0xFFFFFFFFUL;
    canTaskHeartbeatLastAgeBms = canTaskTwaiHeartbeatMs
        ? (uint32_t)(now - canTaskTwaiHeartbeatMs) : 0xFFFFFFFFUL;
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
  if (board == BOARD_TMR) return mcpInitChecked(); // Keep shared SPI and CS_B alive.
  mcpReady = false;
  if (mcpSpiStarted) {
    SPI.end();
    mcpSpiStarted = false;
    delay(20);
  }

  pinMode(MCP2515_CS, OUTPUT);
  digitalWrite(MCP2515_CS, HIGH);
  pinMode(MCP2515_RST, OUTPUT);
  digitalWrite(MCP2515_RST, HIGH);
  delay(1);
  digitalWrite(MCP2515_RST, LOW);
  delay(2);
  digitalWrite(MCP2515_RST, HIGH);
  delay(2);

  SPI.begin(MCP2515_SCLK, MCP2515_MISO, MCP2515_MOSI, MCP2515_CS);
  mcpSpiStarted = true;
  delay(20);

  return mcpInitChecked();
}

static bool recoveryTwaiInstallFresh() {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 256;
  g.tx_queue_len = TWAI_TX_QUEUE_LEN;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t ie = twai_driver_install(&g, &t, &f);
  if (ie != ESP_OK) {
    twaiReady = false;
    T2CAN_SERIAL_PRINTF("[CAN B] fresh install failed: %s\n", esp_err_to_name(ie));
    return false;
  }
  esp_err_t se = twai_start();
  if (se != ESP_OK) {
    twaiReady = false;
    T2CAN_SERIAL_PRINTF("[CAN B] fresh start failed: %s\n", esp_err_to_name(se));
    twai_driver_uninstall();
    return false;
  }
  uint32_t alerts = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS |
                    TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS |
                    TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF |
                    TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL;
  twai_reconfigure_alerts(alerts, NULL);
  twaiReady = true;
  canBTxObserveTxIdleAlert();
  return true;
}

static bool recoveryTwaiFullReinit() {
  twaiReady = false;
  twai_status_info_t st = {};
  esp_err_t gs = twai_get_status_info(&st);

  if (gs == ESP_OK) {
    if (st.state == TWAI_STATE_RUNNING) {
      esp_err_t e = twai_stop();
      if (e != ESP_OK) {
        T2CAN_SERIAL_PRINTF("[CAN B] hard stop failed: %s\n", esp_err_to_name(e));
        return false;
      }
    } else if (st.state == TWAI_STATE_BUS_OFF || st.state == TWAI_STATE_RECOVERING) {
      if (st.state == TWAI_STATE_BUS_OFF) {
        esp_err_t e = twai_initiate_recovery();
        if (e != ESP_OK) {
          T2CAN_SERIAL_PRINTF("[CAN B] hard recovery start failed: %s\n", esp_err_to_name(e));
          return false;
        }
      }
      uint32_t start = (uint32_t)millis();
      while ((uint32_t)((uint32_t)millis() - start) < RECOVERY_TWAI_WAIT_MS) {
        twai_status_info_t cur = {};
        if (twai_get_status_info(&cur) != ESP_OK) break;
        if (cur.state == TWAI_STATE_STOPPED) break;
        delay(25);
      }
      twai_status_info_t cur = {};
      if (twai_get_status_info(&cur) == ESP_OK && cur.state != TWAI_STATE_STOPPED) {
        T2CAN_SERIAL_PRINTLN("[CAN B] recovery did not reach STOPPED");
        return false;
      }
    }
  }

  esp_err_t ue = twai_driver_uninstall();
  if (ue != ESP_OK && ue != ESP_ERR_INVALID_STATE) {
    T2CAN_SERIAL_PRINTF("[CAN B] uninstall failed: %s\n", esp_err_to_name(ue));
    return false;
  }

  if (board == BOARD_T2CAN) {
    pinMode(CAN_TX, INPUT);
    pinMode(CAN_RX, INPUT);
  }
  delay(50);
  return recoveryTwaiInstallFresh();
}

static void recoveryStopCanTasks() {
  canTasksStopping = true;
  canTaskMcpQuiesced = false;
  canTaskTwaiQuiesced = false;

  uint32_t start = (uint32_t)millis();
  while ((!canTaskMcpQuiesced || !canTaskTwaiQuiesced) &&
         (uint32_t)((uint32_t)millis() - start) < 300) {
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  TaskHandle_t a = canTaskMcpHandle;
  TaskHandle_t b = canTaskTwaiHandle;
  canTaskMcpHandle = nullptr;
  canTaskTwaiHandle = nullptr;
  if (a) vTaskDelete(a);
  if (b) vTaskDelete(b);

  canTasksStopping = false;
  canTaskMcpQuiesced = false;
  canTaskTwaiQuiesced = false;
  canTaskMcpHeartbeatMs = 0;
  canTaskTwaiHeartbeatMs = 0;
  vTaskDelay(pdMS_TO_TICKS(RECOVERY_TASK_STOP_SETTLE_MS));
}

static bool recoveryStartCanTasks() {
  BaseType_t a = xTaskCreatePinnedToCore(canTaskMcp, "canA", 8192, nullptr, 5, &canTaskMcpHandle, 1);
  if (a != pdPASS) {
    canTaskMcpHandle = nullptr;
    return false;
  }
  BaseType_t b = xTaskCreatePinnedToCore(canTaskTwai, "canB", 8192, nullptr, board == BOARD_TMR ? 5 : 4, &canTaskTwaiHandle, 1);
  if (b != pdPASS) {
    vTaskDelete(canTaskMcpHandle);
    canTaskMcpHandle = nullptr;
    canTaskTwaiHandle = nullptr;
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
  T2CAN_SERIAL_PRINTF("[CAN SUP] hard CAN reinitialize #%lu reason=%u diag=%s\n",
                (unsigned long)canHardReinitCount, (unsigned)reason,
                canRecoveryDiagnosticReasonName(diagReason));

  recoveryStopCanTasks();
  invalidateCanTxStateForFullRecovery();
  bool aOk = recoveryMcpColdInit();
  bool bOk = recoveryTwaiFullReinit();
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
    T2CAN_SERIAL_PRINTF("[CAN SUP] hard CAN reinitialize FAILED A=%u B=%u tasks=%u\n",
                  aOk ? 1 : 0, bOk ? 1 : 0, tasksOk ? 1 : 0);
    return false;
  }
  bootCaptureHardFinish(bootCapHardIdx, true);
  T2CAN_SERIAL_PRINTLN("[CAN SUP] hard CAN reinitialize complete");
  return true;
}

static void canRecoverySupervisorTick(uint32_t now) {
  canBusOffPersistenceService(now);
}

static void canSupervisorTask(void* arg) {
  T2CAN_SERIAL_PRINTLN("[CAN SUP] recovery-only supervisor started");
  for (;;) {
    uint32_t now = (uint32_t)millis();
    canRecoverySupervisorTick(now);

    if (!canSubsystemBusy) {
      // Independent task heartbeat: still advances while the vehicle is asleep.
      // Therefore silence on the CAN wires is not confused with a wedged task.
      bool graceDone = (uint32_t)(now - canInitTime) >= RECOVERY_TASK_START_GRACE_MS;
      bool aTaskDead = canTaskMcpHandle && graceDone &&
                       (canTaskMcpHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskMcpHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      bool bTaskDead = canTaskTwaiHandle && graceDone &&
                       (canTaskTwaiHeartbeatMs == 0 ||
                        (uint32_t)(now - canTaskTwaiHeartbeatMs) > RECOVERY_TASK_HEARTBEAT_TIMEOUT_MS);
      if (aTaskDead || bTaskDead) {
        T2CAN_SERIAL_PRINTF("[CAN SUP] task heartbeat stale A=%u B=%u\n", aTaskDead ? 1 : 0, bTaskDead ? 1 : 0);
        recordCanTaskHeartbeatTimeout(now, aTaskDead, bTaskDead);
        requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_TASK_HEARTBEAT_TIMEOUT);
      }

      bool aFresh = recoveryFresh(now, lastCanAFrameMs, RECOVERY_BUS_FRESH_MS);
      bool bFresh = recoveryFresh(now, lastCanBFrameMs, RECOVERY_BUS_FRESH_MS);
      bool bothFresh = aFresh && bFresh;
      bool anyFresh = aFresh || bFresh;

      if (bothFresh) {
        if (!recoveryEverBothActive || recoverySleeping || recoveryWakeAcquireStartMs != 0) {
          T2CAN_SERIAL_PRINTLN("[CAN SUP] CAN A+B active");
        }
        recoveryEverBothActive = true;
        recoverySleeping = false;
        recoveryWakeAcquireStartMs = 0;
        recoveryOneBusStaleStartMs = 0;
        recoveryLastBothActiveMs = now;
        recoveryColdRetryCount = 0;
        recoveryColdRetriesExhausted = false;
      } else if (recoveryEverBothActive) {
        // Once a real active session has been observed, both buses going quiet
        // together is treated as normal Tesla sleep, never as a CAN failure.
        if (!anyFresh) {
          recoveryOneBusStaleStartMs = 0;
          if (!recoverySleeping && recoveryLastBothActiveMs != 0 &&
              (uint32_t)(now - recoveryLastBothActiveMs) >= RECOVERY_SLEEP_QUIET_MS) {
            recoverySleeping = true;
            recoveryWakeAcquireStartMs = 0;
            canRecoverySleepCount++;
            T2CAN_SERIAL_PRINTF("[CAN SUP] vehicle CAN sleep #%lu -> passive wait\n",
                          (unsigned long)canRecoverySleepCount);
          }
        } else {
          if (recoverySleeping) {
            recoverySleeping = false;
            recoveryWakeAcquireStartMs = now;
            canRecoveryWakeCount++;
            T2CAN_SERIAL_PRINTF("[CAN SUP] vehicle CAN wake #%lu -> acquire other bus\n",
                          (unsigned long)canRecoveryWakeCount);
          }

          if (recoveryWakeAcquireStartMs == 0) {
            if (recoveryOneBusStaleStartMs == 0) recoveryOneBusStaleStartMs = now;
            if ((uint32_t)(now - recoveryOneBusStaleStartMs) >= RECOVERY_ONE_BUS_STALE_MS &&
                (recoveryLastHardRequestMs == 0 ||
                 (uint32_t)(now - recoveryLastHardRequestMs) >= RECOVERY_HARD_COOLDOWN_MS)) {
              recoveryLastHardRequestMs = now;
              recoveryOneBusStaleStartMs = 0;
              requestCanSubsystemRestart(CAN_SUP_HARD_STALE, CAN_REC_ONE_BUS_STALE);
            }
          }
        }

        if (!recoverySleeping && recoveryWakeAcquireStartMs != 0 &&
            (uint32_t)(now - recoveryWakeAcquireStartMs) >= RECOVERY_WAKE_ACQUIRE_MS &&
            (recoveryLastHardRequestMs == 0 ||
             (uint32_t)(now - recoveryLastHardRequestMs) >= RECOVERY_HARD_COOLDOWN_MS)) {
          recoveryLastHardRequestMs = now;
          recoveryWakeAcquireStartMs = now;
          requestCanSubsystemRestart(CAN_SUP_HARD_ACQUIRE, CAN_REC_WAKE_ACQUIRE_TIMEOUT);
        }
      } else {
        // Cold boot / T-2CAN reset before a full A+B acquisition.
        // Retry hard initialization only a bounded number of times. If the
        // vehicle is simply asleep, stop tearing controllers down and wait for
        // a real CAN frame to wake the acquisition path.
        if (anyFresh && recoveryWakeAcquireStartMs == 0) recoveryWakeAcquireStartMs = now;

        uint32_t sinceInit = (uint32_t)(now - canInitTime);
        bool acquisitionTimedOut = (recoveryWakeAcquireStartMs != 0)
          ? ((uint32_t)(now - recoveryWakeAcquireStartMs) >= RECOVERY_COLD_FIRST_ACQUIRE_MS)
          : (sinceInit >= RECOVERY_COLD_FIRST_ACQUIRE_MS);

        uint32_t interval = (recoveryColdRetryCount == 0)
          ? RECOVERY_COLD_FIRST_ACQUIRE_MS : RECOVERY_COLD_RETRY_INTERVAL_MS;
        bool intervalPassed = (recoveryLastHardRequestMs == 0) ||
                              ((uint32_t)(now - recoveryLastHardRequestMs) >= interval);

        if (acquisitionTimedOut && intervalPassed &&
            recoveryColdRetryCount < RECOVERY_COLD_MAX_RETRIES) {
          recoveryColdRetryCount++;
          recoveryLastHardRequestMs = now;
          recoveryWakeAcquireStartMs = 0;
          T2CAN_SERIAL_PRINTF("[CAN SUP] cold acquire retry %u/%u\n",
                        (unsigned)recoveryColdRetryCount,
                        (unsigned)RECOVERY_COLD_MAX_RETRIES);
          requestCanSubsystemRestart(CAN_SUP_HARD_ACQUIRE, CAN_REC_COLD_ACQUIRE_TIMEOUT);
        } else if (recoveryColdRetryCount >= RECOVERY_COLD_MAX_RETRIES &&
                   !recoveryColdRetriesExhausted) {
          recoveryColdRetriesExhausted = true;
          recoverySleeping = true;
          T2CAN_SERIAL_PRINTLN("[CAN SUP] no RX after bounded retries -> passive sleep/wake wait");
        }

        // If a real frame arrives after passive wait, resume bounded acquisition.
        if (recoveryColdRetriesExhausted && anyFresh) {
          recoveryColdRetriesExhausted = false;
          recoverySleeping = false;
          recoveryColdRetryCount = 0;
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
        T2CAN_SERIAL_PRINTLN("[CAN SUP] subsystem recovery failed -> reboot T-2CAN");
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ═══════════════════════════════════════════════════════════════
// SETUP / LOOP
// ═══════════════════════════════════════════════════════════════
