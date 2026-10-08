#include <assert.h>
#include <stdint.h>
#include <string.h>

#if __has_include("../driver_window_lab_pure.h")
#include "../driver_window_lab_pure.h"

static DriverWindowArmContextPure readyContext() {
  DriverWindowArmContextPure c = {};
  c.labEnabled = true;
  c.modelYlPartyVh = true;
  c.gearFresh = true;
  c.parked = true;
  c.stockValid = true;
  c.stockAgeMs = 100u;
  c.stockInputIdle = true;
  c.canTxAllowed = true;
  c.guardGeneration = 4u;
  return c;
}

static void yl_party_vh_is_the_only_supported_profile() {
  assert(driverWindowLabProfileSupportedPure(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_YL_PARTY_VH));
  assert(!driverWindowLabProfileSupportedPure(
      VEHICLE_MODEL_YL, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS));
  assert(!driverWindowLabProfileSupportedPure(
      VEHICLE_MODEL_Y_JUNIPER, VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS));
}

static void auto_down_replaces_only_byte6_switch_field() {
  uint8_t frame[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x15, 0x00};
  uint8_t expected[8];
  memcpy(expected, frame, sizeof(expected));
  expected[6] = 0x19;

  assert(driverWindowSetAutoDownLfPure(frame, true));
  assert(memcmp(frame, expected, sizeof(frame)) == 0);
  assert(driverWindowSetAutoDownLfPure(frame, false));
  expected[6] = 0x15;
  assert(memcmp(frame, expected, sizeof(frame)) == 0);
}

static void physical_auto_down_input_is_not_idle() {
  const uint8_t idle[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x15, 0x00};
  const uint8_t autoDown[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x19, 0x00};
  assert(driverWindowPhysicalInputIdlePure(idle));
  assert(!driverWindowPhysicalInputIdlePure(autoDown));
}

static void arm_gates_fail_closed() {
  DriverWindowLabStatePure s = {};
  DriverWindowArmContextPure c = readyContext();
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_OK);
  assert(driverWindowLabArmPure(s, 200u, 9u, c) == DRIVER_WINDOW_ARM_BUSY);

  driverWindowLabResetPure(s);
  c.labEnabled = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_LAB_DISABLED);
  c = readyContext(); c.modelYlPartyVh = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_UNSUPPORTED_PROFILE);
  c = readyContext(); c.gearFresh = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_GEAR_STALE);
  c = readyContext(); c.parked = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_NOT_PARKED);
  c = readyContext(); c.stockValid = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_STOCK_MISSING);
  c = readyContext(); c.stockAgeMs = DRIVER_WINDOW_STOCK_FRESH_MS + 1u;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_STOCK_STALE);
  c = readyContext(); c.stockInputIdle = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_PHYSICAL_INPUT);
  c = readyContext(); c.canTxAllowed = false;
  assert(driverWindowLabArmPure(s, 100u, 9u, c) == DRIVER_WINDOW_ARM_CAN_UNAVAILABLE);
}

static void pulse_is_two_stock_synced_frames_and_never_overwrites_input() {
  DriverWindowLabStatePure s = {};
  DriverWindowArmContextPure c = readyContext();
  assert(driverWindowLabArmPure(s, 100u, 77u, c) == DRIVER_WINDOW_ARM_OK);

  const uint8_t idle[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x15, 0x00};
  DriverWindowConsumeResultPure first =
      driverWindowLabConsumeStockPure(s, 150u, 77u, c, idle, 8u);
  assert(first.transmit && !first.completed && s.pending && s.framesSent == 1u);
  assert(first.data[6] == 0x19u);

  DriverWindowConsumeResultPure second =
      driverWindowLabConsumeStockPure(s, 250u, 77u, c, idle, 8u);
  assert(second.transmit && second.completed && !s.pending && s.framesSent == 2u);
  assert(second.data[6] == 0x19u);

  DriverWindowConsumeResultPure third =
      driverWindowLabConsumeStockPure(s, 350u, 77u, c, idle, 8u);
  assert(!third.transmit);

  assert(driverWindowLabArmPure(s, 400u, 77u, c) == DRIVER_WINDOW_ARM_OK);
  uint8_t physical[8]; memcpy(physical, idle, sizeof(physical)); physical[6] = 0x19u;
  DriverWindowConsumeResultPure blocked =
      driverWindowLabConsumeStockPure(s, 450u, 77u, c, physical, 8u);
  assert(!blocked.transmit && blocked.blocked && !s.pending);
  assert(blocked.reason == DRIVER_WINDOW_CONSUME_PHYSICAL_INPUT);
}

