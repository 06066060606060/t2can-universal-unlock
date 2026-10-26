#pragma once
#include "preheat_pure.h"
#include "vehicle_profile.h"
#include <string.h>

// Defined at the end of this header. Declared here so the persistence helper
// below can call it without depending on t2can_forward.h being included first.
static inline void preheatRuntimeResetUnderTxBarrier();

// ===========================================================================
// Battery preheat runtime (Actions tab).
//
// Mirrors the driver-window LAB feature: state and counters behind a portMUX,
// pure decisions in preheat_pure.h, one ingest entry point per physical bus,
// one periodic service tick, and a snapshot struct the web layer renders.
//
// Bus routing follows the profile, not a hardcoded channel:
//   STANDARD_BODY_CHASSIS -> Vehicle/Body is CAN A (MCP2515)
//   YL_PARTY_VH           -> Vehicle/Body is CAN B (TWAI, exposed as VH)
//   STANDARD_PARTY_CHASSIS-> no Vehicle/Body path, feature reports unsupported
// ===========================================================================

static portMUX_TYPE preheatMux = portMUX_INITIALIZER_UNLOCKED;

// Two switches, deliberately independent:
//   preheatMenuEnabled - Settings toggle, presentation only: whether the
//                        Actions tab and its nav button exist at all.
//   preheatEnabled     - the tab's own Battery Preheating toggle, and the only
//                        input that decides whether the injection engine runs.
// Hiding the tab must never silence a preheat that is already running, so the
// engine reads preheatEnabled alone (preheatRuntimeTick). The engine stops only
// through its own toggle or the auto-off target, and its flag is persisted
// under PREHEAT_NVS_ENABLED_KEY independently of the menu flag.
static volatile bool preheatMenuEnabled = false;
static volatile bool preheatEnabled = false;

// RX observations
static volatile bool preheatSourceValid = false;
static volatile uint32_t preheatSourceMs = 0;
static uint8_t preheatSourceRaw[8] = {};
static volatile uint32_t preheatSourceSkipped = 0;

// The car's route block, cached from the last car frame whose three route
// energies were valid. It is a WITNESS only: the transmitted frame is a constant,
// so nothing reads this block to build a payload. It is kept because it answers a
// question the payload cannot - whether the car itself is claiming a routable
// plan - and because /api/preheat/stats reports it as energyCacheValid.
//
// Deliberately NOT cleared by preheatRuntimeResetUnderTxBarrier: it describes the
// car's route, not the bus session, and a restart must not throw away the one
// frame that makes the injected command self-consistent.
static uint8_t preheatEnergyCache[8] = {};
static volatile bool preheatEnergyCacheValid = false;

static volatile bool preheatPackValid = false;
static volatile uint32_t preheatPackMs = 0;
static volatile int16_t preheatPackMinCx10 = 0;
static volatile int16_t preheatPackMaxCx10 = 0;

static volatile bool preheatBmsValid = false;
static volatile uint32_t preheatBmsMs = 0;
static volatile bool preheatBmsAllowed = false;
static volatile bool preheatBmsWorthwhile = false;
static volatile bool preheatBmsKeepWarm = false;

static volatile bool preheatHeatValid = false;
static volatile uint32_t preheatHeatMs = 0;
static volatile uint32_t preheatHeatKwx100 = 0;

static volatile bool preheatCoolantValid = false;
static volatile uint32_t preheatCoolantMs = 0;
static volatile uint16_t preheatCoolantLpmX10 = 0;
static volatile uint8_t preheatCoolantReason = 0;

static volatile bool preheatHeatingValid = false;
static volatile uint32_t preheatHeatingMs = 0;
static volatile bool preheatHeatingActive = false;

// Extra RX observations. Each signal arrives in its own frame at its own rate,
// so each keeps its own stamp: a number is only current while its own frame is
// arriving, and the tab reads that age next to the value instead of assuming
// the whole set is as fresh as the newest one.
static volatile bool preheatAvgValid = false;
static volatile uint32_t preheatAvgMs = 0;
static volatile int16_t preheatPackAvgCx10 = 0;

static volatile bool preheatThermalMux0Valid = false;
static volatile uint32_t preheatThermalMux0Ms = 0;
static volatile int16_t preheatMaxPackCx10 = 0;
static volatile uint8_t preheatFlowReqLpmX4 = 0;

static volatile bool preheatThermalMux1Valid = false;
static volatile uint32_t preheatThermalMux1Ms = 0;
static volatile int16_t preheatCellTargetCx4 = 0;

static volatile bool preheatHpPowerValid = false;
static volatile uint32_t preheatHpPowerMs = 0;
static volatile uint32_t preheatHpPowerW = 0;

static volatile bool preheatPumpValid = false;
static volatile uint32_t preheatPumpMs = 0;
static volatile uint32_t preheatPumpRpm = 0;

static volatile bool preheatAmbientValid = false;
static volatile uint32_t preheatAmbientMs = 0;
static volatile int16_t preheatAmbientCx10 = 0;

// Thermal schematic (the tab's own drawing of the service "Thermal" screen). One
// stamp per source frame, because each frame arrives at its own rate and a number
// is only current while its own frame keeps arriving.
static volatile uint32_t preheatThVitalsMs = 0;   // 0x201 index 0
static volatile uint32_t preheatThPumpBatRpm = 0;
static volatile uint32_t preheatThPumpPtRpm = 0;
static volatile uint32_t preheatThFanRpm = 0;
static volatile uint32_t preheatThModeMs = 0;     // 0x201 index 1
static volatile uint8_t preheatThValveMode = 0;
static volatile uint8_t preheatThHpReq = 0;
static volatile uint32_t preheatThSysMs = 0;      // 0x3A1 mux 1
static volatile bool preheatThHeatPump = false;
static volatile uint32_t preheatThInletsMs = 0;   // 0x321
static volatile int16_t preheatThBatInletCx10 = 0;
static volatile int16_t preheatThPtInletCx10 = 0;
static volatile uint32_t preheatThFlowsMs = 0;    // 0x241
static volatile uint16_t preheatThFlowBatLpmX10 = 0;
static volatile uint16_t preheatThFlowPtLpmX10 = 0;
static volatile uint32_t preheatThPcsMs = 0;      // 0x2A4
static volatile int16_t preheatThDcdcCx10 = 0;
static volatile int16_t preheatThChargerCx10 = 0;
static volatile uint32_t preheatThInvFrontMs = 0; // 0x376 index 0, stator
static volatile int16_t preheatThInvFrontC = 0;
static volatile uint32_t preheatThInvRearMs = 0;  // 0x315 index 0, stator
static volatile int16_t preheatThInvRearC = 0;
static volatile uint32_t preheatThApMs = 0;       // 0x3FC, either bus
static volatile int16_t preheatThApC = 0;
static volatile uint32_t preheatThInfoMs = 0;     // 0x353, either bus
static volatile int16_t preheatThInfoC = 0;

// Stator heating, from the drive units' own power frames: 0x2E5 is the front
// (DIF) unit, 0x266 the rear (DIR) one. Read-only by construction - nothing in
// this file transmits either - and the only kW reading of pack heating that
// decodes at all on this bus.
//
// Both sides are published as they are read, because that pair is what the strip
// shows and what the silent stall criterion watches. Nothing is accumulated from
// them.
static volatile bool preheatStatorFrontValid = false;
static volatile uint32_t preheatStatorFrontMs = 0;
static volatile uint32_t preheatStatorFrontKwx100 = 0;
static volatile bool preheatStatorRearValid = false;
static volatile uint32_t preheatStatorRearMs = 0;
static volatile uint32_t preheatStatorRearKwx100 = 0;


// Auto-shutoff. The target is tenths of a degree Celsius and is the only
// user-settable number in the feature; preheatAutoOffCount records how many
// times the engine switched itself off on reaching it, so the tab can say why
// the toggle is down.
static volatile int16_t preheatTargetCx10 = PREHEAT_TARGET_DEFAULT_CX10;
static volatile uint32_t preheatAutoOffCount = 0;

