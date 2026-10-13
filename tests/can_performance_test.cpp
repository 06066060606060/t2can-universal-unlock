#include "can_service_policy_pure.h"
#include "can_performance_pure.h"
#include <cassert>
#include <cstdio>
#include <climits>

int main() {
  // Frame/time ceilings, wraparound and required idle/watchdog opportunities.
  assert(canRxSliceAvailablePure(0, 100, 100));
  // Preemption before entering the loop must not skip a bus's first poll.
  assert(canRxSliceAvailablePure(0, 100, 10000));
  assert(canRxSliceAvailablePure(3, 100, 699));
  assert(!canRxSliceAvailablePure(4, 100, 100));
  assert(!canRxSliceAvailablePure(1, 100, 700));
  assert(canRxSliceAvailablePure(1, UINT32_MAX - 100, 100));
  assert(!canRxSliceAvailablePure(1, UINT32_MAX - 100, 600));
  assert(canRxShouldSleepPure(false, 100, 101));
  assert(!canRxShouldSleepPure(true, 100, 2099));
  assert(canRxShouldSleepPure(true, 100, 2100));
  assert(canRxShouldSleepPure(true, UINT32_MAX - 100, 2000));

  // Exhaustively decode hardware occupancy without counting unrelated bits.
  for (unsigned status = 0; status < 256; ++status) {
    const unsigned expected = !!(status & 4) + !!(status & 16) + !!(status & 64);
    assert(canPerfPendingPure(status) == expected);
  }
  CanPerfBusPure bus = {};
  canPerfStatusObservePure(bus, 0x54);
  canPerfStatusObservePure(bus, 0);
  assert(bus.txPendingLast == 0 && bus.txPendingMax == 3);
  assert(bus.txStatusSamples == 2 && bus.txFullSamples == 1);
  canPerfOverflowObservePure(bus, 0x1F);
  assert(bus.rxOverflowEvents == 0);
  canPerfOverflowObservePure(bus, 0xC0);
  canPerfOverflowObservePure(bus, 0x40);
  assert(bus.rxOverflowEvents == 2 && bus.rx0OverflowEvents == 2 && bus.rx1OverflowEvents == 1);
  canPerfPollObservePure(bus, UINT32_MAX - 5);
  canPerfPollObservePure(bus, 4);
  assert(bus.pollGap.count == 1 && bus.pollGap.maxUs == 10);

  CanPerfTimingPure timing = {};
  assert(canPerfTimingPercentilePure(timing, 95) == 0);
  for (unsigned i = 0; i < 95; ++i) canPerfTimingObservePure(timing, 9);
  for (unsigned i = 0; i < 5; ++i) canPerfTimingObservePure(timing, 10000);
  assert(timing.count == 100 && timing.totalUs == 50855);
  assert(timing.histogram[1] == 95 && timing.histogram[10] == 5);
  assert(canPerfTimingPercentilePure(timing, 95) == 16);
  assert(canPerfTimingPercentilePure(timing, 99) == 10000);
  timing.totalUs = UINT32_MAX;
  canPerfTimingObservePure(timing, 100);
  assert(timing.totalUs == (uint64_t)UINT32_MAX + 100);
  std::puts("PASS: scheduling limits, wraparound, 256 TX statuses, overflow accounting and latency histogram");
}
