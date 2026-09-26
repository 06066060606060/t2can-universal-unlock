#pragma once
#include <stdint.h>

// Volatile drive-session PedalMap ownership. This is deliberately pure and
// contains no persistence/NVS concept: firmware boot starts from STOCK, and a
// positively observed Park clears the session.
static constexpr uint8_t PEDAL_MAP_RAW_CHILL = 0;
static constexpr uint8_t PEDAL_MAP_RAW_SPORT = 1;
static constexpr uint8_t PEDAL_MAP_RAW_PERFORMANCE = 2;
static constexpr uint8_t PEDAL_MAP_RAW_STOCK = 0xFF;

struct PedalMapSessionPure {
  bool active;
  uint8_t originRaw;
  uint8_t targetRaw;
};

static inline PedalMapSessionPure pedalMapSessionInitialPure() {
  return {false, PEDAL_MAP_RAW_STOCK, PEDAL_MAP_RAW_STOCK};
}

static inline bool pedalMapRawSelectablePure(uint8_t raw) {
  return raw == PEDAL_MAP_RAW_STOCK || raw <= PEDAL_MAP_RAW_PERFORMANCE;
}

static inline void pedalMapSessionClearPure(PedalMapSessionPure &s) {
  s.active = false;
  s.originRaw = PEDAL_MAP_RAW_STOCK;
  s.targetRaw = PEDAL_MAP_RAW_STOCK;
}

static inline bool pedalMapSessionSetTargetPure(PedalMapSessionPure &s,
                                                 uint8_t stockRaw,
                                                 uint8_t targetRaw) {
  if (!pedalMapRawSelectablePure(targetRaw)) return false;
  if (targetRaw == PEDAL_MAP_RAW_STOCK) {
    pedalMapSessionClearPure(s);
    return true;
  }
  if (stockRaw > PEDAL_MAP_RAW_PERFORMANCE) return false;
  if (targetRaw == stockRaw && !s.active) {
    pedalMapSessionClearPure(s);
    return true;
  }
  if (!s.active) s.originRaw = stockRaw;
  s.active = true;
  s.targetRaw = targetRaw;
  return true;
}

static inline uint8_t pedalMapToggleTargetPure(const PedalMapSessionPure &s,
                                                uint8_t stockRaw) {
  const uint8_t current = s.active ? s.targetRaw : stockRaw;
  return current == PEDAL_MAP_RAW_CHILL ? PEDAL_MAP_RAW_SPORT
                                        : PEDAL_MAP_RAW_CHILL;
}

// Stock observation never silently releases a requested drive-session target.
// This lets a target survive stock echo/adoption and resume after temporary AP
// ownership. Explicit STOCK or confirmed Park owns release semantics.
static inline void pedalMapSessionObserveStockPure(PedalMapSessionPure &s,
                                                    uint8_t stockRaw) {
  (void)s;
  (void)stockRaw;
}

static inline bool pedalMapEnteredParkPure(int previousGearState,
                                            int currentGearState) {
  return currentGearState == 1 && previousGearState != 1;
}

static inline void pedalMapSessionOnGearTransitionPure(PedalMapSessionPure &s,
                                                        int previousGearState,
                                                        int currentGearState) {
  if (pedalMapEnteredParkPure(previousGearState, currentGearState)) {
    pedalMapSessionClearPure(s);
  }
}
