#pragma once

// Mode H runtime ownership. Included by can_core.h after shared NAG context
// and echo helpers, preserving initialization and lock ordering.

// Mode H uses the established Rev.4 engine; persisted id 1 and h4* tuning remain stable.
static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();
static portMUX_TYPE nagHumanMux = portMUX_INITIALIZER_UNLOCKED;
static ModeHConfigPure modeHConfig = modeHDefaultConfigPure();
static ModeHStatePure modeHState = {};
static bool modeHInitialized = false;

static uint32_t nagHumanRuntimeSeed() {
  uint32_t seed = (uint32_t)esp_random() ^ (uint32_t)micros() ^ 0x484D4F44u;
  return modeHBaseSanitizeSeedPure(seed);
}

static uint8_t nagHumanVariantSnapshot() {
  portENTER_CRITICAL(&nagHumanMux);
  const uint8_t v = nagModeHVariantValidPure(nagHumanVariant) ? nagHumanVariant : nagModeHDefaultVariantPure();
  portEXIT_CRITICAL(&nagHumanMux);
  return v;
}

static void nagHumanRuntimeReset(bool reseed) {
  const uint32_t seed = reseed ? nagHumanRuntimeSeed() : 0u;
  portENTER_CRITICAL(&nagHumanMux);
  if (!modeHInitialized) {
    modeHInitPure(modeHState, reseed ? seed ^ 0x52345634u : 0x52345634u);
    modeHInitialized = true;
  } else if (reseed) {
    modeHReseedPure(modeHState, seed ^ 0x52345634u);
  } else {
    modeHResetRuntimePure(modeHState, MH_IDLE);
  }
  portEXIT_CRITICAL(&nagHumanMux);
}

static void nagHumanRuntimeSetVariant(uint8_t variant) {
  const uint8_t normalized = nagModeHVariantValidPure(variant) ? variant : nagModeHDefaultVariantPure();
  portENTER_CRITICAL(&nagHumanMux);
  nagHumanVariant = normalized;
  portEXIT_CRITICAL(&nagHumanMux);
  nagExactEchoReset();
  nagHumanRuntimeReset(true);
  if (normalized == H_VARIANT_REV4) {
    uint32_t currentVisualWarningEpoch;
    portENTER_CRITICAL(&nagCtxMux);
    currentVisualWarningEpoch = nagCtx.visualWarningEpoch;
    portEXIT_CRITICAL(&nagCtxMux);
    portENTER_CRITICAL(&nagHumanMux);
    modeHState.visualWarningEpochSeen = currentVisualWarningEpoch;
    modeHState.visualRescuePending = false;
    modeHState.visualRescuePendingSinceMs = 0u;
    portEXIT_CRITICAL(&nagHumanMux);
  }
}

static ModeHBaseStepResultPure nagHumanRuntimeStep(uint32_t nowMs,
                                                     uint16_t sourceRaw,
                                                     bool runAllowed,
                                                     bool speedValid,
                                                     bool speedFresh,
                                                     uint16_t speedRaw,
                                                     uint32_t visualWarningEpoch,
                                                     uint32_t visualWarningEnterMs,
                                                     bool visualWarningActive) {
  ModeHBaseStepResultPure result = {};
  portENTER_CRITICAL(&nagHumanMux);
{
    if (!modeHInitialized) {
      modeHInitPure(modeHState, nagHumanRuntimeSeed() ^ 0x52345634u);
      modeHInitialized = true;
    }
    result = modeHStepPure(modeHState, modeHConfig, nowMs,
                                sourceRaw, runAllowed, speedValid, speedFresh, speedRaw,
                                visualWarningEpoch, visualWarningEnterMs, visualWarningActive);
  }
  portEXIT_CRITICAL(&nagHumanMux);
  return result;
}

static ModeHStatePure modeHRuntimeSnapshot() {
  ModeHStatePure snapshot = {};
  portENTER_CRITICAL(&nagHumanMux);
  snapshot = modeHState;
  portEXIT_CRITICAL(&nagHumanMux);
  return snapshot;
}

static ModeHConfigPure modeHRuntimeConfigSnapshot() {
  ModeHConfigPure snapshot = {};
  portENTER_CRITICAL(&nagHumanMux);
  snapshot = modeHConfig;
  portEXIT_CRITICAL(&nagHumanMux);
  return snapshot;
}

static bool modeHRuntimeSetLabTuning(
    uint16_t peakMinRaw, uint16_t peakMaxRaw,
    uint16_t waitMinMs, uint16_t waitMaxMs,
    uint16_t refractoryMinMs, uint16_t refractoryMaxMs,
    uint16_t carrierMinRaw, uint16_t carrierMaxRaw,
    uint8_t hoPolicy, uint16_t ho1ThresholdRaw, uint16_t ho2ThresholdRaw,
    bool visualRescueEnabled, uint16_t visualRescueDelayMs) {
  ModeHConfigPure next = modeHRuntimeConfigSnapshot();
  next.base.peakMinRaw = peakMinRaw;
  next.base.peakMaxRaw = peakMaxRaw;
  next.base.waitMinMs = waitMinMs;
  next.base.waitMaxMs = waitMaxMs;
  next.base.refractoryMinMs = refractoryMinMs;
  next.base.refractoryMaxMs = refractoryMaxMs;
  next.base.hoOverridePct = 0u;
  next.carrierMinRaw = carrierMinRaw;
  next.carrierMaxRaw = carrierMaxRaw;
  next.hoPolicy = hoPolicy;
  next.ho1ThresholdRaw = ho1ThresholdRaw;
  next.ho2ThresholdRaw = ho2ThresholdRaw;
  next.visualRescueEnabled = visualRescueEnabled;
  next.visualRescueDelayMs = visualRescueDelayMs;
  if (!modeHConfigValidPure(next)) return false;
  portENTER_CRITICAL(&nagHumanMux);
  const uint32_t visualWarningEpochSeen = modeHState.visualWarningEpochSeen;
  const uint32_t visualRescueCount = modeHState.visualRescueCount;
  modeHConfig = next;
  modeHState = ModeHStatePure{};
  modeHState.visualWarningEpochSeen = visualWarningEpochSeen;
  modeHState.visualRescueCount = visualRescueCount;
  modeHInitialized = false;
  portEXIT_CRITICAL(&nagHumanMux);
  nagExactEchoReset();
  return true;
}

static void nagHumanRuntimeLoadDefaults() {
  portENTER_CRITICAL(&nagHumanMux);
  nagHumanVariant = nagModeHDefaultVariantPure();
  modeHConfig = modeHDefaultConfigPure();
  modeHState = ModeHStatePure{};
  modeHInitialized = false;
  portEXIT_CRITICAL(&nagHumanMux);
}

static void nagHumanRuntimeApTransition(bool apActive, bool wasApActive) {
  if (apActive == wasApActive) return;
  uint8_t mode;
  bool ignoreApState;
  portENTER_CRITICAL(&nagCfgMux);
  mode = nagCfg.mode;
  ignoreApState = nagCfg.ignoreApState;
  portEXIT_CRITICAL(&nagCfgMux);
  if (mode == MODE_H && !ignoreApState) {
    nagExactEchoReset();
    nagHumanRuntimeReset(true);
  }
}