// TX state
static volatile uint32_t preheatTxOk = 0;
static volatile uint32_t preheatTxFail = 0;
static volatile uint32_t preheatEchoCount = 0;
static volatile uint32_t preheatLatUs = 0;
static volatile uint32_t preheatLastTxMs = 0;
static volatile uint32_t preheatMaxTxGapMs = 0;
static volatile uint8_t preheatLastReason = MCP_TX_OK;
static volatile uint8_t preheatLastSkipReason = PREHEAT_SKIP_DISABLED;
static volatile uint32_t preheatSkipCount[PREHEAT_SKIP_COUNT] = {};

static volatile bool preheatLastTxValid = false;
static uint8_t preheatLastTxRaw[8] = {};

// The ID the car's own trip planning uses, latched from its frames (0x082 or, on
// HW4, 0x347 - see PreheatTripIdPure). It is a property of the car, not of the
// bus session, so neither the bus reset nor the stats wipe clears it; it lives
// until reboot.
static volatile uint8_t preheatTripId = PREHEAT_TRIP_UNKNOWN;

// Immediate reassertion (see preheatReplyArmPure). The RX path arms, the tick
// transmits, so nothing goes out under a lock or from the receive loop.
// carConflict counts the car's own trip frames that disagreed with ours while the
// engine was on; replyTx / replyFail what the answer did; replyLat* the time from
// receiving that car frame to our answer being accepted by the controller.
static volatile bool preheatReplyArmed = false;
static volatile uint32_t preheatReplyArmedMs = 0;
static volatile uint32_t preheatLastReplyMs = 0;
static volatile uint32_t preheatCarConflictMs = 0;
static volatile uint32_t preheatCarConflict = 0;
static volatile uint32_t preheatReplyTx = 0;
static volatile uint32_t preheatReplyFail = 0;
static volatile uint32_t preheatReplyLatLastMs = 0;
static volatile uint32_t preheatReplyLatMaxMs = 0;
// Rate breaker on the answers (preheatReplyBreakerAllowPure). Guarded by
// preheatMux; a trip lasts until the next arming or bus restart.
static PreheatReplyBreakerPure preheatReplyBreaker = {};
// The field verdict on the reassertion: every hot->cold edge of the heat witness
// while transmitting, and how many of them came within PREHEAT_DROP_NEAR_CAR_MS of
// a disagreeing car frame.
static volatile bool preheatWasHot = false;
static volatile uint32_t preheatHeatDrops = 0;
static volatile uint32_t preheatHeatDropsNearCar = 0;

// ---------------------------------------------------------------------------
// The stator-stall clock, and the reason the engine last switched itself off.
//
// zeroSinceMs is 0 until the first successful transmission of the session and
// then holds the start of the current run of zero stator heat; a hot sample moves
// it to now. It is the whole of the criterion: the auto-off it fires is silent -
// no alarm row, no count-up on the tab - so nothing here needs to survive the
// bus reset that stopping performs. That reset clears this clock, and the engine
// is already off by then. The payload is a constant, so what the engine keeps
// from the car's frames is witnesses only.
// ---------------------------------------------------------------------------
static volatile uint32_t preheatStatorZeroSinceMs = 0;
static volatile uint8_t preheatAutoOffReason = PREHEAT_AUTOOFF_NONE;

// The live gate, republished on every tick. Deliberately separate from
// preheatLastSkipReason, which is history: a BOOT_DELAY skip from boot still
// reads BOOT_DELAY minutes later, so a tab that used it could not tell a
// stalled engine from a transmitting one. This is the value the strip names.
static volatile uint8_t preheatEvalReason = PREHEAT_SKIP_DISABLED;
static volatile uint32_t preheatLastEvalMs = 0;

// ---------------------------------------------------------------------------
// Snapshot handed to the web layer. Kept JSON-free so the runtime has no
// dependency on the writer, matching the other feature runtimes.
// ---------------------------------------------------------------------------
struct PreheatRuntimeSnapshot {
  bool supported;
  bool enabled;
  int16_t targetCx10;
  bool targetReached;
  uint32_t autoOffCount;
  bool sourceSeen;
  uint32_t sourceAgeMs;
  uint32_t sourceSkipped;
  // Witness for the field report: the exact bytes the last trip-planning frame
  // carried (0x082, or 0x347 on HW4 - tripId says which) and the exact bytes this
  // engine sent. srcRaw is what the car was saying, lastTxRaw what went out, so a
  // report can be checked against the car without a CAN log - both captures are
  // RX-only and cannot show our traffic.
  uint8_t tripId;
  uint8_t srcRaw[8];
  uint8_t lastTxRaw[8];
  // False until a frame has been sent, and false again after a bus restart; the
  // web layer renders lastTxRaw only when it is true.
  bool lastTxValid;
  // Whether that car frame's three route energies are valid and whether it claims
  // a Supercharger route (bit0 + bit1). A parked car sends neither.
  bool srcEnergiesValid;
  bool srcRouteBits;
  // True once a car frame with valid route energies has been cached. A witness
  // only, like the pair above: it says whether the car itself claims a routable
  // plan, never what this engine transmits.
  bool energyCacheValid;
  // Why the engine last switched itself off. The payload is a constant, so a
  // stall is the one failure that lastTxRaw cannot show: the frame looks right on
  // the way out and nothing heats. autoOffReason is "STATOR_STALL" for exactly
  // that case, and it is also the only place the stall is readable - the tab names
  // the reason in the status chip and paints nothing else.
  uint8_t autoOffReason;
  bool packSeen;
  int16_t packTempMinCx10;
  int16_t packTempMaxCx10;
  // The car's own pack average (0x332 mux 0 BMS_thermistorTAvg): what the auto-off
  // compares and what the PACK TEMP cell prints. Read from the car, never computed
  // here from the two cells above.
  bool avgSeen;
  uint32_t avgAgeMs;
  int16_t packAvgCx10;
  bool bmsSeen;
  bool bmsAllowed;
  bool bmsWorthwhile;
  bool bmsKeepWarm;
  bool heatSeen;
  uint32_t heatKwx100;
  // Age of the 0x252 frame the limit above came from: the raw telemetry row prints
  // it next to the value, so a remembered permission cannot read as a live one.
  uint32_t heatAgeMs;
  bool coolantSeen;
  uint16_t coolantLpmX10;
  uint8_t coolantReason;
  bool heatingActive;
  bool heatingSeen;
  bool active;
  uint32_t txOk;
  uint32_t txFail;
  uint32_t echoCount;
  uint32_t latUs;
  uint32_t lastTxAgeMs;
  uint32_t maxTxGapMs;
  uint8_t lastReason;
  uint8_t lastSkipReason;
  // The live gate and how long ago it was evaluated. evalAgeMs grows only when
  // the CAN service stage itself stops running, which is the one stall the skip
  // counters cannot show: they freeze with the service instead of moving.
  uint8_t evalReason;
  // Immediate reassertion, per armed session (see preheatReplyArmPure).
  uint32_t carConflict;
  uint32_t replyTx;
  uint32_t replyFail;
  uint32_t replyLatLastMs;
  uint32_t replyLatMaxMs;
  uint32_t heatDrops;
  uint32_t heatDropsNearCar;
  bool replyBreaker;
  uint32_t evalAgeMs;
  // Armed and held back by that gate. The tab keys the strip's status off this
  // and never off lastSkipReason.
  bool blocked;
  // Per-signal telemetry, each with its own ever-seen flag and age: the signals
  // arrive in different IDs at different rates, and a value without its age cannot
  // be told apart from a stale one.
  bool maxPackSeen;
  uint32_t maxPackAgeMs;
  int16_t maxPackCx10;
  // 0x321 byte3 VCFRONT_tempAmbient: the car's own outside-air reading, kept as a
  // witness next to the pack it is preheating. Its only job on the tab is the
  // AMBIENT cell, so it is fetched straight from the car's frame like the rest.
  bool ambientSeen;
  uint32_t ambientAgeMs;
  int16_t ambientCx10;
  // Thermal schematic: value + age of its frame (UINT32_MAX = never seen).
  uint32_t thVitalsAgeMs;
  uint32_t thPumpBatRpm;
  uint32_t thPumpPtRpm;
  uint32_t thFanRpm;
  uint32_t thModeAgeMs;
  uint8_t thValveMode;
  uint8_t thHpReq;
  uint32_t thSysAgeMs;
  bool thHeatPump;
  uint32_t thInletsAgeMs;
  int16_t thBatInletCx10;
  int16_t thPtInletCx10;
  uint32_t thFlowsAgeMs;
  uint16_t thFlowBatLpmX10;
  uint16_t thFlowPtLpmX10;
  uint32_t thPcsAgeMs;
  int16_t thDcdcCx10;
  int16_t thChargerCx10;
  uint32_t thInvFrontAgeMs;
  int16_t thInvFrontC;
  uint32_t thInvRearAgeMs;
  int16_t thInvRearC;
  uint32_t thApAgeMs;
  int16_t thApC;
  uint32_t thInfoAgeMs;
  int16_t thInfoC;
  bool flowReqSeen;
  uint32_t flowReqAgeMs;
  uint8_t flowReqLpmX4;
  bool cellTargetSeen;
  uint32_t cellTargetAgeMs;
  int16_t cellTargetCx4;
  bool hpPowerSeen;
  uint32_t hpPowerAgeMs;
  uint32_t hpPowerW;
  bool pumpSeen;
  uint32_t pumpAgeMs;
  uint32_t pumpRpm;
  // Stator heating, split by drive unit. The two sides are what the strip prints
  // and what the stall criterion reads; the API sums them into statorHeatKw for
  // the HEAT POWER cell.
  bool statorFrontSeen;
  uint32_t statorFrontAgeMs;
  uint32_t statorFrontKwx100;
  bool statorRearSeen;
  uint32_t statorRearAgeMs;
  uint32_t statorRearKwx100;
};

