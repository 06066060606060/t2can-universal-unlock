#pragma once

// WEB API / OTA / DASHBOARD SERVICE
// Kept in the same translation unit to preserve proven runtime behavior.

// ═══════════════════════════════════════════════════════════════
// WEB SERVER
// ═══════════════════════════════════════════════════════════════


static void httpPedalMapStats(){server.send(200,"application/json",pedalMapStatsJson());}

static String nagCfgToJson() {
  NagConfig c;
  const NagHumanConfigPure human = nagHumanRuntimeConfigSnapshot();
  portENTER_CRITICAL(&nagCfgMux);
  c = nagCfg;
  portEXIT_CRITICAL(&nagCfgMux);
  String s;
  s.reserve(620);
  s = "{";
  s += "\"enabled\":";    s += (c.enabled ? "true" : "false");
  s += ",\"pauseAtZeroSpeed\":"; s += (c.pauseAtZeroSpeed ? "true" : "false");
  s += ",\"mode\":";      s += String(c.mode);
  s += ",\"targetId\":";  s += String(c.targetId);
  s += ",\"hoRatePct\":"; s += String(c.hoRatePct);
  s += ",\"burstMs\":";   s += String(c.burstMs);
  s += ",\"pauseMs\":";   s += String(c.pauseMs);
  s += ",\"apStateId\":"; s += String(c.apStateId);
  s += ",\"steeringId\":";s += String(c.steeringId);
  s += ",\"humanPreset\":\"NORMAL_TIMING\"";
  s += ",\"humanPeakMinNm\":" + String((float)human.peakMinRaw / 100.0f, 2);
  s += ",\"humanPeakMaxNm\":" + String((float)human.peakMaxRaw / 100.0f, 2);
s += ",\"torque\":[";
for (uint8_t i = 0; i < c.torqueCount; i++) {
  if (i) s += ",";
  s += "{\"b2\":";
  s += String(c.torqueB2[i]);
  s += ",\"b3\":";
  s += String(c.torqueB3[i]);
  uint16_t raw = ((c.torqueB2[i] & 0x0F) << 8) | c.torqueB3[i];
  float nm = raw * 0.01f - 20.5f;
  s += ",\"nm\":";
  s += String(nm, 2);
  s += "}";
}
s += "]}";
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

  uint32_t txOk, txFail, skDisabled, skBoot, skWarmup, skSelf, skHo, skInvalid, skInactive, skDecision, skStopped, skCadence, skSpeedStale;
  uint32_t bMutex, bMcp, bEpoch, bFresh, bInvalid, sendErr, lastTx, maxGap, sessStart, sessTx;
  uint8_t lastSkip, lastBlock;
  uint32_t lastSkipMs, lastBlockMs;
  portENTER_CRITICAL(&nagDiagMux);
  txOk=nagTxOk; txFail=nagTxFail;
  skDisabled=nagSkipDisabled; skBoot=nagSkipBootDelay; skWarmup=nagSkipWarmup; skSelf=nagSkipSelfFrame;
  skHo=nagSkipHandsOn; skInvalid=nagSkipApInvalid; skInactive=nagSkipApInactive; skDecision=nagSkipDecision; skStopped=nagSkipStopped; skCadence=nagSkipCadence; skSpeedStale=nagSkipSpeedStale;
  bMutex=nagBlockMutex; bMcp=nagBlockMcpNotReady; bEpoch=nagBlockEpoch; bFresh=nagBlockFreshMask;
  bInvalid=nagBlockInvalidMsg; sendErr=nagSendError;
  lastTx=nagLastTxOkMs; maxGap=nagMaxTxGapMs; sessStart=nagSessionStartMs; sessTx=nagSessionTxOk;
  lastSkip=nagLastSkipReason; lastSkipMs=nagLastSkipMs; lastBlock=nagLastTxBlockReason; lastBlockMs=nagLastTxBlockMs;
  portEXIT_CRITICAL(&nagDiagMux);

  const uint32_t now = (uint32_t)millis();
  bool pauseAtZero = false;
  uint8_t nagModeNow = MODE_A;
  portENTER_CRITICAL(&nagCfgMux);
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  nagModeNow = nagCfg.mode;
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t speedAgeMs = c.lastVehicleSpeedMs == 0 ? 999999UL : (uint32_t)(now - c.lastVehicleSpeedMs);
  const bool speedFresh = c.vehicleSpeedValid && c.lastVehicleSpeedMs != 0 && speedAgeMs <= NAG_SPEED_FRESH_MS;
  const float speedKph = c.vehicleSpeedValid ? ((float)nagPartySpeedKphX100Pure(c.vehicleSpeedRaw) / 100.0f) : 0.0f;
  const bool humanMode = nagModeNow == MODE_H;
  NagHumanStatePure human = {};
  const NagHumanConfigPure humanConfig = nagHumanRuntimeConfigSnapshot();
  if (humanMode) human = nagHumanRuntimeSnapshot();
  const bool humanPaused = humanMode && human.phase == H_PAUSED_STOPPED;
  const bool stoppedGate = nagPauseAtZeroBlocksPure(pauseAtZero, c.vehicleSpeedValid, speedFresh, c.vehicleSpeedRaw) || humanPaused;
  const uint16_t humanOutputRaw = human.outputRaw != 0u ? human.outputRaw : NAG_HUMAN_TORQUE_CENTER_RAW;
  const float humanOutputNm = (float)humanOutputRaw * 0.01f - 20.5f;
  String s;
  s.reserve(1950);
  s = "{";
  s += "\"rx\":";            s += String(nagRxFrames);
  s += ",\"echo\":";         s += String(nagEchoCount);
  s += ",\"txOk\":";         s += String(txOk);
  s += ",\"txFail\":";       s += String(txFail);
  s += ",\"mcpTxOkTotal\":"; s += String(mcpTxOk);
  s += ",\"mcpTxFailTotal\":"; s += String(mcpTxFail);
  s += ",\"latUs\":";        s += String(nagEchoLatUs);
  s += ",\"ho\":";           s += String(nagRealHo);
  s += ",\"torque\":";       s += String(nagRealTorque, 2);
  s += ",\"injHo\":";        s += String(nagLastInjectedHo);
  s += ",\"injNm\":";        s += String(nagLastInjectedNm, 2);
  s += ",\"uptimeS\":";      s += String((millis() - bootTime) / 1000);
  s += ",\"apState\":";      s += String(c.apState);
  s += ",\"dasState\":";     s += String((unsigned)apState4);
  s += ",\"dasStateValid\":"; s += (apValid ? "true" : "false");
  s += ",\"handsOnState\":"; s += String(c.handsOnState);
  s += ",\"steeringDeg\":";  s += String(c.steeringAngleDeg, 1);
  s += ",\"vehicleSpeedKph\":"; s += String(speedKph, 2);
  s += ",\"vehicleSpeedRaw\":"; s += String((unsigned)c.vehicleSpeedRaw);
  s += ",\"vehicleSpeedValid\":"; s += c.vehicleSpeedValid ? "true" : "false";
  s += ",\"vehicleSpeedFresh\":"; s += speedFresh ? "true" : "false";
  s += ",\"vehicleSpeedAgeMs\":"; s += String(speedAgeMs);
  s += ",\"pauseAtZeroSpeed\":"; s += pauseAtZero ? "true" : "false";
  s += ",\"stoppedGate\":"; s += stoppedGate ? "true" : "false";
  const bool portableMode = nagModeNow == MODE_D || nagModeNow == MODE_E || nagModeNow == MODE_F;
  s += ",\"portableMode\":"; s += portableMode ? "true" : "false";
  s += ",\"humanMode\":"; s += humanMode ? "true" : "false";
  s += ",\"humanPreset\":\"NORMAL_TIMING\"";
  s += ",\"humanPeakMinNm\":" + String((float)humanConfig.peakMinRaw / 100.0f, 2);
  s += ",\"humanPeakMaxNm\":" + String((float)humanConfig.peakMaxRaw / 100.0f, 2);
  s += ",\"humanPhase\":\"" + String(nagHumanPhaseNamePure(human.phase)) + "\"";
  s += ",\"humanMotion\":\"" + String(nagHumanMotionNamePure(human.motion)) + "\"";
  s += ",\"humanEventType\":\"" + String(nagHumanEventTypeNamePure(human.event.type)) + "\"";
  s += ",\"humanEventCount\":" + String((unsigned long)human.eventCount);
  s += ",\"humanOutputNm\":" + String(humanOutputNm, 2);
  s += ",\"humanDirection\":" + String((int)human.event.direction);
  s += ",\"humanCarrier\":" + String(human.carrier ? "true" : "false");
  s += ",\"humanPaused\":" + String(humanPaused ? "true" : "false");
  s += ",\"humanSessionAgeMs\":" + String((unsigned long)(human.sessionStartMs ? now - human.sessionStartMs : 0UL));
  s += ",\"apStaleMs\":";    s += String((c.lastApStateMs == 0) ? 999999 : (now - c.lastApStateMs));
  s += ",\"dasAgeMs\":";     s += String((dasLast == 0) ? 999999UL : (uint32_t)(now-dasLast));
  s += ",\"stStaleMs\":";    s += String((c.lastSteeringMs == 0) ? 999999 : (now - c.lastSteeringMs));
  s += ",\"canAState\":";    s += String((int)mcpState);
  s += ",\"apActive\":";     s += (apValid && apActive ? "true" : "false");
  s += ",\"lastTxAgeMs\":";  s += String(lastTx ? (uint32_t)(now-lastTx) : 999999UL);
  s += ",\"maxTxGapMs\":";   s += String(maxGap);
  s += ",\"sessionAgeMs\":"; s += String(sessStart ? (uint32_t)(now-sessStart) : 0UL);
  s += ",\"sessionTxOk\":";  s += String(sessTx);
  s += ",\"skipDisabled\":"; s += String(skDisabled);
  s += ",\"skipBootDelay\":"; s += String(skBoot);
  s += ",\"skipWarmup\":"; s += String(skWarmup);
  s += ",\"skipSelfFrame\":"; s += String(skSelf);
  s += ",\"skipHandsOn\":"; s += String(skHo);
  s += ",\"skipApInvalid\":"; s += String(skInvalid);
  s += ",\"skipApInactive\":"; s += String(skInactive);
  s += ",\"skipDecision\":"; s += String(skDecision);
  s += ",\"skipStopped\":"; s += String(skStopped);
  s += ",\"skipCadence\":"; s += String(skCadence);
  s += ",\"skipSpeedStale\":"; s += String(skSpeedStale);
  s += ",\"blockMutex\":"; s += String(bMutex);
  s += ",\"blockMcpNotReady\":"; s += String(bMcp);
  s += ",\"blockEpoch\":"; s += String(bEpoch);
  s += ",\"blockFreshMask\":"; s += String(bFresh);
  s += ",\"blockInvalidMsg\":"; s += String(bInvalid);
  s += ",\"sendError\":"; s += String(sendErr);
  s += ",\"lastSkip\":\"" + String(nagSkipReasonName(lastSkip)) + "\"";
  s += ",\"lastSkipAgeMs\":" + String(lastSkipMs ? (uint32_t)(now-lastSkipMs) : 999999UL);
  s += ",\"lastTxBlock\":\"" + String(nagTxBlockReasonName(lastBlock)) + "\"";
  s += ",\"lastTxBlockAgeMs\":" + String(lastBlockMs ? (uint32_t)(now-lastBlockMs) : 999999UL);
  s += "}";
  return s;
}

