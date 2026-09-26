#pragma once
#include <stdint.h>

enum NagModeHVariantPure : uint8_t {
  H_VARIANT_REV1 = 0,
  H_VARIANT_REV4 = 1,
  H_VARIANT_REV2 = 2,
  H_VARIANT_REV3 = 3
};


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

static inline bool nagModeHVariantValidPure(uint8_t v) {
  return v == H_VARIANT_REV1 || v == H_VARIANT_REV4 ||
         v == H_VARIANT_REV2 || v == H_VARIANT_REV3;
}

static inline const char* nagModeHVariantCodePure(uint8_t v) {
  switch (v) {
    case H_VARIANT_REV1: return "R1";
    case H_VARIANT_REV4: return "R4";
    case H_VARIANT_REV2: return "R2";
    case H_VARIANT_REV3: return "R3";
    default: return "R3";
  }
}

static inline const char* nagModeHVariantLabelPure(uint8_t v) {
  switch (v) {
    case H_VARIANT_REV1: return "Rev.1";
    case H_VARIANT_REV4: return "Rev.4";
    case H_VARIANT_REV2: return "Rev.2";
    case H_VARIANT_REV3: return "Rev.3";
    default: return "Rev.3";
  }
}
