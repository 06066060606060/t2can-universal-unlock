#include <cassert>
#include <cstdint>

#include "auto_blinker_pure.h"

int main() {
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
}
