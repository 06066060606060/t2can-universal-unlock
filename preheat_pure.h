#pragma once
#include <stdint.h>

// ===========================================================================
// Battery preheat (Actions tab) - host-testable pure logic.
//
// The engine re-transmits the car's own 0x082 UI_tripPlanning on the
// Vehicle/Body bus as a FLAT CONSTANT, ten times a second, while it is armed:
//
//   out  AB 50 C8 13 FF 03 8E 14   every PREHEAT_PERIOD_MS, byte for byte
//
// Those bytes are not synthesised. They are exactly what the car itself sent in
// the reference capture (the reference capture, +14.328 s) at the moment it
// preconditioned its pack, and they are the payload a field run accepted. The
// car's own 0x082 is still read, but only as a witness: it fills srcRaw,
// srcEnergiesValid and srcRouteBits for GET /api/preheat/stats and never builds
// the payload.
//
// 0x082 is eight bytes, and the DBC map for it is
// Tesla_2026.26.6 -Model3.dbc, BO_ 130 UI_tripPlanning, DLC 8, VehicleBus. Bit
// numbering is @1 (little-endian), so bit N is byte[N>>3] bit (N&7):
//
//   bit  0      UI_tripPlanningActive            0|1@1+
//   bit  1      UI_navToSupercharger             0|1@1+
//   bits 2..4   UI_navFastchargerType            2|3@1+
//   bits 5..6   UI_battPreconditionOnNavState    5|2@1+
//   bit  7      UI_requestActiveBatteryHeating   7|1@1+
//   bits 8..15  UI_battPreconditionOnNavPowerReq 8|8@1-  125 W, SNA 0x7F, MAX 0x7E
//   bits 16..23 UI_battPreconditionOnNavTargetT 16|8@1+ 0.25 C, SNA 0xFF, MAX 0xFE
//   bits 24..31 UI_ambientTempAtDestination     24|8@1- 0.5 C,  SNA 0x80
//   bits 32..41 UI_tripPlanChargingTargetPercent 32|10@1+ 0.1 %, SNA 1023
//   bits 42..47 (unused - no signal)
//   bits 48..63 UI_energyAtDestination          48|16@1- 0.01 kWh, SNA 0x8000
//
// The enumerations matter as much as the widths, because a frame is read by them
// (VAL_ 130, quoted):
//
//   UI_navFastchargerType          0 NONE, 1 LOW_POWER, 2 V2, 3 V3, 4 V4
//   UI_battPreconditionOnNavState  0 PASSIVE_HEAT, 1 ACTIVE_HEAT,
//                                  2 PASSIVE_COOL, 3 ACTIVE_COOL
//   UI_requestActiveBatteryHeating and UI_navToSupercharger / _tripPlanningActive
//                                  plain 0|1, no table
//
// So on the frame this engine sends, AB 50 C8 13 FF 03 8E 14: the plan is active,
// the route is to a Supercharger of type V2, the BMS is being asked for
// ACTIVE_HEAT, the request is 10000 W at a 50.0 C target, the destination ambient
// is 9.5 C and 52.62 kWh are left - and the charging target is SNA (1023), i.e.
// not stated, the same emptiness as the SNA energies.
//
// The frame has no counter and no checksum, so it can be replayed without any
// recalculation - which is what makes a fixed payload legitimate here.
//
// Why a flat constant, and what the field runs established. The car's own frames
// say which shapes mean "heat": UI_battPreconditionOnNavPowerReq and
// UI_battPreconditionOnNavTargetT are SNA exactly in the states where the car
// does not heat, and valid exactly in the states where it does (22 of 22
// snapshots in a research capture). The frame above is the
// preconditioning one - both request fields valid, destination energy valid.
//
//   * A zero payload (05 00 00 00 00 00 00 00) did not heat. The car never puts
//     a zero payload on this bus: bytes 2..7 are live route energies its modules
//     are already decoding.
//   * The right byte0 over SNA energies (AB 7F FF 80 FF 03 00 80) did not heat.
//     Neither did the same with bit2 raised (AF ...).
//   * The car's own preconditioning frame (AB 50 C8 13 FF 03 8E 14) DID heat -
//     measured, not inferred: 0x2E5/0x266 stator heating 0 -> 3.6 kW per side,
//     about 7.2 kW total - and it did so after a car reboot.
//   * Planning a route TO a Supercharger heats the pack with no device help at
//     all (PASSIVE capture, stator 3.60 + 3.60 kW one second after the car's own
//     0x082 became AB 50 C1 14 FF 03 51 0C). A route NOT to a charger heats
//     nothing, and neither did anything this engine transmitted while such a
//     route was active. That confounder is why past "no heat" verdicts taken on
//     a home route have to be read with care.
//   * bit 2 is NOT set here, and the car heated the pack at 8 kW with it clear,
//     so raising it is a deviation from the frame that works rather than the
//     request that makes it work. A static_assert pins it clear so a future
//     revision has to delete the assert on purpose. On the 2026.26 map that bit is
//     the low bit of UI_navFastchargerType, and the car itself leaves it 0 whenever
//     it asks for a V2 charger - which is what its own preconditioning frame does.
//
// What the car does in response is not ours to command. In one field run the engine
// drove a constant AB 50 DC 13 FF 03 8E 14 at 10 Hz onto a car whose own 0x082
// held 01 50 C1 13 FF 03 FF 0B (route active, not to a charger). The stator
// heated in 14 of that capture's 44 snapshots (32 %), and every burst ended with
// the drive units' own coolant request falling back to zero (0x5D7
// DIR_coolantFlowReq 2.8 -> 0.0 LPM, DIR_activeInletTempReq 37 -> 70 C). Two
// readings of it matter, and neither is a property of the payload:
//
//   * 0x241 VCFRONT_coolantFlowBatReason never entered the drive-unit regime
//     (BMS_FLOW_REQ 40 of 44, HP_FLOW_REQ 4, and never DI_FLOW_REQ / DIS_FLOW_REQ),
//     while the car's own preconditioning - a route-to-charger run and the
//     reference capture - always does. The car was never fully in its
//     preconditioning state: its own planner denied for the same ID what the
//     constant frame claimed.
//   * Nothing on the bus led the power: every ID and every byte was correlated
//     against the heating on/off transitions and only the power frames themselves
//     moved with them. The decision is inside the car.
//
// Because the payload is a constant, "nothing heated" cannot be seen on the way
// out - only on the way back. That is what the drive units' own power reading
// (0x2E5 front / 0x266 rear heatPowerActual) and PREHEAT_STATOR_STALL_OFF_MS
// exist for: the first is what the strip shows, the second stops the engine -
// silently, with no alarm on the tab - when thirty seconds of transmitting have
// moved nothing.
//
// The captures record RX only, so no log can show what this engine transmitted.
// A field verdict has to come from GET /api/preheat/stats - lastTxRaw, the src*
// witness fields and autoOffReason - together with the drive units' own reading,
// never from the log alone.
// ===========================================================================

static constexpr uint32_t PREHEAT_SOURCE_ID = 0x082u;
static constexpr uint8_t  PREHEAT_DLC       = 8u;

// byte0 of the transmitted frame: 0xAB is the byte the car itself sent while it
// preconditioned in the reference capture (bit0 UI_tripPlanningActive + bit1
// UI_navToSupercharger + bits 3/5/7). The engine sends it verbatim, and the
// asserts below pin the two properties that make it the car's own shape rather
// than ours.
//
// PREHEAT_MANUAL_REQUEST_BIT is bit 2, which is where the community map
// (community_dbc\Model3CAN.dbc) puts UI_requestActiveBatteryHeating; under the
// 2026.26 map the same bit is the low bit of UI_navFastchargerType and the request
// itself sits at bit 7. The name is kept because the assert below pins ITS
// subject - bit 2 - and bit 2 is 0 on both maps for the car's own frame (V2 =
// 0b010 there, and the community map's request bit is not raised either). Bit 7 is
// a different matter: it is set in 0xAB, because 0xAB is the car's own byte and
// the car sets it in every one of the 20 frames of the reference capture that
// carry this payload - see PREHEAT_REFERENCE_FRAME below.
static constexpr uint8_t PREHEAT_TRIP_PLAN_BYTE0 = 0xABu;
static constexpr uint8_t PREHEAT_OVERRIDE_BYTE0 = PREHEAT_TRIP_PLAN_BYTE0;
static constexpr uint8_t PREHEAT_MANUAL_REQUEST_BIT = 0x04u;

// The two bits this frame raises above a parked car's - bits 5..6
// UI_battPreconditionOnNavState (set to 1) and bit 7 UI_requestActiveBatteryHeating
// - expressed as the mask they occupy.
//
// PREHEAT_ROUTE_BITS is the low five bits, 0..4: bit0 UI_tripPlanningActive,
// bit1 UI_navToSupercharger and bits 2..4 UI_navFastchargerType, which is the
// route the car itself is on. It deliberately stops below bit5, where the
// request begins.
static constexpr uint8_t PREHEAT_ROUTE_BITS = 0x1Fu;
static constexpr uint8_t PREHEAT_REQUEST_BITS = 0xA0u;

