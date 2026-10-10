#include <cassert>
#include <cstdint>
#include "nag_human_v1_pure.h"

static uint16_t movingSpeedRaw() { return 600u; } // 8 kph

int main() {
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    assert(c.hoOverridePct == 100u);
    assert(nagHumanV1TimingValidPure(c.waitMinMs, c.waitMaxMs,
                                   c.refractoryMinMs, c.refractoryMaxMs));
    assert(!nagHumanV1TimingValidPure(200u, 3000u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 5100u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(2200u, 1200u, 800u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 3000u, 200u, 1800u));
    assert(!nagHumanV1TimingValidPure(1200u, 3000u, 1800u, 1700u));
  }

  // 100% override preserves the old Mode H carrier behavior.
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 100u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0x12345678u);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto r = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(r.tx);
    assert(r.setHo);
  }

  // 0% override must preserve the stock HO bits rather than forcing HO=0.
  // setHo=false is the transport contract for "leave stock untouched".
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 0u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0xCAFEBABEu);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto r = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(r.tx);
    assert(!r.setHo);
  }

  // A held decision must not reroll on every CAN frame.
  {
    NagHumanV1ConfigPure c = nagHumanV1Rev1PlusConfigPure();
    c.hoOverridePct = 50u;
    NagHumanV1StatePure s = {};
    nagHumanV1InitPure(s, 0x0BADF00Du);
    (void)nagHumanV1StepPure(s, c, 0u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                           true, true, true, movingSpeedRaw());
    auto a = nagHumanV1StepPure(s, c, 400u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    auto b = nagHumanV1StepPure(s, c, 450u, NAG_HUMAN_V1_TORQUE_CENTER_RAW,
                              true, true, true, movingSpeedRaw());
    assert(a.tx && b.tx);
    assert(a.setHo == b.setHo);
  }

  return 0;
}
