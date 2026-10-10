#include <cassert>
#include <cstdint>
#include <set>
#include "../nag_human_v4_pure.h"
#include "../nag_mode_h_variant_pure.h"

int main() {
  static_assert(H_VARIANT_REV4 == 1, "Rev.4 keeps persisted slot id 1");
  assert(nagModeHVariantValidPure(H_VARIANT_REV4));

  const NagHumanV4ConfigPure d = nagHumanV4DefaultConfigPure();
  assert(nagHumanV4ConfigValidPure(d));
  assert(d.carrierMinRaw == 10u && d.carrierMaxRaw == 60u);
  assert(d.base.peakMinRaw == 180u && d.base.peakMaxRaw == 260u);
  assert(d.base.waitMinMs == 900u && d.base.waitMaxMs == 3000u);
  assert(d.base.refractoryMinMs == 500u && d.base.refractoryMaxMs == 1500u);
  assert(d.visualRescueEnabled);
  assert(d.visualRescueDelayMs == 500u);
  assert(d.hoPolicy == H3_HO_TIERED_1_2);
  assert(d.ho1ThresholdRaw == 40u && d.ho2ThresholdRaw == 200u);
  assert(NAG_HUMAN_V4_STOCK_DEADBAND_RAW == 5u);
  assert(NAG_HUMAN_V4_NEGATIVE_BIAS_PCT == 80u);

  // Exact stock-opposite formula with a fixed 0.30 Nm carrier.
  NagHumanV4ConfigPure c = d;
  c.carrierMinRaw = c.carrierMaxRaw = 30u;
  NagHumanV4StatePure s = {};
  nagHumanV4InitPure(s, 0x12345678u);
  const uint16_t z = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  assert(nagHumanV4CarrierOutputPure(s, c, z + 10u) == z - 40u); // +0.10 -> -0.40
  assert(s.lastCarrierApplied && s.lastCarrierMagnitudeRaw == 30u);
  assert(nagHumanV4CarrierOutputPure(s, c, z - 10u) == z + 40u); // -0.10 -> +0.40
  assert(nagHumanV4CarrierOutputPure(s, c, z + 5u) == z + 5u);   // near zero -> stock
  assert(!s.lastCarrierApplied);
  assert(nagHumanV4CarrierOutputPure(s, c, z - 5u) == z - 5u);
  assert(!s.lastCarrierApplied);

  // Rev.4 re-samples the carrier on every eligible RX; it has no 350-900 ms hold.
  c = d;
  nagHumanV4InitPure(s, 0xCAFEBABEu);
  std::set<uint16_t> samples;
  for (int i = 0; i < 64; ++i) {
    const uint16_t out = nagHumanV4CarrierOutputPure(s, c, z + 10u);
    assert(out >= z - 70u && out <= z - 20u);
    assert(s.lastCarrierMagnitudeRaw >= 10u && s.lastCarrierMagnitudeRaw <= 60u);
    samples.insert(s.lastCarrierMagnitudeRaw);
  }
  assert(samples.size() > 20u);

  // Primary event direction is independently 80% negative / 20% positive.
  nagHumanV4InitPure(s, 0x31415926u);
  int neg = 0, pos = 0;
  for (int i = 0; i < 10000; ++i) {
    const auto e = nagHumanV4GenerateEventPure(s, d);
    if (e.direction < 0) ++neg; else if (e.direction > 0) ++pos;
    assert(e.peakRaw >= 180u && e.peakRaw <= 260u);
    assert(e.waitDurationMs >= 900u && e.waitDurationMs <= 3000u);
    assert(e.refractoryDurationMs >= 500u && e.refractoryDurationMs <= 1500u);
  }
  assert(neg + pos == 10000);
  assert(neg >= 7800 && neg <= 8200);

  // WAIT applies carrier on every step/RX once movement is already confirmed.
  c = d;
  nagHumanV4InitPure(s, 0x10203040u);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  auto r1 = nagHumanV4StepPure(s, c, 100u, z + 20u, true, true, true, 1000u);
  const uint16_t c1 = s.lastCarrierMagnitudeRaw;
  auto r2 = nagHumanV4StepPure(s, c, 110u, z + 20u, true, true, true, 1000u);
  const uint16_t c2 = s.lastCarrierMagnitudeRaw;
  assert(r1.tx && r2.tx && r1.phase == H1_WAIT && r2.phase == H1_WAIT);
  assert(r1.carrier && r2.carrier);
  // Deterministic seed chosen so the first two samples differ; proves no hold timer.
  assert(c1 != c2);

  // RAMP_OUT decays toward a captured Rev.4 carrier target, not stock. This
  // avoids a stock-sign snap immediately before the carrier-only REFRACTORY.
  c = d;
  c.carrierMinRaw = c.carrierMaxRaw = 30u;
  nagHumanV4InitPure(s, 0xAABBCCDDu);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  s.base.event = nagHumanV4GenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.event.type = H1_EVENT_SIMPLE;
  s.base.event.direction = 1;
  s.base.event.peakRaw = 180u;
  s.base.event.interactDurationMs = 100u;
  s.base.event.rampOutDurationMs = 100u;
  s.base.phase = H1_INTERACT;
  s.base.phaseStartMs = 1000u;
  const auto ro0 = nagHumanV4StepPure(s, c, 1100u, z + 10u, true, true, true, 1000u);
  assert(ro0.tx && ro0.phase == H1_RAMP_OUT);
  assert(s.rampOutTargetValid && s.rampOutTargetRaw == z - 40u);
  assert(ro0.raw == z + 180u);
  const auto roMid = nagHumanV4StepPure(s, c, 1150u, z + 10u, true, true, true, 1000u);
  assert(roMid.phase == H1_RAMP_OUT && !roMid.carrier);
  assert(roMid.raw < z + 180u && roMid.raw > z - 40u);

  // Force REFRACTORY and verify the same Rev.4 carrier rule is active there too.
  nagHumanV4InitPure(s, 0x55667788u);
  s.base.movingConfirmed = true;
  s.base.motion = H1_MOTION_MOVING;
  s.base.event = nagHumanV4GenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = H1_REFRACTORY;
  s.base.phaseStartMs = 1000u;
  s.base.event.refractoryDurationMs = 1000u;
  const auto rr = nagHumanV4StepPure(s, c, 1200u, z - 20u, true, true, true, 1000u);
  assert(rr.tx && rr.phase == H1_REFRACTORY && rr.carrier);
  assert(rr.raw >= z + 30u && rr.raw <= z + 80u); // |-0.20| + 0.10..0.60

  // Default Rev.4 Hands-On policy mirrors Rev.3 TIERED behavior using the
  // final transmitted torque magnitude: preserve stock below 0.40 Nm,
  // generate HO=1 from 0.40 Nm, and HO=2 from 2.00 Nm. HO=3 is never emitted.
  auto hoDecisionAt = [&](const NagHumanV4ConfigPure &hoConfig, uint16_t peakRaw) {
    NagHumanV4StatePure hoState = {};
    nagHumanV4InitPure(hoState, 0x77889900u + peakRaw);
    hoState.base.movingConfirmed = true;
    hoState.base.motion = H1_MOTION_MOVING;
    hoState.base.eventValid = true;
    hoState.base.event.type = H1_EVENT_SIMPLE;
    hoState.base.event.direction = 1;
    hoState.base.event.peakRaw = peakRaw;
    hoState.base.event.interactDurationMs = 1000u;
    hoState.base.phase = H1_INTERACT;
    hoState.base.phaseStartMs = 100u;
    return nagHumanV4StepPure(
        hoState, hoConfig, 200u, z, true, true, true, 1000u);
  };

  const auto hoBelow = hoDecisionAt(d, 39u);
  assert(!hoBelow.hoOverrideValid && hoBelow.hoLevel == 0u);
  const auto hoOne = hoDecisionAt(d, 40u);
  assert(hoOne.hoOverrideValid && hoOne.hoLevel == 1u);
  const auto hoOneUpperBoundary = hoDecisionAt(d, 199u);
  assert(hoOneUpperBoundary.hoOverrideValid && hoOneUpperBoundary.hoLevel == 1u);
  const auto hoTwo = hoDecisionAt(d, 200u);
  assert(hoTwo.hoOverrideValid && hoTwo.hoLevel == 2u);

  NagHumanV4ConfigPure threshold = d;
  threshold.hoPolicy = H3_HO_THRESHOLD_1;
  const auto thresholdLow = hoDecisionAt(threshold, 39u);
  assert(!thresholdLow.hoOverrideValid && thresholdLow.hoLevel == 0u);
  const auto thresholdHigh = hoDecisionAt(threshold, 260u);
  assert(thresholdHigh.hoOverrideValid && thresholdHigh.hoLevel == 1u);

  NagHumanV4ConfigPure always = d;
  always.hoPolicy = H3_HO_ALWAYS_1;
  const auto alwaysLow = hoDecisionAt(always, 1u);
  assert(alwaysLow.hoOverrideValid && alwaysLow.hoLevel == 1u);

  NagHumanV4ConfigPure invalidHo = d;
  invalidHo.ho1ThresholdRaw = 176u;
  invalidHo.ho2ThresholdRaw = 175u;
  assert(!nagHumanV4ConfigValidPure(invalidHo));
  invalidHo = d;
  invalidHo.hoPolicy = 0xFFu;
  assert(!nagHumanV4ConfigValidPure(invalidHo));

  // OTA migration changes only the exact v16 Rev.4 defaults. Any custom
  // value prevents migration so user tuning survives the schema update.
  NagHumanV4ConfigPure oldDefault = d;
  oldDefault.base.peakMinRaw = 150u;
  oldDefault.base.peakMaxRaw = 210u;
  oldDefault.base.waitMaxMs = 2000u;
  oldDefault.visualRescueEnabled = false;
  assert(nagHumanV4MigrateV16DefaultPure(oldDefault));
  assert(oldDefault.base.peakMinRaw == 180u && oldDefault.base.peakMaxRaw == 260u);
  assert(oldDefault.base.waitMinMs == 900u && oldDefault.base.waitMaxMs == 3000u);
  assert(oldDefault.visualRescueEnabled);
  assert(oldDefault.hoPolicy == H3_HO_TIERED_1_2);
  assert(oldDefault.ho1ThresholdRaw == 40u && oldDefault.ho2ThresholdRaw == 200u);

  NagHumanV4ConfigPure custom = d;
  custom.base.peakMinRaw = 170u;
  custom.base.peakMaxRaw = 230u;
  const NagHumanV4ConfigPure customBefore = custom;
  assert(!nagHumanV4MigrateV16DefaultPure(custom));
  assert(custom.base.peakMinRaw == customBefore.base.peakMinRaw);
  assert(custom.base.peakMaxRaw == customBefore.base.peakMaxRaw);
  assert(custom.visualRescueEnabled == customBefore.visualRescueEnabled);

  return 0;
}