// Minimum source byte0 that marks a live route frame: bit1 UI_navToSupercharger
// plus bit0 UI_tripPlanningActive. preheat_runtime.h counts the car frames that
// fail preheatSourceSeenPure, which GET /api/preheat/stats reports as sourceSkipped.
static constexpr uint8_t PREHEAT_SOURCE_MASK = 0x03u;

static_assert(PREHEAT_OVERRIDE_BYTE0 == 0xABu,
              "the transmitted byte0 must be the car's own preconditioning 0xAB");
// The bit this feature must NOT set. Every 0x082 the car sent while the pack
// heated - in the reference capture and in the field - had bit 2 clear, so adding
// it is a deviation from the frame that works rather than the request that makes
// it work. A future revision that decides otherwise has to delete this assert,
// which is the point. (Which signal that bit is depends on the map - see the note
// above PREHEAT_TRIP_PLAN_BYTE0 - and the assert is written to hold on both.)
static_assert((PREHEAT_OVERRIDE_BYTE0 & PREHEAT_MANUAL_REQUEST_BIT) == 0u,
              "bit2 must stay clear: the car heats without it");
// Bits 0+1 must stay set, which is what keeps the frame idempotent: re-reading
// our own transmission reproduces 0xAB instead of drifting, and that matters
// because a missed self-echo would otherwise feed a changing byte0 back in.
static_assert((PREHEAT_OVERRIDE_BYTE0 & PREHEAT_SOURCE_MASK) == PREHEAT_SOURCE_MASK,
              "byte0 must keep tripPlanningActive + navToSupercharger set");
// The property that makes 0xAB the car's own byte rather than a synthesis: it IS
// its route bits plus these two request bits, so the frame a field run accepted
// is reproduced byte for byte from those two pieces. If this ever fails the
// byte is no longer the car's own shape and the field comparison is void.
// The parens matter: == binds tighter than |, so without them the expression
// reduces to (byte0 & route bits), which is nonzero and would pass for the wrong
// reason.
static_assert(((PREHEAT_OVERRIDE_BYTE0 & PREHEAT_ROUTE_BITS) |
               PREHEAT_REQUEST_BITS) == PREHEAT_OVERRIDE_BYTE0,
              "route bits + request bits must reproduce the car's 0xAB");
static_assert((PREHEAT_REQUEST_BITS & PREHEAT_MANUAL_REQUEST_BIT) == 0u,
              "the request bits must not raise bit2");
static_assert((PREHEAT_REQUEST_BITS & PREHEAT_ROUTE_BITS) == 0u,
              "the request bits must not overlap the preserved route bits");
static_assert(PREHEAT_ROUTE_BITS == 0x1Fu,
              "the preserved route bits must be the low five, 0x1F");

// The transmit cadence of the flat payload: 10 Hz, one frame every 100 ms while
// the feature is armed. The frame is a constant, so there is nothing on the
// receive side to wait for - this is the cadence the field-accepted flat
// revision ran at.
static constexpr uint16_t PREHEAT_PERIOD_MS = 100u;

// Same boot grace as the NAG engine, so nothing transmits while the CAN
// subsystem is still settling.
static constexpr uint32_t PREHEAT_BOOT_DELAY_MS = 15000u;

// Bus liveness is deliberately NOT derived from the 0x082's own age. Field
// capture the reference capture shows 0x082 is not a steady 1 Hz beacon: gaps
// of 2.0 s (x8), 2.7 s, 4.0 s and one of 13.0 s. A frame-age gate therefore cut
// the injection off mid-heat and restarted it -- the "heats then cuts out"
// report. Liveness comes from canPhysicalFreshMaskNow() in can_core.h, the
// recovery supervisor's own RECOVERY_BUS_FRESH_MS window over the whole
// Vehicle/Body bus rather than over a single frame ID.

// Retain the established MCP warmup threshold used by the NAG engine.
static constexpr uint32_t PREHEAT_MCP_WARMUP_FRAMES = 1000u;

// A self-echo is our own frame returning on the bus: it must not be mistaken for
// a fresh car frame, because these witnesses describe the car. MCP round-trip is
// well under 20 ms.
static constexpr uint32_t PREHEAT_SELF_ECHO_MAX_AGE_MS = 20u;

// ===========================================================================
// Auto-shutoff target
//
// The engine switches itself off once the pack average reaches the user's
// target, and that average is the car's own: BMS_thermistorTAvg, 0x332 mux 0
// bits 32..39, the mean the BMS computes over its thermistors. It is NOT the
// mean of the two cells the same mux carries, which is what an earlier revision
// derived. On the reference capture the car reported 11.5 C there while
// (min + max) / 2 is 11.8 C, and across 180 mux-0 frames of the eight captures
// the pair disagrees with the car in 111 of them (62 %), by up to 0.5 C. Reading
// the car's own number takes the derived value out of the loop: the cell on the
// strip, the number the engine compares and the number the BMS acts on are then
// one value.
//
// The comparison is a direct at-or-above, because the car's average already
// arrives as the tenth of a degree the strip prints.
//
// The reading must be fresh, otherwise a value left over from before the enable
// decision would stop the heat the instant it was switched on.
//
// The window is the one the dashboard dropdown offers: 20 C to 50 C in 5 C
// steps. The floor is not zero because a target a cold pack already sits above
// would switch the heat off the moment it was enabled.
//
// Fixed point: tenths of a degree Celsius.
// ===========================================================================
static constexpr int16_t PREHEAT_TARGET_MIN_CX10 = 200;     // 20.0 C
static constexpr int16_t PREHEAT_TARGET_MAX_CX10 = 500;     // 50.0 C
static constexpr int16_t PREHEAT_TARGET_DEFAULT_CX10 = 400; // 40.0 C

// Both ends of the window sit on the dropdown's grid, so every value the UI can
// send is accepted as-is and never snapped to a neighbour. The API still accepts
// finer values, because a target saved by an older build - the 2 C dropdown that
// preceded this one, or the 0.5 C number field before that - must keep working
// after an update instead of being rejected.
static constexpr int16_t PREHEAT_TARGET_STEP_CX10 = 50; // 5.0 C

// 0x332 is a fast frame, so 5 s tolerates several missed frames while still
// refusing to act on a reading that predates the session.
static constexpr uint32_t PREHEAT_PACK_FRESH_MS = 5000u;

static inline int16_t preheatClampTargetCx10(int32_t value) {
  if (value < PREHEAT_TARGET_MIN_CX10) return PREHEAT_TARGET_MIN_CX10;
  if (value > PREHEAT_TARGET_MAX_CX10) return PREHEAT_TARGET_MAX_CX10;
  return (int16_t)value;
}

static inline bool preheatTargetInRangeCx10(int32_t value) {
  return value >= PREHEAT_TARGET_MIN_CX10 && value <= PREHEAT_TARGET_MAX_CX10;
}

// True when the engine must switch itself off: a fresh pack average, taken
// straight from the car by preheatDecodeBrickTempAvgPure, at or above the
// target. Nothing here derives a temperature from two others. An absent
// or stale reading never stops the heat, so a value left over from before the
// enable decision cannot cut the heat the instant it starts.
static inline bool preheatTargetReachedPure(bool avgValid, uint32_t avgAgeMs,
                                            int16_t avgCx10, int16_t targetCx10,
                                            uint32_t maxAgeMs) {
  if (!avgValid) return false;
  if (avgAgeMs > maxAgeMs) return false;
  return avgCx10 >= targetCx10;
}

// ===========================================================================
// Skip reasons - why the engine did NOT transmit this tick.
//
// Only gates that can actually hold a transmission back are listed. The engine
// does not read the live 0x082 to build its payload, so "no source frame" is
// not a reason, and the self-echo test is telemetry hygiene rather than
// a transmit gate. What is left is: the engine is off, the profile has no
// Vehicle/Body path, the CAN subsystem is still settling, the controller has not
// warmed up, the bus itself is silent, or the car has not yet shown which ID its
// trip planning uses.
// ===========================================================================
enum PreheatSkipReasonPure : uint8_t {
  PREHEAT_SKIP_NONE = 0,
  PREHEAT_SKIP_DISABLED = 1,
  PREHEAT_SKIP_BOOT_DELAY = 2,
  PREHEAT_SKIP_WARMUP = 3,
  PREHEAT_SKIP_UNSUPPORTED = 4,
  PREHEAT_SKIP_BUS_SILENT = 5,
  // The car has not yet shown which ID its own trip-planning frame uses (0x082 or
  // 0x347), so there is no frame this engine may send - see PreheatTripIdPure.
  PREHEAT_SKIP_NO_TRIP_ID = 6,
  PREHEAT_SKIP_COUNT = 7
};

