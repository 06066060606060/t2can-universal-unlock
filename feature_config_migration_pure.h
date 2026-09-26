#pragma once

#include <stdint.h>

static constexpr uint8_t FEATURE_CONFIG_STOCK_PURE = 0xFFu;
static constexpr uint8_t FEATURE_CONFIG_AUTO_LC_BUS_BOTH_PURE = 3u;
static constexpr uint8_t FEATURE_CONFIG_R79_STOCK_PURE = 0u;
static constexpr uint8_t FEATURE_CONFIG_R79_FORCE_0_PURE = 1u;

struct FeatureConfigV37Pure {
  uint8_t r79Bit18Mode;
  uint8_t noaStabilizationSeconds;
  uint8_t cancelPauseSeconds;
  uint8_t rightScrollWarningSeconds;
};

struct FeatureConfigSchema3CommitPure {
  bool markerAllowed;
  bool cleanupAllowed;
};

static inline FeatureConfigV37Pure featureConfigV37DefaultsPure() {
  FeatureConfigV37Pure config = {};
  config.r79Bit18Mode = FEATURE_CONFIG_R79_FORCE_0_PURE;
  config.noaStabilizationSeconds = 10u;
  config.cancelPauseSeconds = 20u;
  config.rightScrollWarningSeconds = 2u;
  return config;
}

static inline FeatureConfigV37Pure featureConfigV37FromLegacyPure(
    uint8_t legacyR79Bit18Mode) {
  FeatureConfigV37Pure config = featureConfigV37DefaultsPure();
  if (legacyR79Bit18Mode == FEATURE_CONFIG_R79_STOCK_PURE ||
      legacyR79Bit18Mode == FEATURE_CONFIG_R79_FORCE_0_PURE) {
    config.r79Bit18Mode = legacyR79Bit18Mode;
  }
  return config;
}

static inline FeatureConfigSchema3CommitPure featureConfigSchema3CommitPure(
    bool valuesWritten, bool valuesVerified, bool markerDurable) {
  FeatureConfigSchema3CommitPure result = {};
  result.markerAllowed = valuesWritten && valuesVerified;
  result.cleanupAllowed = result.markerAllowed && markerDurable;
  return result;
}

struct LegacyLab3f8ConfigPure {
  uint8_t alcMode;
  uint8_t blindMode;
  uint8_t ulcOffHighwayMode;
  bool confirmFreeEnabled;
  uint8_t confirmFreeTiming;
  bool autoLaneChange293Enabled;
  uint8_t autoLaneChange293Bus;
};

struct ProductionUlcConfigPure {
  bool alcOffHighwayEnabled;
  uint8_t blindSpotMode;
  uint8_t ulcOffHighwayMode;
  bool confirmFreeEnabled;
  uint8_t confirmFreeTiming;
};

struct AutoLaneChangeLabConfigPure {
  bool enabled;
  uint8_t bus;
};

struct FeatureConfigMigrationPure {
  ProductionUlcConfigPure ulc;
  AutoLaneChangeLabConfigPure autoLaneChange;
};

static inline FeatureConfigMigrationPure migrateLegacyLab3f8Pure(
    const LegacyLab3f8ConfigPure &legacy) {
  FeatureConfigMigrationPure migrated = {};
  migrated.ulc.alcOffHighwayEnabled = legacy.alcMode == 2u;
  migrated.ulc.blindSpotMode =
      legacy.blindMode == FEATURE_CONFIG_STOCK_PURE || legacy.blindMode <= 2u
      ? legacy.blindMode : FEATURE_CONFIG_STOCK_PURE;
  migrated.ulc.ulcOffHighwayMode =
      legacy.ulcOffHighwayMode == FEATURE_CONFIG_STOCK_PURE ||
          legacy.ulcOffHighwayMode <= 1u
      ? legacy.ulcOffHighwayMode : FEATURE_CONFIG_STOCK_PURE;
  migrated.ulc.confirmFreeEnabled = legacy.confirmFreeEnabled;
  migrated.ulc.confirmFreeTiming = legacy.confirmFreeTiming <= 1u
      ? legacy.confirmFreeTiming : 0u;
  migrated.autoLaneChange.enabled = legacy.autoLaneChange293Enabled;
  migrated.autoLaneChange.bus =
      legacy.autoLaneChange293Bus >= 1u && legacy.autoLaneChange293Bus <= 3u
      ? legacy.autoLaneChange293Bus : FEATURE_CONFIG_AUTO_LC_BUS_BOTH_PURE;
  return migrated;
}
