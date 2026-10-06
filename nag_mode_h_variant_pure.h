#pragma once
#include <stdint.h>

// Persisted value 1 is retained for existing Mode H tuning.
enum NagModeHVariantPure : uint8_t { H_VARIANT_REV4 = 1 };

static inline uint8_t nagModeHDefaultVariantPure() {
  return H_VARIANT_REV4;
}


enum NagModeHStopBehaviorPure : uint8_t {
  H_STOP_HARD_PAUSE = 0,
  H_STOP_STOCK_CARRIER = 1
};

static inline uint8_t nagModeHDefaultStopBehaviorPure() {
  return H_STOP_HARD_PAUSE;
}

static inline bool nagModeHStopBehaviorValidPure(uint8_t v) {
  return v == H_STOP_HARD_PAUSE || v == H_STOP_STOCK_CARRIER;
}

static inline const char* nagModeHStopBehaviorNamePure(uint8_t v) {
  return v == H_STOP_HARD_PAUSE ? "HARD_PAUSE" : "STOCK_CARRIER";
}

static inline bool nagModeHUseStopCarrierPure(uint8_t behavior, bool confirmedStopped) {
  return behavior == H_STOP_STOCK_CARRIER && confirmedStopped;
}

static inline bool nagModeHVariantValidPure(uint8_t v) { return v == H_VARIANT_REV4; }

static inline const char* nagModeHVariantCodePure(uint8_t) { return "H"; }

static inline const char* nagModeHVariantLabelPure(uint8_t) { return "Mode H"; }