static inline const char *preheatSkipReasonName(uint8_t reason) {
  switch (reason) {
    case PREHEAT_SKIP_NONE: return "NONE";
    case PREHEAT_SKIP_DISABLED: return "DISABLED";
    case PREHEAT_SKIP_BOOT_DELAY: return "BOOT_DELAY";
    case PREHEAT_SKIP_WARMUP: return "MCP_WARMUP";
    case PREHEAT_SKIP_UNSUPPORTED: return "UNSUPPORTED_PROFILE";
    case PREHEAT_SKIP_BUS_SILENT: return "BUS_SILENT";
    case PREHEAT_SKIP_NO_TRIP_ID: return "NO_TRIP_ID";
    default: return "UNKNOWN";
  }
}

// ===========================================================================
// Auto-off reasons - why the engine switched ITSELF off while armed.
//
// Distinct from the skip reasons above, which answer "why did it not transmit
// this tick": these are terminal state changes.
//
// TARGET_REACHED is the pack-average criterion, the only user-settable number in
// the feature. STATOR_STALL is the honesty criterion that came with the flat
// revision: the payload is a constant, so the drive units' own heatPowerActual
// (0x2E5 front / 0x266 rear) is the only evidence that it does anything. Thirty
// seconds of transmitting with both of them at zero is not "warm enough", it is
// "the car is not acting on this frame", and leaving the engine armed would keep
// the strip claiming otherwise.
//
// Both are SILENT on the tab. It names the reason in the status chip - so a
// self-stopped engine is not mistaken for a manual toggle - and that is the whole
// of the presentation: there is no alarm row, so `autoOffReason` in
// GET /api/preheat/stats is where the stall is read.
// ===========================================================================
enum PreheatAutoOffReasonPure : uint8_t {
  PREHEAT_AUTOOFF_NONE = 0,
  PREHEAT_AUTOOFF_TARGET = 1,
  PREHEAT_AUTOOFF_STATOR_STALL = 2
};

static constexpr uint32_t PREHEAT_STATOR_STALL_OFF_MS = 30000u;

static inline const char *preheatAutoOffReasonName(uint8_t reason) {
  switch (reason) {
    case PREHEAT_AUTOOFF_NONE: return "NONE";
    case PREHEAT_AUTOOFF_TARGET: return "TARGET_REACHED";
    case PREHEAT_AUTOOFF_STATOR_STALL: return "STATOR_STALL";
    default: return "UNKNOWN";
  }
}

// Wrap-safe, like preheatDuePure. sinceMs is 0 while the clock is not running -
// it starts at the session's first successful transmission, because the boot
// grace and the MCP warmup can hold the engine back for longer than the window
// and silence before the first frame is not evidence about the payload.
static inline bool preheatStatorStallElapsedPure(uint32_t nowMs,
                                                 uint32_t sinceMs) {
  if (sinceMs == 0u) return false;
  return (uint32_t)(nowMs - sinceMs) >= PREHEAT_STATOR_STALL_OFF_MS;
}

// ===========================================================================
// Eligibility cascade - the first failing gate wins and is reported, exactly
// like nagEligibilityReasonPure. A caller that sees PREHEAT_SKIP_NONE may
// transmit; every other value is a counted, named reason why it did not.
//
// busFresh is the profile's Vehicle/Body channel as reported by
// canPhysicalFreshMaskNow(): the bus delivered at least one frame inside
// RECOVERY_BUS_FRESH_MS. It is the liveness gate and is deliberately independent
// of the 0x082 template, which arrives in bursts rather than as a steady beacon.
// The payload itself needs no source frame - but it does need to know which ID
// the car's trip planning lives on, and only the car's own frame says that
// (tripIdKnown, see PreheatTripIdPure). That is the last gate: a live bus whose
// car has not shown a trip-planning frame yet is waiting, not silent.
// ===========================================================================
static inline uint8_t preheatEligibilityReasonPure(
    bool enabled, bool supported, bool bootDelayPassed, bool canSeen,
    bool busFresh, bool tripIdKnown) {
  if (!enabled) return PREHEAT_SKIP_DISABLED;
  if (!supported) return PREHEAT_SKIP_UNSUPPORTED;
  if (!bootDelayPassed) return PREHEAT_SKIP_BOOT_DELAY;
  if (!canSeen) return PREHEAT_SKIP_WARMUP;
  if (!busFresh) return PREHEAT_SKIP_BUS_SILENT;
  if (!tripIdKnown) return PREHEAT_SKIP_NO_TRIP_ID;
  return PREHEAT_SKIP_NONE;
}

// ===========================================================================
// Stalled state - the one thing the tab must never paint as "Heating".
//
// The engine is armed and a gate is holding it back. This is the state that
// stayed invisible in the field: the strip showed "Heating" while the tick was
// returning above the transmit path on every slot, so a preheat that never
// started looked identical to one that was running. The skip counters do not
// close that gap on their own - they are history, and a BOOT_DELAY skip from
// boot still reads BOOT_DELAY minutes later.
//
// DISABLED and UNSUPPORTED are not stalls; the tab names those two itself.
// preheatEligibilityReasonPure() only returns them when the matching flag is
// false, so an armed engine can never report them here.
// ===========================================================================
static inline bool preheatStalledPure(bool enabled, bool supported,
                                      uint8_t reason) {
  if (!enabled || !supported) return false;
  return reason != PREHEAT_SKIP_NONE;
}

// ===========================================================================
// Cadence and liveness
// ===========================================================================

// Pure form of the bus-liveness test, kept host-testable. The physical fresh
// mask comes from canPhysicalFreshMaskNow() and the required bus from the
// active profile's Vehicle/Body routing. A zero required bus is never
// satisfied, so a profile with no Vehicle/Body channel can never look fresh by
// accident.
static inline bool preheatBusFreshPure(uint8_t physicalFreshMask,
                                       uint8_t requiredBus) {
  if (requiredBus == 0u) return false;
  return (uint8_t)(physicalFreshMask & requiredBus) == requiredBus;
}

// True when the periodic slot is due. lastTxMs == 0 means "never yet".
static inline bool preheatDuePure(uint32_t nowMs, uint32_t lastTxMs,
                                  uint16_t periodMs) {
  if (lastTxMs == 0u) return true;
  return (uint32_t)(nowMs - lastTxMs) >= (uint32_t)periodMs;
}

// Longest observed gap between successful transmissions. A gap far above
// PREHEAT_PERIOD_MS is the single best indicator that the engine stalled, so
// this is surfaced as maxTxGapMs alongside the raw counters.
static inline uint32_t preheatGapMaxUpdatePure(uint32_t previousTxMs,
                                               uint32_t nowMs,
                                               uint32_t currentMaxMs) {
  if (previousTxMs == 0u) return currentMaxMs;
  const uint32_t gap = (uint32_t)(nowMs - previousTxMs);
  return gap > currentMaxMs ? gap : currentMaxMs;
}

// Our own injected frame coming back on the bus must not be mistaken for a new
// source frame, or the template age would be reset by our own traffic.
static inline bool preheatIsSelfEchoPure(bool lastTxValid, uint32_t ageMs,
                                         const uint8_t *rx, const uint8_t *tx,
                                         uint8_t dlc) {
  if (!lastTxValid || !rx || !tx || dlc != PREHEAT_DLC) return false;
  if (ageMs > PREHEAT_SELF_ECHO_MAX_AGE_MS) return false;
  for (uint8_t i = 0; i < PREHEAT_DLC; i++) {
    if (rx[i] != tx[i]) return false;
  }
  return true;
}

// ===========================================================================
// Immediate reassertion - answering the car's own trip frame.
//
// The ID has two writers. The car keeps sending its own trip frame while the
// engine runs - "no preconditioning" (00 7F FF 80 ... / 01 ...), median every 2 s,
// gaps up to 17 s - and between that frame and our next periodic one up to
// PREHEAT_PERIOD_MS passes with the car's statement as the last word on the bus.
// The receiver acts on the content: in the self-heating capture the car's own heat
// started ~1 s after its frame turned AB and stopped 3.0 s after it turned 01.
// That is the "heat jumps to 0 and back" seen with the engine on and
// never with the car's own route to a Supercharger, where its frames agree with
// ours.
//
// T2CAN taps the bus in parallel, so the car's frame cannot be removed. What it
// can do is what the author's R79 engine does for 0x3FD ("immediate reassertion
// and independent periodic refresh"): put our frame on the wire right after every
// car frame that disagrees with it, so the car's statement is the last word for
// milliseconds instead of up to 100 ms.
//
// The rule answers EVERY such frame, repeats included - the car repeats the same
// "no preconditioning" frame every couple of seconds, and each repeat is heard
// again. (Answering only a CHANGED car frame, or dropping any reply within 150 ms
// of our own last frame, would answer almost nothing.) A loop is impossible without any time window: a frame equal to
// the one we send is never answered, and every frame we send is that frame.
// ===========================================================================
static inline bool preheatReplyArmPure(const uint8_t *rx, const uint8_t *ours,
                                       uint8_t dlc) {
  if (!rx || !ours || dlc != PREHEAT_DLC) return false;
  for (uint8_t i = 0; i < PREHEAT_DLC; i++) {
    if (rx[i] != ours[i]) return true;
  }
  return false;
}

