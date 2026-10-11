// Feature regression suite. Independent cases retain their original helper scopes.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../r79_fixed_policy_pure.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "../vehicle_profile.h"
#include "../summon_state_pure.h"
#include "summon_state_pure.h"
#include "runtime_gate_pure.h"

// Coverage origin: test_r79_ap_gate_pure.cpp
namespace case_r79_ap_gate_pure {
static R79ApGateDecisionPure step(R79ApGateSessionPure &s, const R79ApGateConfigPure &c, uint32_t t, bool fresh = true, bool ap = true) {
  r79ApGateObservePure(s, c, t, fresh, ap);
  return r79ApGateDecisionPure(c, s, t, fresh, ap);
}

int run() {
  R79ApGateSessionPure s = {};
  R79ApGateConfigPure c = {true, 0, 2};
  assert(!step(s, c, 0).allowed); // Missing AP restriction must fail here.
  assert(step(s, c, 20000).reason == 2);
  assert(step(s, c, 20001, true, false).allowed);
  assert(step(s, c, 20002, true, false).reason == 1);
  assert(!step(s, c, 20003, false, false).allowed);
  assert(step(s, c, 20004, false, true).reason == 5);

  // Disabled means legacy behavior even with corrupt configuration or stale DAS.
  c = {false, 255, 0};
  auto d = step(s, c, 0, false, true);
  assert(d.allowed && d.reason == 0 && d.remainingMs == 0);

  // Zero is a real engagement timestamp. Exact boundary releases, never early.
  c = {true, 1, 2};
  s = {};
  d = step(s, c, 0);
  assert(!d.allowed && d.reason == 3 && d.remainingMs == 2000);
  d = step(s, c, 1999);
  assert(!d.allowed && d.remainingMs == 1);
  d = step(s, c, 2000);
  assert(d.allowed && d.reason == 4 && d.remainingMs == 0);
  // Released session stays released even after a whole millis epoch.
  assert(step(s, c, 0).allowed);

  // Disengagement and invalid evidence discard all accumulated waiting time.
  step(s, c, 2100, true, false);
  assert(!s.active);
  assert(step(s, c, 2200).remainingMs == 2000);
  step(s, c, 3000, false);
  assert(!s.active);
  assert(step(s, c, 3500).remainingMs == 2000);
  assert(!step(s, c, 5499).allowed);
  assert(step(s, c, 5500).allowed);
  s = {}; // Integration uses same reset on recovery/configuration changes.
  assert(!r79ApGateDecisionPure(c, s, 8000, true, true).allowed);

  // Unsigned elapsed time handles engagement immediately before wrap.
  s = {};
  assert(step(s, c, UINT32_MAX - 999u).remainingMs == 2000);
  assert(step(s, c, 999).remainingMs == 1);
  assert(step(s, c, 1000).allowed);

  // All supported integer delays and both bounds are enforced.
  for (uint8_t seconds = 2; seconds <= 10; ++seconds) {
    c = {true, 1, seconds}; s = {};
    step(s, c, 100);
    assert(!step(s, c, 100u + seconds * 1000u - 1u).allowed);
    assert(step(s, c, 100u + seconds * 1000u).allowed);
  }
  const R79ApGateConfigPure bad[] = {{true, 2, 2}, {true, 255, 2}, {true, 1, 0}, {true, 1, 1}, {true, 1, 11}, {true, 0, 255}};
  for (const auto &invalid : bad) {
    s = {};
    d = step(s, invalid, 0);
    assert(!d.allowed && d.reason == 5);
    assert(!step(s, invalid, 50000, true, false).allowed);
  }
  puts("R79 AP gate pure: 9 behavior groups passed");
  return 0;
}
} // namespace case_r79_ap_gate_pure

