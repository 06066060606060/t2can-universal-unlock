#pragma once

// WEB API / OTA / DASHBOARD SERVICE
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// WEB SERVER
// ═══════════════════════════════════════════════════════════════


static void httpPedalMapStats(){server.send(200,"application/json",pedalMapStatsJson());}

static void httpPedalMapSet() {
  if (!activeProfilePedalMapSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"PedalMap is unavailable for current vehicle/CAN topology\"}");
    return;
  }
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing mode\"}");
    return;
  }
  String mode = server.arg("mode");
  mode.toUpperCase();
  uint8_t target = PEDAL_MAP_RAW_STOCK;
  if (mode == "STOCK") target = PEDAL_MAP_RAW_STOCK;
  else if (mode == "CHILL" || mode == "COMFORT" || mode == "0") target = PEDAL_MAP_RAW_CHILL;
  else if (mode == "SPORT" || mode == "1") target = PEDAL_MAP_RAW_SPORT;
  else if (mode == "PERFORMANCE" || mode == "2") target = PEDAL_MAP_RAW_PERFORMANCE;
  else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid mode\"}");
    return;
  }

  // Deliberately runtime-only. No Preferences/NVS write is allowed here.
  if (!requestPedalMapMode(target, "SETTINGS PedalMap")) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"PedalMap request blocked by live vehicle state\"}");
    return;
  }
  server.send(200, "application/json", pedalMapStatsJson());
}

static String nagCfgToJson() {
  NagConfig c;
  const uint8_t variant = nagHumanVariantSnapshot();
  const NagHumanV4ConfigPure humanV4 = nagHumanV4RuntimeConfigSnapshot();
  portENTER_CRITICAL(&nagCfgMux);
  c = nagCfg;
  portEXIT_CRITICAL(&nagCfgMux);
  bool torqueRightScrollEnabled, tsl9RightPeriodicEnabled;
  uint16_t torqueRightScrollInterval, tsl9RightPeriodicInterval;
  uint8_t torqueRightScrollPattern;
  portENTER_CRITICAL(&nagRightScrollMux);
  torqueRightScrollEnabled = nagTorqueRightScrollEnabled;
  torqueRightScrollInterval = nagTorqueRightScrollIntervalSeconds;
  torqueRightScrollPattern =
      nagRightScrollPatternSanitize(nagTorqueRightScrollPattern);
  tsl9RightPeriodicEnabled = nagTsl9RightPeriodicEnabled;
  tsl9RightPeriodicInterval = nagTsl9RightPeriodicIntervalSeconds;
  portEXIT_CRITICAL(&nagRightScrollMux);
  String s;
  s.reserve(820);
  JsonWriterArduino jw(s);
  jw.boolean("enabled", c.enabled);
  jw.boolean("ignoreApState", c.ignoreApState);
  jw.u32("method", nagMethodSanitizePure(c.method));
  jw.string("methodName", nagMethodNamePure(c.method));
  jw.u32("tsl9Sequence", tsl9SequenceSanitizePure(c.tsl9Sequence));
  jw.string("tsl9SequenceName", tsl9SequenceNamePure(c.tsl9Sequence));
  jw.u32("tsl9Window", tsl9DowngradeWindowSanitizePure(
      c.tsl9DowngradeWindow));
  jw.string("tsl9WindowName", tsl9DowngradeWindowNamePure(
      c.tsl9DowngradeWindow));
  jw.u32("tsl9InputMode", tsl9InputModeSanitizePure(c.tsl9InputMode));
  jw.boolean("torqueRightScrollEnabled", torqueRightScrollEnabled);
  jw.u32("torqueRightScrollIntervalSeconds", torqueRightScrollInterval);
  jw.u32("torqueRightScrollPattern", torqueRightScrollPattern);
  jw.boolean("tsl9RightPeriodicEnabled", tsl9RightPeriodicEnabled);
  jw.u32("tsl9RightPeriodicIntervalSeconds", tsl9RightPeriodicInterval);
  jw.boolean("tsl9IsaChimeSuppress", c.tsl9IsaChimeSuppress);
  jw.u32("tsl9LegacyRoute", tsl9LegacyRouteSanitizePure(c.tsl9LegacyRoute));
  jw.boolean("tsl9LegacyRouteSelectable",
      vehicleProfileLegacyTsl9RouteSelectable(
          activeVehicleProfile, activeVehicleTopology));
  jw.string("tsl9EffectiveRoute", activeProfileIsYl()
      ? "PARTY_0x399" : nagTsl9Body39BSelected()
          ? "BODY_0x39B" : "CHASSIS_0x399");
  jw.boolean("dmsControlEnabled", c.dmsControlEnabled);
  jw.boolean("dmsControlSupported", activeProfileDmsNagSupported());
  jw.boolean("pauseAtZeroSpeed", c.pauseAtZeroSpeed);
  jw.u32("modeHStopBehavior", c.modeHStopBehavior);
  jw.string("modeHStopBehaviorName", nagModeHStopBehaviorNamePure(c.modeHStopBehavior));
  jw.u32("mode", c.mode);
  jw.u32("targetId", c.targetId);
  jw.u32("hoRatePct", c.hoRatePct);
  jw.u32("burstMs", c.burstMs);
  jw.u32("pauseMs", c.pauseMs);
  jw.u32("apStateId", c.apStateId);
  jw.u32("steeringId", c.steeringId);
  jw.u32("humanVariant", variant);
  jw.string("humanVariantCode", nagModeHVariantCodePure(variant));
  jw.string("humanVariantLabel", nagModeHVariantLabelPure(variant));
{
    jw.string("humanPreset", "HUMAN_INTERACTION_OPPOSITE_CARRIER");
    jw.fixed("humanPeakMinNm", (int32_t)humanV4.base.peakMinRaw, 2u);
    jw.fixed("humanPeakMaxNm", (int32_t)humanV4.base.peakMaxRaw, 2u);
    jw.u32("hoPolicy", (uint32_t)humanV4.hoPolicy);
    jw.string("hoPolicyName", nagHumanV3HoPolicyNamePure(humanV4.hoPolicy));
    jw.fixed("ho1ThresholdNm", (int32_t)humanV4.ho1ThresholdRaw, 2u);
    jw.fixed("ho2ThresholdNm", (int32_t)humanV4.ho2ThresholdRaw, 2u);
    jw.boolean("visualRescueEnabled", humanV4.visualRescueEnabled);
    jw.u32("visualRescueDelayMs", (uint32_t)humanV4.visualRescueDelayMs);
  }
  jw.beginArray("torque");
  for (uint8_t i = 0; i < c.torqueCount; i++) {
    if (i) s += ',';
    JsonWriterArduino tw(s);
    tw.u32("b2", c.torqueB2[i]);
    tw.u32("b3", c.torqueB3[i]);
    const uint16_t raw = ((c.torqueB2[i] & 0x0F) << 8) | c.torqueB3[i];
    const int16_t centiNm = (int16_t)raw - 2050;
    tw.fixed("nm", (int32_t)centiNm, 2u);
    tw.finish();
  }
  jw.endArray();
  jw.finish();
  return s;
}

static String nagStatsToJson() {
  NagContext c;
  portENTER_CRITICAL(&nagCtxMux); c = nagCtx; portEXIT_CRITICAL(&nagCtxMux);
  bool apValid, apActive;
  uint8_t apState4;
  uint32_t dasLast;
  portENTER_CRITICAL(&stateMux);
  apValid = dasAutopilotStateValid;
  apActive = gateAPActive;
  apState4 = dasAutopilotState4;
  dasLast = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  uint32_t txOk, txFail, skDisabled, skBoot, skWarmup, skSelf, skHo, skInvalid, skInactive, skDecision, skStopped, skSpeedStale, stopCarrierTxOk;
  uint32_t bMutex, bMcp, bEpoch, bFresh, bInvalid, sendErr, lastTx, maxGap, sessStart, sessTx;
  uint8_t lastSkip, lastBlock;
  uint32_t lastSkipMs, lastBlockMs;
  portENTER_CRITICAL(&nagDiagMux);
  txOk=nagTxOk; txFail=nagTxFail;
  skDisabled=nagSkipDisabled; skBoot=nagSkipBootDelay; skWarmup=nagSkipWarmup; skSelf=nagSkipSelfFrame;
  skHo=nagSkipHandsOn; skInvalid=nagSkipApInvalid; skInactive=nagSkipApInactive; skDecision=nagSkipDecision; skStopped=nagSkipStopped; skSpeedStale=nagSkipSpeedStale; stopCarrierTxOk=nagStopCarrierTxOk;
  bMutex=nagBlockMutex; bMcp=nagBlockMcpNotReady; bEpoch=nagBlockEpoch; bFresh=nagBlockFreshMask;
  bInvalid=nagBlockInvalidMsg; sendErr=nagSendError;
  lastTx=nagLastTxOkMs; maxGap=nagMaxTxGapMs; sessStart=nagSessionStartMs; sessTx=nagSessionTxOk;
  lastSkip=nagLastSkipReason; lastSkipMs=nagLastSkipMs; lastBlock=nagLastTxBlockReason; lastBlockMs=nagLastTxBlockMs;
  portEXIT_CRITICAL(&nagDiagMux);

  const uint32_t now = (uint32_t)millis();
  bool pauseAtZero = false;
  uint8_t nagModeNow = MODE_A;
  uint8_t nagMethodNow = NAG_METHOD_DEFAULT_PURE;
  uint8_t tsl9SequenceNow = TSL9_SEQUENCE_DEFAULT_PURE;
  uint8_t tsl9WindowNow = TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE;
  uint8_t modeHStopBehavior = nagModeHDefaultStopBehaviorPure();
  portENTER_CRITICAL(&nagCfgMux);
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  nagModeNow = nagCfg.mode;
  nagMethodNow = nagMethodSanitizePure(nagCfg.method);
  tsl9SequenceNow = tsl9SequenceSanitizePure(nagCfg.tsl9Sequence);
  tsl9WindowNow = tsl9DowngradeWindowSanitizePure(
      nagCfg.tsl9DowngradeWindow);
  modeHStopBehavior = nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior)
      ? nagCfg.modeHStopBehavior : nagModeHDefaultStopBehaviorPure();
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t speedAgeMs = c.lastVehicleSpeedMs == 0 ? 999999UL : (uint32_t)(now - c.lastVehicleSpeedMs);
  const bool speedFresh = c.vehicleSpeedValid && c.lastVehicleSpeedMs != 0 && speedAgeMs <= NAG_SPEED_FRESH_MS;
  const uint16_t speedKphX100 = c.vehicleSpeedValid ? nagPartySpeedKphX100Pure(c.vehicleSpeedRaw) : 0u;
  const bool humanMode = nagModeNow == MODE_H;
  const uint8_t humanVariant = nagHumanVariantSnapshot();
  const NagHumanV4StatePure humanV4 = humanMode ? nagHumanV4RuntimeSnapshot() : NagHumanV4StatePure{};
  const NagHumanV4ConfigPure humanV4Config = nagHumanV4RuntimeConfigSnapshot();
  const bool humanPaused = humanMode && humanV4.base.phase == H1_PAUSED_STOPPED;
  const bool stoppedGate = nagPauseAtZeroBlocksPure(pauseAtZero, c.vehicleSpeedValid, speedFresh, c.vehicleSpeedRaw) || humanPaused;
  uint32_t tsl9Rx, tsl9Modified, tsl9HandsOnModified, tsl9IsaModified;
  uint32_t tsl9TxOk, tsl9TxFail;
  portENTER_CRITICAL(&nagTsl9Mux);
  tsl9Rx = nagTsl9Rx;
  tsl9Modified = nagTsl9Modified;
  tsl9HandsOnModified = nagTsl9HandsOnModified;
  tsl9IsaModified = nagTsl9IsaModified;
  tsl9TxOk = nagTsl9TxOk;
  tsl9TxFail = nagTsl9TxFail;
  portEXIT_CRITICAL(&nagTsl9Mux);
  Tsl9InputSnapshotPure tsl9Input = {};
  bool tsl9InputTemplateValid = false;
  uint32_t tsl9InputTemplateMs = 0;
  uint32_t tsl9InputMux1Count, tsl9InputOk, tsl9InputFail, tsl9InputCleanup;
  portENTER_CRITICAL(&tsl9InputMux);
  tsl9Input = tsl9InputScheduler.snapshot(now);
  if (activeProfileTsl9InputOnBodyCanA()) {
    tsl9InputTemplateValid = tsl9InputCanATemplateValid;
    tsl9InputTemplateMs = tsl9InputCanATemplateMs;
  } else {
    tsl9InputTemplateValid = tsl9InputCanBTemplateValid;
    tsl9InputTemplateMs = tsl9InputCanBTemplateMs;
  }
  tsl9InputMux1Count = tsl9InputMux1Rx;
  tsl9InputOk = tsl9InputTxOk;
  tsl9InputFail = tsl9InputTxFail;
  tsl9InputCleanup = tsl9InputCleanupTx;
  portEXIT_CRITICAL(&tsl9InputMux);
  const uint16_t humanOutputRaw = humanV4.base.outputRaw != 0u ? humanV4.base.outputRaw : NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  const int16_t humanOutputCenti = (int16_t)humanOutputRaw - 2050;

  String s;
  s.reserve(1750);
  JsonWriterArduino jw(s);
  jw.u32("method", nagMethodNow);
  jw.string("methodName", nagMethodNamePure(nagMethodNow));
  jw.u32("tsl9Sequence", tsl9SequenceNow);
  jw.string("tsl9SequenceName", tsl9SequenceNamePure(tsl9SequenceNow));
  jw.u32("tsl9Window", tsl9WindowNow);
  jw.string("tsl9WindowName", tsl9DowngradeWindowNamePure(tsl9WindowNow));
  jw.u32("tsl9Rx", tsl9Rx);
  jw.u32("tsl9Modified", tsl9Modified);
  jw.u32("tsl9HandsOnModified", tsl9HandsOnModified);
  jw.u32("tsl9IsaModified", tsl9IsaModified);
  jw.u32("tsl9TxOk", tsl9TxOk);
  jw.u32("tsl9TxFail", tsl9TxFail);
  jw.boolean("tsl9InputActive", tsl9Input.active);
  jw.boolean("tsl9InputRetryPending", tsl9Input.retryPending);
  jw.boolean("tsl9InputCleanupPending", tsl9Input.cleanupPending);
  jw.u32("tsl9InputMode", tsl9Input.mode);
  jw.u32("tsl9InputStep", tsl9Input.step);
  jw.u32("tsl9InputIntervalMs", tsl9Input.currentIntervalMs);
  jw.u32("tsl9InputNextMs", tsl9Input.nextActionInMs);
  jw.u32("tsl9InputTriggers", tsl9Input.triggerCount);
  jw.u32("tsl9InputSuccess", tsl9Input.successCount);
  jw.u32("tsl9InputCanceled", tsl9Input.failureCancelCount);
  jw.u32("tsl9InputManualDeferrals", tsl9Input.manualDeferralCount);
  jw.u32("tsl9InputLastFailure", tsl9Input.lastFailure);
  jw.boolean("tsl9InputTemplateValid", tsl9InputTemplateValid);
  jw.u32("tsl9InputTemplateAgeMs", tsl9InputTemplateValid
      ? (uint32_t)(now - tsl9InputTemplateMs) : 999999UL);
  jw.u32("tsl9InputMux1Rx", tsl9InputMux1Count);
  jw.u32("tsl9InputTxOk", tsl9InputOk);
  jw.u32("tsl9InputTxFail", tsl9InputFail);
  jw.u32("tsl9InputCleanupTx", tsl9InputCleanup);
  jw.u32("rx", nagRxFrames);
  jw.u32("echo", nagEchoCount);
  jw.u32("txOk", txOk);
  jw.u32("txFail", txFail);
  jw.u32("mcpTxOkTotal", mcpTxOk);
  jw.u32("mcpTxFailTotal", mcpTxFail);
  jw.u32("latUs", nagEchoLatUs);
  jw.u32("ho", nagRealHo);
  jw.fixed("torque", (int32_t)nagRealTorqueCenti, 2u);
  jw.u32("injHo", nagLastInjectedHo);
  jw.fixed("injNm", (int32_t)nagLastInjectedCenti, 2u);
  jw.u32("uptimeS", (uint32_t)((millis() - bootTime) / 1000));
  jw.u32("apState", c.apState);
  jw.u32("dasState", (uint32_t)(apState4));
  jw.boolean("dasStateValid", apValid);
  jw.u32("handsOnState", c.handsOnState);
  jw.fixed("steeringDeg", (int32_t)c.steeringAngleDeciDeg, 1u);
  jw.fixed("vehicleSpeedKph", (int32_t)speedKphX100, 2u);
  jw.u32("vehicleSpeedRaw", (uint32_t)(c.vehicleSpeedRaw));
  jw.boolean("vehicleSpeedValid", c.vehicleSpeedValid);
  jw.boolean("vehicleSpeedFresh", speedFresh);
  jw.u32("vehicleSpeedAgeMs", speedAgeMs);
  jw.boolean("pauseAtZeroSpeed", pauseAtZero);
  jw.boolean("stoppedGate", stoppedGate);
  jw.boolean("humanMode", humanMode);
  jw.u32("humanVariant", (uint32_t)(humanVariant));
  jw.string("humanVariantCode", nagModeHVariantCodePure(humanVariant));
  jw.string("humanVariantLabel", nagModeHVariantLabelPure(humanVariant));
{
    jw.string("humanPreset", "HUMAN_INTERACTION_OPPOSITE_CARRIER");
    jw.fixed("humanPeakMinNm", (int32_t)humanV4Config.base.peakMinRaw, 2u);
    jw.fixed("humanPeakMaxNm", (int32_t)humanV4Config.base.peakMaxRaw, 2u);
    jw.boolean("visualRescueEnabled", humanV4Config.visualRescueEnabled);
    jw.u32("visualRescueDelayMs", (uint32_t)humanV4Config.visualRescueDelayMs);
    jw.boolean("visualRescuePending", humanV4.visualRescuePending);
    jw.u32("visualRescueCount", humanV4.visualRescueCount);
    jw.string("humanPhase", nagHumanV1PhaseNamePure(humanV4.base.phase));
    jw.string("humanMotion", nagHumanV1MotionNamePure(humanV4.base.motion));
    jw.string("humanEventType", nagHumanV1EventTypeNamePure(humanV4.base.event.type));
    jw.u32("humanEventCount", (uint32_t)(humanV4.base.eventCount));
    jw.i32("humanDirection", (int32_t)(humanV4.base.event.direction));
    jw.boolean("humanCarrier", humanV4.base.carrier);
    jw.fixed("humanCarrierSampleNm", (int32_t)humanV4.lastCarrierMagnitudeRaw, 2u);
    jw.boolean("humanCarrierApplied", humanV4.lastCarrierApplied);
    jw.u32("humanSessionAgeMs", (uint32_t)((humanV4.base.sessionStartMs ? now-humanV4.base.sessionStartMs : 0UL)));
  }
  jw.fixed("humanOutputNm", (int32_t)humanOutputCenti, 2u);
  jw.boolean("humanPaused", humanPaused);
  jw.u32("modeHStopBehavior", modeHStopBehavior);
  jw.string("modeHStopBehaviorName", nagModeHStopBehaviorNamePure(modeHStopBehavior));
  jw.boolean("stopCarrierActive", humanPaused && modeHStopBehavior == H_STOP_STOCK_CARRIER);
  jw.u32("stopCarrierTxOk", stopCarrierTxOk);
  jw.u32("apStaleMs", (uint32_t)((c.lastApStateMs == 0) ? 999999UL : (now - c.lastApStateMs)));
  jw.u32("dasAgeMs", (uint32_t)((dasLast == 0) ? 999999UL : (uint32_t)(now-dasLast)));
  jw.u32("stStaleMs", (uint32_t)((c.lastSteeringMs == 0) ? 999999UL : (now - c.lastSteeringMs)));
  jw.i32("canAState", (int32_t)(mcpState));
  jw.boolean("apActive", apValid && apActive);
  jw.u32("lastTxAgeMs", (uint32_t)(lastTx ? (uint32_t)(now-lastTx) : 999999UL));
  jw.u32("maxTxGapMs", maxGap);
  jw.u32("sessionAgeMs", (uint32_t)(sessStart ? (uint32_t)(now-sessStart) : 0UL));
  jw.u32("sessionTxOk", sessTx);
  jw.u32("skipDisabled", skDisabled);
  jw.u32("skipBootDelay", skBoot);
  jw.u32("skipWarmup", skWarmup);
  jw.u32("skipSelfFrame", skSelf);
  jw.u32("skipHandsOn", skHo);
  jw.u32("skipApInvalid", skInvalid);
  jw.u32("skipApInactive", skInactive);
  jw.u32("skipDecision", skDecision);
  jw.u32("skipStopped", skStopped);
  jw.u32("skipSpeedStale", skSpeedStale);
  jw.u32("blockMutex", bMutex);
  jw.u32("blockMcpNotReady", bMcp);
  jw.u32("blockEpoch", bEpoch);
  jw.u32("blockFreshMask", bFresh);
  jw.u32("blockInvalidMsg", bInvalid);
  jw.u32("sendError", sendErr);
  jw.string("lastSkip", nagSkipReasonName(lastSkip));
  jw.u32("lastSkipAgeMs", (uint32_t)(lastSkipMs ? (uint32_t)(now-lastSkipMs) : 999999UL));
  jw.string("lastTxBlock", nagTxBlockReasonName(lastBlock));
  jw.u32("lastTxBlockAgeMs", (uint32_t)(lastBlockMs ? (uint32_t)(now-lastBlockMs) : 999999UL));
  jw.finish();
  return s;
}