// A storm guard only, measured from our last REPLY - not from the periodic frame,
// which would suppress nearly every reply at a 100 ms period. The car sends its
// trip frame about once a second, so 20 ms never delays a real answer.
static constexpr uint32_t PREHEAT_REPLY_MIN_GAP_MS = 20u;

static inline bool preheatReplyDuePure(bool armed, uint32_t nowMs,
                                       uint32_t lastReplyMs) {
  if (!armed) return false;
  if (lastReplyMs == 0u) return true;
  return (uint32_t)(nowMs - lastReplyMs) >= PREHEAT_REPLY_MIN_GAP_MS;
}

// The breaker - defence in depth against a loop nothing in the data shows. Our
// own frame can never arm an answer (byte compare against our constant, no time
// window), and the car does not speed up its trip frame when ours appear: in
// a capture taken while the engine sent 10 Hz, the car's last 0x082 was
// a median 559 ms old (up to 4.7 s). What is left is a node we have not seen that
// would answer our ANSWER, or rewrite our frame. The car sends about one trip frame
// a second, so more than PREHEAT_REPLY_BREAKER_MAX answers inside one window is not
// the car: the breaker trips and answers stay off for the rest of the session -
// the engine then runs exactly as the build without them, on the 10 Hz frame.
static constexpr uint8_t PREHEAT_REPLY_BREAKER_MAX = 5u;
static constexpr uint32_t PREHEAT_REPLY_BREAKER_WINDOW_MS = 1000u;

struct PreheatReplyBreakerPure {
  bool started;
  uint32_t windowStartMs;
  uint8_t count;
  bool tripped;
};

// Called for each answer about to go out. True = send it; false = the breaker
// is (or just became) tripped. Wrap-safe.
static inline bool preheatReplyBreakerAllowPure(PreheatReplyBreakerPure &b,
                                                uint32_t nowMs) {
  if (b.tripped) return false;
  if (!b.started ||
      (uint32_t)(nowMs - b.windowStartMs) >= PREHEAT_REPLY_BREAKER_WINDOW_MS) {
    b.started = true;
    b.windowStartMs = nowMs;
    b.count = 0;
  }
  if (b.count >= PREHEAT_REPLY_BREAKER_MAX) {
    b.tripped = true;
    return false;
  }
  b.count++;
  return true;
}

// A drop of the heat witness "near" a disagreeing car frame: within the 3 s the
// car itself took to stop heating after its own frame turned 01 (191535 log). The
// field verdict on this revision is the ratio of such drops to all drops.
static constexpr uint32_t PREHEAT_DROP_NEAR_CAR_MS = 3000u;

static inline bool preheatDropNearCarPure(uint32_t dropMs, uint32_t lastCarMs) {
  if (lastCarMs == 0u) return false;
  return (uint32_t)(dropMs - lastCarMs) <= PREHEAT_DROP_NEAR_CAR_MS;
}

// True when the car's frame claims a charger route: bit0 UI_tripPlanningActive
// plus bit1 UI_navToSupercharger. In the reference capture that is the shape of
// the car's own preconditioning frame, while the parked 00 7F FF 80 FF 03 00 80
// and the ramping 0A / AA ones fail it. It is a witness, not a gate: the count of
// car frames that fail it is published as sourceSkipped, which is how
// /api/preheat/stats tells a parked car from one that is navigating.
static inline bool preheatSourceSeenPure(const uint8_t *data, uint8_t dlc) {
  if (!data || dlc < 1u) return false;
  return (data[0] & PREHEAT_SOURCE_MASK) == PREHEAT_SOURCE_MASK;
}

// ===========================================================================
// Route energies - bytes 2..7.
//
// This block is written in the COMMUNITY map's terms
// (community_dbc\Model3CAN.dbc), where bytes 2..7 are three signed 16-bit
// little-endian fields at 0.01 kWh: bytes 2..3 UI_predictedEnergy, 4..5
// UI_hindsightEnergy, 6..7 UI_energyAtDestination. The car's own preconditioning
// frame carries 0x13C8 / 0x03FF / 0x148E there (50.64 / 10.23 / 52.62 kWh); the
// parked and ramping templates carry 0x80FF and 0x8000, which have the sign bit
// set - the DBC's invalid marker, and never a real route energy, since a route
// energy is not negative. The test is therefore the sign bit alone, and it is
// applied to the live template and to every cache candidate the same way.
//
// On the 2026.26 map the same bytes carry different signals: byte1 is
// UI_battPreconditionOnNavPowerReq, byte2 UI_battPreconditionOnNavTargetT, byte3
// UI_ambientTempAtDestination, byte4/5 the 10-bit UI_tripPlanChargingTargetPercent
// and byte6/7 UI_energyAtDestination. The three sign-bit tests therefore land on
// different signals, and this is worth spelling out because it is not obvious:
//
//   index 0 = bytes 2..3 -> low byte is TargetT, HIGH byte is ambient, so the
//             sign bit is ambient's own SNA marker (0x80);
//   index 1 = bytes 4..5 -> the charging target (bits 32..41), so the sign bit is
//             bit 7 of byte5, i.e. one of the two UNUSED bits 42..47; the field's
//             own SNA (1023) does not reach it at all;
//   index 2 = bytes 6..7 -> UI_energyAtDestination, whose SNA is exactly 0x8000.
//
// So on that map the flag means "the car stated an ambient and a destination
// energy, and the spare bits are clear" - and it deliberately does not see
// PowerReq / TargetT SNA, because those are the LOW bytes of reads 0 and 1. That
// is consistent with the census: 01 7F FF 03 FF 03 D7 13 has a SNA request
// (0x7F / 0xFF) and is still "valid" here, which is the whole reason the request
// fields, not this flag, are what tells a heating state from a refusal. The
// behaviour is unchanged either way - only the reading of the same eight bytes
// differs between the two maps.
//
// byte1 opens the same block: it is not a DBC signal on the community map, but it
// moves with the route (0x7F parked -> 0x50 once the route latches) and it is part
// of the frame the car sends while it heats - on 2026.26 it is exactly the power
// request. It is substituted together with the energies, never
// mixed across frames - a parked byte1 above preconditioning energies would be a
// shape the car never transmitted.
// ===========================================================================
static constexpr uint8_t PREHEAT_ENERGY_OFFSET = 2u;
static constexpr uint8_t PREHEAT_ENERGY_FIELDS = 3u;

// One route energy as it sits on the bus, or false when the field cannot be read
// (null frame, short DLC or an out-of-range index). The raw value is already
// 0.01 kWh, so it is kWh x100.
static inline bool preheatEnergyRawPure(const uint8_t *d, uint8_t dlc,
                                        uint8_t index, uint16_t &raw) {
  if (!d || index >= PREHEAT_ENERGY_FIELDS) return false;
  const uint8_t off = (uint8_t)(PREHEAT_ENERGY_OFFSET + index * 2u);
  if (dlc < (uint8_t)(off + 2u)) return false;
  raw = (uint16_t)((uint16_t)d[off] | (uint16_t)((uint16_t)d[off + 1u] << 8u));
  return true;
}

// True when all three route energies are present and none carries the invalid
// marker. False for the parked template, whose UI_predictedEnergy and
// UI_energyAtDestination are SNA, and false for a null or short frame, so an
// unreadable field is never mistaken for a valid route.
static inline bool preheatEnergiesValidPure(const uint8_t *d, uint8_t dlc) {
  for (uint8_t i = 0; i < PREHEAT_ENERGY_FIELDS; i++) {
    uint16_t raw = 0;
    if (!preheatEnergyRawPure(d, dlc, i, raw)) return false;
    if ((raw & 0x8000u) != 0u) return false;
  }
  return true;
}


