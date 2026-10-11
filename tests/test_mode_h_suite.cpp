// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstring>
#include "../mode_h_pure.h"
#include <cstdint>
#include <set>

// Coverage origin: test_v313_mode_h_single_pure.cpp
namespace case_mode_h_single_pure {
int run() {
 assert(nagModeHDefaultVariantPure()==1u);
 for(unsigned v=0;v<256;v++) assert(nagModeHVariantValidPure(v)==(v==1));
 assert(strcmp(nagModeHVariantLabelPure(1),"Mode H")==0);
  return 0;
}
} // namespace case_mode_h_single_pure

// Coverage origin: test_v36d5_mode_h_stop_carrier_pure.cpp
namespace case_mode_h_stop_carrier_pure {
int run() {
  assert(nagModeHStopBehaviorValidPure(H_STOP_HARD_PAUSE));
  assert(nagModeHStopBehaviorValidPure(H_STOP_STOCK_CARRIER));
  assert(!nagModeHStopBehaviorValidPure(2));
  assert(!nagModeHUseStopCarrierPure(H_STOP_HARD_PAUSE, true));
  assert(!nagModeHUseStopCarrierPure(H_STOP_STOCK_CARRIER, false));
  assert(nagModeHUseStopCarrierPure(H_STOP_STOCK_CARRIER, true));
  return 0;
  return 0;
}
} // namespace case_mode_h_stop_carrier_pure

