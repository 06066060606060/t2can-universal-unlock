#include <cassert>
#include <cstdint>

#include "../blinker_tx_policy_pure.h"

static void one_button_press_is_consumed_by_exactly_one_idle_stock_frame() {
  BlinkerTxRequestStatePure request = {};
  assert(blinkerTxArmPure(
      request, 1, BLINKER_TX_SOURCE_S3XY_PURE, 1, 1000));

  const BlinkerTxConsumeResultPure first =
      blinkerTxConsumeStockPure(request, 1049, true, 250);
  assert(first.transmit && first.dir == 1);
  assert(request.pendingDir == 0);
  assert(!blinkerTxConsumeStockPure(request, 1099, true, 250).transmit);
}

static void a_new_button_press_replaces_the_pending_direction() {
  BlinkerTxRequestStatePure request = {};
  assert(blinkerTxArmPure(
      request, 1, BLINKER_TX_SOURCE_S3XY_PURE, 1, 1000));
  assert(blinkerTxArmPure(
      request, 2, BLINKER_TX_SOURCE_S3XY_PURE, 2, 1010));

  const BlinkerTxConsumeResultPure consumed =
      blinkerTxConsumeStockPure(request, 1050, true, 250);
  assert(consumed.transmit && consumed.dir == 2);
}

static void a_physical_stalk_frame_cancels_the_pending_button_request() {
  BlinkerTxRequestStatePure request = {};
  assert(blinkerTxArmPure(
      request, 1, BLINKER_TX_SOURCE_S3XY_PURE, 1, 1000));

  assert(!blinkerTxConsumeStockPure(request, 1050, false, 250).transmit);
  assert(request.pendingDir == 0);
}

static void a_stale_button_request_is_not_sent_later() {
  BlinkerTxRequestStatePure request = {};
  assert(blinkerTxArmPure(
      request, 2, BLINKER_TX_SOURCE_S3XY_PURE, 1, UINT32_MAX - 20));

  assert(!blinkerTxConsumeStockPure(request, 300, true, 250).transmit);
  assert(request.pendingDir == 0);
}

int main() {
  one_button_press_is_consumed_by_exactly_one_idle_stock_frame();
  a_new_button_press_replaces_the_pending_direction();
  a_physical_stalk_frame_cancels_the_pending_button_request();
  a_stale_button_request_is_not_sent_later();
  return 0;
}
