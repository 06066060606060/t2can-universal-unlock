#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../preheat_pure.h"

// The RX fixtures below (0x082 source, 0x332, 0x252, 0x241, 0x212, 0x2E1, 0x312,
// 0x3A1, 0x201, 0x321 and the stator pair) are copied verbatim from the captures
// of three HW3 cars' runs (firmware 2026.20.3) and of the HW4 capture; a
// failure there means a decoder no longer reproduces observed car behaviour.
// ---------------------------------------------------------------------
// The transmitted payload - one constant frame
//
// carPreheat is the frame the car itself sent while it preconditioned, and
// routeHomeLive is the frame its planner held steady through a
// device capture (44 of 44 frames, a route that is NOT to a charger). Both stay
// FACTS about the car; nothing below builds a payload out of them.
int main() {
{
  const uint8_t liveIdle[8] = {0x00, 0x7F, 0xFF, 0x80, 0xFF, 0x03, 0x00, 0x80};
  const uint8_t carPreheat[8] = {0xAB, 0x50, 0xC8, 0x13, 0xFF, 0x03, 0x8E, 0x14};
  // The frame this revision transmits, unchanged, every PREHEAT_PERIOD_MS.
  const uint8_t transmitted[8] = {0xAB, 0x50, 0xC8, 0x13, 0xFF, 0x03, 0x8E, 0x14};
  // The exact frame a field capture shows, verbatim: an active plan
  // that is not to a charger.
  const uint8_t routeHomeLive[8] = {0x01, 0x50, 0xC1, 0x13, 0xFF, 0x03, 0xFF, 0x0B};
  assert(!preheatEnergiesValidPure(liveIdle, 8));   // parked: SNA energies
  assert(preheatEnergiesValidPure(carPreheat, 8));  // the car's own route
  assert(preheatEnergiesValidPure(routeHomeLive, 8));

  // ---- the transmitted frame ---------------------------------------------
  // It IS the car's own preconditioning frame: the flat revision argues about
  // nothing - not the route, not the charger type, not the temperature - and the
  // header's static_asserts pin the same property at compile time.
  for (uint8_t i = 0; i < PREHEAT_DLC; i++) {
    assert(transmitted[i] == carPreheat[i]);
    assert(PREHEAT_REFERENCE_FRAME[i] == transmitted[i]);
  }
  // byte0 is the car's own preconditioning byte, with bit2 clear.
  assert((transmitted[0] & PREHEAT_MANUAL_REQUEST_BIT) == 0);
  assert((transmitted[0] & PREHEAT_SOURCE_MASK) == PREHEAT_SOURCE_MASK);
  assert(transmitted[0] == 0xABu);
  // Idempotent under the byte0 rule: re-reading what went out reproduces it. A
  // parked frame would move instead, which is what the rule is for.
  assert(preheatEchoByte0Pure(transmitted[0]) == transmitted[0]);
  assert(preheatEchoByte0Pure(carPreheat[0]) == carPreheat[0]);
  assert(preheatEchoByte0Pure(liveIdle[0]) != liveIdle[0]);
  assert(preheatEchoByte0Pure(0x01u) == 0xA1u);

  // ---- the witnesses the car's own frames feed ---------------------------
  // The payload never reads them; they are what /api/preheat/stats reports about
  // the car, and they are why a field report can be checked without a CAN log.
  assert(preheatSourceSeenPure(carPreheat, 8));
  // bit1 UI_navToSupercharger is NOT what makes the car heat: a research
  // capture has 01 50 C8 03 FF 03 BE 13 - plan active, NOT to a charger, bit1
  // clear - while that car was heating its own pack. So the home-route frame has
  // to read false here and is still a frame the car heats on.
  assert((routeHomeLive[0] & 0x02u) == 0u);
  assert(!preheatSourceSeenPure(routeHomeLive, 8));
  assert(!preheatSourceSeenPure(liveIdle, 8));
  assert(!preheatSourceSeenPure(nullptr, 8));

  // The target decodes to 50.0 C at the DBC's 0.25 C per count, sits inside the
  // field's own range (0..63.75 C) and is not the SNA marker (0xFF). 50.0 C is the
  // value the field already accepted.
  assert(carPreheat[2] == 0xC8u && transmitted[2] == 0xC8u);
  assert((uint32_t)transmitted[2] * 25u == 5000u);
  assert((uint32_t)transmitted[2] * 25u <= 6375u);
  assert(transmitted[2] != 0xFFu);
  assert(PREHEAT_TARGET_RAW == transmitted[2]);
  assert(PREHEAT_TARGET_RAW == 0xC8u);
  // One shape, one cadence: 10 Hz. Every cadence assertion further down is written
  // against the constant, and this is the one that pins the constant itself.
  assert(PREHEAT_PERIOD_MS == 100u);

  // The energies the car itself carries on that frame, in DBC order. Raw is
  // already kWh x100, and the fixture stays a fact about the car.
  uint16_t energyRaw = 0;
  assert(preheatEnergyRawPure(carPreheat, 8, 0, energyRaw) && energyRaw == 5064);
  assert(preheatEnergyRawPure(carPreheat, 8, 1, energyRaw) && energyRaw == 1023);
  assert(preheatEnergyRawPure(carPreheat, 8, 2, energyRaw) && energyRaw == 5262);
  // The parked payload's two SNA fields have the sign bit set, which is the whole
  // validity test; its UI_hindsightEnergy is a real 10.23 kWh and stays readable.
  assert(preheatEnergyRawPure(liveIdle, 8, 0, energyRaw) && energyRaw == 0x80FF);
  assert(preheatEnergyRawPure(liveIdle, 8, 1, energyRaw) && energyRaw == 1023);
  assert(preheatEnergyRawPure(liveIdle, 8, 2, energyRaw) && energyRaw == 0x8000);
  assert(!preheatEnergyRawPure(liveIdle, 7, 2, energyRaw));  // field 2 needs 8 bytes
  assert(!preheatEnergyRawPure(nullptr, 8, 0, energyRaw));
  assert(!preheatEnergyRawPure(liveIdle, 8, PREHEAT_ENERGY_FIELDS, energyRaw));
  assert(!preheatEnergiesValidPure(nullptr, 8));
  assert(!preheatEnergiesValidPure(liveIdle, 6));
  // The constant and the bit groups must agree.
  assert(PREHEAT_OVERRIDE_BYTE0 == PREHEAT_TRIP_PLAN_BYTE0);
  assert(PREHEAT_OVERRIDE_BYTE0 == 0xAB);
  assert((PREHEAT_OVERRIDE_BYTE0 & PREHEAT_MANUAL_REQUEST_BIT) == 0);
  assert((PREHEAT_OVERRIDE_BYTE0 & PREHEAT_SOURCE_MASK) == PREHEAT_SOURCE_MASK);
  assert(PREHEAT_OVERRIDE_BYTE0 == transmitted[0]);

  // ---- the stall clock ---------------------------------------------------
  // zeroSinceMs is 0 while the clock is not running, which is why the boot grace
  // and the MCP warmup cannot trip the criterion: the runtime arms it on the
  // session's first successful transmission. The stop it fires is silent - no
  // alarm row and no count of the silence lives here any more, so the criterion
  // itself is the whole contract.
  assert(!preheatStatorStallElapsedPure(500000u, 0u));
}
  // ---------------------------------------------------------------------
  // 0x082 source frame - preheatSourceSeenPure
  // ---------------------------------------------------------------------
  const uint8_t srcIdle[8] = {0x00, 0x7F, 0xFF, 0x80, 0xFF, 0x03, 0x00, 0x80};
  const uint8_t srcNavOnly[8] = {0x0A, 0x7F, 0xFF, 0x80, 0xFF, 0x03, 0x00, 0x80};
  const uint8_t srcActive[8] = {0xAB, 0x50, 0xC8, 0x13, 0xFF, 0x03, 0x8E, 0x14};
  assert(!preheatSourceSeenPure(srcIdle, 8));      // no route
  assert(!preheatSourceSeenPure(srcNavOnly, 8));   // bit1 only, trip planning not latched
  assert(preheatSourceSeenPure(srcActive, 8));     // bit0 + bit1
  assert(!preheatSourceSeenPure(nullptr, 8));
  assert(!preheatSourceSeenPure(srcActive, 0));

  // ---------------------------------------------------------------------
  // 0x332 pack temperature - mux 0 only
  // ---------------------------------------------------------------------
  const uint8_t brickMux0[6] = {0x04, 0x05, 0x68, 0x67, 0x67, 0x00};
  const uint8_t brickMux1[6] = {0xC5, 0x1F, 0xEE, 0x07, 0x25, 0x02};
  int16_t tmin = 0, tmax = 0;
  assert(preheatDecodeBrickTempsPure(brickMux0, 6, tmin, tmax));
  assert(tmin == 115);  // 0x67 * 0.5 - 40 = 11.5 C
  assert(tmax == 120);  // 0x68 * 0.5 - 40 = 12.0 C
  assert(tmax >= tmin);
  // Mux 1 carries cell voltages. Decoding it as temperature is the exact bug
  // that produced "Tmin > Tmax"; the guard must reject it.
  assert(!preheatDecodeBrickTempsPure(brickMux1, 6, tmin, tmax));
  assert(!preheatDecodeBrickTempsPure(brickMux0, 5, tmin, tmax));

  // ---------------------------------------------------------------------
  // 0x252 heat power - bits 32..41
  // ---------------------------------------------------------------------
  const uint8_t powerIdle[8] = {0x00, 0x00, 0x2C, 0x07, 0x00, 0x78, 0x17, 0x00};
  const uint8_t powerHeating[8] = {0x00, 0x00, 0x2C, 0x07, 0x22, 0x7B, 0x07, 0x00};
  const uint8_t powerPeak[8] = {0x00, 0x00, 0x28, 0x07, 0x22, 0x7B, 0x07, 0x00};
  uint32_t kwX100 = 0xFFFFFFFFu;
  assert(preheatDecodeHeatPowerPure(powerIdle, 8, kwX100));
  assert(kwX100 == 0);      // 0.00 kW before the event
  assert(preheatDecodeHeatPowerPure(powerHeating, 8, kwX100));
  assert(kwX100 == 802);    // 8.02 kW at the peak
  assert(preheatDecodeHeatPowerPure(powerPeak, 8, kwX100));
  assert(kwX100 == 802);    // bytes 2..3 are regen/discharge and must not leak in
  assert(!preheatDecodeHeatPowerPure(powerHeating, 7, kwX100));

  // ---------------------------------------------------------------------
  // 0x241 coolant flow + reason - DLC 7
  // ---------------------------------------------------------------------
  const uint8_t coolantPre[7] = {0x40, 0x3E, 0x1C, 0x90, 0x0F, 0x57, 0x00};
  const uint8_t coolantHeating[7] = {0x4A, 0x3A, 0x9A, 0x92, 0x8E, 0x66, 0x00};
  uint16_t lpmX10 = 0;
  uint8_t reason = 0xFF;
  assert(preheatDecodeCoolantPure(coolantPre, 7, lpmX10, reason));
  assert(lpmX10 == 64);                          // 6.4 LPM
  // 14 = HP_FLOW_REQ: the heat pump asking for pack coolant flow, which is what
  // the capture carries once the pack starts heating. Read at bit 17 length 5 as
  // the car's own map has it; the earlier bit-18/length-4 read turned this same
  // frame into 7 and labelled it DAS_FLOW_REQ.
  assert(reason == 14);
  assert(preheatDecodeCoolantPure(coolantHeating, 7, lpmX10, reason));
  assert(lpmX10 == 74);                          // 7.4 LPM - rose during preheat
  assert(reason == 13);                          // 13 = DIS_FLOW_REQ, same capture
  assert(!preheatDecodeCoolantPure(coolantHeating, 6, lpmX10, reason));

  // ---------------------------------------------------------------------
  // 0x212 BMS status - the one preconditioning flag this frame carries
  // ---------------------------------------------------------------------
  const uint8_t bmsStatus[8] = {0x58, 0x04, 0x00, 0x00, 0x82, 0xFF, 0x07, 0xC4};
  bool allowed = false;
  assert(preheatDecodeBmsStatusPure(bmsStatus, 8, allowed));
  assert(allowed);          // bit 3 = 1 for all 268 captured frames
  assert(!preheatDecodeBmsStatusPure(bmsStatus, 7, allowed));
  // BMS_preconditionAllowed is the only preconditioning flag here: bit 5 of this
  // frame is BMS_cpMiaOnHvs and bit 30 is BMS_chargeRequest, so the old
  // "worthwhile"/"keep-warm" reads were two other signals. The verdict the tab
  // reports travels in 0x312 mux 0 and is pinned with the thermal block below.

  // ---------------------------------------------------------------------
  // 0x2E1 isActiveHeatingBattery - mux 0 only
  // ---------------------------------------------------------------------
  const uint8_t vcfOff[8] = {0x10, 0x21, 0x04, 0x00, 0x12, 0x00, 0x20, 0x08};
  const uint8_t vcfOn[8] = {0x10, 0x21, 0x04, 0x00, 0x12, 0x00, 0x20, 0x88};
  const uint8_t vcfMux1[8] = {0x29, 0x6A, 0x43, 0xE2, 0x1F, 0x29, 0x10, 0x00};
  bool active = true;
  assert(preheatDecodeHeatingActivePure(vcfOff, 8, active));
  assert(!active);                                              // 0 before +14.73 s
  assert(preheatDecodeHeatingActivePure(vcfOn, 8, active));
  assert(active);                                               // 1 after +14.73 s
  assert(!preheatDecodeHeatingActivePure(vcfMux1, 8, active));  // mux 1 rejected
  assert(!preheatDecodeHeatingActivePure(vcfOn, 7, active));

  // ---------------------------------------------------------------------
  // Eligibility cascade - first failing gate wins
  // ---------------------------------------------------------------------
  assert(preheatEligibilityReasonPure(false, true, true, true, true, true) ==
         PREHEAT_SKIP_DISABLED);
  assert(preheatEligibilityReasonPure(true, false, true, true, true, true) ==
         PREHEAT_SKIP_UNSUPPORTED);
  assert(preheatEligibilityReasonPure(true, true, false, true, true, true) ==
         PREHEAT_SKIP_BOOT_DELAY);
  assert(preheatEligibilityReasonPure(true, true, true, false, true, true) ==
         PREHEAT_SKIP_WARMUP);
  assert(preheatEligibilityReasonPure(true, true, true, true, false, true) ==
         PREHEAT_SKIP_BUS_SILENT);
  // A silent bus is the only liveness stop there is. Neither the 0x082 template age
  // nor the absence of a source frame gates anything now: the payload is a constant
  // and the bus itself is what has to be alive. The template-age gate was the
  // "heats then cuts out" regression, where 0x082 gapped 13.0 s while the
  // Vehicle/Body bus kept delivering frames.
  assert(preheatEligibilityReasonPure(true, true, true, true, true, true) ==
         PREHEAT_SKIP_NONE);
  // A live bus whose car has not yet sent its trip-planning frame is waiting for
  // the ID (0x082 or 0x347), and it is the last gate: everything above outranks it.
  assert(preheatEligibilityReasonPure(true, true, true, true, true, false) ==
         PREHEAT_SKIP_NO_TRIP_ID);
  assert(preheatEligibilityReasonPure(true, true, true, true, false, false) ==
         PREHEAT_SKIP_BUS_SILENT);
  assert(preheatStalledPure(true, true, PREHEAT_SKIP_NO_TRIP_ID));
  // The disabled gate outranks every other reason.
  assert(preheatEligibilityReasonPure(false, false, false, false, false, false) ==
         PREHEAT_SKIP_DISABLED);

  // ---------------------------------------------------------------------
  // Bus liveness mask - the profile picks the Vehicle/Body channel
  // ---------------------------------------------------------------------
  // YL_PARTY_VH: Vehicle/Body is CAN B, so CAN B alone suffices even while the
  // Party bus on CAN A is silent.
  assert(preheatBusFreshPure(0x02u, 0x02u));
  assert(!preheatBusFreshPure(0x01u, 0x02u));
  assert(!preheatBusFreshPure(0x00u, 0x02u));
  // STANDARD_BODY_CHASSIS: Vehicle/Body is CAN A - the mirror image.
  assert(preheatBusFreshPure(0x01u, 0x01u));
  assert(!preheatBusFreshPure(0x02u, 0x01u));
  // Both fresh satisfies either requirement.
  assert(preheatBusFreshPure(0x03u, 0x01u));
  assert(preheatBusFreshPure(0x03u, 0x02u));
  // A zero requirement is never satisfied, so a profile with no Vehicle/Body
  // channel can never look fresh by accident.
  assert(!preheatBusFreshPure(0x03u, 0x00u));

  // ---------------------------------------------------------------------
  // Auto-shutoff target - the car's own pack AVERAGE reaching the target stops it
  // ---------------------------------------------------------------------
  // The number compared is BMS_thermistorTAvg (0x332 mux 0), read by
  // preheatDecodeBrickTempAvgPure: the same value the PACK TEMP cell prints. The
  // boundary is exact, so "at" the target counts as reached and one 0.5 C step
  // below it does not.
  assert(preheatTargetReachedPure(true, 100, 400, 400, PREHEAT_PACK_FRESH_MS));
  assert(preheatTargetReachedPure(true, 100, 405, 400, PREHEAT_PACK_FRESH_MS));
  assert(!preheatTargetReachedPure(true, 100, 395, 400, PREHEAT_PACK_FRESH_MS));
  // The case that separates this criterion from the mean of the two cells the same
  // frame carries, written out: a pack at 39.0..40.5 derives 39.75, which prints as
  // 39.8 and would not have reached a 40.0 target while the car's own average
  // reports 40.5. The pair disagrees with the car in 111 of the 180 mux-0 frames of
  // the eight captures, so this is the ordinary case rather than an edge, and the
  // 0x332 fixture below repeats it on real bytes.
  const int16_t derivedCx10 = (int16_t)((390 + 405 + 1) / 2);
  assert(derivedCx10 == 398 && derivedCx10 < PREHEAT_TARGET_DEFAULT_CX10);
  assert(preheatTargetReachedPure(true, 100, 405, PREHEAT_TARGET_DEFAULT_CX10,
                                  PREHEAT_PACK_FRESH_MS));
  // An absent reading never stops the heat, however hot the last value was.
  assert(!preheatTargetReachedPure(false, 0, 400, 200, PREHEAT_PACK_FRESH_MS));
  assert(!preheatTargetReachedPure(false, 999999, 400, 200, PREHEAT_PACK_FRESH_MS));
  // Freshness boundary, then the dangerous case: a hot but stale reading must
  // not fire, or enabling on a warm pack would cut the heat instantly.
  assert(preheatTargetReachedPure(true, PREHEAT_PACK_FRESH_MS, 400, 200,
                                  PREHEAT_PACK_FRESH_MS));
  assert(!preheatTargetReachedPure(true, PREHEAT_PACK_FRESH_MS + 1u, 400, 200,
                                   PREHEAT_PACK_FRESH_MS));
  assert(!preheatTargetReachedPure(true, 60000u, 400, 200, PREHEAT_PACK_FRESH_MS));

  // ---------------------------------------------------------------------
  // The verdict is exactly the comparison, over the whole grid the bus can report
  // ---------------------------------------------------------------------
  // There is no derived number left to disagree with the printed one: the cell and
  // the engine read the same field, so this is the only invariant to hold. From a
  // winter pack at -10 C to 70 C, at every target the dropdown offers.
  for (int16_t target = PREHEAT_TARGET_MIN_CX10;
       target <= PREHEAT_TARGET_MAX_CX10; target += PREHEAT_TARGET_STEP_CX10) {
    for (int16_t avgCx10 = -100; avgCx10 <= 700; avgCx10 += 5) {
      assert(preheatTargetReachedPure(true, 0, avgCx10, target,
                                      PREHEAT_PACK_FRESH_MS) ==
             (avgCx10 >= target));
    }
  }

  // Range and clamping. The window is the dashboard dropdown's 20..50 C, so the
  // rejected values below are the two either side of each end of it.
  assert(preheatTargetInRangeCx10(200));
  assert(preheatTargetInRangeCx10(500));
  assert(!preheatTargetInRangeCx10(199));
  assert(!preheatTargetInRangeCx10(501));
  assert(preheatClampTargetCx10(-500) == PREHEAT_TARGET_MIN_CX10);
  assert(preheatClampTargetCx10(9999) == PREHEAT_TARGET_MAX_CX10);
  // The API keeps accepting the finer grid: a target saved by the 2 C dropdown
  // that preceded the shipped 5 C one, or by the 0.5 C number field before
  // that, is off the current steps and must still round-trip.
  assert(preheatClampTargetCx10(220) == 220);  // an old 2 C preset
  assert(preheatClampTargetCx10(205) == 205);  // an old 0.5 C value
  assert(preheatTargetInRangeCx10(220));
  // The floor moved up with the grid, so a target stored by the old 10..60 UI
  // clamps into the new window instead of arming a shut-off below it.
  assert(preheatClampTargetCx10(100) == 200);
  assert(PREHEAT_TARGET_DEFAULT_CX10 == 400);
  assert(preheatTargetInRangeCx10(PREHEAT_TARGET_DEFAULT_CX10));
  // Both ends and the default sit on the dropdown's grid, so the UI can never
  // offer a value the API rejects, and the grid has the seven stops the shipped
  // dropdown carries.
  assert(PREHEAT_TARGET_MIN_CX10 % PREHEAT_TARGET_STEP_CX10 == 0);
  assert(PREHEAT_TARGET_MAX_CX10 % PREHEAT_TARGET_STEP_CX10 == 0);
  assert(PREHEAT_TARGET_DEFAULT_CX10 % PREHEAT_TARGET_STEP_CX10 == 0);
  assert((PREHEAT_TARGET_MAX_CX10 - PREHEAT_TARGET_MIN_CX10) /
             PREHEAT_TARGET_STEP_CX10 + 1 == 7);
  // The window tops out at 50 C: a stored 55 C target clamps down to 50.0 C
  // rather than arming a shut-off the UI cannot name.
  assert(PREHEAT_TARGET_MAX_CX10 == 500);
  assert(preheatClampTargetCx10(550) == 500);
  // The default target must not be reachable by a cold pack, or the feature
  // would switch itself off the moment it was enabled. 11.5 C is the reference
  // capture's own average on a parked car.
  assert(!preheatTargetReachedPure(true, 100, 115, PREHEAT_TARGET_DEFAULT_CX10,
                                   PREHEAT_PACK_FRESH_MS));

  // ---------------------------------------------------------------------
  // Cadence and gap tracking
  // ---------------------------------------------------------------------
  assert(preheatDuePure(1000, 0, PREHEAT_PERIOD_MS));               // never sent
  assert(!preheatDuePure(1000, 1000, PREHEAT_PERIOD_MS));           // same tick
  assert(!preheatDuePure(1000u + PREHEAT_PERIOD_MS - 1u, 1000,
                         PREHEAT_PERIOD_MS));                       // one ms short
  assert(preheatDuePure(1000u + PREHEAT_PERIOD_MS, 1000,
                        PREHEAT_PERIOD_MS));                        // exactly a period
  assert(preheatDuePure(5000, 1000, PREHEAT_PERIOD_MS));            // long stall
  // Rollover: nowMs wraps past 0 while lastTxMs sits one period below 2^32, so
  // exactly one period has elapsed. The gap is computed modulo 2^32, so the wrap
  // must not read as a huge or negative gap. The old literal (now=50,
  // last=UINT32_MAX-10) was a 61 ms gap, i.e. not due, and failed the assert.
  assert(preheatDuePure(0u, (uint32_t)0u - PREHEAT_PERIOD_MS, PREHEAT_PERIOD_MS));

  assert(preheatGapMaxUpdatePure(0, 5000, 0) == 0);              // no baseline yet
  assert(preheatGapMaxUpdatePure(1000, 1100, 0) == 100);
  assert(preheatGapMaxUpdatePure(1000, 1100, 250) == 250);       // keeps the max
  assert(preheatGapMaxUpdatePure(1000, 1400, 250) == 400);       // new max

  // ---------------------------------------------------------------------
  // Self-echo rejection
  // ---------------------------------------------------------------------
  const uint8_t ownTx[8] = {0xAB, 0x50, 0xDC, 0x13, 0xFF, 0x03, 0x8E, 0x14};
  assert(preheatIsSelfEchoPure(true, 5, ownTx, ownTx, 8));        // our frame back
  assert(!preheatIsSelfEchoPure(false, 5, srcActive, ownTx, 8));  // never sent
  assert(!preheatIsSelfEchoPure(true, 50, srcActive, ownTx, 8));  // too old
  assert(!preheatIsSelfEchoPure(true, 5, srcIdle, ownTx, 8));     // stock, not ours
  assert(!preheatIsSelfEchoPure(true, 5, srcActive, ownTx, 7));   // wrong DLC

  // ---------------------------------------------------------------------
  // Stalled state - the one thing the strip must not paint as "Heating"
  // ---------------------------------------------------------------------
  // Armed plus any gate is a stall, and the strip names the gate instead of
  // claiming a transmission the engine is not making. This is the state the
  // field report could not tell apart from a working preheat.
  assert(preheatStalledPure(true, true, PREHEAT_SKIP_BOOT_DELAY));
  assert(preheatStalledPure(true, true, PREHEAT_SKIP_WARMUP));
  assert(preheatStalledPure(true, true, PREHEAT_SKIP_BUS_SILENT));
  assert(!preheatStalledPure(true, true, PREHEAT_SKIP_NONE));  // transmitting
  // Neither of these is a stall: the tab renders "Off" and "Unavailable" on its
  // own, and the cascade only returns them when the matching flag is false.
  assert(!preheatStalledPure(false, true, PREHEAT_SKIP_DISABLED));
  assert(!preheatStalledPure(true, false, PREHEAT_SKIP_UNSUPPORTED));
  assert(!preheatStalledPure(false, false, PREHEAT_SKIP_UNSUPPORTED));
  // The two inputs are independent: a stopped engine is never stalled even if
  // its last evaluation was a gate, which is what keeps the strip from naming a
  // gate while the toggle is down.
  assert(!preheatStalledPure(false, true, PREHEAT_SKIP_NONE));
  assert(!preheatStalledPure(false, true, PREHEAT_SKIP_BUS_SILENT));

  // ---------------------------------------------------------------------
  // Reason names are part of the JSON contract
  // ---------------------------------------------------------------------
  assert(preheatSkipReasonName(PREHEAT_SKIP_NONE)[0] == 'N');
  assert(preheatSkipReasonName(PREHEAT_SKIP_UNSUPPORTED)[0] == 'U');
  assert(preheatSkipReasonName(PREHEAT_SKIP_BUS_SILENT)[0] == 'B');
  assert(preheatSkipReasonName(200)[0] == 'U');  // unknown falls back, never UB
  assert(preheatSkipReasonName(PREHEAT_SKIP_NO_TRIP_ID)[0] == 'N');
  assert(PREHEAT_SKIP_COUNT == 7);

  // ---------------------------------------------------------------------
  // 0x332 mux 0 - the pack average the car computes itself
  // ---------------------------------------------------------------------
  // Taken from the same frame as the temperature fixture above. The car reported
  // 11.5 C while (min + max) / 2 is 11.8 C, and this decoder is now the source of
  // BOTH the PACK TEMP cell and the auto-off criterion, so the two can no longer
  // disagree about the pack.
  int16_t tavg = 0;
  assert(preheatDecodeBrickTempAvgPure(brickMux0, 6, tavg));
  assert(tavg == 115);   // 0x67 * 0.5 - 40 = 11.5 C
  assert(!preheatDecodeBrickTempAvgPure(brickMux1, 6, tavg));  // mux 1 is voltages
  assert(!preheatDecodeBrickTempAvgPure(brickMux0, 5, tavg));
  // The discriminating case on real bytes: a target of 11.8 C - exactly the mean of
  // the two cells in this frame - is NOT reached, because the car's own average is
  // 11.5 C. The mean-based rule this replaced would have stopped the heat there.
  assert(!preheatTargetReachedPure(true, 0, 115, 118, PREHEAT_PACK_FRESH_MS));
  assert(preheatTargetReachedPure(true, 0, (int16_t)((115 + 120 + 1) / 2), 118,
                                  PREHEAT_PACK_FRESH_MS));

  // ---------------------------------------------------------------------
  // 0x312 BMS_thermalStatus - pack temperature and flow request (mux 0)
  // ---------------------------------------------------------------------
  const uint8_t thermalMux0[8] = {0x00, 0xE5, 0x66, 0x04, 0xF5, 0x80, 0x10, 0x15};
  // The same frame with bit 30 up: BMS_activeHeatingWorthwhile, the BMS's own
  // "heating this pack is worth it" verdict, which is where the car keeps it - in
  // the mux that also carries the temperature it is a verdict about.
  const uint8_t thermalMux0Worthwhile[8] = {0x80, 0xC5, 0x4A, 0x4C, 0xF5, 0x80, 0x10, 0x15};
  const uint8_t thermalMux1Cold[8] = {0xC9, 0x21, 0x48, 0x17, 0x40, 0x8D, 0x00, 0x00};
  const uint8_t thermalMux1Warm[8] = {0xB1, 0x64, 0x49, 0x17, 0x40, 0x57, 0x02, 0x00};
  int16_t maxPackCx10 = 0;
  uint8_t flowReqLpmX4 = 0;
  bool worthwhile = false;
  assert(preheatDecodeThermalMux0Pure(thermalMux0, 8, maxPackCx10, flowReqLpmX4,
                                      worthwhile));
  assert(maxPackCx10 == 128);  // 12.8 C
  assert(flowReqLpmX4 == 21);  // 5.25 LPM
  assert(!worthwhile);         // bit 30 = 0 in this frame
  assert(preheatDecodeThermalMux0Pure(thermalMux0Worthwhile, 8, maxPackCx10,
                                      flowReqLpmX4, worthwhile));
  assert(maxPackCx10 == 128);  // the temperature is the same, the verdict is not
  assert(worthwhile);
  // Mux 1 carries the cell target instead. Reading it through the mux-0 decoder
  // is how this frame produced a number that belonged to another signal.
  assert(!preheatDecodeThermalMux0Pure(thermalMux1Warm, 8, maxPackCx10,
                                       flowReqLpmX4, worthwhile));
  assert(!preheatDecodeThermalMux0Pure(thermalMux0, 7, maxPackCx10, flowReqLpmX4,
                                       worthwhile));

  // ---------------------------------------------------------------------
  // 0x312 mux 1 - BMS_activeHeatCellTargetT
  // ---------------------------------------------------------------------
  int16_t cellTargetCx4 = 0;
  assert(preheatDecodeThermalMux1Pure(thermalMux1Cold, 8, cellTargetCx4));
  assert(cellTargetCx4 == -30);  // -7.50 C while the car was parked
  assert(preheatDecodeThermalMux1Pure(thermalMux1Warm, 8, cellTargetCx4));
  // 49.75 C: the car aimed at the 50.0 C that the 0x082 request asked for.
  assert(cellTargetCx4 == 199);
  assert(!preheatDecodeThermalMux1Pure(thermalMux0, 8, cellTargetCx4));
  assert(!preheatDecodeThermalMux1Pure(thermalMux1Warm, 7, cellTargetCx4));

  // ---------------------------------------------------------------------
  // 0x3A1 mux 1 - VCFRONT_hpBattHeatingPowerTrgt
  // ---------------------------------------------------------------------
  const uint8_t hpPowerIdle[8] = {0xEB, 0xD3, 0x40, 0x65, 0x3E, 0x00, 0x70, 0xD1};
  const uint8_t hpPowerHeating[8] = {0xEB, 0xD3, 0x40, 0x23, 0x3E, 0x78, 0x10, 0xA7};
  const uint8_t vcStatusMux0[8] = {0xE4, 0x42, 0x00, 0xCA, 0x00, 0x7C, 0xA2, 0xCE};
  uint32_t watts = 0xFFFFFFFFu;
  assert(preheatDecodeHpBattHeatPowerPure(hpPowerIdle, 8, watts));
  assert(watts == 0);
  assert(preheatDecodeHpBattHeatPowerPure(hpPowerHeating, 8, watts));
  assert(watts == 6000);  // 6 kW, inside the 8.02 kW the pack went on to take
  // Mux 0 is a different signal set and must never be read as a power target.
  assert(!preheatDecodeHpBattHeatPowerPure(vcStatusMux0, 8, watts));
  assert(!preheatDecodeHpBattHeatPowerPure(hpPowerHeating, 7, watts));

  // ---------------------------------------------------------------------
  // 0x201 - VCFRONT_pumpBatteryRPMActual
  // ---------------------------------------------------------------------
  const uint8_t pumpSlow[8] = {0xD2, 0xD0, 0x00, 0x00, 0x40, 0x46, 0x9E, 0x39};
  const uint8_t pumpRun[8] = {0xD0, 0x9A, 0x02, 0xD0, 0x20, 0xFF, 0xF1, 0x05};
  uint32_t rpm = 0xFFFFFFFFu;
  // pumpSlow is an index-2 frame (D2 = ...010): its "780 rpm" was never the pump.
  // The frame is indexed and the pump lives on index 0 only.
  assert(!preheatDecodePumpBatteryRpmPure(pumpSlow, 8, rpm));
  assert(rpm == 0xFFFFFFFFu);
  assert(preheatDecodePumpBatteryRpmPure(pumpRun, 8, rpm));
  assert(rpm == 2700);
  assert(!preheatDecodePumpBatteryRpmPure(pumpRun, 7, rpm));

  // ---------------------------------------------------------------------
  // 0x321 - VCFRONT_tempAmbient, byte3
  // ---------------------------------------------------------------------
  // The car's own outside-air reading, and the witness the AMBIENT cell prints.
  // Both fixtures are verbatim: the reference capture carries byte3 = 0x61 in all
  // 89 of its frames (8.5 C), and a PASSIVE capture 0x67 in all 369
  // (11.5 C) - constant per session, which is what makes a wrong byte obvious.
  const uint8_t sensorsAmbient[8] = {0x8D, 0x69, 0xA6, 0x61, 0x01, 0x61, 0x90, 0x13};
  const uint8_t sensorsPassive[8] = {0xA9, 0xB5, 0xA6, 0x67, 0x02, 0x67, 0x50, 0x48};
  int16_t ambientCx10 = 0;
  assert(preheatDecodeAmbientPure(sensorsAmbient, 8, ambientCx10));
  assert(ambientCx10 == 85);   // 0x61 * 0.5 - 40 = 8.5 C
  assert(preheatDecodeAmbientPure(sensorsPassive, 8, ambientCx10));
  assert(ambientCx10 == 115);  // 0x67 * 0.5 - 40 = 11.5 C
  assert(!preheatDecodeAmbientPure(sensorsAmbient, 7, ambientCx10));
  assert(!preheatDecodeAmbientPure(nullptr, 8, ambientCx10));
  // Raw 0 is the field's own SNA marker (VAL_ 801 VCFRONT_tempAmbient 0 "SNA"), so
  // it is refused rather than reported as -40.0 C. Nothing here writes through, so
  // the caller's value must survive the refusal untouched.
  const uint8_t sensorsSna[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  ambientCx10 = 999;
  assert(!preheatDecodeAmbientPure(sensorsSna, 8, ambientCx10));
  assert(ambientCx10 == 999);

  // ---------------------------------------------------------------------
  // Stator heating - 0x2E5 (front drive unit) and 0x266 (rear)
  //
  // The only honest kW reading of pack heating on this bus, and the verdict this
  // feature is judged by now: the multiplexed VCFRONT logging frame that also
  // carries a total (0x381 totalBattHeatingPower) has no index tag in the car's
  // map, so nothing in it decodes. Both fixtures are verbatim from the captures -
  // the Supercharger-route snapshot of a route research capture, taken
  // while the pack was heating, and the parked/home-route shape that reads zero.
  // ---------------------------------------------------------------------
  {
    const uint8_t statorHeating[8] = {0x03, 0x00, 0x03, 0x20, 0x21, 0x1C, 0xD0, 0x00};
    const uint8_t statorIdle[8] = {0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00};
    uint32_t kwX100 = 0xFFFFFFFFu;
    assert(preheatDecodeStatorHeatPure(statorHeating, 8, kwX100));
    assert(kwX100 == 264);   // 2.64 kW, the reading five seconds into that session
    // The neighbouring byte is the same signal's maximum (0x20 -> 2.56 kW), which
    // is the cross-check that the 0.08 kW scale is right: actual tracks it.
    assert((uint32_t)statorHeating[3] * 8u == 256);
    assert(preheatDecodeStatorHeatPure(statorIdle, 8, kwX100));
    assert(kwX100 == 0);
    assert(!preheatDecodeStatorHeatPure(statorHeating, 7, kwX100));
    assert(!preheatDecodeStatorHeatPure(nullptr, 8, kwX100));
    assert(PREHEAT_STATOR_DIF_ID == 0x2E5u);
    assert(PREHEAT_STATOR_DIR_ID == 0x266u);
    // The freshness window both the strip's "current" reading and the stall
    // criterion use: a side older than this is not a reading of now.
    assert(PREHEAT_STATOR_PAIR_FRESH_MS == 2000u);
  }


// ---------------------------------------------------------------------
  // Auto-off: terminal state changes, and the stall criterion
  // ---------------------------------------------------------------------
  // The tab paints these names, so they are part of the contract.
  assert(strcmp(preheatAutoOffReasonName(PREHEAT_AUTOOFF_NONE), "NONE") == 0);
  assert(strcmp(preheatAutoOffReasonName(PREHEAT_AUTOOFF_TARGET),
                "TARGET_REACHED") == 0);
  assert(strcmp(preheatAutoOffReasonName(PREHEAT_AUTOOFF_STATOR_STALL),
                "STATOR_STALL") == 0);
  assert(strcmp(preheatAutoOffReasonName(99u), "UNKNOWN") == 0);
  // Thirty seconds of a silent pair, not 29.9: the boundary is the moment the
  // engine switches itself off.
  assert(PREHEAT_STATOR_STALL_OFF_MS == 30000u);
  assert(!preheatStatorStallElapsedPure(1000u + 29999u, 1000u));
  assert(preheatStatorStallElapsedPure(1000u + 30000u, 1000u));
  assert(preheatStatorStallElapsedPure(1000u + 60000u, 1000u));
  // A stopped clock never fires. 0 means "no frame has gone out yet", which is
  // exactly why the boot grace and the MCP warmup cannot trip the criterion: the
  // runtime arms the clock on the session's first successful transmission.
  assert(!preheatStatorStallElapsedPure(500000u, 0u));
  // Wrap-safe, like the cadence assertions above.
  assert(preheatStatorStallElapsedPure(
      0u, (uint32_t)0u - PREHEAT_STATOR_STALL_OFF_MS));
  // A stale start that has wrapped past zero: sinceMs = UINT32_MAX, nowMs = 0, so
  // exactly 1 ms has elapsed and the window is nowhere near closed.
  assert(!preheatStatorStallElapsedPure(0u, (uint32_t)0u - 1u));

  // ---------------------------------------------------------------------
  // Trip ID - 0x082 on the HW3 test car, 0x347 on the HW4 one
  // ---------------------------------------------------------------------
  // Every 0x347 payload of the HW4 capture, in the order segment A walked
  // them, has the trip shape; so does the HW3 car's 0x082.
  const uint8_t hw4Frames[6][8] = {
      {0x01, 0x1A, 0x9F, 0x1D, 0xFF, 0x03, 0x2F, 0x08},
      {0x00, 0x1A, 0x9F, 0x80, 0xFF, 0x03, 0x00, 0x80},
      {0xAE, 0x1A, 0x9F, 0x80, 0xFF, 0x03, 0x00, 0x80},
      {0xAF, 0x24, 0x9D, 0x1D, 0xFF, 0x03, 0xF2, 0x07},
      {0xAF, 0x1E, 0x9D, 0x1D, 0xFF, 0x03, 0xFC, 0x07},
      {0x01, 0x21, 0x9D, 0x1D, 0xFF, 0x03, 0xEC, 0x07}};
  for (const auto &f : hw4Frames) {
    assert(preheatTripShapePure(f, 8));
    assert(preheatTripIdNextPure(PREHEAT_TRIP_UNKNOWN, 0x347u, 8, f) ==
           PREHEAT_TRIP_347);
  }
  assert(preheatTripShapePure(PREHEAT_REFERENCE_FRAME, 8));
  // The transmitted HW4 frame IS the car's own preconditioning frame, byte for byte.
  for (int i = 0; i < 8; i++)
    assert(PREHEAT_REFERENCE_FRAME_347[i] == hw4Frames[3][i]);
  // An alert matrix on 0x347 (the older maps' CHGPH1) has no hole at bits 42..47,
  // and a short frame is not a trip frame either.
  const uint8_t alert[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  assert(!preheatTripShapePure(alert, 8));
  assert(preheatTripIdNextPure(PREHEAT_TRIP_UNKNOWN, 0x347u, 8, alert) ==
         PREHEAT_TRIP_UNKNOWN);
  assert(!preheatTripShapePure(hw4Frames[3], 6));
  assert(!preheatTripShapePure(nullptr, 8));
  // 0x082 wins outright and for good: a 0x347 after it does not switch the ID,
  // while a 0x082 after a 0x347 does.
  assert(preheatTripIdNextPure(PREHEAT_TRIP_UNKNOWN, 0x082u, 8,
                               PREHEAT_REFERENCE_FRAME) == PREHEAT_TRIP_082);
  assert(preheatTripIdNextPure(PREHEAT_TRIP_082, 0x347u, 8, hw4Frames[3]) ==
         PREHEAT_TRIP_082);
  assert(preheatTripIdNextPure(PREHEAT_TRIP_347, 0x082u, 8,
                               PREHEAT_REFERENCE_FRAME) == PREHEAT_TRIP_082);
  // Any other ID leaves the latch alone.
  assert(preheatTripIdNextPure(PREHEAT_TRIP_347, 0x3A1u, 8, hw4Frames[3]) ==
         PREHEAT_TRIP_347);
  assert(preheatTripIdNextPure(PREHEAT_TRIP_UNKNOWN, 0x082u, 6,
                               PREHEAT_REFERENCE_FRAME) == PREHEAT_TRIP_UNKNOWN);
  // ID and frame follow the latch; UNKNOWN never transmits (the cascade holds it),
  // and maps to the 0x082 frame only so the selectors are total.
  assert(preheatTripCanIdPure(PREHEAT_TRIP_082) == 0x082u);
  assert(preheatTripCanIdPure(PREHEAT_TRIP_347) == 0x347u);
  assert(preheatTripFramePure(PREHEAT_TRIP_082) == PREHEAT_REFERENCE_FRAME);
  assert(preheatTripFramePure(PREHEAT_TRIP_347) == PREHEAT_REFERENCE_FRAME_347);
  assert(preheatTripIdName(PREHEAT_TRIP_082)[3] == '8');
  assert(preheatTripIdName(PREHEAT_TRIP_347)[3] == '4');
  assert(preheatTripIdName(PREHEAT_TRIP_UNKNOWN)[0] == 'u');

  // ---------------------------------------------------------------------
  // Heat witness of the stall criterion
  // ---------------------------------------------------------------------
  // 0x082: the stator pair, exactly as before - the hp request does not enter.
  assert(preheatHeatWitnessHotPure(PREHEAT_TRIP_082, true, false, 0u, 0u));
  assert(!preheatHeatWitnessHotPure(PREHEAT_TRIP_082, false, true, 0u, 4500u));
  // 0x347: 0x3A1 mux 1 hpBattHeatingPowerTrgt. On HW4 the rear stator reads
  // 1.36..1.44 kW with no route at all, so it must not keep the engine armed.
  assert(preheatHeatWitnessHotPure(PREHEAT_TRIP_347, false, true, 100u, 4500u));
  assert(!preheatHeatWitnessHotPure(PREHEAT_TRIP_347, true, true, 100u, 0u));
  assert(!preheatHeatWitnessHotPure(PREHEAT_TRIP_347, false, true,
                                    PREHEAT_HP_POWER_FRESH_MS + 1u, 4500u));
  assert(preheatHeatWitnessHotPure(PREHEAT_TRIP_347, false, true,
                                   PREHEAT_HP_POWER_FRESH_MS, 3800u));
  assert(!preheatHeatWitnessHotPure(PREHEAT_TRIP_347, false, false, 0u, 4500u));
  // The HW4 capture's own 0x3A1 mux 1 at +10 s of segment A: 4500 W.
  const uint8_t hw4Hp[8] = {0x29, 0xBB, 0x40, 0xAB, 0x3D, 0x5A, 0xB0, 0xBA};
  uint32_t hpW = 0u;
  assert(preheatDecodeHpBattHeatPowerPure(hw4Hp, 8, hpW));
  assert(hpW == 4500u);
  assert(preheatHeatWitnessHotPure(PREHEAT_TRIP_347, false, true, 118u, hpW));

  // ---------------------------------------------------------------------
  // Immediate reassertion - answer every car trip frame that disagrees
  // ---------------------------------------------------------------------
  // The car's own "no preconditioning" frames from the captures arm an answer on
  // both IDs; so does a REPEAT of the same frame (the car repeats it every ~2 s).
  const uint8_t carIdle[8] = {0x00, 0x7F, 0xFF, 0x80, 0xFF, 0x03, 0x00, 0x80};
  const uint8_t carHome[8] = {0x01, 0x50, 0xC1, 0x13, 0xFF, 0x03, 0xFF, 0x0B};
  assert(preheatReplyArmPure(carIdle, PREHEAT_REFERENCE_FRAME, 8));
  assert(preheatReplyArmPure(carIdle, PREHEAT_REFERENCE_FRAME, 8));  // repeat
  assert(preheatReplyArmPure(carHome, PREHEAT_REFERENCE_FRAME, 8));
  assert(preheatReplyArmPure(hw4Frames[0], PREHEAT_REFERENCE_FRAME_347, 8));
  // Our own frame never arms - which is what makes a loop impossible without a
  // time window - and neither does the car's frame when it already agrees.
  assert(!preheatReplyArmPure(PREHEAT_REFERENCE_FRAME, PREHEAT_REFERENCE_FRAME, 8));
  assert(!preheatReplyArmPure(PREHEAT_REFERENCE_FRAME_347,
                              PREHEAT_REFERENCE_FRAME_347, 8));
  assert(!preheatReplyArmPure(carIdle, PREHEAT_REFERENCE_FRAME, 6));
  assert(!preheatReplyArmPure(nullptr, PREHEAT_REFERENCE_FRAME, 8));
  // One flipped bit is a disagreement.
  uint8_t oneBit[8];
  for (int i = 0; i < 8; i++) oneBit[i] = PREHEAT_REFERENCE_FRAME[i];
  oneBit[7] ^= 0x01;
  assert(preheatReplyArmPure(oneBit, PREHEAT_REFERENCE_FRAME, 8));
  // A 1000-echo storm of our own frame arms nothing.
  int armed = 0;
  for (int i = 0; i < 1000; i++)
    if (preheatReplyArmPure(PREHEAT_REFERENCE_FRAME, PREHEAT_REFERENCE_FRAME, 8))
      armed++;
  assert(armed == 0);

  // The storm guard is measured from the last REPLY, never from the periodic
  // frame, and it is short: a car frame every ~1 s is always answered.
  assert(!preheatReplyDuePure(false, 5000u, 0u));
  assert(preheatReplyDuePure(true, 5000u, 0u));
  assert(!preheatReplyDuePure(true, 5000u + PREHEAT_REPLY_MIN_GAP_MS - 1u, 5000u));
  assert(preheatReplyDuePure(true, 5000u + PREHEAT_REPLY_MIN_GAP_MS, 5000u));
  assert(preheatReplyDuePure(true, 5u, (uint32_t)0u - 100u));  // wrap-safe
  assert(PREHEAT_REPLY_MIN_GAP_MS < PREHEAT_PERIOD_MS);

  // "Near a car frame" is the 3 s the car itself took to stop heating.
  assert(!preheatDropNearCarPure(10000u, 0u));
  assert(preheatDropNearCarPure(10000u, 10000u - PREHEAT_DROP_NEAR_CAR_MS));
  assert(!preheatDropNearCarPure(10000u, 10000u - PREHEAT_DROP_NEAR_CAR_MS - 1u));
  assert(preheatDropNearCarPure(500u, (uint32_t)0u - 1000u));  // wrap-safe

  // ---------------------------------------------------------------------
  // Reply breaker - no loop can exceed 5 answers a second, then answers stop
  // ---------------------------------------------------------------------
  {
    // The car's own pace: one disagreeing frame a second (up to two at a
    // transition) for 1000 s never trips it.
    PreheatReplyBreakerPure b = {};
    for (uint32_t t = 1000u; t < 1001000u; t += 1000u) {
      assert(preheatReplyBreakerAllowPure(b, t));
      assert(preheatReplyBreakerAllowPure(b, t + 300u));
    }
    assert(!b.tripped);
  }
  {
    // A hypothetical node answering every answer of ours 1 ms later (the guard
    // above lets one through every PREHEAT_REPLY_MIN_GAP_MS): five go out, the
    // sixth trips the breaker, and nothing goes out after that, ever.
    PreheatReplyBreakerPure b = {};
    int sent = 0;
    for (uint32_t t = 5000u; t < 65000u; t += PREHEAT_REPLY_MIN_GAP_MS + 1u)
      if (preheatReplyBreakerAllowPure(b, t)) sent++;
    assert(sent == (int)PREHEAT_REPLY_BREAKER_MAX);
    assert(b.tripped);
  }
  {
    // Exactly MAX in one window is allowed; the window restarts after 1000 ms;
    // a window that straddles the counter wrap behaves the same.
    PreheatReplyBreakerPure b = {};
    const uint32_t t0 = (uint32_t)0u - 500u;
    for (uint8_t i = 0; i < PREHEAT_REPLY_BREAKER_MAX; i++)
      assert(preheatReplyBreakerAllowPure(b, t0 + i));
    assert(preheatReplyBreakerAllowPure(b, t0 + PREHEAT_REPLY_BREAKER_WINDOW_MS));
    assert(!b.tripped);
  }

  // ---------------------------------------------------------------------
  // Thermal schematic decoders - real frames of the self-heating capture
  // ---------------------------------------------------------------------
  {
    // 0x201: index 0 carries the pumps and the fan; the other three indices
    // are other fields and are refused.
    const uint8_t v0[8] = {0x58, 0x42, 0x02, 0xD0, 0x20, 0x9B, 0xF1, 0x07};
    const uint8_t v1[8] = {0x01, 0x08, 0x18, 0x01, 0xC0, 0x0B, 0x91, 0xC4};
    const uint8_t v2[8] = {0xD2, 0xD0, 0x00, 0x00, 0x40, 0x46, 0x9E, 0x39};
    const uint8_t v3[8] = {0xD3, 0xD0, 0x00, 0x00, 0x40, 0x46, 0xBE, 0x19};
    uint32_t pb = 0, pp = 0, fan = 0;
    assert(preheatDecodeVitals201Pure(v0, 8, pb, pp, fan));
    assert(pb == 2250 && pp == 2160 && fan == 0);
    assert(!preheatDecodeVitals201Pure(v1, 8, pb, pp, fan));
    assert(!preheatDecodeVitals201Pure(v2, 8, pb, pp, fan));
    assert(!preheatDecodeVitals201Pure(v3, 8, pb, pp, fan));
    assert(!preheatDecodeVitals201Pure(v0, 7, pb, pp, fan));

    // 0x201 index 1: the coolant valve. At rest SERIES / SERIES_BYPASS ("Series
    // Radiator Bypass"), heating AMBIENT_SOURCE / AMBIENT_SOURCE, the transition
    // frame SERIES / AMBIENT_SOURCE (the reference capture); the HW4 car SERIES / ANY.
    const uint8_t m1Amb[8] = {0xA1, 0x08, 0x18, 0x01, 0xE3, 0x0D, 0xA3, 0xC4};
    const uint8_t m1Tr[8] = {0x11, 0x08, 0x18, 0x01, 0xE0, 0x0D, 0xA3, 0xC4};
    const uint8_t m1Hw4[8] = {0x01, 0x08, 0x38, 0x01, 0x60, 0x03, 0x81, 0xC4};
    uint8_t valve = 9, req = 9;
    assert(preheatDecodeCoolantMode201Pure(v1, 8, valve, req) && valve == 0 && req == 2);
    assert(preheatDecodeCoolantMode201Pure(m1Amb, 8, valve, req) && valve == 3 && req == 4);
    assert(preheatDecodeCoolantMode201Pure(m1Tr, 8, valve, req) && valve == 0 && req == 4);
    assert(preheatDecodeCoolantMode201Pure(m1Hw4, 8, valve, req) && valve == 0 && req == 0);
    assert(!preheatDecodeCoolantMode201Pure(v0, 8, valve, req));
    assert(!preheatDecodeCoolantMode201Pure(v2, 8, valve, req));
    assert(!preheatDecodeCoolantMode201Pure(v1, 7, valve, req));

    // 0x3A1 mux 1 bit 5: heat pump on the HW3 test car; mux 0 is refused.
    const uint8_t vs1[8] = {0xEB, 0xD3, 0x40, 0x65, 0x3E, 0x00, 0x70, 0xD1};
    const uint8_t vs0[8] = {0xE4, 0x42, 0x00, 0xCA, 0x00, 0x7C, 0xA2, 0xCE};
    const uint8_t vsLegacy[8] = {0xCB, 0xD3, 0x40, 0x65, 0x3E, 0x00, 0x70, 0xD1};
    bool hp = false;
    assert(preheatDecodeThermalSystemPure(vs1, 8, hp) && hp);
    assert(preheatDecodeThermalSystemPure(vsLegacy, 8, hp) && !hp);
    assert(!preheatDecodeThermalSystemPure(vs0, 8, hp));

    // 0x321: coolant at both loop inlets, 13.1 / 13.6 C on a cold car.
    const uint8_t sensors[8] = {0xA9, 0xB5, 0xA6, 0x67, 0x02, 0x67, 0x50, 0x48};
    int16_t batIn = 0, ptIn = 0;
    assert(preheatDecodeCoolantInletsPure(sensors, 8, batIn, ptIn));
    assert(batIn == 131 && ptIn == 136);

    // 0x241: one flow in both loops - the SERIES signature.
    const uint8_t coolant[7] = {0x35, 0x36, 0x5C, 0x8D, 0x0D, 0x57, 0x00};
    uint16_t fb = 0, fp = 0;
    assert(preheatDecodeLoopFlowsPure(coolant, 7, fb, fp));
    assert(fb == 53 && fp == 53);

    // 0x2A4: signed fields; DC-DC 14.4 C, the hottest charger phase 13.5 C.
    const uint8_t pcs[8] = {0xF5, 0xBE, 0xF7, 0xBD, 0x01, 0x3E, 0xF2, 0x24};
    int16_t dcdc = 0, chg = 0;
    assert(preheatDecodePcsTempsPure(pcs, 8, dcdc, chg));
    assert(dcdc == 144 && chg == 135);

    // 0x315 / 0x376: index 0 only; the stators at rest, rear 21 C and front 15 C
    // (both inverter boards 14 C in byte 2); raw 0 is SNA.
    const uint8_t dir0[8] = {0x10, 0x3D, 0x36, 0x3D, 0x50, 0x36, 0xAA, 0x7D};
    const uint8_t dir1[8] = {0x01, 0x36, 0x36, 0x00, 0x39, 0x00, 0x00, 0x00};
    const uint8_t dif0[8] = {0x10, 0x3D, 0x36, 0x37, 0x50, 0x36, 0x83, 0x7D};
    const uint8_t sna0[8] = {0x10, 0x3D, 0x36, 0x00, 0x50, 0x36, 0x83, 0x7D};
    int16_t inv = 0;
    assert(preheatDecodeStatorTempPure(dir0, 8, inv) && inv == 21);
    assert(preheatDecodeStatorTempPure(dif0, 8, inv) && inv == 15);
    assert(!preheatDecodeStatorTempPure(dir1, 8, inv));
    assert(!preheatDecodeStatorTempPure(sna0, 8, inv));

    // 0x3FC: Autopilot board 32 C; raw 0 is the car's "no sensor".
    const uint8_t app[7] = {0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t appNone[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int16_t ap = 0;
    assert(preheatDecodeApTempPure(app, 7, ap) && ap == 32);
    assert(!preheatDecodeApTempPure(appNone, 7, ap));

    // 0x353: signed CPU temperature, 24 C (279 C if read unsigned).
    const uint8_t ui[8] = {0xFB, 0x0F, 0x39, 0x80, 0x20, 0x0C, 0xF2, 0xF0};
    int16_t cpu = 0;
    assert(preheatDecodeInfotainmentTempPure(ui, 8, cpu) && cpu == 24);
  }
  return 0;
}

