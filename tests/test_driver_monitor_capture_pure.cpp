#include <cassert>
#include <cstdint>
#include "driver_monitor_capture_pure.h"

int main() {
  using namespace driver_monitor_capture_pure;
  // Universal routing: all five research IDs are observed on both physical buses.
  const uint16_t ids[] = {0x389, 0x5D9, 0x247, 0x399, 0x370};
  for (uint16_t id : ids) {
    assert(targetFrame(BUS_A, id));
    assert(targetFrame(BUS_B, id));
  }
  assert(!targetFrame(BUS_A, 0x118));
  assert(!targetFrame(BUS_B, 0x118));


  // DAS_driverInteractionLevel = start bit 38, length 2, little endian.
  uint8_t d[8] = {};
  d[4] = 0x00; assert(driverMonitorInteractionLevelPure(d, 8) == 0);
  d[4] = 0x40; assert(driverMonitorInteractionLevelPure(d, 8) == 1);
  d[4] = 0x80; assert(driverMonitorInteractionLevelPure(d, 8) == 2);
  d[4] = 0xC0; assert(driverMonitorInteractionLevelPure(d, 8) == 3);
  assert(driverMonitorInteractionLevelPure(d, 4) == 0xFF);

  // DAS_status.DAS_autopilotHandsOnState = bit42, length 4.
  uint8_t das[8] = {};
  das[5] = 0x2C; // bits[5:2] = 0xB
  assert(driverMonitorDasHandsOnPure(das, 8) == 0x0B);
  assert(driverMonitorDasHandsOnPure(das, 5) == 0xFF);

  // EPAS_sysStatus handsOnLevel is bits38..39; torque is raw 12-bit at 16..27.
  uint8_t epas[8] = {};
  epas[2] = 0x08; epas[3] = 0x02; // raw 0x802 = 2050 = 0.00 Nm
  epas[4] = 0x40;                  // HO=1
  assert(driverMonitorEpasHandsOnPure(epas, 8) == 1);
  assert(driverMonitorEpasTorqueRawPure(epas, 8) == 2050u);
  assert(driverMonitorEpasTorqueCentiNmPure(epas, 8) == 0);
  return 0;
}