static String summonStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  bool tlssc, tlsscHighwayGate, tlsscNoaBlock, tlsscOwned, tlsscClear, ap, parked, summon, aca, spr;
  bool acaStateValid, sprStateValid, manualLatchActive;
  uint32_t tlsscClearOk, tlsscClearFail;
  uint8_t gearSource, sprRaw, confirmedGearState, confirmedGearSource;
  uint32_t acaObservedMs, sprObservedMs, gearObservedMs;
  uint32_t confirmedGearObservedMs, confirmedGearTransitions;
  uint32_t tok, tfail, r280, r390, r921, r1016;

  portENTER_CRITICAL(&stateMux);
  tlssc = tlsscEnabled;
  tlsscHighwayGate = tlsscHighwayGateEnabled;
  tlsscNoaBlock = tlsscBlockInNoa;
  tlsscOwned = tlsscInjectedActive;
  tlsscClear = tlsscClearPending;
  tlsscClearOk = tlsscClearTxOk;
  tlsscClearFail = tlsscClearTxFail;
  ap = gateAPActive;
  parked = gateParked;
  summon = gateSummoning;
  aca = lastAca;
  spr = sprSeen;
  acaStateValid = acaValid;
  sprStateValid = sprValid;
  acaObservedMs = lastAcaMillis;
  sprObservedMs = lastSprMillis;
  sprRaw = lastSprRaw;
  gearSource = summonGearSource;
  gearObservedMs = summonGearObservedMs;
  confirmedGearState = summonConfirmedGearLatch.state;
  confirmedGearSource = summonConfirmedGearLatch.source;
  confirmedGearObservedMs = summonConfirmedGearLatch.observedMs;
  confirmedGearTransitions = summonConfirmedGearTransitions;
  manualLatchActive = r79ManualSuppression.active;
  tok = sumTxOk;
  tfail = sumTxFail;
  r280 = sumRx280;
  r390 = sumRx390;
  r921 = sumRx921;
  r1016 = sumRx1016;
  portEXIT_CRITICAL(&stateMux);

  const SummonRoutePure route = activeSummonRoute();
  const uint32_t gearAge = gearObservedMs ? (uint32_t)(now - gearObservedMs) : 999999UL;
  const uint32_t confirmedGearAge = confirmedGearObservedMs
      ? (uint32_t)(now - confirmedGearObservedMs) : 999999UL;
  const uint32_t acaAge = acaObservedMs ? (uint32_t)(now - acaObservedMs) : 999999UL;
  const uint32_t sprAge = sprObservedMs ? (uint32_t)(now - sprObservedMs) : 999999UL;
  const bool acaFresh = acaStateValid && summonAgeFreshPure(now, acaObservedMs, SUMMON_ACA_FRESH_MS);
  const bool sprFresh = sprStateValid && summonAgeFreshPure(now, sprObservedMs, SUMMON_SPR_FRESH_MS);
  const bool remoteStartupEvidence = (acaFresh && aca) || (sprFresh && sprRaw != 0);
  const uint8_t priorityState = summonPriorityStateSnapshot(now);
  bool r79RetryPendingLocal;
  uint8_t r79RetryIndexLocal;
  uint32_t r79RetryScheduledLocal, r79RetryOkLocal, r79RetryFailLocal, r79RetryExhaustedLocal;
  uint32_t r79LastRetryMsLocal, r79FlushCountLocal, r79FlushOkLocal, r79FlushFailLocal;
  portENTER_CRITICAL(&r79LabMux);
  r79RetryPendingLocal = r79RetryPending;
  r79RetryIndexLocal = r79RetryIndex;
  r79RetryScheduledLocal = r79RetryScheduled;
  r79RetryOkLocal = r79RetryTxOk;
  r79RetryFailLocal = r79RetryTxFail;
  r79RetryExhaustedLocal = r79RetryExhausted;
  r79LastRetryMsLocal = r79LastRetryMs;
  r79FlushCountLocal = r79EmergencyQueueFlushCount;
  r79FlushOkLocal = r79FlushTriggeredRetryOk;
  r79FlushFailLocal = r79FlushTriggeredRetryFail;
  portEXIT_CRITICAL(&r79LabMux);
  const char *gearBusName = route.gearBusMask == SUMMON_BUS_A
      ? activeProfileCanAName()
      : (route.gearBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
  const char *dasBusName = route.dasBusMask == SUMMON_BUS_A
      ? activeProfileCanAName()
      : (route.dasBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
  const char *sprBusName = route.sprBusMask == SUMMON_BUS_A
      ? activeProfileCanAName()
      : (route.sprBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
  const char *transportBusName = route.transportBusMask == SUMMON_BUS_A
      ? activeProfileCanAName()
      : (route.transportBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");

  bool roadValid, gpsRoadMatch, navRouteActive, controlledAccess, leftOffRamp, rightOffRamp, highwayConfirmed;
  uint8_t roadClass, highwayPositiveCount, highwayNegativeCount;
  uint32_t roadLast, highwayBlockedCount, highwayTransitions;
  portENTER_CRITICAL(&roadContextMux);
  roadValid = roadContextValid;
  roadClass = roadContextRoadClass;
  gpsRoadMatch = roadContextGpsRoadMatch;
  navRouteActive = roadContextNavRouteActive;
  controlledAccess = roadContextControlledAccess;
  leftOffRamp = roadContextLeftOffRamp;
  rightOffRamp = roadContextRightOffRamp;
  roadLast = roadContextLastRxMs;
  highwayConfirmed = tlsscHighwayHysteresis.confirmed;
  highwayPositiveCount = tlsscHighwayHysteresis.positiveCount;
  highwayNegativeCount = tlsscHighwayHysteresis.negativeCount;
  highwayBlockedCount = tlsscHighwayGateBlockedCount;
  highwayTransitions = tlsscHighwayTransitions;
  portEXIT_CRITICAL(&roadContextMux);
  const uint32_t roadAge = roadLast == 0 ? 999999UL : (uint32_t)(now - roadLast);
  const bool roadFresh = roadValid && roadLast != 0 && roadAge <= ROAD_CONTEXT_FRESH_MS;
  const bool highwayBlocked = tlsscHighwayGateBlocked(now);

  McpChassisStatus st = {};
  const bool chassisStatusOk = (mcpChassisGetStatus(&st) == ESP_OK);

  String out;
  out.reserve(2600);
  JsonWriterArduino jw(out);
  jw.raw("monitoring", "true");
  jw.boolean("tlssc", tlssc);
  jw.boolean("tlsscHighwayGateEnabled", tlsscHighwayGate);
  jw.boolean("tlsscBlockInNoa", tlsscNoaBlock);
  jw.boolean("tlsscInjectedActive", tlsscOwned);
  jw.boolean("tlsscClearPending", tlsscClear);
  jw.u32("tlsscClearTxOk", (uint32_t)(tlsscClearOk));
  jw.u32("tlsscClearTxFail", (uint32_t)(tlsscClearFail));
  jw.boolean("roadContextValid", roadValid);
  jw.boolean("roadContextFresh", roadFresh);
  jw.u32("roadContextAgeMs", (uint32_t)(roadAge));
  jw.i32("roadClass", (int32_t)(roadClass));
  jw.boolean("gpsRoadMatch", gpsRoadMatch);
  jw.boolean("navRouteActive", navRouteActive);
  jw.boolean("controlledAccess", controlledAccess);
  jw.boolean("leftOffRamp", leftOffRamp);
  jw.boolean("rightOffRamp", rightOffRamp);
  jw.boolean("tlsscHighwayConfirmed", highwayConfirmed);
  jw.boolean("tlsscHighwayBlocked", highwayBlocked);
  jw.i32("tlsscHighwayPositiveCount", (int32_t)(highwayPositiveCount));
  jw.i32("tlsscHighwayNegativeCount", (int32_t)(highwayNegativeCount));
  jw.u32("tlsscHighwayBlockedCount", (uint32_t)(highwayBlockedCount));
  jw.u32("tlsscHighwayTransitions", (uint32_t)(highwayTransitions));
  jw.boolean("summonRouteValid", route.valid);
  jw.string("gearBusName", gearBusName);
  jw.string("dasBusName", dasBusName);
  jw.string("sprBusName", sprBusName);
  jw.string("transportBusName", transportBusName);
  jw.boolean("allow186Fallback", route.allow186Fallback);
  jw.u32("gearSource", (uint32_t)(gearSource));
  jw.string("gearSourceName", summonGearSourceNamePure(gearSource));
  jw.u32("gearAgeMs", (uint32_t)(gearAge));
  jw.u32("confirmedGearState", (uint32_t)(confirmedGearState));
  jw.string("confirmedGearStateName", summonConfirmedGearNamePure(confirmedGearState));
  jw.string("confirmedGearSourceName", summonGearSourceNamePure(confirmedGearSource));
  jw.u32("confirmedGearAgeMs", (uint32_t)(confirmedGearAge));
  jw.u32("confirmedGearTransitions", (uint32_t)(confirmedGearTransitions));
  jw.boolean("acaValid", acaStateValid);
  jw.boolean("acaFresh", acaFresh);
  jw.u32("acaAgeMs", (uint32_t)(acaAge));
  jw.boolean("sprValid", sprStateValid);
  jw.u32("sprRaw", (uint32_t)(sprRaw));
  jw.boolean("sprFresh", sprFresh);
  jw.u32("sprAgeMs", (uint32_t)(sprAge));
  jw.boolean("ap", ap);
  jw.boolean("parked", parked);
  jw.boolean("summon", summon);
  jw.boolean("aca", aca);
  jw.boolean("spr", spr);
  jw.boolean("remoteStartupEvidence", remoteStartupEvidence);
  jw.boolean("manualLatchActive", manualLatchActive);
  jw.u32("priorityState", (uint32_t)(priorityState));
  jw.string("priorityStateName", summonPriorityStateName(priorityState));
  jw.boolean("loadSheddingActive", priorityState == SUMMON_PRIORITY_READY || priorityState == SUMMON_PRIORITY_ACTIVE);
  jw.u32("txQueueNow", (uint32_t)(chassisTxQueueNow));
  jw.u32("txQueueMax", (uint32_t)(chassisTxQueueMax));
  jw.u32("nonSummonShed", (uint32_t)(chassisNonSummonShed));
  jw.u32("parkSoftShed", (uint32_t)(chassisParkSoftShed));
  jw.u32("readyShed", (uint32_t)(chassisReadyShed));
  jw.u32("activeShed", (uint32_t)(chassisActiveShed));
  jw.boolean("r79RetryPending", r79RetryPendingLocal);
  jw.u32("r79RetryIndex", (uint32_t)(r79RetryPendingLocal ? r79RetryIndexLocal + 1U : 0U));
  jw.u32("r79RetryScheduled", (uint32_t)(r79RetryScheduledLocal));
  jw.u32("r79RetryTxOk", (uint32_t)(r79RetryOkLocal));
  jw.u32("r79RetryTxFail", (uint32_t)(r79RetryFailLocal));
  jw.u32("r79RetryExhausted", (uint32_t)(r79RetryExhaustedLocal));
  jw.u32("r79LastRetryAgeMs", (uint32_t)(r79LastRetryMsLocal ? now-r79LastRetryMsLocal : 999999UL));
  jw.u32("r79EmergencyQueueFlushCount", (uint32_t)(r79FlushCountLocal));
  jw.u32("r79FlushTriggeredRetryOk", (uint32_t)(r79FlushOkLocal));
  jw.u32("r79FlushTriggeredRetryFail", (uint32_t)(r79FlushFailLocal));
  jw.u32("txOk", tok);
  jw.u32("txFail", tfail);
  jw.u32("rx280", r280);
  jw.u32("rx390", r390);
  jw.u32("rx921", r921);
  jw.u32("rx1016", r1016);
  jw.i32("canState", chassisStatusOk ? (int32_t)st.state : -1);
  jw.string("canStateName", chassisStatusOk ? mcpChassisStateName(st.state) : "UNAVAILABLE");
  jw.u32("uptimeS", (uint32_t)((millis() - bootTime) / 1000));
  jw.finish();
  return out;
}

static const char *autoBlinkerNoaSessionStateName(
    AutoBlinkerNoaPhasePure phase) {
  switch (phase) {
    case AUTO_BLINKER_NOA_STABILIZING_PURE: return "STABILIZATION";
    case AUTO_BLINKER_NOA_READY_PURE: return "READY";
    case AUTO_BLINKER_NOA_EXIT_WAIT_PURE: return "EXIT_WAIT";
    default: return "INACTIVE";
  }
}

static const char *blinkerTxModeName(uint8_t mode) {
  return mode == BLINKER_TX_MODE_LEGACY_PURE
      ? "350ms BURST" : "SINGLE TX";
}

static const char *blinkerTxSourceName(uint8_t source) {
  if (source == BLINKER_TX_SOURCE_AUTO_PURE) return "AUTO_BLINKER";
  if (source == BLINKER_TX_SOURCE_S3XY_PURE) return "S3XY_BUTTON";
  return "NONE";
}

static const char *blinkerTxResultName(uint8_t result) {
  switch (result) {
    case 1: return "PENDING";
    case 2: return "TX_OK";
    case 3: return "TX_FAIL";
    case 4: return "BLOCKED";
    default: return "NONE";
  }
}

static String blinkAStatsToJson() {
  bool en, ap, noaRaw, noaEffective, dasStateValid, alcValid;
  uint8_t dasState4, alcState, noaStabilizationSeconds, cancelPauseSeconds;
  uint8_t curTurn, pending, behavior;
  uint32_t delayMs, remain, retryIn, requestAge, retryCount, txOk, txFail, r249, visualLast;
  uint32_t noaStabilizationRemainingMs, noaExitRemainingMs;
  uint32_t cancelPauseRemainingMs;
  bool armed, seen, selfTest, noaStabilized, cancelPaused;
  AutoBlinkerNoaPhasePure noaSessionState;
  uint8_t txMode, txActiveSource, txLastDirection, txLastSource, txLastResult;
  uint32_t txRequests, txBlocked;
  uint8_t rCnt, rTurn, rCk, rDlc;
  uint8_t raw249[8] = {0};
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  curTurn = activeTurn;
  pending = autoPendingDir;
  delayMs = blinkADelayMs;
  armed = autoArmed;
  uint32_t now = millis();
  noaStabilizationSeconds = blinkANoaStabilizationSeconds;
  cancelPauseSeconds = blinkACancelPauseSeconds;
  noaSessionState = autoBlinkerNoaPhasePure(
      autoBlinkerNoaSessionState, now);
  noaStabilized = noaSessionState == AUTO_BLINKER_NOA_READY_PURE;
  noaStabilizationRemainingMs =
      autoBlinkerNoaRemainingMsPure(autoBlinkerNoaSessionState, now);
  noaExitRemainingMs =
      autoBlinkerNoaExitRemainingMsPure(autoBlinkerNoaSessionState, now);
  cancelPaused = autoBlinkerPauseActivePure(
      autoBlinkerCancelPauseState, now);
  cancelPauseRemainingMs = autoBlinkerPauseRemainingMsPure(
      autoBlinkerCancelPauseState, now);
  remain = (autoArmed && (int32_t)(autoFireAt - now) > 0) ? (autoFireAt - now) : 0;
  retryIn = (autoArmed && (int32_t)(autoRetryAt - now) > 0) ? (autoRetryAt - now) : 0;
  requestAge = autoRequestLastSeenMs ? (uint32_t)(now - autoRequestLastSeenMs) : 999999UL;
  retryCount = autoRetryCount;
  txOk = blkATxOk;
  txFail = blkATxFail;
  txMode = blinkerTxMode;
  txActiveSource = oneShotSource != BLINKER_TX_SOURCE_NONE_PURE
      ? oneShotSource : blinkerTxRequestState.pendingSource;
  txLastDirection = blinkerTxLastDir;
  txLastSource = blinkerTxLastSource;
  txLastResult = blinkerTxLastResult;
  txRequests = blinkerTxRequestCount;
  txBlocked = blinkerTxBlockedCount;
  r249 = rx249;
  rCnt = realCounter;
  rTurn = realTurn;
  rCk = realCksum;
  seen = seen249;
  selfTest = cksumSelfTest;
  rDlc = realDlc;
  behavior = visualBehaviorType;
  visualLast = visualDebugLastMs;
  memcpy(raw249, realRaw249, sizeof(raw249));
  portEXIT_CRITICAL(&blinkAMux);
  uint32_t noaLastMs;
  portENTER_CRITICAL(&stateMux);
  ap = gateAPActive;
  noaRaw = gateNOAActive;
  dasStateValid = dasAutopilotStateValid;
  dasState4 = dasAutopilotState4;
  alcState = dasAutoLaneChangeState;
  alcValid = dasAutoLaneChangeStateValid;
  noaLastMs = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  const uint32_t noaNow = (uint32_t)millis();
  const uint32_t noaAgeMs = (noaLastMs == 0) ? UINT32_MAX : (uint32_t)(noaNow - noaLastMs);
  const uint32_t visualAgeMs = (visualLast == 0) ? UINT32_MAX : (uint32_t)(noaNow - visualLast);
  const bool visualFresh = (visualLast != 0 && visualAgeMs <= BLINKA_REQUEST_FRESH_MS);
  uint8_t rawReqDir = 0;
  if (behavior == 2) rawReqDir = 1;
  else if (behavior == 3) rawReqDir = 2;
  const bool alcDirectionAllowed = rawReqDir != 0 &&
      autoBlinkerALCAllowsDirection(rawReqDir, noaNow);
  const bool pendingAlcAllowed = pending != 0 &&
      autoBlinkerALCAllowsDirection(pending, noaNow);
  const bool waitingEligibility = armed && remain == 0 && !pendingAlcAllowed;
  noaEffective = dasStateValid && noaRaw;

  String s;
  s.reserve(2300);
  JsonWriterArduino jw(s);
  jw.boolean("enabled", en);
  jw.boolean("apActive", ap);
  jw.boolean("noaActive", noaEffective);
  jw.boolean("noaRawActive", noaRaw);
  jw.u32("noaStabilizationSeconds", noaStabilizationSeconds);
  jw.boolean("noaStabilized", noaStabilized);
  jw.u32("noaStabilizationRemainingMs", noaStabilizationRemainingMs);
  jw.u32("noaSessionState", (uint32_t)noaSessionState);
  jw.string("noaSessionStateName",
            autoBlinkerNoaSessionStateName(noaSessionState));
  jw.u32("noaExitRemainingMs", noaExitRemainingMs);
  jw.u32("cancelPauseSeconds", cancelPauseSeconds);
  jw.boolean("cancelPaused", cancelPaused);
  jw.u32("cancelPauseRemainingMs", cancelPauseRemainingMs);
  jw.boolean("dasStateValid", dasStateValid);
  // Backward-compatible key for cached dashboards: mirrors validity, not age.
  jw.boolean("noaFresh", dasStateValid);
  jw.u32("noaAgeMs", (noaAgeMs == UINT32_MAX) ? 999999UL : noaAgeMs);
  jw.u32("noaFreshLimitMs", 0);
  jw.u32("dasState", dasState4);
  jw.u32("alcState", alcState);
  jw.boolean("alcValid", alcValid);
  jw.u32("requestFreshMs", BLINKA_REQUEST_FRESH_MS);
  jw.boolean("visualFresh", visualFresh);
  jw.u32("visualAgeMs", (visualAgeMs == UINT32_MAX) ? 999999UL : visualAgeMs);
  jw.boolean("alcDirectionAllowed", alcDirectionAllowed);
  jw.u32("behaviorType", behavior);
  jw.u32("activeTurn", curTurn);
  jw.u32("delayMs", delayMs);
  jw.boolean("autoArmed", armed);
  jw.u32("autoPending", pending);
  jw.u32("autoRemainMs", remain);
  jw.u32("autoRetryInMs", retryIn);
  jw.u32("autoRetryCount", retryCount);
  jw.u32("autoRequestAgeMs", requestAge);
  jw.boolean("autoWaitingEligibility", waitingEligibility);
  jw.boolean("pendingAlcAllowed", pendingAlcAllowed);
  jw.u32("txMode", txMode);
  jw.string("txModeName", blinkerTxModeName(txMode));
  jw.string("txActiveSource", blinkerTxSourceName(txActiveSource));
  jw.u32("txLastDirection", txLastDirection);
  jw.string("txLastSource", blinkerTxSourceName(txLastSource));
  jw.string("txLastResult", blinkerTxResultName(txLastResult));
  jw.u32("txRequests", txRequests);
  jw.u32("txOk", txOk);
  jw.u32("txFail", txFail);
  jw.u32("txBlocked", txBlocked);
  jw.u32("rx249", r249);
  jw.boolean("seen249", seen);
  jw.u32("realCounter", rCnt);
  jw.u32("realTurn", rTurn);
  jw.u32("realCksum", rCk);
  jw.u32("realDlc", rDlc);
  jw.u32("visualDebugRx", visualDebugRxCount);
  String rawHex;
  rawHex.reserve(24);
  for (uint8_t i = 0; i < rDlc; i++) {
    if (i) rawHex += " ";
    if (raw249[i] < 0x10) rawHex += "0";
    rawHex += String(raw249[i], HEX);
  }
  rawHex.toUpperCase();
  jw.string("realRaw", rawHex);
  jw.boolean("cksumSelfTest", selfTest);
  jw.i32("canBState", (int32_t)mcpChassisReady);
  jw.u32("uptimeS", (millis() - bootTime) / 1000);
  jw.finish();
  return s;
}

static String dasTelemetryStatsToJson() {
  uint32_t now = millis();
  String s;
  s.reserve(256);
  JsonWriterArduino jw(s);
  jw.u32("behaviorType", visualBehaviorType);
  jw.u32("ulcBlindSpotConfig", uiUlcBlindSpotConfig);
  jw.u32("visualDebugRx", visualDebugRxCount);
  jw.u32("visualDebugStaleMs", visualDebugLastMs == 0 ? 999999UL : (now - visualDebugLastMs));
  jw.finish();
  return s;
}


static String lab3f8RawHex(const uint8_t *data, uint8_t dlc) {
  String out;
  if (!data || dlc == 0) return out;
  const uint8_t n = dlc > 8 ? 8 : dlc;
  out.reserve((size_t)n * 3u);
  for (uint8_t i = 0; i < n; i++) {
    if (i) out += " ";
    if (data[i] < 0x10) out += "0";
    out += String(data[i], HEX);
  }
  out.toUpperCase();
  return out;
}

static String ulcStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t alcMode, blindMode, ulcOffHighwayMode;
  uint8_t lastTxBlind, lastTxStockBlind, lastTxSelectedBlind, lastTxResult;
  bool lastTxValid, lastTxBlindChanged, lastTxBit56;
  uint32_t ulcOffTxOk, ulcOffTxFail, ulcOffBlocked, ulcOffLastTxMs;
  bool ulcOffLastTxValid;
  uint8_t ulcOffLastTxRaw;
  bool noConfirmEnabled, stockStalkConfirm, stockStalkConfirmValid, noConfirmLastTxValid, noConfirmLastTxBit1;
  uint8_t noConfirmTimingMode;
  uint32_t lastTxMs, txOk, txFail, blocked, driverAssistLast;
  uint32_t noConfirmTxOk, noConfirmTxFail, noConfirmTxBOk, noConfirmTxBFail;
  uint32_t noConfirmBlocked, noConfirmBlockedB, noConfirmLastTxMs;
  bool driverAssistBValid;
  uint32_t driverAssistRxB, driverAssistBMs, driverAssistBPeriod;
  uint8_t driverAssistBDlc, driverAssistBData[8];
  bool autoLcEnabled, autoLcAValid, autoLcBValid, autoLcLastValid;
  uint8_t autoLcTargetBus, autoLcARaw, autoLcBRaw, autoLcLastBus, autoLcLastRaw;
  uint32_t autoLcAMs, autoLcBMs, autoLcRxA, autoLcRxB, autoLcTxAOk, autoLcTxAFail, autoLcTxBOk, autoLcTxBFail, autoLcBlockedA, autoLcBlockedB, autoLcLastMs;
  portENTER_CRITICAL(&lab3f8Mux);
  alcMode = lab3f8AlcMode;
  blindMode = lab3f8UlcBlindMode;
  ulcOffHighwayMode = lab3f8UlcOffHighwayMode;
  txOk = lab3f8TxOk;
  txFail = lab3f8TxFail;
  blocked = lab3f8GateBlocked;
  lastTxValid = lab3f8LastTxValid;
  lastTxBlind = lab3f8LastTxBlind;
  lastTxStockBlind = lab3f8LastTxStockBlind;
  lastTxSelectedBlind = lab3f8LastTxSelectedBlind;
  lastTxBlindChanged = lab3f8LastTxBlindChanged;
  lastTxResult = lab3f8LastTxResult;
  lastTxMs = lab3f8LastTxMs;
  lastTxBit56 = lab3f8LastTxBit56;
  ulcOffTxOk = ulcOffHighwayTxOk;
  ulcOffTxFail = ulcOffHighwayTxFail;
  ulcOffBlocked = ulcOffHighwayGateBlocked;
  ulcOffLastTxValid = ulcOffHighwayLastTxValid;
  ulcOffLastTxRaw = ulcOffHighwayLastTxRaw;
  ulcOffLastTxMs = ulcOffHighwayLastTxMs;
  noConfirmEnabled = ulcNoConfirmEnabled;
  stockStalkConfirm = uiUlcStalkConfirm;
  stockStalkConfirmValid = false;
  noConfirmTimingMode = ulcNoConfirmTimingMode;
  noConfirmTxOk = ulcNoConfirmTxOk;
  noConfirmTxFail = ulcNoConfirmTxFail;
  noConfirmTxBOk = ulcNoConfirmTxBOk;
  noConfirmTxBFail = ulcNoConfirmTxBFail;
  noConfirmBlocked = ulcNoConfirmGateBlocked;
  noConfirmBlockedB = ulcNoConfirmGateBlockedB;
  noConfirmLastTxValid = ulcNoConfirmLastTxValid;
  noConfirmLastTxBit1 = ulcNoConfirmLastTxBit1;
  noConfirmLastTxMs = ulcNoConfirmLastTxMs;
  driverAssistLast = uiDriverAssistLastRxMs;
  driverAssistBValid = lab3f8CanBValid;
  driverAssistRxB = lab3f8CanBRx;
  driverAssistBMs = lab3f8CanBLastMs;
  driverAssistBPeriod = lab3f8CanBPeriodMs;
  driverAssistBDlc = lab3f8CanBDlc;
  memcpy(driverAssistBData, lab3f8CanBData, 8);
  portEXIT_CRITICAL(&lab3f8Mux);
  portENTER_CRITICAL(&autoLc293Mux);
  autoLcEnabled = uiAutoLaneChangeEnabled;
  autoLcTargetBus = uiAutoLaneChangeTargetBus;
  autoLcAValid = uiAutoLaneChangeStockAValid;
  autoLcBValid = uiAutoLaneChangeStockBValid;
  autoLcARaw = uiAutoLaneChangeStockARaw;
  autoLcBRaw = uiAutoLaneChangeStockBRaw;
  autoLcAMs = uiAutoLaneChangeStockAMs;
  autoLcBMs = uiAutoLaneChangeStockBMs;
  autoLcRxA = uiAutoLaneChangeRxA;
  autoLcRxB = uiAutoLaneChangeRxB;
  autoLcTxAOk = uiAutoLaneChangeTxAOk;
  autoLcTxAFail = uiAutoLaneChangeTxAFail;
  autoLcTxBOk = uiAutoLaneChangeTxBOk;
  autoLcTxBFail = uiAutoLaneChangeTxBFail;
  autoLcBlockedA = uiAutoLaneChangeGateBlockedA;
  autoLcBlockedB = uiAutoLaneChangeGateBlockedB;
  autoLcLastValid = uiAutoLaneChangeLastTxValid;
  autoLcLastBus = uiAutoLaneChangeLastTxBus;
  autoLcLastRaw = uiAutoLaneChangeLastTxRaw;
  autoLcLastMs = uiAutoLaneChangeLastTxMs;
  portEXIT_CRITICAL(&autoLc293Mux);

  uint8_t state4;
  bool dasStateValid;
  uint32_t dasLast;
  portENTER_CRITICAL(&stateMux);
  state4 = dasAutopilotState4;
  dasStateValid = dasAutopilotStateValid;
  dasLast = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  const bool gate = lab3f8AutosteerGateOpen(now);
  const bool policyGate = lab3f8ApPolicyGateOpen(true);
  const bool noConfirmSupported = activeProfileUlcNoConfirmSupported();
  const bool noConfirmGate = ulcNoConfirmGateOpen();
  const bool autoLcGate = uiAutoLaneChangeGateOpen();
  const uint32_t dasAge = dasLast == 0 ? 999999UL : (uint32_t)(now - dasLast);
  const uint32_t rxAge = driverAssistLast == 0 ? 999999UL : (uint32_t)(now - driverAssistLast);
  const uint32_t lastTxAge = (!lastTxValid || lastTxMs == 0) ? 999999UL : (uint32_t)(now - lastTxMs);
  const uint32_t noConfirmLastTxAge = (!noConfirmLastTxValid || noConfirmLastTxMs == 0) ? 999999UL : (uint32_t)(now - noConfirmLastTxMs);
  const uint32_t ulcOffLastTxAge = (!ulcOffLastTxValid || ulcOffLastTxMs == 0) ? 999999UL : (uint32_t)(now - ulcOffLastTxMs);
  const uint32_t autoLcAAge = (!autoLcAValid || autoLcAMs == 0) ? 999999UL : (uint32_t)(now - autoLcAMs);
  const uint32_t autoLcBAge = (!autoLcBValid || autoLcBMs == 0) ? 999999UL : (uint32_t)(now - autoLcBMs);
  const uint32_t autoLcLastAge = (!autoLcLastValid || autoLcLastMs == 0) ? 999999UL : (uint32_t)(now - autoLcLastMs);
  const char *canBName = activeProfileCanBName();
  const uint32_t driverAssistBAge = (!driverAssistBValid || driverAssistBMs == 0) ? 999999UL : (uint32_t)(now - driverAssistBMs);
  const uint32_t driverAssistBHzDeci = driverAssistBPeriod ? (10000u + driverAssistBPeriod / 2u) / driverAssistBPeriod : 0u;
  if (driverAssistBValid && driverAssistBDlc >= 1) {
    stockStalkConfirm = getBit(driverAssistBData, 1);
    stockStalkConfirmValid = true;
  }

  String s;
  s.reserve(4200);
  JsonWriterArduino jw(s);
  jw.boolean("anyOverride", lab3f8AnyOverrideSelected());
  jw.boolean("gateOpen", gate);
  jw.string("gateRule", "AUTOSTEER_ONLY");
  jw.i32("dasState", (int32_t)(state4));
  jw.boolean("dasStateValid", dasStateValid);
  jw.u32("dasAgeMs", (uint32_t)(dasAge));
  jw.i32("alcMode", (int32_t)(alcMode));
  jw.i32("ulcBlindMode", (int32_t)(blindMode));
  jw.i32("ulcOffHighwayMode", (int32_t)(ulcOffHighwayMode));
  jw.boolean("policyGateOpen", policyGate);
  jw.boolean("stockAlcOffHighway", uiAlcOffHighwayEnable);
  jw.boolean("stockUlcOffHighway", uiUlcOffHighway);
  jw.i32("stockUlcBlindSpot", (int32_t)(uiUlcBlindSpotConfig));
  jw.u32("ulcOffHighwayTxOk", (uint32_t)(ulcOffTxOk));
  jw.u32("ulcOffHighwayTxFail", (uint32_t)(ulcOffTxFail));
  jw.u32("ulcOffHighwayBlocked", (uint32_t)(ulcOffBlocked));
  jw.boolean("ulcOffHighwayLastTxValid", ulcOffLastTxValid);
  jw.i32("ulcOffHighwayLastTxRaw", (int32_t)(ulcOffLastTxRaw));
  jw.u32("ulcOffHighwayLastTxAgeMs", (uint32_t)(ulcOffLastTxAge));
  jw.u32("rxAgeMs", (uint32_t)(rxAge));
  jw.u32("txOk", (uint32_t)(txOk));
  jw.u32("txFail", (uint32_t)(txFail));
  jw.u32("gateBlocked", (uint32_t)(blocked));
  jw.boolean("lastTxValid", lastTxValid);
  jw.i32("lastTxBlind", (int32_t)(lastTxBlind));
  jw.i32("lastTxStockBlind", (int32_t)(lastTxStockBlind));
  jw.i32("lastTxSelectedBlind", (int32_t)(lastTxSelectedBlind));
  jw.boolean("lastTxBlindChanged", lastTxBlindChanged);
  jw.i32("lastTxResult", (int32_t)(lastTxResult));
  jw.boolean("lastTxBit56", lastTxBit56);
  jw.u32("lastTxAgeMs", (uint32_t)(lastTxAge));
  jw.boolean("stalkConfirmSupported", noConfirmSupported);
  jw.boolean("stalkConfirmEnabled", noConfirmEnabled);
  jw.i32("stalkConfirmTimingMode", (int32_t)(noConfirmTimingMode));
  jw.boolean("stalkConfirmGateOpen", noConfirmGate);
  jw.string("stalkConfirmRoute", "CAN B / Chassis");
  jw.string("stalkConfirmCanBName", canBName);
  jw.boolean("stockStalkConfirmValid", stockStalkConfirmValid);
  jw.i32("stockStalkConfirm", stockStalkConfirmValid ? (stockStalkConfirm ? 1 : 0) : -1);
  jw.raw("forcedStalkConfirm", "0");
  jw.u32("stalkConfirmTxOk", (uint32_t)(noConfirmTxOk));
  jw.u32("stalkConfirmTxFail", (uint32_t)(noConfirmTxFail));
  jw.u32("stalkConfirmTxBOk", (uint32_t)(noConfirmTxBOk));
  jw.u32("stalkConfirmTxBFail", (uint32_t)(noConfirmTxBFail));
  jw.u32("stalkConfirmBlocked", (uint32_t)(noConfirmBlocked));
  jw.u32("stalkConfirmBlockedB", (uint32_t)(noConfirmBlockedB));
  jw.boolean("stalkConfirmLastTxValid", noConfirmLastTxValid);
  jw.u32("stalkConfirmLastTxBit1", noConfirmLastTxBit1 ? 1u : 0u);
  jw.u32("stalkConfirmLastTxAgeMs", (uint32_t)(noConfirmLastTxAge));
  jw.u32("driverAssistRxB", (uint32_t)(driverAssistRxB));
  jw.u32("driverAssistAgeBMs", (uint32_t)(driverAssistBAge));
  jw.fixed("driverAssistHzB", (int32_t)driverAssistBHzDeci, 1u);
  jw.i32("driverAssistBit1B", driverAssistBValid && driverAssistBDlc ? (getBit(driverAssistBData, 1) ? 1 : 0) : -1);
  jw.string("driverAssistRawB", lab3f8RawHex(driverAssistBData, driverAssistBValid ? driverAssistBDlc : 0));
  jw.boolean("autoLaneChangeEnabled", autoLcEnabled);
  jw.i32("autoLaneChangeTargetBus", (int32_t)(autoLcTargetBus));
  jw.boolean("autoLaneChangeGateOpen", autoLcGate);
  jw.boolean("autoLaneChangeStockAValid", autoLcAValid);
  jw.i32("autoLaneChangeStockARaw", (int32_t)(autoLcARaw));
  jw.u32("autoLaneChangeStockAAgeMs", (uint32_t)(autoLcAAge));
  jw.u32("autoLaneChangeRxA", (uint32_t)(autoLcRxA));
  jw.boolean("autoLaneChangeStockBValid", autoLcBValid);
  jw.i32("autoLaneChangeStockBRaw", (int32_t)(autoLcBRaw));
  jw.u32("autoLaneChangeStockBAgeMs", (uint32_t)(autoLcBAge));
  jw.u32("autoLaneChangeRxB", (uint32_t)(autoLcRxB));
  jw.u32("autoLaneChangeTxAOk", (uint32_t)(autoLcTxAOk));
  jw.u32("autoLaneChangeTxAFail", (uint32_t)(autoLcTxAFail));
  jw.u32("autoLaneChangeTxBOk", (uint32_t)(autoLcTxBOk));
  jw.u32("autoLaneChangeTxBFail", (uint32_t)(autoLcTxBFail));
  jw.u32("autoLaneChangeBlockedA", (uint32_t)(autoLcBlockedA));
  jw.u32("autoLaneChangeBlockedB", (uint32_t)(autoLcBlockedB));
  jw.boolean("autoLaneChangeLastTxValid", autoLcLastValid);
  jw.i32("autoLaneChangeLastTxBus", (int32_t)(autoLcLastBus));
  jw.i32("autoLaneChangeLastTxRaw", (int32_t)(autoLcLastRaw));
  jw.u32("autoLaneChangeLastTxAgeMs", (uint32_t)(autoLcLastAge));
  jw.finish();
  return s;
}

static const char* r79RuntimeReasonNameForUi(const R79RuntimeStatus &status) {
  switch (status.state) {
    case R79_TX_STATE_SUSPENDED:
      return r79TxReasonNamePure(status.decision.reason);
    case R79_TX_STATE_WAIT_TEMPLATE:
    case R79_TX_STATE_CAN_OFFLINE:
    case R79_TX_STATE_ADMIN_HOLD:
    case R79_TX_STATE_AP_BLOCKED:
    case R79_TX_STATE_AP_WAIT:
    case R79_TX_STATE_AP_UNKNOWN:
      return r79RuntimeStateName(status.state);
    default:
      return r79TxReasonNamePure(status.decision.reason);
  }
}

static String r79ApControlStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&stateMux);
  const R79ApGateConfigPure config = r79ApGateConfig;
  const R79ApGateDecisionPure gate = r79ApGateDecisionLocked(now);
  const bool ap = dasStateApActivePure(dasAutopilotStateValid, dasAutopilotState4);
  portEXIT_CRITICAL(&stateMux);
  String s; s.reserve(240);
  JsonWriterArduino jw(s);
  jw.boolean("enabled", config.enabled);
  jw.boolean("allowManualDriving", config.allowManualDriving);
  jw.u32("mode", config.mode);
  jw.u32("delaySeconds", config.delaySeconds);
  jw.boolean("apActive", ap);
  jw.boolean("gateAllowed", gate.allowed);
  jw.string("gateReason", r79ApGateReasonName(gate.reason));
  jw.u32("remainingMs", gate.remainingMs);
  jw.finish();
  return s;
}

static String r79StatsToJson() {
  const uint32_t now = (uint32_t)millis();
  const R79RuntimeStatus runtime = r79RuntimeStatusSnapshot(now);
  uint8_t bit18Policy, transportMode, mode1WaitMode, stockBit43, effectiveBit43;
  bool stockValid, lastTxValid, dmsNagEnabled, mode1Reinject, mode2Reinject;
  bool hw3Enabled;
  uint8_t stockBit47;
  uint16_t mode1Delay, mode2Delay;
  uint32_t bit43Rx0, bit43Rx1, bit43Changes;
  uint32_t rx, txOk, txFail, fastAttempts, fastOk, fastFail;
  uint32_t periodicOk, periodicFail, quietArm, quietFire, quietGuard;
  uint32_t dmsOnlyOk, dmsOnlyFail, dmsBlockedByR79;
  portENTER_CRITICAL(&r79LabMux);
  bit18Policy = r79Bit18Policy;
  transportMode = r79ModeSanitizePure(r79TransportMode);
  mode1WaitMode = r79Mode1WaitModeSanitizePure(r79Mode1TxWaitMode);
  mode1Reinject = r79Mode1ReinjectEnabled;
  mode1Delay = r79FixedQuietDelaySanitizePure(r79Mode1DelayMs);
  mode2Reinject = r79Mode2ReinjectEnabled;
  mode2Delay = r79Mode2DelayMs;
  hw3Enabled = r79Hw3Enabled;
  stockBit47 = (r79LabLastStockRaw[5] >> 7) & 1u;
  stockValid = r79LabStockValid;
  lastTxValid = r79LabLastTxValid;
  stockBit43 = r79LabStockCabinCamera;
  effectiveBit43 = r79LabEffectiveCabinCamera;
  bit43Rx0 = r79LabBit43Rx0;
  bit43Rx1 = r79LabBit43Rx1;
  bit43Changes = r79LabBit43Changes;
  rx = r79Lab3fdRx;
  txOk = r79LabTxOk;
  txFail = r79LabTxFail;
  fastAttempts = r79FastEchoAttempts;
  fastOk = r79FastEchoTxOk;
  fastFail = r79FastEchoTxFail;
  periodicOk = r79LabPeriodicTxOk;
  periodicFail = r79LabPeriodicTxFail;
  quietArm = r79QuietArmCount;
  quietFire = r79QuietFireCount;
  quietGuard = r79QuietGuardSkip;
  dmsOnlyOk = r79DmsOnlyTxOk;
  dmsOnlyFail = r79DmsOnlyTxFail;
  dmsBlockedByR79 = r79DmsOnlyBlockedByR79;
  portEXIT_CRITICAL(&r79LabMux);
  portENTER_CRITICAL(&nagCfgMux);
  dmsNagEnabled = nagCfg.dmsControlEnabled;
  portEXIT_CRITICAL(&nagCfgMux);

  String s;
  s.reserve(760);
  JsonWriterArduino jw(s);
  jw.boolean("fixedPolicy", true);
  jw.u32("mode", transportMode);
  jw.string("modeName", transportMode == R79_MODE_2_PURE ? "Mode 2" : "Mode 1");
  jw.string("transport", transportMode == R79_MODE_2_PURE
      ? "MUX1_FAST_ECHO_0MS"
      : (mode1WaitMode == R79_MODE1_FAST_ECHO_PURE
          ? "D9_MUX1_FAST_ECHO_0MS" : "D9_MUX1_2MS_WAIT"));
  if (transportMode == R79_MODE_2_PURE) {
    jw.string("periodic", mode2Reinject ? "OPTIONAL_MUX2_REINJECTION" : "OFF");
  } else {
    if (mode1Reinject) {
      String periodic = "MUX2_PLUS_";
      periodic += (unsigned int)mode1Delay;
      periodic += "MS_1X";
      jw.string("periodic", periodic);
    } else {
      jw.string("periodic", "OFF");
    }
  }
  jw.u32("mode1TxWaitMode", mode1WaitMode);
  jw.boolean("mode1ReinjectEnabled", mode1Reinject);
  jw.u32("mode1DelayMs", mode1Delay);
  jw.boolean("mode2ReinjectEnabled", mode2Reinject);
  jw.u32("mode2DelayMs", mode2Delay);
  jw.u32("bit18Mode", transportMode == R79_MODE_2_PURE
      ? R79_BIT18_STOCK_PURE : bit18Policy);
  jw.string("bit18ModeName", transportMode == R79_MODE_2_PURE
      ? "STOCK" : (bit18Policy == R79_BIT18_STOCK_PURE ? "STOCK" : "FORCE_0"));
  jw.u32("bit19", 0u);
  const bool hw3Supported = activeProfileR79Hw3Supported();
  const bool hw3Active = hw3Enabled && hw3Supported;
  jw.boolean("hw3Enabled", hw3Enabled);
  jw.boolean("hw3Supported", hw3Supported);
  jw.boolean("hw3Active", hw3Active);
  jw.string("bit47ModeName", hw3Active ? "STOCK" : "FORCE_1");
  jw.boolean("stockBit47Valid", stockValid);
  jw.u32("stockBit47", stockBit47);
  jw.boolean("bit47Valid", !hw3Active || stockValid);
  jw.u32("bit47", hw3Active ? stockBit47 : 1u);
  jw.boolean("dmsNagSupported", activeProfileDmsNagSupported());
  jw.boolean("dmsNagEnabled", dmsNagEnabled);
  jw.boolean("dmsNagActive", r79DmsNagActive());
  jw.boolean("stockBit43Valid", stockValid);
  jw.u32("stockBit43", stockBit43);
  jw.boolean("effectiveBit43Valid", lastTxValid);
  jw.u32("effectiveBit43", effectiveBit43);
  jw.u32("bit43Rx0", bit43Rx0);
  jw.u32("bit43Rx1", bit43Rx1);
  jw.u32("bit43Changes", bit43Changes);
  jw.string("runtimeState", r79RuntimeStateName(runtime.state));
  jw.u32("apWaitRemainingMs", runtime.apGate.remainingMs);
  jw.string("apGateReason", r79ApGateReasonName(runtime.apGate.reason));
  jw.boolean("stockTemplateValid", stockValid);
  jw.u32("stockMux1Rx", rx);
  jw.u32("txOk", txOk);
  jw.u32("txFail", txFail);
  jw.u32("fastAttempts", fastAttempts);
  jw.u32("fastTxOk", fastOk);
  jw.u32("fastTxFail", fastFail);
  jw.u32("periodicTxOk", periodicOk);
  jw.u32("periodicTxFail", periodicFail);
  jw.u32("quietArm", quietArm);
  jw.u32("quietFire", quietFire);
  jw.u32("quietGuardSkip", quietGuard);
  jw.u32("dmsOnlyTxOk", dmsOnlyOk);
  jw.u32("dmsOnlyTxFail", dmsOnlyFail);
  jw.u32("dmsOnlyBlockedByR79", dmsBlockedByR79);
  jw.finish();
  return s;
}

static constexpr uint32_t CAN_TRAFFIC_UI_FRESH_MS = 1500;

struct CanTrafficUiSnapshot {
  bool bodySeen;
  bool chassisSeen;
  bool partySeen;
  bool bodyOnline;
  bool chassisOnline;
  bool partyOnline;
  uint32_t bodyAgeMs;
  uint32_t chassisAgeMs;
  uint32_t partyAgeMs;
};

static CanTrafficUiSnapshot canTrafficUiSnapshot() {
  const uint32_t now = (uint32_t)millis();
  const uint32_t lastBody = lastCanAFrameMs;
  const uint32_t lastChassis = lastCanBFrameMs;
  const uint32_t lastParty = lastCanCFrameMs;
  CanTrafficUiSnapshot t = {};
  t.bodySeen = lastBody != 0;
  t.chassisSeen = lastChassis != 0;
  t.partySeen = lastParty != 0;
  t.bodyAgeMs = t.bodySeen ? (uint32_t)(now - lastBody) : 0;
  t.chassisAgeMs = t.chassisSeen ? (uint32_t)(now - lastChassis) : 0;
  t.partyAgeMs = t.partySeen ? (uint32_t)(now - lastParty) : 0;
  t.bodyOnline = t.bodySeen && t.bodyAgeMs <= CAN_TRAFFIC_UI_FRESH_MS;
  t.chassisOnline = t.chassisSeen && t.chassisAgeMs <= CAN_TRAFFIC_UI_FRESH_MS;
  t.partyOnline = t.partySeen && t.partyAgeMs <= CAN_TRAFFIC_UI_FRESH_MS;
  return t;
}

static String canTrafficStatsToJson() {
  const CanTrafficUiSnapshot t = canTrafficUiSnapshot();
  uint32_t overflowCount, overflowLastMs;
  uint8_t overflowLastFlags;
  mcpRxOverflowSnapshot(overflowCount, overflowLastMs, overflowLastFlags);
  String s;
  s.reserve(384);
  JsonWriterArduino jw(s);
  jw.boolean("bodyTrafficSeen", t.bodySeen);
  jw.boolean("bodyTrafficOnline", t.bodyOnline);
  jw.u32("bodyTrafficAgeMs", t.bodyAgeMs);
  jw.boolean("chassisTrafficSeen", t.chassisSeen);
  jw.boolean("chassisTrafficOnline", t.chassisOnline);
  jw.u32("chassisTrafficAgeMs", t.chassisAgeMs);
  jw.boolean("partyTrafficSeen", t.partySeen);
  jw.boolean("partyTrafficOnline", t.partyOnline);
  jw.u32("partyTrafficAgeMs", t.partyAgeMs);
  jw.u32("trafficFreshMs", CAN_TRAFFIC_UI_FRESH_MS);
  jw.u32("mcpRxOverflowCount", overflowCount);
  jw.u32("mcpRxOverflowLastAgeMs", overflowLastMs ? millis() - overflowLastMs : 999999UL);
  jw.u32("mcpRxOverflowLastFlags", overflowLastFlags);
  jw.finish();
  return s;
}