// Coverage origin: test_r79_hw3_policy.cpp
namespace case_r79_hw3_policy {
int run() {
  for (uint8_t model=0;model<=6;++model) for (uint8_t topology=0;topology<=4;++topology) {
    const bool expected=(model==VEHICLE_MODEL_Y_LEGACY || model==VEHICLE_MODEL_3_LEGACY) &&
        (topology==VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS || topology==VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS);
    assert(vehicleProfileR79Hw3Supported(model,topology)==expected);
  }
  for (unsigned byte2=0;byte2<256;++byte2) for (unsigned byte5=0;byte5<256;++byte5) {
    const uint8_t stock[8]={1,0x92,(uint8_t)byte2,0xAC,0x63,(uint8_t)byte5,0x71,0xE8};
    for (bool preserve : {false,true}) {
      for (uint8_t bit18 : {0,1}) {
        uint8_t out[8];std::memcpy(out,stock,8);
        r79FixedApplyBitsPure(out,bit18,preserve);
        for (unsigned i=0;i<8;++i) {
          const uint8_t expected=i==2?(uint8_t)(stock[i] & (bit18?0xF3:0xF7)):
              i==5?(preserve?stock[i]:(uint8_t)(stock[i]|0x80)):stock[i];
          assert(out[i]==expected);
        }
      }
      uint8_t out[8];std::memcpy(out,stock,8);
      const bool changed=r79Mode2ApplyBitsPure(out,preserve);
      assert(changed==((stock[2]&0x08)!=0 || (!preserve && (stock[5]&0x80)==0)));
      for (unsigned i=0;i<8;++i) assert(out[i]==(i==2?(uint8_t)(stock[i]&0xF7):
          i==5?(preserve?stock[i]:(uint8_t)(stock[i]|0x80)):stock[i]));
    }
    uint8_t defaultFixed[8],defaultMode2[8];
    std::memcpy(defaultFixed,stock,8);std::memcpy(defaultMode2,stock,8);
    r79FixedApplyBitsPure(defaultFixed,0);r79Mode2ApplyBitsPure(defaultMode2);
    assert(defaultFixed[5]==(uint8_t)(stock[5]|0x80));
    assert(std::memcmp(defaultFixed,defaultMode2,8)==0);
  }
  r79FixedApplyBitsPure(nullptr,0,true);assert(!r79Mode2ApplyBitsPure(nullptr,true));
  return 0;
}
} // namespace case_r79_hw3_policy

// Coverage origin: test_r79_manual_drive_pure.cpp
namespace case_r79_manual_drive_pure {
// Compile the pre-feature policy so RED measures authorization, not missing API.
template<class T> auto manualDecision(bool ap,bool summon,const T &manual,bool allow,int)
  ->decltype(r79TxDecisionPure(ap,summon,manual,allow)) {
  return r79TxDecisionPure(ap,summon,manual,allow);
}
template<class T> R79TxDecisionPure manualDecision(bool ap,bool summon,const T &manual,bool,long) {
  return r79TxDecisionPure(ap,summon,manual);
}
int run() {
  for(uint8_t gear:{TESLA_GEAR_D,TESLA_GEAR_R}) {
    const R79ManualSuppressionPure manual={true,gear};
    const auto allowed=manualDecision(false,false,manual,true,0);
    assert(allowed.txEnabled&&!allowed.manualSuppressed);
    const auto blocked=manualDecision(false,false,manual,false,0);
    assert(!blocked.txEnabled&&blocked.manualSuppressed);
    assert(blocked.reason==(gear==TESLA_GEAR_R?R79_TX_REASON_MANUAL_R:R79_TX_REASON_MANUAL_D));
    assert(manualDecision(true,false,manual,true,0).reason==R79_TX_REASON_AUTOPILOT);
    assert(manualDecision(false,true,manual,true,0).reason==R79_TX_REASON_SUMMON);
  }
  // Manual permission only changes the latch clause; AP control stays separate.
  R79ApGateConfigPure config={true,0,2,true};R79ApGateSessionPure session={};
  assert(!r79ApGateDecisionPure(config,session,100,true,true).allowed);
  assert(r79ApGateDecisionPure(config,session,100,true,false).allowed);
  assert(!r79ApGateDecisionPure(config,session,100,false,false).allowed);
  config.mode=1;r79ApGateObservePure(session,config,100,true,true);
  assert(!r79ApGateDecisionPure(config,session,2099,true,true).allowed);
  assert(r79ApGateDecisionPure(config,session,2100,true,true).allowed);
  return 0;
}
} // namespace case_r79_manual_drive_pure

