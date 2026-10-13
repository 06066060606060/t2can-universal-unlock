#pragma once
#include <stdint.h>

static constexpr uint8_t CAN_PERF_HISTOGRAM_BINS = 11;
struct CanPerfTimingPure {
  uint32_t count;
  uint64_t totalUs;
  uint32_t maxUs;
  // Upper bounds: 8,16,32,...,4096 us; last bucket includes larger values.
  uint32_t histogram[CAN_PERF_HISTOGRAM_BINS];
};

static inline void canPerfTimingObservePure(CanPerfTimingPure &s, uint32_t us) {
  ++s.count;
  s.totalUs += us;
  if (us > s.maxUs) s.maxUs = us;
  uint8_t bin = 0;
  while (bin + 1 < CAN_PERF_HISTOGRAM_BINS && us > (8UL << bin)) ++bin;
  ++s.histogram[bin];
}

static inline uint32_t canPerfTimingPercentilePure(const CanPerfTimingPure &s,
                                                 uint8_t percent) {
  if (!s.count) return 0;
  const uint64_t target = ((uint64_t)s.count * percent + 99) / 100;
  uint64_t seen = 0;
  for (uint8_t i = 0; i < CAN_PERF_HISTOGRAM_BINS; ++i) {
    seen += s.histogram[i];
    if (seen >= target)
      return i + 1 == CAN_PERF_HISTOGRAM_BINS ? s.maxUs : (8UL << i);
  }
  return s.maxUs;
}

static inline uint8_t canPerfPendingPure(uint8_t readStatus) {
  return ((readStatus >> 2) & 1u) + ((readStatus >> 4) & 1u) +
         ((readStatus >> 6) & 1u);
}

struct CanPerfBusPure {
  uint32_t polls;
  uint32_t lastPollUs;
  CanPerfTimingPure pollGap;
  uint32_t rxFrames;
  uint32_t rx0Frames;
  uint32_t rx1Frames;
  uint32_t rxConfigSamples;
  uint32_t rxConfigLastUs;
  uint8_t rx0Ctrl;
  uint8_t rx1Ctrl;
  uint32_t rxErrors;
  uint32_t rxOverflowEvents;
  uint32_t rx0OverflowEvents;
  uint32_t rx1OverflowEvents;
  CanPerfTimingPure rxRead;
  CanPerfTimingPure rxDispatch;
  CanPerfTimingPure rxProcess;
  uint32_t txAttempts;
  uint32_t txQueued;
  uint32_t txBusy;
  uint32_t txErrors;
  uint32_t recoveryAttempts;
  uint32_t recoveries;
  uint32_t recoveryErrors;
  uint32_t rx370;
  uint32_t tx370;
  uint32_t tx370Errors;
  CanPerfTimingPure txCall;
  CanPerfTimingPure rxToEnqueue;
  uint32_t txStatusSamples;
  uint32_t txFullSamples;
  uint8_t txPendingLast;
  uint8_t txPendingMax;
};

static inline void canPerfPollObservePure(CanPerfBusPure &s, uint32_t nowUs) {
  if (s.polls) canPerfTimingObservePure(s.pollGap, nowUs - s.lastPollUs);
  ++s.polls;
  s.lastPollUs = nowUs;
}

static inline void canPerfOverflowObservePure(CanPerfBusPure &s, uint8_t flags) {
  if (!(flags & 0xC0u)) return;
  ++s.rxOverflowEvents;
  if (flags & 0x40u) ++s.rx0OverflowEvents;
  if (flags & 0x80u) ++s.rx1OverflowEvents;
}

static inline void canPerfStatusObservePure(CanPerfBusPure &s, uint8_t status) {
  ++s.txStatusSamples;
  s.txPendingLast = canPerfPendingPure(status);
  if (s.txPendingLast > s.txPendingMax) s.txPendingMax = s.txPendingLast;
  if (s.txPendingLast == 3) ++s.txFullSamples;
}