static String summonStatsToJson() {
    bool tlssc, tlsscHighwayGate, ap, parked, summon, aca, spr, fmode, priorityFreshParked, gateGraceActive;
    bool acaStateValid, sprStateValid;
    uint8_t priorityState, gearSource, authorization, sprRaw;
    uint8_t confirmedGearState, confirmedGearSource;
    uint32_t prioritySince, priorityTransitions, priorityFullEnter, priorityFullExit, priorityFullInactiveSince;
    uint32_t gateGraceUntil, gateGraceEnter, gateGraceRecover, gateGraceExpire;
    uint32_t acaObservedMs, sprObservedMs, gearObservedMs, authorizationSinceMs, authorizationTransitions;
    uint32_t confirmedGearObservedMs, confirmedGearTransitions;
    uint32_t rmx, tok, tfail, r280, r390, r921, r1016;
    const uint32_t now = (uint32_t)millis();
    portENTER_CRITICAL(&stateMux);
    tlssc  = tlsscEnabled;
    tlsscHighwayGate = tlsscHighwayGateEnabled;
    ap     = gateAPActive;
    parked = gateParked;
    summon = gateSummoning;
    aca    = lastAca;
    spr    = sprSeen;
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
    authorization = summonAuthorization;
    authorizationSinceMs = summonAuthorizationSinceMs;
    authorizationTransitions = summonAuthorizationTransitions;
    fmode  = forceMode;
    priorityState = summonPriorityState;
    priorityFreshParked = summonPriorityFreshParkedLocked(now);
    prioritySince = summonPriorityStateSinceMs;
    priorityTransitions = summonPriorityTransitions;
    priorityFullEnter = summonPriorityFullEnterCount;
    priorityFullExit = summonPriorityFullExitCount;
    priorityFullInactiveSince = summonPriorityFullInactiveSinceMs;
    gateGraceUntil = summonGateGraceUntilMs;
    gateGraceActive = summonGateGraceActiveLocked(now);
    gateGraceEnter = summonGateGraceEnterCount;
    gateGraceRecover = summonGateGraceRecoverCount;
    gateGraceExpire = summonGateGraceExpireCount;
    rmx    = sumRxMux1;
    tok    = sumTxOk;
    tfail  = sumTxFail;
    r280   = sumRx280;
    r390   = sumRx390;
    r921   = sumRx921;
    r1016  = sumRx1016;
    portEXIT_CRITICAL(&stateMux);

    const SummonRoutePure summonRoute = activeSummonRoute();
    const uint8_t observedFreshMask = canTxFreshMaskSnapshot();
    const bool remoteFallbackAllowed = false; // retired from the functional V2.6 gate
    const uint8_t requiredFreshMask = summonV26CompatRequiredTxFreshMaskPure(
        summonRoute);
    const uint32_t gearAge = gearObservedMs ? (uint32_t)(now - gearObservedMs) : 999999UL;
    const uint32_t confirmedGearAge = confirmedGearObservedMs
        ? (uint32_t)(now - confirmedGearObservedMs) : 999999UL;
    const uint32_t acaAge = acaObservedMs ? (uint32_t)(now - acaObservedMs) : 999999UL;
    const uint32_t sprAge = sprObservedMs ? (uint32_t)(now - sprObservedMs) : 999999UL;
    const bool acaFresh = acaStateValid &&
        summonAgeFreshPure(now, acaObservedMs, SUMMON_ACA_FRESH_MS);
    const bool sprFresh = sprStateValid &&
        summonAgeFreshPure(now, sprObservedMs, SUMMON_SPR_FRESH_MS);
    const bool requiredFreshReady = requiredFreshMask != SUMMON_BUS_NONE &&
        (uint8_t)(observedFreshMask & requiredFreshMask) == requiredFreshMask;
    const char *gearBusName = summonRoute.gearBusMask == SUMMON_BUS_A
        ? activeProfileCanAName()
        : (summonRoute.gearBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
    const char *dasBusName = summonRoute.dasBusMask == SUMMON_BUS_A
        ? activeProfileCanAName()
        : (summonRoute.dasBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
    const char *sprBusName = summonRoute.sprBusMask == SUMMON_BUS_A
        ? activeProfileCanAName()
        : (summonRoute.sprBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");
    const char *transportBusName = summonRoute.transportBusMask == SUMMON_BUS_A
        ? activeProfileCanAName()
        : (summonRoute.transportBusMask == SUMMON_BUS_B ? activeProfileCanBName() : "NONE");

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

    const uint32_t gateGraceRemaining = (gateGraceActive && (int32_t)(gateGraceUntil - now) > 0)
        ? (uint32_t)(gateGraceUntil - now) : 0;
    const bool gateRaw = parked || summon;
    const bool gate = authorization != SUMMON_AUTH_NONE;
    twai_status_info_t st = {};
    const bool twaiStatusOk = (twai_get_status_info(&st) == ESP_OK);

    String s;
    s.reserve(3900);
    s = "{";
    s += "\"monitoring\":true";
    s += ",\"tlssc\":"   + String(tlssc ? "true" : "false");
    s += ",\"tlsscHighwayGateEnabled\":" + String(tlsscHighwayGate ? "true" : "false");
    s += ",\"roadContextValid\":" + String(roadValid ? "true" : "false");
    s += ",\"roadContextFresh\":" + String(roadFresh ? "true" : "false");
    s += ",\"roadContextAgeMs\":" + String((unsigned long)roadAge);
    s += ",\"roadContextFreshMs\":" + String((unsigned long)ROAD_CONTEXT_FRESH_MS);
    s += ",\"roadClass\":" + String((int)roadClass);
    s += ",\"gpsRoadMatch\":" + String(gpsRoadMatch ? "true" : "false");
    s += ",\"navRouteActive\":" + String(navRouteActive ? "true" : "false");
    s += ",\"controlledAccess\":" + String(controlledAccess ? "true" : "false");
    s += ",\"leftOffRamp\":" + String(leftOffRamp ? "true" : "false");
    s += ",\"rightOffRamp\":" + String(rightOffRamp ? "true" : "false");
    s += ",\"tlsscHighwayConfirmed\":" + String(highwayConfirmed ? "true" : "false");
    s += ",\"tlsscHighwayBlocked\":" + String(highwayBlocked ? "true" : "false");
    s += ",\"tlsscHighwayPositiveCount\":" + String((int)highwayPositiveCount);
    s += ",\"tlsscHighwayNegativeCount\":" + String((int)highwayNegativeCount);
    s += ",\"tlsscHighwayBlockedCount\":" + String((unsigned long)highwayBlockedCount);
    s += ",\"tlsscHighwayTransitions\":" + String((unsigned long)highwayTransitions);
    s += ",\"monitorMode\":0,\"monitorModeName\":\"ALWAYS_ON\"";
    s += ",\"summonRouteValid\":" + String(summonRoute.valid ? "true" : "false");
    s += ",\"gearBusName\":\"" + String(gearBusName) + "\"";
    s += ",\"dasBusName\":\"" + String(dasBusName) + "\"";
    s += ",\"sprBusName\":\"" + String(sprBusName) + "\"";
    s += ",\"transportBusName\":\"" + String(transportBusName) + "\"";
    s += ",\"allow186Fallback\":" + String(summonRoute.allow186Fallback ? "true" : "false");
    s += ",\"requiredFreshMask\":" + String((unsigned)requiredFreshMask);
    s += ",\"requiredFreshMaskName\":\"" + String(summonBusMaskNamePure(requiredFreshMask)) + "\"";
    s += ",\"observedFreshMask\":" + String((unsigned)observedFreshMask);
    s += ",\"observedFreshMaskName\":\"" + String(summonBusMaskNamePure(observedFreshMask)) + "\"";
    s += ",\"requiredFreshReady\":" + String(requiredFreshReady ? "true" : "false");
    s += ",\"gearSource\":" + String((unsigned)gearSource);
    s += ",\"gearSourceName\":\"" + String(summonGearSourceNamePure(gearSource)) + "\"";
    s += ",\"gearAgeMs\":" + String((unsigned long)gearAge);
    s += ",\"gearFreshMs\":" + String((unsigned long)SUMMON_GEAR_FRESH_MS);
    s += ",\"confirmedGearState\":" + String((unsigned)confirmedGearState);
    s += ",\"confirmedGearStateName\":\"" + String(summonConfirmedGearNamePure(confirmedGearState)) + "\"";
    s += ",\"confirmedGearSource\":" + String((unsigned)confirmedGearSource);
    s += ",\"confirmedGearSourceName\":\"" + String(summonGearSourceNamePure(confirmedGearSource)) + "\"";
    s += ",\"confirmedGearAgeMs\":" + String((unsigned long)confirmedGearAge);
    s += ",\"confirmedGearTransitions\":" + String((unsigned long)confirmedGearTransitions);
    s += ",\"remoteFallbackAllowed\":" + String(remoteFallbackAllowed ? "true" : "false");
    s += ",\"acaValid\":" + String(acaStateValid ? "true" : "false");
    s += ",\"acaFresh\":" + String(acaFresh ? "true" : "false");
    s += ",\"acaAgeMs\":" + String((unsigned long)acaAge);
    s += ",\"acaFreshMs\":" + String((unsigned long)SUMMON_ACA_FRESH_MS);
    s += ",\"sprValid\":" + String(sprStateValid ? "true" : "false");
    s += ",\"sprRaw\":" + String((unsigned)sprRaw);
    s += ",\"sprFresh\":" + String(sprFresh ? "true" : "false");
    s += ",\"sprAgeMs\":" + String((unsigned long)sprAge);
    s += ",\"sprFreshMs\":" + String((unsigned long)SUMMON_SPR_FRESH_MS);
    s += ",\"authorization\":" + String((unsigned)authorization);
    s += ",\"authorizationName\":\"" + String(summonAuthorizationNamePure(authorization)) + "\"";
    s += ",\"authorizationSinceMs\":" + String((unsigned long)authorizationSinceMs);
    s += ",\"authorizationTransitions\":" + String((unsigned long)authorizationTransitions);
    s += ",\"gate\":"    + String(gate ? "true" : "false");
    s += ",\"gateRaw\":" + String(gateRaw ? "true" : "false");
    s += ",\"gateGraceMs\":" + String((unsigned long)SUMMON_GATE_DROPOUT_GRACE_MS);
    s += ",\"gateGraceActive\":" + String(gateGraceActive ? "true" : "false");
    s += ",\"gateGraceRemainingMs\":" + String((unsigned long)gateGraceRemaining);
    s += ",\"gateGraceEnter\":" + String((unsigned long)gateGraceEnter);
    s += ",\"gateGraceRecover\":" + String((unsigned long)gateGraceRecover);
    s += ",\"gateGraceExpire\":" + String((unsigned long)gateGraceExpire);
    s += ",\"ap\":"      + String(ap ? "true" : "false");
    s += ",\"parked\":"  + String(parked ? "true" : "false");
    s += ",\"summon\":"  + String(summon ? "true" : "false");
    s += ",\"aca\":"     + String(aca ? "true" : "false");
    s += ",\"spr\":"     + String(spr ? "true" : "false");
    s += ",\"forceMode\":"+ String(fmode ? "true" : "false");
    s += ",\"priorityState\":" + String((int)priorityState);
    s += ",\"priorityStateName\":\"" + String(summonPriorityStateName(priorityState)) + "\"";
    s += ",\"priorityFreshParked\":" + String(priorityFreshParked ? "true" : "false");
    s += ",\"priorityStateSinceMs\":" + String((unsigned long)prioritySince);
    s += ",\"priorityTransitions\":" + String((unsigned long)priorityTransitions);
    s += ",\"priorityFullEnter\":" + String((unsigned long)priorityFullEnter);
    s += ",\"priorityFullExit\":" + String((unsigned long)priorityFullExit);
    s += ",\"priorityExitGraceActive\":" + String(priorityFullInactiveSince != 0 ? "true" : "false");
    s += ",\"txQueueNow\":" + String((unsigned long)twaiTxQueueNow);
    s += ",\"txQueueMax\":" + String((unsigned long)twaiTxQueueMax);
    s += ",\"nonSummonShed\":" + String((unsigned long)twaiNonSummonShed);
    s += ",\"standbyShed\":" + String((unsigned long)twaiStandbyShed);
    s += ",\"fullShed\":" + String((unsigned long)twaiFullShed);
    s += ",\"summonQueueFlush\":" + String((unsigned long)twaiSummonQueueFlush);
    s += ",\"summonTxNormal\":" + String((unsigned long)twaiSummonTxNormal);
    s += ",\"summonTxStandby\":" + String((unsigned long)twaiSummonTxStandby);
    s += ",\"summonTxFull\":" + String((unsigned long)twaiSummonTxFull);
    s += ",\"rxMux1\":"  + String(rmx);
    s += ",\"txOk\":"    + String(tok);
    s += ",\"txFail\":"  + String(tfail);
    s += ",\"rx280\":"   + String(r280);
    s += ",\"rx390\":"   + String(r390);
    s += ",\"rx921\":"   + String(r921);
    s += ",\"rx1016\":"  + String(r1016);
    s += ",\"canState\":" + String(twaiStatusOk ? (int)st.state : -1);
    s += ",\"canStateName\":\"" + String(twaiStatusOk ? twaiStateName(st.state) : "UNAVAILABLE") + "\"";
    s += ",\"uptimeS\":"  + String((millis() - bootTime) / 1000);
    s += "}";
    return s;
}

static String blinkAStatsToJson() {
  bool en, ap, noaRaw, noaEffective, dasStateValid, fmode, alcValid;
  uint8_t dasState4, alcState;
  uint8_t curTurn, pending, behavior;
  uint32_t delayMs, remain, retryIn, requestAge, retryCount, txOk, txFail, r249, visualLast;
  bool armed, seen, selfTest;
  uint8_t rCnt, rTurn, rCk, rDlc;
  uint8_t raw249[8] = {0};
  portENTER_CRITICAL(&blinkAMux);
  en = blinkAEnabled;
  curTurn = activeTurn;
  pending = autoPendingDir;
  delayMs = blinkADelayMs;
  armed = autoArmed;
  uint32_t now = millis();
  remain = (autoArmed && (int32_t)(autoFireAt - now) > 0) ? (autoFireAt - now) : 0;
  retryIn = (autoArmed && (int32_t)(autoRetryAt - now) > 0) ? (autoRetryAt - now) : 0;
  requestAge = autoRequestLastSeenMs ? (uint32_t)(now - autoRequestLastSeenMs) : 999999UL;
  retryCount = autoRetryCount;
  txOk = blkATxOk;
  txFail = blkATxFail;
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
  fmode = forceMode;
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
  s = "{";
  s += "\"enabled\":" + String(en ? "true" : "false");
  s += ",\"apActive\":" + String(ap ? "true" : "false");
  s += ",\"noaActive\":" + String(noaEffective ? "true" : "false");
  s += ",\"noaRawActive\":" + String(noaRaw ? "true" : "false");
  s += ",\"dasStateValid\":" + String(dasStateValid ? "true" : "false");
  // Backward-compatible key for cached dashboards: mirrors validity, not age.
  s += ",\"noaFresh\":" + String(dasStateValid ? "true" : "false");
  s += ",\"noaAgeMs\":" + String((noaAgeMs == UINT32_MAX) ? 999999UL : (unsigned long)noaAgeMs);
  s += ",\"noaFreshLimitMs\":0";
  s += ",\"dasState\":" + String((int)dasState4);
  s += ",\"alcState\":" + String((int)alcState);
  s += ",\"alcValid\":" + String(alcValid ? "true" : "false");
  s += ",\"requestFreshMs\":" + String((unsigned long)BLINKA_REQUEST_FRESH_MS);
  s += ",\"visualFresh\":" + String(visualFresh ? "true" : "false");
  s += ",\"visualAgeMs\":" + String((visualAgeMs == UINT32_MAX) ? 999999UL : (unsigned long)visualAgeMs);
  s += ",\"alcDirectionAllowed\":" + String(alcDirectionAllowed ? "true" : "false");
  s += ",\"forceMode\":" + String(fmode ? "true" : "false");
  s += ",\"behaviorType\":" + String((int)behavior);
  s += ",\"activeTurn\":" + String(curTurn);
  s += ",\"delayMs\":" + String(delayMs);
  s += ",\"autoArmed\":" + String(armed ? "true" : "false");
  s += ",\"autoPending\":" + String(pending);
  s += ",\"autoRemainMs\":" + String(remain);
  s += ",\"autoRetryInMs\":" + String(retryIn);
  s += ",\"autoRetryCount\":" + String(retryCount);
  s += ",\"autoRequestAgeMs\":" + String(requestAge);
  s += ",\"autoWaitingEligibility\":" + String(waitingEligibility ? "true" : "false");
  s += ",\"pendingAlcAllowed\":" + String(pendingAlcAllowed ? "true" : "false");
  s += ",\"txOk\":" + String(txOk);
  s += ",\"txFail\":" + String(txFail);
  s += ",\"rx249\":" + String(r249);
  s += ",\"seen249\":" + String(seen ? "true" : "false");
  s += ",\"realCounter\":" + String(rCnt);
  s += ",\"realTurn\":" + String(rTurn);
  s += ",\"realCksum\":" + String(rCk);
  s += ",\"realDlc\":" + String(rDlc);
  String rawHex;
  rawHex.reserve(24);
  for (uint8_t i = 0; i < rDlc; i++) {
    if (i) rawHex += " ";
    if (raw249[i] < 0x10) rawHex += "0";
    rawHex += String(raw249[i], HEX);
  }
  rawHex.toUpperCase();
  s += ",\"realRaw\":\"" + rawHex + "\"";
  s += ",\"cksumSelfTest\":" + String(selfTest ? "true" : "false");
  s += ",\"canBState\":" + String((int)twaiReady);
  s += ",\"uptimeS\":" + String((millis() - bootTime) / 1000);
  s += "}";
  return s;
}

static String dasTelemetryStatsToJson() {
  uint32_t now = millis();
  String s;
  s.reserve(256);
  s = "{";
  s += "\"behaviorType\":" + String((int)visualBehaviorType);
  s += ",\"ulcBlindSpotConfig\":" + String((int)uiUlcBlindSpotConfig);
  s += ",\"ulcSpeedConfig\":" + String((int)uiUlcSpeedConfig);
  s += ",\"visualDebugRx\":" + String((unsigned long)visualDebugRxCount);
  s += ",\"visualDebugStaleMs\":" + String(visualDebugLastMs == 0 ? 999999UL : (now - visualDebugLastMs));
  s += "}";
  return s;
}


static String lab3f8StatsToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t alcMode, blindMode, accMode;
  uint8_t lastTxBlind, lastTxStockBlind, lastTxSelectedBlind, lastTxResult;
  bool lastTxValid, lastTxBlindChanged, lastTxBit56;
  uint32_t lastTxMs, txOk, txFail, blocked, driverAssistLast;
  portENTER_CRITICAL(&lab3f8Mux);
  alcMode = lab3f8AlcMode;
  blindMode = lab3f8UlcBlindMode;
  accMode = lab3f8AccFollowRaw;
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
  driverAssistLast = uiDriverAssistLastRxMs;
  portEXIT_CRITICAL(&lab3f8Mux);

  uint8_t state4;
  bool dasStateValid;
  uint32_t dasLast;
  portENTER_CRITICAL(&stateMux);
  state4 = dasAutopilotState4;
  dasStateValid = dasAutopilotStateValid;
  dasLast = lastDASStatusMillis;
  portEXIT_CRITICAL(&stateMux);

  const bool gate = lab3f8AutosteerGateOpen(now);
  const uint32_t dasAge = dasLast == 0 ? 999999UL : (uint32_t)(now - dasLast);
  const uint32_t rxAge = driverAssistLast == 0 ? 999999UL : (uint32_t)(now - driverAssistLast);
  const uint32_t lastTxAge = (!lastTxValid || lastTxMs == 0) ? 999999UL : (uint32_t)(now - lastTxMs);

  String s;
  s.reserve(1024);
  s = "{";
  s += "\"anyOverride\":" + String(lab3f8AnyOverrideSelected() ? "true" : "false");
  s += ",\"gateOpen\":" + String(gate ? "true" : "false");
  s += ",\"gateRule\":\"AUTOSTEER_ONLY\"";
  s += ",\"dasState\":" + String((int)state4);
  s += ",\"dasStateValid\":" + String(dasStateValid ? "true" : "false");
  s += ",\"dasAgeMs\":" + String((unsigned long)dasAge);
  s += ",\"alcMode\":" + String((int)alcMode);
  s += ",\"ulcBlindMode\":" + String((int)blindMode);
  s += ",\"accFollowRaw\":" + String((int)accMode);
  s += ",\"stockAlcOffHighway\":" + String(uiAlcOffHighwayEnable ? "true" : "false");
  s += ",\"stockUlcBlindSpot\":" + String((int)uiUlcBlindSpotConfig);
  s += ",\"stockUlcSpeed\":" + String((int)uiUlcSpeedConfig);
  s += ",\"stockAccFollowRaw\":" + String((int)uiAccFollowDistanceRaw);
  s += ",\"rxAgeMs\":" + String((unsigned long)rxAge);
  s += ",\"txOk\":" + String((unsigned long)txOk);
  s += ",\"txFail\":" + String((unsigned long)txFail);
  s += ",\"gateBlocked\":" + String((unsigned long)blocked);
  s += ",\"lastTxValid\":" + String(lastTxValid ? "true" : "false");
  s += ",\"lastTxBlind\":" + String((int)lastTxBlind);
  s += ",\"lastTxStockBlind\":" + String((int)lastTxStockBlind);
  s += ",\"lastTxSelectedBlind\":" + String((int)lastTxSelectedBlind);
  s += ",\"lastTxBlindChanged\":" + String(lastTxBlindChanged ? "true" : "false");
  s += ",\"lastTxResult\":" + String((int)lastTxResult);
  s += ",\"lastTxBit56\":" + String(lastTxBit56 ? "true" : "false");
  s += ",\"lastTxAgeMs\":" + String((unsigned long)lastTxAge);
  s += "}";
  return s;
}

static const char* r79LabTxKindName(uint8_t kind) {
  switch (kind) {
    case R79LAB_TX_IMMEDIATE: return "IMMEDIATE";
    case R79LAB_TX_PERIODIC: return "PERIODIC";
    default: return "NONE";
  }
}

static String r79LabStatsToJson() {
  const uint32_t now = (uint32_t)millis();
  uint8_t smartMode, stockApply, stockSmart, stockHard, lastTxApply, lastTxSmart, lastTxHard;
  uint8_t gateReason, lastTxKind;
  uint16_t periodMs;
  bool stockValid, lastTxValid;
  uint32_t last3fd, rx3fd, c18, c19, c47, txOk, txFail, blocked, applied;
  uint32_t b18z,b18o,b19z,b19o,b47z,b47o,immOk,immFail,perOk,perFail;
  uint32_t lastAttempt, lastTx, noTemplateSkip, queueSkip;
  uint32_t pendingRequests, pendingCoalesced, pendingRetries, freshMaskWaits;
  uint32_t reassertLatencyLast, reassertLatencyMax, lastBlockMs;
  uint8_t lastRequiredFreshMask, lastObservedFreshMask, lastBlockReason;
  R79PendingPure pendingSnapshot = {};
  uint8_t stockRaw[8], lastTxRaw[8];
  portENTER_CRITICAL(&r79LabMux);
  smartMode=r79LabSmartMode;
  periodMs=r79LabPeriodMs; gateReason=r79LabLastGateReason;
  stockValid=r79LabStockValid; stockApply=r79LabStockApply; stockSmart=r79LabStockSmart; stockHard=r79LabStockHardCore;
  lastTxValid=r79LabLastTxValid; lastTxApply=r79LabEffectiveApply; lastTxSmart=r79LabEffectiveSmart; lastTxHard=r79LabEffectiveHardCore; lastTxKind=r79LabLastTxKind;
  last3fd=r79LabLast3fdMs; rx3fd=r79Lab3fdRx; c18=r79LabBit18Changes; c19=r79LabBit19Changes; c47=r79LabBit47Changes;
  b18z=r79LabBit18Rx0; b18o=r79LabBit18Rx1; b19z=r79LabBit19Rx0; b19o=r79LabBit19Rx1; b47z=r79LabBit47Rx0; b47o=r79LabBit47Rx1;
  immOk=r79LabImmediateTxOk; immFail=r79LabImmediateTxFail; perOk=r79LabPeriodicTxOk; perFail=r79LabPeriodicTxFail;
  txOk=r79LabTxOk; txFail=r79LabTxFail; blocked=r79LabGateBlocked; applied=r79LabAppliedFrames;
  lastAttempt=r79LabLastAttemptMs; lastTx=r79LabLastTxMs; noTemplateSkip=r79LabNoTemplateSkip; queueSkip=r79LabQueueSkip;
  pendingSnapshot=r79LabPending;
  pendingRequests=r79LabPendingRequestCount; pendingCoalesced=r79LabPendingCoalesceCount; pendingRetries=r79LabPendingRetryCount;
  freshMaskWaits=r79LabFreshMaskWaitCount;
  reassertLatencyLast=r79LabLastReassertLatencyMs; reassertLatencyMax=r79LabMaxReassertLatencyMs;
  lastRequiredFreshMask=r79LabLastRequiredFreshMask; lastObservedFreshMask=r79LabLastObservedFreshMask;
  lastBlockReason=r79LabLastBlockReason; lastBlockMs=r79LabLastBlockMs;
  memcpy(stockRaw,r79LabLastStockRaw,8); memcpy(lastTxRaw,r79LabLastEffectiveRaw,8);
  portEXIT_CRITICAL(&r79LabMux);
  uint8_t state4; bool dasStateValid; uint32_t dasLast;
  portENTER_CRITICAL(&stateMux); state4=dasAutopilotState4; dasStateValid=dasAutopilotStateValid; dasLast=lastDASStatusMillis; portEXIT_CRITICAL(&stateMux);
  char stockHex[24]={}, lastTxHex[24]={};
  snprintf(stockHex,sizeof(stockHex),"%02X %02X %02X %02X %02X %02X %02X %02X",stockRaw[0],stockRaw[1],stockRaw[2],stockRaw[3],stockRaw[4],stockRaw[5],stockRaw[6],stockRaw[7]);
  snprintf(lastTxHex,sizeof(lastTxHex),"%02X %02X %02X %02X %02X %02X %02X %02X",lastTxRaw[0],lastTxRaw[1],lastTxRaw[2],lastTxRaw[3],lastTxRaw[4],lastTxRaw[5],lastTxRaw[6],lastTxRaw[7]);
  const uint8_t liveGateReason = r79LabGateReason(now);
  const uint8_t currentRequiredFreshMask = activeSummonRequiredTxFreshMask();
  const uint8_t currentObservedFreshMask = canTxFreshMaskSnapshot();
  const bool currentFreshReady = currentRequiredFreshMask != SUMMON_BUS_NONE &&
      (uint8_t)(currentObservedFreshMask & currentRequiredFreshMask) == currentRequiredFreshMask;
  const uint32_t age3fd = last3fd ? (uint32_t)(now-last3fd) : 999999UL;
  const uint32_t pendingAge = pendingSnapshot.pending && pendingSnapshot.requestedMs
      ? (uint32_t)(now - pendingSnapshot.requestedMs) : 0;
  const uint32_t pendingAttemptAge = pendingSnapshot.pending && pendingSnapshot.lastAttemptMs
      ? (uint32_t)(now - pendingSnapshot.lastAttemptMs) : 999999UL;
  const uint32_t blockAge = lastBlockMs ? (uint32_t)(now - lastBlockMs) : 999999UL;
  const bool injectionActive = liveGateReason != R79LAB_GATE_BLOCKED && stockValid && currentFreshReady;
  String j; j.reserve(3000); j="{";
  j += "\"gateOpen\":" + String(liveGateReason!=R79LAB_GATE_BLOCKED?"true":"false");
  j += ",\"injectionActive\":" + String(injectionActive?"true":"false");
  j += ",\"gateRule\":\"SUMMON_OR_AP_OR_PARK_V26_COMPAT\"";
  j += ",\"gateReason\":\"" + String(r79LabGateReasonName(liveGateReason)) + "\"";
  j += ",\"dasState\":" + String((unsigned)state4);
  j += ",\"dasStateValid\":" + String(dasStateValid ? "true" : "false");
  j += ",\"dasAgeMs\":" + String((unsigned long)(dasLast?now-dasLast:999999UL));
  j += ",\"policyFixed\":true";
  j += ",\"fixedApplyValue\":" + String(R79_POLICY_APPLY_ECE_VALUE?1:0);
  j += ",\"fixedHardCoreValue\":" + String(R79_POLICY_HARD_CORE_VALUE?1:0);
  // Backward-compatible mode keys: FORCE0=1, FORCE1=2.
  j += ",\"applyMode\":" + String((unsigned)R79LAB_FORCE_0);
  j += ",\"smartMode\":" + String((unsigned)smartMode);
  j += ",\"hardMode\":" + String((unsigned)R79LAB_FORCE_1);
  j += ",\"periodMs\":" + String((unsigned)periodMs);
  j += ",\"parkPolicyAlwaysOn\":true";
  // Backward-compatible fields for cached b6 dashboards; they are fixed true and non-configurable.
  j += ",\"parkInjectionEnabled\":true";
  j += ",\"parkTestBurst\":true";
  j += ",\"stockValid\":" + String(stockValid?"true":"false");
  j += ",\"stockApply\":" + String((unsigned)stockApply);
  j += ",\"stockSmart\":" + String((unsigned)stockSmart);
  j += ",\"stockHardCore\":" + String((unsigned)stockHard);
  j += ",\"lastTxValid\":" + String(lastTxValid?"true":"false");
  j += ",\"lastTxApply\":" + String((unsigned)lastTxApply);
  j += ",\"lastTxSmart\":" + String((unsigned)lastTxSmart);
  j += ",\"lastTxHardCore\":" + String((unsigned)lastTxHard);
  j += ",\"effectiveApply\":" + String((unsigned)lastTxApply);
  j += ",\"effectiveSmart\":" + String((unsigned)lastTxSmart);
  j += ",\"effectiveHardCore\":" + String((unsigned)lastTxHard);
  j += ",\"lastTxKind\":\"" + String(r79LabTxKindName(lastTxKind)) + "\"";
  j += ",\"rx3fd\":" + String((unsigned long)rx3fd);
  j += ",\"age3fdMs\":" + String((unsigned long)age3fd);
  j += ",\"bit18Changes\":" + String((unsigned long)c18);
  j += ",\"bit19Changes\":" + String((unsigned long)c19);
  j += ",\"bit47Changes\":" + String((unsigned long)c47);
  j += ",\"bit18Rx0\":" + String((unsigned long)b18z) + ",\"bit18Rx1\":" + String((unsigned long)b18o);
  j += ",\"bit19Rx0\":" + String((unsigned long)b19z) + ",\"bit19Rx1\":" + String((unsigned long)b19o);
  j += ",\"bit47Rx0\":" + String((unsigned long)b47z) + ",\"bit47Rx1\":" + String((unsigned long)b47o);
  j += ",\"txOk\":" + String((unsigned long)txOk);
  j += ",\"txFail\":" + String((unsigned long)txFail);
  j += ",\"immediateTxOk\":" + String((unsigned long)immOk) + ",\"immediateTxFail\":" + String((unsigned long)immFail);
  j += ",\"periodicTxOk\":" + String((unsigned long)perOk) + ",\"periodicTxFail\":" + String((unsigned long)perFail);
  j += ",\"gateBlocked\":" + String((unsigned long)blocked);
  j += ",\"appliedFrames\":" + String((unsigned long)applied);
  j += ",\"noTemplateSkip\":" + String((unsigned long)noTemplateSkip);
  j += ",\"staleSkip\":0"; // backward-compatible: stock-age cutoff no longer exists
  j += ",\"queueSkip\":" + String((unsigned long)queueSkip);
  j += ",\"pending\":" + String(pendingSnapshot.pending ? "true" : "false");
  j += ",\"pendingKind\":\"" + String(r79PendingKindNamePure(pendingSnapshot.kind)) + "\"";
  j += ",\"pendingAgeMs\":" + String((unsigned long)pendingAge);
  j += ",\"pendingAttemptAgeMs\":" + String((unsigned long)pendingAttemptAge);
  j += ",\"pendingSequence\":" + String((unsigned long)pendingSnapshot.sequence);
  j += ",\"pendingRequestCount\":" + String((unsigned long)pendingRequests);
  j += ",\"pendingCoalesceCount\":" + String((unsigned long)pendingCoalesced);
  j += ",\"pendingRetryCount\":" + String((unsigned long)pendingRetries);
  j += ",\"freshMaskWaitCount\":" + String((unsigned long)freshMaskWaits);
  j += ",\"requiredFreshMask\":" + String((unsigned)currentRequiredFreshMask);
  j += ",\"requiredFreshMaskName\":\"" + String(summonBusMaskNamePure(currentRequiredFreshMask)) + "\"";
  j += ",\"observedFreshMask\":" + String((unsigned)currentObservedFreshMask);
  j += ",\"observedFreshMaskName\":\"" + String(summonBusMaskNamePure(currentObservedFreshMask)) + "\"";
  j += ",\"freshMaskReady\":" + String(currentFreshReady ? "true" : "false");
  j += ",\"lastAttemptRequiredFreshMask\":" + String((unsigned)lastRequiredFreshMask);
  j += ",\"lastAttemptRequiredFreshMaskName\":\"" + String(summonBusMaskNamePure(lastRequiredFreshMask)) + "\"";
  j += ",\"lastAttemptObservedFreshMask\":" + String((unsigned)lastObservedFreshMask);
  j += ",\"lastAttemptObservedFreshMaskName\":\"" + String(summonBusMaskNamePure(lastObservedFreshMask)) + "\"";
  j += ",\"lastBlockReason\":\"" + String(r79LabBlockReasonName(lastBlockReason)) + "\"";
  j += ",\"lastBlockAgeMs\":" + String((unsigned long)blockAge);
  j += ",\"reassertLatencyLastMs\":" + String((unsigned long)reassertLatencyLast);
  j += ",\"reassertLatencyMaxMs\":" + String((unsigned long)reassertLatencyMax);
  j += ",\"lastAttemptAgeMs\":" + String((unsigned long)(lastAttempt?now-lastAttempt:999999UL));
  j += ",\"lastTxAgeMs\":" + String((unsigned long)(lastTx?now-lastTx:999999UL));
  j += ",\"stockRaw\":\"" + String(stockHex) + "\"";
  j += ",\"lastTxRaw\":\"" + String(lastTxHex) + "\"";
  j += ",\"effectiveRaw\":\"" + String(lastTxHex) + "\"";
  j += "}"; return j;
}

static constexpr uint32_t CAN_TRAFFIC_UI_FRESH_MS = 1500;

struct CanTrafficUiSnapshot {
  bool mcpSeen;
  bool twaiSeen;
  bool mcpOnline;
  bool twaiOnline;
  uint32_t mcpAgeMs;
  uint32_t twaiAgeMs;
};

static CanTrafficUiSnapshot canTrafficUiSnapshot() {
  const uint32_t now = (uint32_t)millis();
  const uint32_t lastA = lastCanAFrameMs;
  const uint32_t lastB = lastCanBFrameMs;
  CanTrafficUiSnapshot t = {};
  t.mcpSeen = lastA != 0;
  t.twaiSeen = lastB != 0;
  t.mcpAgeMs = t.mcpSeen ? (uint32_t)(now - lastA) : 0;
  t.twaiAgeMs = t.twaiSeen ? (uint32_t)(now - lastB) : 0;
  t.mcpOnline = t.mcpSeen && t.mcpAgeMs <= CAN_TRAFFIC_UI_FRESH_MS;
  t.twaiOnline = t.twaiSeen && t.twaiAgeMs <= CAN_TRAFFIC_UI_FRESH_MS;
  return t;
}

static String canTrafficStatsToJson() {
  const CanTrafficUiSnapshot t = canTrafficUiSnapshot();
  uint32_t overflowCount, overflowLastMs;
  uint8_t overflowLastFlags;
  mcpRxOverflowSnapshot(overflowCount, overflowLastMs, overflowLastFlags);
  String s;
  s.reserve(384);
  s = "{";
  s += "\"mcpTrafficSeen\":" + String(t.mcpSeen ? "true" : "false");
  s += ",\"mcpTrafficOnline\":" + String(t.mcpOnline ? "true" : "false");
  s += ",\"mcpTrafficAgeMs\":" + String((unsigned long)t.mcpAgeMs);
  s += ",\"twaiTrafficSeen\":" + String(t.twaiSeen ? "true" : "false");
  s += ",\"twaiTrafficOnline\":" + String(t.twaiOnline ? "true" : "false");
  s += ",\"twaiTrafficAgeMs\":" + String((unsigned long)t.twaiAgeMs);
  s += ",\"trafficFreshMs\":" + String((unsigned long)CAN_TRAFFIC_UI_FRESH_MS);
  s += ",\"mcpRxOverflowCount\":" + String((unsigned long)overflowCount);
  s += ",\"mcpRxOverflowLastAgeMs\":" + String((unsigned long)(overflowLastMs ? millis() - overflowLastMs : 999999UL));
  s += ",\"mcpRxOverflowLastFlags\":" + String((unsigned)overflowLastFlags);
  s += "}";
  return s;
}

static String systemStatsToJson() {
  String s;
  s.reserve(4096);
  s = "{";
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t minFreeHeap = ESP.getMinFreeHeap();
  const uint32_t largestHeapBlock = ESP.getMaxAllocHeap();
  const uint32_t stackCanA = canTaskMcpHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskMcpHandle) : 0;
  const uint32_t stackCanB = canTaskTwaiHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canTaskTwaiHandle) : 0;
  const uint32_t stackCanSup = canSupervisorHandle ? (uint32_t)uxTaskGetStackHighWaterMark(canSupervisorHandle) : 0;
  const uint32_t stackWeb = webTaskHandle ? (uint32_t)uxTaskGetStackHighWaterMark(webTaskHandle) : 0;
  const uint32_t stackS3xy = s3xyTaskHandle ? (uint32_t)uxTaskGetStackHighWaterMark(s3xyTaskHandle) : 0;
  twai_status_info_t twaiNow = {};
  const bool twaiStatusOk = (twai_get_status_info(&twaiNow) == ESP_OK);
  CanTwaiRecoverySnapshot busOffSnap = {};
  uint32_t twaiBusOffCount, twaiStoppedCount, twaiLocalRecoveryStartCount;
  uint32_t twaiRecoveryStartFailCount, twaiRestartOkCount, twaiRestartFailCount;
  uint32_t twaiLastEventMs, lastRxGapMs, maxRxGapMs;
  uint8_t twaiLastEventReason, lastHardDiagReason;
  portENTER_CRITICAL(&canRecoveryMux);
  busOffSnap = canTwaiLastBusOffSnapshot;
  twaiBusOffCount = canTwaiBusOffCount;
  twaiStoppedCount = canTwaiStoppedCount;
  twaiLocalRecoveryStartCount = canTwaiLocalRecoveryStartCount;
  twaiRecoveryStartFailCount = canTwaiRecoveryStartFailCount;
  twaiRestartOkCount = canTwaiRestartOkCount;
  twaiRestartFailCount = canTwaiRestartFailCount;
  twaiLastEventReason = canTwaiLastEventReason;
  twaiLastEventMs = canTwaiLastEventMs;
  lastHardDiagReason = canLastHardDiagReason;
  lastRxGapMs = canBLastRxGapMs;
  maxRxGapMs = canBMaxRxGapMs;
  portEXIT_CRITICAL(&canRecoveryMux);
  const uint32_t statsNow = (uint32_t)millis();
  s += "\"fwVersion\":\"" + String(FW_VERSION) + "\"";
  s += ",\"freeHeap\":"      + String(freeHeap);
  s += ",\"minFreeHeap\":"   + String(minFreeHeap);
  s += ",\"largestHeapBlock\":" + String(largestHeapBlock);
  s += ",\"stackCanA\":"     + String(stackCanA);
  s += ",\"stackCanB\":"     + String(stackCanB);
  s += ",\"stackCanSup\":"   + String(stackCanSup);
  s += ",\"stackWeb\":"      + String(stackWeb);
  s += ",\"stackS3xy\":"     + String(stackS3xy);
  s += ",\"s3xyDiagnosticsEnabled\":" + String(S3XY_DIAGNOSTICS_ENABLED ? "true" : "false");
  s += ",\"s3xyLogCapacity\":" + String(S3XY_DIAGNOSTICS_ENABLED ? (unsigned)S3XY_LOG_MAX : 0U);
  s += ",\"uptimeS\":"      + String((millis() - bootTime) / 1000);
  s += ",\"mcpReady\":"     + String(mcpReady  ? "true" : "false");
  s += ",\"twaiReady\":"    + String(twaiReady ? "true" : "false");
  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  s += ",\"mcpTrafficSeen\":" + String(traffic.mcpSeen ? "true" : "false");
  s += ",\"mcpTrafficOnline\":" + String(traffic.mcpOnline ? "true" : "false");
  s += ",\"mcpTrafficAgeMs\":" + String((unsigned long)traffic.mcpAgeMs);
  s += ",\"twaiTrafficSeen\":" + String(traffic.twaiSeen ? "true" : "false");
  s += ",\"twaiTrafficOnline\":" + String(traffic.twaiOnline ? "true" : "false");
  s += ",\"twaiTrafficAgeMs\":" + String((unsigned long)traffic.twaiAgeMs);
  s += ",\"trafficFreshMs\":" + String((unsigned long)CAN_TRAFFIC_UI_FRESH_MS);
  uint32_t mcpOverflowCount, mcpOverflowLastMs;
  uint8_t mcpOverflowLastFlags;
  mcpRxOverflowSnapshot(mcpOverflowCount, mcpOverflowLastMs, mcpOverflowLastFlags);
  s += ",\"mcpRxOverflowCount\":" + String((unsigned long)mcpOverflowCount);
  s += ",\"mcpRxOverflowLastAgeMs\":" + String((unsigned long)(mcpOverflowLastMs ? millis() - mcpOverflowLastMs : 999999UL));
  s += ",\"mcpRxOverflowLastFlags\":" + String((unsigned)mcpOverflowLastFlags);
  s += ",\"rtcBootCount\":" + String((unsigned long)rtcBootCount);
  s += ",\"runtimeStatsResetCount\":" + String((unsigned long)runtimeStatsResetCount);
  s += ",\"runtimeStatsLastResetMs\":" + String((unsigned long)runtimeStatsLastResetMs);
  s += ",\"canHardReinit\":" + String((unsigned long)canHardReinitCount);
  s += ",\"canHardReinitFail\":" + String((unsigned long)canHardReinitFailCount);
  s += ",\"canLastHardReason\":" + String((int)canLastHardReinitReason);
  s += ",\"canLastHardDiagReason\":" + String((int)lastHardDiagReason);
  s += ",\"canLastHardDiagReasonName\":\"" + String(canRecoveryDiagnosticReasonName(lastHardDiagReason)) + "\"";
  s += ",\"canRecoverySleeping\":" + String(recoverySleeping ? "true" : "false");
  s += ",\"twaiState\":" + String(twaiStatusOk ? (int)twaiNow.state : -1);
  s += ",\"twaiStateName\":\"" + String(twaiStatusOk ? twaiStateName(twaiNow.state) : "UNAVAILABLE") + "\"";
  s += ",\"twaiBusOffCount\":" + String((unsigned long)twaiBusOffCount);
  s += ",\"twaiStoppedCount\":" + String((unsigned long)twaiStoppedCount);
  s += ",\"twaiLocalRecoveryStartCount\":" + String((unsigned long)twaiLocalRecoveryStartCount);
  s += ",\"twaiRecoveryStartFailCount\":" + String((unsigned long)twaiRecoveryStartFailCount);
  s += ",\"twaiRestartOkCount\":" + String((unsigned long)twaiRestartOkCount);
  s += ",\"twaiRestartFailCount\":" + String((unsigned long)twaiRestartFailCount);
  s += ",\"twaiLastEventReason\":" + String((int)twaiLastEventReason);
  s += ",\"twaiLastEventReasonName\":\"" + String(canRecoveryDiagnosticReasonName(twaiLastEventReason)) + "\"";
  s += ",\"twaiLastEventAgeMs\":" + String((unsigned long)(twaiLastEventMs ? statsNow - twaiLastEventMs : 999999UL));
  s += ",\"canBLastRxGapMs\":" + String((unsigned long)lastRxGapMs);
  s += ",\"canBMaxRxGapMs\":" + String((unsigned long)maxRxGapMs);
  s += ",\"twaiTxErrorCounter\":" + String((unsigned long)(twaiStatusOk ? twaiNow.tx_error_counter : 0));
  s += ",\"twaiRxErrorCounter\":" + String((unsigned long)(twaiStatusOk ? twaiNow.rx_error_counter : 0));
  s += ",\"twaiTxFailedCount\":" + String((unsigned long)(twaiStatusOk ? twaiNow.tx_failed_count : 0));
  s += ",\"twaiRxMissedCount\":" + String((unsigned long)(twaiStatusOk ? twaiNow.rx_missed_count : 0));
  s += ",\"twaiRxOverrunCount\":" + String((unsigned long)(twaiStatusOk ? twaiNow.rx_overrun_count : 0));
  s += ",\"twaiArbLostCount\":" + String((unsigned long)(twaiStatusOk ? twaiNow.arb_lost_count : 0));
  s += ",\"twaiBusErrorCount\":" + String((unsigned long)(twaiStatusOk ? twaiNow.bus_error_count : 0));
  s += ",\"twaiBusOffSnapshotValid\":" + String(busOffSnap.valid ? "true" : "false");
  s += ",\"twaiBusOffSnapshotAgeMs\":" + String((unsigned long)(busOffSnap.valid ? statsNow - busOffSnap.capturedMs : 999999UL));
  s += ",\"twaiBusOffSnapshotRxGapMs\":" + String((unsigned long)busOffSnap.rxGapMs);
  s += ",\"twaiBusOffSnapshotTxQueue\":" + String((unsigned long)busOffSnap.msgsToTx);
  s += ",\"twaiBusOffSnapshotRxQueue\":" + String((unsigned long)busOffSnap.msgsToRx);
  s += ",\"twaiBusOffSnapshotTxErr\":" + String((unsigned long)busOffSnap.txErrorCounter);
  s += ",\"twaiBusOffSnapshotRxErr\":" + String((unsigned long)busOffSnap.rxErrorCounter);
  s += ",\"twaiBusOffSnapshotTxFailed\":" + String((unsigned long)busOffSnap.txFailedCount);
  s += ",\"twaiBusOffSnapshotRxMissed\":" + String((unsigned long)busOffSnap.rxMissedCount);
  s += ",\"twaiBusOffSnapshotRxOverrun\":" + String((unsigned long)busOffSnap.rxOverrunCount);
  s += ",\"twaiBusOffSnapshotArbLost\":" + String((unsigned long)busOffSnap.arbLostCount);
  s += ",\"twaiBusOffSnapshotBusError\":" + String((unsigned long)busOffSnap.busErrorCount);
  uint8_t txTraceFrozenCount = 0;
  uint32_t txTraceFrozenMs = 0, txTraceBusOffOrdinal = 0;
  portENTER_CRITICAL(&canBTxTraceMux);
  txTraceFrozenCount = canBTxTraceFrozenCount;
  txTraceFrozenMs = canBTxTraceFrozenMs;
  txTraceBusOffOrdinal = canBTxTraceFrozenBusOffOrdinal;
  portEXIT_CRITICAL(&canBTxTraceMux);
  s += ",\"canBTxTraceCount\":" + String((unsigned)txTraceFrozenCount);
  s += ",\"canBTxTraceAgeMs\":" + String((unsigned long)(txTraceFrozenMs ? statsNow - txTraceFrozenMs : 999999UL));
  s += ",\"canBTxTraceBusOffOrdinal\":" + String((unsigned long)txTraceBusOffOrdinal);
  s += ",\"twaiTxQueueNow\":" + String((unsigned long)twaiTxQueueNow);
  s += ",\"twaiTxQueueMax\":" + String((unsigned long)twaiTxQueueMax);
  s += ",\"twaiRxQueueNow\":" + String((unsigned long)twaiRxQueueNow);
  s += ",\"twaiRxQueueMax\":" + String((unsigned long)twaiRxQueueMax);
  s += ",\"twaiNonSummonShed\":" + String((unsigned long)twaiNonSummonShed);
  s += ",\"twaiStandbyShed\":" + String((unsigned long)twaiStandbyShed);
  s += ",\"twaiFullShed\":" + String((unsigned long)twaiFullShed);
  s += ",\"summonPriorityState\":" + String((int)getSummonPriorityState());
  s += ",\"summonPriorityStateName\":\"" + String(summonPriorityStateName(getSummonPriorityState())) + "\"";
  s += ",\"twaiSummonTxNormal\":" + String((unsigned long)twaiSummonTxNormal);
  s += ",\"twaiSummonTxStandby\":" + String((unsigned long)twaiSummonTxStandby);
  s += ",\"twaiSummonTxFull\":" + String((unsigned long)twaiSummonTxFull);
  s += ",\"twaiSummonQueueFlush\":" + String((unsigned long)twaiSummonQueueFlush);
  s += ",\"twaiSummonRetryOk\":" + String((unsigned long)twaiSummonRetryOk);
  s += ",\"twaiSummonRetryFail\":" + String((unsigned long)twaiSummonRetryFail);
  s += ",\"otaInProgress\":" + String(otaInProgress ? "true" : "false");
  s += ",\"otaSuccess\":"    + String(otaSuccess    ? "true" : "false");
  s += ",\"otaError\":"      + String(otaError      ? "true" : "false");
  s += ",\"otaErrMsg\":\""   + String(otaErrMsg) + "\"";
  s += ",\"otaBytes\":"      + String(otaBytes);
  s += ",\"otaTotal\":"      + String(otaTotal);
  s += "}";
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
  bootCaptureAppendEvent(out, "FIRST_CAN_B_ANY", firstB, "VH/TWAI");

  String d370;
  if (first370Raw != 0xFFFF) {
    const float nm = first370Raw * 0.01f - 20.5f;
    d370 = "raw=" + String((unsigned)first370Raw) + ";torque_nm=" + String(nm, 2);
  } else d370 = "not_seen";
  bootCaptureAppendEvent(out, "FIRST_PARTY_0x370", first370, d370);

  String dTorque;
  if (first370TorqueRaw != 0xFFFF) {
    const float nm = first370TorqueRaw * 0.01f - 20.5f;
    dTorque = "abs_torque_ge_0.10Nm;raw=" + String((unsigned)first370TorqueRaw) + ";torque_nm=" + String(nm, 2);
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
  server.sendHeader("Content-Disposition", "attachment; filename=T2CAN_boot_capture.csv");
  server.send(200, "text/csv", bootCaptureToCsv());
}

static const char *canBTxTraceSourceName(uint16_t id) {
  switch (id) {
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

  server.sendHeader("Content-Disposition", "attachment; filename=T2CAN_CANB_TX_TRACE.csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  char line[220];
  snprintf(line, sizeof(line), "#bus_off_ordinal,%lu\n#frozen_uptime_ms,%lu\n#entry_count,%u\n",
           (unsigned long)busOffOrdinal, (unsigned long)frozenMs, (unsigned)count);
  server.sendContent(line);
  server.sendContent("seq,relative_ms,uptime_ms,id,source,dlc,result,result_name,raw\n");
  for (uint8_t i = 0; i < count; i++) {
    const CanBTxTraceEntry &e = entries[i];
    char raw[32] = {};
    char *w = raw;
    size_t remain = sizeof(raw);
    for (uint8_t j = 0; j < e.dlc && j < 8; j++) {
      const int n = snprintf(w, remain, "%s%02X", j ? " " : "", e.data[j]);
      if (n <= 0 || (size_t)n >= remain) break;
      w += n; remain -= (size_t)n;
    }
    const int32_t rel = frozenMs ? (int32_t)(e.capturedMs - frozenMs) : 0;
    snprintf(line, sizeof(line), "%lu,%ld,%lu,0x%03X,%s,%u,%ld,%s,%s\n",
             (unsigned long)e.seq, (long)rel, (unsigned long)e.capturedMs,
             (unsigned)e.id, canBTxTraceSourceName(e.id), (unsigned)e.dlc,
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
  snprintf(buf, sizeof(buf), "T2CAN-%02X%02X", mac[4], mac[5]);
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
    Serial.printf("WiFi: AP start failed for SSID=%s attempt=%u/3\n",
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
  Serial.println("WiFi: custom AP failed; restoring fail-safe default AP");
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
  j = "{\"ok\":true,\"ssid\":\"" + wifiApJsonEscape(ssid) + "\"";
  j += ",\"passwordIsDefault\":" + String(password == "12345678" ? "true" : "false");
  j += ",\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
  j += "}";
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
  Serial.printf("WiFi: AP config apply %s · SSID=%s IP=%s\n",
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
  const uint8_t action = s3xyParseAction(server.arg("action"));
  if (!s3xyActionSupportedForCurrentProfile(action)) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Acceleration Mode Toggle unavailable for current CAN topology\"}");
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
  server.sendHeader("Content-Disposition", "attachment; filename=T2CAN_S3XY_multi.csv");
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
        Serial.printf("[OTA] Start: %s\n", up.filename.c_str());

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] begin() failed: %s\n", otaErrMsg);
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (!otaError && Update.write(up.buf, up.currentSize) != up.currentSize) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] write() failed: %s\n", otaErrMsg);
        }
        otaBytes += up.currentSize;
    } else if (up.status == UPLOAD_FILE_END) {
        if (!otaError && Update.end(true)) {
            otaSuccess = true;
            otaTotal   = otaBytes;
            Serial.printf("[OTA] Success: %u bytes\n", up.totalSize);
        } else if (!otaError) {
            otaError = true;
            strncpy(otaErrMsg, Update.errorString(), sizeof(otaErrMsg) - 1);
            Serial.printf("[OTA] end() failed: %s\n", otaErrMsg);
        }
        otaInProgress = false;
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        otaInProgress = false;
        otaError      = true;
        strncpy(otaErrMsg, "aborted", sizeof(otaErrMsg) - 1);
        Serial.println("[OTA] Aborted");
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
        ESP.restart();
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

static const char *researchCaptureBusName(uint8_t bus) {
  const bool canA = bus == RESEARCH_CAPTURE_BUS_PARTY ||
                    bus == RESEARCH_CAPTURE_BUS_PARTY_TX_OK ||
                    bus == RESEARCH_CAPTURE_BUS_PARTY_TX_FAIL;
  return canA ? activeProfileCanAName()
              : activeProfileCanBName();
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
  const float virtualLaneWidth = laneValid ? (2.0f + 0.3125f * (float)((lane239.data[0] >> 4) & 0x0FU)) : 0.0f;
  const uint16_t laneViewRange = laneValid ? (uint16_t)lane239.data[1] : 0U;
  const float virtualLaneC0 = laneValid ? (-3.5f + 0.035f * (float)lane239.data[2]) : 0.0f;
  const float virtualLaneC1 = laneValid ? (-0.2f + 0.0016f * (float)lane239.data[3]) : 0.0f;
  const float virtualLaneC2 = laneValid ? (-0.0025f + 0.00002f * (float)lane239.data[4]) : 0.0f;
  const bool rawMode = researchCaptureModeIsRaw(mode);
  const uint32_t rawRequiredPreMs = mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC
      ? RESEARCH_CAPTURE_RAW_AUTO_PRE_MS : RESEARCH_CAPTURE_RAW_MANUAL_PRE_MS;
  const bool rawPreReady = rawPreCoverageMs >= rawRequiredPreMs;
  const bool rawManualPreReady = rawPreCoverageMs >= RESEARCH_CAPTURE_RAW_MANUAL_PRE_MS;

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
  const float rawRingUsagePct = RESEARCH_CAPTURE_RAW_PRE_CAPACITY
      ? (100.0f * (float)rawPreFrameCount / (float)RESEARCH_CAPTURE_RAW_PRE_CAPACITY) : 0.0f;
  const float rawArchiveUsagePct = RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY
      ? (100.0f * (float)rawArchiveCount / (float)RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY) : 0.0f;
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
  j += ",\"state\":\"" + String(researchCaptureStateName(state)) + "\"";
  j += ",\"fullReason\":\"" + String(fullReason) + "\"";
  j += ",\"exporting\":" + String(exporting ? "true" : "false");
  j += ",\"captureMode\":\"" + String(researchCaptureModeName(mode)) + "\"";
  j += ",\"canAName\":\"" + String(activeProfileCanAName()) + "\"";
  j += ",\"canBName\":\"" + String(activeProfileCanBName()) + "\"";
  j += ",\"rawAutoAlcSupported\":" + String(activeProfileIsYl() ? "true" : "false");
  j += ",\"capturing\":" + String(state == RESEARCH_CAPTURE_CAPTURING ? "true" : "false");
  j += ",\"segment\":" + String((unsigned)segment);
  j += ",\"completedSegments\":" + String((unsigned)completed);
  j += ",\"currentSlot\":\"" + String(researchCaptureLabelSlotName(currentSlot)) + "\"";
  j += ",\"lastSlot\":\"" + String(researchCaptureLabelSlotName(lastSlot)) + "\"";
  j += ",\"currentLabel\":\"" + researchCaptureJsonEscape(currentLabel) + "\"";
  j += ",\"lastLabel\":\"" + researchCaptureJsonEscape(lastLabel) + "\"";
  j += ",\"labels\":{";
  for (uint8_t i = 0; i < RESEARCH_CAPTURE_LABEL_SLOTS; i++) {
    if (i) j += ',';
    j += "\"" + String(researchCaptureLabelSlotName(i)) + "\":\"" + researchCaptureJsonEscape(labels[i]) + "\"";
  }
  j += "}";
  j += ",\"samples\":" + String((unsigned long)count);
  j += ",\"capacity\":" + String((unsigned long)effectiveCapacity);
  j += ",\"maxSegments\":" + String((unsigned)effectiveMaxSegments);
  j += ",\"knownIds\":" + String((unsigned)knownIds);
  j += ",\"preSnapshotsReady\":" + String((unsigned)preValid);
  j += ",\"preSnapshotSlots\":" + String((unsigned)RESEARCH_CAPTURE_PRE_SLOT_COUNT);
  j += ",\"lastPreSnapshotsCopied\":" + String((unsigned)lastPre);
  j += ",\"lastTriggerRows\":" + String((unsigned)triggerRows);
  j += ",\"lastPostSnapshotsCopied\":" + String((unsigned)lastPost);
  j += ",\"nextPostIndex\":" + String((unsigned)nextPost);
  j += ",\"remainingMs\":" + String((unsigned long)remaining);
  j += ",\"preMs\":" + String((unsigned long)preWindowMs); // backward-compatible alias
  j += ",\"postMs\":" + String((unsigned long)postWindowMs); // backward-compatible alias
  j += ",\"preWindowMs\":" + String((unsigned long)preWindowMs);
  j += ",\"postWindowMs\":" + String((unsigned long)postWindowMs);
  j += ",\"preDenseIntervalMs\":" + String((unsigned long)RESEARCH_CAPTURE_PRE_DENSE_INTERVAL_MS);
  j += ",\"preExtendedIntervalMs\":" + String((unsigned long)extendedPreIntervalMs);
  j += ",\"postSnapshotCount\":" + String((unsigned)postCount);
  j += ",\"requests\":" + String((unsigned long)requests);
  j += ",\"ignored\":" + String((unsigned long)ignored);
  j += ",\"dropped\":" + String((unsigned long)dropped);
  j += ",\"rawTriggered\":" + String(rawTriggered ? "true" : "false");
  j += ",\"rawTriggerMs\":" + String((unsigned long)rawTriggerMs);
  j += ",\"rawRingStartIndex\":" + String((unsigned long)rawPreStartIndex);
  j += ",\"rawRingFrames\":" + String((unsigned long)rawPreFrameCount);
  j += ",\"rawRingCapacity\":" + String((unsigned long)RESEARCH_CAPTURE_RAW_PRE_CAPACITY);
  j += ",\"rawRingUsagePct\":" + String(rawRingUsagePct, 1);
  j += ",\"rawArchiveFrames\":" + String((unsigned long)rawArchiveCount);
  j += ",\"rawArchiveCapacity\":" + String((unsigned long)RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY);
  j += ",\"rawArchiveUsagePct\":" + String(rawArchiveUsagePct, 1);
  j += ",\"rawEvicted\":" + String((unsigned long)rawEvicted);
  j += ",\"rawPreCoverageMs\":" + String((unsigned long)rawPreCoverageMs);
  j += ",\"rawPreReady\":" + String(rawPreReady ? "true" : "false");
  j += ",\"rawManualPreReady\":" + String(rawManualPreReady ? "true" : "false");
  j += ",\"autoQualifiedTransitions\":" + String((unsigned long)autoQualifiedTransitions);
  j += ",\"autoTriggerCount\":" + String((unsigned long)autoTriggerCount);
  j += ",\"autoRejectLane\":" + String((unsigned long)autoRejectLane);
  j += ",\"autoRejectWarmup\":" + String((unsigned long)autoRejectWarmup);
  j += ",\"autoRejectBusy\":" + String((unsigned long)autoRejectBusy);
  j += ",\"autoMergedEvents\":" + String((unsigned long)autoMergedEvents);
  j += ",\"autoOpenCount\":" + String((unsigned long)autoOpenCount);
  j += ",\"autoBlockedCount\":" + String((unsigned long)autoBlockedCount);
  j += ",\"autoLastTriggerAgeMs\":" + String((unsigned long)(autoLastTriggerMs ? now - autoLastTriggerMs : 999999UL));
  j += ",\"autoLastEvent\":\"" + String(researchCaptureAutoEventName(autoLastEvent)) + "\"";
  j += ",\"autoLastFrom\":" + String((unsigned)autoLastFrom);
  j += ",\"autoLastTo\":" + String((unsigned)autoLastTo);
  j += ",\"autoLastFromName\":\"" + String(autoLastFrom == 0xFF ? "NONE" : alcStateName(autoLastFrom)) + "\"";
  j += ",\"autoLastToName\":\"" + String(autoLastTo == 0xFF ? "NONE" : alcStateName(autoLastTo)) + "\"";
  j += ",\"rawPreMs\":" + String((unsigned long)rawRequiredPreMs);
  j += ",\"rawPostMs\":" + String((unsigned long)RESEARCH_CAPTURE_RAW_POST_MS);
  j += ",\"usingPsram\":" + String(psram ? "true" : "false");
  j += ",\"memoryBytes\":" + String((unsigned long)memoryBytes);
  j += ",\"psramTotalBytes\":" + String((unsigned long)psramTotalBytes);
  j += ",\"psramFreeBytes\":" + String((unsigned long)psramFreeBytes);
  j += ",\"snapshotPlan\":\"" + String(rawMode
      ? (mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC ? "RAW AUTO LEFT OPEN 6/8 ↔ BLOCKED other state · PRE 2s + POST 2s · manual C/D PRE 5s + POST 2s" : "RAW RX ring PRE 5s + trigger + POST 2s · auto re-arm")
      : "rolling snapshot PRE + trigger + selected POST") + "\"";
  j += ",\"alcValid\":" + String(alcValid ? "true" : "false");
  j += ",\"alcRaw\":" + String((unsigned)alcRaw);
  j += ",\"alcName\":\"" + String(alcValid ? alcStateName(alcRaw) : "NO DATA") + "\"";
  j += ",\"alcAgeMs\":" + String((unsigned long)alcAge);
  j += ",\"lane239Valid\":" + String(laneValid ? "true" : "false");
  j += ",\"lane239AgeMs\":" + String((unsigned long)laneAge);
  j += ",\"leftLaneExists\":" + String((unsigned)leftLaneExists);
  j += ",\"rightLaneExists\":" + String((unsigned)rightLaneExists);
  j += ",\"leftLineUsageRaw\":" + String((unsigned)leftLineUsage);
  j += ",\"rightLineUsageRaw\":" + String((unsigned)rightLineUsage);
  j += ",\"leftForkRaw\":" + String((unsigned)leftFork);
  j += ",\"rightForkRaw\":" + String((unsigned)rightFork);
  j += ",\"virtualLaneWidth\":" + String(virtualLaneWidth, 4);
  j += ",\"laneViewRange\":" + String((unsigned)laneViewRange);
  j += ",\"virtualLaneC0\":" + String(virtualLaneC0, 4);
  j += ",\"virtualLaneC1\":" + String(virtualLaneC1, 5);
  j += ",\"virtualLaneC2\":" + String(virtualLaneC2, 6);
  j += "}";
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
  else if (raw == "RAW_AUTO_ALC" || raw == "AUTO_ALC" || raw == "AUTO" || raw == "2") mode = RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC;
  if (mode == 0xFF) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"mode must be SNAPSHOT, RAW_TRANSITION or RAW_AUTO_ALC\"}");
    return;
  }
  if (mode == RESEARCH_CAPTURE_MODE_RAW_AUTO_ALC && !activeProfileIsYl()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"RAW AUTO ALC is available only on Model Y L\"}");
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

  server.sendHeader("Content-Disposition", "attachment; filename=CAN_Research_Capture.csv");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  server.sendContent("segment_id,label_slot,label,phase,relative_ms,trigger_uptime_ms,snapshot_uptime_ms,frame_age_ms,bus,source,tx_ok,id,dlc,raw\n");

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
        const bool isPartyTx = e.bus == RESEARCH_CAPTURE_BUS_PARTY_TX_OK ||
                               e.bus == RESEARCH_CAPTURE_BUS_PARTY_TX_FAIL;
        const char *bus = researchCaptureBusName(e.bus);
        const char *source = isPartyTx ? "T2CAN_TX" : "RX";
        const int txOk = isPartyTx ? (e.bus == RESEARCH_CAPTURE_BUS_PARTY_TX_OK ? 1 : 0) : -1;
        char raw[32]; researchCaptureCsvHex(e.data, e.dlc, raw, sizeof(raw));
        snprintf(line, sizeof(line), "%u,%s,%s,%s,%ld,%lu,%lu,0,%s,%s,%d,0x%03X,%u,\"%s\"\n",
                 (unsigned)seg, researchCaptureLabelSlotName(meta.labelSlot), quotedLabel, phase, (long)rel,
                 (unsigned long)meta.triggerMs, (unsigned long)e.timestampMs, bus, source, txOk,
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
    const char *bus = researchCaptureBusName(e.bus);
    const bool isTx = (e.flags & RESEARCH_CAPTURE_FLAG_TX) != 0;
    const int txOk = isTx ? ((e.flags & RESEARCH_CAPTURE_FLAG_TX_OK) != 0 ? 1 : 0) : -1;
    const int64_t snapMs = (int64_t)meta.triggerMs + (int64_t)e.relativeMs;
    char raw[32]; researchCaptureCsvHex(e.data, e.dlc, raw, sizeof(raw));
    char quotedLabel[(RESEARCH_CAPTURE_LABEL_BYTES * 2) + 4];
    researchCaptureCsvQuote(meta.label, quotedLabel, sizeof(quotedLabel));
    snprintf(line, sizeof(line), "%u,%s,%s,%s,%d,%lu,%lld,%u,%s,%s,%d,0x%03X,%u,\"%s\"\n",
             (unsigned)e.segment, researchCaptureLabelSlotName(meta.labelSlot), quotedLabel, phase, (int)e.relativeMs,
             (unsigned long)meta.triggerMs, (long long)snapMs, (unsigned)e.frameAgeMs,
             bus, isTx ? "T2CAN_TX" : "RX", txOk, (unsigned)e.id, (unsigned)e.dlc, raw);
    researchCaptureCsvAppend(chunk, line);
    if ((i & 0x3FFU) == 0x3FFU) vTaskDelay(1);
  }
  researchCaptureCsvFlush(chunk);
  server.sendContent("", 0);
  researchCaptureCsvEndExport();
}

static void httpSystemStats() { server.send(200, "application/json", systemStatsToJson()); }



// ─── Universal v3.3 profile / feature policy APIs ───────────────────
static String vehicleProfileStatusJson() {
  String j;
  j.reserve(420);
  j += "{\"ok\":true";
  j += ",\"setupMode\":"; j += vehicleProfileSetupMode ? "true" : "false";
  j += ",\"nvsError\":"; j += vehicleProfileNvsError ? "true" : "false";
  j += ",\"migrationNotice\":"; j += vehicleProfileMigrationNotice ? "true" : "false";
  j += ",\"profile\":"; j += String((unsigned)activeVehicleProfile);
  j += ",\"profileName\":\""; j += vehicleProfileName(activeVehicleProfile); j += "\"";
  j += ",\"topology\":"; j += String((unsigned)activeVehicleTopology);
  j += ",\"topologyName\":\""; j += vehicleProfileTopologyName(activeVehicleTopology); j += "\"";
  j += ",\"canA\":\""; j += activeProfileCanAName(); j += "\"";
  j += ",\"canB\":\""; j += activeProfileCanBName(); j += "\"";
  j += ",\"turn\":"; j += String((unsigned)activeTurnSignalVariant);
  j += ",\"turnName\":\""; j += turnSignalVariantName(activeTurnSignalVariant); j += "\"";
  j += ",\"nagSupported\":"; j += activeProfileNagSupported() ? "true" : "false";
  j += ",\"advancedEapSupported\":"; j += activeProfileAdvancedEapSupported() ? "true" : "false";
  j += ",\"pedalMapSupported\":"; j += activeProfilePedalMapSupported() ? "true" : "false";
  j += ",\"bodyControlsSupported\":"; j += activeProfileBodyControlsSupported() ? "true" : "false";
  j += ",\"euUnlockSupported\":"; j += activeProfileEuUnlockSupported() ? "true" : "false";
  j += ",\"tlsscRestoreSupported\":"; j += activeProfileTlsscRestoreSupported() ? "true" : "false";
  j += ",\"bannedSupported\":"; j += activeProfileBannedCarSupported() ? "true" : "false";
  j += "}";
  return j;
}

static String v3FeaturePolicyJson() {
  const bool doorCancelReported = (activeProfileBodyControlsSupported() || activeProfileIsYl()) && doorOpenCancelEnabled;
  String j;
  j.reserve(280);
  j += "{\"ok\":true";
  j += ",\"lab\":"; j += labMenuEnabled ? "true" : "false";
  j += ",\"doorCancel\":"; j += doorCancelReported ? "true" : "false";
  j += ",\"nagSupported\":"; j += activeProfileNagSupported() ? "true" : "false";
  j += ",\"advancedEapSupported\":"; j += activeProfileAdvancedEapSupported() ? "true" : "false";
  j += ",\"pedalMapSupported\":"; j += activeProfilePedalMapSupported() ? "true" : "false";
  j += ",\"bodyControlsSupported\":"; j += activeProfileBodyControlsSupported() ? "true" : "false";
  j += ",\"euUnlockSupported\":"; j += activeProfileEuUnlockSupported() ? "true" : "false";
  j += ",\"banned\":"; j += bannedCar ? "true" : "false";
  j += ",\"tlsscRestore\":"; j += tlsscRestoreEnabled ? "true" : "false";
  j += ",\"tlsscRestoreSupported\":"; j += activeProfileTlsscRestoreSupported() ? "true" : "false";
  j += ",\"bannedSupported\":"; j += activeProfileBannedCarSupported() ? "true" : "false";
  j += ",\"s3xy\":"; j += s3xyBluetoothMasterIsEnabled() ? "true" : "false";
  j += "}";
  return j;
}

static bool httpBoolArg(const char *name, bool &out) {
  if (!server.hasArg(name)) return false;
  const String a = server.arg(name);
  if (a == "1" || a == "true" || a == "on") { out = true; return true; }
  if (a == "0" || a == "false" || a == "off") { out = false; return true; }
  return false;
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

  uint8_t topology = (uint8_t)vehicleProfileDefaultTopology((uint8_t)profile);
  if ((uint8_t)profile != VEHICLE_MODEL_YL) {
    if (!server.hasArg("topology")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing topology\"}");
      return;
    }
    topology = (uint8_t)server.arg("topology").toInt();
  } else if (server.hasArg("topology")) {
    topology = (uint8_t)server.arg("topology").toInt();
  }
  if (!vehicleProfileTopologyValid((uint8_t)profile, topology)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid topology for profile\"}");
    return;
  }

  uint8_t turn = TURN_SIGNAL_UNSET;
  if (topology != VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS) {
    turn = (uint8_t)vehicleProfileDefaultTurn((uint8_t)profile);
    if ((uint8_t)profile == VEHICLE_MODEL_3_HIGHLAND) {
      if (!server.hasArg("turn")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Highland Body+Chassis requires turn variant\"}");
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
  canTxAdministrativeHold = true;
  if (!vehicleProfileSetupMode) invalidateCanTxStateForFullRecovery();
  if (!vehicleProfileSave((uint8_t)profile, topology, turn)) {
    canTxAdministrativeHold = false;
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"profile NVS write failed\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
  delay(300);
  ESP.restart();
}

static void httpFeatureLab() {
  bool enabled;
  if (!httpBoolArg("enabled", enabled)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid enabled\"}");
    return;
  }
  labMenuEnabled = enabled;
  if (enabled && !researchCaptureEntries) {
    if (!researchCaptureInit()) {
      labMenuEnabled = false;
      featureCfgSave();
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"Research Capture allocation failed\"}");
      return;
    }
  } else if (!enabled) {
    portENTER_CRITICAL(&researchCaptureMux);
    if (!researchCaptureExporting && researchCaptureState == RESEARCH_CAPTURE_CAPTURING)
      researchCaptureState = RESEARCH_CAPTURE_PAUSED;
    portEXIT_CRITICAL(&researchCaptureMux);
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
    canTxAdministrativeHold = true;
    tlsscRestoreEnabled = false;
    bannedCar = false;
    featureCfgSave();
    canTxAdministrativeHold = false;
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
    "nag", "summon", "lab3f8", "r79lab", "researchcap", "features"
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

static void httpResetFirmwareSettings() {
  if (vehicleProfileSetupMode || vehicleProfileNvsError) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"not available in setup mode\"}");
    return;
  }
  canTxAdministrativeHold = true;
  invalidateCanTxStateForFullRecovery();
  tlsscRestoreEnabled = false;
  if (!resetFirmwareSettingsPreserveProfileAndBle()) {
    canTxAdministrativeHold = false;
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"settings reset failed\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true,\"profilePreserved\":true,\"blePreserved\":true}");
  delay(300);
  ESP.restart();
}

static void httpFactoryReset() {
  canTxAdministrativeHold = true;
  if (!vehicleProfileSetupMode && !vehicleProfileNvsError) invalidateCanTxStateForFullRecovery();
  const bool ok = factoryResetAllNvs();
  server.send(ok ? 200 : 500, "application/json",
              ok ? "{\"ok\":true,\"rebooting\":true}"
                 : "{\"ok\":false,\"error\":\"factory reset failed; rebooting safe\"}");
  delay(300);
  ESP.restart();
}

static void httpCanHardReinit() {
  requestCanSubsystemRestart(CAN_SUP_HARD_MANUAL, CAN_REC_MANUAL);
  server.send(202, "application/json", "{\"ok\":true,\"action\":\"hard-can-reinit-requested\"}");
}

static void httpRebootT2Can() {
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"rebooting\"}");
  delay(250);
  ESP.restart();
}