// Coverage origin: test_r79_mode2_pure.cpp
namespace case_r79_mode2_pure {
static void assertOnlyMode2BitsChanged(const uint8_t stock[8], const uint8_t out[8]) {
  for (uint8_t byte = 0; byte < 8; ++byte) {
    uint8_t allowed = 0;
    if (byte == 2) allowed = (uint8_t)(1u << 3);  // bit19 only
    if (byte == 5) allowed = (uint8_t)(1u << 7);  // bit47 only
    assert((((uint8_t)(stock[byte] ^ out[byte])) & (uint8_t)~allowed) == 0u);
  }
}

int run() {
  static_assert(R79_MODE_DEFAULT_PURE == R79_MODE_1_PURE,
                "upgrades retain the existing transport");
  static_assert(R79_MODE2_TX_WAIT_MS_PURE == 0u,
                "Mode 2 uses a non-blocking TWAI enqueue");
  static_assert(R79_MODE2_DELAY_DEFAULT_MS_PURE == 150u,
                "optional MUX2 reinjection defaults to +150 ms");
  static_assert(R79_MODE2_DELAY_MAX_MS_PURE == 2000u,
                "dashboard delay range is bounded");

  assert(r79ModeSanitizePure(R79_MODE_1_PURE) == R79_MODE_1_PURE);
  assert(r79ModeSanitizePure(R79_MODE_2_PURE) == R79_MODE_2_PURE);
  assert(r79ModeSanitizePure(0xFFu) == R79_MODE_DEFAULT_PURE);
  assert(r79Mode2DelaySanitizePure(0u) == 0u);
  assert(r79Mode2DelaySanitizePure(2000u) == 2000u);
  assert(r79Mode2DelaySanitizePure(2001u) == R79_MODE2_DELAY_DEFAULT_MS_PURE);

  // Mode 2 preserves stock bit18, clears bit19, and sets bit47. No other bit
  // may move. Exercise both possible stock bit18 values.
  const uint8_t stockSet[8] = {0x01u, 0xA5u, 0x0Cu, 0x33u, 0x55u, 0x00u, 0x77u, 0x99u};
  uint8_t out[8];
  std::memcpy(out, stockSet, sizeof(out));
  assert(r79Mode2ApplyBitsPure(out));
  assert(((out[2] >> 2) & 1u) == 1u);
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyMode2BitsChanged(stockSet, out);

  uint8_t stockClear[8] = {0x01u, 0xA5u, 0x08u, 0x33u, 0x55u, 0x80u, 0x77u, 0x99u};
  std::memcpy(out, stockClear, sizeof(out));
  assert(r79Mode2ApplyBitsPure(out));
  assert(((out[2] >> 2) & 1u) == 0u);
  assertOnlyMode2BitsChanged(stockClear, out);
  assert(!r79Mode2ApplyBitsPure(out));  // already at the Mode 2 target

  // Disabled delayed reinjection never arms.
  R79Mode2DelayedStatePure delayed = {};
  assert(!r79Mode2ObserveStockPure(delayed, 2u, 1000u, false, 150u).armed);
  assert(!delayed.pending);

  // Enabled MUX2 arms exactly one user-selected delayed shot.
  assert(r79Mode2ObserveStockPure(delayed, 2u, 2000u, true, 275u).armed);
  assert(delayed.pending && delayed.dueMs == 2275u);
  assert(r79Mode2DelayedStepPure(delayed, 2274u, true) == R79_MODE2_DELAY_WAIT_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 2275u, true) == R79_MODE2_DELAY_FIRE_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 2276u, true) == R79_MODE2_DELAY_WAIT_PURE);

  // A stock MUX0/MUX1 begins the next cycle and cancels the pending MUX2 shot.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, 3000u, true, 150u);
  const R79Mode2ObserveResultPure cancelled =
      r79Mode2ObserveStockPure(delayed, 1u, 3050u, true, 150u);
  assert(cancelled.cancelled && !delayed.pending);

  // The manual D/R policy is passed in as the common authorization result.
  // A denied delayed slot is consumed once and never retried.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, 4000u, true, 0u);
  assert(r79Mode2DelayedStepPure(delayed, 4000u, false) ==
         R79_MODE2_DELAY_DISALLOWED_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 4001u, true) ==
         R79_MODE2_DELAY_WAIT_PURE);

  // uint32 rollover preserves the configurable deadline comparison.
  delayed = {};
  r79Mode2ObserveStockPure(delayed, 2u, UINT32_MAX - 20u, true, 50u);
  assert(r79Mode2DelayedStepPure(delayed, 28u, true) == R79_MODE2_DELAY_WAIT_PURE);
  assert(r79Mode2DelayedStepPure(delayed, 29u, true) == R79_MODE2_DELAY_FIRE_PURE);
  return 0;
  return 0;
}
} // namespace case_r79_mode2_pure