static String systemStatsToJson() {
  String s;
  s.reserve(4600);
  JsonWriterArduino jw(s);
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t minFreeHeap = ESP.getMinFreeHeap();
  const uint32_t largestHeapBlock = ESP.getMaxAllocHeap();
  const uint32_t stackCanA = canTaskBodyHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskBodyHandle) : 0;
  const uint32_t stackCanB = canTaskChassisHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskChassisHandle) : 0;
  const uint32_t stackCanSup = canSupervisorHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canSupervisorHandle) : 0;
  const uint32_t stackWeb = webTaskHandle ? (uint32_t)uxTaskGetStackHighWaterMark(webTaskHandle) : 0;
  const uint32_t stackS3xy = s3xyTaskHandle ? (uint32_t)uxTaskGetStackHighWaterMark(s3xyTaskHandle) : 0;
  McpChassisStatus chassisNow = {};
  const bool chassisStatusOk = (mcpChassisGetStatus(&chassisNow) == ESP_OK);
  CanChassisRecoverySnapshot busOffSnap = {};
  uint32_t chassisBusOffCount, chassisStoppedCount, chassisLocalRecoveryStartCount;
  uint32_t chassisRecoveryStartFailCount, chassisRestartOkCount, chassisRestartFailCount;
  uint32_t chassisLastEventMs, lastRxGapMs, maxRxGapMs;
  uint8_t chassisLastEventReason, lastHardDiagReason;
  uint8_t taskHeartbeatLastCause;
  uint32_t taskHeartbeatLastAgeAms, taskHeartbeatLastAgeBms;
  uint32_t taskHeartbeatTimeoutCountA, taskHeartbeatTimeoutCountB, taskHeartbeatTimeoutCountBoth;
  CanTaskTimeoutSnapshotPure taskSnapshotA = {};
  CanTaskTimeoutSnapshotPure taskSnapshotB = {};
  portENTER_CRITICAL(&canRecoveryMux);
  busOffSnap = canChassisLastBusOffSnapshot;
  chassisBusOffCount = canChassisBusOffCount;
  chassisStoppedCount = canChassisStoppedCount;
  chassisLocalRecoveryStartCount = canChassisLocalRecoveryStartCount;
  chassisRecoveryStartFailCount = canChassisRecoveryStartFailCount;
  chassisRestartOkCount = canChassisRestartOkCount;
  chassisRestartFailCount = canChassisRestartFailCount;
  chassisLastEventReason = canChassisLastEventReason;
  chassisLastEventMs = canChassisLastEventMs;
  lastHardDiagReason = canLastHardDiagReason;
  lastRxGapMs = canBLastRxGapMs;
  maxRxGapMs = canBMaxRxGapMs;
  taskHeartbeatLastCause = canTaskHeartbeatLastCause;
  taskHeartbeatLastAgeAms = canTaskHeartbeatLastAgeAms;
  taskHeartbeatLastAgeBms = canTaskHeartbeatLastAgeBms;
  taskHeartbeatTimeoutCountA = canTaskHeartbeatTimeoutCountA;
  taskHeartbeatTimeoutCountB = canTaskHeartbeatTimeoutCountB;
  taskHeartbeatTimeoutCountBoth = canTaskHeartbeatTimeoutCountBoth;
  taskSnapshotA = canTaskHeartbeatLastSnapshotA;
  taskSnapshotB = canTaskHeartbeatLastSnapshotB;
  portEXIT_CRITICAL(&canRecoveryMux);
  const uint32_t statsNow = (uint32_t)millis();
  const bool canARecordCurrent =
      canBusOffPersistenceBoot[0] == CAN_BUS_OFF_BOOT_CURRENT;
  const bool canBRecordCurrent =
      canBusOffPersistenceBoot[1] == CAN_BUS_OFF_BOOT_CURRENT;
  jw.string("fwVersion", FW_VERSION);
  jw.u32("freeHeap", freeHeap);
  jw.u32("minFreeHeap", minFreeHeap);
  jw.u32("largestHeapBlock", largestHeapBlock);
  jw.u32("stackCanA", stackCanA);
  jw.u32("stackCanB", stackCanB);
  jw.u32("stackCanSup", stackCanSup);
  jw.u32("stackWeb", stackWeb);
  jw.u32("stackS3xy", stackS3xy);
  jw.boolean("s3xyDiagnosticsEnabled", S3XY_DIAGNOSTICS_ENABLED);
  jw.u32("s3xyLogCapacity", (uint32_t)(S3XY_DIAGNOSTICS_ENABLED ? (unsigned)S3XY_LOG_MAX : 0U));
  jw.u32("uptimeS", (uint32_t)((millis() - bootTime) / 1000));
  jw.boolean("mcpReady", mcpReady);
  jw.u32("mcpState", (uint32_t)(mcpState));
  jw.boolean("mcpChassisReady", mcpChassisReady);
  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  jw.boolean("mcpTrafficSeen", traffic.bodySeen);
  jw.boolean("mcpTrafficOnline", traffic.bodyOnline);
  jw.u32("mcpTrafficAgeMs", (uint32_t)(traffic.bodyAgeMs));
  jw.boolean("chassisTrafficSeen", traffic.chassisSeen);
  jw.boolean("chassisTrafficOnline", traffic.chassisOnline);
  jw.u32("chassisTrafficAgeMs", (uint32_t)(traffic.chassisAgeMs));
  jw.u32("trafficFreshMs", (uint32_t)(CAN_TRAFFIC_UI_FRESH_MS));
  uint32_t mcpOverflowCount, mcpOverflowLastMs;
  uint8_t mcpOverflowLastFlags;
  mcpRxOverflowSnapshot(mcpOverflowCount, mcpOverflowLastMs, mcpOverflowLastFlags);
  jw.u32("mcpRxOverflowCount", (uint32_t)(mcpOverflowCount));
  jw.u32("mcpRxOverflowLastAgeMs", (uint32_t)((mcpOverflowLastMs ? millis() - mcpOverflowLastMs : 999999UL)));
  jw.u32("mcpRxOverflowLastFlags", (uint32_t)(mcpOverflowLastFlags));
  jw.u32("canARxFramesProcessed", (uint32_t)canARxDiagnostics.framesProcessed);
  jw.u32("canARxMaxFramesPerLoop", (uint32_t)canARxDiagnostics.maxFramesPerLoop);
  jw.u32("canARxBudgetExhaustedLoops", (uint32_t)canARxDiagnostics.budgetExhaustedLoops);
  jw.u32("mcpRx0OverflowObservations", (uint32_t)canARxDiagnostics.rx0OverflowObservations);
  jw.u32("mcpRx1OverflowObservations", (uint32_t)canARxDiagnostics.rx1OverflowObservations);
  jw.u32("mcpRxFramesAtLastOverflow", (uint32_t)canARxDiagnostics.framesAtLastOverflow);
  jw.u32("mcpRxBudgetHitsAtLastOverflow", (uint32_t)canARxDiagnostics.budgetExhaustedAtLastOverflow);
  CanABusOffSnapshot canASnap = {};
  uint8_t canATraceFrozenCountLocal = 0;
  uint32_t canATraceFrozenMsLocal = 0, canATraceBusOffOrdinalLocal = 0;
  portENTER_CRITICAL(&canATxTraceMux);
  canASnap = canALastBusOffSnapshot;
  canATraceFrozenCountLocal = canATxTraceFrozenCount;
  canATraceFrozenMsLocal = canATxTraceFrozenMs;
  canATraceBusOffOrdinalLocal = canATxTraceFrozenBusOffOrdinal;
  portEXIT_CRITICAL(&canATxTraceMux);
  jw.u32("mcpErrorFlags", (uint32_t)(mcpLastErrorFlags));
  jw.u32("mcpErrorFlagsAgeMs", (uint32_t)((mcpLastErrorFlagsMs ? statsNow - mcpLastErrorFlagsMs : 999999UL)));
  jw.u32("mcpTxFailConsecutive", (uint32_t)(mcpTxFailConsecutive));
  jw.u32("mcpTxOk", (uint32_t)(mcpTxOk));
  jw.u32("mcpTxFail", (uint32_t)(mcpTxFail));
  jw.u32("mcpLastRecoverAgeMs", (uint32_t)((lastMcpRecoverMs ? statsNow - lastMcpRecoverMs : 999999UL)));
  jw.u32("mcpBusOffCount", (uint32_t)(canAMcpBusOffCount));
  jw.boolean("mcpBusOffSnapshotValid", canASnap.valid);
  jw.u32("mcpBusOffSnapshotAgeMs", (uint32_t)((canASnap.valid && canARecordCurrent ? statsNow - canASnap.capturedMs : 999999UL)));
  jw.u32("mcpBusOffSnapshotEflg", (uint32_t)(canASnap.eflg));
  jw.u32("mcpBusOffSnapshotTxFailSeq", (uint32_t)(canASnap.txFailConsecutive));
  jw.u32("mcpBusOffSnapshotRxAgeMs", (uint32_t)(canASnap.rxAgeMs));
  jw.u32("mcpBusOffSnapshotRxOverflow", (uint32_t)(canASnap.rxOverflowCount));
  jw.u32("mcpBusOffSnapshotTxOk", (uint32_t)(canASnap.txOk));
  jw.u32("mcpBusOffSnapshotTxFail", (uint32_t)(canASnap.txFail));
  jw.u32("canATxTraceCount", (uint32_t)(canATraceFrozenCountLocal));
  jw.u32("canATxTraceAgeMs", (uint32_t)((canATraceFrozenMsLocal && canARecordCurrent ? statsNow - canATraceFrozenMsLocal : 999999UL)));
  jw.u32("canATxTraceBusOffOrdinal", (uint32_t)(canATraceBusOffOrdinalLocal));
  jw.beginObject("canABusOffRecord");
  jw.string("busOffRecordBoot", canBusOffRecordBootName(CAN_BUS_OFF_BUS_A_PURE));
  jw.u32("busOffEventUptimeMs", canATraceFrozenMsLocal);
  jw.string("busOffPersistState", canBusOffPersistStateName(CAN_BUS_OFF_BUS_A_PURE));
  jw.string("busOffPersistError", canBusOffPersistErrorName(CAN_BUS_OFF_BUS_A_PURE));
  jw.endObject();
  jw.u32("rtcBootCount", (uint32_t)(rtcBootCount));
  jw.u32("runtimeStatsResetCount", (uint32_t)(runtimeStatsResetCount));
  jw.u32("runtimeStatsLastResetMs", (uint32_t)(runtimeStatsLastResetMs));
  jw.u32("canHardReinit", (uint32_t)(canHardReinitCount));
  jw.u32("canHardReinitFail", (uint32_t)(canHardReinitFailCount));
  jw.i32("canLastHardReason", (int32_t)(canLastHardReinitReason));
  jw.i32("canLastHardDiagReason", (int32_t)(lastHardDiagReason));
  jw.string("canLastHardDiagReasonName", canRecoveryDiagnosticReasonName(lastHardDiagReason));
  jw.u32("canTaskHeartbeatLastCause", (uint32_t)taskHeartbeatLastCause);
  jw.string("canTaskHeartbeatLastCauseName", canTaskHeartbeatTimeoutCauseName(taskHeartbeatLastCause));
  jw.u32("canTaskHeartbeatLastAgeAms", (uint32_t)taskHeartbeatLastAgeAms);
  jw.u32("canTaskHeartbeatLastAgeBms", (uint32_t)taskHeartbeatLastAgeBms);
  jw.u32("canTaskHeartbeatTimeoutCountA", (uint32_t)taskHeartbeatTimeoutCountA);
  jw.u32("canTaskHeartbeatTimeoutCountB", (uint32_t)taskHeartbeatTimeoutCountB);
  jw.u32("canTaskHeartbeatTimeoutCountBoth", (uint32_t)taskHeartbeatTimeoutCountBoth);
  jw.string("canTaskSnapshotAStateName", taskHeartbeatLastCause
      ? canTaskStateName(taskSnapshotA.taskState) : "NONE");
  jw.string("canTaskSnapshotAStageName", canTaskStageNamePure(taskSnapshotA.stage));
  jw.u32("canTaskSnapshotAStageAgeMs", (uint32_t)taskSnapshotA.stageAgeMs);
  jw.u32("canTaskSnapshotAHeartbeatCount", (uint32_t)taskSnapshotA.heartbeatCount);
  jw.u32("canTaskSnapshotAMaxHeartbeatGapMs", (uint32_t)taskSnapshotA.maxHeartbeatGapMs);
  jw.u32("canTaskSnapshotALoopCount", (uint32_t)taskSnapshotA.loopCount);
  jw.u32("canTaskSnapshotALastLoopUs", (uint32_t)taskSnapshotA.lastLoopDurationUs);
  jw.u32("canTaskSnapshotAMaxLoopUs", (uint32_t)taskSnapshotA.maxLoopDurationUs);
  jw.u32("canTaskSnapshotAStackHighWater", (uint32_t)taskSnapshotA.stackHighWater);
  jw.string("canTaskSnapshotBStateName", taskHeartbeatLastCause
      ? canTaskStateName(taskSnapshotB.taskState) : "NONE");
  jw.string("canTaskSnapshotBStageName", canTaskStageNamePure(taskSnapshotB.stage));
  jw.u32("canTaskSnapshotBStageAgeMs", (uint32_t)taskSnapshotB.stageAgeMs);
  jw.u32("canTaskSnapshotBHeartbeatCount", (uint32_t)taskSnapshotB.heartbeatCount);
  jw.u32("canTaskSnapshotBMaxHeartbeatGapMs", (uint32_t)taskSnapshotB.maxHeartbeatGapMs);
  jw.u32("canTaskSnapshotBLoopCount", (uint32_t)taskSnapshotB.loopCount);
  jw.u32("canTaskSnapshotBLastLoopUs", (uint32_t)taskSnapshotB.lastLoopDurationUs);
  jw.u32("canTaskSnapshotBMaxLoopUs", (uint32_t)taskSnapshotB.maxLoopDurationUs);
  jw.u32("canTaskSnapshotBStackHighWater", (uint32_t)taskSnapshotB.stackHighWater);
  jw.boolean("canRecoverySleeping", recoverySleeping);
  jw.i32("chassisState", chassisStatusOk ? (int32_t)chassisNow.state : -1);
  jw.string("mcpChassisStateName", chassisStatusOk ? mcpChassisStateName(chassisNow.state) : "UNAVAILABLE");
  jw.boolean("mcpPartyReady", mcpPartyReady);
  jw.u32("mcpPartyState", (uint32_t)mcpPartyState);
  jw.u32("mcpPartyTxOk", (uint32_t)mcpPartyTxOk);
  jw.u32("mcpPartyTxFail", (uint32_t)mcpPartyTxFail);
  jw.u32("mcpPartyRxCount", (uint32_t)mcpPartyRxCount);
  jw.u32("mcpPartyTrafficAgeMs", (uint32_t)((lastCanCFrameMs ? statsNow - lastCanCFrameMs : 999999UL)));
  jw.u32("chassisBusOffCount", (uint32_t)(chassisBusOffCount));
  jw.u32("chassisStoppedCount", (uint32_t)(chassisStoppedCount));
  jw.u32("chassisLocalRecoveryStartCount", (uint32_t)(chassisLocalRecoveryStartCount));
  jw.u32("chassisRecoveryStartFailCount", (uint32_t)(chassisRecoveryStartFailCount));
  jw.u32("chassisRestartOkCount", (uint32_t)(chassisRestartOkCount));
  jw.u32("chassisRestartFailCount", (uint32_t)(chassisRestartFailCount));
  jw.i32("chassisLastEventReason", (int32_t)(chassisLastEventReason));
  jw.string("chassisLastEventReasonName", canRecoveryDiagnosticReasonName(chassisLastEventReason));
  jw.u32("chassisLastEventAgeMs", (uint32_t)((chassisLastEventMs ? statsNow - chassisLastEventMs : 999999UL)));
  jw.u32("canBLastRxGapMs", (uint32_t)(lastRxGapMs));
  jw.u32("canBMaxRxGapMs", (uint32_t)(maxRxGapMs));
  jw.u32("chassisTxErrorCounter", (uint32_t)((chassisStatusOk ? chassisNow.tx_error_counter : 0)));
  jw.u32("chassisRxErrorCounter", (uint32_t)((chassisStatusOk ? chassisNow.rx_error_counter : 0)));
  jw.u32("chassisTxFailedCount", (uint32_t)((chassisStatusOk ? chassisNow.tx_failed_count : 0)));
  jw.u32("chassisRxMissedCount", (uint32_t)((chassisStatusOk ? chassisNow.rx_missed_count : 0)));
  jw.u32("chassisRxOverrunCount", (uint32_t)((chassisStatusOk ? chassisNow.rx_overrun_count : 0)));
  jw.u32("chassisArbLostCount", (uint32_t)((chassisStatusOk ? chassisNow.arb_lost_count : 0)));
  jw.u32("chassisBusErrorCount", (uint32_t)((chassisStatusOk ? chassisNow.bus_error_count : 0)));
  jw.boolean("chassisBusOffSnapshotValid", busOffSnap.valid);
  jw.u32("chassisBusOffSnapshotAgeMs", (uint32_t)((busOffSnap.valid && canBRecordCurrent ? statsNow - busOffSnap.capturedMs : 999999UL)));
  jw.u32("chassisBusOffSnapshotRxGapMs", (uint32_t)(busOffSnap.rxGapMs));
  jw.u32("chassisBusOffSnapshotTxQueue", (uint32_t)(busOffSnap.msgsToTx));
  jw.u32("chassisBusOffSnapshotRxQueue", (uint32_t)(busOffSnap.msgsToRx));
  jw.u32("chassisBusOffSnapshotTxErr", (uint32_t)(busOffSnap.txErrorCounter));
  jw.u32("chassisBusOffSnapshotRxErr", (uint32_t)(busOffSnap.rxErrorCounter));
  jw.u32("chassisBusOffSnapshotTxFailed", (uint32_t)(busOffSnap.txFailedCount));
  jw.u32("chassisBusOffSnapshotRxMissed", (uint32_t)(busOffSnap.rxMissedCount));
  jw.u32("chassisBusOffSnapshotRxOverrun", (uint32_t)(busOffSnap.rxOverrunCount));
  jw.u32("chassisBusOffSnapshotArbLost", (uint32_t)(busOffSnap.arbLostCount));
  jw.u32("chassisBusOffSnapshotBusError", (uint32_t)(busOffSnap.busErrorCount));
  uint8_t txTraceFrozenCount = 0;
  uint32_t txTraceFrozenMs = 0, txTraceBusOffOrdinal = 0;
  portENTER_CRITICAL(&canBTxTraceMux);
  txTraceFrozenCount = canBTxTraceFrozenCount;
  txTraceFrozenMs = canBTxTraceFrozenMs;
  txTraceBusOffOrdinal = canBTxTraceFrozenBusOffOrdinal;
  portEXIT_CRITICAL(&canBTxTraceMux);
  jw.u32("canBTxTraceCount", (uint32_t)(txTraceFrozenCount));
  jw.u32("canBTxTraceAgeMs", (uint32_t)((txTraceFrozenMs && canBRecordCurrent ? statsNow - txTraceFrozenMs : 999999UL)));
  jw.u32("canBTxTraceBusOffOrdinal", (uint32_t)(txTraceBusOffOrdinal));
  jw.beginObject("canBBusOffRecord");
  jw.string("busOffRecordBoot", canBusOffRecordBootName(CAN_BUS_OFF_BUS_B_PURE));
  jw.u32("busOffEventUptimeMs", txTraceFrozenMs);
  jw.string("busOffPersistState", canBusOffPersistStateName(CAN_BUS_OFF_BUS_B_PURE));
  jw.string("busOffPersistError", canBusOffPersistErrorName(CAN_BUS_OFF_BUS_B_PURE));
  jw.endObject();
  jw.u32("chassisTxQueueNow", (uint32_t)(chassisTxQueueNow));
  jw.u32("chassisTxQueueMax", (uint32_t)(chassisTxQueueMax));
  jw.u32("chassisRxQueueNow", (uint32_t)(chassisRxQueueNow));
  jw.u32("chassisRxQueueMax", (uint32_t)(chassisRxQueueMax));
  jw.u32("chassisNonSummonShed", (uint32_t)(chassisNonSummonShed));
  jw.boolean("otaInProgress", otaInProgress);
  jw.boolean("otaSuccess", otaSuccess);
  jw.boolean("otaError", otaError);
  jw.string("otaErrMsg", otaErrMsg);
  jw.u32("otaBytes", otaBytes);
  jw.u32("otaTotal", otaTotal);
  jw.finish();
  return s;
}

// ─── Boot timing capture export ─────────────────────────────

static const char* bootCaptureHardReasonName(uint8_t reason) {
  switch (reason) {
    case CAN_SUP_HARD_ACQUIRE: return "ACQUIRE";
    case CAN_SUP_HARD_STALE:   return "STALE";
    case CAN_SUP_HARD_MANUAL:  return "MANUAL";
    default:                   return "UNKNOWN";
  }
}

static void bootCaptureAppendEvent(String &out, const char *event, uint32_t t, const String &detail = String()) {
  out += event;
  out += ",";
  if (t == BOOT_CAPTURE_UNSET) out += "-1";
  else out += String((unsigned long)t);
  out += ",\"";
  out += detail;
  out += "\"\n";
}

static String bootCaptureToCsv() {
  uint32_t canInitDone, canTasks, wifiReady, firstA, firstB;
  uint32_t first370, first370Torque, first399, first24A, first249;
  uint16_t first370Raw, first370TorqueRaw;
  uint8_t hardCount;
  uint32_t hardDropped;
  BootHardReinitEvent hard[BOOT_CAPTURE_HARD_MAX];

  portENTER_CRITICAL(&bootCaptureMux);
  canInitDone = bootCapCanInitDoneMs;
  canTasks = bootCapCanTasksStartedMs;
  wifiReady = bootCapWifiReadyMs;
  firstA = bootCapFirstCanAMs;
  firstB = bootCapFirstCanBMs;
  first370 = bootCapFirst370Ms;
  first370Torque = bootCapFirst370TorqueMs;
  first399 = bootCapFirst399Ms;
  first24A = bootCapFirstParty24AMs;
  first249 = bootCapFirstVh249Ms;
  first370Raw = bootCapFirst370Raw;
  first370TorqueRaw = bootCapFirst370TorqueRaw;
  hardCount = bootCapHardCount;
  hardDropped = bootCapHardDropped;
  for (uint8_t i = 0; i < hardCount && i < BOOT_CAPTURE_HARD_MAX; i++) hard[i] = bootCapHard[i];
  portEXIT_CRITICAL(&bootCaptureMux);

  String out;
  out.reserve(2200);
  out = "event,time_ms,detail\n";
  bootCaptureAppendEvent(out, "BOOT_SETUP_START", 0, String(FW_VERSION));
  bootCaptureAppendEvent(out, "CAN_INIT_DONE", canInitDone);
  bootCaptureAppendEvent(out, "CAN_RX_TASKS_STARTED", canTasks);
  bootCaptureAppendEvent(out, "WIFI_AP_READY", wifiReady);
  bootCaptureAppendEvent(out, "FIRST_CAN_A_ANY", firstA, "Party/MCP2515");
  bootCaptureAppendEvent(out, "FIRST_CAN_B_ANY", firstB, "VH/CHASSIS");

  String d370;
  if (first370Raw != 0xFFFF) {
    const int16_t centiNm = (int16_t)first370Raw - 2050;
    d370 = "raw=" + String((unsigned)first370Raw) + ";torque_nm=" + centiString((int32_t)centiNm);
  } else d370 = "not_seen";
  bootCaptureAppendEvent(out, "FIRST_PARTY_0x370", first370, d370);

  String dTorque;
  if (first370TorqueRaw != 0xFFFF) {
    const int16_t centiNm = (int16_t)first370TorqueRaw - 2050;
    dTorque = "abs_torque_ge_0.10Nm;raw=" + String((unsigned)first370TorqueRaw) + ";torque_nm=" + centiString((int32_t)centiNm);
  } else dTorque = "not_seen";
  bootCaptureAppendEvent(out, "FIRST_0x370_ABS_TORQUE_GE_0.10NM", first370Torque, dTorque);

  bootCaptureAppendEvent(out, "FIRST_PARTY_0x399", first399, "DAS/AP state");
  bootCaptureAppendEvent(out, "FIRST_PARTY_0x24A_DLC8", first24A, "DAS visual debug / Auto Blinker source");
  bootCaptureAppendEvent(out, "FIRST_VH_0x249_DLC4", first249, "SCCM stalk status");

  for (uint8_t i = 0; i < hardCount && i < BOOT_CAPTURE_HARD_MAX; i++) {
    String startName = "HARD_REINIT_" + String((unsigned)(i + 1)) + "_START";
    String endName = "HARD_REINIT_" + String((unsigned)(i + 1)) + "_END";
    String detail = "reason=" + String(bootCaptureHardReasonName(hard[i].reason));
    bootCaptureAppendEvent(out, startName.c_str(), hard[i].startMs, detail);
    String endDetail = detail + ";success=" + String(hard[i].success == 1 ? "1" : hard[i].success == 0 ? "0" : "in_progress");
    bootCaptureAppendEvent(out, endName.c_str(), hard[i].endMs, endDetail);
  }

  bootCaptureAppendEvent(out, "EXPORT", bootCaptureNowMs(),
    "hard_reinit_events=" + String((unsigned)hardCount) +
    ";hard_reinit_dropped=" + String((unsigned long)hardDropped) +
    ";mcp_rx_count=" + String((unsigned long)__atomic_load_n(&canARxCount, __ATOMIC_RELAXED)) +
    ";vh_rx_count=" + String((unsigned long)__atomic_load_n(&canBRxCount, __ATOMIC_RELAXED)));
  return out;
}

static void httpBootCaptureCsv() {
  server.sendHeader("Content-Disposition", "attachment; filename=TMR_boot_capture.csv");
  server.send(200, "text/csv", bootCaptureToCsv());
}

static const char *canTxTraceSourceName(uint8_t source) {
  if (source == CAN_TX_TRACE_SOURCE_AUTO_BLINKER) return "AUTO_BLINKER";
  if (source == CAN_TX_TRACE_SOURCE_S3XY_BUTTON) return "S3XY_BUTTON";
  if (source == CAN_TX_TRACE_SOURCE_DRIVER_WINDOW_LAB) return "DRIVER_WINDOW_LAB";
  return nullptr;
}

static void canTxTraceRawHex(const uint8_t *data, uint8_t dlc,
                             char *out, size_t outLen) {
  if (!out || outLen == 0u) return;
  out[0] = '\0';
  if (!data) return;
  char *write = out;
  size_t remain = outLen;
  for (uint8_t i = 0; i < dlc && i < 8u; ++i) {
    const int n = snprintf(write, remain, "%s%02X", i ? " " : "", data[i]);
    if (n <= 0 || (size_t)n >= remain) break;
    write += n;
    remain -= (size_t)n;
  }
}

static const char *canATxTraceSourceName(uint8_t source, uint16_t id) {
  const char *tagged = canTxTraceSourceName(source);
  if (tagged) return tagged;
  if (id == (uint16_t)(nagCfg.targetId & 0x7FFU)) return "NAG";
  switch (id) {
    case 0x293: return "AUTO_LANE_CHANGE_ENABLE";
    case 0x334: return "PEDAL_MAP";
    case 0x249: return "AUTO_BLINKER_STALK";
    case 0x3C2: return "AUTO_BLINKER_STALKLESS";
    default: return "OTHER";
  }
}

static const char *canATxGateReasonName(uint8_t reason) {
  return reason == MCP_TX_OK ? "OK" : nagTxBlockReasonName(reason);
}

static void httpCanATxTraceCsv() {
  CanATxTraceEntry entries[CAN_A_TX_TRACE_CAPACITY] = {};
  uint8_t count = 0;
  uint32_t frozenMs = 0;
  uint32_t busOffOrdinal = 0;
  CanABusOffSnapshot snap = {};
  portENTER_CRITICAL(&canATxTraceMux);
  count = canATxTraceFrozenCount;
  frozenMs = canATxTraceFrozenMs;
  busOffOrdinal = canATxTraceFrozenBusOffOrdinal;
  snap = canALastBusOffSnapshot;
  if (count > CAN_A_TX_TRACE_CAPACITY) count = CAN_A_TX_TRACE_CAPACITY;
  memcpy(entries, canATxTraceFrozen, sizeof(CanATxTraceEntry) * count);
  portEXIT_CRITICAL(&canATxTraceMux);

  server.sendHeader("Content-Disposition", "attachment; filename=TMR_CANA_TX_TRACE.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  char line[384];
  snprintf(line, sizeof(line),
           "#record_boot,%s\n#persist_state,%s\n#persist_error,%s\n#bus_off_ordinal,%lu\n#event_uptime_ms,%lu\n#entry_count,%u\n#snapshot_eflg,0x%02X\n#snapshot_tx_fail_seq,%u\n#snapshot_rx_age_ms,%lu\n",
           canBusOffRecordBootName(CAN_BUS_OFF_BUS_A_PURE),
           canBusOffPersistStateName(CAN_BUS_OFF_BUS_A_PURE),
           canBusOffPersistErrorName(CAN_BUS_OFF_BUS_A_PURE),
           (unsigned long)busOffOrdinal, (unsigned long)frozenMs, (unsigned)count,
           (unsigned)snap.eflg, (unsigned)snap.txFailConsecutive, (unsigned long)snap.rxAgeMs);
  server.sendContent(line);
  server.sendContent("seq,relative_ms,uptime_ms,id,source,dlc,mcp_result,result_name,gate_reason,raw\n");
  for (uint8_t i = 0; i < count; i++) {
    const CanATxTraceEntry &e = entries[i];
    char raw[32] = {};
    canTxTraceRawHex(e.data, e.dlc, raw, sizeof(raw));
    const int32_t rel = frozenMs ? (int32_t)(e.capturedMs - frozenMs) : 0;
    const char *resultName = e.result == (int32_t)MCP2515::ERROR_OK ? "OK" : "ERROR";
    snprintf(line, sizeof(line), "%lu,%ld,%lu,0x%03X,%s,%u,%ld,%s,%s,%s\n",
             (unsigned long)e.seq, (long)rel, (unsigned long)e.capturedMs,
             (unsigned)e.id, canATxTraceSourceName(e.source, e.id), (unsigned)e.dlc,
             (long)e.result, resultName, canATxGateReasonName(e.reason), raw);
    server.sendContent(line);
  }
  server.sendContent("");
}

static const char *canBTxTraceSourceName(uint8_t source, uint16_t id) {
  const char *tagged = canTxTraceSourceName(source);
  if (tagged) return tagged;
  switch (id) {
    case 0x293: return "AUTO_LANE_CHANGE_ENABLE";
    case 0x249: return "AUTO_BLINKER";
    case 0x334: return "PEDAL_MAP";
    case 0x3F8: return "DRIVER_ASSIST_OVERLAY";
    case 0x3FD: return "R79_SUMMON_TLSSC";
    default: return "OTHER";
  }
}

static void httpCanBTxTraceCsv() {
  CanBTxTraceEntry entries[CAN_B_TX_TRACE_CAPACITY] = {};
  uint8_t count = 0;
  uint32_t frozenMs = 0;
  uint32_t busOffOrdinal = 0;
  portENTER_CRITICAL(&canBTxTraceMux);
  count = canBTxTraceFrozenCount;
  frozenMs = canBTxTraceFrozenMs;
  busOffOrdinal = canBTxTraceFrozenBusOffOrdinal;
  if (count > CAN_B_TX_TRACE_CAPACITY) count = CAN_B_TX_TRACE_CAPACITY;
  memcpy(entries, canBTxTraceFrozen, sizeof(CanBTxTraceEntry) * count);
  portEXIT_CRITICAL(&canBTxTraceMux);

  server.sendHeader("Content-Disposition", "attachment; filename=TMR_CANB_TX_TRACE.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  char line[320];
  snprintf(line, sizeof(line), "#record_boot,%s\n#persist_state,%s\n#persist_error,%s\n#bus_off_ordinal,%lu\n#event_uptime_ms,%lu\n#entry_count,%u\n",
           canBusOffRecordBootName(CAN_BUS_OFF_BUS_B_PURE),
           canBusOffPersistStateName(CAN_BUS_OFF_BUS_B_PURE),
           canBusOffPersistErrorName(CAN_BUS_OFF_BUS_B_PURE),
           (unsigned long)busOffOrdinal, (unsigned long)frozenMs, (unsigned)count);
  server.sendContent(line);
  server.sendContent("seq,relative_ms,uptime_ms,id,source,dlc,result,result_name,raw\n");
  for (uint8_t i = 0; i < count; i++) {
    const CanBTxTraceEntry &e = entries[i];
    char raw[32] = {};
    canTxTraceRawHex(e.data, e.dlc, raw, sizeof(raw));
    const int32_t rel = frozenMs ? (int32_t)(e.capturedMs - frozenMs) : 0;
    snprintf(line, sizeof(line), "%lu,%ld,%lu,0x%03X,%s,%u,%ld,%s,%s\n",
             (unsigned long)e.seq, (long)rel, (unsigned long)e.capturedMs,
             (unsigned)e.id, canBTxTraceSourceName(e.source, e.id), (unsigned)e.dlc,
             (long)e.result, esp_err_to_name((esp_err_t)e.result), raw);
    server.sendContent(line);
  }
  server.sendContent("");
}


// ─── Wi-Fi access-point configuration ────────────────────────
static String wifiApActiveSsid;
static String wifiApActivePassword;

static void wifiApBuildDefaultSsid(String &ssid) {
  uint8_t mac[6] = {0};
  WiFi.softAPmacAddress(mac);
  char buf[24];
  snprintf(buf, sizeof(buf), "TMR-%02X%02X", mac[4], mac[5]);
  ssid = buf;
}

static bool wifiApTextHasControl(const String &v) {
  for (size_t i = 0; i < v.length(); i++) {
    const uint8_t c = (uint8_t)v[i];
    if (c < 0x20 || c == 0x7F) return true;
  }
  return false;
}

static bool wifiApConfigValid(const String &ssid, const String &password, String *error = nullptr) {
  if (ssid.length() < 1 || ssid.length() > 32) {
    if (error) *error = "SSID must be 1-32 bytes";
    return false;
  }
  if (password.length() < 8 || password.length() > 63) {
    if (error) *error = "password must be 8-63 bytes";
    return false;
  }
  if (wifiApTextHasControl(ssid) || wifiApTextHasControl(password)) {
    if (error) *error = "SSID/password contain control characters";
    return false;
  }
  return true;
}

static void wifiApLoadConfig(String &ssid, String &password) {
  wifiApBuildDefaultSsid(ssid);
  password = "12345678";
  Preferences p;
  if (p.begin("wifiap", true)) {
    const String storedSsid = p.getString("ssid", "");
    const String storedPass = p.getString("pass", "");
    p.end();
    String ignored;
    if (wifiApConfigValid(storedSsid, storedPass, &ignored)) {
      ssid = storedSsid;
      password = storedPass;
    }
  }
}

static bool wifiApPersistConfig(const String &ssid, const String &password) {
  Preferences p;
  if (!p.begin("wifiap", false)) return false;
  const size_t a = p.putString("ssid", ssid);
  const size_t b = p.putString("pass", password);
  p.end();
  return a > 0 && b > 0;
}

static bool wifiApStartOnce(const String &ssid, const String &password) {
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (WiFi.softAP(ssid.c_str(), password.c_str())) {
      wifiApActiveSsid = ssid;
      wifiApActivePassword = password;
      return true;
    }
    TMR_SERIAL_PRINTF("WiFi: AP start failed for SSID=%s attempt=%u/3\n",
                  ssid.c_str(), (unsigned)(attempt + 1));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  return false;
}

static bool wifiApActivate(const String &requestedSsid, const String &requestedPassword, bool allowFallback) {
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  if (wifiApStartOnce(requestedSsid, requestedPassword)) return true;
  if (!allowFallback) return false;

  String fallbackSsid;
  wifiApBuildDefaultSsid(fallbackSsid);
  const String fallbackPassword = "12345678";
  TMR_SERIAL_PRINTLN("WiFi: custom AP failed; restoring fail-safe default AP");
  if (!wifiApStartOnce(fallbackSsid, fallbackPassword)) return false;
  wifiApPersistConfig(fallbackSsid, fallbackPassword);
  return true;
}

static String wifiApJsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); i++) {
    const char c = in[i];
    if (c == '\\' || c == '"') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

static String wifiApStatusJson() {
  String ssid = wifiApActiveSsid;
  String password = wifiApActivePassword;
  if (!ssid.length() || !password.length()) wifiApLoadConfig(ssid, password);
  String j;
  j.reserve(ssid.length() + 120);
  JsonWriterArduino jw(j);
  jw.boolean("ok", true);
  jw.string("ssid", wifiApJsonEscape(ssid));
  jw.boolean("passwordIsDefault", password == "12345678");
  jw.string("ip", WiFi.softAPIP().toString());
  jw.finish();
  return j;
}

static void httpWifiStatus() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", wifiApStatusJson());
}

static void httpWifiApply() {
  if (!server.hasArg("ssid")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing ssid\"}");
    return;
  }
  String ssid = server.arg("ssid");
  String password = server.hasArg("password") ? server.arg("password") : String();
  // A blank password field means preserve the currently configured password.
  // The saved password is never returned by /api/wifi/status.
  if (password.length() == 0) {
    password = wifiApActivePassword;
    if (!password.length()) {
      String currentSsid;
      wifiApLoadConfig(currentSsid, password);
    }
  }
  String err;
  if (!wifiApConfigValid(ssid, password, &err)) {
    server.send(400, "application/json",
                String("{\"ok\":false,\"error\":\"") + wifiApJsonEscape(err) + "\"}");
    return;
  }
  if (!wifiApPersistConfig(ssid, password)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}");
    return;
  }
  const bool changed = (ssid != wifiApActiveSsid) || (password != wifiApActivePassword);
  String resp = String("{\"ok\":true,\"reconnecting\":") + (changed ? "true" : "false") +
                ",\"ssid\":\"" + wifiApJsonEscape(ssid) + "\"}";
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", resp);
  if (!changed) return;

  // Only the SoftAP is recycled. CAN, BLE and the MCU remain running.
  delay(450);  // allow the HTTP response to leave before the client is dropped
  const bool ok = wifiApActivate(ssid, password, true);
  TMR_SERIAL_PRINTF("WiFi: AP config apply %s · SSID=%s IP=%s\n",
                ok ? "OK" : "FAILED", wifiApActiveSsid.c_str(), WiFi.softAPIP().toString().c_str());
}

// ─── S3XY BLE multi-device HTTP ──────────────────────────────
static void httpS3xyStats() {
  server.send(200, "application/json", s3xyStatsToJson());
}

static int s3xyHttpSlotArg() {
  if (!server.hasArg("id")) return -1;
  const int id = server.arg("id").toInt();
  if (id < 1 || id > S3XY_MAX_DEVICES) return -1;
  return id - 1;
}

static bool httpS3xyRequireBluetooth() {
  if (s3xyBluetoothMasterIsEnabled()) return true;
  server.send(409, "application/json", "{\"ok\":false,\"error\":\"bluetooth disabled\"}");
  return false;
}

static void httpS3xyBluetoothEnable() {
  // If an OFF transition is still draining, finish that serialized shutdown
  // before starting a fresh same-boot BLE runtime. Also repair the defensive
  // edge case where the master flag is already OFF but the mapper still exists.
  if (!s3xyBluetoothMasterIsEnabled() && s3xyMapperTaskIsRunning() && !s3xyRuntimeStopIsPending()) {
    s3xyRequestRuntimeStop();
  }
  if (s3xyRuntimeStopIsPending()) {
    if (!s3xyWaitForRuntimeStopped(2500)) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"BLE shutdown still in progress\"}");
      return;
    }
  }

  if (!s3xyBluetoothMasterIsEnabled() && !s3xyBluetoothMasterPersist(true)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}");
    return;
  }
  if (!s3xyTaskHandle) {
    const BaseType_t ret = xTaskCreatePinnedToCore(s3xyMapperTask, "s3xyMap", 8192, nullptr, 1, &s3xyTaskHandle, 0);
    if (ret != pdPASS) {
      s3xyTaskHandle = nullptr;
      s3xyBluetoothMasterPersist(false);
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"BLE task start failed\"}");
      return;
    }
  }

  // Re-arm the exact boot-time reconnect scheduler instead of creating a
  // second runtime reconnect implementation.
  s3xyRuntimeRearmAutoReconnect();
  server.send(200, "application/json", "{\"ok\":true,\"bluetoothEnabled\":true,\"rebooting\":false}");
}