static void httpRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", (const char*)INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
}
static void httpNagConfig() { server.send(200, "application/json", nagCfgToJson()); }
static void httpNagStats()  { server.send(200, "application/json", nagStatsToJson()); }

static void httpNagSetMode() {
  int m = server.arg("m").toInt();
  bool pauseAtZero = false;
  portENTER_CRITICAL(&nagCfgMux);
  pauseAtZero = nagCfg.pauseAtZeroSpeed;
  portEXIT_CRITICAL(&nagCfgMux);

  NagConfig nc;
  if      (m == MODE_B) nagCfgDefaultsModeB(nc);
  else if (m == MODE_C) nagCfgDefaultsModeC(nc);
  else if (m == MODE_D) nagCfgDefaultsModeD(nc);
  else if (m == MODE_E) nagCfgDefaultsModeE(nc);
  else if (m == MODE_F) nagCfgDefaultsModeF(nc);
  else if (m == MODE_H) nagCfgDefaultsModeH(nc);
  else                  nagCfgDefaultsModeA(nc);
  // Pause-at-zero is a common NAG policy, not a mode waveform parameter.
  // Preserve it when switching modes. Explicit NAG reset still restores OFF.
  nc.pauseAtZeroSpeed = pauseAtZero;
  nagCfgCommit(nc);
  nagCfgSave();
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagUpdate() {
  NagConfig nc;
  portENTER_CRITICAL(&nagCfgMux); nc = nagCfg; portEXIT_CRITICAL(&nagCfgMux);
  if (server.hasArg("enabled"))
    nc.enabled = (server.arg("enabled") == "1");
  if (server.hasArg("pauseAtZeroSpeed"))
    nc.pauseAtZeroSpeed = (server.arg("pauseAtZeroSpeed") == "1" || server.arg("pauseAtZeroSpeed") == "true");
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
  nagCfgCommit(nc);
  nagCfgSave();
  server.send(200, "application/json", nagCfgToJson());
}

static void httpNagReset() {
  NagConfig nc;
  nagCfgDefaultsModeA(nc);
  nagCfgCommit(nc);
  nagCfgSave();
  nagRxFrames = nagEchoCount = mcpTxOk = mcpTxFail = 0;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOk=nagTxFail=0; nagSkipDisabled=nagSkipBootDelay=nagSkipWarmup=nagSkipSelfFrame=0;
  nagSkipHandsOn=nagSkipApInvalid=nagSkipApInactive=nagSkipDecision=nagSkipStopped=nagSkipCadence=nagSkipSpeedStale=0;
  nagBlockMutex=nagBlockMcpNotReady=nagBlockEpoch=nagBlockFreshMask=nagBlockInvalidMsg=nagSendError=0;
  nagLastTxOkMs=nagMaxTxGapMs=nagSessionTxOk=0; nagSessionStartMs=0;
  nagLastSkipReason=NAG_SKIP_NONE; nagLastSkipMs=0; nagLastTxBlockReason=MCP_TX_OK; nagLastTxBlockMs=0;
  portEXIT_CRITICAL(&nagDiagMux);
  portENTER_CRITICAL(&nagPortableMux);
  nagPortableLastTxValid=false; nagPortableLastTxMs=0; memset(nagPortableLastTxRaw,0,sizeof(nagPortableLastTxRaw));
  portEXIT_CRITICAL(&nagPortableMux);
  server.send(200, "application/json", nagCfgToJson());
}

static void httpSummonStats()  { server.send(200, "application/json", summonStatsToJson()); }
static void httpSummonTlsscEnable() {
    portENTER_CRITICAL(&stateMux); tlsscEnabled = true;  portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
    server.send(200, "application/json", summonStatsToJson());
}
static void httpSummonTlsscDisable() {
    portENTER_CRITICAL(&stateMux); tlsscEnabled = false; portEXIT_CRITICAL(&stateMux);
    summonCfgSave();
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

static void httpDasTelemetryStats() {
  server.send(200, "application/json", dasTelemetryStatsToJson());
}


static bool httpRequireLab() {
  if (labMenuEnabled) return true;
  server.send(409, "application/json", "{\"ok\":false,\"error\":\"LAB disabled\"}");
  return false;
}

static String nagHumanLabStatsToJson() {
  const NagHumanConfigPure c = nagHumanRuntimeConfigSnapshot();
  const NagHumanStatePure state = nagHumanRuntimeSnapshot();
  String j;
  j.reserve(720);
  j = "{";
  j += "\"peakMinNm\":" + String((float)c.peakMinRaw / 100.0f, 2);
  j += ",\"peakMaxNm\":" + String((float)c.peakMaxRaw / 100.0f, 2);
  j += ",\"allowedMinNm\":" + String((float)NAG_HUMAN_PEAK_ALLOWED_MIN_RAW / 100.0f, 2);
  j += ",\"allowedMaxNm\":" + String((float)NAG_HUMAN_PEAK_ALLOWED_MAX_RAW / 100.0f, 2);
  j += ",\"defaultMinNm\":" + String((float)NAG_HUMAN_PEAK_DEFAULT_MIN_RAW / 100.0f, 2);
  j += ",\"defaultMaxNm\":" + String((float)NAG_HUMAN_PEAK_DEFAULT_MAX_RAW / 100.0f, 2);
  j += ",\"waitMinMs\":" + String((unsigned)c.waitMinMs);
  j += ",\"waitMaxMs\":" + String((unsigned)c.waitMaxMs);
  j += ",\"refractoryMinMs\":" + String((unsigned)c.refractoryMinMs);
  j += ",\"refractoryMaxMs\":" + String((unsigned)c.refractoryMaxMs);
  j += ",\"phase\":\"" + String(nagHumanPhaseNamePure(state.phase)) + "\"";
  j += ",\"eventType\":\"" + String(nagHumanEventTypeNamePure(state.event.type)) + "\"";
  j += ",\"activeEventPeakNm\":" + String((float)state.event.peakRaw / 100.0f, 2);
  j += ",\"eventCount\":" + String((unsigned long)state.eventCount);
  j += "}";
  return j;
}

static bool httpParseHumanPeakNm(const String &arg, uint16_t &rawOut) {
  String text = arg;
  text.trim();
  if (text.length() == 0) return false;
  char *end = nullptr;
  const double nm = strtod(text.c_str(), &end);
  if (!end || *end != '\0' || nm != nm || nm < 0.0 || nm > 100.0) return false;
  const uint32_t rounded = (uint32_t)(nm * 100.0 + 0.5);
  if (rounded > 0xFFFFu) return false;
  rawOut = (uint16_t)rounded;
  return true;
}

static void httpNagHumanLabStats() {
  if (!httpRequireLab()) return;
  if (!activeProfileNagSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }
  server.send(200, "application/json", nagHumanLabStatsToJson());
}

static void httpNagHumanLabUpdate() {
  if (!httpRequireLab()) return;
  if (!activeProfileNagSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }
  if (!server.hasArg("minNm") || !server.hasArg("maxNm")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"minNm and maxNm required\"}");
    return;
  }
  uint16_t minRaw = 0, maxRaw = 0;
  if (!httpParseHumanPeakNm(server.arg("minNm"), minRaw) ||
      !httpParseHumanPeakNm(server.arg("maxNm"), maxRaw) ||
      !nagHumanPeakRangeValidPure(minRaw, maxRaw)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"range must be 1.00..3.00 Nm and min <= max\"}");
    return;
  }
  if (!nagHumanRuntimeSetPeakRange(minRaw, maxRaw)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid peak range\"}");
    return;
  }
  nagCfgSave();
  server.send(200, "application/json", nagHumanLabStatsToJson());
}