// Coverage origin: test_r79_policy.cpp
namespace case_r79_policy {
static void expect(bool enabled, uint8_t reason, const R79TxDecisionPure &d) {
  assert(d.txEnabled == enabled);
  assert(d.reason == reason);
  assert(d.manualSuppressed == !enabled);
}

int run() {
  // Tesla OFF/manual states used by the dashboard/runtime must agree. State 2
  // is observed on YL during ordinary D driving and must not bounce R79 ACTIVE.
  for (uint8_t s : {0, 1, 2, 8, 9, 14}) assert(dasStateManualPure(true, s));
  for (uint8_t s : {3, 4, 5, 6}) assert(!dasStateManualPure(true, s));
  assert(!dasStateManualPure(false, 1));

  R79ManualSuppressionPure manual = {};

  // Fresh remote-start evidence prevents entering manual suppression before a
  // Summon session is fully confirmed, but does not itself claim SUMMON.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, false, true,
      false, true);
  assert(!manual.active);
  expect(true, R79_TX_REASON_DEFAULT,
         r79TxDecisionPure(false, false, manual));

  // Definite D + manual DAS with no remote-start evidence latches suppression.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, false, true,
      false, false);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_D);
  expect(false, R79_TX_REASON_MANUAL_D,
         r79TxDecisionPure(false, false, manual));

  // Transient unknown DAS/gear and fresh ACA/SPR evidence must not make a
  // confirmed manual drive bounce ACTIVE/SUSPENDED.
  r79ManualSuppressionUpdatePure(manual,
      false, TESLA_GEAR_INVALID, false, false, false,
      false, true);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_D);
  expect(false, R79_TX_REASON_MANUAL_D,
         r79TxDecisionPure(false, false, manual));

  // D -> R while still manual remains suppressed and updates the reason.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_R, true, false, true,
      false, false);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_R);
  expect(false, R79_TX_REASON_MANUAL_R,
         r79TxDecisionPure(false, false, manual));

  // Confirmed Summon immediately releases manual suppression, including in R.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_R, true, false, true,
      true, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_SUMMON,
         r79TxDecisionPure(false, true, manual));

  // AP active also releases suppression immediately and has display priority
  // over SUMMON if both flags are ever simultaneously present.
  manual.active = true;
  manual.gearRaw = TESLA_GEAR_D;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, true, false,
      true, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_AUTOPILOT,
         r79TxDecisionPure(true, true, manual));

  // Explicit P/N ends a manual-driving latch. Unknown/stale alone does not.
  manual.active = true;
  manual.gearRaw = TESLA_GEAR_D;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_N, true, false, true,
      false, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_DEFAULT,
         r79TxDecisionPure(false, false, manual));

  manual.active = true;
  manual.gearRaw = TESLA_GEAR_R;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_P, true, false, true,
      false, false);
  assert(!manual.active);

  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_AUTOPILOT), "AUTOPILOT") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_SUMMON), "SUMMON") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_MANUAL_D), "MANUAL D") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_MANUAL_R), "MANUAL R") == 0);
  return 0;
  return 0;
}
} // namespace case_r79_policy