static void httpS3xyBluetoothDisable() {
  if (s3xyBluetoothMasterIsEnabled() && !s3xyBluetoothMasterPersist(false)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}");
    return;
  }

  if (s3xyMapperTaskIsRunning() || s3xyBleInitialized) {
    s3xyRequestRuntimeStop();
    // Keep the HTTP path bounded. Scan stop normally makes this complete well
    // inside the window; if not, shutdown continues in the mapper task.
    const bool stopped = s3xyWaitForRuntimeStopped(1500);
    server.send(stopped ? 200 : 202, "application/json",
                stopped ? "{\"ok\":true,\"bluetoothEnabled\":false,\"rebooting\":false,\"stopping\":false}"
                        : "{\"ok\":true,\"bluetoothEnabled\":false,\"rebooting\":false,\"stopping\":true}");
    return;
  }

  server.send(200, "application/json", "{\"ok\":true,\"bluetoothEnabled\":false,\"rebooting\":false,\"stopping\":false}");
}


static void httpS3xyResetAllBluetooth() {
  if (!httpS3xyRequireBluetooth()) return;
  bool ok = s3xyQueueCommand(S3XY_CMD_RESET_ALL_BLUETOOTH);
  server.send(ok ? 202 : 409, "application/json",
              ok ? "{\"ok\":true,\"action\":\"reset_all_bluetooth\",\"rebooting\":true}"
                 : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyScan() {
  if (!httpS3xyRequireBluetooth()) return;
  if (!s3xyBleInitialized && s3xyRegisteredCount() == 0) {
    // Mapper task initializes BLE when it consumes the command.
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_DISCOVERY_SCAN);
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true,\"action\":\"scan\"}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyPair() {
  if (!httpS3xyRequireBluetooth()) return;
  if (!server.hasArg("address") || !server.arg("address").length()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"address required\"}");
    return;
  }
  String address = server.arg("address");
  if (s3xyFindSlotByAddress(address.c_str()) >= 0) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"already registered\"}");
    return;
  }
  if (s3xyFindFreeSlot() < 0) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"registry full\"}");
    return;
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_PAIR_ADDRESS, -1, address.c_str());
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true,\"action\":\"pair\"}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyDeviceConnect() {
  if (!httpS3xyRequireBluetooth()) return;
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"device not found\"}");
    return;
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_CONNECT_SLOT, (int8_t)slot);
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyDeviceDisconnect() {
  if (!httpS3xyRequireBluetooth()) return;
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"device not found\"}");
    return;
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_DISCONNECT_SLOT, (int8_t)slot);
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyDeviceForget() {
  if (!httpS3xyRequireBluetooth()) return;
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"device not found\"}");
    return;
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_FORGET_SLOT, (int8_t)slot);
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true,\"action\":\"forget\"}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyDeviceHandshake() {
  if (!httpS3xyRequireBluetooth()) return;
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"device not found\"}");
    return;
  }
  bool ok = s3xyQueueCommand(S3XY_CMD_HANDSHAKE_SLOT, (int8_t)slot);
  server.send(ok ? 202 : 409, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"busy\"}");
}

static void httpS3xyDeviceRename() {
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot) || !server.hasArg("name")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"device/name required\"}");
    return;
  }
  String name = server.arg("name");
  name.trim();
  if (!name.length()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"empty name\"}");
    return;
  }
  if (name.length() > 24) name = name.substring(0, 24);
  portENTER_CRITICAL(&s3xyMux);
  strncpy(s3xyDevices[slot].name, name.c_str(), sizeof(s3xyDevices[slot].name) - 1);
  s3xyDevices[slot].name[sizeof(s3xyDevices[slot].name) - 1] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
  s3xyRegistrySaveSlot((uint8_t)slot);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void httpS3xyDeviceAction() {
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot) || !server.hasArg("gesture") || !server.hasArg("action")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"device/gesture/action required\"}");
    return;
  }
  const String gesture = server.arg("gesture");
  uint8_t action = S3XY_ACTION_NONE;
  if (!s3xyActionParseKnown(server.arg("action"), action)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid action\"}");
    return;
  }
  if (!s3xyActionSupportedForCurrentProfile(action)) {
    String msg = "{\"ok\":false,\"error\":\"";
    msg += s3xyActionLabel(action);
    msg += " unavailable for current CAN topology\"}";
    server.send(409, "application/json", msg);
    return;
  }
  bool validGesture = true;
  portENTER_CRITICAL(&s3xyMux);
  if (gesture == "single") s3xyDevices[slot].singleAction = action;
  else if (gesture == "double") s3xyDevices[slot].doubleAction = action;
  else if (gesture == "long") s3xyDevices[slot].longAction = action;
  else validGesture = false;
  portEXIT_CRITICAL(&s3xyMux);
  if (!validGesture) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid gesture\"}");
    return;
  }
  s3xyRegistrySaveSlot((uint8_t)slot);
  server.send(200, "application/json", "{\"ok\":true}");
}

static void httpS3xyDeviceAuto() {
  const int slot = s3xyHttpSlotArg();
  if (slot < 0 || !s3xySlotIsUsed(slot) || !server.hasArg("enabled")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"device/enabled required\"}");
    return;
  }
  const bool enabled = server.arg("enabled") == "1" || server.arg("enabled") == "true" || server.arg("enabled") == "on";
  const uint32_t now = millis();
  portENTER_CRITICAL(&s3xyMux);
  s3xyDevices[slot].autoConnect = enabled;
  if (enabled) {
    s3xyDevices[slot].manualPaused = false;
    const bool schedule = s3xyBluetoothEnabled && s3xyAutoEnabled && !s3xyDevices[slot].connected;
    s3xyDevices[slot].nextAttemptMs = schedule ? (now + 250U) : 0;
  } else {
    s3xyDevices[slot].nextAttemptMs = 0;
  }
  portEXIT_CRITICAL(&s3xyMux);
  s3xyRegistrySaveSlot((uint8_t)slot);
  s3xyLogPush(S3XY_LOG_INFO, enabled ? "device auto-connect enabled" : "device auto-connect disabled", nullptr, 0, -127, (int8_t)slot);
  server.send(200, "application/json", enabled ? "{\"ok\":true,\"autoConnect\":true}" : "{\"ok\":true,\"autoConnect\":false}");
}

static void httpS3xyAutoEnable() {
  if (!httpS3xyRequireBluetooth()) return;
  s3xyAutoSetEnabled(true, true);
  server.send(200, "application/json", "{\"ok\":true,\"autoEnabled\":true}");
}

static void httpS3xyAutoDisable() {
  if (!httpS3xyRequireBluetooth()) return;
  s3xyAutoSetEnabled(false, true);
  server.send(200, "application/json", "{\"ok\":true,\"autoEnabled\":false}");
}

static void httpS3xyClear() {
#if S3XY_DIAGNOSTICS_ENABLED
  s3xyClearLog();
  server.send(200, "application/json", "{\"ok\":true}");
#else
  server.send(404, "application/json", "{\"ok\":false,\"error\":\"S3XY diagnostics disabled in this build\"}");
#endif
}

static void httpS3xyLogCsv() {
#if S3XY_DIAGNOSTICS_ENABLED
  server.sendHeader("Content-Disposition", "attachment; filename=TMR_S3XY_multi.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent("seq,ms,slot,type,rssi,len,data_hex,detail\n", sizeof("seq,ms,slot,type,rssi,len,data_hex,detail\n") - 1);

  uint16_t count, head;
  uint32_t dropped;
  portENTER_CRITICAL(&s3xyMux);
  count = s3xyLogCount;
  head = s3xyLogHead;
  dropped = s3xyLogDropped;
  portEXIT_CRITICAL(&s3xyMux);

  const uint16_t start = (uint16_t)((head + S3XY_LOG_MAX - count) % S3XY_LOG_MAX);
  for (uint16_t i = 0; i < count; i++) {
    S3xyLogEntry e;
    const uint16_t idx = (uint16_t)((start + i) % S3XY_LOG_MAX);
    portENTER_CRITICAL(&s3xyMux);
    e = s3xyLog[idx];
    portEXIT_CRITICAL(&s3xyMux);

    char hex[3 * S3XY_LOG_DATA_MAX + 1] = {};
    char detailEsc[sizeof(e.detail) * 2 + 1] = {};
    char line[320] = {};
    s3xyBytesToHex(e.data, e.len, hex, sizeof(hex));
    csvEscapeField(e.detail, detailEsc, sizeof(detailEsc));
    snprintf(line, sizeof(line), "%u,%lu,%d,%s,%d,%u,\"%s\",\"%s\"\n",
             (unsigned)i, (unsigned long)e.ms, (int)e.slot + 1,
             s3xyLogTypeName(e.type), (int)e.rssi, (unsigned)e.len,
             hex, detailEsc);
    server.sendContent(line, strlen(line));
    if ((i & 0x0F) == 0x0F) vTaskDelay(1);
  }
  uint32_t exactHits, exactMisses, linkFails;
  char autoTrace[112];
  portENTER_CRITICAL(&s3xyMux);
  exactHits = s3xyAutoExactHits;
  exactMisses = s3xyAutoExactMisses;
  linkFails = s3xyAutoLinkFailures;
  strncpy(autoTrace, s3xyAutoTrace, sizeof(autoTrace) - 1); autoTrace[sizeof(autoTrace) - 1] = '\0';
  portEXIT_CRITICAL(&s3xyMux);
  char tail[280];
  snprintf(tail, sizeof(tail), "# dropped=%lu,auto_exact_hits=%lu,auto_exact_misses=%lu,auto_link_failures=%lu,auto_trace=%s\n",
           (unsigned long)dropped, (unsigned long)exactHits, (unsigned long)exactMisses,
           (unsigned long)linkFails, autoTrace);
  server.sendContent(tail, strlen(tail));
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    bool used, verified; uint8_t idLen; char addr[24], idHex[64], autoPath[24];
    uint32_t discoverMs, connectMs, readyMs;
    portENTER_CRITICAL(&s3xyMux);
    used = s3xyDevices[i].used; verified = s3xyDevices[i].identityPersistVerified; idLen = s3xyDevices[i].peerIdLen;
    discoverMs = s3xyDevices[i].lastDiscoverMs; connectMs = s3xyDevices[i].lastConnectMs; readyMs = s3xyDevices[i].lastReadyMs;
    strncpy(addr, s3xyDevices[i].address, sizeof(addr)-1); addr[sizeof(addr)-1] = '\0';
    strncpy(idHex, s3xyDevices[i].idHex, sizeof(idHex)-1); idHex[sizeof(idHex)-1] = '\0';
    strncpy(autoPath, s3xyDevices[i].lastAutoPath, sizeof(autoPath)-1); autoPath[sizeof(autoPath)-1] = '\0';
    portEXIT_CRITICAL(&s3xyMux);
    if (!used) continue;
    snprintf(tail, sizeof(tail), "# slot=%u,address=%s,peer_id_len=%u,nvs_verified=%u,peer_id=%s,last_auto_path=%s,discover_ms=%lu,connect_ms=%lu,ready_ms=%lu\n",
             (unsigned)(i + 1), addr, (unsigned)idLen, verified ? 1U : 0U, idHex,
             autoPath[0] ? autoPath : "NONE", (unsigned long)discoverMs, (unsigned long)connectMs, (unsigned long)readyMs);
    server.sendContent(tail, strlen(tail));
  }
  server.sendContent("", 0);

#else
  server.send(404, "application/json", "{\"ok\":false,\"error\":\"S3XY diagnostics disabled in this build\"}");
#endif
}


// ─── OTA update ─────────────────────────────────────────────

static void httpOtaUpload() {
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        otaInProgress = true;
        otaSuccess    = false;
        otaError      = false;
        otaBytes      = 0;
        otaErrMsg[0]  = '\0';
        TMR_SERIAL_PRINTF("[OTA] Start: %s\n", up.filename.c_str());

        if (!prepareCanForMaintenance()) {
            otaError = true;
            strncpy(otaErrMsg, "CAN stop not confirmed; OTA refused. Reboot required.", sizeof(otaErrMsg) - 1);
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            TMR_SERIAL_PRINTF("[OTA] begin() failed: %s\n", otaErrMsg);
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (!otaError && Update.write(up.buf, up.currentSize) != up.currentSize) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            TMR_SERIAL_PRINTF("[OTA] write() failed: %s\n", otaErrMsg);
        }
        otaBytes += up.currentSize;
    } else if (up.status == UPLOAD_FILE_END) {
        if (!otaError && Update.end(true)) {
            otaSuccess = true;
            otaTotal   = otaBytes;
            TMR_SERIAL_PRINTF("[OTA] Success: %u bytes\n", up.totalSize);
        } else if (!otaError) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            TMR_SERIAL_PRINTF("[OTA] end() failed: %s\n", otaErrMsg);
        }
        otaInProgress = false;
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
        otaInProgress = false;
        otaError      = true;
        strncpy(otaErrMsg, "aborted", sizeof(otaErrMsg) - 1);
        TMR_SERIAL_PRINTLN("[OTA] Aborted");
    }
}

static void httpOtaFinish() {
    bool ok = otaSuccess && !otaError;
    String resp = String("{\"ok\":") + (ok ? "true" : "false") +
                  ",\"error\":\"" + String(otaErrMsg) + "\"}";
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", resp);
    if (ok) {
        delay(700);
        restartTmrCanSafely();
    }
}

static uint8_t researchCaptureParseSlot(const String &raw) {
  String s = raw;
  s.trim();
  s.toUpperCase();
  if (s == "A" || s == "0") return RESEARCH_CAPTURE_LABEL_A;
  if (s == "B" || s == "1") return RESEARCH_CAPTURE_LABEL_B;
  if (s == "C" || s == "2") return RESEARCH_CAPTURE_LABEL_C;
  if (s == "D" || s == "3") return RESEARCH_CAPTURE_LABEL_D;
  return RESEARCH_CAPTURE_LABEL_NONE;
}

static String researchCaptureJsonEscape(const char *in) {
  String out;
  if (!in) return out;
  out.reserve(strlen(in) + 8);
  for (const uint8_t *p = (const uint8_t *)in; *p; ++p) {
    const uint8_t c = *p;
    if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else if (c >= 0x20) out += (char)c;
  }
  return out;
}

static const char *researchCapturePhysicalBusName(uint8_t bus) {
  return bus == RESEARCH_CAPTURE_BUS_PARTY ? "CAN A" :
         bus == RESEARCH_CAPTURE_BUS_VH ? "CAN B" : "UNKNOWN";
}

static const char *researchCaptureProfileBusName(uint8_t bus) {
  return bus == RESEARCH_CAPTURE_BUS_PARTY ? activeProfileCanAName() :
         bus == RESEARCH_CAPTURE_BUS_VH ? activeProfileCanBName() : "UNKNOWN";
}

static String researchCaptureStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t state, mode, currentSlot, lastSlot, preValid, nextPost, lastPre, lastPost, postCount;
  uint16_t segment, completed, knownIds, triggerRows;
  uint32_t count, start, requests, ignored, dropped, preWindowMs, postWindowMs, extendedPreIntervalMs;
  uint32_t rawPreStartIndex, rawPreFrameCount, rawArchiveCount, rawTriggerMs, rawPostDeadlineMs, rawEvicted, rawPreCoverageMs;
  uint32_t autoQualifiedTransitions, autoTriggerCount, autoRejectLane, autoRejectWarmup, autoRejectBusy, autoMergedEvents;
  uint32_t autoOpenCount, autoBlockedCount, autoLastTriggerMs;
  uint8_t autoLastEvent, autoLastFrom, autoLastTo;
  bool rawTriggered;
  bool exporting;
  bool psram;
  char labels[RESEARCH_CAPTURE_LABEL_SLOTS][RESEARCH_CAPTURE_LABEL_BYTES] = {};
  char currentLabel[RESEARCH_CAPTURE_LABEL_BYTES] = {};
  char lastLabel[RESEARCH_CAPTURE_LABEL_BYTES] = {};
  ResearchCaptureLatest lane239 = {};
  ResearchCaptureLatest alc399 = {};

  portENTER_CRITICAL(&researchCaptureMux);
  state = researchCaptureState;
  mode = researchCaptureMode;
  count = researchCaptureCount;
  segment = researchCaptureSegment;
  completed = researchCaptureCompletedSegments;
  knownIds = researchCaptureKnownIds;
  currentSlot = researchCaptureCurrentLabelSlot;
  lastSlot = researchCaptureLastLabelSlot;
  preValid = (uint8_t)(researchCapturePreValidCount + researchCapturePreExtValidCount);
  preWindowMs = researchCapturePreWindowMs;
  postWindowMs = researchCapturePostWindowMs;
  extendedPreIntervalMs = researchCaptureExtendedIntervalMs(preWindowMs);
  postCount = researchCapturePostCountForWindow(postWindowMs);
  nextPost = researchCaptureNextPostIndex;
  lastPre = researchCaptureLastPreSnapshotsCopied;
  lastPost = researchCaptureLastPostSnapshotsCopied;
  triggerRows = researchCaptureLastTriggerRows;
  start = researchCaptureStartMs;
  requests = researchCaptureRequests;
  ignored = researchCaptureIgnored;
  dropped = researchCaptureDropped;
  rawPreStartIndex = researchCaptureRawPreStartIndex;
  rawPreFrameCount = researchCaptureRawPreFrameCount;
  rawArchiveCount = researchCaptureRawArchiveCount;
  rawTriggerMs = researchCaptureRawTriggerMs;
  rawPostDeadlineMs = researchCaptureRawPostDeadlineMs;
  rawEvicted = researchCaptureRawEvicted;
  rawPreCoverageMs = researchCaptureRawPreCoverageMsLocked(now);
  rawTriggered = researchCaptureRawTriggered;
  exporting = researchCaptureExporting;
  autoQualifiedTransitions = researchCaptureAutoQualifiedTransitions;
  autoTriggerCount = researchCaptureAutoTriggerCount;
  autoRejectLane = researchCaptureAutoRejectLane;
  autoRejectWarmup = researchCaptureAutoRejectWarmup;
  autoRejectBusy = researchCaptureAutoRejectBusy;
  autoMergedEvents = researchCaptureAutoMergedEvents;
  autoOpenCount = researchCaptureAutoOpenCount;
  autoBlockedCount = researchCaptureAutoBlockedCount;
  autoLastTriggerMs = researchCaptureAutoLastTriggerMs;
  autoLastEvent = researchCaptureAutoLastEvent;
  autoLastFrom = researchCaptureAutoLastFrom;
  autoLastTo = researchCaptureAutoLastTo;
  psram = researchCaptureUsingPsram;
  memcpy(labels, researchCaptureLabels, sizeof(labels));
  if (researchCaptureLatest) {
    lane239 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x239)];
    alc399 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x399)];
  }
  if (segment > 0 && segment <= RESEARCH_CAPTURE_MAX_SEGMENTS) {
    const ResearchCaptureSegmentMeta &meta = researchCaptureSegments[segment - 1];
    strncpy(lastLabel, meta.label, sizeof(lastLabel) - 1);
    if (state == RESEARCH_CAPTURE_CAPTURING) strncpy(currentLabel, meta.label, sizeof(currentLabel) - 1);
  }
  portEXIT_CRITICAL(&researchCaptureMux);

  const bool laneValid = lane239.valid && lane239.dlc >= 7;
  const bool alcValid = alc399.valid && alc399.dlc >= 7;
  const DasLane239Decoded laneDecoded = dasLane239DecodePure(lane239.data, lane239.dlc);
  const uint8_t alcRaw = alcValid ? das399ReadAlcPure(alc399.data, alc399.dlc) : 0xFF;
  const uint8_t leftLaneExists = laneDecoded.valid ? (laneDecoded.leftLaneExists ? 1 : 0) : 0xFF;
  const uint8_t rightLaneExists = laneDecoded.valid ? (laneDecoded.rightLaneExists ? 1 : 0) : 0xFF;
  const uint8_t leftLineUsage = laneDecoded.valid ? laneDecoded.leftLineUsage : 0xFF;
  const uint8_t rightLineUsage = laneDecoded.valid ? laneDecoded.rightLineUsage : 0xFF;
  const uint8_t leftFork = laneDecoded.valid ? laneDecoded.leftFork : 0xFF;
  const uint8_t rightFork = laneDecoded.valid ? laneDecoded.rightFork : 0xFF;
  const uint32_t laneAge = lane239.valid ? (uint32_t)(now - lane239.lastSeenMs) : 999999UL;
  const uint32_t alcAge = alc399.valid ? (uint32_t)(now - alc399.lastSeenMs) : 999999UL;

  // Reference decode for legacy/public Tesla DAS_lanes geometry. YL mapping is being validated empirically.
  const int32_t virtualLaneWidthX10000 = laneValid ? (20000 + 3125 * (int32_t)((lane239.data[0] >> 4) & 0x0FU)) : 0;
  const uint16_t laneViewRange = laneValid ? (uint16_t)lane239.data[1] : 0U;
  const int32_t virtualLaneC0X10000 = laneValid ? (-35000 + 350 * (int32_t)lane239.data[2]) : 0;
  const int32_t virtualLaneC1X100000 = laneValid ? (-20000 + 160 * (int32_t)lane239.data[3]) : 0;
  const int32_t virtualLaneC2X1000000 = laneValid ? (-2500 + 20 * (int32_t)lane239.data[4]) : 0;
  const bool rawMode = researchCaptureModeIsRaw(mode);
  const uint32_t rawRequiredPreMs = mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC
      ? RESEARCH_CAPTURE_RAW_AUTO_PRE_MS : researchCaptureRawManualPreMsForMode(mode);
  const uint32_t rawPostMs = researchCaptureRawPostMsForMode(mode);
  const bool rawPreReady = rawPreCoverageMs >= rawRequiredPreMs;
  const bool rawManualPreReady = rawPreCoverageMs >= researchCaptureRawManualPreMsForMode(mode);

  uint32_t remaining = 0;
  if (state == RESEARCH_CAPTURE_CAPTURING) {
    if (rawMode && rawPostDeadlineMs != 0) {
      remaining = (int32_t)(rawPostDeadlineMs - now) > 0 ? (uint32_t)(rawPostDeadlineMs - now) : 0U;
    } else if (!rawMode && start != 0) {
      const uint32_t elapsed = (uint32_t)(now - start);
      const uint32_t postEnd = postCount ? RESEARCH_CAPTURE_POST_TARGETS[postCount - 1U] : 0U;
      remaining = elapsed >= postEnd ? 0 : (postEnd - elapsed);
    }
  }
  const uint32_t rawRingUsagePermille = RESEARCH_CAPTURE_RAW_PRE_CAPACITY
      ? ((uint32_t)rawPreFrameCount * 1000u + RESEARCH_CAPTURE_RAW_PRE_CAPACITY / 2u) / RESEARCH_CAPTURE_RAW_PRE_CAPACITY : 0u;
  const uint32_t rawArchiveUsagePermille = RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY
      ? ((uint32_t)rawArchiveCount * 1000u + RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY / 2u) / RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY : 0u;
  size_t allocatedMainBytes, allocatedAuxBytes, allocatedCommonBytes;
  portENTER_CRITICAL(&researchCaptureMux);
  allocatedMainBytes = researchCaptureAllocatedMainBytes;
  allocatedAuxBytes = researchCaptureAllocatedAuxBytes;
  allocatedCommonBytes = (researchCaptureLatest ? RESEARCH_CAPTURE_LATEST_BYTES : 0U)
                       + (researchCaptureKnownIndices ? RESEARCH_CAPTURE_KNOWN_BYTES : 0U);
  portEXIT_CRITICAL(&researchCaptureMux);
  const size_t memoryBytes = allocatedMainBytes + allocatedAuxBytes + allocatedCommonBytes;
  const size_t psramTotalBytes = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  const size_t psramFreeBytes = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const uint32_t effectiveCapacity = rawMode ? RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY : RESEARCH_CAPTURE_CAPACITY;
  const uint8_t effectiveMaxSegments = rawMode ? RESEARCH_CAPTURE_RAW_MAX_SEGMENTS : RESEARCH_CAPTURE_MAX_SEGMENTS;
  const char *fullReason = "NONE";
  if (state == RESEARCH_CAPTURE_FULL) {
    if (rawMode && rawArchiveCount >= RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) fullReason = "ARCHIVE";
    else if (segment >= effectiveMaxSegments) fullReason = "SEGMENTS";
    else fullReason = rawMode ? "ARCHIVE" : "SAMPLES";
  }

  String j; j.reserve(1440);
  j = "{\"diagnosticOnly\":true,\"captureGeneratesTx\":false,\"recordsExisting3f8Tx\":true,\"recordsExisting399Tx\":true";
  JsonWriterArduino jw(j, true);
  jw.string("state", researchCaptureStateName(state));
  jw.string("fullReason", fullReason);
  jw.boolean("exporting", exporting);
  jw.string("captureMode", researchCaptureModeName(mode));
  jw.string("canAName", activeProfileCanAName());
  jw.string("canBName", activeProfileCanBName());
  jw.string("canCName", activeProfileCanCName());
  jw.boolean("rawAutoAlcSupported", activeProfileIsYl());
  jw.boolean("capturing", state == RESEARCH_CAPTURE_CAPTURING);
  jw.u32("segment", (uint32_t)(segment));
  jw.u32("completedSegments", (uint32_t)(completed));
  jw.string("currentSlot", researchCaptureLabelSlotName(currentSlot));
  jw.string("lastSlot", researchCaptureLabelSlotName(lastSlot));
  jw.string("currentLabel", researchCaptureJsonEscape(currentLabel));
  jw.string("lastLabel", researchCaptureJsonEscape(lastLabel));
  jw.beginObject("labels");
  for (uint8_t i = 0; i < RESEARCH_CAPTURE_LABEL_SLOTS; i++) {
    if (i) j += ',';
    j += "\"" + String(researchCaptureLabelSlotName(i)) + "\":\"" + researchCaptureJsonEscape(labels[i]) + "\"";
  }
  jw.endObject();
  jw.u32("samples", (uint32_t)(count));
  jw.u32("capacity", (uint32_t)(effectiveCapacity));
  jw.u32("maxSegments", (uint32_t)(effectiveMaxSegments));
  jw.u32("knownIds", (uint32_t)(knownIds));
  jw.u32("preSnapshotsReady", (uint32_t)(preValid));
  jw.u32("preSnapshotSlots", (uint32_t)(RESEARCH_CAPTURE_PRE_SLOT_COUNT));
  jw.u32("lastPreSnapshotsCopied", (uint32_t)(lastPre));
  jw.u32("lastTriggerRows", (uint32_t)(triggerRows));
  jw.u32("lastPostSnapshotsCopied", (uint32_t)(lastPost));
  jw.u32("nextPostIndex", (uint32_t)(nextPost));
  jw.u32("remainingMs", (uint32_t)(remaining));
  jw.u32("preMs", preWindowMs); // backward-compatible alias
  jw.u32("postMs", postWindowMs); // backward-compatible alias
  jw.u32("preWindowMs", (uint32_t)(preWindowMs));
  jw.u32("postWindowMs", (uint32_t)(postWindowMs));
  jw.u32("preDenseIntervalMs", (uint32_t)(RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS));
  jw.u32("preExtendedIntervalMs", (uint32_t)(extendedPreIntervalMs));
  jw.u32("postSnapshotCount", (uint32_t)(postCount));
  jw.u32("requests", (uint32_t)(requests));
  jw.u32("ignored", (uint32_t)(ignored));
  jw.u32("dropped", (uint32_t)(dropped));
  jw.boolean("rawTriggered", rawTriggered);
  jw.u32("rawTriggerMs", (uint32_t)(rawTriggerMs));
  jw.u32("rawRingStartIndex", (uint32_t)(rawPreStartIndex));
  jw.u32("rawRingFrames", (uint32_t)(rawPreFrameCount));
  jw.u32("rawRingCapacity", (uint32_t)(RESEARCH_CAPTURE_RAW_PRE_CAPACITY));
  jw.fixed("rawRingUsagePct", (int32_t)rawRingUsagePermille, 1u);
  jw.u32("rawArchiveFrames", (uint32_t)(rawArchiveCount));
  jw.u32("rawArchiveCapacity", (uint32_t)(RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY));
  jw.fixed("rawArchiveUsagePct", (int32_t)rawArchiveUsagePermille, 1u);
  jw.u32("rawEvicted", (uint32_t)(rawEvicted));
  jw.u32("rawPreCoverageMs", (uint32_t)(rawPreCoverageMs));
  jw.boolean("rawPreReady", rawPreReady);
  jw.boolean("rawManualPreReady", rawManualPreReady);
  jw.u32("autoQualifiedTransitions", (uint32_t)(autoQualifiedTransitions));
  jw.u32("autoTriggerCount", (uint32_t)(autoTriggerCount));
  jw.u32("autoRejectLane", (uint32_t)(autoRejectLane));
  jw.u32("autoRejectWarmup", (uint32_t)(autoRejectWarmup));
  jw.u32("autoRejectBusy", (uint32_t)(autoRejectBusy));
  jw.u32("autoMergedEvents", (uint32_t)(autoMergedEvents));
  jw.u32("autoOpenCount", (uint32_t)(autoOpenCount));
  jw.u32("autoBlockedCount", (uint32_t)(autoBlockedCount));
  jw.u32("autoLastTriggerAgeMs", (uint32_t)((autoLastTriggerMs ? now - autoLastTriggerMs : 999999UL)));
  jw.string("autoLastEvent", researchCaptureAutoEventName(autoLastEvent));
  jw.u32("autoLastFrom", (uint32_t)(autoLastFrom));
  jw.u32("autoLastTo", (uint32_t)(autoLastTo));
  jw.string("autoLastFromName", autoLastFrom == 0xFF ? "NONE" : alcStateName(autoLastFrom));
  jw.string("autoLastToName", autoLastTo == 0xFF ? "NONE" : alcStateName(autoLastTo));
  jw.u32("rawPreMs", (uint32_t)(rawRequiredPreMs));
  jw.u32("rawPostMs", (uint32_t)(rawPostMs));
  jw.boolean("usingPsram", psram);
  jw.u32("memoryBytes", (uint32_t)(memoryBytes));
  jw.u32("psramTotalBytes", (uint32_t)(psramTotalBytes));
  jw.u32("psramFreeBytes", (uint32_t)(psramFreeBytes));
  jw.string("snapshotPlan", rawMode
      ? (mode == RESEARCH_CAPTURE_MODE_ULC_CONFIRM
          ? "ULC CONFIRM targeted RX · PRE 3s + trigger + POST 7s · 6 IDs · CAN A + CAN B"
          : mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC
            ? "RAW AUTO LEFT OPEN 6/8 ↔ BLOCKED other state · PRE 2s + POST 2s · manual C/D PRE 5s + POST 2s"
            : "RAW RX ring PRE 5s + trigger + POST 2s · auto re-arm")
      : "rolling snapshot PRE + trigger + selected POST");
  jw.boolean("alcValid", alcValid);
  jw.u32("alcRaw", (uint32_t)(alcRaw));
  jw.string("alcName", alcValid ? alcStateName(alcRaw) : "NO DATA");
  jw.u32("alcAgeMs", (uint32_t)(alcAge));
  jw.boolean("lane239Valid", laneValid);
  jw.u32("lane239AgeMs", (uint32_t)(laneAge));
  jw.u32("leftLaneExists", (uint32_t)(leftLaneExists));
  jw.u32("rightLaneExists", (uint32_t)(rightLaneExists));
  jw.u32("leftLineUsageRaw", (uint32_t)(leftLineUsage));
  jw.u32("rightLineUsageRaw", (uint32_t)(rightLineUsage));
  jw.u32("leftForkRaw", (uint32_t)(leftFork));
  jw.u32("rightForkRaw", (uint32_t)(rightFork));
  jw.fixed("virtualLaneWidth", virtualLaneWidthX10000, 4u);
  jw.u32("laneViewRange", (uint32_t)(laneViewRange));
  jw.fixed("virtualLaneC0", virtualLaneC0X10000, 4u);
  jw.fixed("virtualLaneC1", virtualLaneC1X100000, 5u);
  jw.fixed("virtualLaneC2", virtualLaneC2X1000000, 6u);
  jw.finish();
  return j;
}

static void httpResearchCaptureStats() {
  server.send(200, "application/json", researchCaptureStatsToJson());
}

static void httpResearchCaptureStart() {
  if (!httpRequireLab()) return;
  const uint8_t slot = server.hasArg("slot") ? researchCaptureParseSlot(server.arg("slot")) : RESEARCH_CAPTURE_LABEL_NONE;
  if (slot >= RESEARCH_CAPTURE_LABEL_SLOTS) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"slot must be A, B, C or D\"}");
    return;
  }
  const bool ok = researchCaptureRequest(slot);
  server.send(ok ? 202 : 409, "application/json", researchCaptureStatsToJson());
}

static void httpResearchCaptureLabels() {
  if (!httpRequireLab()) return;
  if (!server.hasArg("slot") || !server.hasArg("label")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"slot and label are required\"}");
    return;
  }
  const uint8_t slot = researchCaptureParseSlot(server.arg("slot"));
  const String label = server.arg("label");
  if (slot >= RESEARCH_CAPTURE_LABEL_SLOTS || !researchCaptureSetLabel(slot, label)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"label must be 1-63 UTF-8 bytes\"}");
    return;
  }
  server.send(200, "application/json", researchCaptureStatsToJson());
}


static void httpResearchCaptureConfig() {
  if (!httpRequireLab()) return;
  if (!server.hasArg("preMs") || !server.hasArg("postMs")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"preMs and postMs are required\"}");
    return;
  }
  const uint32_t preMs = (uint32_t)server.arg("preMs").toInt();
  const uint32_t postMs = (uint32_t)server.arg("postMs").toInt();
  if (!researchCapturePreWindowSupported(preMs) || !researchCapturePostWindowSupported(postMs)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"PRE must be 2000/5000/10000/15000 ms and POST 2000/5000/10000 ms\"}");
    return;
  }
  if (!researchCaptureSetConfig(preMs, postMs)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"capture busy or NVS write failed\"}");
    return;
  }
  server.send(200, "application/json", researchCaptureStatsToJson());
}

static void httpResearchCaptureMode() {
  if (!httpRequireLab()) return;
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode is required\"}");
    return;
  }
  String raw = server.arg("mode");
  raw.trim(); raw.toUpperCase();
  uint8_t mode = 0xFF;
  if (raw == "SNAPSHOT" || raw == "0") mode = RESEARCH_CAPTURE_MODE_SNAPSHOT;
  else if (raw == "RAW_TRANSITION" || raw == "RAW" || raw == "1") mode = RESEARCH_CAPTURE_MODE_RAW_TRANSITION;
  if (mode == 0xFF) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode must be SNAPSHOT or RAW_TRANSITION\"}");
    return;
  }
  if (!researchCaptureSetMode(mode)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"capture busy or NVS write failed\"}");
    return;
  }
  server.send(200, "application/json", researchCaptureStatsToJson());
}

static void httpResearchCaptureReset() {
  if (!httpRequireLab()) return;
  portENTER_CRITICAL(&researchCaptureMux);
  const bool exporting = researchCaptureExporting;
  portEXIT_CRITICAL(&researchCaptureMux);
  if (exporting) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"CSV export active\"}");
    return;
  }
  researchCaptureReset();
  server.send(200, "application/json", researchCaptureStatsToJson());
}

static void researchCaptureCsvHex(const uint8_t *data, uint8_t dlc, char *out, size_t outLen) {
  if (!out || outLen == 0) return;
  out[0] = '\0';
  const uint8_t n = dlc > 8 ? 8 : dlc;
  size_t pos = 0;
  for (uint8_t i = 0; i < n && pos + 4 < outLen; i++) {
    const int w = snprintf(out + pos, outLen - pos, "%s%02X", i ? " " : "", data[i]);
    if (w <= 0) break;
    pos += (size_t)w;
  }
}

static void researchCaptureCsvQuote(const char *in, char *out, size_t outLen) {
  if (!out || outLen < 3) return;
  size_t pos = 0;
  out[pos++] = '"';
  for (const char *p = in ? in : ""; *p && pos + 3 < outLen; ++p) {
    if (*p == '"') out[pos++] = '"';
    out[pos++] = *p;
  }
  out[pos++] = '"';
  out[pos] = '\0';
}

static constexpr size_t RESEARCH_CAPTURE_CSV_CHUNK_BYTES = 8192;

static inline void researchCaptureCsvFlush(String &chunk) {
  if (!chunk.length()) return;
  server.sendContent(chunk.c_str(), chunk.length());
  chunk.remove(0);
}

static inline void researchCaptureCsvAppend(String &chunk, const char *line) {
  if (!line || !line[0]) return;
  const size_t len = strlen(line);
  if (chunk.length() && chunk.length() + len > RESEARCH_CAPTURE_CSV_CHUNK_BYTES) {
    researchCaptureCsvFlush(chunk);
  }
  if (len >= RESEARCH_CAPTURE_CSV_CHUNK_BYTES) {
    server.sendContent(line, len);
    return;
  }
  chunk.concat(line, len);
}

