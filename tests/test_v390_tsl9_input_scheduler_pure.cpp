#include <assert.h>
#include <stdint.h>

#include "../tsl9_input_scheduler_pure.h"

static Tsl9InputInputsPure inputs(uint32_t now, bool enabled,
                                  bool warning, uint32_t random = 0u) {
  Tsl9InputInputsPure in = {};
  in.nowMs = now;
  in.enabled = enabled;
  in.warningActive = warning;
  in.txAllowed = true;
  in.templateFresh = true;
  in.mode = TSL9_INPUT_MODE_LEFT_VOLUME_PURE;
  in.randomValue = random;
  return in;
}

int main() {
  static_assert(TSL9_INPUT_MODE_DEFAULT_PURE ==
                    TSL9_INPUT_MODE_LEFT_VOLUME_PURE,
                "left volume must be the default");
  static_assert(TSL9_INPUT_STEP_MS_PURE == 100u, "v8.2 step cadence");
  static_assert(TSL9_INPUT_RETRY_MS_PURE == 250u, "v8.2 retry cadence");
  static_assert(TSL9_INPUT_REPEAT_MIN_MS_PURE == 2000u, "repeat floor");
  static_assert(TSL9_INPUT_REPEAT_MAX_MS_PURE == 3000u, "repeat ceiling");

  assert(tsl9InputRandomRepeatMsPure(0u) == 2000u);
  assert(tsl9InputRandomRepeatMsPure(10u) == 3000u);
  assert(tsl9InputRandomRepeatMsPure(11u) == 2000u);
  assert(tsl9InputDecodeSignedSixBitPure(0x01u) == 1);
  assert(tsl9InputDecodeSignedSixBitPure(0x3Fu) == -1);
  assert(tsl9InputDecodeSignedSixBitPure(0x00u) == 0);
  assert(tsl9InputWarningStatePure(3u));
  assert(tsl9InputWarningStatePure(6u));
  assert(tsl9InputWarningStatePure(9u));
  assert(tsl9InputWarningStatePure(10u));
  assert(!tsl9InputWarningStatePure(2u));
  assert(!tsl9InputWarningStatePure(11u));

  // No ordinary/background action is ever scheduled.
  Tsl9InputSchedulerPure idle;
  assert(idle.service(inputs(1000u, true, false)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  assert(idle.service(inputs(5000u, true, false)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  Tsl9InputCommandPure c = {};
  Tsl9InputCommandPure cleanup = {};

  // Opt-in periodic mode waits for the configured interval and uses the
  // timer-owned 100 ms cadence rather than stock-frame arrival cadence.
  Tsl9InputSchedulerPure periodic;
  Tsl9InputInputsPure periodicIn = inputs(1000u, true, false);
  periodicIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  periodicIn.periodicIntervalSeconds = 30u;
  assert(periodic.service(periodicIn).kind == TSL9_INPUT_COMMAND_NONE_PURE);
  periodicIn.nowMs = 30999u;
  assert(periodic.service(periodicIn).kind == TSL9_INPUT_COMMAND_NONE_PURE);
  periodicIn.nowMs = 31000u;
  c = periodic.service(periodicIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);
  periodic.onCommandResult(c, true, 31000u, 0u);
  periodicIn.nowMs = 31100u;
  c = periodic.service(periodicIn);
  assert(c.step == 1u && c.rightTick == 0);

  // Torque policy supports the compact +1/-1 waveform. A warning edge fires
  // immediately, then returns to the selected periodic interval instead of
  // using TSL9's legacy 2-3 second warning repeat.
  Tsl9InputSchedulerPure torquePair;
  Tsl9InputInputsPure torqueIn = inputs(50000u, true, true);
  torqueIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  torqueIn.periodicIntervalSeconds = 30u;
  torqueIn.sequencePattern = TSL9_INPUT_PATTERN_PAIR_PURE;
  torqueIn.warningEdgeOnly = true;
  c = torquePair.service(torqueIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);
  torquePair.onCommandResult(c, true, 50000u, 0u);
  torqueIn.nowMs = 50100u;
  c = torquePair.service(torqueIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == -1);
  torquePair.onCommandResult(c, true, 50100u, 0u);
  torqueIn.nowMs = 53100u;
  assert(torquePair.service(torqueIn).kind == TSL9_INPUT_COMMAND_NONE_PURE);
  torqueIn.nowMs = 80099u;
  assert(torquePair.service(torqueIn).kind == TSL9_INPUT_COMMAND_NONE_PURE);
  torqueIn.nowMs = 80100u;
  c = torquePair.service(torqueIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);

  // A Torque warning arriving during an active periodic pair is retained as
  // an edge event and runs immediately after the in-flight pair completes,
  // even if the visual warning clears before then.
  Tsl9InputSchedulerPure torqueEdgeDuringPeriodic;
  Tsl9InputInputsPure edgeIn = inputs(200000u, true, false);
  edgeIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  edgeIn.periodicIntervalSeconds = 30u;
  edgeIn.sequencePattern = TSL9_INPUT_PATTERN_PAIR_PURE;
  edgeIn.warningEdgeOnly = true;
  assert(torqueEdgeDuringPeriodic.service(edgeIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  edgeIn.nowMs = 230000u;
  c = torqueEdgeDuringPeriodic.service(edgeIn);
  assert(c.rightTick == 1);
  torqueEdgeDuringPeriodic.onCommandResult(c, true, 230000u, 0u);
  edgeIn.nowMs = 230050u;
  edgeIn.warningActive = true;
  assert(torqueEdgeDuringPeriodic.service(edgeIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  edgeIn.nowMs = 230060u;
  edgeIn.warningActive = false;
  assert(torqueEdgeDuringPeriodic.service(edgeIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  edgeIn.nowMs = 230100u;
  c = torqueEdgeDuringPeriodic.service(edgeIn);
  assert(c.rightTick == -1);
  torqueEdgeDuringPeriodic.onCommandResult(c, true, 230100u, 0u);
  edgeIn.nowMs = 230101u;
  c = torqueEdgeDuringPeriodic.service(edgeIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);

  // TSL9 warning-owned repeat work is discarded when the warning clears;
  // the next action returns to the full configured periodic interval.
  Tsl9InputSchedulerPure warningClearPeriodic;
  Tsl9InputInputsPure clearIn = inputs(300000u, true, true);
  clearIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  clearIn.periodicIntervalSeconds = 30u;
  c = warningClearPeriodic.service(clearIn);
  for (uint8_t step = 0u; step < 4u; ++step) {
    if (step != 0u) {
      clearIn.nowMs += 100u;
      c = warningClearPeriodic.service(clearIn);
    }
    assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == step);
    warningClearPeriodic.onCommandResult(c, true, clearIn.nowMs, 0u);
  }
  clearIn.nowMs = 300400u;
  clearIn.warningActive = false;
  assert(warningClearPeriodic.service(clearIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  clearIn.nowMs = 302300u;
  assert(warningClearPeriodic.service(clearIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  clearIn.nowMs = 330399u;
  assert(warningClearPeriodic.service(clearIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  clearIn.nowMs = 330400u;
  c = warningClearPeriodic.service(clearIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);

  // Clearing a TSL9 warning during its sequence cancels warning-owned work,
  // emits CENTER cleanup, and resumes only after the periodic interval.
  Tsl9InputSchedulerPure activeWarningClear;
  Tsl9InputInputsPure activeClearIn = inputs(340000u, true, true);
  activeClearIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  activeClearIn.periodicIntervalSeconds = 30u;
  c = activeWarningClear.service(activeClearIn);
  activeWarningClear.onCommandResult(c, true, 340000u, 0u);
  activeClearIn.nowMs = 340100u;
  activeClearIn.warningActive = false;
  cleanup = activeWarningClear.service(activeClearIn);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  activeWarningClear.onCommandResult(cleanup, true, 340100u, 0u);
  activeClearIn.nowMs = 370099u;
  assert(activeWarningClear.service(activeClearIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  activeClearIn.nowMs = 370100u;
  c = activeWarningClear.service(activeClearIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);

  // Physical input cancels a periodic gesture and restarts the full interval.
  Tsl9InputSchedulerPure periodicManual;
  Tsl9InputInputsPure manualPeriodicIn = inputs(400000u, true, false);
  manualPeriodicIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  manualPeriodicIn.periodicIntervalSeconds = 30u;
  assert(periodicManual.service(manualPeriodicIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  manualPeriodicIn.nowMs = 430000u;
  c = periodicManual.service(manualPeriodicIn);
  periodicManual.onCommandResult(c, true, 430000u, 0u);
  periodicManual.observeManualTicks(0, 1, 430050u);
  manualPeriodicIn.nowMs = 430100u;
  assert(periodicManual.service(manualPeriodicIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  manualPeriodicIn.nowMs = 430350u;
  cleanup = periodicManual.service(manualPeriodicIn);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  periodicManual.onCommandResult(cleanup, true, 430350u, 0u);
  manualPeriodicIn.nowMs = 460099u;
  assert(periodicManual.service(manualPeriodicIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  manualPeriodicIn.nowMs = 460100u;
  c = periodicManual.service(manualPeriodicIn);
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.rightTick == 1);

  // Warning edge starts +1, 0, -1, 0 with 100 ms spacing on the left wheel.
  Tsl9InputSchedulerPure left;
  c = left.service(inputs(1000u, true, true));
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == 0u);
  assert(c.leftTick == 1 && c.rightTick == 0);
  assert(left.onCommandResult(c, true, 1000u, 10u).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  assert(left.service(inputs(1099u, true, true)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  c = left.service(inputs(1100u, true, true));
  assert(c.step == 1u && c.leftTick == 0);
  left.onCommandResult(c, true, 1100u, 10u);
  c = left.service(inputs(1200u, true, true));
  assert(c.step == 2u && c.leftTick == -1);
  left.onCommandResult(c, true, 1200u, 10u);
  c = left.service(inputs(1300u, true, true));
  assert(c.step == 3u && c.leftTick == 0);
  left.onCommandResult(c, true, 1300u, 10u);
  assert(left.service(inputs(4299u, true, true)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  c = left.service(inputs(4300u, true, true));
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == 0u);

  // Right-speed mode uses the right field and center-state bit contract.
  Tsl9InputSchedulerPure right;
  Tsl9InputInputsPure rightIn = inputs(2000u, true, true);
  rightIn.mode = TSL9_INPUT_MODE_RIGHT_SPEED_PURE;
  c = right.service(rightIn);
  assert(c.rightTick == 1 && c.leftTick == 0);
  uint8_t payload[8] = {};
  tsl9InputApplyCommandPure(c, payload);
  assert((payload[3] & 0x3Fu) == 0x01u);
  assert((payload[6] & 0x10u) == 0u);
  c.rightTick = 0;
  tsl9InputApplyCommandPure(c, payload);
  assert((payload[3] & 0x3Fu) == 0u);
  assert((payload[6] & 0x10u) != 0u);

  // A volume send failure requests an immediate center cleanup and retries
  // the warning sequence only after 250 ms.
  Tsl9InputSchedulerPure failed;
  c = failed.service(inputs(3000u, true, true));
  cleanup = failed.onCommandResult(c, false, 3000u, 0u);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  failed.onCommandResult(cleanup, true, 3000u, 0u);
  assert(failed.service(inputs(3249u, true, true)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  assert(failed.service(inputs(3250u, true, true)).kind ==
         TSL9_INPUT_COMMAND_STEP_PURE);

  // Physical input during a sequence cancels generated output and centers it.
  Tsl9InputSchedulerPure manual;
  c = manual.service(inputs(4000u, true, true));
  manual.onCommandResult(c, true, 4000u, 0u);
  manual.observeManualTicks(1, 0, 4050u);
  cleanup = manual.service(inputs(4100u, true, true));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_NONE_PURE);
  assert(manual.snapshot(4100u).cleanupPending);
  cleanup = manual.service(inputs(4350u, true, true));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  assert(cleanup.leftTick == 0 && cleanup.rightTick == 0);

  // Disable/config quiesce cleans an active non-center command.
  Tsl9InputSchedulerPure disabled;
  c = disabled.service(inputs(5000u, true, true));
  disabled.requestConfigQuiesce(
      TSL9_INPUT_FAILURE_DISABLED_PURE, 5001u);
  cleanup = disabled.service(inputs(5001u, false, false));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  assert(!disabled.configQuiesceComplete());
  disabled.onCommandResult(cleanup, true, 5001u, 0u);
  assert(disabled.configQuiesceComplete());

  // A failed CENTER is owned by the scheduler and retried after 250 ms.
  Tsl9InputSchedulerPure cleanupRetry;
  c = cleanupRetry.service(inputs(6000u, true, true));
  cleanupRetry.onCommandResult(c, true, 6000u, 0u);
  cleanup = cleanupRetry.service(inputs(6100u, true, false));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  cleanupRetry.onCommandResult(cleanup, false, 6100u, 0u);
  assert(cleanupRetry.snapshot(6100u).cleanupPending);
  assert(cleanupRetry.service(inputs(6349u, false, false)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  cleanup = cleanupRetry.service(inputs(6350u, false, false));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  cleanupRetry.onCommandResult(cleanup, true, 6350u, 0u);
  assert(!cleanupRetry.snapshot(6350u).cleanupPending);

  // Cleanup never replays a stale full-frame template. It remains pending
  // until the runtime presents a fresh route-local template.
  Tsl9InputSchedulerPure staleCleanup;
  c = staleCleanup.service(inputs(7000u, true, true));
  staleCleanup.onCommandResult(c, true, 7000u, 0u);
  Tsl9InputInputsPure stale = inputs(7100u, false, false);
  stale.templateFresh = false;
  assert(staleCleanup.service(stale).kind == TSL9_INPUT_COMMAND_NONE_PURE);
  assert(staleCleanup.snapshot(7100u).cleanupPending);
  stale.nowMs = 7350u;
  stale.templateFresh = true;
  cleanup = staleCleanup.service(stale);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  staleCleanup.onCommandResult(cleanup, true, 7350u, 0u);
  assert(!staleCleanup.snapshot(7350u).cleanupPending);

  // Config quiesce cannot complete on a failed or stale cleanup. Repeated
  // service owns the retry until CENTER succeeds.
  Tsl9InputSchedulerPure quiesceRetry;
  c = quiesceRetry.service(inputs(8000u, true, true));
  quiesceRetry.onCommandResult(c, true, 8000u, 0u);
  quiesceRetry.requestConfigQuiesce(
      TSL9_INPUT_FAILURE_MODE_CHANGED_PURE, 8001u);
  Tsl9InputInputsPure quiesceIn = inputs(8001u, false, false);
  quiesceIn.templateFresh = false;
  assert(quiesceRetry.service(quiesceIn).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  assert(!quiesceRetry.configQuiesceComplete());
  quiesceIn.nowMs = 8251u;
  quiesceIn.templateFresh = true;
  cleanup = quiesceRetry.service(quiesceIn);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  quiesceRetry.onCommandResult(cleanup, false, 8251u, 0u);
  assert(!quiesceRetry.configQuiesceComplete());
  quiesceIn.nowMs = 8501u;
  cleanup = quiesceRetry.service(quiesceIn);
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  quiesceRetry.onCommandResult(cleanup, true, 8501u, 0u);
  assert(quiesceRetry.configQuiesceComplete());

  // Cross-core interleaving: the CAN task has issued STEP but has not reported
  // its TX result when the web task requests quiesce. Quiesce remains blocked;
  // the stale STEP result cannot clear the owned CENTER cleanup.
  Tsl9InputSchedulerPure issuedRace;
  c = issuedRace.service(inputs(9000u, true, true));
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE);
  issuedRace.requestConfigQuiesce(
      TSL9_INPUT_FAILURE_MODE_CHANGED_PURE, 9001u);
  assert(!issuedRace.configQuiesceComplete());
  assert(issuedRace.onCommandResult(c, true, 9002u, 0u).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  assert(issuedRace.snapshot(9002u).cleanupPending);
  cleanup = issuedRace.service(inputs(9002u, false, false));
  assert(cleanup.kind == TSL9_INPUT_COMMAND_CENTER_PURE);
  issuedRace.onCommandResult(cleanup, true, 9002u, 0u);
  assert(issuedRace.configQuiesceComplete());

  // Signed deadline arithmetic remains correct over millis rollover.
  Tsl9InputSchedulerPure rollover;
  c = rollover.service(inputs(UINT32_MAX - 50u, true, true));
  rollover.onCommandResult(c, true, UINT32_MAX - 50u, 0u);
  assert(rollover.service(inputs(48u, true, true)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  c = rollover.service(inputs(49u, true, true));
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == 1u);

  // The randomized repeat deadline also remains ordered across rollover.
  Tsl9InputSchedulerPure repeatRollover;
  uint32_t repeatNow = UINT32_MAX - 400u;
  c = repeatRollover.service(inputs(repeatNow, true, true));
  repeatRollover.onCommandResult(c, true, repeatNow, 0u);
  for (uint8_t step = 1u; step < 4u; ++step) {
    repeatNow += 100u;
    c = repeatRollover.service(inputs(repeatNow, true, true));
    assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == step);
    repeatRollover.onCommandResult(c, true, repeatNow, 0u);
  }
  const uint32_t repeatDue = repeatNow + 2000u;
  assert(repeatRollover.service(inputs(repeatDue - 1u, true, true)).kind ==
         TSL9_INPUT_COMMAND_NONE_PURE);
  c = repeatRollover.service(inputs(repeatDue, true, true));
  assert(c.kind == TSL9_INPUT_COMMAND_STEP_PURE && c.step == 0u);

  return 0;
}