static void invalid_mux_epoch_timeout_and_rollover_fail_closed() {
  DriverWindowLabStatePure s = {};
  DriverWindowArmContextPure c = readyContext();
  const uint8_t idle[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x15, 0x00};
  uint8_t mux1[8]; memcpy(mux1, idle, sizeof(mux1)); mux1[0] = 0x01u;

  assert(driverWindowLabArmPure(s, 100u, 10u, c) == DRIVER_WINDOW_ARM_OK);
  assert(!driverWindowLabConsumeStockPure(s, 150u, 10u, c, mux1, 8u).transmit);
  assert(s.pending);
  DriverWindowConsumeResultPure epoch =
      driverWindowLabConsumeStockPure(s, 160u, 11u, c, idle, 8u);
  assert(!epoch.transmit && epoch.blocked && !s.pending);
  assert(epoch.reason == DRIVER_WINDOW_CONSUME_EPOCH_CHANGED);

  assert(driverWindowLabArmPure(s, 100u, 12u, c) == DRIVER_WINDOW_ARM_OK);
  DriverWindowConsumeResultPure expired = driverWindowLabConsumeStockPure(
      s, 100u + DRIVER_WINDOW_REQUEST_TIMEOUT_MS + 1u, 12u, c, idle, 8u);
  assert(!expired.transmit && expired.blocked && !s.pending);
  assert(expired.reason == DRIVER_WINDOW_CONSUME_TIMEOUT);

  assert(driverWindowLabArmPure(s, 500u, 12u, c) == DRIVER_WINDOW_ARM_OK);
  DriverWindowConsumeResultPure noStockTimeout =
      driverWindowLabConsumeStockPure(
          s, 500u + DRIVER_WINDOW_REQUEST_TIMEOUT_MS + 1u,
          12u, c, nullptr, 0u);
  assert(!noStockTimeout.transmit && noStockTimeout.blocked && !s.pending);
  assert(noStockTimeout.reason == DRIVER_WINDOW_CONSUME_TIMEOUT);

  driverWindowLabResetPure(s);
  assert(driverWindowLabArmPure(s, UINT32_MAX - 100u, 13u, c) == DRIVER_WINDOW_ARM_OK);
  DriverWindowConsumeResultPure wrapped =
      driverWindowLabConsumeStockPure(s, 50u, 13u, c, idle, 8u);
  assert(wrapped.transmit && s.pending);
}

static void runtime_authorization_and_stock_gap_fail_closed() {
  DriverWindowLabStatePure s = {};
  DriverWindowArmContextPure c = readyContext();
  const uint8_t idle[8] = {0x00, 0x55, 0x55, 0x55, 0x00, 0x00, 0x15, 0x00};

  assert(driverWindowLabArmPure(s, 100u, 20u, c) == DRIVER_WINDOW_ARM_OK);
  c.parked = false;
  DriverWindowConsumeResultPure drive =
      driverWindowLabConsumeStockPure(s, 120u, 20u, c, idle, 8u);
  assert(drive.blocked && !drive.transmit && !s.pending);
  assert(drive.reason == DRIVER_WINDOW_CONSUME_NOT_PARKED);

  c = readyContext();
  assert(driverWindowLabArmPure(s, 200u, 21u, c) == DRIVER_WINDOW_ARM_OK);
  c.gearFresh = false;
  DriverWindowConsumeResultPure staleGear =
      driverWindowLabConsumeStockPure(s, 220u, 21u, c, idle, 8u);
  assert(staleGear.blocked && staleGear.reason == DRIVER_WINDOW_CONSUME_GEAR_STALE);

  c = readyContext();
  assert(driverWindowLabArmPure(s, 300u, 22u, c) == DRIVER_WINDOW_ARM_OK);
  c.stockAgeMs = DRIVER_WINDOW_STOCK_FRESH_MS + 1u;
  DriverWindowConsumeResultPure staleStock =
      driverWindowLabConsumeStockPure(s, 560u, 22u, c, idle, 8u);
  assert(staleStock.blocked && staleStock.reason == DRIVER_WINDOW_CONSUME_STOCK_STALE);

  c = readyContext();
  assert(driverWindowLabArmPure(s, 600u, 23u, c) == DRIVER_WINDOW_ARM_OK);
  c.labEnabled = false;
  DriverWindowConsumeResultPure labOff =
      driverWindowLabConsumeStockPure(s, 620u, 23u, c, idle, 8u);
  assert(labOff.blocked && labOff.reason == DRIVER_WINDOW_CONSUME_LAB_DISABLED);
}

int main() {
  yl_party_vh_is_the_only_supported_profile();
  auto_down_replaces_only_byte6_switch_field();
  physical_auto_down_input_is_not_idle();
  arm_gates_fail_closed();
  pulse_is_two_stock_synced_frames_and_never_overwrites_input();
  invalid_mux_epoch_timeout_and_rollover_fail_closed();
  runtime_authorization_and_stock_gap_fail_closed();
  return 0;
}

#else
int main() {
  assert(false && "driver window LAB pure helper missing");
  return 0;
}
#endif