static inline void researchCaptureCsvEndExport() {
  portENTER_CRITICAL(&researchCaptureMux);
  researchCaptureExporting = false;
  portEXIT_CRITICAL(&researchCaptureMux);
}

static uint32_t researchCaptureCsvExportRows(uint8_t mode, uint32_t count, uint16_t rawCompleted,
                                             const ResearchCaptureEntry *entries,
                                             const ResearchCaptureRawEntry *archive) {
  if (researchCaptureModeIsRaw(mode)) {
    if (!archive || rawCompleted == 0) return 0;
    const uint16_t exportSegments = rawCompleted > RESEARCH_CAPTURE_RAW_MAX_SEGMENTS
        ? RESEARCH_CAPTURE_RAW_MAX_SEGMENTS : rawCompleted;
    uint32_t rows = 0;
    for (uint16_t seg = 1; seg <= exportSegments; seg++) {
      const ResearchCaptureSegmentMeta meta = researchCaptureSegments[seg - 1U];
      if (meta.rawFrameCount == 0) continue;
      const uint32_t end = meta.rawStartIndex + meta.rawFrameCount;
      if (end > RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) continue;
      rows += meta.rawFrameCount;
    }
    return rows;
  }

  if (!entries) return 0;
  uint32_t rows = 0;
  for (uint32_t i = 0; i < count; i++) {
    const uint16_t segment = entries[i].segment;
    if (segment != 0 && segment <= RESEARCH_CAPTURE_MAX_SEGMENTS) rows++;
  }
  return rows;
}

static void httpResearchCaptureCsv() {
  uint8_t state, mode;
  uint32_t count;
  uint16_t rawCompleted;
  ResearchCaptureEntry *entries;
  ResearchCaptureRawEntry *archive;

  // Freeze only archive-mutating operations while exporting. RX observation and
  // the independent RAW PRE ring continue running; completed archive rows are
  // immutable until this flag is cleared.
  portENTER_CRITICAL(&researchCaptureMux);
  state = researchCaptureState;
  if (state == RESEARCH_CAPTURE_CAPTURING || researchCaptureExporting) {
    portEXIT_CRITICAL(&researchCaptureMux);
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"capture or CSV export still active\"}");
    return;
  }
  researchCaptureExporting = true;
  mode = researchCaptureMode;
  count = researchCaptureCount;
  rawCompleted = researchCaptureCompletedSegments;
  entries = researchCaptureEntries;
  archive = researchCaptureRawArchiveBase();
  portEXIT_CRITICAL(&researchCaptureMux);

  const uint32_t exportRows = researchCaptureCsvExportRows(mode, count, rawCompleted, entries, archive);
  server.sendHeader("X-TMR-CSV-Rows", String((unsigned long)exportRows));
  server.sendHeader("Content-Disposition", "attachment; filename=CAN_Research_Capture.csv");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent("segment_id,label_slot,label,phase,relative_ms,trigger_uptime_ms,snapshot_uptime_ms,frame_age_ms,physical_bus,bus_role,source,tx_ok,id,dlc,raw\n");

  String chunk;
  chunk.reserve(RESEARCH_CAPTURE_CSV_CHUNK_BYTES + 384U);
  char line[640];

  if (researchCaptureModeIsRaw(mode)) {
    if (rawCompleted == 0 || !archive) {
      server.sendContent("# RAW capture has no completed segment yet\n");
      server.sendContent("", 0);
      researchCaptureCsvEndExport();
      return;
    }

    const uint16_t exportSegments = rawCompleted > RESEARCH_CAPTURE_RAW_MAX_SEGMENTS
        ? RESEARCH_CAPTURE_RAW_MAX_SEGMENTS : rawCompleted;
    for (uint16_t seg = 1; seg <= exportSegments; seg++) {
      // Completed segment metadata and archive rows cannot be mutated while
      // researchCaptureExporting is true, so no per-row critical section is needed.
      const ResearchCaptureSegmentMeta meta = researchCaptureSegments[seg - 1U];
      if (meta.rawFrameCount == 0) continue;

      char quotedLabel[(RESEARCH_CAPTURE_LABEL_BYTES * 2) + 4];
      researchCaptureCsvQuote(meta.label, quotedLabel, sizeof(quotedLabel));
      const uint32_t end = meta.rawStartIndex + meta.rawFrameCount;
      if (end > RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) continue;

      snprintf(line, sizeof(line),
               "#segment_summary,segment_id=%u,label_slot=%s,label=%s,trigger_alc_from=%d,trigger_alc_to=%d,left_lane_exists=%d,left_line_usage=%d,right_lane_exists=%d,das_state=%d,road_class=%d,gps_road_match=%d,nav_route_active=%d,controlled_access=%d,left_off_ramp=%d,right_off_ramp=%d,road_age_ms=%ld\n",
               (unsigned)seg, researchCaptureLabelSlotName(meta.labelSlot), quotedLabel,
               meta.triggerAlcFrom == 0xFF ? -1 : (int)meta.triggerAlcFrom,
               meta.triggerAlcTo == 0xFF ? -1 : (int)meta.triggerAlcTo,
               meta.triggerLeftLaneExists == 0xFF ? -1 : (int)meta.triggerLeftLaneExists,
               meta.triggerLeftLineUsage == 0xFF ? -1 : (int)meta.triggerLeftLineUsage,
               meta.triggerRightLaneExists == 0xFF ? -1 : (int)meta.triggerRightLaneExists,
               meta.triggerDasState == 0xFF ? -1 : (int)meta.triggerDasState,
               meta.triggerRoadClass == 0xFF ? -1 : (int)meta.triggerRoadClass,
               meta.triggerGpsRoadMatch == 0xFF ? -1 : (int)meta.triggerGpsRoadMatch,
               meta.triggerNavRouteActive == 0xFF ? -1 : (int)meta.triggerNavRouteActive,
               meta.triggerControlledAccess == 0xFF ? -1 : (int)meta.triggerControlledAccess,
               meta.triggerLeftOffRamp == 0xFF ? -1 : (int)meta.triggerLeftOffRamp,
               meta.triggerRightOffRamp == 0xFF ? -1 : (int)meta.triggerRightOffRamp,
               meta.triggerRoadAgeMs == 0xFFFFFFFFUL ? -1L : (long)meta.triggerRoadAgeMs);
      researchCaptureCsvAppend(chunk, line);

      for (uint32_t i = 0; i < meta.rawFrameCount; i++) {
        const ResearchCaptureRawEntry e = archive[meta.rawStartIndex + i];
        const int32_t rel = (int32_t)(e.timestampMs - meta.triggerMs);
        const char *phase = rel < 0 ? "RAW_PRE" : (rel == 0 ? "RAW_TRIGGER_MS" : "RAW_POST");
        const char *physicalBus = researchCapturePhysicalBusName(e.bus);
        const char *busRole = researchCaptureProfileBusName(e.bus);
        const char *source = "RX";
        const int txOk = -1;
        char raw[32]; researchCaptureCsvHex(e.data, e.dlc, raw, sizeof(raw));
        snprintf(line, sizeof(line), "%u,%s,%s,%s,%ld,%lu,%lu,0,%s,%s,%s,%d,0x%03X,%u,\"%s\"\n",
                 (unsigned)seg, researchCaptureLabelSlotName(meta.labelSlot), quotedLabel, phase, (long)rel,
                 (unsigned long)meta.triggerMs, (unsigned long)e.timestampMs, physicalBus, busRole, source, txOk,
                 (unsigned)e.id, (unsigned)e.dlc, raw);
        researchCaptureCsvAppend(chunk, line);
        if ((i & 0x3FFU) == 0x3FFU) vTaskDelay(1);
      }
    }
    researchCaptureCsvFlush(chunk);
    server.sendContent("", 0);
    researchCaptureCsvEndExport();
    return;
  }

  // SNAPSHOT CSV path. Export guard keeps entries and segment metadata stable.
  if (!entries) {
    server.sendContent("# Snapshot capture buffer unavailable\n");
    server.sendContent("", 0);
    researchCaptureCsvEndExport();
    return;
  }
  uint16_t lastSummarySegment = 0;
  for (uint32_t i = 0; i < count; i++) {
    const ResearchCaptureEntry e = entries[i];
    if (e.segment == 0 || e.segment > RESEARCH_CAPTURE_MAX_SEGMENTS) continue;

    const ResearchCaptureSegmentMeta meta = researchCaptureSegments[e.segment - 1U];
    if (lastSummarySegment != e.segment) {
      lastSummarySegment = e.segment;
      char summaryLabel[(RESEARCH_CAPTURE_LABEL_BYTES * 2) + 4];
      researchCaptureCsvQuote(meta.label, summaryLabel, sizeof(summaryLabel));
      snprintf(line, sizeof(line),
               "#segment_summary,segment_id=%u,label_slot=%s,label=%s,trigger_alc_from=%d,trigger_alc_to=%d,left_lane_exists=%d,left_line_usage=%d,right_lane_exists=%d,das_state=%d,road_class=%d,gps_road_match=%d,nav_route_active=%d,controlled_access=%d,left_off_ramp=%d,right_off_ramp=%d,road_age_ms=%ld\n",
               (unsigned)e.segment, researchCaptureLabelSlotName(meta.labelSlot), summaryLabel,
               meta.triggerAlcFrom == 0xFF ? -1 : (int)meta.triggerAlcFrom,
               meta.triggerAlcTo == 0xFF ? -1 : (int)meta.triggerAlcTo,
               meta.triggerLeftLaneExists == 0xFF ? -1 : (int)meta.triggerLeftLaneExists,
               meta.triggerLeftLineUsage == 0xFF ? -1 : (int)meta.triggerLeftLineUsage,
               meta.triggerRightLaneExists == 0xFF ? -1 : (int)meta.triggerRightLaneExists,
               meta.triggerDasState == 0xFF ? -1 : (int)meta.triggerDasState,
               meta.triggerRoadClass == 0xFF ? -1 : (int)meta.triggerRoadClass,
               meta.triggerGpsRoadMatch == 0xFF ? -1 : (int)meta.triggerGpsRoadMatch,
               meta.triggerNavRouteActive == 0xFF ? -1 : (int)meta.triggerNavRouteActive,
               meta.triggerControlledAccess == 0xFF ? -1 : (int)meta.triggerControlledAccess,
               meta.triggerLeftOffRamp == 0xFF ? -1 : (int)meta.triggerLeftOffRamp,
               meta.triggerRightOffRamp == 0xFF ? -1 : (int)meta.triggerRightOffRamp,
               meta.triggerRoadAgeMs == 0xFFFFFFFFUL ? -1L : (long)meta.triggerRoadAgeMs);
      researchCaptureCsvAppend(chunk, line);
    }
    const char *phase = e.relativeMs < 0 ? "PRE_STATE" : (e.relativeMs == 0 ? "TRIGGER_STATE" : "POST_STATE");
    const char *physicalBus = researchCapturePhysicalBusName(e.bus);
    const char *busRole = researchCaptureProfileBusName(e.bus);
    const bool isTx = (e.flags & RESEARCH_CAPTURE_FLAG_TX) != 0;
    const int txOk = isTx ? ((e.flags & RESEARCH_CAPTURE_FLAG_TX_OK) != 0 ? 1 : 0) : -1;
    const int64_t snapMs = (int64_t)meta.triggerMs + (int64_t)e.relativeMs;
    char raw[32]; researchCaptureCsvHex(e.data, e.dlc, raw, sizeof(raw));
    char quotedLabel[(RESEARCH_CAPTURE_LABEL_BYTES * 2) + 4];
    researchCaptureCsvQuote(meta.label, quotedLabel, sizeof(quotedLabel));
    snprintf(line, sizeof(line), "%u,%s,%s,%s,%d,%lu,%lld,%u,%s,%s,%s,%d,0x%03X,%u,\"%s\"\n",
             (unsigned)e.segment, researchCaptureLabelSlotName(meta.labelSlot), quotedLabel, phase, (int)e.relativeMs,
             (unsigned long)meta.triggerMs, (long long)snapMs, (unsigned)e.frameAgeMs,
             physicalBus, busRole, isTx ? "TMR_TX" : "RX", txOk, (unsigned)e.id, (unsigned)e.dlc, raw);
    researchCaptureCsvAppend(chunk, line);
    if ((i & 0x3FFU) == 0x3FFU) vTaskDelay(1);
  }
  researchCaptureCsvFlush(chunk);
  server.sendContent("", 0);
  researchCaptureCsvEndExport();
}


// ─── Universal Driver Monitoring Capture · read-only CAN A/B research ──────
static uint8_t driverMonitorParseSlot(const String &rawIn) {
  String raw = rawIn;
  raw.trim(); raw.toUpperCase();
  if (raw.length() != 1) return DRIVER_MONITOR_LABEL_NONE;
  const char c = raw[0];
  return (c >= 'A' && c <= 'F') ? (uint8_t)(c - 'A') : DRIVER_MONITOR_LABEL_NONE;
}

static void httpDriverMonitorStats() {
  server.send(200, "application/json", driverMonitorCaptureStatsToJson());
}

static void httpDriverMonitorStart() {
  if (!httpRequireLab()) return;
  const uint8_t slot = server.hasArg("slot") ? driverMonitorParseSlot(server.arg("slot")) : DRIVER_MONITOR_LABEL_NONE;
  if (slot >= DRIVER_MONITOR_LABEL_COUNT) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"slot must be A-F\"}");
    return;
  }
  const bool ok = driverMonitorCaptureRequest(slot);
  server.send(ok ? 202 : 409, "application/json", driverMonitorCaptureStatsToJson());
}

static void httpDriverMonitorReset() {
  if (!httpRequireLab()) return;
  driverMonitorCaptureReset();
  server.send(200, "application/json", driverMonitorCaptureStatsToJson());
}

static void httpDriverMonitorCsv() {
  if (!httpRequireLab()) return;

  uint16_t count = 0;
  if (!driverMonitorCaptureBeginExport(&count)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"capture or CSV export still active\"}");
    return;
  }

  server.sendHeader("X-TMR-CSV-Rows", String((unsigned)count));
  server.sendHeader("Content-Disposition", "attachment; filename=Driver_Monitoring_Capture.csv");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent("segment,label,label_name,phase,relative_ms,uptime_ms,physical_bus,bus_role,can_id,dlc,raw,driver_interaction_raw,driver_interaction_name,das_hands_on_raw,epas_hands_on_raw,epas_torque_nm\n");

  char line[520];
  char raw[32];
  char epasTorque[20];
  for (uint16_t i = 0; i < count; i++) {
    DriverMonitorRecord r = {};
    if (!driverMonitorCaptureCopyRecord(i, &r)) continue;
    researchCaptureCsvHex(r.sample.data, r.sample.dlc, raw, sizeof(raw));
    const char *phase = r.relativeMs < 0 ? "PRE" : (r.relativeMs == 0 ? "TRIGGER" : "POST");
    const uint8_t interaction = (r.sample.id == DRIVER_MONITOR_ID_STATUS2)
        ? driver_monitor_capture_pure::driverMonitorInteractionLevelPure(r.sample.data, r.sample.dlc) : 0xFF;
    const int interactionRaw = interaction == 0xFF ? -1 : (int)interaction;
    const char *interactionName = interaction == 0xFF ? "" : driverMonitorInteractionName(interaction);
    const uint8_t dasHands = (r.sample.id == DRIVER_MONITOR_ID_DAS_STATUS)
        ? driver_monitor_capture_pure::driverMonitorDasHandsOnPure(r.sample.data, r.sample.dlc) : 0xFF;
    const uint8_t epasHands = (r.sample.id == DRIVER_MONITOR_ID_EPAS_STATUS)
        ? driver_monitor_capture_pure::driverMonitorEpasHandsOnPure(r.sample.data, r.sample.dlc) : 0xFF;
    const int16_t epasCenti = (r.sample.id == DRIVER_MONITOR_ID_EPAS_STATUS)
        ? driver_monitor_capture_pure::driverMonitorEpasTorqueCentiNmPure(r.sample.data, r.sample.dlc) : INT16_MIN;
    epasTorque[0] = '\0';
    if (epasCenti != INT16_MIN) (void)formatCentiPure((int32_t)epasCenti, epasTorque, sizeof(epasTorque));
    snprintf(line, sizeof(line), "%u,%s,%s,%s,%ld,%lu,%s,%s,0x%03X,%u,\"%s\",%d,%s,%d,%d,%s\n",
             (unsigned)r.segment, driverMonitorLabelSlotName(r.label), driverMonitorLabelName(r.label), phase,
             (long)r.relativeMs, (unsigned long)r.sample.ms, driverMonitorPhysicalBusName(r.sample.bus),
             driverMonitorProfileBusName(r.sample.bus), (unsigned)r.sample.id, (unsigned)r.sample.dlc, raw,
             interactionRaw, interactionName, dasHands == 0xFF ? -1 : (int)dasHands,
             epasHands == 0xFF ? -1 : (int)epasHands, epasTorque);
    server.sendContent(line);
    if ((i & 0x7FU) == 0x7FU) vTaskDelay(1);
  }
  server.sendContent("", 0);
  driverMonitorCaptureEndExport();
}

static void httpSystemStats() { server.send(200, "application/json", systemStatsToJson()); }



// ─── Universal v3.3 profile / feature policy APIs ───────────────────
static void writeProfileCapabilityJson(JsonWriterArduino &jw) {
  jw.boolean("nagSupported", activeProfileNagSupported());
  jw.boolean("nagTorqueSupported", activeProfileNagTorqueSupported());
  jw.boolean("nagTsl9Supported", activeProfileNagTsl9Supported());
  jw.boolean("advancedEapSupported", activeProfileAdvancedEapSupported());
  jw.boolean("pedalMapSupported", activeProfilePedalMapSupported());
  jw.boolean("apDriveProfileSupported", activeProfileApDriveProfileSupported());
  jw.boolean("apDriveProfile", activeProfileApDriveProfileSupported() && apDriveProfileEnabled);
  jw.u32("apDriveProfileRegenRaw", apDriveProfileRegenRaw);
  jw.string("apDriveProfileRegenName", apDriveRegenNamePure(apDriveProfileRegenRaw));
  jw.boolean("bodyControlsSupported", activeProfileBodyControlsSupported());
  jw.boolean("euUnlockSupported", activeProfileEuUnlockSupported());
}

static String vehicleProfileStatusJson() {
  String j;
  j.reserve(520);
  JsonWriterArduino jw(j);
  jw.boolean("ok", true);
  jw.boolean("setupMode", vehicleProfileSetupMode);
  jw.boolean("nvsError", vehicleProfileNvsError);
  jw.boolean("migrationNotice", vehicleProfileMigrationNotice);
  jw.u32("profile", activeVehicleProfile);
  jw.string("profileName", vehicleProfileName(activeVehicleProfile));
  jw.u32("topology", activeVehicleTopology);
  jw.string("topologyName", vehicleProfileTopologyName(activeVehicleTopology));
  jw.string("canA", activeProfileCanAName());
  jw.string("canB", activeProfileCanBName());
  jw.string("canC", activeProfileCanCName());
  jw.u32("turn", activeTurnSignalVariant);
  jw.string("turnName", turnSignalVariantName(activeTurnSignalVariant));
  writeProfileCapabilityJson(jw);
  jw.boolean("tlsscRestoreSupported", activeProfileTlsscRestoreSupported());
  jw.boolean("bannedSupported", activeProfileBannedCarSupported());
  jw.finish();
  return j;
}

static String v3FeaturePolicyJson() {
  const bool doorCancelReported = (activeProfileBodyControlsSupported() || activeProfileIsYl()) && doorOpenCancelEnabled;
  String j;
  j.reserve(390);
  JsonWriterArduino jw(j);
  jw.boolean("ok", true);
  jw.boolean("lab", labMenuEnabled);
  jw.boolean("doorCancel", doorCancelReported);
  writeProfileCapabilityJson(jw);
  jw.boolean("banned", bannedCar);
  jw.boolean("tlsscRestore", tlsscRestoreEnabled);
  jw.boolean("tlsscRestoreSupported", activeProfileTlsscRestoreSupported());
  jw.boolean("bannedSupported", activeProfileBannedCarSupported());
  jw.boolean("s3xy", s3xyBluetoothMasterIsEnabled());
  jw.finish();
  return j;
}

static bool httpBoolArg(const char *name, bool &out) {
  if (!server.hasArg(name)) return false;
  const String a = server.arg(name);
  if (a == "1" || a == "true" || a == "on") { out = true; return true; }
  if (a == "0" || a == "false" || a == "off") { out = false; return true; }
  return false;
}

static bool httpDecimalU16InRange(const String &raw, uint16_t minimum,
                                  uint16_t maximum, uint16_t &out) {
  if (raw.length() == 0u) return false;
  uint32_t value = 0u;
  for (size_t i = 0; i < raw.length(); ++i) {
    const char ch = raw.charAt(i);
    if (ch < '0' || ch > '9') return false;
    value = value * 10u + (uint32_t)(ch - '0');
    if (value > maximum) return false;
  }
  if (value < minimum) return false;
  out = (uint16_t)value;
  return true;
}

static void httpProfileStatus() {
  server.send(200, "application/json", vehicleProfileStatusJson());
}

static void httpFeatureStatus() {
  server.send(200, "application/json", v3FeaturePolicyJson());
}

static void httpProfileSelect() {
  if (vehicleProfileNvsError) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"nvs unavailable; factory reset required\"}");
    return;
  }
  if (!server.hasArg("profile")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing profile\"}");
    return;
  }
  const int profile = server.arg("profile").toInt();
  if (!vehicleProfileValid((uint8_t)profile)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid profile\"}");
    return;
  }

  // TMR has a single fixed physical topology. Ignore legacy/client topology
  // parameters and always persist PARTY + BODY + CHASSIS.
  const uint8_t topology = (uint8_t)VEHICLE_TOPOLOGY_PARTY_BODY_CHASSIS;


  uint8_t turn = TURN_SIGNAL_UNSET;
  if (topology == VEHICLE_TOPOLOGY_PARTY_BODY_CHASSIS) {
    turn = (uint8_t)vehicleProfileDefaultTurn((uint8_t)profile);
    if ((uint8_t)profile == VEHICLE_MODEL_3_HIGHLAND) {
      if (!server.hasArg("turn")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Highland TMR requires turn variant\"}");
        return;
      }
      turn = (uint8_t)server.arg("turn").toInt();
    }
  }
  if (!vehicleProfileTurnValid((uint8_t)profile, topology, turn)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid turn variant\"}");
    return;
  }

  // Runtime profile changes are boot-boundary only. Close the global TX gate
  // before persistence so no old-profile frame can race the reboot.
  if (!prepareCanForMaintenance()) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN stop not confirmed; retry reboot\"}");
    return;
  }
  setCanTxAdministrativeHold(true);
  if (!vehicleProfileSetupMode) invalidateCanTxStateForFullRecovery();
  if (!vehicleProfileSave((uint8_t)profile, topology, turn)) {
    setCanTxAdministrativeHold(false);
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"profile NVS write failed\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
  delay(300);
  restartTmrCanSafely();
}

static void httpFeatureLab() {
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  if (!countryOverrideSetLabEnabledWithBarrier(enabled)) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"TX barrier unavailable\"}");
    return;
  }
  if (enabled && !researchCaptureEntries) {
    // Research Capture is optional LAB functionality. Its allocation state
    // must never gate the LAB Menu feature itself. LAB stays enabled even
    // when capture buffers cannot be allocated.
    (void)researchCaptureInit();
  }
  if (enabled && !driverMonitorPreRing) {
    // Driver Monitoring Capture is diagnostic-only; failure does not disable
    // the rest of LAB. Its panel will report ERROR if allocation is unavailable.
    driverMonitorCaptureInit();
  } else if (!enabled) {
    // LAB OFF suspends LAB-only capture and injection features. Production
    // NAG/DMS ownership is intentionally independent of this menu gate.
    portENTER_CRITICAL(&researchCaptureMux);
    if (!researchCaptureExporting && researchCaptureState == RESEARCH_CAPTURE_CAPTURING)
      researchCaptureState = RESEARCH_CAPTURE_PAUSED;
    portEXIT_CRITICAL(&researchCaptureMux);
    driverMonitorCaptureSuspend();
    driverWindowLabResetRuntime();
  }
  featureCfgSave();
  server.send(200, "application/json", v3FeaturePolicyJson());
}


static void httpFeatureDoorCancel() {
  if (!activeProfileBodyControlsSupported() && !activeProfileIsYl()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"door cancel requires Body CAN\"}");
    return;
  }
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  doorOpenCancelEnabled = enabled;
  featureCfgSave();
  server.send(200, "application/json", v3FeaturePolicyJson());
}

static void httpFeatureApDriveProfile() {
  if (!activeProfileApDriveProfileSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"AP Pedal / Regen Profile requires a supported 0x334 route\"}");
    return;
  }

  bool changed = false;
  if (server.hasArg("enabled")) {
    bool enabled = false;
    if (!httpBoolArg("enabled", enabled)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
      return;
    }
    portENTER_CRITICAL(&stateMux);
    apDriveProfileEnabled = enabled;
    portEXIT_CRITICAL(&stateMux);
    changed = true;
  }

  if (server.hasArg("regen")) {
    String regen = server.arg("regen");
    regen.toUpperCase();
    uint8_t raw = 0xFF;
    if (regen == "STANDARD" || regen == "20") raw = AP_DRIVE_REGEN_STANDARD_RAW;
    else if (regen == "REDUCED" || regen == "10") raw = AP_DRIVE_REGEN_REDUCED_RAW;
    else if (regen == "MINIMAL" || regen == "1") raw = AP_DRIVE_REGEN_MINIMAL_RAW;
    if (!apDriveRegenRawSelectablePure(raw)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid regen profile\"}");
      return;
    }
    portENTER_CRITICAL(&stateMux);
    apDriveProfileRegenRaw = raw;
    portEXIT_CRITICAL(&stateMux);
    changed = true;
  }

  if (!changed) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing enabled or regen\"}");
    return;
  }

  // AP Drive Profile owns outgoing 0x334 only while its AP gate is open.
  // A manual PedalMap drive-session target remains armed and resumes after AP.
  featureCfgSave();
  server.send(200, "application/json", v3FeaturePolicyJson());
}

static void httpFeatureBanned() {
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  if (enabled && !activeProfileBannedCarSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"banned car not supported for current profile\"}");
    return;
  }
  if (!enabled) {
    // Close every application TX path before changing the Restore invariant.
    // This prevents a concurrent CAN task from enqueueing a Restore frame while
    // Banned Car is transitioning OFF and its persisted state is sanitized.
    setCanTxAdministrativeHold(true);
    tlsscRestoreEnabled = false;
    bannedCar = false;
    featureCfgSave();
    setCanTxAdministrativeHold(false);
  } else {
    bannedCar = true;
    featureCfgSave();
  }
  server.send(200, "application/json", v3FeaturePolicyJson());
}

static void httpFeatureTlsscRestore() {
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  if (enabled && (!activeProfileBannedCarSupported() || !bannedCar || !activeProfileTlsscRestoreSupported())) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"restore not allowed for current state/profile\"}");
    return;
  }
  tlsscRestoreEnabled = enabled;
  featureCfgSave();
  server.send(200, "application/json", v3FeaturePolicyJson());
}

static void httpFeatureS3xy() {
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  if (enabled) httpS3xyBluetoothEnable();
  else httpS3xyBluetoothDisable();
}

static bool resetFirmwareSettingsPreserveProfileAndBle() {
  // Deliberately preserve v3profile, wifiap, s3xy and s3xyreg. t2meta is internal
  // schema metadata and is also preserved so a settings reset cannot replay
  // old migrations.
  static const char *const clearNamespaces[] = {
    "nag", "summon", "lab3f8", "ulc", "alc293lab", "countrylab", "r79lab", "r79", "lanegraph",
    "researchcap", "features", "labv3fd"
  };
  bool ok = true;
  for (const char *ns : clearNamespaces) {
    Preferences p;
    if (!p.begin(ns, false)) { ok = false; continue; }
    if (!p.clear()) ok = false;
    p.end();
  }
  return ok;
}

static bool factoryResetAllNvs() {
  const esp_err_t eraseErr = nvs_flash_erase();
  if (eraseErr != ESP_OK) return false;
  const esp_err_t initErr = nvs_flash_init();
  if (initErr != ESP_OK) return false;
  // Explicit Factory Reset is not an OTA migration, so do not show the
  // MAJOR FIRMWARE UPDATE notice on the resulting setup screen.
  return vehicleProfileWriteBootstrapMarker(false);
}

static constexpr const char *NVS_KEEP_BLE_RESET_GUARD_NAMESPACE = "t2reset";
static constexpr const char *NVS_KEEP_BLE_RESET_GUARD_KEY = "pending";

static bool resetNvsKeepBleGuardRead(bool &active) {
  active = false;
  Preferences p;
  // Open read/write so the namespace is created on ordinary installations;
  // Preferences read-only open reports NOT_FOUND when the guard never existed.
  if (!p.begin(NVS_KEEP_BLE_RESET_GUARD_NAMESPACE, false)) return false;
  active = p.getBool(NVS_KEEP_BLE_RESET_GUARD_KEY, false);
  p.end();
  return true;
}

static bool resetNvsKeepBleGuardWrite(bool active) {
  Preferences p;
  if (!p.begin(NVS_KEEP_BLE_RESET_GUARD_NAMESPACE, false)) return false;
  const bool ok = active
      ? p.putBool(NVS_KEEP_BLE_RESET_GUARD_KEY, true) > 0u
      : p.clear();
  p.end();
  return ok;
}

static bool resetNvsKeepBleClearApplicationNamespaces() {
  // Explicit application allowlist: S3XY settings/registry and opaque BLE
  // stack bond namespaces are intentionally absent.
  static const char *const clearNamespaces[] = {
    "v3profile", "wifiap", "nag", "summon", "lab3f8", "ulc",
    "alc293lab", "countrylab", "r79lab", "r79", "lanegraph", "researchcap", "features",
    "canDiag", "t2meta", "labv3fd"
  };
  static_assert(
      sizeof(clearNamespaces) / sizeof(clearNamespaces[0]) ==
          NVS_KEEP_BLE_APPLICATION_NAMESPACE_COUNT_PURE,
      "keep-BLE reset allowlist count must match the failure model");
  bool ok = true;
  for (const char *ns : clearNamespaces) {
    Preferences p;
    if (!p.begin(ns, false)) { ok = false; continue; }
    if (!p.clear()) ok = false;
    p.end();
  }
  return ok;
}

static bool resetNvsKeepBleFinalize() {
  if (!resetNvsKeepBleClearApplicationNamespaces()) return false;
  if (!vehicleProfileWriteBootstrapMarker(false)) return false;
  return resetNvsKeepBleGuardWrite(false);
}

static bool resetNvsKeepBleRecoverIfNeeded() {
  bool active = false;
  if (!resetNvsKeepBleGuardRead(active)) return false;
  return !active || resetNvsKeepBleFinalize();
}

static void httpResetNvsPreserveS3xyBle() {
  if (!prepareCanForMaintenance()) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN stop not confirmed; retry reboot\"}");
    return;
  }

  if (!resetNvsKeepBleGuardWrite(true)) {
    server.send(500, "application/json",
                "{\"ok\":false,\"error\":\"BLE-preserving reset guard write failed; nothing was cleared\"}");
    return;
  }
  setCanTxAdministrativeHold(true);
  if (!vehicleProfileSetupMode && !vehicleProfileNvsError)
    invalidateCanTxStateForFullRecovery();
  const bool completed = resetNvsKeepBleFinalize();
  if (!completed) {
    server.send(500, "application/json",
                "{\"ok\":false,\"rebooting\":true,\"error\":\"BLE-preserving reset interrupted; boot recovery will retry without erasing BLE\"}");
  } else {
    server.send(200, "application/json",
                "{\"ok\":true,\"rebooting\":true,\"profileSetup\":true,\"blePreserved\":true}");
  }
  delay(300);
  restartTmrCanSafely();
}

static void httpResetFirmwareSettings() {
  if (vehicleProfileSetupMode || vehicleProfileNvsError) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"not available in setup mode\"}");
    return;
  }
  if (!prepareCanForMaintenance()) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN stop not confirmed; retry reboot\"}");
    return;
  }
  setCanTxAdministrativeHold(true);
  invalidateCanTxStateForFullRecovery();
  tlsscRestoreEnabled = false;
  if (!resetFirmwareSettingsPreserveProfileAndBle()) {
    setCanTxAdministrativeHold(false);
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"settings reset failed\"}");
    return;
  }
  const FeatureConfigV37Pure defaults = featureConfigV37DefaultsPure();
  if (!featureConfigWriteSchema3Values(defaults) ||
      !featureConfigSchema3ReadBackMatches(defaults)) {
    setCanTxAdministrativeHold(false);
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"settings defaults write failed\"}");
    return;
  }
  portENTER_CRITICAL(&r79LabMux);
  r79Bit18Policy = defaults.r79Bit18Mode;
  r79FixedQuietState = {};
  portEXIT_CRITICAL(&r79LabMux);
  portENTER_CRITICAL(&blinkAMux);
  blinkANoaStabilizationSeconds = defaults.noaStabilizationSeconds;
  blinkACancelPauseSeconds = defaults.cancelPauseSeconds;
  autoBlinkerNoaSessionResetPure(autoBlinkerNoaSessionState);
  autoBlinkerCancelPauseResetPure(autoBlinkerCancelPauseState);
  portEXIT_CRITICAL(&blinkAMux);
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true,\"profilePreserved\":true,\"blePreserved\":true}");
  delay(300);
  restartTmrCanSafely();
}

static void httpFactoryReset() {
  if (!prepareCanForMaintenance()) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN stop not confirmed; retry reboot\"}");
    return;
  }

  setCanTxAdministrativeHold(true);
  if (!vehicleProfileSetupMode && !vehicleProfileNvsError) invalidateCanTxStateForFullRecovery();
  const bool ok = factoryResetAllNvs();
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true,\"rebooting\":true}"
                 : "{\"ok\":false,\"error\":\"factory reset failed; rebooting safe\"}");
  delay(300);
  restartTmrCanSafely();
}

static void httpCanHardReinit() {
  if (canMaintenanceActive()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"CAN stopped for maintenance; reboot required\"}");
    return;
  }
  requestCanSubsystemRestart(CAN_SUP_HARD_MANUAL, CAN_REC_MANUAL);
  server.send(202, "application/json", "{\"ok\":true,\"action\":\"hard-can-reinit-requested\"}");
}

static void httpRebootTmrCan() {
  if (!prepareCanForMaintenance()) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"CAN stop not confirmed; retry reboot\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"rebooting\"}");
  delay(250);
  restartTmrCanSafely();
}

static void httpRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", (const char*)INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
}
static void httpAppleTouchIcon() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "image/png", (const char*)APPLE_TOUCH_ICON, APPLE_TOUCH_ICON_LEN);
}
static void httpLabGeistFont() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "font/woff2", (const char*)LAB_GEIST_FONT, LAB_GEIST_FONT_LEN);
}
static void httpLabGeistMonoFont() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "font/woff2", (const char*)LAB_GEIST_MONO_FONT, LAB_GEIST_MONO_FONT_LEN);
}
static void httpNagConfig() { server.send(200, "application/json", nagCfgToJson()); }
static void httpNagStats()  { server.send(200, "application/json", nagStatsToJson()); }

