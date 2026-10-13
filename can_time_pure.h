#pragma once
#include <stdint.h>

// A timestamp published on the other core can be newer than a reader's
// sampled clock. Clamp only small future skews, retaining unsigned wraparound
// and genuinely old timestamps (including ages above INT32_MAX).
static inline uint32_t canAgeMsPure(uint32_t now, uint32_t timestamp) {
  const uint32_t elapsed = now - timestamp;
  return elapsed > UINT32_MAX - 1000u ? 0u : elapsed;
}

static inline bool canHeartbeatExpiredPure(uint32_t now, uint32_t heartbeat,
                                           uint32_t timeoutMs) {
  return heartbeat == 0 || canAgeMsPure(now, heartbeat) > timeoutMs;
}
