#include <cassert>

#include "../nag_human_v4_pure.h"
#include "../nag_mode_h_variant_pure.h"

static void test_v37_defaults_match_the_approved_profile() {
  assert(nagModeHDefaultVariantPure() == H_VARIANT_REV4);
  const NagHumanV4ConfigPure defaults = nagHumanV4DefaultConfigPure();
  assert(defaults.base.peakMinRaw == 180u);
  assert(defaults.base.peakMaxRaw == 260u);
  assert(defaults.base.waitMinMs == 900u);
  assert(defaults.base.waitMaxMs == 3000u);
  assert(defaults.base.refractoryMinMs == 500u);
  assert(defaults.base.refractoryMaxMs == 1500u);
  assert(defaults.carrierMinRaw == 10u);
  assert(defaults.carrierMaxRaw == 60u);
  assert(defaults.hoPolicy == H3_HO_TIERED_1_2);
  assert(defaults.ho1ThresholdRaw == 40u);
  assert(defaults.ho2ThresholdRaw == 200u);
  assert(defaults.visualRescueEnabled);
  assert(defaults.visualRescueDelayMs == 500u);
  assert(nagModeHDefaultStopBehaviorPure() == H_STOP_HARD_PAUSE);
}

static void test_exact_v17_default_migrates() {
  NagHumanV4ConfigPure old = nagHumanV4DefaultConfigPure();
  old.ho2ThresholdRaw = 175u;
  assert(nagHumanV4MigrateV17DefaultPure(old));
  assert(old.ho2ThresholdRaw == 200u);
}

static void test_custom_v17_profile_is_preserved() {
  NagHumanV4ConfigPure old = nagHumanV4DefaultConfigPure();
  old.base.peakMaxRaw = 255u;
  old.ho2ThresholdRaw = 175u;
  assert(!nagHumanV4MigrateV17DefaultPure(old));
  assert(old.base.peakMaxRaw == 255u);
  assert(old.ho2ThresholdRaw == 175u);
}

int main() {
  test_v37_defaults_match_the_approved_profile();
  test_exact_v17_default_migrates();
  test_custom_v17_profile_is_preserved();
  return 0;
}
