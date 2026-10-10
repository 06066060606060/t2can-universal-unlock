#include <cassert>
#include <cstdint>
#include <iostream>

#include "../can_task_diagnostics_pure.h"

int main() {
  CanTaskLiveDiagnosticsPure task = {};

  const CanTaskTimeoutSnapshotPure neverStarted =
      canTaskDiagnosticsSnapshotPure(task, 5000u, 1u, 7000u);
  assert(neverStarted.stage == CAN_TASK_STAGE_NEVER_STARTED);
  assert(neverStarted.heartbeatAgeMs == UINT32_MAX);
  assert(neverStarted.stageAgeMs == UINT32_MAX);
  assert(neverStarted.taskState == 1u);
  assert(neverStarted.stackHighWater == 7000u);

  canTaskDiagnosticsHeartbeatPure(task, 100u, 1000u);
  canTaskDiagnosticsEnterStagePure(task, CAN_TASK_STAGE_PROCESS, 110u);
  canTaskDiagnosticsFinishLoopPure(task, 1250u);
  assert(task.loopCount == 1u);
  assert(task.lastLoopDurationUs == 250u);
  assert(task.maxLoopDurationUs == 250u);

  canTaskDiagnosticsHeartbeatPure(task, 160u, UINT32_MAX - 99u);
  canTaskDiagnosticsEnterStagePure(task, CAN_TASK_STAGE_RECEIVE, 170u);
  canTaskDiagnosticsFinishLoopPure(task, 75u);
  canTaskDiagnosticsPulsePure(task, 190u);
  const CanTaskTimeoutSnapshotPure stalled =
      canTaskDiagnosticsSnapshotPure(task, 200u, 2u, 6800u);
  assert(stalled.stage == CAN_TASK_STAGE_RECEIVE);
  assert(stalled.heartbeatAgeMs == 10u);
  assert(stalled.stageAgeMs == 30u);
  assert(stalled.loopCount == 2u);
  assert(stalled.lastLoopDurationUs == 175u);
  assert(stalled.maxLoopDurationUs == 250u);
  assert(stalled.maxHeartbeatGapMs == 60u);

  // A supervisor can sample its timestamp before a slower persistence step,
  // while the CAN task publishes a newer heartbeat during that step.  This is
  // a small future timestamp, not a 49.7-day-old heartbeat.
  const CanTaskTimeoutSnapshotPure concurrentUpdate =
      canTaskDiagnosticsSnapshotPure(task, 100u, 2u, 6800u);
  assert(concurrentUpdate.heartbeatAgeMs == 0u);
  assert(concurrentUpdate.stageAgeMs == 0u);

  assert(canTaskHeartbeatTimedOutPure(5000u, 0u, 3000u));
  assert(!canTaskHeartbeatTimedOutPure(1000u, 1131u, 3000u));
  assert(!canTaskHeartbeatTimedOutPure(4000u, 1000u, 3000u));
  assert(canTaskHeartbeatTimedOutPure(4001u, 1000u, 3000u));
  assert(!canTaskHeartbeatTimedOutPure(
      2899u, UINT32_MAX - 100u, 3000u));
  assert(canTaskHeartbeatTimedOutPure(
      2900u, UINT32_MAX - 100u, 3000u));

  CanARxDiagnosticsPure rx = {};
  canARxDiagnosticsCompleteLoopPure(rx, 12u, 32u);
  canARxDiagnosticsCompleteLoopPure(rx, 32u, 32u);
  canARxDiagnosticsObserveOverflowPure(rx, 1000u, 0x40u, true, false);
  canARxDiagnosticsObserveOverflowPure(rx, 1200u, 0xC0u, true, true);

  assert(rx.loopCount == 2u);
  assert(rx.framesProcessed == 44u);
  assert(rx.maxFramesPerLoop == 32u);
  assert(rx.budgetExhaustedLoops == 1u);
  assert(rx.overflowObservations == 2u);
  assert(rx.rx0OverflowObservations == 2u);
  assert(rx.rx1OverflowObservations == 1u);
  assert(rx.lastOverflowMs == 1200u);
  assert(rx.lastOverflowFlags == 0xC0u);
  assert(rx.framesAtLastOverflow == 44u);
  assert(rx.budgetExhaustedAtLastOverflow == 1u);

  canTaskDiagnosticsResetPure(task);
  canARxDiagnosticsResetPure(rx);
  assert(task.heartbeatCount == 0u);
  assert(task.stage == CAN_TASK_STAGE_NEVER_STARTED);
  assert(rx.loopCount == 0u);
  assert(rx.overflowObservations == 0u);

  std::cout << "CAN task diagnostic snapshots: PASS\n";
  return 0;
}