static void httpNagHumanLabReset() {
  if (!httpRequireLab()) return;
  if (!activeProfileNagSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"NAG unavailable for current topology\"}");
    return;
  }
  nagHumanRuntimeResetPeakRange();
  nagCfgSave();
  server.send(200, "application/json", nagHumanLabStatsToJson());
}

static void httpR79LabStats() { server.send(200, "application/json", r79LabStatsToJson()); }
static void httpR79LabUpdate() {
  if (!httpRequireLab()) return;
  uint8_t smart; uint16_t period;
  portENTER_CRITICAL(&r79LabMux); smart=r79LabSmartMode; period=r79LabPeriodMs; portEXIT_CRITICAL(&r79LabMux);

  // Cached older dashboards may still send apply/hard/park. Fixed policy values
  // remain accepted for compatibility; park is deliberately ignored because
  // fresh Park is now an always-on production R79 gate source.
  if (server.hasArg("apply") && server.arg("apply").toInt() != R79LAB_FORCE_0) {
    server.send(409,"application/json","{\"ok\":false,\"error\":\"bit19-policy-fixed-force0\"}"); return;
  }
  if (server.hasArg("hard") && server.arg("hard").toInt() != R79LAB_FORCE_1) {
    server.send(409,"application/json","{\"ok\":false,\"error\":\"bit47-policy-fixed-force1\"}"); return;
  }
  if (server.hasArg("smart")) { int v=server.arg("smart").toInt(); if(v<0||v>2){server.send(400,"text/plain","invalid smart");return;} smart=(uint8_t)v; }
  if (server.hasArg("period")) { int v=server.arg("period").toInt(); if(v<0||v>65535||!r79LabPeriodValid((uint16_t)v)){server.send(400,"text/plain","invalid period");return;} period=(uint16_t)v; }
  if (server.hasArg("park")) { int v=server.arg("park").toInt(); if(v!=0&&v!=1){server.send(400,"text/plain","invalid park");return;} }
portENTER_CRITICAL(&r79LabMux);
  r79LabSmartMode=smart; r79LabPeriodMs=period;
  portEXIT_CRITICAL(&r79LabMux);
  r79LabCfgSave();
  server.send(200,"application/json",r79LabStatsToJson());
}

