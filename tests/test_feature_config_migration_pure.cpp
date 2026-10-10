#include <cassert>

#include "../feature_config_migration_pure.h"

int main() {
  LegacyLab3f8ConfigPure old = {};
  old.alcMode = 2;
  old.blindMode = 2;
  old.ulcOffHighwayMode = 1;
  old.confirmFreeEnabled = true;
  old.confirmFreeTiming = 1;
  old.autoLaneChange293Enabled = true;
  old.autoLaneChange293Bus = 3;
  const FeatureConfigMigrationPure migrated = migrateLegacyLab3f8Pure(old);
  assert(migrated.ulc.alcOffHighwayEnabled);
  assert(migrated.ulc.blindSpotMode == 2);
  assert(migrated.ulc.ulcOffHighwayMode == 1);
  assert(migrated.ulc.confirmFreeEnabled);
  assert(migrated.ulc.confirmFreeTiming == 1);
  assert(migrated.autoLaneChange.enabled);
  assert(migrated.autoLaneChange.bus == 3);

  old.alcMode = 1;
  old.blindMode = 9;
  old.ulcOffHighwayMode = 4;
  old.confirmFreeTiming = 7;
  old.autoLaneChange293Bus = 9;
  const FeatureConfigMigrationPure sanitized = migrateLegacyLab3f8Pure(old);
  assert(!sanitized.ulc.alcOffHighwayEnabled);
  assert(sanitized.ulc.blindSpotMode == FEATURE_CONFIG_STOCK_PURE);
  assert(sanitized.ulc.ulcOffHighwayMode == FEATURE_CONFIG_STOCK_PURE);
  assert(sanitized.ulc.confirmFreeTiming == 0);
  assert(sanitized.autoLaneChange.bus == 3);

  const FeatureConfigV37Pure defaults = featureConfigV37DefaultsPure();
  assert(defaults.r79Bit18Mode == FEATURE_CONFIG_R79_STOCK_PURE);
  assert(defaults.noaStabilizationSeconds == 10u);
  assert(defaults.cancelPauseSeconds == 20u);
  assert(defaults.rightScrollWarningSeconds == 2u);

  assert(featureConfigV37FromLegacyPure(0u).r79Bit18Mode ==
         FEATURE_CONFIG_R79_STOCK_PURE);
  assert(featureConfigV37FromLegacyPure(1u).r79Bit18Mode ==
         FEATURE_CONFIG_R79_FORCE_0_PURE);
  assert(featureConfigV37FromLegacyPure(2u).r79Bit18Mode ==
         FEATURE_CONFIG_R79_STOCK_PURE);
  assert(featureConfigV37FromLegacyPure(0xFFu).r79Bit18Mode ==
         FEATURE_CONFIG_R79_STOCK_PURE);

  return 0;
}
