#include <cassert>
#include <cstdint>

#include "../blinker_tx_policy_pure.h"
#include "../auto_blinker_pure.h"

int main() {
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
}