static void httpR79LabStock() {
  if (!httpRequireLab()) return;
  portENTER_CRITICAL(&r79LabMux);
  r79LabSmartMode=R79LAB_STOCK;
  portEXIT_CRITICAL(&r79LabMux);
  r79LabCfgSave();
  server.send(200,"application/json",r79LabStatsToJson());
}

static void httpLab3f8Stats() {
  server.send(200, "application/json", lab3f8StatsToJson());
}

static void httpLab3f8Update() {
  if (!labMenuEnabled && (server.hasArg("ulcbs") || server.hasArg("acc"))) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"LAB disabled\"}");
    return;
  }
  uint8_t alc, blind, acc;
  portENTER_CRITICAL(&lab3f8Mux);
  alc = lab3f8AlcMode;
  blind = lab3f8UlcBlindMode;
  acc = lab3f8AccFollowRaw;
  portEXIT_CRITICAL(&lab3f8Mux);

  if (server.hasArg("alc")) {
    int v = server.arg("alc").toInt();
    if (v != LAB3F8_ALC_STOCK && v != LAB3F8_ALC_FORCE_ON) { server.send(400, "text/plain", "invalid alc"); return; }
    alc = (uint8_t)v;
  }
  if (server.hasArg("ulcbs")) {
    String a = server.arg("ulcbs");
    if (a == "stock") blind = LAB3F8_STOCK;
    else {
      int v = a.toInt();
      if (v < 0 || v > 2) { server.send(400, "text/plain", "invalid ulcbs"); return; }
      blind = (uint8_t)v;
    }
  }
  if (server.hasArg("acc")) {
    String a = server.arg("acc");
    if (a == "stock") acc = LAB3F8_STOCK;
    else {
      int v = a.toInt();
      if (v < 0 || v > 6) { server.send(400, "text/plain", "invalid acc"); return; }
      acc = (uint8_t)v;
    }
  }
  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8AlcMode = alc;
  lab3f8UlcBlindMode = blind;
  lab3f8AccFollowRaw = acc;
  portEXIT_CRITICAL(&lab3f8Mux);
  lab3f8CfgSave();
  server.send(200, "application/json", lab3f8StatsToJson());
}