// The one frame this engine transmits, verbatim.
//
// It is the car's own preconditioning frame from the reference capture
// (the reference capture), byte for byte - nothing is added to it and nothing is
// raised. Decoded against Tesla_2026.26.6 -Model3.dbc, BO_ 130 UI_tripPlanning.
// The full bit map is in the header of this file; per byte, with the car's own
// values:
//
//   byte0  0xAB  tripPlanningActive 1, navToSupercharger 1, navFastchargerType 2
//                (V2), battPreconditionOnNavState 1 (ACTIVE_HEAT),
//                requestActiveBatteryHeating 1
//   byte1  0x50  UI_battPreconditionOnNavPowerReq = 0x50 * 125 W = 10000 W
//   byte2  0xC8  UI_battPreconditionOnNavTargetT = 0xC8 * 0.25 = 50.0 C
//   byte3  0x13  UI_ambientTempAtDestination = 0x13 * 0.5 = 9.5 C
//   byte4  0xFF  low byte of the 10-bit tripPlanChargingTargetPercent field
//   byte5  0x03  its high 2 bits -> 0b11_1111_1111 = 1023 = the field's own SNA
//   byte6  0x8E  UI_energyAtDestination low byte
//   byte7  0x14  UI_energyAtDestination high byte = 0x148E = 52.62 kWh
//
// Two of those are easy to misread, and both come from VAL_ 130:
//
//   * The charging target is NOT "102.3 %". 1023 is exactly the value the map
//     declares SNA for UI_tripPlanChargingTargetPercent, so the field says
//     "no charging target stated" - the same kind of emptiness as the SNA
//     energies, just at the other end of its range. byte4/byte5 share their
//     positions with UI_hindsightEnergy in the community map, which is where the
//     "102.3 %" reading came from.
//   * byte0 bit 7, UI_requestActiveBatteryHeating under this map, IS set here.
//     That is not this engine's doing: 0xAB is the byte the car itself sent, 20
//     of the 34 frames of the reference capture carry exactly AB 50 C8 13 FF 03
//     8E 14, and the field is set in every one of them. What the asserts below
//     pin clear is bit 2, which is the whole of the request on the community map
//     (and the low bit of navFastchargerType on 2026.26, where the car's own V2
//     also leaves it 0).
//
// 0xC8 is inside the field's own range (0..63.75 C) and is not the SNA marker
// (0xFF) - one of the properties the static_asserts below pin. The target byte
// shares its position with the low byte of UI_predictedEnergy in the community
// map, so this byte moves that energy with it (0x13C8); the car does the same
// thing itself - its own frames carry 0x13C8 at 50.0 C and 0x14C1 at 48.25 C -
// so this is the field behaving as the car does, not a mismatch.
static constexpr uint8_t PREHEAT_REFERENCE_FRAME[PREHEAT_DLC] = {
    0xABu, 0x50u, 0xC8u, 0x13u, 0xFFu, 0x03u, 0x8Eu, 0x14u};

// The SNA markers of the two request fields, named because the asserts below pin
// the frame clear of both: PowerReq and TargetT are SNA in exactly the states
// where the car does not heat (the payload census in .clinerules/10-preheat-0x082.md),
// so a transmitted frame carrying either would claim a trip plan over a request
// that asks for nothing.
static constexpr uint8_t PREHEAT_POWER_REQ_SNA = 0x7Fu;
static constexpr uint8_t PREHEAT_TARGET_SNA = 0xFFu;

// The transmit target, in the fixed point the field carries (0.25 C per count).
static constexpr uint8_t PREHEAT_TARGET_RAW = PREHEAT_REFERENCE_FRAME[2];
static_assert(PREHEAT_TARGET_RAW == 0xC8u,
              "the transmitted target must be 0xC8");
static_assert((uint32_t)PREHEAT_TARGET_RAW * 25u == 5000u,
              "0xC8 * 0.25 C per count must decode to 50.00 C");
static_assert(PREHEAT_TARGET_RAW != 0xFFu,
              "the target must not be the field's SNA marker");
static_assert(PREHEAT_REFERENCE_FRAME[0] == PREHEAT_OVERRIDE_BYTE0,
              "byte0 must stay the car's own preconditioning byte");
// Both request fields must be REAL in the transmitted frame. A frame whose
// PowerReq or TargetT is SNA asks the car for nothing while claiming a trip plan,
// which is the shape the car itself never sends - it is what the first field
// revision transmitted, and it did not heat.
static_assert(PREHEAT_REFERENCE_FRAME[1] != PREHEAT_POWER_REQ_SNA,
              "byte1 must not be the PowerReq SNA marker");
static_assert(PREHEAT_REFERENCE_FRAME[2] != PREHEAT_TARGET_SNA,
              "byte2 must not be the TargetT SNA marker");

// ===========================================================================
// The byte0 rule of the frame, kept as one named place.
//
// The byte a field run accepted is the car's own preconditioning byte, and that
// byte IS its route bits plus this feature's two request bits. The asserts below
// prove it at compile time, which is why this is constexpr rather than inline: if
// a revision ever changes the request bits without changing the frame, the build
// stops instead of the field finding out.
// ===========================================================================
static constexpr uint8_t preheatEchoByte0Pure(uint8_t byte0) {
  return (uint8_t)((byte0 & PREHEAT_ROUTE_BITS) | PREHEAT_REQUEST_BITS);
}

// The car's own preconditioning frame must survive the rule untouched: that is
// the field-verified case (AB 50 C8 13 FF 03 8E 14 heated the pack) and it is the
// proof that these two bits are the right two.
static_assert(preheatEchoByte0Pure(0xABu) == 0xABu,
              "the car's preconditioning frame must reproduce itself");
// ...and so must the transmitted frame, or the two would disagree.
static_assert(preheatEchoByte0Pure(PREHEAT_REFERENCE_FRAME[0]) ==
                  PREHEAT_REFERENCE_FRAME[0],
              "the transmitted frame must reproduce itself");
// Raising the request bits must not disturb the route bits.
static_assert((preheatEchoByte0Pure(0x01u) & PREHEAT_ROUTE_BITS) == 0x01u,
              "a home-route frame must keep its route bits");

// ===========================================================================
// Which ID the car's trip planning lives on: 0x082, or 0x347 on HW4.
//
// The HW4 capture (a car on another
// platform, LFP pack, rear motor only) carries NO 0x082 at all - zero frames on
// either bus across 33 snapshots that each hold 0x3B3 - yet the car preconditioned.
// Its trip planning goes out on 0x347, CAN A, about 1 Hz, in exactly the 0x082
// layout of the 2026.26 map. Segment A (route to a Supercharger, banner on) walks
// the same sequence the HW3 test car walks on 0x082:
//
//   01 1A 9F 1D FF 03 2F 08   route, not to a Supercharger
//   00 1A 9F 80 FF 03 00 80   no plan, parameters remembered
//   AE 1A 9F 80 FF 03 00 80   to a Supercharger, energies not yet in
//   AF 24 9D 1D FF 03 F2 07   preconditioning
//
// and the car answered it: PowerReq 0x24 * 125 W = 4500 W is exactly what 0x3A1
// mux 1 hpBattHeatingPowerTrgt read in the same snapshot, 0x1E/0x1F (3750/3875 W)
// sit next to its 3800/3900 W while driving, and TargetT 0x9F/0x9D (39.75/39.25 C)
// are exactly the 0x312 mux 1 cell target. With no route both read 0 W / -7.5 C.
//
// 0x347 is a charger alert matrix (CHGPH1) on the older maps, so the engine never
// sends it on its own say-so: the ID is taken from the car. 0x082 wins outright -
// the HW3 test car sends it - and 0x347 is taken only from a frame of the trip
// shape on a car that has not shown a 0x082.
// ===========================================================================
static constexpr uint32_t PREHEAT_TRIP_ID_ALT = 0x347u;

enum PreheatTripIdPure : uint8_t {
  PREHEAT_TRIP_UNKNOWN = 0,
  PREHEAT_TRIP_082 = 1,
  PREHEAT_TRIP_347 = 2
};

static inline const char *preheatTripIdName(uint8_t tripId) {
  switch (tripId) {
    case PREHEAT_TRIP_082: return "0x082";
    case PREHEAT_TRIP_347: return "0x347";
    default: return "unknown";
  }
}

// The HW4 car's own preconditioning frame, sent verbatim, as the 0x082 one is. It
// is NOT the HW3 car's request moved to another ID: that car's pack is LFP, and the
// 39.25 C / 4500 W it asked for is its own number, which 50 C / 10 kW from the
// other chemistry has no business overriding. byte0 0xAF is the car's: active, to a
// Supercharger, fastcharger type V3 (0b011 - which is why bit 2 is set here and
// clear in 0xAB), ACTIVE_HEAT, bit 7.
static constexpr uint8_t PREHEAT_REFERENCE_FRAME_347[PREHEAT_DLC] = {
    0xAFu, 0x24u, 0x9Du, 0x1Du, 0xFFu, 0x03u, 0xF2u, 0x07u};

static_assert(PREHEAT_REFERENCE_FRAME_347[0] == 0xAFu,
              "byte0 must be the HW4 car's own preconditioning 0xAF");
static_assert((PREHEAT_REFERENCE_FRAME_347[0] & PREHEAT_SOURCE_MASK) ==
                  PREHEAT_SOURCE_MASK,
              "byte0 must keep tripPlanningActive + navToSupercharger set");