static inline bool preheatProfileSupported() {
  return activeProfilePreheatSupported();
}

// Whether this build and profile can preheat at all. The Settings menu flag is
// deliberately absent: it is presentation, and the engine keeps running while
// the tab is hidden, so a capability report that followed it would contradict
// the tab the moment the tab is shown again.
static inline bool preheatFeatureSupported() {
  return preheatProfileSupported();
}

// ---------------------------------------------------------------------------
// Persistence. Kept in its own NVS namespace, exactly like the USB logger's
// listen-only flag, so enabling the Actions tab never touches the packed
// feature-config blob or its migration path.
// ---------------------------------------------------------------------------
static constexpr const char *PREHEAT_NVS_NAMESPACE = "preheat";
static constexpr const char *PREHEAT_NVS_MENU_KEY = "menu";
static constexpr const char *PREHEAT_NVS_ENABLED_KEY = "enabled";
static constexpr const char *PREHEAT_NVS_TARGET_KEY = "target";

static bool preheatNvsWrite(const char *key, bool value) {
  Preferences prefs;
  if (!prefs.begin(PREHEAT_NVS_NAMESPACE, false)) return false;
  const bool ok = prefs.putBool(key, value) == 1 &&
                  prefs.getBool(key, !value) == value;
  prefs.end();
  return ok;
}

static void preheatMenuLoad() {
  Preferences prefs;
  // A failed configuration access leaves the tab hidden and the engine stopped
  // rather than enabling a CAN transmitter by default.
  bool menu = false;
  bool enabled = false;
  int16_t target = PREHEAT_TARGET_DEFAULT_CX10;
  if (prefs.begin(PREHEAT_NVS_NAMESPACE, false)) {
    menu = prefs.getBool(PREHEAT_NVS_MENU_KEY, false);
    enabled = prefs.getBool(PREHEAT_NVS_ENABLED_KEY, false);
    // A value written by an older or newer build is clamped rather than
    // trusted, so a bad blob can never arm an auto-shutoff that fires at once.
    target = preheatClampTargetCx10(
        prefs.getInt(PREHEAT_NVS_TARGET_KEY, PREHEAT_TARGET_DEFAULT_CX10));
    prefs.end();
  }
  // The two switches are read independently. `menu` only decides whether the
  // tab is in the nav; `enabled` is the engine, so a preheat that was running
  // when the tab was hidden resumes after a reboot exactly as it was left.
  preheatMenuEnabled = menu;
  preheatEnabled = enabled;
  preheatTargetCx10 = target;
}

// Settings toggle: shows or hides the whole Actions tab. Presentation only -
// it deliberately neither stops the engine nor clears its flag, so a preheat
// that was already running keeps running and the tab comes back showing the
// state the car is actually in. Stopping is the tab's own toggle
// (preheatSetEnabled) or the auto-off target.
static bool preheatMenuSetEnabled(bool enabled) {
  if (!preheatNvsWrite(PREHEAT_NVS_MENU_KEY, enabled)) return false;
  preheatMenuEnabled = enabled;
  return true;
}

// A fresh armed session: the stall clock and the reason the last one ended are
// cleared, so a re-armed engine never opens on the previous run's verdict. Called
// from every path that arms the engine and from the stats reset.
static inline void preheatSessionRestart() {
  portENTER_CRITICAL(&preheatMux);
  preheatStatorZeroSinceMs = 0;
  preheatAutoOffReason = PREHEAT_AUTOOFF_NONE;
  // The reassertion witnesses describe one run, so each arming starts them at 0
  // and a field reading is never mixed with the previous session's.
  preheatReplyArmed = false;
  preheatLastReplyMs = 0;
  preheatCarConflictMs = 0;
  preheatCarConflict = 0;
  preheatReplyTx = 0;
  preheatReplyFail = 0;
  preheatReplyLatLastMs = 0;
  preheatReplyLatMaxMs = 0;
  preheatWasHot = false;
  preheatHeatDrops = 0;
  preheatHeatDropsNearCar = 0;
  preheatReplyBreaker = {};
  portEXIT_CRITICAL(&preheatMux);
}

// Tab toggle: starts or stops the injection engine.
static bool preheatSetEnabled(bool enabled) {
  if (!preheatNvsWrite(PREHEAT_NVS_ENABLED_KEY, enabled)) return false;
  preheatEnabled = enabled;
  if (!enabled) preheatRuntimeResetUnderTxBarrier();
  if (enabled) preheatSessionRestart();
  return true;
}

// S3XY button binding. It flips the same engine flag the tab's own toggle does,
// so the button and the UI can never disagree, and the NVS write is the one
// preheatSetEnabled already performs. The profile gate repeats what
// s3xyActionSupportedForCurrentProfile() checked when the gesture fired, because
// the profile can change while a stored mapping is still in place.
static void togglePreheatEnabledFromButton() {
  if (!activeProfilePreheatSupported()) {
    s3xyLogPush(S3XY_LOG_INFO, "Battery Preheating unavailable for current profile");
    return;
  }
  const bool wasEnabled = preheatEnabled;
  if (!preheatSetEnabled(!wasEnabled)) {
    s3xyLogPush(S3XY_LOG_INFO, "Battery Preheating toggle failed: settings save failed");
    return;
  }
  s3xyLogPush(S3XY_LOG_INFO, wasEnabled ? "Battery Preheating -> OFF" : "Battery Preheating -> ON");
}

// Auto-shutoff target setter. Clamping is a safety net only: the HTTP layer
// validates the range and answers 400 before this is reached.
static bool preheatSetTargetCx10(int32_t value) {
  const int16_t target = preheatClampTargetCx10(value);
  Preferences prefs;
  if (!prefs.begin(PREHEAT_NVS_NAMESPACE, false)) return false;
  const bool ok =
      prefs.putInt(PREHEAT_NVS_TARGET_KEY, (int32_t)target) == sizeof(int32_t) &&
      prefs.getInt(PREHEAT_NVS_TARGET_KEY, -1) == (int32_t)target;
  prefs.end();
  if (ok) preheatTargetCx10 = target;
  return ok;
}