static void httpNagSetMode() {
  if (!activeProfileNagTorqueSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Torque NAG unavailable for current topology\"}");
    return;
  }
  int m = server.arg("m").toInt();
  bool enabled, ignoreApState;
  bool pauseAtZero = false;
  uint8_t method = NAG_METHOD_DEFAULT_PURE;
  uint8_t tsl9Sequence = TSL9_SEQUENCE_DEFAULT_PURE;
  uint8_t tsl9Window = TSL9_DOWNGRADE_WINDOW_DEFAULT_PURE;
  uint8_t tsl9InputMode = TSL9_INPUT_MODE_DEFAULT_PURE;
  bool tsl9IsaChimeSuppress = false;
  uint8_t tsl9LegacyRoute = TSL9_LEGACY_ROUTE_DEFAULT_PURE;
  bool dmsControlEnabled = false;
  portENTER_CRITICAL(&nagCfgMux);
  enabled = nagCfg.enabled;
  ignoreApState = nagCfg.ignoreApState;
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  method = nagMethodSanitizePure(nagCfg.method);
  tsl9Sequence = tsl9SequenceSanitizePure(nagCfg.tsl9Sequence);
  tsl9Window = tsl9DowngradeWindowSanitizePure(
      nagCfg.tsl9DowngradeWindow);
  tsl9InputMode = tsl9InputModeSanitizePure(nagCfg.tsl9InputMode);
  tsl9IsaChimeSuppress = nagCfg.tsl9IsaChimeSuppress;
  tsl9LegacyRoute = tsl9LegacyRouteSanitizePure(nagCfg.tsl9LegacyRoute);
  dmsControlEnabled = nagCfg.dmsControlEnabled;
  portEXIT_CRITICAL(&nagCfgMux);

  NagConfig nc;
  if      (m == MODE_B) nagCfgDefaultsModeB(nc);
  else if (m == MODE_C) nagCfgDefaultsModeC(nc);
  else if (m == MODE_H) nagCfgDefaultsModeH(nc);
  else                  nagCfgDefaultsModeA(nc);
  // Pause-at-zero is a common NAG policy, not a mode waveform parameter.
  // Preserve it when switching modes. Explicit NAG reset still restores OFF.
  nc.enabled = enabled;
  nc.ignoreApState = ignoreApState;
  nc.pauseAtZeroSpeed = pauseAtZero;
  nc.method = method;
  nc.tsl9Sequence = tsl9Sequence;
  nc.tsl9DowngradeWindow = tsl9Window;
  nc.tsl9InputMode = tsl9InputMode;
  nc.tsl9IsaChimeSuppress = tsl9IsaChimeSuppress;
  nc.tsl9LegacyRoute = tsl9LegacyRoute;
  nc.dmsControlEnabled = dmsControlEnabled;
  nagCfgCommit(nc);
  nagCfgSave();
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagSetHumanVariant() {
  server.send(410, "application/json", "{\"ok\":false,\"error\":\"Mode H profiles retired; Mode H uses one engine\"}");
}

static void httpNagUpdate() {
  NagConfig nc;
  portENTER_CRITICAL(&nagCfgMux); nc = nagCfg; portEXIT_CRITICAL(&nagCfgMux);
  const NagConfig previous = nc;
  bool torqueRightScrollEnabled, tsl9RightPeriodicEnabled;
  uint16_t torqueRightScrollInterval, tsl9RightPeriodicInterval;
  uint8_t torqueRightScrollPattern;
  portENTER_CRITICAL(&nagRightScrollMux);
  torqueRightScrollEnabled = nagTorqueRightScrollEnabled;
  torqueRightScrollInterval = nagTorqueRightScrollIntervalSeconds;
  torqueRightScrollPattern =
      nagRightScrollPatternSanitize(nagTorqueRightScrollPattern);
  tsl9RightPeriodicEnabled = nagTsl9RightPeriodicEnabled;
  tsl9RightPeriodicInterval = nagTsl9RightPeriodicIntervalSeconds;
  portEXIT_CRITICAL(&nagRightScrollMux);
  const bool previousTorqueRightScrollEnabled = torqueRightScrollEnabled;
  const uint16_t previousTorqueRightScrollInterval = torqueRightScrollInterval;
  const uint8_t previousTorqueRightScrollPattern = torqueRightScrollPattern;
  const bool previousTsl9RightPeriodicEnabled = tsl9RightPeriodicEnabled;
  const uint16_t previousTsl9RightPeriodicInterval = tsl9RightPeriodicInterval;
  nagCfgApplyActiveProfilePolicy(nc, false);
  if (server.hasArg("ignoreApState") && !httpBoolArg("ignoreApState", nc.ignoreApState)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid ignoreApState\"}");
    return;
  }
  if (server.hasArg("enabled"))
    nc.enabled = (server.arg("enabled") == "1");
  if (server.hasArg("method")) {
    const String raw = server.arg("method");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"method-must-be-0-or-1\"}");
      return;
    }
    const uint8_t requested = nagMethodSanitizePure((uint8_t)raw.toInt());
    if ((requested == NAG_METHOD_TORQUE_PURE && !activeProfileNagTorqueSupported()) ||
        (requested == NAG_METHOD_TSL9_PURE && !activeProfileNagTsl9Supported())) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG method unavailable for current topology\"}");
      return;
    }
    nc.method = requested;
  }
  if (server.hasArg("tsl9Sequence")) {
    const String raw = server.arg("tsl9Sequence");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"tsl9Sequence-must-be-0-or-1\"}");
      return;
    }
    nc.tsl9Sequence = tsl9SequenceSanitizePure((uint8_t)raw.toInt());
  }
  if (server.hasArg("tsl9Window")) {
    const String raw = server.arg("tsl9Window");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"tsl9Window-must-be-0-or-1\"}");
      return;
    }
    nc.tsl9DowngradeWindow =
        tsl9DowngradeWindowSanitizePure((uint8_t)raw.toInt());
  }
  if (server.hasArg("tsl9InputMode")) {
    const String raw = server.arg("tsl9InputMode");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"tsl9InputMode-must-be-0-or-1\"}");
      return;
    }
    nc.tsl9InputMode = tsl9InputModeSanitizePure((uint8_t)raw.toInt());
  }
  if (server.hasArg("torqueRightScrollEnabled")) {
    const String raw = server.arg("torqueRightScrollEnabled");
    if (raw != "0" && raw != "1" && raw != "false" && raw != "true") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid torqueRightScrollEnabled\"}");
      return;
    }
    torqueRightScrollEnabled = raw == "1" || raw == "true";
  }
  if (server.hasArg("torqueRightScrollIntervalSeconds")) {
    uint16_t value;
    if (!httpDecimalU16InRange(
            server.arg("torqueRightScrollIntervalSeconds"), 1u, 600u,
            value)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"torqueRightScrollIntervalSeconds must be 1..600\"}");
      return;
    }
    torqueRightScrollInterval = value;
  }
  if (server.hasArg("torqueRightScrollPattern")) {
    const String raw = server.arg("torqueRightScrollPattern");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"torqueRightScrollPattern must be 0 or 1\"}");
      return;
    }
    torqueRightScrollPattern =
        nagRightScrollPatternSanitize((uint8_t)raw.toInt());
  }
  if (server.hasArg("tsl9RightPeriodicEnabled")) {
    const String raw = server.arg("tsl9RightPeriodicEnabled");
    if (raw != "0" && raw != "1" && raw != "false" && raw != "true") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid tsl9RightPeriodicEnabled\"}");
      return;
    }
    tsl9RightPeriodicEnabled = raw == "1" || raw == "true";
  }
  if (server.hasArg("tsl9RightPeriodicIntervalSeconds")) {
    uint16_t value;
    if (!httpDecimalU16InRange(
            server.arg("tsl9RightPeriodicIntervalSeconds"), 1u, 600u,
            value)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"tsl9RightPeriodicIntervalSeconds must be 1..600\"}");
      return;
    }
    tsl9RightPeriodicInterval = value;
  }
  if (server.hasArg("tsl9IsaChimeSuppress"))
    nc.tsl9IsaChimeSuppress = server.arg("tsl9IsaChimeSuppress") == "1" ||
        server.arg("tsl9IsaChimeSuppress") == "true";
  if (server.hasArg("dmsControlEnabled"))
    nc.dmsControlEnabled = server.arg("dmsControlEnabled") == "1" ||
        server.arg("dmsControlEnabled") == "true";
  if (server.hasArg("tsl9LegacyRoute")) {
    const String raw = server.arg("tsl9LegacyRoute");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"tsl9LegacyRoute-must-be-0-or-1\"}");
      return;
    }
    if (!vehicleProfileLegacyTsl9RouteSelectable(
            activeVehicleProfile, activeVehicleTopology)) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"Legacy TSL9 route selector unavailable for current profile\"}");
      return;
    }
    nc.tsl9LegacyRoute = tsl9LegacyRouteSanitizePure(
        (uint8_t)raw.toInt());
  }
  if (server.hasArg("pauseAtZeroSpeed"))
    nc.pauseAtZeroSpeed = (server.arg("pauseAtZeroSpeed") == "1" || server.arg("pauseAtZeroSpeed") == "true");
  if (server.hasArg("modeHStopBehavior")) {
    int val = server.arg("modeHStopBehavior").toInt();
    if (val < 0 || val > 255 || !nagModeHStopBehaviorValidPure((uint8_t)val)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid Mode H stop behavior\"}");
      return;
    }
    nc.modeHStopBehavior = (uint8_t)val;
  }
  if (server.hasArg("targetId")) {
    char* endptr;
    long val = strtol(server.arg("targetId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF)
      nc.targetId = (uint16_t)val;
  }
  if (server.hasArg("hoRatePct")) {
    int val = server.arg("hoRatePct").toInt();
    if (val >= 0 && val <= 100) nc.hoRatePct = (uint8_t)val;
  }
  if (server.hasArg("burstMs")) {
    int val = server.arg("burstMs").toInt();
    if (val >= 50 && val <= 10000) nc.burstMs = (uint16_t)val;
  }
  if (server.hasArg("pauseMs")) {
    int val = server.arg("pauseMs").toInt();
    if (val >= 0 && val <= 10000) nc.pauseMs = (uint16_t)val;
  }
  if (server.hasArg("apStateId")) {
    char* endptr;
    long val = strtol(server.arg("apStateId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF)
      nc.apStateId = (uint16_t)val;
  }
  if (server.hasArg("steeringId")) {
    char* endptr;
    long val = strtol(server.arg("steeringId").c_str(), &endptr, 0);
    if (*endptr == '\0' && val > 0 && val <= 0x7FF)
      nc.steeringId = (uint16_t)val;
  }
  if (server.hasArg("count")) {
    uint8_t n = (uint8_t)server.arg("count").toInt();
    if (n > NAG_MAX_TORQUE_ENTRIES) n = NAG_MAX_TORQUE_ENTRIES;
    if (n < 1) n = 1;
    for (uint8_t i = 0; i < n; i++) {
      String k2 = "b2_" + String(i);
      String k3 = "b3_" + String(i);
      if (server.hasArg(k2)) {
        char* endptr;
        long val = strtol(server.arg(k2).c_str(), &endptr, 0);
        if (*endptr == '\0' && val >= 0 && val <= 255)
          nc.torqueB2[i] = (uint8_t)val;
      }
      if (server.hasArg(k3)) {
        char* endptr;
        long val = strtol(server.arg(k3).c_str(), &endptr, 0);
        if (*endptr == '\0' && val >= 0 && val <= 255)
          nc.torqueB3[i] = (uint8_t)val;
      }
    }
    nc.torqueCount = n;
  }
  nagCfgClampAll(nc);
  const bool tsl9TransportChanged =
      previous.enabled != nc.enabled ||
      nagMethodSanitizePure(previous.method) !=
          nagMethodSanitizePure(nc.method) ||
      tsl9InputModeSanitizePure(previous.tsl9InputMode) !=
          tsl9InputModeSanitizePure(nc.tsl9InputMode) ||
      tsl9LegacyRouteSanitizePure(previous.tsl9LegacyRoute) !=
          tsl9LegacyRouteSanitizePure(nc.tsl9LegacyRoute);
  const bool rightScrollChanged =
      previousTorqueRightScrollEnabled != torqueRightScrollEnabled ||
      previousTorqueRightScrollInterval != torqueRightScrollInterval ||
      previousTorqueRightScrollPattern != torqueRightScrollPattern ||
      previousTsl9RightPeriodicEnabled != tsl9RightPeriodicEnabled ||
      previousTsl9RightPeriodicInterval != tsl9RightPeriodicInterval ||
      nagMethodSanitizePure(previous.method) != nagMethodSanitizePure(nc.method) ||
      tsl9InputModeSanitizePure(previous.tsl9InputMode) !=
          tsl9InputModeSanitizePure(nc.tsl9InputMode);
  const bool inputTransportChanged =
      tsl9TransportChanged || rightScrollChanged;
  if (inputTransportChanged && !tsl9InputQuiesceForConfig()) {
    server.send(503, "application/json",
                "{\"ok\":false,\"error\":\"TSL9 input cleanup pending; retry after a fresh MUX1 frame\"}");
    return;
  }
  if (server.hasArg("ignoreApState") && !nagIgnoreApStatePersist(nc.ignoreApState)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NAG option NVS write failed\"}");
    return;
  }
  if (inputTransportChanged) setCanTxAdministrativeHold(true);
  if (tsl9TransportChanged) nagTsl9RuntimeReset();
  nagCfgCommit(nc);
  if (previous.ignoreApState != nc.ignoreApState) {
    nagExactEchoReset();
    nagHumanRuntimeReset(false);
  }
  nagCfgSave();
  if (rightScrollChanged) {
    portENTER_CRITICAL(&nagRightScrollMux);
    nagTorqueRightScrollEnabled = torqueRightScrollEnabled;
    nagTorqueRightScrollIntervalSeconds = torqueRightScrollInterval;
    nagTorqueRightScrollPattern = torqueRightScrollPattern;
    nagTsl9RightPeriodicEnabled = tsl9RightPeriodicEnabled;
    nagTsl9RightPeriodicIntervalSeconds = tsl9RightPeriodicInterval;
    portEXIT_CRITICAL(&nagRightScrollMux);
    featureCfgSave();
  }
  if (inputTransportChanged) {
    tsl9InputHardResetAfterQuiesce(false);
    setCanTxAdministrativeHold(false);
  }
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagReset() {
  if (!tsl9InputQuiesceForConfig()) {
    server.send(503, "application/json",
                "{\"ok\":false,\"error\":\"TSL9 input cleanup pending; retry after a fresh MUX1 frame\"}");
    return;
  }
  setCanTxAdministrativeHold(true);
  NagConfig nc;
  nagCfgDefaultsModeH(nc);
  nagCfgApplyActiveProfilePolicy(nc, true);
  nagHumanRuntimeLoadDefaults();
  nagCfgCommit(nc);
  nagCfgSave();
  nagRxFrames = nagEchoCount = mcpTxOk = mcpTxFail = 0;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOk=nagTxFail=0; nagSkipDisabled=nagSkipBootDelay=nagSkipWarmup=nagSkipSelfFrame=0;
  nagSkipHandsOn=nagSkipApInvalid=nagSkipApInactive=nagSkipDecision=nagSkipStopped=nagSkipSpeedStale=nagStopCarrierTxOk=0;
  nagBlockMutex=nagBlockMcpNotReady=nagBlockEpoch=nagBlockFreshMask=nagBlockInvalidMsg=nagSendError=0;
  nagLastTxOkMs=nagMaxTxGapMs=nagSessionTxOk=0; nagSessionStartMs=0;
  nagLastSkipReason=NAG_SKIP_NONE; nagLastSkipMs=0; nagLastTxBlockReason=MCP_TX_OK; nagLastTxBlockMs=0;
  portEXIT_CRITICAL(&nagDiagMux);
  portENTER_CRITICAL(&nagExactEchoMux);
  nagExactEchoLastTxValid=false; nagExactEchoLastTxMs=0; memset(nagExactEchoLastTxRaw,0,sizeof(nagExactEchoLastTxRaw));
  portEXIT_CRITICAL(&nagExactEchoMux);
  portENTER_CRITICAL(&nagTsl9Mux);
  nagTsl9State = {};
  nagTsl9Rx = nagTsl9Modified = nagTsl9HandsOnModified =
      nagTsl9IsaModified = nagTsl9TxOk = nagTsl9TxFail = 0;
  portEXIT_CRITICAL(&nagTsl9Mux);
  portENTER_CRITICAL(&nagRightScrollMux);
  nagTorqueRightScrollEnabled = false;
  nagTorqueRightScrollIntervalSeconds =
      AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE;
  nagTorqueRightScrollPattern = NAG_RIGHT_SCROLL_PATTERN_DEFAULT;
  nagTsl9RightPeriodicEnabled = false;
  nagTsl9RightPeriodicIntervalSeconds =
      AP_RIGHT_SCROLL_DEFAULT_INTERVAL_S_PURE;
  portEXIT_CRITICAL(&nagRightScrollMux);
  featureCfgSave();
  tsl9InputHardResetAfterQuiesce(true);
  setCanTxAdministrativeHold(false);
  server.send(200, "application/json", nagCfgToJson());
}

static void httpSummonStats()  { server.send(200, "application/json", summonStatsToJson()); }
static void httpSummonTlsscEnable() {
    setTlsscEnabled(true, true);
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonTlsscDisable() {
    setTlsscEnabled(false, true);
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonTlsscHighwayGate() {
    if (!server.hasArg("enabled")) {
      server.send(400, "text/plain", "missing enabled");
      return;
    }
    const String arg = server.arg("enabled");
    if (arg != "0" && arg != "1") {
      server.send(400, "text/plain", "invalid enabled");
      return;
    }
    const bool enabled = (arg == "1");
    portENTER_CRITICAL(&stateMux);
    tlsscHighwayGateEnabled = enabled;
    portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}

static void httpSummonTlsscNoaBlock() {
    bool enabled;
    if (!httpBoolArg("enabled", enabled)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
      return;
    }
    portENTER_CRITICAL(&stateMux);
    tlsscBlockInNoa = enabled;
    if (enabled && gateNOAActive && tlsscInjectedActive) tlsscClearPending = true;
    portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}

static void httpDasTelemetryStats() {
  server.send(200, "application/json", dasTelemetryStatsToJson());
}


static bool httpRequireLab() {
  if (labMenuEnabled) return true;
  server.send(409, "application/json", "{\"ok\":false,\"error\":\"LAB disabled\"}");
  return false;
}

static String driverWindowLabStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  const DriverWindowArmResultPure availability = driverWindowLabAvailability(now);
  bool pending, stockValid, lastTxValid;
  uint8_t framesSent, lastArm, lastConsume, lastState;
  uint32_t stockMs, lastTxMs, requests, completed, txOk, txFail, blocked;
  uint8_t stockRaw[8], lastTxRaw[8];
  portENTER_CRITICAL(&driverWindowLabMux);
  pending = driverWindowLabState.pending;
  framesSent = driverWindowLabState.framesSent;
  stockValid = driverWindowLabStockValid;
  stockMs = driverWindowLabStockMs;
  memcpy(stockRaw, driverWindowLabStockRaw, sizeof(stockRaw));
  lastTxValid = driverWindowLabLastTxValid;
  lastTxMs = driverWindowLabLastTxMs;
  memcpy(lastTxRaw, driverWindowLabLastTxRaw, sizeof(lastTxRaw));
  requests = driverWindowLabRequests;
  completed = driverWindowLabCompleted;
  txOk = driverWindowLabTxOk;
  txFail = driverWindowLabTxFail;
  blocked = driverWindowLabBlocked;
  lastArm = driverWindowLabLastArmResult;
  lastConsume = driverWindowLabLastConsumeReason;
  lastState = driverWindowLabLastResult;
  portEXIT_CRITICAL(&driverWindowLabMux);

  const char *lastResult = "IDLE";
  if (pending || lastState == DRIVER_WINDOW_RESULT_ARMED) lastResult = "PENDING";
  else if (lastState == DRIVER_WINDOW_RESULT_COMPLETED) lastResult = "COMPLETED";
  else if (lastState == DRIVER_WINDOW_RESULT_CANCELED) lastResult = "CANCELED";
  else if (lastState == DRIVER_WINDOW_RESULT_BLOCKED)
    lastResult = driverWindowLabConsumeReasonName(lastConsume);
  else if (lastState == DRIVER_WINDOW_RESULT_REJECTED)
    lastResult = driverWindowLabArmResultName(lastArm);
  String j;
  j.reserve(512);
  JsonWriterArduino jw(j);
  jw.boolean("ok", true);
  jw.boolean("supported", driverWindowLabProfileSupported());
  jw.boolean("available", availability == DRIVER_WINDOW_ARM_OK);
  jw.boolean("pending", pending);
  jw.u32("framesSent", framesSent);
  jw.u32("stockAgeMs", stockValid ? (uint32_t)(now - stockMs) : 999999u);
  jw.u32("lastTxAgeMs", lastTxValid ? (uint32_t)(now - lastTxMs) : 999999u);
  jw.u32("requests", requests);
  jw.u32("completed", completed);
  jw.u32("txOk", txOk);
  jw.u32("txFail", txFail);
  jw.u32("blocked", blocked);
  jw.string("availability", driverWindowLabArmResultName(availability));
  jw.string("lastResult", lastResult);
  jw.string("stockRaw", lab3f8RawHex(stockRaw, stockValid ? 8u : 0u));
  jw.string("lastTxRaw", lab3f8RawHex(lastTxRaw, lastTxValid ? 8u : 0u));
  jw.finish();
  return j;
}

static void httpDriverWindowLabStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", driverWindowLabStatsToJson());
}

static void httpDriverWindowLabOpen() {
  if (!httpRequireLab()) return;
  const DriverWindowArmResultPure result = driverWindowLabRequestOpen();
  server.send(result == DRIVER_WINDOW_ARM_OK ? 202 : 409,
              "application/json", driverWindowLabStatsToJson());
}

static void writeCanARxLabJson(JsonWriterArduino &jw) {
  const uint8_t savedMode = canARxSavedMode;
  jw.u32("savedMode", savedMode);
  jw.u32("effectiveMode", canARxReadBatchBudgetPure(labMenuEnabled, savedMode) == 1
                            ? CAN_A_RX_IMMEDIATE : CAN_A_RX_PREFETCH_4);
  jw.boolean("labEnabled", labMenuEnabled);
}

static String canARxLabStatsToJson() {
  String j;
  j.reserve(96);
  JsonWriterArduino jw(j);
  jw.boolean("ok", true);
  writeCanARxLabJson(jw);
  jw.finish();
  return j;
}

static void httpCanARxLabStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", canARxLabStatsToJson());
}

static void httpCanARxLabUpdate() {
  if (!httpRequireLab()) return;
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing mode\"}");
    return;
  }
  const String mode = server.arg("mode");
  if (mode != "0" && mode != "1") {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid mode\"}");
    return;
  }
  const uint8_t requested = mode == "1" ? CAN_A_RX_IMMEDIATE : CAN_A_RX_PREFETCH_4;
  if (!canARxModePersist(requested)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"CAN A RX mode NVS write failed\"}");
    return;
  }
  canARxSavedMode = requested;
  server.send(200, "application/json", canARxLabStatsToJson());
}


static void writeNagHumanV1ConfigJson(JsonWriterArduino &jw,
                                      const NagHumanV1ConfigPure &config) {
  jw.raw("tuningEditable", "true");
  jw.fixed("peakMinNm", (int32_t)config.peakMinRaw, 2u);
  jw.fixed("peakMaxNm", (int32_t)config.peakMaxRaw, 2u);
  jw.u32("waitMinMs", (uint32_t)config.waitMinMs);
  jw.u32("waitMaxMs", (uint32_t)config.waitMaxMs);
  jw.u32("refractoryMinMs", (uint32_t)config.refractoryMinMs);
  jw.u32("refractoryMaxMs", (uint32_t)config.refractoryMaxMs);
}

static void writeNagHumanV1StateJson(JsonWriterArduino &jw,
                                     const NagHumanV1StatePure &state,
                                     uint16_t outputRaw,
                                     uint8_t stopBehavior) {
  jw.string("phase", nagHumanV1PhaseNamePure(state.phase));
  jw.string("motion", nagHumanV1MotionNamePure(state.motion));
  jw.string("eventType", nagHumanV1EventTypeNamePure(state.event.type));
  jw.fixed("activeEventPeakNm", (int32_t)state.event.peakRaw, 2u);
  jw.u32("eventCount", (uint32_t)state.eventCount);
  jw.fixed("outputNm", (int32_t)outputRaw - 2050, 2u);
  jw.boolean("carrier", state.carrier);
  jw.boolean("stopCarrierActive",
             state.phase == H1_PAUSED_STOPPED &&
                 stopBehavior == H_STOP_STOCK_CARRIER);
}

static String nagHumanProfileStatsToJson() {
  const uint8_t variant = nagHumanVariantSnapshot();
  uint8_t stopBehavior = nagModeHDefaultStopBehaviorPure();
  uint32_t stopCarrierTxOk = 0;
  portENTER_CRITICAL(&nagCfgMux);
  stopBehavior = nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior)
      ? nagCfg.modeHStopBehavior : nagModeHDefaultStopBehaviorPure();
  portEXIT_CRITICAL(&nagCfgMux);
  portENTER_CRITICAL(&nagDiagMux);
  stopCarrierTxOk = nagStopCarrierTxOk;
  portEXIT_CRITICAL(&nagDiagMux);
  String j;
  j.reserve(1300);
  JsonWriterArduino jw(j);
  jw.u32("variant", (uint32_t)(variant));
  jw.string("variantCode", nagModeHVariantCodePure(variant));
  jw.string("variantLabel", nagModeHVariantLabelPure(variant));
  jw.u32("stopBehavior", (uint32_t)(stopBehavior));
  jw.string("stopBehaviorName", nagModeHStopBehaviorNamePure(stopBehavior));
  jw.u32("stopCarrierTxOk", stopCarrierTxOk);
{
    const NagHumanV4ConfigPure c = nagHumanV4RuntimeConfigSnapshot();
    const NagHumanV4StatePure state = nagHumanV4RuntimeSnapshot();
    const uint16_t outputRaw = state.base.outputRaw != 0u ? state.base.outputRaw : NAG_HUMAN_V1_TORQUE_CENTER_RAW;
    writeNagHumanV1ConfigJson(jw, c.base);
    jw.fixed("carrierMinNm", (int32_t)c.carrierMinRaw, 2u);
    jw.fixed("carrierMaxNm", (int32_t)c.carrierMaxRaw, 2u);
    jw.u32("hoPolicy", (uint32_t)c.hoPolicy);
    jw.string("hoPolicyName", nagHumanV3HoPolicyNamePure(c.hoPolicy));
    jw.fixed("ho1ThresholdNm", (int32_t)c.ho1ThresholdRaw, 2u);
    jw.fixed("ho2ThresholdNm", (int32_t)c.ho2ThresholdRaw, 2u);
    jw.raw("negativeBiasPct", "80");
    jw.fixed("stockDeadbandNm", (int32_t)NAG_HUMAN_V4_STOCK_DEADBAND_RAW, 2u);
    jw.string("carrierMode", "OPPOSITE_STOCK_PER_RX");
    jw.boolean("visualRescueEnabled", c.visualRescueEnabled);
    jw.u32("visualRescueDelayMs", (uint32_t)c.visualRescueDelayMs);
    jw.boolean("visualRescuePending", state.visualRescuePending);
    jw.u32("visualRescueCount", state.visualRescueCount);
    jw.fixed("carrierSampleNm", (int32_t)state.lastCarrierMagnitudeRaw, 2u);
    jw.boolean("carrierApplied", state.lastCarrierApplied);
    writeNagHumanV1StateJson(jw, state.base, outputRaw, stopBehavior);
  }
  jw.finish();
  return j;
}

static bool httpParseHumanNm(const String &arg, uint16_t &rawOut) {
  String text = arg;
  text.trim();
  return parseUnsignedCentiPure(text.c_str(), rawOut, 10000u);
}

static void httpNagHumanProfileStats() {
  if (!activeProfileNagTorqueSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }
  server.send(200, "application/json", nagHumanProfileStatsToJson());
}

static void httpNagHumanProfileUpdate() {
  if (!activeProfileNagTorqueSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }


  auto parse16=[&](const char* name,uint16_t& dst)->bool{
    if(!server.hasArg(name)) return true;
    String v=server.arg(name); char* e=nullptr; long n=strtol(v.c_str(),&e,10);
    if(!e||*e!='\0'||n<0||n>65535) return false;
    dst=(uint16_t)n; return true;
  };

{
    NagHumanV4ConfigPure c = nagHumanV4RuntimeConfigSnapshot();
    uint16_t peakMinRaw=c.base.peakMinRaw, peakMaxRaw=c.base.peakMaxRaw;
    uint16_t waitMinMs=c.base.waitMinMs, waitMaxMs=c.base.waitMaxMs;
    uint16_t refMinMs=c.base.refractoryMinMs, refMaxMs=c.base.refractoryMaxMs;
    uint16_t carrierMinRaw=c.carrierMinRaw, carrierMaxRaw=c.carrierMaxRaw;
    uint16_t ho1Raw=c.ho1ThresholdRaw, ho2Raw=c.ho2ThresholdRaw;
    uint16_t visualRescueDelayMs=c.visualRescueDelayMs;
    uint8_t hoPolicy=c.hoPolicy;
    bool visualRescueEnabled=c.visualRescueEnabled;
    if (server.hasArg("peakMinNm") && !httpParseHumanNm(server.arg("peakMinNm"), peakMinRaw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid peakMinNm\"}"); return; }
    if (server.hasArg("peakMaxNm") && !httpParseHumanNm(server.arg("peakMaxNm"), peakMaxRaw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid peakMaxNm\"}"); return; }
    if (server.hasArg("carrierMinNm") && !httpParseHumanNm(server.arg("carrierMinNm"), carrierMinRaw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid carrierMinNm\"}"); return; }
    if (server.hasArg("carrierMaxNm") && !httpParseHumanNm(server.arg("carrierMaxNm"), carrierMaxRaw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid carrierMaxNm\"}"); return; }
    if (server.hasArg("ho1ThresholdNm") && !httpParseHumanNm(server.arg("ho1ThresholdNm"), ho1Raw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid ho1ThresholdNm\"}"); return; }
    if (server.hasArg("ho2ThresholdNm") && !httpParseHumanNm(server.arg("ho2ThresholdNm"), ho2Raw)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid ho2ThresholdNm\"}"); return; }
    if (!parse16("waitMinMs",waitMinMs)||!parse16("waitMaxMs",waitMaxMs)||!parse16("refractoryMinMs",refMinMs)||!parse16("refractoryMaxMs",refMaxMs)||!parse16("visualRescueDelayMs",visualRescueDelayMs)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid Mode H timing\"}"); return; }
    if (server.hasArg("hoPolicy")) { int v=server.arg("hoPolicy").toInt(); if(v<0||v>255||!nagHumanV3HoPolicyValidPure((uint8_t)v)){server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid hoPolicy\"}");return;} hoPolicy=(uint8_t)v; }
    if (server.hasArg("visualRescueEnabled") && !httpBoolArg("visualRescueEnabled", visualRescueEnabled)) { server.send(400,"application/json","{\"ok\":false,\"error\":\"invalid visualRescueEnabled\"}"); return; }
    if (!nagHumanV4RuntimeSetLabTuning(peakMinRaw,peakMaxRaw,waitMinMs,waitMaxMs,refMinMs,refMaxMs,
                                       carrierMinRaw,carrierMaxRaw,hoPolicy,ho1Raw,ho2Raw,
                                       visualRescueEnabled,visualRescueDelayMs)) {
      server.send(400,"application/json","{\"ok\":false,\"error\":\"Rev.4 ranges invalid: peak 1.00..3.00 Nm, carrier 0.10..0.80 Nm, HO thresholds 0.10..3.00 Nm with HO1 <= HO2, wait 0.3..5.0 s, refractory 0.3..2.5 s, rescue delay 0..2.0 s\"}");
      return;
    }
  }
  nagCfgSave();
  server.send(200,"application/json",nagHumanProfileStatsToJson());
}

static void httpNagHumanProfileReset() {
  if (!activeProfileNagTorqueSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }
{
    const NagHumanV4ConfigPure d = nagHumanV4DefaultConfigPure();
    (void)nagHumanV4RuntimeSetLabTuning(d.base.peakMinRaw,d.base.peakMaxRaw,
        d.base.waitMinMs,d.base.waitMaxMs,d.base.refractoryMinMs,d.base.refractoryMaxMs,
        d.carrierMinRaw,d.carrierMaxRaw,d.hoPolicy,d.ho1ThresholdRaw,d.ho2ThresholdRaw,
        d.visualRescueEnabled,d.visualRescueDelayMs);
  }
  nagCfgSave();
  server.send(200, "application/json", nagHumanProfileStatsToJson());
}

static void httpR79ApControlStats() {
  server.send(200, "application/json", r79ApControlStatsToJson());
}

static void httpR79ApControlUpdate() {
  if (!labMenuEnabled) {
    server.send(403, "application/json", "{\"error\":\"lab-disabled\"}");
    return;
  }
  if (!server.hasArg("enabled") || !server.hasArg("mode") ||
      !server.hasArg("delaySeconds") || !server.hasArg("allowManualDriving")) {
    server.send(400, "application/json", "{\"error\":\"missing-ap-control-setting\"}");
    return;
  }
  const String enabled = server.arg("enabled");
  const String mode = server.arg("mode");
  const String delay = server.arg("delaySeconds");
  const String allowManual = server.arg("allowManualDriving");
  uint16_t seconds = 0u;
  if ((enabled != "0" && enabled != "1") ||
      (mode != "0" && mode != "1") ||
      (allowManual != "0" && allowManual != "1") ||
      !r79DelayTextParsePure(delay.c_str(), 10u, seconds) || seconds < 2u) {
    server.send(400, "application/json", "{\"error\":\"invalid-ap-control-setting\"}");
    return;
  }
  const R79ApGateConfigPure next = {enabled == "1", (uint8_t)(mode == "1"), (uint8_t)seconds, allowManual == "1"};
  if (!r79ApControlApply(next)) {
    server.send(503, "application/json", "{\"error\":\"ap-control-save-failed\"}");
    return;
  }
  server.send(200, "application/json", r79ApControlStatsToJson());
}

static void httpR79Stats() { server.send(200, "application/json", r79StatsToJson()); }
static void httpR79Update() {
  if (server.hasArg("hw3Enabled")) {
    // One scalar transaction; do not partially commit a mixed settings request.
    if (server.hasArg("mode") || server.hasArg("bit18Mode") ||
        server.hasArg("mode1TxWaitMode") || server.hasArg("mode1Reinject") ||
        server.hasArg("mode1DelayMs") || server.hasArg("mode2Reinject") ||
        server.hasArg("mode2DelayMs")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"mixed-hw3-settings\"}");
      return;
    }
    const String raw = server.arg("hw3Enabled");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"hw3Enabled-must-be-0-or-1\"}");
      return;
    }
    if (raw == "1" && !activeProfileR79Hw3Supported()) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"hw3-unsupported-profile\"}");
      return;
    }
    if (!r79Hw3Apply(raw == "1")) {
      server.send(503, "application/json", "{\"ok\":false,\"error\":\"hw3-save-failed\"}");
      return;
    }
    server.send(200, "application/json", r79StatsToJson());
    return;
  }
  const bool hasMode = server.hasArg("mode");
  const bool hasBit18 = server.hasArg("bit18Mode");
  const bool hasMode1Wait = server.hasArg("mode1TxWaitMode");
  const bool hasMode1Reinject = server.hasArg("mode1Reinject");
  const bool hasMode1Delay = server.hasArg("mode1DelayMs");
  const bool hasMode2Reinject = server.hasArg("mode2Reinject");
  const bool hasMode2Delay = server.hasArg("mode2DelayMs");
  if (!hasMode && !hasBit18 && !hasMode1Wait && !hasMode1Reinject && !hasMode1Delay &&
      !hasMode2Reinject && !hasMode2Delay) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing-r79-setting\"}");
    return;
  }
  uint8_t nextMode, nextBit18, nextMode1Wait;
  bool nextMode1Reinject, nextMode2Reinject;
  uint16_t nextMode1Delay, nextMode2Delay;
  portENTER_CRITICAL(&r79LabMux);
  nextMode = r79TransportMode;
  nextBit18 = r79Bit18Policy;
  nextMode1Wait = r79Mode1TxWaitMode;
  nextMode1Reinject = r79Mode1ReinjectEnabled;
  nextMode1Delay = r79Mode1DelayMs;
  nextMode2Reinject = r79Mode2ReinjectEnabled;
  nextMode2Delay = r79Mode2DelayMs;
  portEXIT_CRITICAL(&r79LabMux);
  if (hasMode) {
    const String raw = server.arg("mode");
    if (raw != "1" && raw != "2") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode-must-be-1-or-2\"}");
      return;
    }
    nextMode = (uint8_t)raw.toInt();
  }
  if (hasBit18) {
    const String raw = server.arg("bit18Mode");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"bit18Mode-must-be-0-or-1\"}");
      return;
    }
    nextBit18 = (uint8_t)raw.toInt();
  }
  if (hasMode1Wait) {
    const String raw = server.arg("mode1TxWaitMode");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode1TxWaitMode-must-be-0-or-1\"}");
      return;
    }
    nextMode1Wait = (uint8_t)raw.toInt();
  }
  if (hasMode1Reinject) {
    const String raw = server.arg("mode1Reinject");
    if (raw != "0" && raw != "1" && raw != "false" && raw != "true") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid-mode1Reinject\"}");
      return;
    }
    nextMode1Reinject = raw == "1" || raw == "true";
  }
  if (hasMode1Delay) {
    const String rawText = server.arg("mode1DelayMs");
    uint16_t raw = 0u;
    if (!r79DelayTextParsePure(
            rawText.c_str(), R79_FIXED_QUIET_HARD_END_MS_PURE, raw)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode1DelayMs-must-be-0-to-340\"}");
      return;
    }
    nextMode1Delay = raw;
  }
  if (hasMode2Reinject) {
    const String raw = server.arg("mode2Reinject");
    if (raw != "0" && raw != "1" && raw != "false" && raw != "true") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid-mode2Reinject\"}");
      return;
    }
    nextMode2Reinject = raw == "1" || raw == "true";
  }
  if (hasMode2Delay) {
    const String rawText = server.arg("mode2DelayMs");
    uint16_t raw = 0u;
    if (!r79DelayTextParsePure(
            rawText.c_str(), R79_MODE2_DELAY_MAX_MS_PURE, raw)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode2DelayMs-must-be-0-to-2000\"}");
      return;
    }
    nextMode2Delay = raw;
  }

  uint8_t currentMode, currentBit18, currentMode1Wait;
  bool currentMode1Reinject, currentMode2Reinject;
  uint16_t currentMode1Delay, currentMode2Delay;
  portENTER_CRITICAL(&r79LabMux);
  currentMode = r79TransportMode;
  currentBit18 = r79Bit18Policy;
  currentMode1Wait = r79Mode1TxWaitMode;
  currentMode1Reinject = r79Mode1ReinjectEnabled;
  currentMode1Delay = r79Mode1DelayMs;
  currentMode2Reinject = r79Mode2ReinjectEnabled;
  currentMode2Delay = r79Mode2DelayMs;
  portEXIT_CRITICAL(&r79LabMux);
  const bool onlyMode1ReinjectChanged =
      nextMode1Reinject != currentMode1Reinject &&
      nextMode == currentMode && nextBit18 == currentBit18 &&
      nextMode1Wait == currentMode1Wait &&
      nextMode1Delay == currentMode1Delay &&
      nextMode2Reinject == currentMode2Reinject &&
      nextMode2Delay == currentMode2Delay;
  const bool disablingMode1Reinject =
      currentMode1Reinject && !nextMode1Reinject;

  if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(true);
  portENTER_CRITICAL(&r79LabMux);
  r79TransportMode = r79ModeSanitizePure(nextMode);
  r79Bit18Policy = r79Bit18PolicySanitizePure(nextBit18);
  r79Mode1TxWaitMode = r79Mode1WaitModeSanitizePure(nextMode1Wait);
  r79Mode1ReinjectEnabled = nextMode1Reinject;
  r79Mode1DelayMs = r79FixedQuietDelaySanitizePure(nextMode1Delay);
  r79Mode2ReinjectEnabled = nextMode2Reinject;
  r79Mode2DelayMs = r79Mode2DelaySanitizePure(nextMode2Delay);
  r79FixedQuietState = {};
  if (onlyMode1ReinjectChanged) {
    if (disablingMode1Reinject &&
        r79Mode1DisableCancelsRetryPure(
            r79RetryPending, r79RetryOriginKind == R79LAB_TX_PERIODIC)) {
      r79RetryPending = false;
      r79RetryIndex = 0;
      r79RetryOriginKind = R79LAB_TX_NONE;
      r79RetryDueMs = 0;
      r79RetryGeneration = 0u;
    }
  } else {
    r79Mode2DelayedState = {};
    r79RetryPending = false;
    r79RetryIndex = 0;
    r79RetryOriginKind = R79LAB_TX_NONE;
    r79RetryDueMs = 0;
    r79RetryGeneration = 0u;
  }
  portEXIT_CRITICAL(&r79LabMux);
  if (disablingMode1Reinject)
    canTxCancellationGenerationAdvance(&r79Mode1PostMux2Generation);
  r79CfgSave();
  if (!onlyMode1ReinjectChanged) setCanTxAdministrativeHold(false);
  server.send(200, "application/json", r79StatsToJson());
}
static void httpUlcStats() {
  server.send(200, "application/json", ulcStatsToJson());
}