static_assert(preheatEchoByte0Pure(PREHEAT_REFERENCE_FRAME_347[0]) ==
                  PREHEAT_REFERENCE_FRAME_347[0],
              "0xAF must be its route bits plus the same two request bits");
static_assert(PREHEAT_REFERENCE_FRAME_347[1] != PREHEAT_POWER_REQ_SNA,
              "byte1 must not be the PowerReq SNA marker");
static_assert(PREHEAT_REFERENCE_FRAME_347[2] != PREHEAT_TARGET_SNA,
              "byte2 must not be the TargetT SNA marker");

// Bits 42..47 carry no signal in the trip-planning layout, and every 0x347 the HW4
// car sent has them clear, as every 0x082 does. An alert matrix has no such hole,
// so this is what tells the car's trip frame from the charger alert on 0x347.
static inline bool preheatTripShapePure(const uint8_t *d, uint8_t dlc) {
  if (!d || dlc != PREHEAT_DLC) return false;
  return (d[5] & 0xFCu) == 0u;
}

// One step of the latch. A 0x082 settles it for good; a 0x347 of the trip shape
// settles it only while no 0x082 has been seen. Our own echo is filtered before
// this is called, so the engine cannot vote for the ID it is sending on.
static inline uint8_t preheatTripIdNextPure(uint8_t current, uint32_t id,
                                            uint8_t dlc, const uint8_t *d) {
  if (id == PREHEAT_SOURCE_ID && d && dlc == PREHEAT_DLC) return PREHEAT_TRIP_082;
  if (id == PREHEAT_TRIP_ID_ALT && current != PREHEAT_TRIP_082 &&
      preheatTripShapePure(d, dlc))
    return PREHEAT_TRIP_347;
  return current;
}

static inline uint32_t preheatTripCanIdPure(uint8_t tripId) {
  return tripId == PREHEAT_TRIP_347 ? PREHEAT_TRIP_ID_ALT : PREHEAT_SOURCE_ID;
}

static inline const uint8_t *preheatTripFramePure(uint8_t tripId) {
  return tripId == PREHEAT_TRIP_347 ? PREHEAT_REFERENCE_FRAME_347
                                    : PREHEAT_REFERENCE_FRAME;
}

// ===========================================================================
// Telemetry decoders.
//
// All values are returned as scaled integers so the feature stays fixed-point
// and host-testable, matching nagPartySpeedKphX100Pure.
//
// Little-endian ("sawtooth") bit order, identical to the DBC @1+ encoding:
// bit N is byte[N>>3], bit (N&7).
// ===========================================================================
static inline uint32_t preheatGetBitsPure(const uint8_t *d, uint8_t start,
                                          uint8_t len) {
  uint32_t v = 0;
  for (uint8_t i = 0; i < len; i++) {
    const uint8_t bit = (uint8_t)(start + i);
    v |= (uint32_t)((d[bit >> 3] >> (bit & 7u)) & 1u) << i;
  }
  return v;
}

// --- 0x332 ID332BattBrickMinMax, DLC 6 -----------------------------------
// Mux = byte0 & 0x03. Temperatures live in mux 0 (byte3 = min, byte2 = max);
// mux 1 carries cell voltages. Decoding the wrong mux produces -36.5 / 79.0 C
// garbage, which is exactly the "Tmin > Tmax" trap seen on 0x312.
// raw * 0.5 - 40, scaled x10 => raw * 5 - 400.
static constexpr uint8_t PREHEAT_BRICK_DLC = 6u;

static inline bool preheatDecodeBrickTempsPure(const uint8_t *d, uint8_t dlc,
                                               int16_t &minCx10,
                                               int16_t &maxCx10) {
  if (!d || dlc < PREHEAT_BRICK_DLC) return false;
  if ((d[0] & 0x03u) != 0u) return false;
  minCx10 = (int16_t)((int16_t)d[3] * 5 - 400);
  maxCx10 = (int16_t)((int16_t)d[2] * 5 - 400);
  return true;
}

// --- 0x252 BMS_powerAvailable, DLC 8 -------------------------------------
// bits 32..41 BMS_maxStationaryHeatPower, factor 0.01 kW => value is already
// kW x100. Reference capture: 0 -> 802 (0.00 -> 8.02 kW) during preheat.
static inline bool preheatDecodeHeatPowerPure(const uint8_t *d, uint8_t dlc,
                                              uint32_t &kwX100) {
  if (!d || dlc < 8u) return false;
  kwX100 = preheatGetBitsPure(d, 32, 10);
  return true;
}

// --- 0x241 ID241VCFRONT_coolant, DLC 7 -----------------------------------
// bits 0..8  VCFRONT_coolantFlowBatActual, factor 0.1 LPM => value is LPM x10.
// bits 17..21 VCFRONT_coolantFlowBatReason, 5 bits: 6 = BMS_FLOW_REQ,
// 13 = DIS_FLOW_REQ, 14 = HP_FLOW_REQ.
//
// Read at bit 17 length 5, as the car's own 2026.26 map has it. The earlier bit
// 18 length 4 read shifted every code by one bit and reported 7 for the 13/14
// the reference capture carries while the pack is heating - which is how a note
// claiming "reason -> 6 (BMS_FLOW_REQ) at +18.787 s" got into the rules.
static constexpr uint8_t PREHEAT_COOLANT_DLC = 7u;
static constexpr uint8_t PREHEAT_FLOW_REASON_BMS = 6u;

static inline bool preheatDecodeCoolantPure(const uint8_t *d, uint8_t dlc,
                                            uint16_t &lpmX10,
                                            uint8_t &reason) {
  if (!d || dlc < PREHEAT_COOLANT_DLC) return false;
  lpmX10 = (uint16_t)preheatGetBitsPure(d, 0, 9);
  reason = (uint8_t)preheatGetBitsPure(d, 17, 5);
  return true;
}

// --- 0x212 BMS_status, DLC 8 ---------------------------------------------
// bit 3 BMS_preconditionAllowed: the only preconditioning flag the car's own
// 2026.26 map puts in this frame, and the one this decoder returns.
//
// The two flags the community map places beside it are somebody else's signals
// here: bit 5 is BMS_cpMiaOnHvs and bit 30 is BMS_chargeRequest. Reading them as
// "activeHeatingWorthwhile" and "keepWarmRequest" is what made the old note say
// the car's own heating verdict never rose - the real
// BMS_activeHeatingWorthwhile is in 0x312 mux0 bit30 and is decoded with the
// thermal block below. No keep-warm signal exists in the 2026.26 dictionary at
// all, so the API field stays false rather than mirroring a bit that means
// something else.
static inline bool preheatDecodeBmsStatusPure(const uint8_t *d, uint8_t dlc,
                                              bool &allowed) {
  if (!d || dlc < 8u) return false;
  allowed = ((d[0] >> 3) & 1u) != 0u;
  return true;
}

// --- 0x2E1 ID2E1VCFRONT_status, DLC 8 ------------------------------------
// Mux = byte0 & 0x07 (VCFRONT_statusIndex, a 3-bit field: eight muxes over the
// frame's 86 signals). isActiveHeatingBattery is bit 63, which the map does not
// tag with a mux index - every index carries it - so the frame is filtered on the
// index the capture shows it in. Without the filter the flag flickers, because
// the remaining seven payloads reuse bit 63 for other signals.
static inline bool preheatDecodeHeatingActivePure(const uint8_t *d, uint8_t dlc,
                                                  bool &active) {
  if (!d || dlc < 8u) return false;
  if ((d[0] & 0x07u) != 0u) return false;
  active = ((d[7] >> 7) & 1u) != 0u;
  return true;
}

// --- 0x332 ID332BattBrickMinMax mux 0: the car's own pack average --------
// BMS_thermistorTAvg, bits 32..39, raw * 0.5 - 40 (scaled x10 => raw * 5 - 400).
// This is the average the car computes over its own thermistors, and it is what
// the auto-off compares AND what the PACK TEMP cell prints: one number, read in
// one place. It is not the mean of the two cells mux 0 also carries - the two
// differ by up to half a degree (11.5 C here against 11.8 C for (min + max) / 2
// on the reference capture), and across 180 mux-0 frames of the eight captures
// the derived pair disagrees with the car in 111 of them.
static inline bool preheatDecodeBrickTempAvgPure(const uint8_t *d, uint8_t dlc,
                                                int16_t &avgCx10) {
  if (!d || dlc < PREHEAT_BRICK_DLC) return false;
  if ((d[0] & 0x03u) != 0u) return false;
  avgCx10 = (int16_t)((int16_t)d[4] * 5 - 400);
  return true;
}

// --- 0x312 BMS_thermalStatus, DLC 8 ---------------------------------------
// Mux = bits 0..1. Mux 0 carries the pack temperature and the flow request,
// mux 1 the cell target; decoding the wrong mux yields a number that belongs
// to a different signal entirely, which is why each is gated on the mux.
static constexpr uint8_t PREHEAT_THERMAL_DLC = 8u;

