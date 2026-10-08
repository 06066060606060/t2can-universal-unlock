#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "summon_state_pure.h"
#include "runtime_gate_pure.h"

static void expect(bool enabled, uint8_t reason, const R79TxDecisionPure &d) {
  assert(d.txEnabled == enabled);
  assert(d.reason == reason);
  assert(d.manualSuppressed == !enabled);
}

int main() {
  // Tesla OFF/manual states used by the dashboard/runtime must agree. State 2
  // is observed on YL during ordinary D driving and must not bounce R79 ACTIVE.
  for (uint8_t s : {0, 1, 2, 8, 9, 14}) assert(dasStateManualPure(true, s));
  for (uint8_t s : {3, 4, 5, 6}) assert(!dasStateManualPure(true, s));
  assert(!dasStateManualPure(false, 1));

  R79ManualSuppressionPure manual = {};

  // Fresh remote-start evidence prevents entering manual suppression before a
  // Summon session is fully confirmed, but does not itself claim SUMMON.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, false, true,
      false, true);
  assert(!manual.active);
  expect(true, R79_TX_REASON_DEFAULT,
         r79TxDecisionPure(false, false, manual));

  // Definite D + manual DAS with no remote-start evidence latches suppression.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, false, true,
      false, false);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_D);
  expect(false, R79_TX_REASON_MANUAL_D,
         r79TxDecisionPure(false, false, manual));

  // Transient unknown DAS/gear and fresh ACA/SPR evidence must not make a
  // confirmed manual drive bounce ACTIVE/SUSPENDED.
  r79ManualSuppressionUpdatePure(manual,
      false, TESLA_GEAR_INVALID, false, false, false,
      false, true);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_D);
  expect(false, R79_TX_REASON_MANUAL_D,
         r79TxDecisionPure(false, false, manual));

  // D -> R while still manual remains suppressed and updates the reason.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_R, true, false, true,
      false, false);
  assert(manual.active && manual.gearRaw == TESLA_GEAR_R);
  expect(false, R79_TX_REASON_MANUAL_R,
         r79TxDecisionPure(false, false, manual));

  // Confirmed Summon immediately releases manual suppression, including in R.
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_R, true, false, true,
      true, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_SUMMON,
         r79TxDecisionPure(false, true, manual));

  // AP active also releases suppression immediately and has display priority
  // over SUMMON if both flags are ever simultaneously present.
  manual.active = true;
  manual.gearRaw = TESLA_GEAR_D;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_D, true, true, false,
      true, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_AUTOPILOT,
         r79TxDecisionPure(true, true, manual));

  // Explicit P/N ends a manual-driving latch. Unknown/stale alone does not.
  manual.active = true;
  manual.gearRaw = TESLA_GEAR_D;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_N, true, false, true,
      false, false);
  assert(!manual.active);
  expect(true, R79_TX_REASON_DEFAULT,
         r79TxDecisionPure(false, false, manual));

  manual.active = true;
  manual.gearRaw = TESLA_GEAR_R;
  r79ManualSuppressionUpdatePure(manual,
      true, TESLA_GEAR_P, true, false, true,
      false, false);
  assert(!manual.active);

  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_AUTOPILOT), "AUTOPILOT") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_SUMMON), "SUMMON") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_MANUAL_D), "MANUAL D") == 0);
  assert(std::strcmp(r79TxReasonNamePure(R79_TX_REASON_MANUAL_R), "MANUAL R") == 0);
  return 0;
}