static void httpUlcUpdate() {
  FeatureConfigMigrationPure before = featureConfigRuntimeSnapshot();
  FeatureConfigMigrationPure next = before;
  bool any = false;
  if (server.hasArg("alcOff")) {
    bool enabled;
    if (!httpBoolArg("alcOff", enabled)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid alcOff\"}"); return;
    }
    next.ulc.alcOffHighwayEnabled = enabled; any = true;
  }
  if (server.hasArg("blind")) {
    const String arg = server.arg("blind");
    if (arg == "stock") next.ulc.blindSpotMode = LAB3F8_STOCK;
    else if (arg == "0" || arg == "1" || arg == "2") next.ulc.blindSpotMode = (uint8_t)arg.toInt();
    else { server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid blind\"}"); return; }
    any = true;
  }
  if (server.hasArg("ulcOff")) {
    const String arg = server.arg("ulcOff");
    if (arg == "stock") next.ulc.ulcOffHighwayMode = LAB3F8_STOCK;
    else if (arg == "0" || arg == "1") next.ulc.ulcOffHighwayMode = (uint8_t)arg.toInt();
    else { server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid ulcOff\"}"); return; }
    any = true;
  }
  if (server.hasArg("confirm")) {
    if (!httpBoolArg("confirm", next.ulc.confirmFreeEnabled)) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid confirm\"}"); return;
    }
    if (next.ulc.confirmFreeEnabled && !activeProfileUlcNoConfirmSupported()) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"Confirm-Free unsupported\"}"); return;
    }
    any = true;
  }
  if (server.hasArg("timing")) {
    const String arg = server.arg("timing");
    if (arg != "0" && arg != "1") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid timing\"}"); return;
    }
    next.ulc.confirmFreeTiming = (uint8_t)arg.toInt(); any = true;
  }
  if (!any) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no setting supplied\"}"); return;
  }

  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8AlcMode = next.ulc.alcOffHighwayEnabled ? LAB3F8_ALC_FORCE_ON : LAB3F8_ALC_STOCK;
  lab3f8UlcBlindMode = next.ulc.blindSpotMode;
  lab3f8UlcOffHighwayMode = next.ulc.ulcOffHighwayMode;
  ulcNoConfirmEnabled = next.ulc.confirmFreeEnabled;
  ulcNoConfirmTimingMode = next.ulc.confirmFreeTiming;
  portEXIT_CRITICAL(&lab3f8Mux);
  if (!ulcCfgSave()) {
    portENTER_CRITICAL(&lab3f8Mux);
    lab3f8AlcMode = before.ulc.alcOffHighwayEnabled ? LAB3F8_ALC_FORCE_ON : LAB3F8_ALC_STOCK;
    lab3f8UlcBlindMode = before.ulc.blindSpotMode;
    lab3f8UlcOffHighwayMode = before.ulc.ulcOffHighwayMode;
    ulcNoConfirmEnabled = before.ulc.confirmFreeEnabled;
    ulcNoConfirmTimingMode = before.ulc.confirmFreeTiming;
    portEXIT_CRITICAL(&lab3f8Mux);
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}"); return;
  }
  if (next.ulc.confirmFreeEnabled) {
    portENTER_CRITICAL(&blinkAMux);
    autoBlinkerClearPendingLocked();
    oneShotTurn = STALK_IDLE; oneShotUntil = 0; oneShotReleaseAt = 0; activeTurn = STALK_IDLE;
    portEXIT_CRITICAL(&blinkAMux);
  }
  server.send(200, "application/json", ulcStatsToJson());
}

static void httpAutoLaneChangeLabStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", ulcStatsToJson());
}


static void httpVisionControlStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", getVisionControlStatsJson());
}

static void httpVisionControlUpdate() {
  if (!httpRequireLab()) return;
  if (!visionControlSupported()) {
    server.send(409, "application/json", "{\"error\":\"unsupported-profile\"}"); return;
  }
  const bool hasDisabled = server.hasArg("disabled"), hasBus = server.hasArg("bus");
  if ((!hasDisabled && !hasBus) || server.args() != (int)hasDisabled + (int)hasBus) {
    server.send(400, "application/json", "{\"error\":\"disabled-or-bus-only\"}"); return;
  }
  bool disabled = visionControlRequestDisabled();
  uint8_t bus = visionControlBusSnapshot();
  if (hasDisabled) {
    const String raw = server.arg("disabled");
    if (raw != "0" && raw != "1" && raw != "false" && raw != "true") {
      server.send(400, "application/json", "{\"error\":\"invalid-disabled\"}"); return;
    }
    disabled = raw == "1" || raw == "true";
  }
  if (hasBus) {
    const String raw = server.arg("bus");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"error\":\"invalid-bus\"}"); return;
    }
    bus = raw == "1" ? 1u : 0u;
  }
  if (!visionControlBusSupported(bus)) {
    server.send(409, "application/json", "{\"error\":\"unsupported-bus\"}"); return;
  }
  if (!visionControlApplySettings(disabled, bus)) {
    server.send(503, "application/json", "{\"error\":\"save-failed\"}"); return;
  }
  server.send(200, "application/json", getVisionControlStatsJson());
}


static void httpAutoLaneChangeLabUpdate() {
  if (!httpRequireLab()) return;
  const bool hasEnabled = server.hasArg("enabled");
  const bool hasBus = server.hasArg("bus");
  if (!hasEnabled && !hasBus) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"enabled or bus required\"}"); return;
  }
  FeatureConfigMigrationPure before = featureConfigRuntimeSnapshot();
  bool enabled = before.autoLaneChange.enabled;
  uint8_t targetBus = before.autoLaneChange.bus;
  if (hasEnabled && !httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}"); return;
  }
  if (hasBus) {
    const String arg = server.arg("bus");
    if (arg != "1" && arg != "2" && arg != "3") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid bus\"}"); return;
    }
    targetBus = (uint8_t)arg.toInt();
  }
  portENTER_CRITICAL(&autoLc293Mux);
  uiAutoLaneChangeEnabled = enabled;
  uiAutoLaneChangeTargetBus = targetBus;
  portEXIT_CRITICAL(&autoLc293Mux);
  if (!autoLaneChangeLabCfgSave()) {
    portENTER_CRITICAL(&autoLc293Mux);
    uiAutoLaneChangeEnabled = before.autoLaneChange.enabled;
    uiAutoLaneChangeTargetBus = before.autoLaneChange.bus;
    portEXIT_CRITICAL(&autoLc293Mux);
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}"); return;
  }
  server.send(200, "application/json", ulcStatsToJson());
}

static String countryOverrideStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t mode, mapMode, lastBus, lastPage, lastResult;
  uint16_t lastId;
  uint32_t rx238, rx7ffA, rx7ffB, blocked, txOk, txFail, lastMs;
  bool lastValid;
  uint8_t lastRaw[8];
  portENTER_CRITICAL(&countryOverrideMux);
  mode = countryOverrideMode;
  mapMode = countryOverrideMapMode;
  rx238 = countryOverrideRx238;
  rx7ffA = countryOverrideRx7ffA;
  rx7ffB = countryOverrideRx7ffB;
  blocked = countryOverrideBlocked;
  txOk = countryOverrideTxOk;
  txFail = countryOverrideTxFail;
  lastValid = countryOverrideLastValid;
  lastResult = countryOverrideLastResult;
  lastBus = countryOverrideLastBus;
  lastPage = countryOverrideLastPage;
  lastId = countryOverrideLastId;
  lastMs = countryOverrideLastMs;
  memcpy(lastRaw, countryOverrideLastRaw, sizeof(lastRaw));
  portEXIT_CRITICAL(&countryOverrideMux);

  bool r79StockValid;
  uint32_t r79StockMs, r79StockRx;
  uint8_t r79StockRaw[8];
  portENTER_CRITICAL(&r79LabMux);
  r79StockValid = r79LabStockValid;
  r79StockMs = r79LabLastStockMs;
  r79StockRx = r79Lab3fdRx;
  memcpy(r79StockRaw, r79LabLastStockRaw, sizeof(r79StockRaw));
  portEXIT_CRITICAL(&r79LabMux);

  String s;
  s.reserve(820);
  JsonWriterArduino jw(s);
  jw.boolean("countrySupported",
      vehicleProfileTopologyValid(activeVehicleProfile, activeVehicleTopology));
  jw.i32("countryMode", (int32_t)mode);
  jw.string("countryModeName", mode == COUNTRY_OVERRIDE_US_PURE ? "US" :
      mode == COUNTRY_OVERRIDE_KR_PURE ? "KOREA" :
      mode == COUNTRY_OVERRIDE_NZ_PURE ? "NEW ZEALAND" : "STOCK");
  const bool gate = countryOverrideGateOpen();
  jw.i32("mode", (int32_t)mode);
  jw.string("modeName", mode == 1u ? "US" : mode == 2u ? "KOREA" : mode == 3u ? "NEW ZEALAND" : "STOCK");
  jw.i32("mapMode", (int32_t)mapMode);
  jw.string("mapModeName", mapMode == 1u ? "US" : mapMode == 2u ? "KOREA" : "STOCK");
  jw.boolean("gateAllowed", gate);
  jw.boolean("countryActive", gate && mode != 0u);
  jw.boolean("mapActive", gate && mapMode != 0u);
  jw.string("countryCanAName", activeProfileCanAName());
  jw.string("countryCanBName", activeProfileCanBName());
  jw.boolean("countryGateOpen", countryOverrideGateOpen());
  jw.u32("countryRx238", rx238);
  jw.u32("countryRx7ffA", rx7ffA);
  jw.u32("countryRx7ffB", rx7ffB);
  jw.u32("countryBlocked", blocked);
  jw.u32("countryTxOk", txOk);
  jw.u32("countryTxFail", txFail);
  jw.boolean("countryLastValid", lastValid);
  jw.string("countryLastResult", lastResult == 1u ? "QUEUED" : lastResult == 2u ? "FAILED" : "NONE");
  jw.i32("countryLastBus", (int32_t)lastBus);
  jw.i32("countryLastId", (int32_t)lastId);
  jw.i32("countryLastPage", lastValid && lastPage != 0xFFu ? (int32_t)lastPage : -1);
  jw.u32("countryLastAgeMs", !lastValid || lastMs == 0 ? 999999UL : (uint32_t)(now - lastMs));
  jw.string("countryLastRaw", lastValid ? lab3f8RawHex(lastRaw, 8) : String());
  jw.boolean("r79StockValid", r79StockValid);
  jw.u32("r79StockBit19", r79StockValid ? ((r79StockRaw[2] >> 3) & 0x01u) : 0u);
  jw.u32("r79StockBit47", r79StockValid ? ((r79StockRaw[5] >> 7) & 0x01u) : 0u);
  jw.u32("r79StockAgeMs", !r79StockValid || r79StockMs == 0
      ? 999999UL : (uint32_t)(now - r79StockMs));
  jw.u32("r79StockRx", r79StockRx);
  jw.string("r79StockRaw", r79StockValid ? lab3f8RawHex(r79StockRaw, 8) : String());
  jw.finish();
  return s;
}

static void httpCountryOverrideStats() {
  server.send(200, "application/json", countryOverrideStatsToJson());
}

static void httpCountryOverrideUpdate() {
  if (!vehicleProfileTopologyValid(activeVehicleProfile, activeVehicleTopology)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Invalid vehicle topology\"}"); return;
  }
  const bool hasCountry = server.hasArg("countryMode"), hasLegacy = server.hasArg("mode");
  const bool hasMap = server.hasArg("mapMode");
  if ((!hasCountry && !hasLegacy && !hasMap) || (hasCountry && hasLegacy)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"countryMode or mapMode required\"}"); return;
  }
  uint8_t country = countryOverrideModeSnapshot(), map = countryOverrideMapModeSnapshot();
  if (hasCountry || hasLegacy) {
    const String arg = server.arg(hasCountry ? "countryMode" : "mode");
    if (arg != "0" && arg != "1" && arg != "2" && arg != "3") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid countryMode\"}"); return;
    }
    country = (uint8_t)arg.toInt();
  }
  if (hasMap) {
    const String arg = server.arg("mapMode");
    if (arg != "0" && arg != "1" && arg != "2") {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid mapMode\"}"); return;
    }
    map = (uint8_t)arg.toInt();
  }
  if (!countryOverrideApplySelection(country, map)) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"settings save failed\"}"); return;
  }
  server.send(200, "application/json", countryOverrideStatsToJson());
}

static String laneGraphStatsToJson() {
  const uint32_t now = (uint32_t)millis(), epoch = canTxEpochSnapshot();
  uint8_t mode, bus, lastBit; bool lastValid; uint32_t ok, fail;
  LaneGraphStockPure stock = {};
  portENTER_CRITICAL(&r79LabMux);
  mode = laneGraphMode; bus = laneGraphBus;
  if (laneGraphBusValidPure(bus)) stock = laneGraphStock[bus];
  lastBit = laneGraphLastTxBit45; lastValid = laneGraphLastTxValid;
  ok = laneGraphTxOk; fail = laneGraphTxFail;
  portEXIT_CRITICAL(&r79LabMux);
  portENTER_CRITICAL(&stateMux);
  const bool ap = dasStateApActivePure(dasAutopilotStateValid, dasAutopilotState4);
  const bool fresh = dasAutopilotStateValid && (uint32_t)(now - lastDASStatusMillis) <= LANE_GRAPH_AP_FRESH_MS_PURE;
  portEXIT_CRITICAL(&stateMux);
  const bool supported = vehicleProfileTopologyValid(activeVehicleProfile, activeVehicleTopology);
  const bool busSupported = laneGraphBusSupportedPure(activeVehicleProfile, activeVehicleTopology, bus);
  const bool stockCurrent = stock.valid && stock.epoch == epoch &&
      stock.profile == activeVehicleProfile && stock.topology == activeVehicleTopology;
  const bool stockValid = busSupported && laneGraphStockFreshPure(stock, now, epoch, activeVehicleProfile, activeVehicleTopology);
  const uint8_t stockBit = stockCurrent ? ((stock.raw[5] >> 5) & 1u) : 0u;
  const bool transportReady = !canTxAdministrativeHold &&
      (bus == LANE_GRAPH_BODY_PURE ? mcpReady : mcpChassisReady) &&
      canTxBarrierAllowsMaskedPure(canTxBarrierState, epoch,
          bus == LANE_GRAPH_BODY_PURE ? CAN_TX_FRESH_PARTY : CAN_TX_FRESH_VH);
  const bool active = laneGraphActive() && stockValid && transportReady;
  const char *state = !busSupported ? "UNSUPPORTED" : mode == 0u || !labMenuEnabled ? "OFF" :
      !transportReady ? "TRANSPORT_BLOCKED" : !stockCurrent ? "WAITING" :
      !stockValid ? "STOCK_STALE" : !active ? "AP_INACTIVE" : "ACTIVE";
  String out; out.reserve(600); JsonWriterArduino jw(out);
  jw.u32("mode", mode);
  jw.string("modeName", mode == 1u ? "AP_ACTIVE" : mode == 2u ? "ALWAYS" : "OFF");
  jw.u32("bus", bus); jw.string("busName", bus == 1u ? "BODY" : "CHASSIS");
  jw.boolean("busSelectorVisible", activeVehicleProfile != VEHICLE_MODEL_YL);
  jw.boolean("bodySupported", laneGraphBusSupportedPure(activeVehicleProfile, activeVehicleTopology, LANE_GRAPH_BODY_PURE));
  jw.boolean("busSupported", busSupported);
  jw.string("selectedBusName", bus == LANE_GRAPH_BODY_PURE ? "BODY" : activeVehicleProfile == VEHICLE_MODEL_YL ? "VH" : "CHASSIS");
  jw.u32("selectedBusRx", stock.rx);
  jw.u32("stockAgeMs", stockCurrent ? (uint32_t)(now - stock.lastMs) : 999999u);
  jw.string("state", state);
  jw.boolean("supported", supported); jw.boolean("labEnabled", labMenuEnabled);
  jw.boolean("apActive", ap); jw.boolean("apFresh", fresh); jw.boolean("active", active);
  jw.boolean("stockValid", stockValid); jw.u32("stockBit45", stockBit);
  jw.boolean("effectiveBit45Valid", stockValid); jw.u32("effectiveBit45", active ? 1u : stockBit);
  jw.boolean("lastTxValid", lastValid); jw.u32("lastTxBit45", lastBit);
  jw.u32("txOk", ok); jw.u32("txFail", fail); jw.finish(); return out;
}

static void httpLaneGraphStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", laneGraphStatsToJson());
}

static void httpLaneGraphUpdate() {
  if (!httpRequireLab()) return;
  if (!vehicleProfileTopologyValid(activeVehicleProfile, activeVehicleTopology)) {
    server.send(409, "application/json", "{\"error\":\"unsupported-profile\"}"); return;
  }
  const bool hasMode = server.hasArg("mode"), hasBus = server.hasArg("bus");
  if (!hasMode && !hasBus) {
    server.send(400, "application/json", "{\"error\":\"mode-or-bus-required\"}"); return;
  }
  uint8_t mode, bus;
  portENTER_CRITICAL(&r79LabMux);
  mode = laneGraphMode; bus = laneGraphBus;
  portEXIT_CRITICAL(&r79LabMux);
  if (hasMode) {
    const String raw = server.arg("mode");
    if (raw != "0" && raw != "1" && raw != "2") {
      server.send(400, "application/json", "{\"error\":\"invalid-mode\"}"); return;
    }
    mode = (uint8_t)raw.toInt();
  }
  if (hasBus) {
    const String raw = server.arg("bus");
    if (raw != "0" && raw != "1") {
      server.send(400, "application/json", "{\"error\":\"invalid-bus\"}"); return;
    }
    bus = (uint8_t)raw.toInt();
  }
  if (!laneGraphBusSupportedPure(activeVehicleProfile, activeVehicleTopology, bus)) {
    server.send(409, "application/json", "{\"error\":\"unsupported-bus\"}"); return;
  }
  if (!laneGraphApplySelection(mode, bus)) {
    server.send(503, "application/json", "{\"error\":\"save-failed\"}"); return;
  }
  server.send(200, "application/json", laneGraphStatsToJson());
}

static void httpUlcMonitorLabStats() {
  if (!httpRequireLab()) return;
  server.send(200, "application/json", ulcStatsToJson());
}


static void httpBlinkAStats() {
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkAEnable() {
  if (!setAutoBlinkerEnabled(true, true)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Advanced EAP unavailable for current topology\"}");
    return;
  }
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADisable() {
  setAutoBlinkerEnabled(false, true);
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADelay() {
  if (!activeProfileAdvancedEapSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Advanced EAP unavailable for current topology\"}");
    return;
  }
  int v = server.hasArg("ms") ? server.arg("ms").toInt() : (int)BLINKA_AUTO_DELAY_DEFAULT_MS;
  v = constrain(v, 0, 30000);
  portENTER_CRITICAL(&blinkAMux);
  blinkADelayMs = (uint32_t)v;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkATiming() {
  if (!activeProfileAdvancedEapSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Advanced EAP unavailable for current topology\"}");
    return;
  }
  const bool hasNoa = server.hasArg("noaStabilizationSeconds");
  const bool hasPause = server.hasArg("cancelPauseSeconds");
  if (!hasNoa && !hasPause) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no timing setting supplied\"}");
    return;
  }

  uint8_t previousNoa, nextNoa, nextPause;
  portENTER_CRITICAL(&blinkAMux);
  previousNoa = blinkANoaStabilizationSeconds;
  nextNoa = previousNoa;
  nextPause = blinkACancelPauseSeconds;
  portEXIT_CRITICAL(&blinkAMux);
  if (hasNoa) {
    const int value = server.arg("noaStabilizationSeconds").toInt();
    if (value < BLINKA_NOA_STABILIZE_MIN_S_PURE ||
        value > BLINKA_NOA_STABILIZE_MAX_S_PURE) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"noaStabilizationSeconds must be 1..20\"}");
      return;
    }
    nextNoa = (uint8_t)value;
  }
  if (hasPause) {
    const int value = server.arg("cancelPauseSeconds").toInt();
    if (value < BLINKA_CANCEL_PAUSE_MIN_S_PURE ||
        value > BLINKA_CANCEL_PAUSE_MAX_S_PURE) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"cancelPauseSeconds must be 10..100\"}");
      return;
    }
    nextPause = (uint8_t)value;
  }

  const uint32_t now = (uint32_t)millis();
  portENTER_CRITICAL(&blinkAMux);
  if (hasNoa && nextNoa != previousNoa)
    autoBlinkerNoaReconfigurePure(
        autoBlinkerNoaSessionState, now, nextNoa);
  blinkANoaStabilizationSeconds = nextNoa;
  blinkACancelPauseSeconds = nextPause;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkATxMode() {
  if (!server.hasArg("mode")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing mode\"}");
    return;
  }
  const String modeArg = server.arg("mode");
  if (modeArg != "0" && modeArg != "1") {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid mode\"}");
    return;
  }
  const uint8_t requested = (uint8_t)modeArg.toInt();
  if (!blinkerTxModePersist(requested)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"NVS write failed\"}");
    return;
  }
  if (!blinkerTxSetMode(requested)) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"runtime mode update failed\"}");
    return;
  }
  server.send(200, "application/json", blinkAStatsToJson());
}


// Adaptive dashboard snapshots. These serializers are display-only consumers:
// they never change CAN state, injection decisions, BLE behavior, or recovery state.
// HOME uses one request at the selected 250/500/1000 ms rate and conditionally
// includes slow/on-demand groups so HTTP requests never overlap just to refresh UI.
static String homeFastSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();

  bool apActive, noaRaw, dasValid;
  uint8_t dasState4;
  ApDisplayHoldPure apDisplayHoldLocal;
  portENTER_CRITICAL(&stateMux);
  apActive = gateAPActive;
  noaRaw = gateNOAActive;
  dasValid = dasAutopilotStateValid;
  dasState4 = dasAutopilotState4;
  apDisplayHoldLocal = apDisplayHold;
  portEXIT_CRITICAL(&stateMux);
  const bool canReinitializing = canSubsystemBusy;
  const ApDisplayValuePure apDisplay = apDisplayResolvePure(
      apDisplayHoldLocal, dasValid, dasState4, canReinitializing, now);

  NagContext nagHomeCtx;
  portENTER_CRITICAL(&nagCtxMux); nagHomeCtx = nagCtx; portEXIT_CRITICAL(&nagCtxMux);
  bool nagPauseZero;
  uint8_t nagModeHome;
  uint8_t nagStopBehaviorHome;
  portENTER_CRITICAL(&nagCfgMux);
  nagPauseZero = nagCfg.pauseAtZeroSpeed;
  nagModeHome = nagCfg.mode;
  nagStopBehaviorHome = nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior)
      ? nagCfg.modeHStopBehavior : nagModeHDefaultStopBehaviorPure();
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t nagSpeedAge = nagHomeCtx.lastVehicleSpeedMs ? (uint32_t)(now - nagHomeCtx.lastVehicleSpeedMs) : 999999UL;
  const bool nagSpeedFresh = nagHomeCtx.vehicleSpeedValid && nagHomeCtx.lastVehicleSpeedMs != 0 && nagSpeedAge <= NAG_SPEED_FRESH_MS;
  bool nagHumanPaused = false;
  if (nagModeHome == MODE_H) {
    nagHumanPaused = nagHumanV4RuntimeSnapshot().base.phase == H1_PAUSED_STOPPED;
  }
  const bool nagStoppedGate = nagPauseAtZeroBlocksPure(nagPauseZero, nagHomeCtx.vehicleSpeedValid, nagSpeedFresh, nagHomeCtx.vehicleSpeedRaw) || nagHumanPaused;
  const bool nagStopCarrierActive = nagModeHome == MODE_H && nagHumanPaused && nagStopBehaviorHome == H_STOP_STOCK_CARRIER;

  bool blinkEnabled, blinkCancelPaused;
  AutoBlinkerNoaPhasePure blinkNoaPhase;
  uint32_t blinkNoaRemainingMs, blinkNoaExitRemainingMs;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkNoaPhase = autoBlinkerNoaPhasePure(autoBlinkerNoaSessionState, now);
  blinkNoaRemainingMs = autoBlinkerNoaRemainingMsPure(
      autoBlinkerNoaSessionState, now);
  blinkNoaExitRemainingMs = autoBlinkerNoaExitRemainingMsPure(
      autoBlinkerNoaSessionState, now);
  blinkCancelPaused = autoBlinkerPauseActivePure(
      autoBlinkerCancelPauseState, now);
  portEXIT_CRITICAL(&blinkAMux);
  const bool noaActive = dasValid && noaRaw;

  bool r79LastTxValidLocal;
  uint32_t r79TxOkLocal, r79TxFailLocal, r79LastTxMsLocal;
  portENTER_CRITICAL(&r79LabMux);
  r79LastTxValidLocal = r79LabLastTxValid;
  r79TxOkLocal = r79LabTxOk;
  r79TxFailLocal = r79LabTxFail;
  r79LastTxMsLocal = r79LabLastTxMs;
  portEXIT_CRITICAL(&r79LabMux);
  const R79RuntimeStatus r79RuntimeLocal = r79RuntimeStatusSnapshot(now);
  const char *r79TxReasonLocal = r79RuntimeReasonNameForUi(r79RuntimeLocal);

  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  McpChassisStatus chassisHome = {};
  const bool chassisHomeOk = mcpChassisGetStatus(&chassisHome) == ESP_OK;

  String j; j.reserve(820);
  JsonWriterArduino jw(j);
  jw.beginObject("nag");
  jw.fixed("torque", (int32_t)nagRealTorqueCenti, 2u);
  jw.u32("handsOnState", nagHomeCtx.handsOnState);
  jw.boolean("stoppedGate", nagStoppedGate);
  jw.boolean("stopCarrierActive", nagStopCarrierActive);
  jw.boolean("apActive", dasValid && apActive);
  jw.i32("canAState", (int32_t)mcpState);
  jw.endObject();
  jw.beginObject("blink");
  jw.boolean("enabled", blinkEnabled);
  jw.boolean("noaActive", noaActive);
  jw.u32("noaSessionState", (uint32_t)blinkNoaPhase);
  jw.string("noaSessionStateName",
            autoBlinkerNoaSessionStateName(blinkNoaPhase));
  jw.u32("noaStabilizationRemainingMs", blinkNoaRemainingMs);
  jw.u32("noaExitRemainingMs", blinkNoaExitRemainingMs);
  jw.boolean("cancelPaused", blinkCancelPaused);
  jw.boolean("dasStateValid", dasValid);
  jw.u32("dasState", dasState4);
  jw.boolean("displayStateValid", apDisplay.valid);
  jw.u32("displayState", apDisplay.state);
  jw.boolean("canReinitializing", canReinitializing);
  jw.endObject();
  jw.beginObject("summon");
  jw.i32("canState", chassisHomeOk ? (int32_t)chassisHome.state : -1);
  jw.string("canStateName", chassisHomeOk ? mcpChassisStateName(chassisHome.state) : "UNAVAILABLE");
  jw.endObject();
  jw.beginObject("cantraffic");
  jw.boolean("bodyTrafficSeen", traffic.bodySeen);
  jw.boolean("bodyTrafficOnline", traffic.bodyOnline);
  jw.u32("bodyTrafficAgeMs", traffic.bodyAgeMs);
  jw.boolean("chassisTrafficSeen", traffic.chassisSeen);
  jw.boolean("chassisTrafficOnline", traffic.chassisOnline);
  jw.u32("chassisTrafficAgeMs", traffic.chassisAgeMs);
  jw.boolean("partyTrafficSeen", traffic.partySeen);
  jw.boolean("partyTrafficOnline", traffic.partyOnline);
  jw.u32("partyTrafficAgeMs", traffic.partyAgeMs);
  // Legacy dashboard keys retained for clients that still request them.
  jw.boolean("mcpTrafficSeen", traffic.bodySeen);
  jw.boolean("mcpTrafficOnline", traffic.bodyOnline);
  jw.u32("mcpTrafficAgeMs", traffic.bodyAgeMs);
  jw.endObject();
  jw.beginObject("r79");
  jw.string("txState", r79RuntimeStateName(r79RuntimeLocal.state));
  jw.u32("apWaitRemainingMs", r79RuntimeLocal.apGate.remainingMs);
  jw.string("apGateReason", r79ApGateReasonName(r79RuntimeLocal.apGate.reason));
  jw.string("txReason", r79TxReasonLocal);
  jw.string("gearName", r79RuntimeLocal.gearValid ? r79GearName(r79RuntimeLocal.gearRaw) : "UNKNOWN");
  jw.u32("dasState", r79RuntimeLocal.dasState4);
  jw.boolean("dasStateValid", r79RuntimeLocal.dasValid);
  jw.u32("txOk", r79TxOkLocal);
  jw.u32("txFail", r79TxFailLocal);
  jw.boolean("lastTxValid", r79LastTxValidLocal);
  jw.u32("lastTxAgeMs", r79LastTxMsLocal ? now - r79LastTxMsLocal : 999999UL);
  jw.endObject();
  jw.finish();
  return j;
}

static String homeSlowSnapshotToJson() {
  bool blinkEnabled, tlssc;
  uint32_t blinkDelayMsLocal;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkDelayMsLocal = blinkADelayMs;
  portEXIT_CRITICAL(&blinkAMux);
  portENTER_CRITICAL(&stateMux); tlssc = tlsscEnabled; portEXIT_CRITICAL(&stateMux);

  bool btEnabled;
  uint8_t s3Paired = 0, s3Connected = 0;
  portENTER_CRITICAL(&s3xyMux);
  btEnabled = s3xyBluetoothEnabled;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    s3Paired++;
    if (s3xyDevices[i].connected) s3Connected++;
  }
  portEXIT_CRITICAL(&s3xyMux);

  uint8_t r79Smart, alcModeLocal;
  portENTER_CRITICAL(&r79LabMux); r79Smart = r79Bit18Policy; portEXIT_CRITICAL(&r79LabMux);
  portENTER_CRITICAL(&lab3f8Mux); alcModeLocal = lab3f8AlcMode; portEXIT_CRITICAL(&lab3f8Mux);

  String j; j.reserve(360);
  JsonWriterArduino jw(j);
  jw.beginObject("blink");
  jw.boolean("enabled", blinkEnabled);
  jw.u32("delayMs", blinkDelayMsLocal);
  jw.endObject();
  jw.beginObject("summon");
  jw.boolean("tlssc", tlssc);
  jw.endObject();
  jw.beginObject("s3xy");
  jw.boolean("bluetoothEnabled", btEnabled);
  jw.u32("pairedCount", s3Paired);
  jw.u32("connectedCount", s3Connected);
  jw.endObject();
  jw.beginObject("r79");
  jw.u32("smartMode", r79Smart);
  jw.endObject();
  jw.beginObject("lab3f8");
  jw.u32("alcMode", alcModeLocal);
  jw.endObject();
  jw.finish();
  return j;
}

static String settingsLiteSnapshotToJson() {
  bool blinkEnabled, tlssc, summonSession;
  uint32_t blinkDelayMsLocal;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkDelayMsLocal = blinkADelayMs;
  portEXIT_CRITICAL(&blinkAMux);
  portENTER_CRITICAL(&stateMux);
  tlssc = tlsscEnabled;
  summonSession = gateSummoning;
  portEXIT_CRITICAL(&stateMux);

  bool btEnabled, autoEnabled;
  uint8_t paired = 0, connected = 0;
  portENTER_CRITICAL(&s3xyMux);
  btEnabled = s3xyBluetoothEnabled;
  autoEnabled = s3xyAutoEnabled;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    paired++;
    if (s3xyDevices[i].connected) connected++;
  }
  portEXIT_CRITICAL(&s3xyMux);

  uint8_t alcModeLocal;
  portENTER_CRITICAL(&lab3f8Mux); alcModeLocal = lab3f8AlcMode; portEXIT_CRITICAL(&lab3f8Mux);

  String j; j.reserve(500);
  JsonWriterArduino jw(j);
  jw.beginObject("system");
  jw.string("fwVersion", FW_VERSION);
  jw.endObject();
  jw.beginObject("blink");
  jw.boolean("enabled", blinkEnabled);
  jw.u32("delayMs", blinkDelayMsLocal);
  jw.endObject();
  jw.beginObject("summon");
  jw.boolean("tlssc", tlssc);
  jw.boolean("sessionActive", summonSession);
  jw.endObject();
  jw.beginObject("s3xy");
  jw.boolean("bluetoothEnabled", btEnabled);
  jw.boolean("autoEnabled", autoEnabled);
  jw.u32("pairedCount", paired);
  jw.u32("connectedCount", connected);
  jw.endObject();
  jw.beginObject("lab3f8");
  jw.u32("alcMode", alcModeLocal);
  jw.endObject();
  jw.finish();
  return j;
}

