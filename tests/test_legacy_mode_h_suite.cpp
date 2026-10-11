// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include "fixtures/mode_h_legacy/nag_human_v1_pure.h"
#include "fixtures/mode_h_legacy/nag_human_v2_pure.h"
#include "fixtures/mode_h_legacy/nag_human_v3_pure.h"
#include "fixtures/mode_h_legacy/nag_human_v4_pure.h"
#include "fixtures/mode_h_legacy/nag_mode_h_variant_pure.h"
#include <cstring>

// Coverage origin: test_v36b16_mode_h_rev1_pure.cpp
namespace case_mode_h_rev1_pure {
static uint16_t movingSpeedRaw() { return 600u; } // 8 kph

int run() {
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    assert(c.hoOverridePct == 100u);
    assert(nagHumanV1TimingValidPure(c.waitMinMs, c.waitMaxMs,
                                   c.refractoryMinMs, c.refractoryMaxMs));
    assert(!nagHumanV1TimingValidPure(200u, 3000u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 5100u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(2200u, 1200u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 3000u, 200u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 3000u, 1800u, 1700u));
  }

  // 100% override preserves the old Mode H carrier behavior.
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 100u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0x12345678u);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto r = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(r.tx);
    assert(r.setHo);
  }

  // 0% override must preserve the stock HO bits rather than forcing HO=0.
  // setHo=false is the transport contract for "leave stock untouched".
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 0u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0xCAFEBABEu);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto r = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(r.tx);
    assert(!r.setHo);
  }

  // A held decision must not reroll on every CAN frame.
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 50u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0x0BADF00Du);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto a = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    auto b = nagHumanV1StepPure(s, c, 450u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(a.tx && b.tx);
    assert(a.setHo == b.setHo);
  }

  return 0;
  return 0;
}
} // namespace case_mode_h_rev1_pure

// Coverage origin: test_v36b20_mode_h_variants_pure.cpp
namespace case_mode_h_variants_pure {
int run() {
  static_assert(H_VARIANT_REV4 == 1, "Mode H persisted slot retained");
  for (unsigned raw = 0; raw <= 255; ++raw) {
    assert(nagModeHVariantValidPure(static_cast<uint8_t>(raw)) == (raw == 1));
  }
  assert(nagModeHDefaultVariantPure() == 1);
  // Historical engine helpers remain tested below as shared-header regressions;
  // raw persisted legacy selections 0, 2 and 3 are no longer valid modes.

  const auto r1 = nagHumanV1Rev1ConfigPure();
  const auto r1p = nagHumanV1Rev1PlusConfigPure();
  assert(r1.peakMinRaw == 150u && r1.peakMaxRaw == 200u);
  assert(r1p.peakMinRaw == 150u && r1p.peakMaxRaw == 220u);
  // Timing / HO policy stays identical between Rev.1 and Rev.1 Plus.
  assert(r1.waitMinMs == r1p.waitMinMs && r1.waitMaxMs == r1p.waitMaxMs);
  assert(r1.interactMinMs == r1p.interactMinMs && r1.interactMaxMs == r1p.interactMaxMs);
  assert(r1.refractoryMinMs == r1p.refractoryMinMs && r1.refractoryMaxMs == r1p.refractoryMaxMs);
  assert(r1.hoOverridePct == 100u && r1p.hoOverridePct == 100u);

  const auto r4 = nagHumanV4DefaultConfigPure();
  assert(nagHumanV4ConfigValidPure(r4));
  assert(r4.base.peakMinRaw == 180u && r4.base.peakMaxRaw == 260u);
  assert(r4.base.waitMinMs == 900u && r4.base.waitMaxMs == 3000u);
  assert(r4.base.refractoryMinMs == 500u && r4.base.refractoryMaxMs == 1500u);
  assert(r4.carrierMinRaw == 10u && r4.carrierMaxRaw == 60u);
  assert(r4.visualRescueEnabled && r4.visualRescueDelayMs == 500u);

  const auto r2 = nagHumanV2DefaultConfigPure();
  assert(r2.holdNormalMinRaw == 15u && r2.holdNormalMaxRaw == 45u);
  assert(r2.holdExcursionMaxRaw == 65u);
  assert(r2.naturalTapMinRaw == 180u && r2.naturalTapMaxRaw == 220u);
  assert(r2.naturalTapIntervalMinMs == 3000u && r2.naturalTapIntervalMaxMs == 6000u);
  assert((uint32_t)r2.tapAttackMinMs + r2.tapPeakMinMs + r2.tapReleaseMinMs == 330u);
  assert((uint32_t)r2.tapAttackMaxMs + r2.tapPeakMaxMs + r2.tapReleaseMaxMs == 700u);

  const auto r3 = nagHumanV3DefaultConfigPure();
  assert(nagHumanV3ConfigValidPure(r3));
  assert(r3.base.peakMinRaw == 150u && r3.base.peakMaxRaw == 210u);
  assert(r3.base.waitMinMs == 1200u && r3.base.waitMaxMs == 2500u);
  assert(r3.base.refractoryMinMs == 800u && r3.base.refractoryMaxMs == 1800u);
  assert(r3.carrierMinRaw == 30u && r3.carrierMaxRaw == 40u);
  assert(r3.ho1ThresholdRaw == 40u && r3.ho2ThresholdRaw == 175u);


  // Runtime cadence contract: after a natural peak starts, the next peak is
  // scheduled 3–6 seconds from that start (the approved comparison graphs use
  // start-to-start cadence; the 0.33–0.70 s waveform must not be added on top).
  {
    NagHumanV2StatePure rt = {};
    nagHumanV2InitPure(rt, 0x31415926u);
    const uint16_t speedRaw = 600u; // (600*8-4000)/100 = 8 km/h
    bool sawTap = false;
    uint32_t tapStart = 0u;
    for (uint32_t now = 0u; now < 12000u; now += 20u) {
      const auto step = nagHumanV2StepPure(rt, r2, now, NAG_HUMAN_V2_TORQUE_CENTER_RAW,
                                           true, true, true, speedRaw);
      (void)step;
      if (rt.eventValid && rt.event.type == H_EVENT_NATURAL_TAP) {
        sawTap = true;
        tapStart = now;
        break;
      }
    }
    assert(sawTap);
    const uint32_t delta = rt.nextNaturalTapMs - tapStart;
    assert(delta >= 3000u && delta <= 6000u);
  }

  // Each completed 5-event bag is exactly 4 negative + 1 positive.
  NagHumanV2StatePure s = {};
  nagHumanV2InitPure(s, 0x12345678u);
  for (int block = 0; block < 8; ++block) {
    int neg = 0, pos = 0;
    for (int i = 0; i < 5; ++i) {
      const int8_t dir = nagHumanV2NextPeakDirectionPure(s);
      if (dir < 0) ++neg; else if (dir > 0) ++pos;
    }
    assert(neg == 4 && pos == 1);
  }

  return 0;
  return 0;
}
} // namespace case_mode_h_variants_pure

// Coverage origin: test_v36c2_mode_h_rev3_pure.cpp
namespace case_mode_h_rev3_pure {
int run() {
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
  return 0;
}
} // namespace case_mode_h_rev3_pure

// Coverage origin: test_v36d4_defaults_pure.cpp
namespace case_defaults_pure {
int run() {
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
  return 0;
}
} // namespace case_defaults_pure

int main() {
  if (case_mode_h_rev1_pure::run()) return 1;
  if (case_mode_h_variants_pure::run()) return 1;
  if (case_mode_h_rev3_pure::run()) return 1;
  if (case_defaults_pure::run()) return 1;
  return 0;
}