static inline uint32_t preheatAgeMs(uint32_t now, uint32_t stamp) {
  return stamp ? (uint32_t)(now - stamp) : UINT32_MAX;
}

// ---------------------------------------------------------------------------
// Snapshot. One lock, one copy: the web layer never touches live state.
// ---------------------------------------------------------------------------
static inline void preheatRuntimeSnapshot(uint32_t now,
                                          PreheatRuntimeSnapshot &out) {
  out = {};
  portENTER_CRITICAL(&preheatMux);
  out.enabled = preheatEnabled;
  out.targetCx10 = preheatTargetCx10;
  out.autoOffCount = preheatAutoOffCount;
  out.sourceSeen = preheatSourceValid;
  out.sourceAgeMs = preheatAgeMs(now, preheatSourceMs);
  out.sourceSkipped = preheatSourceSkipped;
  out.tripId = preheatTripId;
  memcpy(out.srcRaw, preheatSourceRaw, sizeof(out.srcRaw));
  memcpy(out.lastTxRaw, preheatLastTxRaw, sizeof(out.lastTxRaw));
  out.lastTxValid = preheatLastTxValid;
  out.srcEnergiesValid = preheatEnergiesValidPure(preheatSourceRaw, PREHEAT_DLC);
  out.srcRouteBits = preheatSourceSeenPure(preheatSourceRaw, PREHEAT_DLC);
  out.energyCacheValid = preheatEnergyCacheValid;
  out.autoOffReason = preheatAutoOffReason;
  out.packSeen = preheatPackValid;
  out.packTempMinCx10 = preheatPackMinCx10;
  out.packTempMaxCx10 = preheatPackMaxCx10;
  out.avgSeen = preheatAvgValid;
  out.avgAgeMs = preheatAgeMs(now, preheatAvgMs);
  out.packAvgCx10 = preheatPackAvgCx10;
  // The same predicate the tick acts on, evaluated on the same locked snapshot
  // and on the same number - the car's own average - so the tab can never show
  // "Target reached" while the engine keeps heating.
  out.targetReached = preheatTargetReachedPure(
      preheatAvgValid, preheatAgeMs(now, preheatAvgMs), preheatPackAvgCx10,
      preheatTargetCx10, PREHEAT_PACK_FRESH_MS);
  out.bmsSeen = preheatBmsValid;
  out.bmsAllowed = preheatBmsAllowed;
  out.bmsWorthwhile = preheatBmsWorthwhile;
  out.bmsKeepWarm = preheatBmsKeepWarm;
  out.heatSeen = preheatHeatValid;
  out.heatKwx100 = preheatHeatKwx100;
  out.heatAgeMs = preheatAgeMs(now, preheatHeatMs);
  out.coolantSeen = preheatCoolantValid;
  out.coolantLpmX10 = preheatCoolantLpmX10;
  out.coolantReason = preheatCoolantReason;
  out.heatingSeen = preheatHeatingValid;
  out.heatingActive = preheatHeatingActive;
  out.txOk = preheatTxOk;
  out.txFail = preheatTxFail;
  out.echoCount = preheatEchoCount;
  out.latUs = preheatLatUs;
  out.lastTxAgeMs = preheatAgeMs(now, preheatLastTxMs);
  out.maxTxGapMs = preheatMaxTxGapMs;
  out.lastReason = preheatLastReason;
  out.lastSkipReason = preheatLastSkipReason;
  out.evalReason = preheatEvalReason;
  out.carConflict = preheatCarConflict;
  out.replyTx = preheatReplyTx;
  out.replyFail = preheatReplyFail;
  out.replyLatLastMs = preheatReplyLatLastMs;
  out.replyLatMaxMs = preheatReplyLatMaxMs;
  out.heatDrops = preheatHeatDrops;
  out.heatDropsNearCar = preheatHeatDropsNearCar;
  out.replyBreaker = preheatReplyBreaker.tripped;
  out.maxPackSeen = preheatThermalMux0Valid;
  out.maxPackAgeMs = preheatAgeMs(now, preheatThermalMux0Ms);
  out.maxPackCx10 = preheatMaxPackCx10;
  out.flowReqSeen = preheatThermalMux0Valid;
  out.flowReqAgeMs = preheatAgeMs(now, preheatThermalMux0Ms);
  out.flowReqLpmX4 = preheatFlowReqLpmX4;
  out.cellTargetSeen = preheatThermalMux1Valid;
  out.cellTargetAgeMs = preheatAgeMs(now, preheatThermalMux1Ms);
  out.cellTargetCx4 = preheatCellTargetCx4;
  out.hpPowerSeen = preheatHpPowerValid;
  out.hpPowerAgeMs = preheatAgeMs(now, preheatHpPowerMs);
  out.hpPowerW = preheatHpPowerW;
  out.pumpSeen = preheatPumpValid;
  out.pumpAgeMs = preheatAgeMs(now, preheatPumpMs);
  out.pumpRpm = preheatPumpRpm;
  out.ambientSeen = preheatAmbientValid;
  out.ambientAgeMs = preheatAgeMs(now, preheatAmbientMs);
  out.ambientCx10 = preheatAmbientCx10;
  out.thVitalsAgeMs = preheatAgeMs(now, preheatThVitalsMs);
  out.thPumpBatRpm = preheatThPumpBatRpm;
  out.thPumpPtRpm = preheatThPumpPtRpm;
  out.thFanRpm = preheatThFanRpm;
  out.thModeAgeMs = preheatAgeMs(now, preheatThModeMs);
  out.thValveMode = preheatThValveMode;
  out.thHpReq = preheatThHpReq;
  out.thSysAgeMs = preheatAgeMs(now, preheatThSysMs);
  out.thHeatPump = preheatThHeatPump;
  out.thInletsAgeMs = preheatAgeMs(now, preheatThInletsMs);
  out.thBatInletCx10 = preheatThBatInletCx10;
  out.thPtInletCx10 = preheatThPtInletCx10;
  out.thFlowsAgeMs = preheatAgeMs(now, preheatThFlowsMs);
  out.thFlowBatLpmX10 = preheatThFlowBatLpmX10;
  out.thFlowPtLpmX10 = preheatThFlowPtLpmX10;
  out.thPcsAgeMs = preheatAgeMs(now, preheatThPcsMs);
  out.thDcdcCx10 = preheatThDcdcCx10;
  out.thChargerCx10 = preheatThChargerCx10;
  out.thInvFrontAgeMs = preheatAgeMs(now, preheatThInvFrontMs);
  out.thInvFrontC = preheatThInvFrontC;
  out.thInvRearAgeMs = preheatAgeMs(now, preheatThInvRearMs);
  out.thInvRearC = preheatThInvRearC;
  out.thApAgeMs = preheatAgeMs(now, preheatThApMs);
  out.thApC = preheatThApC;
  out.thInfoAgeMs = preheatAgeMs(now, preheatThInfoMs);
  out.thInfoC = preheatThInfoC;
  out.statorFrontSeen = preheatStatorFrontValid;
  out.statorFrontAgeMs = preheatAgeMs(now, preheatStatorFrontMs);
  out.statorFrontKwx100 = preheatStatorFrontKwx100;
  out.statorRearSeen = preheatStatorRearValid;
  out.statorRearAgeMs = preheatAgeMs(now, preheatStatorRearMs);
  out.statorRearKwx100 = preheatStatorRearKwx100;
  const uint32_t lastEvalMs = preheatLastEvalMs;
  portEXIT_CRITICAL(&preheatMux);
  // 0 means "not evaluated yet", which reads the same as "evaluated just now":
  // both are fine, and only a growing value means the service stopped.
  out.evalAgeMs = lastEvalMs ? (uint32_t)(now - lastEvalMs) : 0u;
  // The heat capability is what "the BMS will accept preheat" means in
  // practice: 0x212 bit 5 never rises on the reference firmware, so the row
  // must not depend on it alone.
  out.supported = preheatFeatureSupported();
  out.active = out.enabled && out.supported;
  // Armed and held back by a gate: the state the strip must not paint as
  // "Heating". Evaluated with the same predicate the tick acts on, on the same
  // locked copy, so the two cannot disagree.
  out.blocked = preheatStalledPure(out.enabled, out.supported, out.evalReason);
}

