#pragma once

static void canPerfTimingJson(JsonWriterArduino &jw, const char *key,
                              const CanPerfTimingPure &t) {
  jw.beginObject(key);
  jw.u32("count", t.count);
  char total[24];
  snprintf(total, sizeof(total), "%llu", (unsigned long long)t.totalUs);
  jw.raw("totalUs", total);
  jw.u32("meanUs", t.count ? (uint32_t)(t.totalUs / t.count) : 0);
  jw.u32("maxUs", t.maxUs);
  jw.u32("p95UpperUs", canPerfTimingPercentilePure(t, 95));
  String bins("[");
  for (uint8_t i = 0; i < CAN_PERF_HISTOGRAM_BINS; ++i) {
    if (i) bins += ',';
    bins += (unsigned long)t.histogram[i];
  }
  bins += ']';
  jw.raw("histogram", bins);
  jw.endObject();
}

static void canPerfBusJson(JsonWriterArduino &jw, const char *key, CanPerfBus bus) {
  const CanPerfBusPure s = canPerfSnapshot(bus);
  jw.beginObject(key);
  jw.u32("polls", s.polls);
  jw.u32("rxFrames", s.rxFrames);
  jw.u32("rx0Frames", s.rx0Frames);
  jw.u32("rx1Frames", s.rx1Frames);
  jw.u32("rxConfigSamples", s.rxConfigSamples);
  jw.u32("rx0Ctrl", s.rx0Ctrl);
  jw.u32("rx1Ctrl", s.rx1Ctrl);
  jw.boolean("rxConfigValid", s.rxConfigSamples != 0 &&
      (s.rx0Ctrl & 0x64u) == 0x04u && (s.rx1Ctrl & 0x60u) == 0);
  jw.boolean("rxRolloverEnabled", s.rxConfigSamples != 0 && (s.rx0Ctrl & 0x04u));
  jw.boolean("rxFilterModeEnabled", s.rxConfigSamples != 0 &&
      (s.rx0Ctrl & 0x60u) == 0 && (s.rx1Ctrl & 0x60u) == 0);
  jw.u32("rxErrors", s.rxErrors);
  // Latched overflow events, not an exact number of lost frames.
  jw.u32("rxOverflowEvents", s.rxOverflowEvents);
  jw.u32("rx0OverflowEvents", s.rx0OverflowEvents);
  jw.u32("rx1OverflowEvents", s.rx1OverflowEvents);
  jw.u32("txAttempts", s.txAttempts);
  jw.u32("txQueued", s.txQueued);
  jw.u32("txBusy", s.txBusy);
  jw.u32("txErrors", s.txErrors);
  jw.u32("recoveryAttempts", s.recoveryAttempts);
  jw.u32("recoveries", s.recoveries);
  jw.u32("recoveryErrors", s.recoveryErrors);
  if (bus == CAN_PERF_PARTY) {
    jw.u32("rx370", s.rx370);
    jw.u32("tx370", s.tx370);
    jw.u32("tx370Errors", s.tx370Errors);
  }
  jw.u32("txStatusSamples", s.txStatusSamples);
  jw.u32("txFullSamples", s.txFullSamples);
  jw.u32("txPendingLast", s.txPendingLast);
  jw.u32("txPendingMax", s.txPendingMax);
  canPerfTimingJson(jw, "pollGap", s.pollGap);
  canPerfTimingJson(jw, "rxRead", s.rxRead);
  canPerfTimingJson(jw, "rxDispatch", s.rxDispatch);
  canPerfTimingJson(jw, "rxProcess", s.rxProcess);
  canPerfTimingJson(jw, "txCall", s.txCall);
  canPerfTimingJson(jw, "rxToEnqueue", s.rxToEnqueue);
  jw.endObject();
}

static void canPerformanceJson(JsonWriterArduino &jw) {
  jw.beginObject("canPerformance");
  jw.u32("schemaVersion", 1);
#if defined(TMR_PERF_BUILD_O2) && !defined(__OPTIMIZE_SIZE__)
  jw.string("compilerOptimization", "O2");
#elif defined(__OPTIMIZE_SIZE__)
  jw.string("compilerOptimization", "Os");
#else
  jw.string("compilerOptimization", "other");
#endif
  jw.string("latencyOrigin", "software_read_complete");
  jw.string("txSuccessMeaning", "controller_enqueue");
  jw.string("rxLossMeaning", "latched_overflow_events_not_lost_frame_count");
  jw.u32("rxSliceFrames", CAN_RX_SLICE_FRAMES);
  jw.u32("rxSliceUs", CAN_RX_SLICE_US);
  jw.u32("maxBurstUs", CAN_RX_MAX_BURST_US);
  jw.u32("idleRetryUs", CAN_RX_IDLE_RETRY_US);
  jw.u32("rxBuffersPerController", 2);
  jw.u32("rxBuffersTotal", 6);
  jw.u32("rxTaskPriority", CAN_RX_TASK_PRIORITY);
  canPerfBusJson(jw, "body", CAN_PERF_BODY);
  canPerfBusJson(jw, "chassis", CAN_PERF_CHASSIS);
  canPerfBusJson(jw, "party", CAN_PERF_PARTY);
  jw.endObject();
}