// Coverage origin: test_v37_r79_fixed_policy_pure.cpp
namespace case_r79_fixed_policy_pure {
static void assertOnlyPolicyBitsChanged(const uint8_t stock[8], const uint8_t out[8]) {
  for (uint8_t byte = 0u; byte < 8u; ++byte) {
    uint8_t allowed = 0u;
    if (byte == 2u) allowed = (uint8_t)((1u << 2) | (1u << 3));
    if (byte == 5u) allowed = (uint8_t)(1u << 7);
    assert((((uint8_t)(stock[byte] ^ out[byte])) & (uint8_t)~allowed) == 0u);
  }
}

template <typename State>
static auto observeMode1PostMux2(
    State &state, bool enabled, uint16_t delayMs, int)
    -> decltype(r79FixedQuietObserveStockPure(
        state, 2u, 1000u, enabled, delayMs)) {
  return r79FixedQuietObserveStockPure(
      state, 2u, 1000u, enabled, delayMs);
}

template <typename State>
static R79FixedQuietObserveResultPure observeMode1PostMux2(
    State &, bool, uint16_t, long) {
  // Keeps the regression runnable before the new overload exists so RED is
  // an assertion failure rather than an invalid compile.
  R79FixedQuietObserveResultPure missing = {};
  missing.armed = true;
  return missing;
}

template <typename Pending, typename Periodic>
static auto mode1DisableCancelsRetry(Pending pending, Periodic periodic, int)
    -> decltype(r79Mode1DisableCancelsRetryPure(pending, periodic)) {
  return r79Mode1DisableCancelsRetryPure(pending, periodic);
}

template <typename Pending, typename Periodic>
static bool mode1DisableCancelsRetry(Pending, Periodic, long) {
  return true;
}

int run() {
  static_assert(R79_FIXED_FAST_WAIT_MS_PURE == 2u, "Fast Echo waits at most 2 ms");
  static_assert(R79_FIXED_QUIET_DELAY_MS_PURE == 150u, "quiet slot is MUX2 +150 ms");
  static_assert(R79_FIXED_QUIET_HARD_END_MS_PURE == 340u, "quiet slot hard guard");

  // Mode 1 keeps the current 2 ms behavior as the upgrade-safe default while
  // restoring the old zero-wait Fast Echo comparison option.
  assert(r79Mode1WaitModeSanitizePure(R79_MODE1_FAST_ECHO_PURE) ==
         R79_MODE1_FAST_ECHO_PURE);
  assert(r79Mode1WaitModeSanitizePure(R79_MODE1_2MS_WAIT_PURE) ==
         R79_MODE1_2MS_WAIT_PURE);
  assert(r79Mode1WaitModeSanitizePure(99u) == R79_MODE1_WAIT_DEFAULT_PURE);
  assert(r79Mode1TxWaitMsPure(R79_MODE1_FAST_ECHO_PURE) == 0u);
  assert(r79Mode1TxWaitMsPure(R79_MODE1_2MS_WAIT_PURE) == 2u);

  uint16_t parsedDelay = 999u;
  assert(!r79DelayTextParsePure("", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("abc", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("-1", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(!r79DelayTextParsePure("341", R79_FIXED_QUIET_HARD_END_MS_PURE,
                                parsedDelay));
  assert(r79DelayTextParsePure("0", R79_FIXED_QUIET_HARD_END_MS_PURE,
                               parsedDelay));
  assert(parsedDelay == 0u);
  assert(r79DelayTextParsePure("340", R79_FIXED_QUIET_HARD_END_MS_PURE,
                               parsedDelay));
  assert(parsedDelay == 340u);

  const uint8_t stock[8] = {0xA5u, 0x5Au, 0xFFu, 0x12u, 0x34u, 0x00u, 0x78u, 0x9Au};
  uint8_t out[8];

  std::memcpy(out, stock, sizeof(out));
  r79FixedApplyBitsPure(out, R79_BIT18_STOCK_PURE);
  assert(((out[2] >> 2) & 1u) == ((stock[2] >> 2) & 1u));
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyPolicyBitsChanged(stock, out);

  std::memcpy(out, stock, sizeof(out));
  r79FixedApplyBitsPure(out, R79_BIT18_FORCE_0_PURE);
  assert(((out[2] >> 2) & 1u) == 0u);
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);
  assertOnlyPolicyBitsChanged(stock, out);
  assert(r79Bit18PolicySanitizePure(99u) == R79_BIT18_DEFAULT_PURE);
  assert(R79_BIT18_DEFAULT_PURE == R79_BIT18_STOCK_PURE);

  // One MUX2 anchor produces exactly one shot at +150 ms.
  R79FixedQuietStatePure quiet = {};
  assert(!r79FixedQuietObserveStockPure(quiet, 1u, 1000u).armed);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 1050u).armed);
  assert(r79FixedQuietStepPure(quiet, 1199u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 1200u, true) == R79_FIXED_QUIET_FIRE_PURE);
  assert(r79FixedQuietStepPure(quiet, 1201u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Mode 1 Post-MUX2 remains upgrade-safe ON, but an explicit OFF prevents a
  // new reservation and clears an already pending reservation immediately.
  quiet = {};
  assert(observeMode1PostMux2(quiet, true, 150u, 0).armed);
  const R79FixedQuietObserveResultPure disabled =
      observeMode1PostMux2(quiet, false, 150u, 0);
  assert(!disabled.armed);
  assert(disabled.cancelled);
  assert(!quiet.pending);
  assert(!mode1DisableCancelsRetry(true, false, 0));
  assert(mode1DisableCancelsRetry(true, true, 0));
  assert(!mode1DisableCancelsRetry(false, true, 0));

  // A user-selected Mode 1 Post-MUX2 delay changes the actual one-shot
  // deadline. Values outside the existing 340 ms safety window fall back to
  // the established +150 ms default.
  quiet = {};
  assert(r79FixedQuietDelaySanitizePure(0u) == 0u);
  assert(r79FixedQuietDelaySanitizePure(340u) == 340u);
  assert(r79FixedQuietDelaySanitizePure(341u) == R79_FIXED_QUIET_DELAY_MS_PURE);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 1300u, 275u).armed);
  assert(quiet.dueMs == 1575u);
  assert(r79FixedQuietStepPure(quiet, 1574u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 1575u, true) == R79_FIXED_QUIET_FIRE_PURE);

  // The complete selectable range is schedulable. At the hard endpoint the
  // enqueue budget becomes zero so the sender cannot block past the window.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2000u, 0u).armed);
  assert(r79FixedQuietStepPure(quiet, 2000u, true) == R79_FIXED_QUIET_FIRE_PURE);
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 3000u, 340u).armed);
  assert(r79FixedQuietStepPure(quiet, 3339u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 3340u, true) == R79_FIXED_QUIET_FIRE_PURE);
  const uint32_t hardDeadline = r79FixedQuietHardDeadlinePure(quiet);
  assert(hardDeadline == 3340u);
  assert(r79FixedDeadlineWaitBudgetPure(3338u, hardDeadline, 2u) == 2u);
  assert(r79FixedDeadlineWaitBudgetPure(3339u, hardDeadline, 2u) == 1u);
  assert(r79FixedDeadlineWaitBudgetPure(3340u, hardDeadline, 2u) == 0u);
  assert(!r79FixedDeadlineExpiredPure(3340u, hardDeadline));
  assert(r79FixedDeadlineExpiredPure(3341u, hardDeadline));
  assert(r79FixedDeadlineWaitBudgetPure(UINT32_MAX - 1u, 2u, 2u) == 2u);
  assert(!r79FixedDeadlineExpiredPure(2u, 2u));
  assert(r79FixedDeadlineExpiredPure(3u, 2u));
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 4000u, 340u);
  assert(r79FixedQuietStepPure(quiet, 4341u, true) ==
         R79_FIXED_QUIET_GUARD_SKIP_PURE);

  // A newer MUX2 re-anchors the pending slot.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2000u).armed);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 2100u).armed);
  assert(r79FixedQuietStepPure(quiet, 2249u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 2250u, true) == R79_FIXED_QUIET_FIRE_PURE);

  // A new stock MUX0/MUX1 cycle cancels the prior MUX2 slot.
  quiet = {};
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 3000u).armed);
  const R79FixedQuietObserveResultPure cancelled =
      r79FixedQuietObserveStockPure(quiet, 0u, 3060u);
  assert(cancelled.cancelled && !cancelled.armed);
  assert(r79FixedQuietStepPure(quiet, 3150u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Authorization failure consumes the one slot rather than retrying forever.
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 4000u);
  assert(r79FixedQuietStepPure(quiet, 4150u, false) == R79_FIXED_QUIET_DISALLOWED_SKIP_PURE);
  assert(r79FixedQuietStepPure(quiet, 4151u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // Delayed servicing beyond the hard window drops the slot.
  quiet = {};
  r79FixedQuietObserveStockPure(quiet, 2u, 5000u);
  assert(r79FixedQuietStepPure(quiet, 5341u, true) == R79_FIXED_QUIET_GUARD_SKIP_PURE);
  assert(r79FixedQuietStepPure(quiet, 5342u, true) == R79_FIXED_QUIET_WAIT_PURE);

  // uint32_t rollover keeps both due and hard-window comparisons correct.
  quiet = {};
  const uint32_t nearWrap = UINT32_MAX - 100u;
  r79FixedQuietObserveStockPure(quiet, 2u, nearWrap);
  assert(r79FixedQuietStepPure(quiet, 48u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 49u, true) == R79_FIXED_QUIET_FIRE_PURE);

  return 0;
  return 0;
}
} // namespace case_r79_fixed_policy_pure

// Coverage origin: test_v37_r79_safety_pure.cpp
namespace case_r79_safety_pure {
int run() {
  assert(summonTxPriorityStatePure(false, TESLA_GEAR_INVALID, false, false) ==
         SUMMON_PRIORITY_NORMAL);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_D, false, false) ==
         SUMMON_PRIORITY_NORMAL);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_P, false, false) ==
         SUMMON_PRIORITY_PARK_STANDBY);
  assert(summonTxPriorityStatePure(true, TESLA_GEAR_P, true, false) ==
         SUMMON_PRIORITY_READY);
  assert(summonTxPriorityStatePure(false, TESLA_GEAR_INVALID, false, true) ==
         SUMMON_PRIORITY_ACTIVE);

  assert(!summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_NORMAL));
  assert(!summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_PARK_STANDBY));
  assert(summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_READY));
  assert(summonPriorityAllowsR79FlushPure(SUMMON_PRIORITY_ACTIVE));

  assert(summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_PARK_STANDBY, 13u, 14u, 6u));
  assert(!summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_PARK_STANDBY, 14u, 14u, 6u));
  assert(summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_READY, 5u, 14u, 6u));
  assert(!summonPriorityNonR79AdmissionPure(
      SUMMON_PRIORITY_READY, 6u, 14u, 6u));

  assert(r79RetryDelayMsPure(0u) == 5u);
  assert(r79RetryDelayMsPure(1u) == 15u);
  assert(r79RetryDelayMsPure(2u) == 30u);
  assert(r79RetryDelayMsPure(3u) == 0u);
  return 0;
  return 0;
}
} // namespace case_r79_safety_pure

int main() {
  if (case_r79_ap_gate_pure::run()) return 1;
  if (case_r79_hw3_policy::run()) return 1;
  if (case_r79_manual_drive_pure::run()) return 1;
  if (case_r79_mode2_pure::run()) return 1;
  if (case_r79_policy::run()) return 1;
  if (case_r79_fixed_policy_pure::run()) return 1;
  if (case_r79_safety_pure::run()) return 1;
  return 0;
}
