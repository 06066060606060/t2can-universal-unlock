#include <cassert>
#include <cstdint>
#include <cstring>

#include "../nag_mode_h_variant_pure.h"
#include "../nag_human_v3_pure.h"

int main() {
  assert(!nagModeHVariantValidPure(3u));
  assert(std::strcmp(nagModeHVariantCodePure(3u), "H") == 0);
  assert(std::strcmp(nagModeHVariantLabelPure(3u), "Mode H") == 0);

  NagHumanV3ConfigPure c = nagHumanV3DefaultConfigPure();
  assert(c.base.peakMinRaw == 150u);
  assert(c.base.peakMaxRaw == 210u);
  assert(c.base.waitMinMs == 1200u);
  assert(c.base.waitMaxMs == 2500u);
  assert(c.base.refractoryMinMs == 800u);
  assert(c.base.refractoryMaxMs == 1800u);
  assert(c.carrierMinRaw == 30u);
  assert(c.carrierMaxRaw == 40u);
  assert(c.carrierDirectionMode == H3_CARRIER_FOLLOW_STOCK);
  assert(c.hoPolicy == H3_HO_TIERED_1_2);
  assert(c.ho1ThresholdRaw == 40u);
  assert(c.ho2ThresholdRaw == 175u);
  assert(nagHumanV3ConfigValidPure(c));

  // WAIT carrier follows stock sign and adds 0.30-0.40 Nm to the live stock torque.
  NagHumanV3StatePure s = {};
  nagHumanV3InitPure(s, 0x12345678u);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  auto r = nagHumanV3StepPure(s, c, 100u,
                              NAG_HUMAN_V1_TORQUE_CENTER_RAW + 10u,
                              true, true, true, 1000u);
  assert(r.tx);
  assert(r.phase == H1_WAIT);
  assert(r.carrier);
  assert(r.raw >= NAG_HUMAN_V1_TORQUE_CENTER_RAW + 40u);
  assert(r.raw <= NAG_HUMAN_V1_TORQUE_CENTER_RAW + 50u);

  // Re-enter WAIT with negative stock torque; FOLLOW_STOCK must follow the negative sign.
  nagHumanV3InitPure(s, 0xABCDEF01u);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  r = nagHumanV3StepPure(s, c, 200u,
                         NAG_HUMAN_V1_TORQUE_CENTER_RAW - 10u,
                         true, true, true, 1000u);
  assert(r.tx && r.phase == H1_WAIT && r.carrier);
  assert(r.raw <= NAG_HUMAN_V1_TORQUE_CENTER_RAW - 40u);
  assert(r.raw >= NAG_HUMAN_V1_TORQUE_CENTER_RAW - 50u);

  // RANDOM mode still adds a bounded 0.30-0.40 Nm offset, but chooses the sign independently.
  c.carrierDirectionMode = H3_CARRIER_RANDOM_SIGN;
  nagHumanV3InitPure(s, 0xCAFEBABEu);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  const uint16_t stock = NAG_HUMAN_V1_TORQUE_CENTER_RAW + 8u;
  r = nagHumanV3StepPure(s, c, 300u, stock, true, true, true, 1000u);
  const uint16_t delta = r.raw > stock ? (uint16_t)(r.raw - stock) : (uint16_t)(stock - r.raw);
  assert(r.tx && r.phase == H1_WAIT && r.carrier);
  assert(delta >= 30u && delta <= 40u);

  // HO policies: never emit 3. d4 tiered defaults use 0.40 / 1.75 Nm boundaries.
  NagHumanV3HoDecisionPure h = nagHumanV3HoDecisionPure(H3_HO_ALWAYS_1, 0u, 40u, 175u);
  assert(h.overrideValid && h.level == 1u);
  h = nagHumanV3HoDecisionPure(H3_HO_THRESHOLD_1, 39u, 40u, 175u);
  assert(!h.overrideValid);
  h = nagHumanV3HoDecisionPure(H3_HO_THRESHOLD_1, 40u, 40u, 175u);
  assert(h.overrideValid && h.level == 1u);
  h = nagHumanV3HoDecisionPure(H3_HO_TIERED_1_2, 174u, 40u, 175u);
  assert(h.overrideValid && h.level == 1u);
  h = nagHumanV3HoDecisionPure(H3_HO_TIERED_1_2, 175u, 40u, 175u);
  assert(h.overrideValid && h.level == 2u);
  h = nagHumanV3HoDecisionPure(H3_HO_TIERED_1_2, 300u, 40u, 175u);
  assert(h.overrideValid && h.level == 2u);
  assert(nagHumanV3SafeHoLevelPure(3u) == 2u);

  // Invalid threshold ordering must be rejected.
  c.ho1ThresholdRaw = 180u;
  c.ho2ThresholdRaw = 175u;
  assert(!nagHumanV3ConfigValidPure(c));
  return 0;
}
