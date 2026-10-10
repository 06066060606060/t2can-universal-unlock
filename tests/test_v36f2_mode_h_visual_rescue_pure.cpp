#include <cassert>
#include <cstdint>
#include "../nag_human_v4_pure.h"

static void primeMoving(NagHumanV4StatePure &state, uint32_t seed) {
  nagHumanV4InitPure(state, seed);
  state.base.movingConfirmed = true;
  state.base.motion = H1_MOTION_MOVING;
}

static NagHumanV1StepResultPure step(
    NagHumanV4StatePure &state,
    const NagHumanV4ConfigPure &config,
    uint32_t nowMs,
    uint32_t warningEpoch,
    uint32_t warningEnterMs,
    bool warningActive) {
  return nagHumanV4StepPure(
      state, config, nowMs, NAG_HUMAN_V1_TORQUE_CENTER_RAW + 20u,
      true, true, true, 1000u,
      warningEpoch, warningEnterMs, warningActive);
}

int main() {
  const uint16_t center = NAG_HUMAN_V1_TORQUE_CENTER_RAW;
  assert(!nagHumanV4VisualWarningActivePure(2u));
  assert(nagHumanV4VisualWarningActivePure(3u));
  assert(nagHumanV4VisualWarningActivePure(5u));
  assert(!nagHumanV4VisualWarningActivePure(6u));
  assert(nagHumanV4VisualWarningEdgePure(2u, 3u));
  assert(nagHumanV4VisualWarningEdgePure(0u, 5u));
  assert(!nagHumanV4VisualWarningEdgePure(3u, 4u));
  assert(!nagHumanV4VisualWarningEdgePure(5u, 2u));
  assert(!nagHumanV4VisualWarningEdgePure(6u, 3u));

  NagHumanV4ConfigPure config = nagHumanV4DefaultConfigPure();
  assert(config.visualRescueEnabled);
  assert(config.visualRescueDelayMs == 500u);
  assert(nagHumanV4ConfigValidPure(config));

  // Disabled Rescue consumes the warning edge without disturbing Rev.4.
  config.visualRescueEnabled = false;
  NagHumanV4StatePure state = {};
  primeMoving(state, 0x1001u);
  auto result = step(state, config, 100u, 1u, 100u, true);
  assert(result.phase == H1_WAIT);
  assert(!state.visualRescuePending);
  assert(state.visualRescueCount == 0u);

  // A new <3 -> 3/4/5 epoch waits for the configured delay. At the first
  // eligible stock 0x370 on/after the deadline it enters full PRIMARY output,
  // skipping RAMP_IN regardless of the previous phase.
  config.visualRescueEnabled = true;
  primeMoving(state, 0x1002u);
  result = step(state, config, 100u, 1u, 100u, true);
  assert(result.phase == H1_WAIT && state.visualRescuePending);
  result = step(state, config, 599u, 1u, 100u, true);
  assert(result.phase == H1_WAIT && state.visualRescuePending);
  result = step(state, config, 600u, 1u, 100u, true);
  assert(result.phase == H1_INTERACT);
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
  assert(result.phase != H1_INTERACT && state.visualRescueCount == 0u);

  // 3 -> 4 -> 5 keeps one warning epoch. Once fired, it cannot retrigger even
  // if the ordinary event engine later returns to WAIT while the warning stays.
  primeMoving(state, 0x1004u);
  (void)step(state, config, 100u, 11u, 100u, true);
  (void)step(state, config, 600u, 11u, 100u, true);
  assert(state.visualRescueCount == 1u);
  nagHumanV4BeginWaitPure(state, config, 700u, false);
  result = step(state, config, 1200u, 11u, 100u, true);
  assert(result.phase == H1_WAIT);
  assert(state.visualRescueCount == 1u);

  // After a clear state, a later <3 -> 3/4/5 transition has a new epoch and
  // can trigger exactly once. Zero delay fires on the first eligible RX.
  result = step(state, config, 1300u, 11u, 100u, false);
  assert(!state.visualRescuePending);
  config.visualRescueDelayMs = 0u;
  result = step(state, config, 1400u, 12u, 1400u, true);
  assert(result.phase == H1_INTERACT);
  assert(state.visualRescueCount == 2u);

  // The deadline comparison remains correct when millis() wraps around.
  config.visualRescueDelayMs = 500u;
  primeMoving(state, 0x1005u);
  result = step(state, config, 0xFFFFFF00u, 13u, 0xFFFFFF00u, true);
  assert(state.visualRescuePending && result.phase == H1_WAIT);
  result = step(state, config, 0x000000F4u, 13u, 0xFFFFFF00u, true);
  assert(result.phase == H1_INTERACT);
  assert(state.visualRescueCount == 1u);

  // Range validation protects the dashboard/NVS boundary.
  config.visualRescueDelayMs = NAG_HUMAN_V4_VISUAL_RESCUE_MAX_DELAY_MS;
  assert(nagHumanV4ConfigValidPure(config));
  config.visualRescueDelayMs++;
  assert(!nagHumanV4ConfigValidPure(config));

  return 0;
}