// BMS_maxPackTemperature bits 43..53, raw * 0.1 - 40 (x10 => raw - 400).
// BMS_flowRequest bits 56..63, raw * 0.25 LPM (x4 => raw).
// BMS_activeHeatingWorthwhile bit 30: the BMS's own "heating this pack is worth
// it" verdict. It lives here - not in 0x212, where the community map puts it and
// where the car keeps BMS_cpMiaOnHvs - so this is the flag the tab reports as
// bmsHeatingWorthwhile, next to the temperature it is a verdict about.
static inline bool preheatDecodeThermalMux0Pure(const uint8_t *d, uint8_t dlc,
                                               int16_t &maxPackCx10,
                                               uint8_t &flowReqLpmX4,
                                               bool &worthwhile) {
  if (!d || dlc < PREHEAT_THERMAL_DLC) return false;
  if ((d[0] & 0x03u) != 0u) return false;
  maxPackCx10 = (int16_t)((int16_t)preheatGetBitsPure(d, 43, 11) - 400);
  flowReqLpmX4 = (uint8_t)preheatGetBitsPure(d, 56, 8);
  worthwhile = ((d[3] >> 6) & 1u) != 0u;
  return true;
}

// BMS_activeHeatCellTargetT bits 41..49, raw * 0.25 - 25 (x4 => raw - 100).
static inline bool preheatDecodeThermalMux1Pure(const uint8_t *d, uint8_t dlc,
                                               int16_t &cellTargetCx4) {
  if (!d || dlc < PREHEAT_THERMAL_DLC) return false;
  if ((d[0] & 0x03u) != 1u) return false;
  cellTargetCx4 = (int16_t)((int16_t)preheatGetBitsPure(d, 41, 9) - 100);
  return true;
}

// --- 0x3A1 VCFRONT_vehicleStatus, DLC 8 -----------------------------------
// Mux = bit 0. VCFRONT_hpBattHeatingPowerTrgt bits 41..47 in mux 1, raw * 100 W:
// the pack heating power the car has been asked for. Mux 0 shares bit 1 with
// VCFRONT_bmsHvChargeEnable, so the mux gate keeps the two apart.
static inline bool preheatDecodeHpBattHeatPowerPure(const uint8_t *d, uint8_t dlc,
                                                   uint32_t &watts) {
  if (!d || dlc < 8u) return false;
  if ((d[0] & 0x01u) != 1u) return false;
  watts = preheatGetBitsPure(d, 41, 7) * 100u;
  return true;
}

// VCFRONT_thermalSystemType, bit 5 of the same mux 1: 1 HEAT_PUMP_THERMAL_SYSTEM
// (the Octovalve), 0 LEGACY_THERMAL_SYSTEM (the older 4-way valve). Mux 0 puts
// VCFRONT_3RowRightUnbuckled over bits 4..5, hence the gate. 1 in every mux-1
// frame of the HW3 captures (314 + 1368) and of the HW4 capture (20).
static inline bool preheatDecodeThermalSystemPure(const uint8_t *d, uint8_t dlc,
                                                 bool &heatPump) {
  if (!d || dlc < 8u) return false;
  if ((d[0] & 0x01u) != 1u) return false;
  heatPump = ((d[0] >> 5) & 0x01u) != 0u;
  return true;
}

// --- 0x201 VCFRONT_loggingAndVitals10Hz, DLC 8 ----------------------------
// VCFRONT_pumpBatteryRPMActual bits 3..10, raw * 30 rpm: the battery coolant
// pump's own tachometer, independent of the 0x241 flow figure.
//
// The frame is INDEXED: bits 0..2 are VCFRONT_loggingAndVitals10HzIndex and the
// map's signals live on index 0 only - the 2026.26 map does not tag them. On the
// other indices the same bits carry other fields: in the self-heating capture index 0
// reads the pumps at 2250 / 2160 rpm, index 1 reads 0 / 30, indices 2 and 3 read
// 780 / 780 - about three frames in four. The gate below is what makes the number
// the pump's; without it the value is whichever index came last.
static inline bool preheatVitals201IndexZeroPure(const uint8_t *d, uint8_t dlc) {
  return d && dlc >= 8u && (d[0] & 0x07u) == 0u;
}

static inline bool preheatDecodePumpBatteryRpmPure(const uint8_t *d, uint8_t dlc,
                                                  uint32_t &rpm) {
  if (!preheatVitals201IndexZeroPure(d, dlc)) return false;
  rpm = preheatGetBitsPure(d, 3, 8) * 30u;
  return true;
}

// The rest of index 0, for the Thermal schematic: the powertrain pump (bits
// 11..18, x30 rpm) and the radiator fan (bits 19..26, x40 rpm).
static inline bool preheatDecodeVitals201Pure(const uint8_t *d, uint8_t dlc,
                                             uint32_t &pumpBatRpm,
                                             uint32_t &pumpPtRpm,
                                             uint32_t &fanRpm) {
  if (!preheatVitals201IndexZeroPure(d, dlc)) return false;
  pumpBatRpm = preheatGetBitsPure(d, 3, 8) * 30u;
  pumpPtRpm = preheatGetBitsPure(d, 11, 8) * 30u;
  fanRpm = preheatGetBitsPure(d, 19, 8) * 40u;
  return true;
}

// Index 1 carries the coolant valve: VCFRONT_coolantValveMode bits 32..33, where
// the valve actually is (0 SERIES, 1 PARALLEL, 2 BLEND, 3 AMBIENT_SOURCE), and
// VCFRONT_hpReqCoolantMode bits 51..53, what the heat pump asks of it (0 ANY,
// 1 SERIES_NO_BYPASS, 2 SERIES_BYPASS, 3 PARALLEL, 4 AMBIENT_SOURCE). The service
// screen's "Series Radiator Bypass" is SERIES + SERIES_BYPASS and its "Ambient
// Source" is AMBIENT_SOURCE (photos of that screen). Measured: every index-1
// frame of the HW3 captures is SERIES / SERIES_BYPASS at rest or
// AMBIENT_SOURCE while the stators heat, plus a few transition frames; the HW4
// capture has three, SERIES / ANY. Bits 32..33 of index 0 are another field, 0 in
// every frame - reading the valve there always gives SERIES.
static inline bool preheatDecodeCoolantMode201Pure(const uint8_t *d, uint8_t dlc,
                                                  uint8_t &valveMode,
                                                  uint8_t &hpReq) {
  if (!d || dlc < 8u || (d[0] & 0x07u) != 1u) return false;
  valveMode = (uint8_t)preheatGetBitsPure(d, 32, 2);
  hpReq = (uint8_t)preheatGetBitsPure(d, 51, 3);
  return true;
}

// --- 0x321 VCFRONT_sensors, DLC 8 -----------------------------------------
// VCFRONT_tempAmbient bits 24..31, raw * 0.5 - 40 (x10 => raw * 5 - 400). The
// car's own outside-air reading, and the only frame on this bus that carries one.
//
// Measured constant per session, which is what a witness has to be: 0x61 (8.5 C)
// in all 89 frames of the reference capture, 0x67 (11.5 C) in all 369 frames of
// a PASSIVE capture (96 on CAN A, 273 on CAN B) and 0x68 (12.0 C) in
// all 174 frames of a route capture. So a number that jumps between two
// reads means a decoder picked the wrong byte, not that the weather changed.
//
// The field's own SNA marker is raw 0 (VAL_ 801 VCFRONT_tempAmbient 0 "SNA"),
// which is why it is refused rather than reported as -40.0 C.
static inline bool preheatDecodeAmbientPure(const uint8_t *d, uint8_t dlc,
                                            int16_t &ambientCx10) {
  if (!d || dlc < 8u) return false;
  if (d[3] == 0u) return false;
  ambientCx10 = (int16_t)((int16_t)d[3] * 5 - 400);
  return true;
}

// Not used on purpose: BMS_timeToFinishHeatForCharge (0x472
// bits 40..46, 5 min per count, range 0..635 min) read its own maximum - 635 -
// in all 323 frames of all six captures, while the car was neither charging nor
// heating. The top of the range is "no estimate", so the row could only ever
// print one number.