// Coverage origin: test_v36e2_mode_h_rev4_pure.cpp
namespace case_mode_h_rev4_pure {
int run() {
  static_assert(H_VARIANT_REV4 == 1, "Rev.4 keeps persisted slot id 1");
  assert(nagModeHVariantValidPure(H_VARIANT_REV4));

  const ModeHConfigPure d = modeHDefaultConfigPure();
  assert(modeHConfigValidPure(d));
  assert(d.carrierMinRaw == 10u && d.carrierMaxRaw == 60u);
  assert(d.base.peakMinRaw == 180u && d.base.peakMaxRaw == 260u);
  assert(d.base.waitMinMs == 900u && d.base.waitMaxMs == 3000u);
  assert(d.base.refractoryMinMs == 500u && d.base.refractoryMaxMs == 1500u);
  assert(d.visualRescueEnabled);
  assert(d.visualRescueDelayMs == 500u);
  assert(d.hoPolicy == MH_HO_TIERED_1_2);
  assert(d.ho1ThresholdRaw == 40u && d.ho2ThresholdRaw == 200u);
  assert(MODE_H_STOCK_DEADBAND_RAW == 5u);
  assert(MODE_H_NEGATIVE_BIAS_PCT == 80u);

  // Exact stock-opposite formula with a fixed 0.30 Nm carrier.
  ModeHConfigPure c = d;
  c.carrierMinRaw = c.carrierMaxRaw = 30u;
  ModeHStatePure s = {};
  modeHInitPure(s, 0x12345678u);
  const uint16_t z = MODE_H_BASE_TORQUE_CENTER_RAW;
  assert(modeHCarrierOutputPure(s, c, z + 10u) == z - 40u); // +0.10 -> -0.40
  assert(s.lastCarrierApplied && s.lastCarrierMagnitudeRaw == 30u);
  assert(modeHCarrierOutputPure(s, c, z - 10u) == z + 40u); // -0.10 -> +0.40
  assert(modeHCarrierOutputPure(s, c, z + 5u) == z + 5u);   // near zero -> stock
  assert(!s.lastCarrierApplied);
  assert(modeHCarrierOutputPure(s, c, z - 5u) == z - 5u);
  assert(!s.lastCarrierApplied);

  // Rev.4 re-samples the carrier on every eligible RX; it has no 350-900 ms hold.
  c = d;
  modeHInitPure(s, 0xCAFEBABEu);
  std::set<uint16_t> samples;
  for (int i = 0; i < 64; ++i) {
    const uint16_t out = modeHCarrierOutputPure(s, c, z + 10u);
    assert(out >= z - 70u && out <= z - 20u);
    assert(s.lastCarrierMagnitudeRaw >= 10u && s.lastCarrierMagnitudeRaw <= 60u);
    samples.insert(s.lastCarrierMagnitudeRaw);
  }
  assert(samples.size() > 20u);

  // Primary event direction is independently 80% negative / 20% positive.
  modeHInitPure(s, 0x31415926u);
  int neg = 0, pos = 0;
  for (int i = 0; i < 10000; ++i) {
    const auto e = modeHGenerateEventPure(s, d);
    if (e.direction < 0) ++neg; else if (e.direction > 0) ++pos;
    assert(e.peakRaw >= 180u && e.peakRaw <= 260u);
    assert(e.waitDurationMs >= 900u && e.waitDurationMs <= 3000u);
    assert(e.refractoryDurationMs >= 500u && e.refractoryDurationMs <= 1500u);
  }
  assert(neg + pos == 10000);
  assert(neg >= 7800 && neg <= 8200);

  // WAIT applies carrier on every step/RX once movement is already confirmed.
  c = d;
  modeHInitPure(s, 0x10203040u);
  s.base.movingConfirmed = true;
  s.base.motion = MH_MOTION_MOVING;
  auto r1 = modeHStepPure(s, c, 100u, z + 20u, true, true, true, 1000u);
  const uint16_t c1 = s.lastCarrierMagnitudeRaw;
  auto r2 = modeHStepPure(s, c, 110u, z + 20u, true, true, true, 1000u);
  const uint16_t c2 = s.lastCarrierMagnitudeRaw;
  assert(r1.tx && r2.tx && r1.phase == MH_WAIT && r2.phase == MH_WAIT);
  assert(r1.carrier && r2.carrier);
  // Deterministic seed chosen so the first two samples differ; proves no hold timer.
  assert(c1 != c2);

  // RAMP_OUT decays toward a captured Rev.4 carrier target, not stock. This
  // avoids a stock-sign snap immediately before the carrier-only REFRACTORY.
  c = d;
  c.carrierMinRaw = c.carrierMaxRaw = 30u;
  modeHInitPure(s, 0xAABBCCDDu);
  s.base.movingConfirmed = true;
  s.base.motion = MH_MOTION_MOVING;
  s.base.event = modeHGenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.event.type = MH_EVENT_SIMPLE;
  s.base.event.direction = 1;
  s.base.event.peakRaw = 180u;
  s.base.event.interactDurationMs = 100u;
  s.base.event.rampOutDurationMs = 100u;
  s.base.phase = MH_INTERACT;
  s.base.phaseStartMs = 1000u;
  const auto ro0 = modeHStepPure(s, c, 1100u, z + 10u, true, true, true, 1000u);
  assert(ro0.tx && ro0.phase == MH_RAMP_OUT);
  assert(s.rampOutTargetValid && s.rampOutTargetRaw == z - 40u);
  assert(ro0.raw == z + 180u);
  const auto roMid = modeHStepPure(s, c, 1150u, z + 10u, true, true, true, 1000u);
  assert(roMid.phase == MH_RAMP_OUT && !roMid.carrier);
  assert(roMid.raw < z + 180u && roMid.raw > z - 40u);

  // Force REFRACTORY and verify the same Rev.4 carrier rule is active there too.
  modeHInitPure(s, 0x55667788u);
  s.base.movingConfirmed = true;
  s.base.motion = MH_MOTION_MOVING;
  s.base.event = modeHGenerateEventPure(s, c);
  s.base.eventValid = true;
  s.base.phase = MH_REFRACTORY;
  s.base.phaseStartMs = 1000u;
  s.base.event.refractoryDurationMs = 1000u;
  const auto rr = modeHStepPure(s, c, 1200u, z - 20u, true, true, true, 1000u);
  assert(rr.tx && rr.phase == MH_REFRACTORY && rr.carrier);
  assert(rr.raw >= z + 30u && rr.raw <= z + 80u); // |-0.20| + 0.10..0.60

  // Default Rev.4 Hands-On policy mirrors Rev.3 TIERED behavior using the
  // final transmitted torque magnitude: preserve stock below 0.40 Nm,
  // generate HO=1 from 0.40 Nm, and HO=2 from 2.00 Nm. HO=3 is never emitted.
  auto hoDecisionAt = [&](const ModeHConfigPure &hoConfig, uint16_t peakRaw) {
    ModeHStatePure hoState = {};
    modeHInitPure(hoState, 0x77889900u + peakRaw);
    hoState.base.movingConfirmed = true;
    hoState.base.motion = MH_MOTION_MOVING;
    hoState.base.eventValid = true;
    hoState.base.event.type = MH_EVENT_SIMPLE;
    hoState.base.event.direction = 1;
    hoState.base.event.peakRaw = peakRaw;
    hoState.base.event.interactDurationMs = 1000u;
    hoState.base.phase = MH_INTERACT;
    hoState.base.phaseStartMs = 100u;
    return modeHStepPure(
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

  ModeHConfigPure threshold = d;
  threshold.hoPolicy = MH_HO_THRESHOLD_1;
  const auto thresholdLow = hoDecisionAt(threshold, 39u);
  assert(!thresholdLow.hoOverrideValid && thresholdLow.hoLevel == 0u);
  const auto thresholdHigh = hoDecisionAt(threshold, 260u);
  assert(thresholdHigh.hoOverrideValid && thresholdHigh.hoLevel == 1u);

  ModeHConfigPure always = d;
  always.hoPolicy = MH_HO_ALWAYS_1;
  const auto alwaysLow = hoDecisionAt(always, 1u);
  assert(alwaysLow.hoOverrideValid && alwaysLow.hoLevel == 1u);

  ModeHConfigPure invalidHo = d;
  invalidHo.ho1ThresholdRaw = 176u;
  invalidHo.ho2ThresholdRaw = 175u;
  assert(!modeHConfigValidPure(invalidHo));
  invalidHo = d;
  invalidHo.hoPolicy = 0xFFu;
  assert(!modeHConfigValidPure(invalidHo));

  // OTA migration changes only the exact v16 Rev.4 defaults. Any custom
  // value prevents migration so user tuning survives the schema update.
  ModeHConfigPure oldDefault = d;
  oldDefault.base.peakMinRaw = 150u;
  oldDefault.base.peakMaxRaw = 210u;
  oldDefault.base.waitMaxMs = 2000u;
  oldDefault.visualRescueEnabled = false;
  assert(modeHMigrateV16DefaultPure(oldDefault));
  assert(oldDefault.base.peakMinRaw == 180u && oldDefault.base.peakMaxRaw == 260u);
  assert(oldDefault.base.waitMinMs == 900u && oldDefault.base.waitMaxMs == 3000u);
  assert(oldDefault.visualRescueEnabled);
  assert(oldDefault.hoPolicy == MH_HO_TIERED_1_2);
  assert(oldDefault.ho1ThresholdRaw == 40u && oldDefault.ho2ThresholdRaw == 200u);

  ModeHConfigPure custom = d;
  custom.base.peakMinRaw = 170u;
  custom.base.peakMaxRaw = 230u;
  const ModeHConfigPure customBefore = custom;
  assert(!modeHMigrateV16DefaultPure(custom));
  assert(custom.base.peakMinRaw == customBefore.base.peakMinRaw);
  assert(custom.base.peakMaxRaw == customBefore.base.peakMaxRaw);
  assert(custom.visualRescueEnabled == customBefore.visualRescueEnabled);

  return 0;
  return 0;
}
} // namespace case_mode_h_rev4_pure

// Coverage origin: test_v36f2_mode_h_visual_rescue_pure.cpp
namespace case_mode_h_visual_rescue_pure {
static void primeMoving(ModeHStatePure &state, uint32_t seed) {
  modeHInitPure(state, seed);
  state.base.movingConfirmed = true;
  state.base.motion = MH_MOTION_MOVING;
}

static ModeHBaseStepResultPure step(
    ModeHStatePure &state,
    const ModeHConfigPure &config,
    uint32_t nowMs,
    uint32_t warningEpoch,
    uint32_t warningEnterMs,
    bool warningActive) {
  return modeHStepPure(
      state, config, nowMs, MODE_H_BASE_TORQUE_CENTER_RAW + 20u,
      true, true, true, 1000u,
      warningEpoch, warningEnterMs, warningActive);
}

int run() {
  const uint16_t center = MODE_H_BASE_TORQUE_CENTER_RAW;
  assert(!modeHVisualWarningActivePure(2u));
  assert(modeHVisualWarningActivePure(3u));
  assert(modeHVisualWarningActivePure(5u));
  assert(!modeHVisualWarningActivePure(6u));
  assert(modeHVisualWarningEdgePure(2u, 3u));
  assert(modeHVisualWarningEdgePure(0u, 5u));
  assert(!modeHVisualWarningEdgePure(3u, 4u));
  assert(!modeHVisualWarningEdgePure(5u, 2u));
  assert(!modeHVisualWarningEdgePure(6u, 3u));

  ModeHConfigPure config = modeHDefaultConfigPure();
  assert(config.visualRescueEnabled);
  assert(config.visualRescueDelayMs == 500u);
  assert(modeHConfigValidPure(config));

  // Disabled Rescue consumes the warning edge without disturbing Rev.4.
  config.visualRescueEnabled = false;
  ModeHStatePure state = {};
  primeMoving(state, 0x1001u);
  auto result = step(state, config, 100u, 1u, 100u, true);
  assert(result.phase == MH_WAIT);
  assert(!state.visualRescuePending);
  assert(state.visualRescueCount == 0u);

  // A new <3 -> 3/4/5 epoch waits for the configured delay. At the first
  // eligible stock 0x370 on/after the deadline it enters full PRIMARY output,
  // skipping RAMP_IN regardless of the previous phase.
  config.visualRescueEnabled = true;
  primeMoving(state, 0x1002u);
  result = step(state, config, 100u, 1u, 100u, true);
  assert(result.phase == MH_WAIT && state.visualRescuePending);
  result = step(state, config, 599u, 1u, 100u, true);
  assert(result.phase == MH_WAIT && state.visualRescuePending);
  result = step(state, config, 600u, 1u, 100u, true);
  assert(result.phase == MH_INTERACT);
  assert(!result.carrier && !state.visualRescuePending);
  assert(state.visualRescueCount == 1u);
  assert(state.base.eventCount == 1u);
  const uint16_t peak = state.base.event.peakRaw;
  assert(peak >= config.base.peakMinRaw && peak <= config.base.peakMaxRaw);
  assert(result.raw == (state.base.event.direction < 0 ? center - peak : center + peak));

  // Clearing below 3 before the deadline cancels the pending rescue. The old
  // epoch cannot fire later, even if a stale warning-active flag reappears.
  primeMoving(state, 0x1003u);
  result = step(state, config, 100u, 7u, 100u, true);
  assert(state.visualRescuePending);
  result = step(state, config, 300u, 7u, 100u, false);
  assert(!state.visualRescuePending && state.visualRescueCount == 0u);
  result = step(state, config, 700u, 7u, 100u, true);
  assert(result.phase != MH_INTERACT && state.visualRescueCount == 0u);

  // 3 -> 4 -> 5 keeps one warning epoch. Once fired, it cannot retrigger even
  // if the ordinary event engine later returns to WAIT while the warning stays.
  primeMoving(state, 0x1004u);
  (void)step(state, config, 100u, 11u, 100u, true);
  (void)step(state, config, 600u, 11u, 100u, true);
  assert(state.visualRescueCount == 1u);
  modeHBeginWaitPure(state, config, 700u, false);
  result = step(state, config, 1200u, 11u, 100u, true);
  assert(result.phase == MH_WAIT);
  assert(state.visualRescueCount == 1u);

  // After a clear state, a later <3 -> 3/4/5 transition has a new epoch and
  // can trigger exactly once. Zero delay fires on the first eligible RX.
  result = step(state, config, 1300u, 11u, 100u, false);
  assert(!state.visualRescuePending);
  config.visualRescueDelayMs = 0u;
  result = step(state, config, 1400u, 12u, 1400u, true);
  assert(result.phase == MH_INTERACT);
  assert(state.visualRescueCount == 2u);

  // The deadline comparison remains correct when millis() wraps around.
  config.visualRescueDelayMs = 500u;
  primeMoving(state, 0x1005u);
  result = step(state, config, 0xFFFFFF00u, 13u, 0xFFFFFF00u, true);
  assert(state.visualRescuePending && result.phase == MH_WAIT);
  result = step(state, config, 0x000000F4u, 13u, 0xFFFFFF00u, true);
  assert(result.phase == MH_INTERACT);
  assert(state.visualRescueCount == 1u);

  // Range validation protects the dashboard/NVS boundary.
  config.visualRescueDelayMs = MODE_H_VISUAL_RESCUE_MAX_DELAY_MS;
  assert(modeHConfigValidPure(config));
  config.visualRescueDelayMs++;
  assert(!modeHConfigValidPure(config));

  return 0;
  return 0;
}
} // namespace case_mode_h_visual_rescue_pure

// Coverage origin: test_v37_mode_h_defaults_pure.cpp
namespace case_mode_h_defaults_pure {
static void test_v37_defaults_match_the_approved_profile() {
  assert(nagModeHDefaultVariantPure() == H_VARIANT_REV4);
  const ModeHConfigPure defaults = modeHDefaultConfigPure();
  assert(defaults.base.peakMinRaw == 180u);
  assert(defaults.base.peakMaxRaw == 260u);
  assert(defaults.base.waitMinMs == 900u);
  assert(defaults.base.waitMaxMs == 3000u);
  assert(defaults.base.refractoryMinMs == 500u);
  assert(defaults.base.refractoryMaxMs == 1500u);
  assert(defaults.carrierMinRaw == 10u);
  assert(defaults.carrierMaxRaw == 60u);
  assert(defaults.hoPolicy == MH_HO_TIERED_1_2);
  assert(defaults.ho1ThresholdRaw == 40u);
  assert(defaults.ho2ThresholdRaw == 200u);
  assert(defaults.visualRescueEnabled);
  assert(defaults.visualRescueDelayMs == 500u);
  assert(nagModeHDefaultStopBehaviorPure() == H_STOP_HARD_PAUSE);
}

static void test_exact_v17_default_migrates() {
  ModeHConfigPure old = modeHDefaultConfigPure();
  old.ho2ThresholdRaw = 175u;
  assert(modeHMigrateV17DefaultPure(old));
  assert(old.ho2ThresholdRaw == 200u);
}

static void test_custom_v17_profile_is_preserved() {
  ModeHConfigPure old = modeHDefaultConfigPure();
  old.base.peakMaxRaw = 255u;
  old.ho2ThresholdRaw = 175u;
  assert(!modeHMigrateV17DefaultPure(old));
  assert(old.base.peakMaxRaw == 255u);
  assert(old.ho2ThresholdRaw == 175u);
}

int run() {
  test_v37_defaults_match_the_approved_profile();
  test_exact_v17_default_migrates();
  test_custom_v17_profile_is_preserved();
  return 0;
  return 0;
}
} // namespace case_mode_h_defaults_pure

int main() {
  if (case_mode_h_single_pure::run()) return 1;
  if (case_mode_h_stop_carrier_pure::run()) return 1;
  if (case_mode_h_rev4_pure::run()) return 1;
  if (case_mode_h_visual_rescue_pure::run()) return 1;
  if (case_mode_h_defaults_pure::run()) return 1;
  return 0;
}
