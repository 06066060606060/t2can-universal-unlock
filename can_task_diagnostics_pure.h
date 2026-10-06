#pragma once

#include <stdint.h>

enum CanTaskStagePure : uint8_t {
  CAN_TASK_STAGE_NEVER_STARTED = 0,
  CAN_TASK_STAGE_START = 1,
  CAN_TASK_STAGE_ALERTS = 2,
  CAN_TASK_STAGE_RECEIVE = 3,
  CAN_TASK_STAGE_PROCESS = 4,
  CAN_TASK_STAGE_SERVICE = 5,
  CAN_TASK_STAGE_STATUS = 6,
  CAN_TASK_STAGE_DELAY = 7,
  CAN_TASK_STAGE_QUIESCE = 8
};

static inline const char *canTaskStageNamePure(uint8_t stage) {
  switch (stage) {
    case CAN_TASK_STAGE_START: return "START";
    case CAN_TASK_STAGE_ALERTS: return "ALERTS";
    case CAN_TASK_STAGE_RECEIVE: return "RECEIVE";
    case CAN_TASK_STAGE_PROCESS: return "PROCESS";
    case CAN_TASK_STAGE_SERVICE: return "SERVICE";
    case CAN_TASK_STAGE_STATUS: return "STATUS";
    case CAN_TASK_STAGE_DELAY: return "DELAY";
    case CAN_TASK_STAGE_QUIESCE: return "QUIESCE";
    default: return "NEVER_STARTED";
  }
}

struct CanTaskLiveDiagnosticsPure {
  uint8_t stage;
  uint32_t stageEnteredMs;
  uint32_t heartbeatCount;
  uint32_t lastHeartbeatMs;
  uint32_t maxHeartbeatGapMs;
  uint32_t loopStartedUs;
  uint32_t loopCount;
  uint32_t lastLoopDurationUs;
  uint32_t maxLoopDurationUs;
};

struct CanTaskTimeoutSnapshotPure {
  uint8_t stage;
  uint8_t taskState;
  uint32_t heartbeatAgeMs;
  uint32_t stageAgeMs;
  uint32_t heartbeatCount;
  uint32_t maxHeartbeatGapMs;
  uint32_t loopCount;
  uint32_t lastLoopDurationUs;
  uint32_t maxLoopDurationUs;
  uint32_t stackHighWater;
};

struct CanARxDiagnosticsPure {
  uint32_t loopCount;
  uint32_t framesProcessed;
  uint32_t maxFramesPerLoop;
  uint32_t budgetExhaustedLoops;
  uint32_t overflowObservations;
  uint32_t rx0OverflowObservations;
  uint32_t rx1OverflowObservations;
  uint32_t lastOverflowMs;
  uint8_t lastOverflowFlags;
  uint32_t framesAtLastOverflow;
  uint32_t budgetExhaustedAtLastOverflow;
};

static inline void canTaskDiagnosticsResetPure(volatile CanTaskLiveDiagnosticsPure &d) {
  d.stage = CAN_TASK_STAGE_NEVER_STARTED;
  d.stageEnteredMs = 0;
  d.heartbeatCount = 0;
  d.lastHeartbeatMs = 0;
  d.maxHeartbeatGapMs = 0;
  d.loopStartedUs = 0;
  d.loopCount = 0;
  d.lastLoopDurationUs = 0;
  d.maxLoopDurationUs = 0;
}

static inline void canTaskDiagnosticsHeartbeatPure(
    volatile CanTaskLiveDiagnosticsPure &d, uint32_t nowMs, uint32_t nowUs) {
  if (d.heartbeatCount != 0) {
    const uint32_t gap = nowMs - d.lastHeartbeatMs;
    if (gap > d.maxHeartbeatGapMs) d.maxHeartbeatGapMs = gap;
  }
  d.heartbeatCount++;
  d.lastHeartbeatMs = nowMs;
  d.loopStartedUs = nowUs;
  d.stage = CAN_TASK_STAGE_START;
  d.stageEnteredMs = nowMs;
}