// ---------------------------------------------------------------------------
// RX ingest. Shared by both physical buses: the caller has already established
// that this frame arrived on the Vehicle/Body channel.
//
// Only the mux/index the signal the tab consumes lives in is accepted; the other
// payloads of a multiplexed frame carry unrelated fields and would corrupt the
// display (the "Tmin > Tmax" trap).
// ---------------------------------------------------------------------------

// Both stator frames feed the same question - is either drive unit putting heat
// in the pack right now - so the freshness rule lives here rather than twice in
// the switch. Nothing is accumulated: the tick reads this to run the stall clock.
// Caller holds the mux.
static inline bool preheatStatorHotLocked(uint32_t now) {
  const bool fuseFresh =
      preheatStatorFrontValid &&
      preheatAgeMs(now, preheatStatorFrontMs) <= PREHEAT_STATOR_PAIR_FRESH_MS;
  const bool rearFresh =
      preheatStatorRearValid &&
      preheatAgeMs(now, preheatStatorRearMs) <= PREHEAT_STATOR_PAIR_FRESH_MS;
  return (fuseFresh && preheatStatorFrontKwx100 > 0u) ||
         (rearFresh && preheatStatorRearKwx100 > 0u);
}
static inline void preheatRuntimeObserve(uint16_t id, uint8_t dlc,
                                         const uint8_t *data, uint32_t now) {
  if (data == nullptr) return;
  switch (id) {
    case PREHEAT_SOURCE_ID:
    case PREHEAT_TRIP_ID_ALT: {
      // Our own injected frame must not be read back as a car observation, or the
      // src* witness fields would be describing our own traffic - nor vote for the
      // trip ID it was sent on.
      bool lastValid = false;
      uint32_t lastMs = 0;
      uint8_t lastRaw[8];
      portENTER_CRITICAL(&preheatMux);
      lastValid = preheatLastTxValid;
      lastMs = preheatLastTxMs;
      memcpy(lastRaw, preheatLastTxRaw, sizeof(lastRaw));
      portEXIT_CRITICAL(&preheatMux);
      if (preheatIsSelfEchoPure(lastValid, preheatAgeMs(now, lastMs), data,
                                lastRaw, dlc)) {
        // Our own transmission is not a new observation of the car, so it must
        // not refresh the src* witness fields.
        return;
      }
      // First the latch: which ID the car's trip planning is on. Then the
      // witnesses, from that ID's frames only - a 0x347 on a car that speaks 0x082
      // is something else (a charger alert on the older maps) and is not described.
      const uint8_t tripId = preheatTripIdNextPure(preheatTripId, id, dlc, data);
      preheatTripId = tripId;
      if (tripId == PREHEAT_TRIP_UNKNOWN || preheatTripCanIdPure(tripId) != id)
        return;
      // The car stated something other than our frame on our ID: arm the answer.
      // Only while the engine is on - an idle engine has nothing to reassert.
      if (preheatEnabled &&
          preheatReplyArmPure(data, preheatTripFramePure(tripId), dlc)) {
        portENTER_CRITICAL(&preheatMux);
        preheatReplyArmed = true;
        preheatReplyArmedMs = now;
        preheatCarConflictMs = now;
        if (preheatCarConflict < UINT32_MAX) preheatCarConflict++;
        portEXIT_CRITICAL(&preheatMux);
      }
      // The engine keeps witnesses from the car's frame and nothing else: the
      // payload is a constant, so there is no template to hold and no slot to arm.
      const bool energiesValid = preheatEnergiesValidPure(data, dlc);
      portENTER_CRITICAL(&preheatMux);
      preheatSourceValid = true;
      preheatSourceMs = now;
      memcpy(preheatSourceRaw, data, sizeof(preheatSourceRaw));
      if (energiesValid) {
        memcpy(preheatEnergyCache, data, sizeof(preheatEnergyCache));
        preheatEnergyCacheValid = true;
      }
      if (!preheatSourceSeenPure(data, dlc) && preheatSourceSkipped < UINT32_MAX)
        preheatSourceSkipped++;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x332: {
      int16_t tmin = 0, tmax = 0, tavg = 0;
      if (!preheatDecodeBrickTempsPure(data, dlc, tmin, tmax)) return;
      // The car computes its own average over its thermistors; it is carried in
      // the same mux and is not the mean of the two cells above.
      const bool hasAvg = preheatDecodeBrickTempAvgPure(data, dlc, tavg);
      portENTER_CRITICAL(&preheatMux);
      preheatPackValid = true;
      preheatPackMs = now;
      preheatPackMinCx10 = tmin;
      preheatPackMaxCx10 = tmax;
      if (hasAvg) {
        preheatAvgValid = true;
        preheatAvgMs = now;
        preheatPackAvgCx10 = tavg;
      }
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x252: {
      uint32_t kw = 0;
      if (!preheatDecodeHeatPowerPure(data, dlc, kw)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatHeatValid = true;
      preheatHeatMs = now;
      preheatHeatKwx100 = kw;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x241: {
      uint16_t lpm = 0;
      uint8_t reason = 0;
      if (!preheatDecodeCoolantPure(data, dlc, lpm, reason)) return;
      uint16_t flowBat = 0, flowPt = 0;
      const bool flows = preheatDecodeLoopFlowsPure(data, dlc, flowBat, flowPt);
      portENTER_CRITICAL(&preheatMux);
      preheatCoolantValid = true;
      preheatCoolantMs = now;
      preheatCoolantLpmX10 = lpm;
      preheatCoolantReason = reason;
      if (flows) {
        preheatThFlowsMs = now;
        preheatThFlowBatLpmX10 = flowBat;
        preheatThFlowPtLpmX10 = flowPt;
      }
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x212: {
      bool allowed = false;
      if (!preheatDecodeBmsStatusPure(data, dlc, allowed)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatBmsValid = true;
      preheatBmsMs = now;
      preheatBmsAllowed = allowed;
      // This frame carries no heating verdict on this firmware: bit 5 is
      // BMS_cpMiaOnHvs and bit 30 BMS_chargeRequest. The worthwhile flag is
      // published from 0x312 mux0 below, and no keep-warm signal exists in the
      // car's own map, so the field stays false rather than mirroring a bit that
      // means something else.
      preheatBmsKeepWarm = false;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x2E1: {
      bool active = false;
      if (!preheatDecodeHeatingActivePure(data, dlc, active)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatHeatingValid = true;
      preheatHeatingMs = now;
      preheatHeatingActive = active;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x312: {
      int16_t maxPackCx10 = 0;
      uint8_t flowReqLpmX4 = 0;
      int16_t cellTargetCx4 = 0;
      bool worthwhile = false;
      if (preheatDecodeThermalMux0Pure(data, dlc, maxPackCx10, flowReqLpmX4,
                                       worthwhile)) {
        portENTER_CRITICAL(&preheatMux);
        preheatThermalMux0Valid = true;
        preheatThermalMux0Ms = now;
        preheatMaxPackCx10 = maxPackCx10;
        preheatFlowReqLpmX4 = flowReqLpmX4;
        // The BMS's own "heating is worth it" verdict travels here, in the same
        // mux as the temperature it is about.
        preheatBmsWorthwhile = worthwhile;
        portEXIT_CRITICAL(&preheatMux);
        return;
      }
      if (preheatDecodeThermalMux1Pure(data, dlc, cellTargetCx4)) {
        portENTER_CRITICAL(&preheatMux);
        preheatThermalMux1Valid = true;
        preheatThermalMux1Ms = now;
        preheatCellTargetCx4 = cellTargetCx4;
        portEXIT_CRITICAL(&preheatMux);
      }
      return;
    }
    case 0x3A1: {
      uint32_t watts = 0;
      bool heatPump = false;
      if (!preheatDecodeHpBattHeatPowerPure(data, dlc, watts)) return;
      preheatDecodeThermalSystemPure(data, dlc, heatPump);
      portENTER_CRITICAL(&preheatMux);
      preheatHpPowerValid = true;
      preheatHpPowerMs = now;
      preheatHpPowerW = watts;
      preheatThSysMs = now;
      preheatThHeatPump = heatPump;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x201: {
      // Index 0: pumps and fan (see preheatVitals201IndexZeroPure); index 1: the
      // coolant valve. Indices 2 and 3 are not read.
      uint8_t valve = 0, hpReq = 0;
      if (preheatDecodeCoolantMode201Pure(data, dlc, valve, hpReq)) {
        portENTER_CRITICAL(&preheatMux);
        preheatThModeMs = now;
        preheatThValveMode = valve;
        preheatThHpReq = hpReq;
        portEXIT_CRITICAL(&preheatMux);
        return;
      }
      uint32_t pumpBat = 0, pumpPt = 0, fan = 0;
      if (!preheatDecodeVitals201Pure(data, dlc, pumpBat, pumpPt, fan)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatPumpValid = true;
      preheatPumpMs = now;
      preheatPumpRpm = pumpBat;
      preheatThVitalsMs = now;
      preheatThPumpBatRpm = pumpBat;
      preheatThPumpPtRpm = pumpPt;
      preheatThFanRpm = fan;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x2A4: {
      int16_t dcdc = 0, charger = 0;
      if (!preheatDecodePcsTempsPure(data, dlc, dcdc, charger)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatThPcsMs = now;
      preheatThDcdcCx10 = dcdc;
      preheatThChargerCx10 = charger;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case PREHEAT_DIF_TEMP_ID:
    case PREHEAT_DIR_TEMP_ID: {
      int16_t inv = 0;
      if (!preheatDecodeStatorTempPure(data, dlc, inv)) return;
      portENTER_CRITICAL(&preheatMux);
      if (id == PREHEAT_DIF_TEMP_ID) {
        preheatThInvFrontMs = now;
        preheatThInvFrontC = inv;
      } else {
        preheatThInvRearMs = now;
        preheatThInvRearC = inv;
      }
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case PREHEAT_APP_TEMP_ID: {
      int16_t ap = 0;
      if (!preheatDecodeApTempPure(data, dlc, ap)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatThApMs = now;
      preheatThApC = ap;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x353: {
      int16_t cpu = 0;
      if (!preheatDecodeInfotainmentTempPure(data, dlc, cpu)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatThInfoMs = now;
      preheatThInfoC = cpu;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case 0x321: {
      int16_t batIn = 0, ptIn = 0;
      const bool inlets = preheatDecodeCoolantInletsPure(data, dlc, batIn, ptIn);
      if (inlets) {
        portENTER_CRITICAL(&preheatMux);
        preheatThInletsMs = now;
        preheatThBatInletCx10 = batIn;
        preheatThPtInletCx10 = ptIn;
        portEXIT_CRITICAL(&preheatMux);
      }
      int16_t ambientCx10 = 0;
      if (!preheatDecodeAmbientPure(data, dlc, ambientCx10)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatAmbientValid = true;
      preheatAmbientMs = now;
      preheatAmbientCx10 = ambientCx10;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case PREHEAT_STATOR_DIF_ID: {
      uint32_t kwX100 = 0;
      if (!preheatDecodeStatorHeatPure(data, dlc, kwX100)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatStatorFrontValid = true;
      preheatStatorFrontMs = now;
      preheatStatorFrontKwx100 = kwX100;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    case PREHEAT_STATOR_DIR_ID: {
      uint32_t kwX100 = 0;
      if (!preheatDecodeStatorHeatPure(data, dlc, kwX100)) return;
      portENTER_CRITICAL(&preheatMux);
      preheatStatorRearValid = true;
      preheatStatorRearMs = now;
      preheatStatorRearKwx100 = kwX100;
      portEXIT_CRITICAL(&preheatMux);
      return;
    }
    default: return;
  }
}

// ---------------------------------------------------------------------------
// Bus-specific ingest. Each bus only feeds the feature when the active profile
// says that channel carries the Vehicle/Body bus, and only for the IDs the
// tab consumes. The ID gate keeps the per-frame cost on the RX hot path to a
// single comparison chain.
// ---------------------------------------------------------------------------
static inline bool preheatIdOfInterest(uint16_t id) {
  return id == PREHEAT_SOURCE_ID || id == PREHEAT_TRIP_ID_ALT ||
         id == 0x212 || id == 0x241 ||
         id == 0x252 || id == 0x2E1 || id == 0x332 || id == 0x312 ||
         id == 0x3A1 || id == 0x201 || id == 0x321 || id == 0x2A4 ||
         id == 0x353 || id == PREHEAT_DIF_TEMP_ID || id == PREHEAT_DIR_TEMP_ID ||
         id == PREHEAT_STATOR_DIF_ID || id == PREHEAT_STATOR_DIR_ID;
}

// Read-only IDs taken from whichever channel carries them. The Autopilot board
// temperature travels on the Chassis bus only. 0x321 (ambient, coolant inlets) and
// 0x353 (infotainment CPU) are on both, with the same content (self-heating capture: 96 / 273
// and 136 / 273 frames on A / B, identical values), but at 2 s with gaps up to
// 17 s on the Vehicle bus against 1 s on the other - so either copy refreshes them.
static inline bool preheatIdEitherBus(uint16_t id) {
  return id == PREHEAT_APP_TEMP_ID || id == 0x321 || id == 0x353;
}

static inline void preheatRuntimeIngestCanA(const struct can_frame &f) {
  const uint16_t id = (uint16_t)(f.can_id & 0x7FFu);
  if (preheatIdEitherBus(id)) {
    preheatRuntimeObserve(id, f.can_dlc, f.data, (uint32_t)millis());
    return;
  }
  if (!activeProfileVehicleBusOnCanA()) return;
  if (!preheatIdOfInterest(id)) return;
  preheatRuntimeObserve(id, f.can_dlc, f.data, (uint32_t)millis());
}

static inline void preheatRuntimeIngestCanB(const twai_message_t &f) {
  const uint16_t id = (uint16_t)(f.identifier & 0x7FFu);
  if (preheatIdEitherBus(id)) {
    preheatRuntimeObserve(id, (uint8_t)f.data_length_code, f.data,
                          (uint32_t)millis());
    return;
  }
  if (!activeProfileVehicleBusOnCanB()) return;
  if (!preheatIdOfInterest(id)) return;
  preheatRuntimeObserve(id, (uint8_t)f.data_length_code, f.data,
                        (uint32_t)millis());
}

// ---------------------------------------------------------------------------
// Transmit. The channel is resolved from the profile, never hardcoded: on
// YL_PARTY_VH the Vehicle/Body bus is CAN B (TWAI), everywhere else CAN A.
//
// One shape per car, one cadence: the car's own preconditioning frame goes out
// unchanged on the ID its trip planning uses - PREHEAT_REFERENCE_FRAME on 0x082,
// PREHEAT_REFERENCE_FRAME_347 on 0x347. Both are constexpr, so there is no shared
// state to copy out - the payload is built here, and nothing is transmitted while
// holding a critical section, which is the rule the R79 paths follow too. The tick
// only calls this once the trip ID is known.
// ---------------------------------------------------------------------------
static inline void preheatRuntimeTransmit(uint32_t now, bool isReply = false) {
  const uint8_t tripId = preheatTripId;
  const uint32_t txId = preheatTripCanIdPure(tripId);
  uint8_t payload[PREHEAT_DLC] = {};
  memcpy(payload, preheatTripFramePure(tripId), sizeof(payload));
  const uint32_t txEpoch = canTxEpochSnapshot();
  const uint32_t t0 = micros();
  bool txOk = false;
  uint8_t reason = MCP_TX_INVALID_MSG;

  // Capture the previous transmission stamp before any success path overwrites
  // it, otherwise the gap measurement would always read zero.
  uint32_t prevTxMs = 0;
  portENTER_CRITICAL(&preheatMux);
  prevTxMs = preheatLastTxMs;
  portEXIT_CRITICAL(&preheatMux);

  if (activeProfileVehicleBusOnCanA()) {
    struct can_frame txf = {};
    txf.can_id = txId;
    txf.can_dlc = PREHEAT_DLC;
    memcpy(txf.data, payload, PREHEAT_DLC);
    MCP2515::ERROR err = MCP2515::ERROR_FAIL;
    McpTxResultReason txReason = MCP_TX_INVALID_MSG;
    const bool attempted = canTxMcpSend(&txf, txEpoch, err, &txReason);
    txOk = attempted && err == MCP2515::ERROR_OK;
    reason = (uint8_t)txReason;
    if (txOk) {
      portENTER_CRITICAL(&preheatMux);
      memcpy(preheatLastTxRaw, txf.data, sizeof(preheatLastTxRaw));
      preheatLastTxMs = now;
      preheatLastTxValid = true;
      portEXIT_CRITICAL(&preheatMux);
    }
  } else if (activeProfileVehicleBusOnCanB()) {
    twai_message_t txb = {};
    txb.identifier = txId;
    txb.data_length_code = PREHEAT_DLC;
    memcpy(txb.data, payload, PREHEAT_DLC);
    const esp_err_t err =
        canTxTwaiTransmitWithMask(&txb, txEpoch, CAN_TX_FRESH_VH);
    txOk = err == ESP_OK;
    // The TWAI path reports an esp_err_t rather than an McpTxResultReason, so
    // only success and a generic send failure can be distinguished here.
    reason = txOk ? (uint8_t)MCP_TX_OK : (uint8_t)MCP_TX_SEND_ERROR;
    if (txOk) {
      portENTER_CRITICAL(&preheatMux);
      memcpy(preheatLastTxRaw, txb.data, sizeof(preheatLastTxRaw));
      preheatLastTxMs = now;
      preheatLastTxValid = true;
      portEXIT_CRITICAL(&preheatMux);
    }
  } else {
    // The profile has no Vehicle/Body channel on either bus. The eligibility gate
    // reports that as UNSUPPORTED_PROFILE long before this point, so there is
    // nothing to account for here.
    return;
  }

  const uint32_t latUs = (uint32_t)(micros() - t0);
  portENTER_CRITICAL(&preheatMux);
  preheatLastReason = reason;
  preheatLatUs = latUs;
  if (isReply) {
    // The arm is consumed whatever happened: one answer per car frame, and a
    // failed one is not retried - the periodic frame follows within 100 ms.
    preheatReplyArmed = false;
    preheatLastReplyMs = now;
    if (txOk) {
      if (preheatReplyTx < UINT32_MAX) preheatReplyTx++;
      const uint32_t lat = (uint32_t)(now - preheatReplyArmedMs);
      preheatReplyLatLastMs = lat;
      if (lat > preheatReplyLatMaxMs) preheatReplyLatMaxMs = lat;
    } else if (preheatReplyFail < UINT32_MAX) {
      preheatReplyFail++;
    }
  }
  if (txOk) {
    preheatTxOk++;
    preheatEchoCount++;
    preheatMaxTxGapMs =
        preheatGapMaxUpdatePure(prevTxMs, now, preheatMaxTxGapMs);
  } else {
    preheatTxFail++;
  }
  portEXIT_CRITICAL(&preheatMux);
}

// ---------------------------------------------------------------------------
// Periodic service. Runs from the CAN service stage, so it inherits the same
// cadence as the other feature ticks.
// ---------------------------------------------------------------------------
static inline void preheatRuntimeCountSkip(uint8_t reason) {
  if (reason >= PREHEAT_SKIP_COUNT) return;
  portENTER_CRITICAL(&preheatMux);
  preheatLastSkipReason = reason;
  if (preheatSkipCount[reason] < UINT32_MAX) preheatSkipCount[reason]++;
  portEXIT_CRITICAL(&preheatMux);
}

// Auto-shutoff test against the live state, on one locked read. It reads the
// car's own pack average - the same field the PACK TEMP cell prints and the same
// predicate the snapshot publishes - so the verdict cannot disagree with what the
// tab shows.
static inline bool preheatTargetReachedNow(uint32_t now) {
  bool avgValid = false;
  uint32_t avgMs = 0;
  int16_t avgCx10 = 0;
  int16_t targetCx10 = PREHEAT_TARGET_DEFAULT_CX10;
  portENTER_CRITICAL(&preheatMux);
  avgValid = preheatAvgValid;
  avgMs = preheatAvgMs;
  avgCx10 = preheatPackAvgCx10;
  targetCx10 = preheatTargetCx10;
  portEXIT_CRITICAL(&preheatMux);
  return preheatTargetReachedPure(avgValid, preheatAgeMs(now, avgMs),
                                  avgCx10, targetCx10, PREHEAT_PACK_FRESH_MS);
}

static inline void preheatRuntimeTick() {
  const uint32_t now = (uint32_t)millis();

  // The engine follows its own flag only: preheatMenuEnabled is presentation,
  // and the tab may be hidden while the car keeps heating.
  if (!preheatEnabled) {
    preheatRuntimeCountSkip(PREHEAT_SKIP_DISABLED);
    return;
  }
  // A profile without a Vehicle/Body channel cannot be asked at all; see the
  // eligibility cascade below, which reports that as UNSUPPORTED_PROFILE.
  if (!preheatProfileSupported()) {
    preheatRuntimeCountSkip(PREHEAT_SKIP_UNSUPPORTED);
    return;
  }

  // Auto-shutoff sits ABOVE the periodic gate: it is a state change, not a
  // transmission, so it must not wait for the next PREHEAT_PERIOD_MS slot.
  // The engine is stopped in RAM first and the NVS write is best-effort: if the
  // write failed, deferring the stop to it would leave the engine running and
  // retry this branch on every tick, hammering the flash. The next tick then
  // takes the disabled path, so the counter cannot run away.
  if (preheatTargetReachedNow(now)) {
    portENTER_CRITICAL(&preheatMux);
    if (preheatAutoOffCount < UINT32_MAX) preheatAutoOffCount++;
    preheatAutoOffReason = PREHEAT_AUTOOFF_TARGET;
    portEXIT_CRITICAL(&preheatMux);
    preheatEnabled = false;
    preheatRuntimeResetUnderTxBarrier();
    (void)preheatNvsWrite(PREHEAT_NVS_ENABLED_KEY, false);
    return;
  }

  uint32_t lastTxMs = 0;
  portENTER_CRITICAL(&preheatMux);
  lastTxMs = preheatLastTxMs;
  portEXIT_CRITICAL(&preheatMux);

  // Is the car putting heat in the pack right now? On the 0x082 car that is
  // either drive unit's stator; on the HW4 (0x347) car it is 0x3A1's heating
  // request, because its rear stator reads the same kW with and without a route -
  // see preheatHeatWitnessHotPure. One locked read, so the values describe the
  // same instant. This is the input to the stall clock below, and it answers for
  // the whole session - the panel does not care whether this tick transmitted.
  const uint8_t tripId = preheatTripId;
  bool hot = false;
  portENTER_CRITICAL(&preheatMux);
  hot = preheatHeatWitnessHotPure(tripId, preheatStatorHotLocked(now),
                                  preheatHpPowerValid,
                                  preheatAgeMs(now, preheatHpPowerMs),
                                  preheatHpPowerW);
  portEXIT_CRITICAL(&preheatMux);

  // The stall criterion of the flat revision, and it sits here - above the
  // periodic gate below - because switching the engine off is a state change, not
  // a transmission, so it must not wait for a slot. The clock starts at the first
  // frame that actually went out this session: the boot grace and the MCP warmup
  // can hold the engine back for longer than the window, and silence before the
  // first transmission says nothing about the payload.
  //
  // The stop is silent: the tab names the reason in the status chip and nothing
  // else, so GET /api/preheat/stats is where a stall is read. Nothing is
  // snapshotted here, because nothing on the screen counts the silence.
  if (lastTxMs != 0u) {
    // The reassertion's field verdict: a hot->cold edge while we transmit, and
    // whether a disagreeing car frame came within PREHEAT_DROP_NEAR_CAR_MS before it.
    portENTER_CRITICAL(&preheatMux);
    if (preheatWasHot && !hot) {
      if (preheatHeatDrops < UINT32_MAX) preheatHeatDrops++;
      if (preheatDropNearCarPure(now, preheatCarConflictMs) &&
          preheatHeatDropsNearCar < UINT32_MAX)
        preheatHeatDropsNearCar++;
    }
    preheatWasHot = hot;
    portEXIT_CRITICAL(&preheatMux);
    if (hot) {
      preheatStatorZeroSinceMs = now;
    } else if (preheatStatorZeroSinceMs == 0u) {
      preheatStatorZeroSinceMs = now;
    } else if (preheatStatorStallElapsedPure(now, preheatStatorZeroSinceMs)) {
      portENTER_CRITICAL(&preheatMux);
      preheatAutoOffReason = PREHEAT_AUTOOFF_STATOR_STALL;
      portEXIT_CRITICAL(&preheatMux);
      // Same order as the target auto-off: stop in RAM first, write NVS
      // best-effort, and let the next tick take the disabled path if the write
      // failed rather than retrying against the flash on every tick.
      preheatEnabled = false;
      preheatRuntimeResetUnderTxBarrier();
      (void)preheatNvsWrite(PREHEAT_NVS_ENABLED_KEY, false);
      return;
    }
  }

  const bool bootDelayPassed =
      (uint32_t)(now - canInitTime) >= PREHEAT_BOOT_DELAY_MS;
  const bool canSeen = mcpRxCount > PREHEAT_MCP_WARMUP_FRAMES;
  // Liveness comes from the supervisor's own per-bus freshness mask rather than
  // from the 0x082's own age: that frame is bursty, the bus is not. The profile
  // gate above already established which channel carries Vehicle/Body, so exactly
  // one of the two selectors is true here.
  const uint8_t requiredBus =
      activeProfileVehicleBusOnCanA() ? (uint8_t)SUMMON_BUS_A
                                      : (uint8_t)SUMMON_BUS_B;
  const bool busFresh =
      preheatBusFreshPure(canPhysicalFreshMaskNow(now), requiredBus);
  // Self-echo is rejected on the RX side (preheatRuntimeObserve) so that our own
  // traffic cannot pollute the src* witness fields; it is never a tick-time reason.
  // The payload is a constant and needs no source frame, so the age of the car's
  // own trip frame is not a gate - but WHICH ID it uses is: until the car has sent
  // one, the engine does not know whether to speak 0x082 or 0x347.
  const uint8_t reason = preheatEligibilityReasonPure(
      true, true, bootDelayPassed, canSeen, busFresh,
      tripId != PREHEAT_TRIP_UNKNOWN);

  // The cascade is evaluated on EVERY tick and published, not only on the ticks
  // the periodic slot lets through. A value that refreshed once per
  // PREHEAT_PERIOD_MS would lag the state it describes by up to 100 ms, and the
  // strip reads it to tell a stalled engine from a transmitting one - the one
  // distinction the field report could not make.
  portENTER_CRITICAL(&preheatMux);
  preheatEvalReason = reason;
  preheatLastEvalMs = now;
  portEXIT_CRITICAL(&preheatMux);

  // THE ANSWER: a car frame that disagreed with ours armed it on the RX side, and
  // it goes out here, ahead of the periodic slot, so the car's statement is the
  // last word on the bus for about one service pass instead of up to
  // PREHEAT_PERIOD_MS. It obeys the same cascade as the periodic frame - a gated
  // engine consumes the arm instead of transmitting - and it restamps
  // preheatLastTxMs, so the periodic slot is measured from it.
  bool replyArmed = false;
  uint32_t lastReplyMs = 0;
  portENTER_CRITICAL(&preheatMux);
  replyArmed = preheatReplyArmed;
  lastReplyMs = preheatLastReplyMs;
  portEXIT_CRITICAL(&preheatMux);
  if (replyArmed) {
    if (reason != PREHEAT_SKIP_NONE) {
      portENTER_CRITICAL(&preheatMux);
      preheatReplyArmed = false;
      portEXIT_CRITICAL(&preheatMux);
    } else if (preheatReplyDuePure(replyArmed, now, lastReplyMs)) {
      // The breaker has the last word: more answers per second than the car could
      // ever ask for means a loop, and then answers stay off for the session.
      bool allowed = false;
      portENTER_CRITICAL(&preheatMux);
      allowed = preheatReplyBreakerAllowPure(preheatReplyBreaker, now);
      if (!allowed) preheatReplyArmed = false;
      portEXIT_CRITICAL(&preheatMux);
      if (allowed) {
        preheatRuntimeTransmit(now, true);
        return;
      }
    }
  }

  // One cadence, one shape: the constant frame goes out every PREHEAT_PERIOD_MS
  // while the engine is armed and no gate above held it back. The evaluation above
  // is published on every tick; the skip counting below runs per opportunity, not
  // per tick, so the counters stay comparable across builds.
  if (!preheatDuePure(now, lastTxMs, PREHEAT_PERIOD_MS)) return;

  if (reason != PREHEAT_SKIP_NONE) {
    preheatRuntimeCountSkip(reason);
    return;
  }

  preheatRuntimeTransmit(now);
}

// ---------------------------------------------------------------------------
// Controller recovery. The bus session is over, so the witnesses that describe it
// are cleared and the engine re-arms its clock. Lifetime counters are preserved.
// ---------------------------------------------------------------------------
// Clears the bus session, not the car: preheatEnergyCache survives, because it
// describes the car's own route and is what keeps the injected command
// self-consistent across a CAN restart.
static inline void preheatRuntimeResetUnderTxBarrier() {
  portENTER_CRITICAL(&preheatMux);
  preheatSourceValid = false;
  preheatSourceMs = 0;
  preheatLastTxValid = false;
  preheatLastTxMs = 0;
  preheatLastSkipReason = PREHEAT_SKIP_NONE;
  // The stall clock runs from the first frame of the session, so a bus restart
  // stops it rather than letting a stale start count against a fresh bus.
  preheatStatorZeroSinceMs = 0;
  // An answer armed on the old bus must not fire on the new one, and the heat edge
  // detector restarts with the transmissions.
  preheatReplyArmed = false;
  preheatWasHot = false;
  preheatReplyBreaker = {};
  portEXIT_CRITICAL(&preheatMux);
}

static inline void preheatRuntimeClearStats() {
  portENTER_CRITICAL(&preheatMux);
  preheatTxOk = 0;
  preheatTxFail = 0;
  preheatEchoCount = 0;
  preheatLatUs = 0;
  preheatMaxTxGapMs = 0;
  preheatSourceSkipped = 0;
  preheatAutoOffCount = 0;
  // lastTxRaw and preheatAutoOffReason are deliberately kept: they describe the
  // frame currently on the bus and the last state change rather than a count.
  preheatLastReason = MCP_TX_OK;
  for (uint8_t i = 0; i < PREHEAT_SKIP_COUNT; i++) preheatSkipCount[i] = 0;
  portEXIT_CRITICAL(&preheatMux);
  // The stall clock and the auto-off reason belong to a session, so a stats wipe
  // clears them with the counters above rather than leaving the previous run's
  // verdict on the chip.
  preheatSessionRestart();
}

static inline uint32_t preheatRuntimeSkipCount(uint8_t reason) {
  if (reason >= PREHEAT_SKIP_COUNT) return 0;
  uint32_t value = 0;
  portENTER_CRITICAL(&preheatMux);
  value = preheatSkipCount[reason];
  portEXIT_CRITICAL(&preheatMux);
  return value;
}



