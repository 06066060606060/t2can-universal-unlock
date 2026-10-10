#include <cassert>
#include <cstdint>
#include "../nag_human_v1_pure.h"
#include "../nag_human_v2_pure.h"
#include "../nag_human_v3_pure.h"
#include "../nag_human_v4_pure.h"
#include "../nag_mode_h_variant_pure.h"

int main() {
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
}
