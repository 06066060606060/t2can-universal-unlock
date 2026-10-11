// Feature regression suite. Independent cases retain their original helper scopes.
#include <cassert>
#include <cstdint>
#include "../auto_blinker_pure.h"
#include "auto_blinker_pure.h"

// Coverage origin: test_blinker_tx_policy_pure.cpp
namespace case_blinker_tx_policy_pure {
int run() {
  BlinkerTxRequestStatePure state = {};
  assert(blinkerTxArmPure(state, 1, BLINKER_TX_SOURCE_AUTO_PURE, 77, 1000));
  assert(!blinkerTxArmPure(state, 1, BLINKER_TX_SOURCE_AUTO_PURE, 77, 1010));

  const BlinkerTxConsumeResultPure first =
      blinkerTxConsumeStockPure(state, 1050, true, 250);
  assert(first.transmit && first.dir == 1);
  assert(first.source == BLINKER_TX_SOURCE_AUTO_PURE);
  assert(!blinkerTxConsumeStockPure(state, 1070, true, 250).transmit);

  assert(blinkerTxArmPure(state, 2, BLINKER_TX_SOURCE_S3XY_PURE, 9, 2000));
  assert(!blinkerTxConsumeStockPure(state, 2050, false, 250).transmit);
  assert(!blinkerTxConsumeStockPure(state, 2060, true, 250).transmit);

  assert(!blinkerTxArmPure(state, 0, BLINKER_TX_SOURCE_AUTO_PURE, 1, 3000));
  assert(!blinkerTxArmPure(state, 1, BLINKER_TX_SOURCE_NONE_PURE, 1, 3000));
  assert(blinkerTxArmPure(
      state, 1, BLINKER_TX_SOURCE_AUTO_PURE, 78, UINT32_MAX - 10));
  const BlinkerTxConsumeResultPure rollover =
      blinkerTxConsumeStockPure(state, 5, true, 250);
  assert(rollover.transmit && rollover.dir == 1);

  assert(blinkerTxArmPure(state, 1, BLINKER_TX_SOURCE_AUTO_PURE, 79, 4000));
  assert(blinkerTxArmPure(state, 2, BLINKER_TX_SOURCE_S3XY_PURE, 10, 4010));
  const BlinkerTxConsumeResultPure replacement =
      blinkerTxConsumeStockPure(state, 4020, true, 250);
  assert(replacement.transmit && replacement.dir == 2);
  assert(replacement.source == BLINKER_TX_SOURCE_S3XY_PURE);

  assert(blinkerTxDefaultModePure(true) == BLINKER_TX_MODE_SINGLE_PURE);
  assert(blinkerTxDefaultModePure(false) == BLINKER_TX_MODE_LEGACY_PURE);
  assert(blinkerTxStoredModePure(BLINKER_TX_MODE_SINGLE_PURE, false) ==
         BLINKER_TX_MODE_SINGLE_PURE);
  assert(blinkerTxStoredModePure(BLINKER_TX_MODE_LEGACY_PURE, true) ==
         BLINKER_TX_MODE_LEGACY_PURE);
  assert(blinkerTxStoredModePure(2, true) == BLINKER_TX_MODE_SINGLE_PURE);
  assert(blinkerTxStoredModePure(2, false) == BLINKER_TX_MODE_LEGACY_PURE);

  const BlinkerTxModeTransitionPure toSingle = blinkerTxModeTransitionPure(
      BLINKER_TX_MODE_LEGACY_PURE, BLINKER_TX_MODE_SINGLE_PURE);
  assert(toSingle.valid && toSingle.cancelLegacy);
  assert(toSingle.effectiveMode == BLINKER_TX_MODE_SINGLE_PURE);
  const BlinkerTxModeTransitionPure remainBurst = blinkerTxModeTransitionPure(
      BLINKER_TX_MODE_LEGACY_PURE, BLINKER_TX_MODE_LEGACY_PURE);
  assert(remainBurst.valid && !remainBurst.cancelLegacy);
  assert(!blinkerTxModeTransitionPure(
      BLINKER_TX_MODE_SINGLE_PURE, 2).valid);
  assert(!blinkerTxModeTransitionPure(
      2, BLINKER_TX_MODE_SINGLE_PURE).valid);

  // Planner pause timing is independent from a direct S3XY request already
  // queued for the next stock frame.
  assert(blinkerTxArmPure(
      state, 2, BLINKER_TX_SOURCE_S3XY_PURE, 11, 5000));
  AutoBlinkerCancelPauseStatePure pause = {};
  assert(autoBlinkerCancelTogglePure(pause, 5000, true, 20) ==
         AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
  const BlinkerTxConsumeResultPure directWhilePaused =
      blinkerTxConsumeStockPure(state, 5010, true, 250);
  assert(directWhilePaused.transmit && directWhilePaused.dir == 2);
  assert(directWhilePaused.source == BLINKER_TX_SOURCE_S3XY_PURE);

  // A stalkless Single TX overlays exactly one live mux1 frame. The next
  // unmodified stock frame supplies the natural OFF state.
  const uint8_t stalklessStock[8] = {
      0xA1, 0x12, 0x34, 0x5A, 0x56, 0x58, 0x9A, 0xBC};
  uint8_t stalklessLeft[8] = {};
  uint8_t stalklessRight[8] = {};
  for (uint8_t i = 0; i < 8; ++i) {
    stalklessLeft[i] = stalklessStock[i];
    stalklessRight[i] = stalklessStock[i];
  }
  vcleftSetLeftButtonPure(stalklessLeft, 2u);
  vcleftSetRightButtonPure(stalklessRight, 2u);
  const uint8_t expectedLeft[8] = {
      0xA1, 0x12, 0x34, 0x9A, 0x56, 0x58, 0x9A, 0xBC};
  const uint8_t expectedRight[8] = {
      0xA1, 0x12, 0x34, 0x5A, 0x56, 0x68, 0x9A, 0xBC};
  for (uint8_t i = 0; i < 8; ++i) {
    assert(stalklessLeft[i] == expectedLeft[i]);
    assert(stalklessRight[i] == expectedRight[i]);
  }
  assert(vcleftLeftButtonPure(stalklessStock) == 1u);
  assert(vcleftRightButtonPure(stalklessStock) == 1u);

  return 0;
  return 0;
}
} // namespace case_blinker_tx_policy_pure

// Coverage origin: test_stalk_frame_pure.cpp
namespace case_stalk_frame_pure {
int run() {
  const uint8_t original[4] = {0xAA, 0xB3, 0xC4, 0x5D};
  const uint8_t stock[4] = {0xAA, 0xB3, 0xC4, 0x5D};
  uint8_t out[4] = {};

  assert(stalkFramePreparePure(stock, 4, 5, 6, out));
  const uint8_t expectedLeft[4] = {0xC8, 0xB5, 0xC6, 0x5D};
  for (uint8_t i = 0; i < 4; ++i) assert(out[i] == expectedLeft[i]);

  assert(stalkFramePreparePure(stock, 4, 5, 2, out));
  const uint8_t expectedRight[4] = {0xF0, 0xB5, 0xC2, 0x5D};
  for (uint8_t i = 0; i < 4; ++i) assert(out[i] == expectedRight[i]);

  assert(!stalkFramePreparePure(nullptr, 4, 5, 6, out));
  assert(!stalkFramePreparePure(stock, 3, 5, 6, out));
  assert(!stalkFramePreparePure(stock, 4, 5, 6, nullptr));
  for (uint8_t i = 0; i < 4; ++i) assert(stock[i] == original[i]);
  return 0;
  return 0;
}
} // namespace case_stalk_frame_pure

// Coverage origin: test_v37_auto_blinker_timers_pure.cpp
namespace case_auto_blinker_timers_pure {
int run() {
  static_assert(BLINKA_NOA_STABILIZE_MIN_S_PURE == 1u, "NOA minimum");
  static_assert(BLINKA_NOA_STABILIZE_MAX_S_PURE == 20u, "NOA maximum");
  static_assert(BLINKA_NOA_STABILIZE_DEFAULT_S_PURE == 10u, "NOA default");
  static_assert(BLINKA_NOA_EXIT_CONFIRM_MS_PURE == 2000u,
                "NOA exit confirmation");
  static_assert(BLINKA_CANCEL_PAUSE_MIN_S_PURE == 10u, "pause minimum");
  static_assert(BLINKA_CANCEL_PAUSE_MAX_S_PURE == 100u, "pause maximum");
  static_assert(BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE == 20u, "pause default");

  assert(autoBlinkerNoaStabilizationSecondsSanitizePure(0u) == 10u);
  assert(autoBlinkerNoaStabilizationSecondsSanitizePure(1u) == 1u);
  assert(autoBlinkerNoaStabilizationSecondsSanitizePure(20u) == 20u);
  assert(autoBlinkerNoaStabilizationSecondsSanitizePure(21u) == 10u);
  assert(autoBlinkerCancelPauseSecondsSanitizePure(9u) == 20u);
  assert(autoBlinkerCancelPauseSecondsSanitizePure(10u) == 10u);
  assert(autoBlinkerCancelPauseSecondsSanitizePure(100u) == 100u);
  assert(autoBlinkerCancelPauseSecondsSanitizePure(101u) == 20u);

  AutoBlinkerNoaSessionStatePure noa = {};
  autoBlinkerObserveNoaPure(noa, 1000u, true, true, 10u);
  assert(autoBlinkerNoaPhasePure(noa, 1000u) ==
         AUTO_BLINKER_NOA_STABILIZING_PURE);
  assert(!autoBlinkerNoaReadyPure(noa, 10999u));
  assert(autoBlinkerNoaReadyPure(noa, 11000u));
  assert(autoBlinkerNoaPhasePure(noa, 11000u) ==
         AUTO_BLINKER_NOA_READY_PURE);
  autoBlinkerObserveNoaPure(noa, 10500u, true, true, 10u);
  assert(noa.readyAtMs == 11000u);  // repeated observations do not extend it

  // A sub-two-second NOA dropout preserves the original session. TX remains
  // blocked while the raw signal is absent, but READY resumes without another
  // stabilization interval when NOA returns.
  autoBlinkerObserveNoaPure(noa, 12000u, true, false, 10u);
  assert(autoBlinkerNoaPhasePure(noa, 12000u) ==
         AUTO_BLINKER_NOA_EXIT_WAIT_PURE);
  assert(!autoBlinkerNoaReadyPure(noa, 12000u));
  assert(autoBlinkerNoaExitRemainingMsPure(noa, 12000u) == 2000u);
  assert(autoBlinkerNoaExitRemainingMsPure(noa, 13999u) == 1u);
  autoBlinkerObserveNoaPure(noa, 13999u, true, true, 10u);
  assert(autoBlinkerNoaReadyPure(noa, 13999u));

  // Two full seconds outside NOA closes the session. A later NOA observation
  // must start a completely new stabilization interval.
  autoBlinkerObserveNoaPure(noa, 15000u, true, false, 10u);
  autoBlinkerObserveNoaPure(noa, 16999u, true, false, 10u);
  assert(autoBlinkerNoaPhasePure(noa, 16999u) ==
         AUTO_BLINKER_NOA_EXIT_WAIT_PURE);
  autoBlinkerObserveNoaPure(noa, 17000u, true, false, 10u);
  assert(autoBlinkerNoaPhasePure(noa, 17000u) ==
         AUTO_BLINKER_NOA_INACTIVE_PURE);
  autoBlinkerObserveNoaPure(noa, 18000u, true, true, 10u);
  assert(!autoBlinkerNoaReadyPure(noa, 27999u));
  assert(autoBlinkerNoaReadyPure(noa, 28000u));

  // A short dropout while still stabilizing keeps the original deadline.
  AutoBlinkerNoaSessionStatePure shortDrop = {};
  autoBlinkerObserveNoaPure(shortDrop, 1000u, true, true, 10u);
  autoBlinkerObserveNoaPure(shortDrop, 4000u, true, false, 10u);
  autoBlinkerObserveNoaPure(shortDrop, 5500u, true, true, 10u);
  assert(shortDrop.readyAtMs == 11000u);
  assert(!autoBlinkerNoaReadyPure(shortDrop, 10999u));
  assert(autoBlinkerNoaReadyPure(shortDrop, 11000u));

  // Dashboard changes replace an in-progress deadline from the update time.
  AutoBlinkerNoaSessionStatePure reconfigured = {};
  autoBlinkerObserveNoaPure(reconfigured, 0u, true, true, 10u);
  assert(autoBlinkerNoaReconfigurePure(reconfigured, 4000u, 3u));
  assert(reconfigured.enteredAtMs == 4000u);
  assert(reconfigured.readyAtMs == 7000u);
  assert(!autoBlinkerNoaReadyPure(reconfigured, 6999u));
  assert(autoBlinkerNoaReadyPure(reconfigured, 7000u));

  // Once READY, a setting edit is stored for the next NOA session and cannot
  // close the current gate again.
  assert(!autoBlinkerNoaReconfigurePure(reconfigured, 8000u, 20u));
  assert(reconfigured.readyAtMs == 7000u);
  assert(autoBlinkerNoaReadyPure(reconfigured, 8000u));

  // Invalid DAS state fails closed immediately rather than using the exit
  // debounce intended only for a valid, explicit non-NOA observation.
  autoBlinkerObserveNoaPure(reconfigured, 9000u, false, true, 20u);
  assert(autoBlinkerNoaPhasePure(reconfigured, 9000u) ==
         AUTO_BLINKER_NOA_INACTIVE_PURE);

  AutoBlinkerCancelPauseStatePure pause = {};
  assert(autoBlinkerCancelTogglePure(pause, 20000u, false, 20u) ==
         AUTO_BLINKER_CANCEL_REJECTED_PURE);
  assert(!autoBlinkerPauseActivePure(pause, 20000u));
  assert(autoBlinkerCancelTogglePure(pause, 20000u, true, 20u) ==
         AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
  assert(autoBlinkerPauseActivePure(pause, 20999u));
  assert(autoBlinkerCancelTogglePure(pause, 21000u, false, 20u) ==
         AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE);
  assert(!autoBlinkerPauseActivePure(pause, 21000u));

  assert(autoBlinkerCancelTogglePure(pause, 30000u, true, 10u) ==
         AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
  assert(autoBlinkerPauseActivePure(pause, 39999u));
  assert(!autoBlinkerPauseActivePure(pause, 40000u));
  assert(!pause.active);

  AutoBlinkerNoaSessionStatePure rolloverNoa = {};
  AutoBlinkerCancelPauseStatePure rolloverPause = {};
  const uint32_t nearWrap = 0xFFFFFF00u;
  autoBlinkerObserveNoaPure(rolloverNoa, nearWrap, true, true, 1u);
  assert(!autoBlinkerNoaReadyPure(rolloverNoa, nearWrap + 999u));
  assert(autoBlinkerNoaReadyPure(rolloverNoa, nearWrap + 1000u));
  assert(autoBlinkerCancelTogglePure(rolloverPause, nearWrap, true, 10u) ==
         AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
  assert(autoBlinkerPauseActivePure(rolloverPause, nearWrap + 9999u));
  assert(!autoBlinkerPauseActivePure(rolloverPause, nearWrap + 10000u));

  autoBlinkerObserveNoaPure(noa, 50000u, true, true, 1u);
  assert(autoBlinkerCancelTogglePure(pause, 50000u, true, 10u) ==
         AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
  autoBlinkerNoaSessionResetPure(noa);
  assert(!noa.sessionActive);
  assert(!autoBlinkerNoaReadyPure(noa, 60000u));
  // Resetting the NOA session does not alter the independent cancel pause.
  assert(autoBlinkerPauseActivePure(pause, 50001u));
  autoBlinkerCancelPauseResetPure(pause);
  assert(!autoBlinkerPauseActivePure(pause, 60000u));

  return 0;
  return 0;
}
} // namespace case_auto_blinker_timers_pure

int main() {
  if (case_blinker_tx_policy_pure::run()) return 1;
  if (case_stalk_frame_pure::run()) return 1;
  if (case_auto_blinker_timers_pure::run()) return 1;
  return 0;
}