static void httpLab3f8Stock() {
  if (!httpRequireLab()) return;
  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8AlcMode = LAB3F8_ALC_STOCK;
  lab3f8UlcBlindMode = LAB3F8_STOCK;
  lab3f8AccFollowRaw = LAB3F8_STOCK;
  portEXIT_CRITICAL(&lab3f8Mux);
  lab3f8CfgSave();
  server.send(200, "application/json", lab3f8StatsToJson());
}


static void httpBlinkAStats() {
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkAEnable() {
  if (!activeProfileAdvancedEapSupported()) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Advanced EAP unavailable for current topology\"}");
    return;
  }
  portENTER_CRITICAL(&blinkAMux);
  blinkAEnabled = true;
  portEXIT_CRITICAL(&blinkAMux);
  evaluateAutoBlinker();
  summonCfgSave();
  server.send(200, "application/json", blinkAStatsToJson());
}

static void httpBlinkADisable() {
  portENTER_CRITICAL(&blinkAMux);
  blinkAEnabled = false;
  autoBlinkerClearPendingLocked();
  oneShotTurn = STALK_IDLE;
  oneShotUntil = 0;
  oneShotReleaseAt = 0;
  activeTurn = STALK_IDLE;
  lastReqDir = 0;
  autoRequestLastSeenMs = 0;
  portEXIT_CRITICAL(&blinkAMux);
  summonCfgSave();
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


// Adaptive dashboard snapshots. These serializers are display-only consumers:
// they never change CAN state, injection decisions, BLE behavior, or recovery state.
// HOME uses one request at the selected 250/500/1000 ms rate and conditionally
// includes slow/on-demand groups so HTTP requests never overlap just to refresh UI.
static String homeFastSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();

  bool apActive, noaRaw, dasValid, parked, summon, aca, spr, priorityFreshParked, gateGraceActive;
  uint8_t dasState4, priorityState;
  portENTER_CRITICAL(&stateMux);
  apActive = gateAPActive;
  noaRaw = gateNOAActive;
  dasValid = dasAutopilotStateValid;
  dasState4 = dasAutopilotState4;
  parked = gateParked;
  summon = gateSummoning;
  aca = lastAca;
  spr = sprSeen;
  priorityState = summonPriorityState;
  priorityFreshParked = summonPriorityFreshParkedLocked(now);
  gateGraceActive = summonGateGraceActiveLocked(now);
  portEXIT_CRITICAL(&stateMux);
  const bool summonGate = parked || summon || gateGraceActive;

  NagContext nagHomeCtx;
  portENTER_CRITICAL(&nagCtxMux); nagHomeCtx = nagCtx; portEXIT_CRITICAL(&nagCtxMux);
  bool nagPauseZero;
  uint8_t nagModeHome;
  portENTER_CRITICAL(&nagCfgMux);
  nagPauseZero = nagCfg.pauseAtZeroSpeed;
  nagModeHome = nagCfg.mode;
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t nagSpeedAge = nagHomeCtx.lastVehicleSpeedMs ? (uint32_t)(now - nagHomeCtx.lastVehicleSpeedMs) : 999999UL;
  const bool nagSpeedFresh = nagHomeCtx.vehicleSpeedValid && nagHomeCtx.lastVehicleSpeedMs != 0 && nagSpeedAge <= NAG_SPEED_FRESH_MS;
  bool nagHumanPaused = false;
  if (nagModeHome == MODE_H) {
    const NagHumanStatePure nagHumanHome = nagHumanRuntimeSnapshot();
    nagHumanPaused = nagHumanHome.phase == H_PAUSED_STOPPED;
  }
  const bool nagStoppedGate = nagPauseAtZeroBlocksPure(nagPauseZero, nagHomeCtx.vehicleSpeedValid, nagSpeedFresh, nagHomeCtx.vehicleSpeedRaw) || nagHumanPaused;

  bool blinkEnabled;
  portENTER_CRITICAL(&blinkAMux); blinkEnabled = blinkAEnabled; portEXIT_CRITICAL(&blinkAMux);
  const bool noaActive = dasValid && noaRaw;

  bool r79LastTxValidLocal, r79StockValidLocal;
  uint32_t r79TxOkLocal, r79TxFailLocal, r79LastTxMsLocal;
  portENTER_CRITICAL(&r79LabMux);
  r79LastTxValidLocal = r79LabLastTxValid;
  r79StockValidLocal = r79LabStockValid;
  r79TxOkLocal = r79LabTxOk;
  r79TxFailLocal = r79LabTxFail;
  r79LastTxMsLocal = r79LabLastTxMs;
  portEXIT_CRITICAL(&r79LabMux);
  const uint8_t r79GateReasonLocal = r79LabGateReason(now);
  const bool r79GateOpenLocal = r79GateReasonLocal != R79LAB_GATE_BLOCKED;
  const bool r79InjectionActiveLocal = r79GateOpenLocal && r79StockValidLocal;

  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  twai_status_info_t twaiHome = {};
  const bool twaiHomeOk = twai_get_status_info(&twaiHome) == ESP_OK;

  String j; j.reserve(1040);
  j = "{\"nag\":{";
  j += "\"torque\":" + String(nagRealTorque, 2);
  j += ",\"stoppedGate\":" + String(nagStoppedGate ? "true" : "false");
  j += ",\"apActive\":" + String((dasValid && apActive) ? "true" : "false");
  j += ",\"canAState\":" + String((int)mcpState);
  j += "},\"blink\":{";
  j += "\"enabled\":" + String(blinkEnabled ? "true" : "false");
  j += ",\"noaActive\":" + String(noaActive ? "true" : "false");
  j += ",\"dasStateValid\":" + String(dasValid ? "true" : "false");
  j += ",\"dasState\":" + String((unsigned)dasState4);
  j += "},\"summon\":{";
  j += "\"priorityStateName\":\"" + String(summonPriorityStateName(priorityState)) + "\"";
  j += ",\"gate\":" + String(summonGate ? "true" : "false");
  j += ",\"gateGraceActive\":" + String(gateGraceActive ? "true" : "false");
  j += ",\"priorityFreshParked\":" + String(priorityFreshParked ? "true" : "false");
  j += ",\"parked\":" + String(parked ? "true" : "false");
  j += ",\"aca\":" + String(aca ? "true" : "false");
  j += ",\"spr\":" + String(spr ? "true" : "false");
  j += ",\"txQueueNow\":" + String((unsigned long)twaiTxQueueNow);
  j += ",\"txQueueMax\":" + String((unsigned long)twaiTxQueueMax);
  j += ",\"canState\":" + String(twaiHomeOk ? (int)twaiHome.state : -1);
  j += ",\"canStateName\":\"" + String(twaiHomeOk ? twaiStateName(twaiHome.state) : "UNAVAILABLE") + "\"";
  j += "},\"cantraffic\":{";
  j += "\"mcpTrafficSeen\":" + String(traffic.mcpSeen ? "true" : "false");
  j += ",\"mcpTrafficOnline\":" + String(traffic.mcpOnline ? "true" : "false");
  j += ",\"mcpTrafficAgeMs\":" + String((unsigned long)traffic.mcpAgeMs);
  j += ",\"twaiTrafficSeen\":" + String(traffic.twaiSeen ? "true" : "false");
  j += ",\"twaiTrafficOnline\":" + String(traffic.twaiOnline ? "true" : "false");
  j += ",\"twaiTrafficAgeMs\":" + String((unsigned long)traffic.twaiAgeMs);
  j += "},\"r79\":{";
  j += "\"gateOpen\":" + String(r79GateOpenLocal ? "true" : "false");
  j += ",\"injectionActive\":" + String(r79InjectionActiveLocal ? "true" : "false");
  j += ",\"gateReason\":\"" + String(r79LabGateReasonName(r79GateReasonLocal)) + "\"";
  j += ",\"txOk\":" + String((unsigned long)r79TxOkLocal);
  j += ",\"txFail\":" + String((unsigned long)r79TxFailLocal);
  j += ",\"lastTxValid\":" + String(r79LastTxValidLocal ? "true" : "false");
  j += ",\"lastTxAgeMs\":" + String((unsigned long)(r79LastTxMsLocal ? now - r79LastTxMsLocal : 999999UL));
  j += "}}";
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
  portENTER_CRITICAL(&r79LabMux); r79Smart = r79LabSmartMode; portEXIT_CRITICAL(&r79LabMux);
  portENTER_CRITICAL(&lab3f8Mux); alcModeLocal = lab3f8AlcMode; portEXIT_CRITICAL(&lab3f8Mux);

  String j; j.reserve(360);
  j = "{\"blink\":{";
  j += "\"enabled\":" + String(blinkEnabled ? "true" : "false");
  j += ",\"delayMs\":" + String((unsigned long)blinkDelayMsLocal);
  j += "},\"summon\":{\"tlssc\":" + String(tlssc ? "true" : "false") + "}";
  j += ",\"s3xy\":{\"bluetoothEnabled\":" + String(btEnabled ? "true" : "false");
  j += ",\"pairedCount\":" + String((unsigned)s3Paired);
  j += ",\"connectedCount\":" + String((unsigned)s3Connected) + "}";
  j += ",\"r79\":{\"smartMode\":" + String((unsigned)r79Smart) + "}";
  j += ",\"lab3f8\":{\"alcMode\":" + String((unsigned)alcModeLocal) + "}}";
  return j;
}

