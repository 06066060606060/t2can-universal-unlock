#pragma once
#include <stdint.h>

// Cooperative limits, checked between frames. One decoder/SPI transaction can
// exceed the time limit; no frame already read is discarded to enforce it.
static constexpr uint8_t CAN_RX_SLICE_FRAMES = 4;
static constexpr uint32_t CAN_RX_SLICE_US = 600;
static constexpr uint32_t CAN_RX_MAX_BURST_US = 4000;
static constexpr uint32_t CAN_RX_IDLE_RETRY_US = 250;
static constexpr uint8_t CAN_RX_TASK_PRIORITY = 4;

struct CanRxPauseStatePure {
  uint32_t burstStartedUs;
  uint32_t lastReceiveUs;
};

static inline bool canRxSliceAvailablePure(uint32_t frames, uint32_t startedUs,
                                          uint32_t nowUs) {
  return frames < CAN_RX_SLICE_FRAMES &&
         (frames == 0 || (uint32_t)(nowUs - startedUs) < CAN_RX_SLICE_US);
}

static inline bool canRxShouldSleepPure(bool received, CanRxPauseStatePure &state,
                                      uint32_t nowUs) {
  if (received) state.lastReceiveUs = nowUs;
  return (uint32_t)(nowUs - state.burstStartedUs) >= CAN_RX_MAX_BURST_US ||
         (!received && (uint32_t)(nowUs - state.lastReceiveUs) >= CAN_RX_IDLE_RETRY_US);
}
