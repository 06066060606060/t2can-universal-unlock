#include <cassert>
#include "../nag_human_v3_pure.h"

int main() {
  const auto d = nagHumanV3DefaultConfigPure();
  assert(d.base.peakMinRaw == 150u);
  assert(d.base.peakMaxRaw == 210u);
  assert(d.base.waitMinMs == 1200u);
  assert(d.base.waitMaxMs == 2500u);
  assert(d.base.refractoryMinMs == 800u);
  assert(d.base.refractoryMaxMs == 1800u);
  assert(d.carrierMinRaw == 30u && d.carrierMaxRaw == 40u);
  assert(d.carrierDirectionMode == H3_CARRIER_FOLLOW_STOCK);
  assert(d.hoPolicy == H3_HO_TIERED_1_2);
  assert(d.ho1ThresholdRaw == 40u);
  assert(d.ho2ThresholdRaw == 175u);

  auto old = d;
  old.base.peakMaxRaw = 220u; old.base.waitMaxMs = 3000u;
  old.ho1ThresholdRaw = 50u; old.ho2ThresholdRaw = 160u;
  assert(nagHumanV3MigrateD3DefaultToD4Pure(old));
  assert(old.base.peakMaxRaw == 210u && old.base.waitMaxMs == 2500u);
  assert(old.ho1ThresholdRaw == 40u && old.ho2ThresholdRaw == 175u);

  auto approved = d; approved.ho1ThresholdRaw = 50u;
  assert(nagHumanV3MigrateD3DefaultToD4Pure(approved));
  assert(approved.ho1ThresholdRaw == 40u);

  auto custom = d; custom.base.peakMaxRaw = 230u;
  assert(!nagHumanV3MigrateD3DefaultToD4Pure(custom));
  assert(custom.base.peakMaxRaw == 230u);
  return 0;
}