static String homeLiveSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();
  uint32_t nagTxOkLocal, nagTxFailLocal, nagLastTxLocal, nagMaxGapLocal;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOkLocal = nagTxOk; nagTxFailLocal = nagTxFail;
  nagLastTxLocal = nagLastTxOkMs; nagMaxGapLocal = nagMaxTxGapMs;
  portEXIT_CRITICAL(&nagDiagMux);

  uint32_t blinkRx249Local;
  portENTER_CRITICAL(&blinkAMux); blinkRx249Local = rx249; portEXIT_CRITICAL(&blinkAMux);
  uint32_t sumTxOkLocal, sumTxFailLocal;
  portENTER_CRITICAL(&stateMux); sumTxOkLocal = sumTxOk; sumTxFailLocal = sumTxFail; portEXIT_CRITICAL(&stateMux);
  uint32_t ulcTxOkLocal, ulcTxFailLocal;
  portENTER_CRITICAL(&ulcSnoozeMux); ulcTxOkLocal = ulcSnoozeTxOk; ulcTxFailLocal = ulcSnoozeTxFail; portEXIT_CRITICAL(&ulcSnoozeMux);

  String j; j.reserve(430);
  j = "{\"nag\":{";
  j += "\"ho\":" + String((unsigned)nagRealHo);
  j += ",\"injNm\":" + String(nagLastInjectedNm, 2);
  j += ",\"injHo\":" + String((unsigned)nagLastInjectedHo);
  j += ",\"rx\":" + String((unsigned long)nagRxFrames);
  j += ",\"txOk\":" + String((unsigned long)nagTxOkLocal);
  j += ",\"txFail\":" + String((unsigned long)nagTxFailLocal);
  j += ",\"lastTxAgeMs\":" + String((unsigned long)(nagLastTxLocal ? now - nagLastTxLocal : 999999UL));
  j += ",\"maxTxGapMs\":" + String((unsigned long)nagMaxGapLocal) + "}";
  j += ",\"blink\":{\"rx249\":" + String((unsigned long)blinkRx249Local) + "}";
  j += ",\"summon\":{\"txOk\":" + String((unsigned long)sumTxOkLocal) + ",\"txFail\":" + String((unsigned long)sumTxFailLocal) + "}";
  j += ",\"s3xy\":{\"ulcTxOk\":" + String((unsigned long)ulcTxOkLocal) + ",\"ulcTxFail\":" + String((unsigned long)ulcTxFailLocal) + "}}";
  return j;
}

static String settingsLiteSnapshotToJson() {
  bool blinkEnabled, tlssc;
  uint32_t blinkDelayMsLocal;
  uint8_t priorityState;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkDelayMsLocal = blinkADelayMs;
  portEXIT_CRITICAL(&blinkAMux);
  portENTER_CRITICAL(&stateMux);
  tlssc = tlsscEnabled;
  priorityState = summonPriorityState;
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
  j = "{\"system\":{\"fwVersion\":\"" + String(FW_VERSION) + "\"}";
  j += ",\"blink\":{\"enabled\":" + String(blinkEnabled ? "true" : "false") + ",\"delayMs\":" + String((unsigned long)blinkDelayMsLocal) + "}";
  j += ",\"summon\":{\"tlssc\":" + String(tlssc ? "true" : "false") + ",\"priorityStateName\":\"" + String(summonPriorityStateName(priorityState)) + "\"}";
  j += ",\"s3xy\":{\"bluetoothEnabled\":" + String(btEnabled ? "true" : "false") + ",\"autoEnabled\":" + String(autoEnabled ? "true" : "false") + ",\"pairedCount\":" + String((unsigned)paired) + ",\"connectedCount\":" + String((unsigned)connected) + "}";
  j += ",\"lab3f8\":{\"alcMode\":" + String((unsigned)alcModeLocal) + "}}";
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
  const uint8_t gateReason = r79LabGateReason(now);

  ResearchCaptureLatest lane239 = {};
  ResearchCaptureLatest alc399 = {};
  portENTER_CRITICAL(&researchCaptureMux);
  if (researchCaptureLatest) {
    lane239 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x239)];
    alc399 = researchCaptureLatest[researchCaptureStateIndex(RESEARCH_CAPTURE_BUS_PARTY, 0x399)];
  }
  portEXIT_CRITICAL(&researchCaptureMux);

  const bool laneValid = lane239.valid && lane239.dlc >= 7;
  const bool alcValid = alc399.valid && alc399.dlc >= 7;
  const DasLane239Decoded laneDecoded = dasLane239DecodePure(lane239.data, lane239.dlc);
  const uint8_t alcRaw = alcValid ? das399ReadAlcPure(alc399.data, alc399.dlc) : 0xFF;
  const uint32_t laneAge = lane239.valid ? (uint32_t)(now - lane239.lastSeenMs) : 999999UL;
  const uint32_t alcAge = alc399.valid ? (uint32_t)(now - alc399.lastSeenMs) : 999999UL;

  String j; j.reserve(520);
  j = "{\"r79\":{";
  j += "\"gateOpen\":" + String(gateReason != R79LAB_GATE_BLOCKED ? "true" : "false");
  j += ",\"gateReason\":\"" + String(r79LabGateReasonName(gateReason)) + "\"";
  j += ",\"dasState\":" + String((unsigned)dasState4);
  j += ",\"dasStateValid\":" + String(dasStateValid ? "true" : "false");
  j += ",\"dasAgeMs\":" + String((unsigned long)(dasLast ? now - dasLast : 999999UL));
  j += "},\"alc\":{";
  j += "\"alcValid\":" + String(alcValid ? "true" : "false");
  j += ",\"alcRaw\":" + String((unsigned)alcRaw);
  j += ",\"alcAgeMs\":" + String((unsigned long)alcAge);
  j += ",\"lane239Valid\":" + String(laneDecoded.valid ? "true" : "false");
  j += ",\"lane239AgeMs\":" + String((unsigned long)laneAge);
  j += ",\"leftLaneExists\":" + String((unsigned)(laneDecoded.valid ? (laneDecoded.leftLaneExists ? 1 : 0) : 255));
  j += ",\"rightLaneExists\":" + String((unsigned)(laneDecoded.valid ? (laneDecoded.rightLaneExists ? 1 : 0) : 255));
  j += ",\"leftLineUsageRaw\":" + String((unsigned)(laneDecoded.valid ? laneDecoded.leftLineUsage : 255));
  j += ",\"rightLineUsageRaw\":" + String((unsigned)(laneDecoded.valid ? laneDecoded.rightLineUsage : 255));
  j += ",\"leftForkRaw\":" + String((unsigned)(laneDecoded.valid ? laneDecoded.leftFork : 255));
  j += ",\"rightForkRaw\":" + String((unsigned)(laneDecoded.valid ? laneDecoded.rightFork : 255));
  j += "}}";
  return j;
}