// --- Stator heating: 0x2E5 DIF_power (front), 0x266 DIR_power (rear) --------
//
// The drive units publish the heat they are actually putting into the pack as
// DIF_heatPowerActual / DIR_heatPowerActual, bits 32..39 at 0.08 kW per count, in
// a plain 10 Hz frame with no mux and no rotating index. This is the one honest
// kW reading of pack heating on this bus: the VCFRONT logging frame that also
// carries a total (0x381 totalBattHeatingPower) is multiplexed with a 5-bit index
// and the map does not tag its payload signals, so nothing in it can be decoded
// at all.
//
// Measured, and the reason this exists:
//   * the reference capture, from the frame the car's own 0x82 went to AB -
//     front 0 -> 0.72 -> 3.6 kW and rear 0 -> 0.56 -> ~2.0 kW within six
//     seconds, a total that matches the 5.8 kW the same capture asks for in
//     0x3A1 hpBattHeatingPowerTrgt.
//   * a route research capture, home-route segment - 0.00 kW in all
//     eleven snapshots; Supercharger-route segment - 2.64 kW five seconds in,
//     0.00 again five seconds later.
//
// Both IDs are read-only: they are drive-unit outputs on checksummed frames and
// nothing here ever transmits them. The strip shows the two sides of this pair as
// they are, and the same pair is what the silent stall criterion watches.
static constexpr uint16_t PREHEAT_STATOR_DIF_ID = 0x2E5u;  // front drive unit
static constexpr uint16_t PREHEAT_STATOR_DIR_ID = 0x266u;  // rear drive unit

// Both sides use the same layout, so one decoder serves both. 0.08 kW per count
// is 8 in the kW x100 this feature carries everywhere else.
static inline bool preheatDecodeStatorHeatPure(const uint8_t *d, uint8_t dlc,
                                              uint32_t &kwX100) {
  if (!d || dlc < 8u) return false;
  kwX100 = (uint32_t)d[4] * 8u;
  return true;
}

// How long a side's reading may be and still count as current. The car can pulse
// this power, so a side older than a couple of seconds is not a reading of now -
// neither for the stall criterion below, which would otherwise hold an engine
// armed on a half of the pair that has stopped arriving.
static constexpr uint32_t PREHEAT_STATOR_PAIR_FRESH_MS = 2000u;

// --- What the stall criterion watches, per trip ID --------------------------
//
// On the 0x082 car it is the stator pair above, unchanged. On the HW4 car it
// cannot be: 0x266 byte 4 reads 1.36..1.44 kW in every snapshot of the capture,
// including the no-route segment where 0x3A1 asks for 0 W, so a stall would never
// be seen. There the car's own heating request is the clean reading - 0 W before
// the 0x347 frame turns AF, 4500 W the snapshot it does, 3800..3900 W while it
// preconditions, 0 W again with no route.
//
// 0x3A1 mux 1 arrives at 10 Hz (median gap 101 ms, worst 1.4 s across 314 frames
// of the reference capture), so the stator pair's 2 s window fits it too.
static constexpr uint32_t PREHEAT_HP_POWER_FRESH_MS = 2000u;

static inline bool preheatHeatWitnessHotPure(uint8_t tripId, bool statorHot,
                                             bool hpValid, uint32_t hpAgeMs,
                                             uint32_t hpW) {
  if (tripId != PREHEAT_TRIP_347) return statorHot;
  return hpValid && hpAgeMs <= PREHEAT_HP_POWER_FRESH_MS && hpW > 0u;
}

// ===========================================================================
// Thermal schematic decoders. The Actions tab draws its own version
// of the car's service "Thermal" screen; every number on it comes from one of the
// decoders below. Each was checked against the self-heating capture (the car heating
// itself) and the HW4 capture. Fixed point as everywhere else: x10 for C and LPM.
// ===========================================================================

// Sign-extends an n-bit little-endian field.
static inline int32_t preheatGetSignedBitsPure(const uint8_t *d, uint8_t start,
                                               uint8_t len) {
  const uint32_t raw = preheatGetBitsPure(d, start, len);
  const uint32_t sign = 1u << (len - 1u);
  return (raw & sign) ? (int32_t)raw - (int32_t)(1u << len) : (int32_t)raw;
}

// --- 0x321 VCFRONT_sensors: coolant at the two loop inlets ------------------
// VCFRONT_tempCoolantBatInlet bits 0..9 and VCFRONT_tempCoolantPTInlet bits
// 10..20, both raw * 0.125 - 40 C. self-heating capture: 13.1 / 13.6 C with 11.5 C outside.
static inline bool preheatDecodeCoolantInletsPure(const uint8_t *d, uint8_t dlc,
                                                  int16_t &batInletCx10,
                                                  int16_t &ptInletCx10) {
  if (!d || dlc < 8u) return false;
  batInletCx10 = (int16_t)((int32_t)preheatGetBitsPure(d, 0, 10) * 5 / 4 - 400);
  ptInletCx10 = (int16_t)((int32_t)preheatGetBitsPure(d, 10, 11) * 5 / 4 - 400);
  return true;
}

// --- 0x241 VCFRONT_coolant: actual flow in both loops ------------------------
// coolantFlowBatActual bits 0..8 and coolantFlowPTActual bits 22..30, x0.1 LPM.
// In SERIES the two loops are one, and the frame then reads the same number twice.
static inline bool preheatDecodeLoopFlowsPure(const uint8_t *d, uint8_t dlc,
                                              uint16_t &batLpmX10,
                                              uint16_t &ptLpmX10) {
  if (!d || dlc < PREHEAT_COOLANT_DLC) return false;
  batLpmX10 = (uint16_t)preheatGetBitsPure(d, 0, 9);
  ptLpmX10 = (uint16_t)preheatGetBitsPure(d, 22, 9);
  return true;
}

// --- 0x2A4 PCS_thermalStatus: DC-DC and charger ------------------------------
// Signed 11-bit fields, raw * 0.1 + 40 C: PCS_chgPhA/B/CTemp at 0/11/22 and
// PCS_dcdcTemp at 33. The schematic's Charger is the hottest of the three phases.
static inline bool preheatDecodePcsTempsPure(const uint8_t *d, uint8_t dlc,
                                             int16_t &dcdcCx10,
                                             int16_t &chargerCx10) {
  if (!d || dlc < 8u) return false;
  int32_t hot = preheatGetSignedBitsPure(d, 0, 11);
  const int32_t b = preheatGetSignedBitsPure(d, 11, 11);
  const int32_t c = preheatGetSignedBitsPure(d, 22, 11);
  if (b > hot) hot = b;
  if (c > hot) hot = c;
  chargerCx10 = (int16_t)(hot + 400);
  dcdcCx10 = (int16_t)(preheatGetSignedBitsPure(d, 33, 11) + 400);
  return true;
}

// --- 0x376 DIF_temperature / 0x315 DIR_temperature: drive units --------------
// Indexed by DI*_tempIndex (bits 0..2) like 0x201; the temperatures are valid on
// index 0 only - indices 2..4 carry 215 / -40 placeholders. The service screen's
// "Front / Rear Drive Inverter" is the stator, DI*_statorT (bits 24..31, raw - 40 C,
// raw 0 = SNA), not the inverter board DI*_inverterT (bits 16..23): the screen
// read 42 / 41 C at rest and 51 C while the stators heated, where the board never
// leaves 11..18 C in any capture (17 C at the same minute)
// and the stator climbs 11 -> 36..44 C within a minute of heating and cools slowly.
// A HW4 car without a front motor sends no 0x376 at all, and the 0x315 of the HW4
// capture is some other frame (FB FF FB FF ..., 04 FF FF FF FF 4B 48 5E) that never
// passes the index-0 gate - both rows are dashes there.
static constexpr uint16_t PREHEAT_DIF_TEMP_ID = 0x376u;
static constexpr uint16_t PREHEAT_DIR_TEMP_ID = 0x315u;

static inline bool preheatDecodeStatorTempPure(const uint8_t *d, uint8_t dlc,
                                               int16_t &statorC) {
  if (!d || dlc < 8u || (d[0] & 0x07u) != 0u) return false;
  if (d[3] == 0u) return false;
  statorC = (int16_t)((int16_t)d[3] - 40);
  return true;
}

// --- 0x3FC APP_boardTemperatures: the Autopilot computer ---------------------
// APP_decisionTemperature byte 0, raw - 128 C. It travels on the Chassis bus (CAN
// B here), which is why the runtime takes this one ID from either channel. Raw 0
// (-128 C) is the car's "no sensor" fill, as on the other five board sensors.
static constexpr uint16_t PREHEAT_APP_TEMP_ID = 0x3FCu;

static inline bool preheatDecodeApTempPure(const uint8_t *d, uint8_t dlc,
                                           int16_t &apC) {
  if (!d || dlc < 1u || d[0] == 0u) return false;
  apC = (int16_t)((int16_t)d[0] - 128);
  return true;
}

// --- 0x353 UI_status: the infotainment computer ------------------------------
// UI_cpuTemperature byte 7, SIGNED, raw + 40 C (24 C in the self-heating capture; read as
// unsigned it would print 279 C).
static inline bool preheatDecodeInfotainmentTempPure(const uint8_t *d,
                                                     uint8_t dlc,
                                                     int16_t &cpuC) {
  if (!d || dlc < 8u) return false;
  cpuC = (int16_t)((int16_t)(int8_t)d[7] + 40);
  return true;
}