static String labLiteSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t dasState4;
  bool dasStateValid;
  uint32_t dasLast;
  portENTER_CRITICAL(&stateMux);
  dasState4 = dasAutopilotState4;
  dasStateValid = dasAutopilotStateValid;
  dasLast = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);
  const R79RuntimeStatus r79Runtime = r79RuntimeStatusSnapshot(now);
  const char *r79TxReason = r79RuntimeReasonNameForUi(r79Runtime);
  bool dmsNagEnabled, dmsStockValid, dmsTxValid;
  uint8_t dmsStockBit43, dmsTxBit43;
  portENTER_CRITICAL(&nagCfgMux);
  dmsNagEnabled = nagCfg.dmsControlEnabled;
  portEXIT_CRITICAL(&nagCfgMux);
  portENTER_CRITICAL(&r79LabMux);
  dmsStockValid = r79LabStockValid;
  dmsTxValid = r79LabLastTxValid;
  dmsStockBit43 = r79LabStockCabinCamera;
  dmsTxBit43 = r79LabEffectiveCabinCamera;
  portEXIT_CRITICAL(&r79LabMux);

  ResearchCaptureLatest lane239 = {};
  ResearchCaptureLatest alc399 = {};
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureLatest) {
    lane239 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x239)];
    alc399 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x399)];
  }
  portEXIT_CRITICAL(&researchCaptureMux);

  const bool alcValid = alc399.valid && alc399.dlc >= 7;
  const DasLane239Decoded laneDecoded = dasLane239DecodePure(lane239.data, lane239.dlc);
  const uint8_t alcRaw = alcValid ? das399ReadAlcPure(alc399.data, alc399.dlc) : 0xFF;
  const uint32_t laneAge = lane239.valid ? (uint32_t)(now - lane239.lastSeenMs) : 999999UL;
  const uint32_t alcAge = alc399.valid ? (uint32_t)(now - alc399.lastSeenMs) : 999999UL;

  String j; j.reserve(680);
  JsonWriterArduino jw(j);
  jw.beginObject("r79");
  jw.string("txState", r79RuntimeStateName(r79Runtime.state));
  jw.u32("apWaitRemainingMs", r79Runtime.apGate.remainingMs);
  jw.string("apGateReason", r79ApGateReasonName(r79Runtime.apGate.reason));
  jw.string("txReason", r79TxReason);
  jw.boolean("txEnabled", r79Runtime.state == R79_TX_STATE_ACTIVE);
  jw.boolean("manualSuppressed", r79Runtime.decision.manualSuppressed);
  jw.u32("gearRaw", r79Runtime.gearRaw);
  jw.string("gearName", r79Runtime.gearValid ? r79GearName(r79Runtime.gearRaw) : "UNKNOWN");
  jw.u32("dasState", dasState4);
  jw.boolean("dasStateValid", dasStateValid);
  jw.u32("dasAgeMs", dasLast ? now - dasLast : 999999UL);
  jw.endObject();
  jw.beginObject("dmsNag");
  jw.boolean("supported", activeProfileDmsNagSupported());
  jw.boolean("enabled", dmsNagEnabled);
  jw.boolean("active", r79DmsNagActive());
  jw.boolean("stockValid", dmsStockValid);
  jw.u32("stockBit43", dmsStockBit43);
  jw.boolean("txValid", dmsTxValid);
  jw.u32("txBit43", dmsTxBit43);
  jw.endObject();
  jw.beginObject("canARx");
  writeCanARxLabJson(jw);
  jw.endObject();
  jw.beginObject("alc");
  jw.boolean("alcValid", alcValid);
  jw.u32("alcRaw", alcRaw);
  jw.u32("alcAgeMs", alcAge);
  jw.boolean("lane239Valid", laneDecoded.valid);
  jw.u32("lane239AgeMs", laneAge);
  jw.u32("leftLaneExists", laneDecoded.valid ? (laneDecoded.leftLaneExists ? 1u : 0u) : 255u);
  jw.u32("rightLaneExists", laneDecoded.valid ? (laneDecoded.rightLaneExists ? 1u : 0u) : 255u);
  jw.u32("leftLineUsageRaw", laneDecoded.valid ? laneDecoded.leftLineUsage : 255u);
  jw.u32("rightLineUsageRaw", laneDecoded.valid ? laneDecoded.rightLineUsage : 255u);
  jw.u32("leftForkRaw", laneDecoded.valid ? laneDecoded.leftFork : 255u);
  jw.u32("rightForkRaw", laneDecoded.valid ? laneDecoded.rightFork : 255u);
  jw.endObject();
  jw.finish();
  return j;
}

// Lightweight HOME snapshot. The HOME page is polled at 250/500/1000 ms,
// so avoid building the full diagnostics payloads that are only needed inside
// feature/detail panels. This keeps visible HOME behavior unchanged while
// reducing transient String allocation, JSON serialization and Wi-Fi payload.
static String homeSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();

  // Shared vehicle/AP/Summon state in one short stateMux snapshot.
  bool apActive, noaRaw, dasValid, parked, summon, aca, spr, tlssc;
  uint8_t dasState4;
  uint32_t sumTxOkLocal, sumTxFailLocal;
  portENTER_CRITICAL(&stateMux);
  apActive = gateAPActive;
  noaRaw = gateNOAActive;
  dasValid = dasAutopilotStateValid;
  dasState4 = dasAutopilotState4;
  parked = gateParked;
  summon = gateSummoning;
  aca = lastAca;
  spr = sprSeen;
  tlssc = tlsscEnabled;
  sumTxOkLocal = sumTxOk;
  sumTxFailLocal = sumTxFail;
  portEXIT_CRITICAL(&stateMux);

  // NAG HOME telemetry only.
  NagContext nagHomeCtx;
  portENTER_CRITICAL(&nagCtxMux); nagHomeCtx = nagCtx; portEXIT_CRITICAL(&nagCtxMux);
  uint32_t nagTxOkLocal, nagTxFailLocal, nagLastTxLocal, nagMaxGapLocal;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOkLocal = nagTxOk; nagTxFailLocal = nagTxFail;
  nagLastTxLocal = nagLastTxOkMs; nagMaxGapLocal = nagMaxTxGapMs;
  portEXIT_CRITICAL(&nagDiagMux);
  bool nagPauseZero;
  uint8_t nagModeHome;
  uint8_t nagStopBehaviorHome;
  portENTER_CRITICAL(&nagCfgMux);
  nagPauseZero = nagCfg.pauseAtZeroSpeed;
  nagModeHome = nagCfg.mode;
  nagStopBehaviorHome = nagModeHStopBehaviorValidPure(nagCfg.modeHStopBehavior)
      ? nagCfg.modeHStopBehavior : nagModeHDefaultStopBehaviorPure();
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t nagSpeedAge = nagHomeCtx.lastVehicleSpeedMs ? (uint32_t)(now - nagHomeCtx.lastVehicleSpeedMs) : 999999UL;
  const bool nagSpeedFresh = nagHomeCtx.vehicleSpeedValid && nagHomeCtx.lastVehicleSpeedMs != 0 && nagSpeedAge <= NAG_SPEED_FRESH_MS;
  bool nagHumanPaused = false;
  if (nagModeHome == MODE_H) {
    nagHumanPaused = nagHumanV4RuntimeSnapshot().base.phase == H1_PAUSED_STOPPED;
  }
  const bool nagStoppedGate = nagPauseAtZeroBlocksPure(nagPauseZero, nagHomeCtx.vehicleSpeedValid, nagSpeedFresh, nagHomeCtx.vehicleSpeedRaw) || nagHumanPaused;
  const bool nagStopCarrierActive = nagModeHome == MODE_H && nagHumanPaused && nagStopBehaviorHome == H_STOP_STOCK_CARRIER;

  // Auto Blinker HOME telemetry only.
  bool blinkEnabled, blinkCancelPaused;
  AutoBlinkerNoaPhasePure blinkNoaPhase;
  uint32_t blinkDelayMsLocal, blinkRx249Local;
  uint32_t blinkNoaRemainingMs, blinkNoaExitRemainingMs;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkDelayMsLocal = blinkADelayMs;
  blinkRx249Local = rx249;
  blinkNoaPhase = autoBlinkerNoaPhasePure(autoBlinkerNoaSessionState, now);
  blinkNoaRemainingMs = autoBlinkerNoaRemainingMsPure(
      autoBlinkerNoaSessionState, now);
  blinkNoaExitRemainingMs = autoBlinkerNoaExitRemainingMsPure(
      autoBlinkerNoaSessionState, now);
  blinkCancelPaused = autoBlinkerPauseActivePure(
      autoBlinkerCancelPauseState, now);
  portEXIT_CRITICAL(&blinkAMux);
  const bool noaActive = dasValid && noaRaw;

  // S3XY HOME telemetry only; no device registry JSON or diagnostics. Snapshot
  // the three-device registry in one short lock instead of taking separate
  // locks for master/registered/connected counts every HOME poll.
  bool btEnabled;
  uint8_t s3Paired = 0, s3Connected = 0;
  portENTER_CRITICAL(&s3xyMux);
  btEnabled = s3xyBluetoothEnabled;
  for (uint8_t i = 0; i < S3XY_MAX_DEVICES; i++) {
    if (!s3xyDevices[i].used) continue;
    s3Paired++;
    if (s3xyDevices[i].connected) s3Connected++;
  }
  portEXIT_CRITICAL(&s3xyMux);
  uint32_t ulcTxOkLocal, ulcTxFailLocal;
  portENTER_CRITICAL(&ulcSnoozeMux);
  ulcTxOkLocal = ulcSnoozeTxOk;
  ulcTxFailLocal = ulcSnoozeTxFail;
  portEXIT_CRITICAL(&ulcSnoozeMux);

  // R79 HOME telemetry only.
  uint8_t r79Smart;
  bool r79LastTxValidLocal;
  uint32_t r79TxOkLocal, r79TxFailLocal, r79LastTxMsLocal;
  portENTER_CRITICAL(&r79LabMux);
  r79Smart = r79Bit18Policy;
  r79LastTxValidLocal = r79LabLastTxValid;
  r79TxOkLocal = r79LabTxOk;
  r79TxFailLocal = r79LabTxFail;
  r79LastTxMsLocal = r79LabLastTxMs;
  portEXIT_CRITICAL(&r79LabMux);
  const R79RuntimeStatus r79RuntimeLocal = r79RuntimeStatusSnapshot(now);
  const char *r79TxReasonLocal = r79RuntimeReasonNameForUi(r79RuntimeLocal);

  uint8_t alcModeLocal;
  portENTER_CRITICAL(&lab3f8Mux); alcModeLocal = lab3f8AlcMode; portEXIT_CRITICAL(&lab3f8Mux);

  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  McpChassisStatus chassisHome = {};
  const bool chassisHomeOk = mcpChassisGetStatus(&chassisHome) == ESP_OK;

  String j;
  j.reserve(1550);
  JsonWriterArduino jw(j);
  jw.beginObject("nag");
  jw.fixed("torque", (int32_t)nagRealTorqueCenti, 2u);
  jw.u32("ho", nagRealHo);
  jw.fixed("injNm", (int32_t)nagLastInjectedCenti, 2u);
  jw.u32("injHo", nagLastInjectedHo);
  jw.u32("rx", nagRxFrames);
  jw.u32("txOk", nagTxOkLocal);
  jw.u32("txFail", nagTxFailLocal);
  jw.u32("lastTxAgeMs", nagLastTxLocal ? now - nagLastTxLocal : 999999UL);
  jw.u32("maxTxGapMs", nagMaxGapLocal);
  jw.boolean("stoppedGate", nagStoppedGate);
  jw.boolean("stopCarrierActive", nagStopCarrierActive);
  jw.boolean("apActive", dasValid && apActive);
  jw.i32("canAState", (int32_t)mcpState);
  jw.endObject();

  jw.beginObject("blink");
  jw.boolean("enabled", blinkEnabled);
  jw.u32("delayMs", blinkDelayMsLocal);
  jw.boolean("noaActive", noaActive);
  jw.u32("noaSessionState", (uint32_t)blinkNoaPhase);
  jw.string("noaSessionStateName",
            autoBlinkerNoaSessionStateName(blinkNoaPhase));
  jw.u32("noaStabilizationRemainingMs", blinkNoaRemainingMs);
  jw.u32("noaExitRemainingMs", blinkNoaExitRemainingMs);
  jw.boolean("cancelPaused", blinkCancelPaused);
  jw.boolean("dasStateValid", dasValid);
  jw.u32("dasState", dasState4);
  jw.u32("rx249", blinkRx249Local);
  jw.endObject();

  jw.beginObject("summon");
  jw.boolean("tlssc", tlssc);
  jw.boolean("sessionActive", summon);
  jw.boolean("loadSheddingActive", summon);
  jw.boolean("parked", parked);
  jw.boolean("aca", aca);
  jw.boolean("spr", spr);
  jw.u32("txQueueNow", chassisTxQueueNow);
  jw.u32("txQueueMax", chassisTxQueueMax);
  jw.u32("txOk", sumTxOkLocal);
  jw.u32("txFail", sumTxFailLocal);
  jw.i32("canState", chassisHomeOk ? (int32_t)chassisHome.state : -1);
  jw.string("canStateName", chassisHomeOk ? mcpChassisStateName(chassisHome.state) : "UNAVAILABLE");
  jw.endObject();

  jw.beginObject("s3xy");
  jw.boolean("bluetoothEnabled", btEnabled);
  jw.u32("pairedCount", s3Paired);
  jw.u32("connectedCount", s3Connected);
  jw.u32("ulcTxOk", ulcTxOkLocal);
  jw.u32("ulcTxFail", ulcTxFailLocal);
  jw.endObject();

  jw.beginObject("cantraffic");
  jw.boolean("mcpTrafficSeen", traffic.bodySeen);
  jw.boolean("mcpTrafficOnline", traffic.bodyOnline);
  jw.u32("mcpTrafficAgeMs", traffic.bodyAgeMs);
  jw.boolean("chassisTrafficSeen", traffic.chassisSeen);
  jw.boolean("chassisTrafficOnline", traffic.chassisOnline);
  jw.u32("chassisTrafficAgeMs", traffic.chassisAgeMs);
  jw.endObject();

  jw.beginObject("r79");
  jw.u32("smartMode", r79Smart);
  jw.string("txState", r79RuntimeStateName(r79RuntimeLocal.state));
  jw.u32("apWaitRemainingMs", r79RuntimeLocal.apGate.remainingMs);
  jw.string("apGateReason", r79ApGateReasonName(r79RuntimeLocal.apGate.reason));
  jw.string("txReason", r79TxReasonLocal);
  jw.boolean("txEnabled", r79RuntimeLocal.state == R79_TX_STATE_ACTIVE);
  jw.boolean("manualSuppressed", r79RuntimeLocal.decision.manualSuppressed);
  jw.u32("gearRaw", r79RuntimeLocal.gearRaw);
  jw.string("gearName", r79RuntimeLocal.gearValid ? r79GearName(r79RuntimeLocal.gearRaw) : "UNKNOWN");
  jw.u32("dasState", r79RuntimeLocal.dasState4);
  jw.boolean("dasStateValid", r79RuntimeLocal.dasValid);
  jw.boolean("summonSessionActive", r79RuntimeLocal.summonSessionActive);
  jw.boolean("remoteStartupEvidence", r79RuntimeLocal.remoteStartupEvidence);
  jw.boolean("manualLatchActive", r79RuntimeLocal.manualLatchActive);
  jw.u32("txOk", r79TxOkLocal);
  jw.u32("txFail", r79TxFailLocal);
  jw.boolean("lastTxValid", r79LastTxValidLocal);
  jw.u32("lastTxAgeMs", r79LastTxMsLocal ? now - r79LastTxMsLocal : 999999UL);
  jw.endObject();

  jw.beginObject("lab3f8");
  jw.u32("alcMode", alcModeLocal);
  jw.endObject();
  jw.finish();
  return j;
}

static bool snapshotGroupRequested(const String &groups, const char *name) {
  if (!name || !name[0]) return false;
  if (groups == "all") return true;
  const int len = (int)groups.length();
  const int nameLen = (int)strlen(name);
  int pos = 0;
  while ((pos = groups.indexOf(name, pos)) >= 0) {
    const int end = pos + nameLen;
    const bool leftOk = (pos == 0) || groups.charAt(pos - 1) == ',';
    const bool rightOk = (end == len) || groups.charAt(end) == ',';
    if (leftOk && rightOk) return true;
    pos = end;
  }
  return false;
}

static void snapshotSendGroup(bool &first, const char *name, const String &payload) {
  if (!first) server.sendContent(",");
  server.sendContent("\"");
  server.sendContent(name);
  server.sendContent("\":");
  server.sendContent(payload);
  first = false;
}

using SnapshotJsonBuilder = String (*)();
struct SnapshotGroupSpec {
  const char *name;
  SnapshotJsonBuilder build;
};

static const SnapshotGroupSpec SNAPSHOT_GROUP_SPECS[] = {
  {"nag", nagStatsToJson},
  {"blink", blinkAStatsToJson},
  {"das", dasTelemetryStatsToJson},
  {"summon", summonStatsToJson},
  {"s3xy", s3xyStatsToJson},
  {"system", systemStatsToJson},
  {"cantraffic", canTrafficStatsToJson},
  {"ulc", ulcStatsToJson},
  {"r79", r79StatsToJson},
  {"researchcapture", researchCaptureStatsToJson},
};
static constexpr size_t SNAPSHOT_GROUP_SPEC_COUNT = sizeof(SNAPSHOT_GROUP_SPECS) / sizeof(SNAPSHOT_GROUP_SPECS[0]);

static void httpSnapshot() {
  String groups;
  groups.reserve(80);
  groups = server.hasArg("groups") ? server.arg("groups") : "home";
  if (groups == "home-fast") {
    const bool includeSlow = server.hasArg("slow") && server.arg("slow") == "1";
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent("{\"fast\":");
    server.sendContent(homeFastSnapshotToJson());
    if (includeSlow) { server.sendContent(",\"slow\":"); server.sendContent(homeSlowSnapshotToJson()); }
    server.sendContent("}");
    server.sendContent("");
    return;
  }
  if (groups == "lab-lite") {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", labLiteSnapshotToJson());
    return;
  }
  if (groups == "settings-lite") {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", settingsLiteSnapshotToJson());
    return;
  }
  if (groups == "heartbeat") {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  if (groups == "home-lite") {
    // Backward compatibility for cached v3.3b4 dashboards.
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", homeSnapshotToJson());
    return;
  }
  if (groups == "home") groups = "nag,blink,summon,s3xy,cantraffic,r79,lab3f8";
  else if (groups == "lab") groups = "lab3f8,r79,researchcapture";
  else if (groups == "settings") groups = "system,s3xy,lab3f8";

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{");
  bool first = true;
  for (size_t i = 0; i < SNAPSHOT_GROUP_SPEC_COUNT; ++i) {
    const SnapshotGroupSpec &spec = SNAPSHOT_GROUP_SPECS[i];
    if (snapshotGroupRequested(groups, spec.name)) snapshotSendGroup(first, spec.name, spec.build());
  }
  server.sendContent("}");
  server.sendContent("");
}

// Reset only diagnostic/session counters. No NVS/configuration, live feature state,
// CAN liveness timestamps, or mcpRxCount warmup state is modified.
static bool resetRuntimeStats() {
  const bool durableCleared = canBusOffPersistenceClear();
  if (!durableCleared) return false;
  nagRxFrames = 0;
  nagEchoCount = 0;
  mcpTxOk = 0;
  mcpTxFail = 0;
  mcpRxOverflowReset();
  nagEchoLatUs = 0;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOk=nagTxFail=0; nagSkipDisabled=nagSkipBootDelay=nagSkipWarmup=nagSkipSelfFrame=0;
  nagSkipHandsOn=nagSkipApInvalid=nagSkipApInactive=nagSkipDecision=nagSkipStopped=nagSkipSpeedStale=nagStopCarrierTxOk=0;
  nagBlockMutex=nagBlockMcpNotReady=nagBlockEpoch=nagBlockFreshMask=nagBlockInvalidMsg=nagSendError=0;
  nagLastTxOkMs=nagMaxTxGapMs=nagSessionTxOk=0; nagSessionStartMs=0;
  nagLastSkipReason=NAG_SKIP_NONE; nagLastSkipMs=0; nagLastTxBlockReason=MCP_TX_OK; nagLastTxBlockMs=0;
  portEXIT_CRITICAL(&nagDiagMux);

  portENTER_CRITICAL(&stateMux);
  sumTxOk = 0;
  sumTxFail = 0;
  sumRx280 = 0;
  sumRx390 = 0;
  sumRx921 = 0;
  sumRx1016 = 0;
  summonConfirmedGearTransitions = 0;
  portEXIT_CRITICAL(&stateMux);

  portENTER_CRITICAL(&r79LabMux);
  r79LabTxOk = 0;
  r79LabTxFail = 0;
  r79LabPeriodicTxOk = 0;
  r79LabPeriodicTxFail = 0;
  r79RetryScheduled = 0;
  r79RetryTxOk = 0;
  r79RetryTxFail = 0;
  r79RetryExhausted = 0;
  r79LastRetryMs = 0;
  r79EmergencyQueueFlushCount = 0;
  r79FlushTriggeredRetryOk = 0;
  r79FlushTriggeredRetryFail = 0;
  r79LabBit43Rx0 = 0;
  r79LabBit43Rx1 = 0;
  r79LabBit43Changes = 0;
  r79DmsOnlyTxOk = 0;
  r79DmsOnlyTxFail = 0;
  r79DmsOnlyBlockedByR79 = 0;
  r79QuietArmCount = 0;
  r79QuietFireCount = 0;
  r79QuietGuardSkip = 0;
  r79FixedQuietState = {};
  r79FastEchoAttempts = 0;
  r79FastEchoTxOk = 0;
  r79FastEchoTxFail = 0;
  portEXIT_CRITICAL(&r79LabMux);

  portENTER_CRITICAL(&blinkAMux);
  rx249 = 0;
  blkATxOk = 0;
  blkATxFail = 0;
  blinkerTxRequestCount = 0;
  blinkerTxBlockedCount = 0;
  blinkerTxLastDir = 0;
  blinkerTxLastSource = BLINKER_TX_SOURCE_NONE_PURE;
  blinkerTxLastResult = 0;
  autoRetryCount = 0;
  portEXIT_CRITICAL(&blinkAMux);
  portENTER_CRITICAL(&driverWindowLabMux);
  driverWindowLabRequests = 0;
  driverWindowLabCompleted = 0;
  driverWindowLabTxOk = 0;
  driverWindowLabTxFail = 0;
  driverWindowLabBlocked = 0;
  driverWindowLabLastTxValid = false;
  driverWindowLabLastTxMs = 0;
  driverWindowLabLastResult = DRIVER_WINDOW_RESULT_IDLE;
  memset(driverWindowLabLastTxRaw, 0, sizeof(driverWindowLabLastTxRaw));
  portEXIT_CRITICAL(&driverWindowLabMux);
  tsl9InputResetCounters();
  visualDebugRxCount = 0;

  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8TxOk = 0;
  lab3f8TxFail = 0;
  lab3f8GateBlocked = 0;
  ulcOffHighwayTxOk = ulcOffHighwayTxFail = ulcOffHighwayGateBlocked = 0;
  ulcOffHighwayLastTxValid = false;
  ulcOffHighwayLastTxMs = 0;
  ulcNoConfirmTxOk = 0;
  ulcNoConfirmTxFail = 0;
  ulcNoConfirmTxBOk = ulcNoConfirmTxBFail = 0;
  ulcNoConfirmGateBlocked = 0;
  ulcNoConfirmGateBlockedB = 0;
  ulcNoConfirmLastTxValid = false;
  ulcNoConfirmLastTxMs = 0;
  lab3f8CanBRx = 0;
  lab3f8CanBValid = false;
  lab3f8CanBLastMs = 0;
  lab3f8CanBPeriodMs = 0;
  portEXIT_CRITICAL(&lab3f8Mux);
  portENTER_CRITICAL(&autoLc293Mux);
  uiAutoLaneChangeRxA = uiAutoLaneChangeRxB = 0;
  uiAutoLaneChangeTxAOk = uiAutoLaneChangeTxAFail = 0;
  uiAutoLaneChangeTxBOk = uiAutoLaneChangeTxBFail = 0;
  uiAutoLaneChangeGateBlockedA = uiAutoLaneChangeGateBlockedB = 0;
  uiAutoLaneChangeLastTxValid = false;
  uiAutoLaneChangeLastTxMs = 0;
  portEXIT_CRITICAL(&autoLc293Mux);

  portENTER_CRITICAL(&countryOverrideMux);
  countryOverrideRx238 = 0;
  countryOverrideRx7ffA = countryOverrideRx7ffB = 0;
  countryOverrideBlocked = 0;
  countryOverrideTxOk = countryOverrideTxFail = 0;
  countryOverrideLastValid = false;
  countryOverrideLastResult = 0;
  countryOverrideLastMs = 0;
  portEXIT_CRITICAL(&countryOverrideMux);


  portENTER_CRITICAL(&roadContextMux);
  tlsscHighwayGateBlockedCount = 0;
  tlsscHighwayTransitions = 0;
  portEXIT_CRITICAL(&roadContextMux);

  mcpChassisReadQueueStatus();
  chassisTxQueueMax = chassisTxQueueNow;
  chassisRxQueueMax = chassisRxQueueNow;
  chassisNonSummonShed = 0;
  chassisParkSoftShed = 0;
  chassisReadyShed = 0;
  chassisActiveShed = 0;

  portENTER_CRITICAL(&canRecoveryMux);
  canHardReinitCount = 0;
  canHardReinitFailCount = 0;
  canRecoverySleepCount = 0;
  canRecoveryWakeCount = 0;
  canLastHardReinitReason = CAN_SUP_NONE;
  canPendingHardDiagReason = CAN_REC_NONE;
  canLastHardDiagReason = CAN_REC_NONE;
  canTaskHeartbeatLastCause = CAN_TASK_HEARTBEAT_NONE;
  canTaskHeartbeatLastAgeAms = 0;
  canTaskHeartbeatLastAgeBms = 0;
  canTaskHeartbeatTimeoutCountA = 0;
  canTaskHeartbeatTimeoutCountB = 0;
  canTaskHeartbeatTimeoutCountBoth = 0;
  canTaskHeartbeatLastSnapshotA = {};
  canTaskHeartbeatLastSnapshotB = {};
  canChassisBusOffCount = 0;
  canChassisStoppedCount = 0;
  canChassisLocalRecoveryStartCount = 0;
  canChassisRecoveryStartFailCount = 0;
  canChassisRestartOkCount = 0;
  canChassisRestartFailCount = 0;
  canChassisLastEventReason = CAN_REC_NONE;
  canChassisLastEventMs = 0;
  canBLastRxGapMs = 0;
  canBMaxRxGapMs = 0;
  canChassisLastBusOffSnapshot = {};
  portEXIT_CRITICAL(&canRecoveryMux);
  canTaskDiagnosticsResetPure(canTaskMcpDiagnostics);
  canTaskDiagnosticsResetPure(canTaskChassisDiagnostics);
  canARxDiagnosticsResetPure(canARxDiagnostics);
  canATraceReset();
  canBTraceReset();

  runtimeStatsResetCount++;
  runtimeStatsLastResetMs = (uint32_t)millis();
  return true;
}

static void httpResetRuntimeStats() {
  if (!resetRuntimeStats()) {
    server.send(500, "application/json",
                "{\"ok\":false,\"error\":\"bus-off-persistence-clear-failed\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"runtime-stats-reset\"}");
}

static void webTask(void *arg) {
  TMR_SERIAL_PRINTLN("WiFi: Starting AP...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  String ssid, password;
  wifiApLoadConfig(ssid, password);
  while (!wifiApActivate(ssid, password, true)) {
    TMR_SERIAL_PRINTLN("WiFi: Failed to start AP, retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
#if TMR_SERIAL_DIAGNOSTICS
  IPAddress ip = WiFi.softAPIP();
#endif
  bootCaptureMarkOnce(&bootCapWifiReadyMs);
#if TMR_SERIAL_DIAGNOSTICS
  Serial.printf("AP: SSID=%s IP=%s\n", wifiApActiveSsid.c_str(), ip.toString().c_str());
#endif

  server.on("/", HTTP_GET, httpRoot);
  server.on("/apple-touch-icon.png", HTTP_GET, httpAppleTouchIcon);
  server.on("/fonts/geist-lab.woff2", HTTP_GET, httpLabGeistFont);
  server.on("/fonts/geist-mono-lab.woff2", HTTP_GET, httpLabGeistMonoFont);
  server.on("/api/profile/status", HTTP_GET, httpProfileStatus);
  server.on("/api/profile/select", HTTP_POST, httpProfileSelect);
  server.on("/api/system/factory-reset", HTTP_POST, httpFactoryReset);
  server.on("/api/system/reset-nvs-keep-ble", HTTP_POST, httpResetNvsPreserveS3xyBle);
  server.on("/api/system/reboot", HTTP_POST, httpRebootTmrCan);
  server.on("/api/wifi/status", HTTP_GET, httpWifiStatus);
  server.on("/api/wifi/apply", HTTP_POST, httpWifiApply);
  server.on("/update", HTTP_POST, httpOtaFinish, httpOtaUpload);

  if (!vehicleProfileSetupMode && !vehicleProfileNvsError) {
    server.on("/api/snapshot", HTTP_GET, httpSnapshot);
    if (activeProfileNagSupported()) {
      server.on("/api/nag/config", HTTP_GET, httpNagConfig);
      server.on("/api/nag/stats", HTTP_GET, httpNagStats);
      server.on("/api/nag/mode", HTTP_POST, httpNagSetMode);
    server.on("/api/nag/h-variant", HTTP_POST, httpNagSetHumanVariant);
      server.on("/api/nag/update", HTTP_POST, httpNagUpdate);
      server.on("/api/nag/reset", HTTP_POST, httpNagReset);
    }
    server.on("/api/summon/stats", HTTP_GET, httpSummonStats);
    server.on("/api/summon/tlssc-enable", HTTP_POST, httpSummonTlsscEnable);
    server.on("/api/summon/tlssc-disable", HTTP_POST, httpSummonTlsscDisable);
    server.on("/api/summon/tlssc-highway-gate", HTTP_POST, httpSummonTlsscHighwayGate);
    server.on("/api/summon/tlssc-noa-block", HTTP_POST, httpSummonTlsscNoaBlock);
    server.on("/api/blinkA/stats", HTTP_GET, httpBlinkAStats);
    server.on("/api/blinkA/enable", HTTP_POST, httpBlinkAEnable);
    server.on("/api/blinkA/disable", HTTP_POST, httpBlinkADisable);
    server.on("/api/blinkA/delay", HTTP_POST, httpBlinkADelay);
    server.on("/api/blinkA/timing", HTTP_POST, httpBlinkATiming);
    server.on("/api/blinkA/tx-mode", HTTP_POST, httpBlinkATxMode);
    server.on("/api/lab/can-a-rx/stats", HTTP_GET, httpCanARxLabStats);
    server.on("/api/lab/can-a-rx/update", HTTP_POST, httpCanARxLabUpdate);
    server.on("/api/lab/driver-window/stats", HTTP_GET, httpDriverWindowLabStats);
    server.on("/api/lab/driver-window/open", HTTP_POST, httpDriverWindowLabOpen);
    server.on("/api/features/status", HTTP_GET, httpFeatureStatus);
    server.on("/api/features/lab", HTTP_POST, httpFeatureLab);
    server.on("/api/features/door-cancel", HTTP_POST, httpFeatureDoorCancel);
    server.on("/api/features/ap-drive-profile", HTTP_POST, httpFeatureApDriveProfile);
    server.on("/api/features/banned", HTTP_POST, httpFeatureBanned);
    server.on("/api/features/tlssc-restore", HTTP_POST, httpFeatureTlsscRestore);
    server.on("/api/features/s3xy", HTTP_POST, httpFeatureS3xy);
    server.on("/api/das/stats", HTTP_GET, httpDasTelemetryStats);
    server.on("/api/r79/stats", HTTP_GET, httpR79Stats);
    server.on("/api/r79/update", HTTP_POST, httpR79Update);
    server.on("/api/r79/ap-control/stats", HTTP_GET, httpR79ApControlStats);
    server.on("/api/r79/ap-control/update", HTTP_POST, httpR79ApControlUpdate);
    server.on("/api/nag/h-profile/stats", HTTP_GET, httpNagHumanProfileStats);
    server.on("/api/nag/h-profile/update", HTTP_POST, httpNagHumanProfileUpdate);
    server.on("/api/nag/h-profile/reset", HTTP_POST, httpNagHumanProfileReset);
    server.on("/api/ulc/stats", HTTP_GET, httpUlcStats);
    server.on("/api/ulc/update", HTTP_POST, httpUlcUpdate);
    server.on("/api/lab/auto-lane-change/stats", HTTP_GET, httpAutoLaneChangeLabStats);
    server.on("/api/lab/auto-lane-change/update", HTTP_POST, httpAutoLaneChangeLabUpdate);
    server.on("/api/lab/vision-control/stats", HTTP_GET, httpVisionControlStats);
    server.on("/api/lab/vision-control/update", HTTP_POST, httpVisionControlUpdate);
    server.on("/api/country/stats", HTTP_GET, httpCountryOverrideStats);
    server.on("/api/country/update", HTTP_POST, httpCountryOverrideUpdate);
    server.on("/api/lab/lane-graph/stats", HTTP_GET, httpLaneGraphStats);
    server.on("/api/lab/lane-graph/update", HTTP_POST, httpLaneGraphUpdate);
    server.on("/api/lab/country/stats", HTTP_GET, httpCountryOverrideStats);
    server.on("/api/lab/country/update", HTTP_POST, httpCountryOverrideUpdate);
    server.on("/api/lab/ulc-monitor/stats", HTTP_GET, httpUlcMonitorLabStats);
    server.on("/api/researchcapture/stats", HTTP_GET, httpResearchCaptureStats);
    server.on("/api/researchcapture/start", HTTP_POST, httpResearchCaptureStart);
    server.on("/api/researchcapture/labels", HTTP_POST, httpResearchCaptureLabels);
    server.on("/api/researchcapture/config", HTTP_POST, httpResearchCaptureConfig);
    server.on("/api/researchcapture/mode", HTTP_POST, httpResearchCaptureMode);
    server.on("/api/researchcapture/reset", HTTP_POST, httpResearchCaptureReset);
    server.on("/api/researchcapture/log.csv", HTTP_GET, httpResearchCaptureCsv);
    server.on("/api/drivermonitor/stats", HTTP_GET, httpDriverMonitorStats);
    server.on("/api/drivermonitor/start", HTTP_POST, httpDriverMonitorStart);
    server.on("/api/drivermonitor/reset", HTTP_POST, httpDriverMonitorReset);
    server.on("/api/drivermonitor/log.csv", HTTP_GET, httpDriverMonitorCsv);
    server.on("/api/pedalmap/stats", HTTP_GET, httpPedalMapStats);
    server.on("/api/pedalmap/set", HTTP_POST, httpPedalMapSet);
    server.on("/api/system/stats", HTTP_GET, httpSystemStats);
    server.on("/api/system/boot-capture.csv", HTTP_GET, httpBootCaptureCsv);
    server.on("/api/system/cana-tx.csv", HTTP_GET, httpCanATxTraceCsv);
    server.on("/api/system/canb-tx.csv", HTTP_GET, httpCanBTxTraceCsv);
    server.on("/api/system/reset-stats", HTTP_POST, httpResetRuntimeStats);
    server.on("/api/system/reinit-can", HTTP_POST, httpCanHardReinit);
    server.on("/api/system/reset-settings", HTTP_POST, httpResetFirmwareSettings);
    server.on("/api/s3xy/stats", HTTP_GET, httpS3xyStats);
    server.on("/api/s3xy/bluetooth-enable", HTTP_POST, httpS3xyBluetoothEnable);
    server.on("/api/s3xy/bluetooth-disable", HTTP_POST, httpS3xyBluetoothDisable);
    server.on("/api/s3xy/reset-all", HTTP_POST, httpS3xyResetAllBluetooth);
    server.on("/api/s3xy/scan", HTTP_POST, httpS3xyScan);
    server.on("/api/s3xy/pair", HTTP_POST, httpS3xyPair);
    server.on("/api/s3xy/device/connect", HTTP_POST, httpS3xyDeviceConnect);
    server.on("/api/s3xy/device/disconnect", HTTP_POST, httpS3xyDeviceDisconnect);
    server.on("/api/s3xy/device/forget", HTTP_POST, httpS3xyDeviceForget);
    server.on("/api/s3xy/device/handshake", HTTP_POST, httpS3xyDeviceHandshake);
    server.on("/api/s3xy/device/rename", HTTP_POST, httpS3xyDeviceRename);
    server.on("/api/s3xy/device/action", HTTP_POST, httpS3xyDeviceAction);
    server.on("/api/s3xy/device/auto", HTTP_POST, httpS3xyDeviceAuto);
    server.on("/api/s3xy/auto-enable", HTTP_POST, httpS3xyAutoEnable);
    server.on("/api/s3xy/auto-disable", HTTP_POST, httpS3xyAutoDisable);
    server.on("/api/s3xy/clear", HTTP_POST, httpS3xyClear);
    server.on("/api/s3xy/log.csv", HTTP_GET, httpS3xyLogCsv);
  }
  server.begin();

  for (;;) {
    server.handleClient();
    webBeat++;
    vTaskDelay(1);
  }
}