// Lightweight HOME snapshot. The HOME page is polled at 250/500/1000 ms,
// so avoid building the full diagnostics payloads that are only needed inside
// feature/detail panels. This keeps visible HOME behavior unchanged while
// reducing transient String allocation, JSON serialization and Wi-Fi payload.
static String homeSnapshotToJson() {
  const uint32_t now = (uint32_t)millis();

  // Shared vehicle/AP/Summon state in one short stateMux snapshot.
  bool apActive, noaRaw, dasValid, parked, summon, aca, spr, priorityFreshParked, gateGraceActive, tlssc;
  uint8_t dasState4, priorityState;
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
  priorityState = summonPriorityState;
  priorityFreshParked = summonPriorityFreshParkedLocked(now);
  gateGraceActive = summonGateGraceActiveLocked(now);
  tlssc = tlsscEnabled;
  sumTxOkLocal = sumTxOk;
  sumTxFailLocal = sumTxFail;
  portEXIT_CRITICAL(&stateMux);
  const bool summonGate = parked || summon || gateGraceActive;

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
  portENTER_CRITICAL(&nagCfgMux);
  nagPauseZero = nagCfg.pauseAtZeroSpeed;
  nagModeHome = nagCfg.mode;
  portEXIT_CRITICAL(&nagCfgMux);
  const uint32_t nagSpeedAge = nagHomeCtx.lastVehicleSpeedMs ? (uint32_t)(now - nagHomeCtx.lastVehicleSpeedMs) : 999999UL;
  const bool nagSpeedFresh = nagHomeCtx.vehicleSpeedValid && nagHomeCtx.lastVehicleSpeedMs != 0 && nagSpeedAge <= NAG_SPEED_FRESH_MS;
  bool nagHumanPaused = false;
  if (nagModeHome == MODE_H) {
    const NagHumanStatePure nagHumanHome = nagHumanRuntimeSnapshot();
    nagHumanPaused = nagHumanHome.phase == H_PAUSED_STOPPED;
  }
  const bool nagStoppedGate = nagPauseAtZeroBlocksPure(nagPauseZero, nagHomeCtx.vehicleSpeedValid, nagSpeedFresh, nagHomeCtx.vehicleSpeedRaw) || nagHumanPaused;

  // Auto Blinker HOME telemetry only.
  bool blinkEnabled;
  uint32_t blinkDelayMsLocal, blinkRx249Local;
  portENTER_CRITICAL(&blinkAMux);
  blinkEnabled = blinkAEnabled;
  blinkDelayMsLocal = blinkADelayMs;
  blinkRx249Local = rx249;
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
  bool r79LastTxValidLocal, r79StockValidLocal;
  uint32_t r79TxOkLocal, r79TxFailLocal, r79LastTxMsLocal;
  portENTER_CRITICAL(&r79LabMux);
  r79Smart = r79LabSmartMode;
  r79LastTxValidLocal = r79LabLastTxValid;
  r79StockValidLocal = r79LabStockValid;
  r79TxOkLocal = r79LabTxOk;
  r79TxFailLocal = r79LabTxFail;
  r79LastTxMsLocal = r79LabLastTxMs;
  portEXIT_CRITICAL(&r79LabMux);
  const uint8_t r79GateReasonLocal = r79LabGateReason(now);
  const bool r79GateOpenLocal = r79GateReasonLocal != R79LAB_GATE_BLOCKED;
  const bool r79InjectionActiveLocal = r79GateOpenLocal && r79StockValidLocal;

  uint8_t alcModeLocal;
  portENTER_CRITICAL(&lab3f8Mux); alcModeLocal = lab3f8AlcMode; portEXIT_CRITICAL(&lab3f8Mux);

  const CanTrafficUiSnapshot traffic = canTrafficUiSnapshot();
  twai_status_info_t twaiHome = {};
  const bool twaiHomeOk = twai_get_status_info(&twaiHome) == ESP_OK;

  String j;
  j.reserve(1550);
  j = "{\"nag\":{";
  j += "\"torque\":" + String(nagRealTorque, 2);
  j += ",\"ho\":" + String((unsigned)nagRealHo);
  j += ",\"injNm\":" + String(nagLastInjectedNm, 2);
  j += ",\"injHo\":" + String((unsigned)nagLastInjectedHo);
  j += ",\"rx\":" + String((unsigned long)nagRxFrames);
  j += ",\"txOk\":" + String((unsigned long)nagTxOkLocal);
  j += ",\"txFail\":" + String((unsigned long)nagTxFailLocal);
  j += ",\"lastTxAgeMs\":" + String((unsigned long)(nagLastTxLocal ? now - nagLastTxLocal : 999999UL));
  j += ",\"maxTxGapMs\":" + String((unsigned long)nagMaxGapLocal);
  j += ",\"stoppedGate\":" + String(nagStoppedGate ? "true" : "false");
  j += ",\"apActive\":" + String((dasValid && apActive) ? "true" : "false");
  j += ",\"canAState\":" + String((int)mcpState);
  j += "},\"blink\":{";
  j += "\"enabled\":" + String(blinkEnabled ? "true" : "false");
  j += ",\"delayMs\":" + String((unsigned long)blinkDelayMsLocal);
  j += ",\"noaActive\":" + String(noaActive ? "true" : "false");
  j += ",\"dasStateValid\":" + String(dasValid ? "true" : "false");
  j += ",\"dasState\":" + String((unsigned)dasState4);
  j += ",\"rx249\":" + String((unsigned long)blinkRx249Local);
  j += "},\"summon\":{";
  j += "\"tlssc\":" + String(tlssc ? "true" : "false");
  j += ",\"priorityStateName\":\"" + String(summonPriorityStateName(priorityState)) + "\"";
  j += ",\"gate\":" + String(summonGate ? "true" : "false");
  j += ",\"gateGraceActive\":" + String(gateGraceActive ? "true" : "false");
  j += ",\"priorityFreshParked\":" + String(priorityFreshParked ? "true" : "false");
  j += ",\"parked\":" + String(parked ? "true" : "false");
  j += ",\"aca\":" + String(aca ? "true" : "false");
  j += ",\"spr\":" + String(spr ? "true" : "false");
  j += ",\"txQueueNow\":" + String((unsigned long)twaiTxQueueNow);
  j += ",\"txQueueMax\":" + String((unsigned long)twaiTxQueueMax);
  j += ",\"txOk\":" + String((unsigned long)sumTxOkLocal);
  j += ",\"txFail\":" + String((unsigned long)sumTxFailLocal);
  j += ",\"canState\":" + String(twaiHomeOk ? (int)twaiHome.state : -1);
  j += ",\"canStateName\":\"" + String(twaiHomeOk ? twaiStateName(twaiHome.state) : "UNAVAILABLE") + "\"";
  j += "},\"s3xy\":{";
  j += "\"bluetoothEnabled\":" + String(btEnabled ? "true" : "false");
  j += ",\"pairedCount\":" + String((unsigned)s3Paired);
  j += ",\"connectedCount\":" + String((unsigned)s3Connected);
  j += ",\"ulcTxOk\":" + String((unsigned long)ulcTxOkLocal);
  j += ",\"ulcTxFail\":" + String((unsigned long)ulcTxFailLocal);
  j += "},\"cantraffic\":{";
  j += "\"mcpTrafficSeen\":" + String(traffic.mcpSeen ? "true" : "false");
  j += ",\"mcpTrafficOnline\":" + String(traffic.mcpOnline ? "true" : "false");
  j += ",\"mcpTrafficAgeMs\":" + String((unsigned long)traffic.mcpAgeMs);
  j += ",\"twaiTrafficSeen\":" + String(traffic.twaiSeen ? "true" : "false");
  j += ",\"twaiTrafficOnline\":" + String(traffic.twaiOnline ? "true" : "false");
  j += ",\"twaiTrafficAgeMs\":" + String((unsigned long)traffic.twaiAgeMs);
  j += "},\"r79\":{";
  j += "\"smartMode\":" + String((unsigned)r79Smart);
  j += ",\"gateOpen\":" + String(r79GateOpenLocal ? "true" : "false");
  j += ",\"injectionActive\":" + String(r79InjectionActiveLocal ? "true" : "false");
  j += ",\"gateReason\":\"" + String(r79LabGateReasonName(r79GateReasonLocal)) + "\"";
  j += ",\"txOk\":" + String((unsigned long)r79TxOkLocal);
  j += ",\"txFail\":" + String((unsigned long)r79TxFailLocal);
  j += ",\"lastTxValid\":" + String(r79LastTxValidLocal ? "true" : "false");
  j += ",\"lastTxAgeMs\":" + String((unsigned long)(r79LastTxMsLocal ? now - r79LastTxMsLocal : 999999UL));
  j += "},\"lab3f8\":{\"alcMode\":" + String((unsigned)alcModeLocal) + "}}";
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

static void httpSnapshot() {
  String groups;
  groups.reserve(80);
  groups = server.hasArg("groups") ? server.arg("groups") : "home";
  if (groups == "home-fast") {
    const bool includeSlow = server.hasArg("slow") && server.arg("slow") == "1";
    const bool includeLive = server.hasArg("live") && server.arg("live") == "1";
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    server.sendContent("{\"fast\":");
    server.sendContent(homeFastSnapshotToJson());
    if (includeSlow) { server.sendContent(",\"slow\":"); server.sendContent(homeSlowSnapshotToJson()); }
    if (includeLive) { server.sendContent(",\"live\":"); server.sendContent(homeLiveSnapshotToJson()); }
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
  if (snapshotGroupRequested(groups,"nag"))          snapshotSendGroup(first,"nag",nagStatsToJson());
  if (snapshotGroupRequested(groups,"blink"))        snapshotSendGroup(first,"blink",blinkAStatsToJson());
  if (snapshotGroupRequested(groups,"das"))          snapshotSendGroup(first,"das",dasTelemetryStatsToJson());
  if (snapshotGroupRequested(groups,"summon"))       snapshotSendGroup(first,"summon",summonStatsToJson());
  if (snapshotGroupRequested(groups,"s3xy"))         snapshotSendGroup(first,"s3xy",s3xyStatsToJson());
  if (snapshotGroupRequested(groups,"system"))       snapshotSendGroup(first,"system",systemStatsToJson());
  if (snapshotGroupRequested(groups,"cantraffic"))   snapshotSendGroup(first,"cantraffic",canTrafficStatsToJson());
  if (snapshotGroupRequested(groups,"lab3f8"))       snapshotSendGroup(first,"lab3f8",lab3f8StatsToJson());
  if (snapshotGroupRequested(groups,"r79"))          snapshotSendGroup(first,"r79",r79LabStatsToJson());
  if (snapshotGroupRequested(groups,"researchcapture"))   snapshotSendGroup(first,"researchcapture",researchCaptureStatsToJson());
  server.sendContent("}");
  server.sendContent("");
}

// Reset only diagnostic/session counters. No NVS/configuration, live feature state,
// CAN liveness timestamps, or mcpRxCount warmup state is modified.
static void resetRuntimeStats() {
  nagRxFrames = 0;
  nagEchoCount = 0;
  mcpTxOk = 0;
  mcpTxFail = 0;
  mcpRxOverflowReset();
  nagEchoLatUs = 0;
  portENTER_CRITICAL(&nagDiagMux);
  nagTxOk=nagTxFail=0; nagSkipDisabled=nagSkipBootDelay=nagSkipWarmup=nagSkipSelfFrame=0;
  nagSkipHandsOn=nagSkipApInvalid=nagSkipApInactive=nagSkipDecision=nagSkipStopped=nagSkipCadence=nagSkipSpeedStale=0;
  nagBlockMutex=nagBlockMcpNotReady=nagBlockEpoch=nagBlockFreshMask=nagBlockInvalidMsg=nagSendError=0;
  nagLastTxOkMs=nagMaxTxGapMs=nagSessionTxOk=0; nagSessionStartMs=0;
  nagLastSkipReason=NAG_SKIP_NONE; nagLastSkipMs=0; nagLastTxBlockReason=MCP_TX_OK; nagLastTxBlockMs=0;
  portEXIT_CRITICAL(&nagDiagMux);

  portENTER_CRITICAL(&stateMux);
  sumRxMux1 = 0;
  sumTxOk = 0;
  sumTxFail = 0;
  sumRx280 = 0;
  sumRx390 = 0;
  sumRx921 = 0;
  sumRx1016 = 0;
  summonPriorityTransitions = 0;
  summonPriorityFullEnterCount = 0;
  summonPriorityFullExitCount = 0;
  summonGateGraceEnterCount = 0;
  summonGateGraceRecoverCount = 0;
  summonGateGraceExpireCount = 0;
  summonAuthorizationTransitions = 0;
  summonConfirmedGearTransitions = 0;
  portEXIT_CRITICAL(&stateMux);

  portENTER_CRITICAL(&r79LabMux);
  r79LabPendingRequestCount = 0;
  r79LabPendingCoalesceCount = 0;
  r79LabPendingRetryCount = 0;
  r79LabFreshMaskWaitCount = 0;
  r79LabLastReassertLatencyMs = 0;
  r79LabMaxReassertLatencyMs = 0;
  r79LabNoTemplateSkip = 0;
  r79LabQueueSkip = 0;
  r79LabGateBlocked = 0;
  r79LabTxOk = 0;
  r79LabTxFail = 0;
  r79LabImmediateTxOk = 0;
  r79LabImmediateTxFail = 0;
  r79LabPeriodicTxOk = 0;
  r79LabPeriodicTxFail = 0;
  r79LabAppliedFrames = 0;
  r79LabLastBlockReason = R79LAB_BLOCK_NONE;
  r79LabLastBlockMs = 0;
  portEXIT_CRITICAL(&r79LabMux);

  portENTER_CRITICAL(&blinkAMux);
  rx249 = 0;
  blkATxOk = 0;
  blkATxFail = 0;
  autoRetryCount = 0;
  portEXIT_CRITICAL(&blinkAMux);
  visualDebugRxCount = 0;

  portENTER_CRITICAL(&lab3f8Mux);
  lab3f8TxOk = 0;
  lab3f8TxFail = 0;
  lab3f8GateBlocked = 0;
  portEXIT_CRITICAL(&lab3f8Mux);


  portENTER_CRITICAL(&roadContextMux);
  tlsscHighwayGateBlockedCount = 0;
  tlsscHighwayTransitions = 0;
  portEXIT_CRITICAL(&roadContextMux);

  twaiReadQueueStatus();
  twaiTxQueueMax = twaiTxQueueNow;
  twaiRxQueueMax = twaiRxQueueNow;
  twaiNonSummonShed = 0;
  twaiStandbyShed = 0;
  twaiFullShed = 0;
  twaiSummonQueueFlush = 0;
  twaiSummonRetryOk = 0;
  twaiSummonRetryFail = 0;
  twaiSummonTxNormal = 0;
  twaiSummonTxStandby = 0;
  twaiSummonTxFull = 0;

  portENTER_CRITICAL(&canRecoveryMux);
  canHardReinitCount = 0;
  canHardReinitFailCount = 0;
  canRecoverySleepCount = 0;
  canRecoveryWakeCount = 0;
  canLastHardReinitReason = CAN_SUP_NONE;
  canPendingHardDiagReason = CAN_REC_NONE;
  canLastHardDiagReason = CAN_REC_NONE;
  canTwaiBusOffCount = 0;
  canTwaiStoppedCount = 0;
  canTwaiLocalRecoveryStartCount = 0;
  canTwaiRecoveryStartFailCount = 0;
  canTwaiRestartOkCount = 0;
  canTwaiRestartFailCount = 0;
  canTwaiLastEventReason = CAN_REC_NONE;
  canTwaiLastEventMs = 0;
  canBLastRxGapMs = 0;
  canBMaxRxGapMs = 0;
  canTwaiLastBusOffSnapshot = {};
  portEXIT_CRITICAL(&canRecoveryMux);
  canBTraceReset();

  runtimeStatsResetCount++;
  runtimeStatsLastResetMs = (uint32_t)millis();
}

static void httpResetRuntimeStats() {
  resetRuntimeStats();
  server.send(200, "application/json", "{\"ok\":true,\"action\":\"runtime-stats-reset\"}");
}

static void webTask(void *arg) {
  Serial.println("WiFi: Starting AP...");
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  String ssid, password;
  wifiApLoadConfig(ssid, password);
  while (!wifiApActivate(ssid, password, true)) {
    Serial.println("WiFi: Failed to start AP, retrying...");
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  IPAddress ip = WiFi.softAPIP();
  bootCaptureMarkOnce(&bootCapWifiReadyMs);
  Serial.printf("AP: SSID=%s IP=%s\n", wifiApActiveSsid.c_str(), ip.toString().c_str());

  server.on("/", HTTP_GET, httpRoot);
  server.on("/api/profile/status", HTTP_GET, httpProfileStatus);
  server.on("/api/profile/select", HTTP_POST, httpProfileSelect);
  server.on("/api/system/factory-reset", HTTP_POST, httpFactoryReset);
  server.on("/api/system/reboot", HTTP_POST, httpRebootT2Can);
  server.on("/api/wifi/status", HTTP_GET, httpWifiStatus);
  server.on("/api/wifi/apply", HTTP_POST, httpWifiApply);
  server.on("/update", HTTP_POST, httpOtaFinish, httpOtaUpload);

  if (!vehicleProfileSetupMode && !vehicleProfileNvsError) {
    server.on("/api/snapshot", HTTP_GET, httpSnapshot);
    if (activeProfileNagSupported()) {
      server.on("/api/nag/config", HTTP_GET, httpNagConfig);
      server.on("/api/nag/stats", HTTP_GET, httpNagStats);
      server.on("/api/nag/mode", HTTP_POST, httpNagSetMode);
      server.on("/api/nag/update", HTTP_POST, httpNagUpdate);
      server.on("/api/nag/reset", HTTP_POST, httpNagReset);
    }
    server.on("/api/summon/stats", HTTP_GET, httpSummonStats);
    server.on("/api/summon/tlssc-enable", HTTP_POST, httpSummonTlsscEnable);
    server.on("/api/summon/tlssc-disable", HTTP_POST, httpSummonTlsscDisable);
    server.on("/api/summon/tlssc-highway-gate", HTTP_POST, httpSummonTlsscHighwayGate);
    server.on("/api/blinkA/stats", HTTP_GET, httpBlinkAStats);
    server.on("/api/blinkA/enable", HTTP_POST, httpBlinkAEnable);
    server.on("/api/blinkA/disable", HTTP_POST, httpBlinkADisable);
    server.on("/api/blinkA/delay", HTTP_POST, httpBlinkADelay);
    server.on("/api/features/status", HTTP_GET, httpFeatureStatus);
    server.on("/api/features/lab", HTTP_POST, httpFeatureLab);
    server.on("/api/features/door-cancel", HTTP_POST, httpFeatureDoorCancel);
    server.on("/api/features/banned", HTTP_POST, httpFeatureBanned);
    server.on("/api/features/tlssc-restore", HTTP_POST, httpFeatureTlsscRestore);
    server.on("/api/features/s3xy", HTTP_POST, httpFeatureS3xy);
    server.on("/api/das/stats", HTTP_GET, httpDasTelemetryStats);
    server.on("/api/r79lab/stats", HTTP_GET, httpR79LabStats);
    server.on("/api/r79lab/update", HTTP_POST, httpR79LabUpdate);
    server.on("/api/r79lab/stock", HTTP_POST, httpR79LabStock);
    server.on("/api/nag-human-lab/stats", HTTP_GET, httpNagHumanLabStats);
    server.on("/api/nag-human-lab/update", HTTP_POST, httpNagHumanLabUpdate);
    server.on("/api/nag-human-lab/reset", HTTP_POST, httpNagHumanLabReset);
    server.on("/api/lab3f8/stats", HTTP_GET, httpLab3f8Stats);
    server.on("/api/lab3f8/update", HTTP_POST, httpLab3f8Update);
    server.on("/api/lab3f8/stock", HTTP_POST, httpLab3f8Stock);
    server.on("/api/researchcapture/stats", HTTP_GET, httpResearchCaptureStats);
    server.on("/api/researchcapture/start", HTTP_POST, httpResearchCaptureStart);
    server.on("/api/researchcapture/labels", HTTP_POST, httpResearchCaptureLabels);
    server.on("/api/researchcapture/config", HTTP_POST, httpResearchCaptureConfig);
    server.on("/api/researchcapture/mode", HTTP_POST, httpResearchCaptureMode);
    server.on("/api/researchcapture/reset", HTTP_POST, httpResearchCaptureReset);
    server.on("/api/researchcapture/log.csv", HTTP_GET, httpResearchCaptureCsv);
    server.on("/api/pedalmap/stats", HTTP_GET, httpPedalMapStats);
    server.on("/api/system/stats", HTTP_GET, httpSystemStats);
    server.on("/api/system/boot-capture.csv", HTTP_GET, httpBootCaptureCsv);
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