static inline void canTaskDiagnosticsPulsePure(
    volatile CanTaskLiveDiagnosticsPure &d, uint32_t nowMs) {
  if (d.heartbeatCount != 0) {
    const uint32_t gap = nowMs - d.lastHeartbeatMs;
    if (gap > d.maxHeartbeatGapMs) d.maxHeartbeatGapMs = gap;
  }
  d.heartbeatCount++;
  d.lastHeartbeatMs = nowMs;
}

static inline void canTaskDiagnosticsEnterStagePure(
    volatile CanTaskLiveDiagnosticsPure &d, uint8_t stage, uint32_t nowMs) {
  d.stage = stage;
  d.stageEnteredMs = nowMs;
}

static inline void canTaskDiagnosticsFinishLoopPure(
    volatile CanTaskLiveDiagnosticsPure &d, uint32_t nowUs) {
  const uint32_t duration = nowUs - d.loopStartedUs;
  d.loopCount++;
  d.lastLoopDurationUs = duration;
  if (duration > d.maxLoopDurationUs) d.maxLoopDurationUs = duration;
}

static inline CanTaskTimeoutSnapshotPure canTaskDiagnosticsSnapshotPure(
    const volatile CanTaskLiveDiagnosticsPure &d, uint32_t nowMs,
    uint8_t taskState, uint32_t stackHighWater) {
  CanTaskTimeoutSnapshotPure out = {};
  out.stage = d.stage;
  out.taskState = taskState;
  out.heartbeatAgeMs = d.heartbeatCount != 0
      ? nowMs - d.lastHeartbeatMs : UINT32_MAX;
  out.stageAgeMs = d.stage != CAN_TASK_STAGE_NEVER_STARTED
      ? nowMs - d.stageEnteredMs : UINT32_MAX;
  out.heartbeatCount = d.heartbeatCount;
  out.maxHeartbeatGapMs = d.maxHeartbeatGapMs;
  out.loopCount = d.loopCount;
  out.lastLoopDurationUs = d.lastLoopDurationUs;
  out.maxLoopDurationUs = d.maxLoopDurationUs;
  out.stackHighWater = stackHighWater;
  return out;
}

static inline void canARxDiagnosticsResetPure(volatile CanARxDiagnosticsPure &d) {
  d.loopCount = 0;
  d.framesProcessed = 0;
  d.maxFramesPerLoop = 0;
  d.budgetExhaustedLoops = 0;
  d.overflowObservations = 0;
  d.rx0OverflowObservations = 0;
  d.rx1OverflowObservations = 0;
  d.lastOverflowMs = 0;
  d.lastOverflowFlags = 0;
  d.framesAtLastOverflow = 0;
  d.budgetExhaustedAtLastOverflow = 0;
}

static inline void canARxDiagnosticsCompleteLoopPure(
    volatile CanARxDiagnosticsPure &d, uint32_t processed, uint32_t budget) {
  d.loopCount++;
  d.framesProcessed += processed;
  if (processed > d.maxFramesPerLoop) d.maxFramesPerLoop = processed;
  if (processed >= budget) d.budgetExhaustedLoops++;
}

static inline void canARxDiagnosticsObserveOverflowPure(
    volatile CanARxDiagnosticsPure &d, uint32_t nowMs, uint8_t flags,
    bool rx0Overflow, bool rx1Overflow) {
  d.overflowObservations++;
  if (rx0Overflow) d.rx0OverflowObservations++;
  if (rx1Overflow) d.rx1OverflowObservations++;
  d.lastOverflowMs = nowMs;
  d.lastOverflowFlags = flags;
  d.framesAtLastOverflow = d.framesProcessed;
  d.budgetExhaustedAtLastOverflow = d.budgetExhaustedLoops;
}
