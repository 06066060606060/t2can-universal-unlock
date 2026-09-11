#pragma once
#include <stdint.h>

enum VehicleProfileId : uint8_t {
  VEHICLE_PROFILE_NONE = 0,
  VEHICLE_MODEL_YL = 1,
  VEHICLE_MODEL_Y_JUNIPER = 2,
  VEHICLE_MODEL_Y_LEGACY = 3,
  VEHICLE_MODEL_3_HIGHLAND = 4,
  VEHICLE_MODEL_3_LEGACY = 5
};

enum VehicleCanTopology : uint8_t {
  VEHICLE_TOPOLOGY_NONE = 0,
  VEHICLE_TOPOLOGY_YL_PARTY_VH = 1,
  VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS = 2,
  VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS = 3
};

enum TurnSignalVariant : uint8_t {
  TURN_SIGNAL_UNSET = 0,
  TURN_SIGNAL_STALK = 1,
  TURN_SIGNAL_STALKLESS = 2
};

struct VehicleProfileSpec {
  VehicleProfileId id;
  const char *name;
  const char *shortName;
  VehicleCanTopology defaultTopology;
  TurnSignalVariant defaultTurn;
  bool tlsscRestoreSupported;
};

static inline const VehicleProfileSpec *vehicleProfileSpec(uint8_t id) {
  static const VehicleProfileSpec specs[] = {
    {VEHICLE_MODEL_YL, "Model Y L", "YL", VEHICLE_TOPOLOGY_YL_PARTY_VH, TURN_SIGNAL_STALK, false},
    {VEHICLE_MODEL_Y_JUNIPER, "Model Y Juniper", "YJ", VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS, TURN_SIGNAL_STALK, true},
    {VEHICLE_MODEL_Y_LEGACY, "Model Y Legacy", "Y", VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS, TURN_SIGNAL_STALK, true},
    {VEHICLE_MODEL_3_HIGHLAND, "Model 3 Highland", "3H", VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS, TURN_SIGNAL_UNSET, true},
    {VEHICLE_MODEL_3_LEGACY, "Model 3 Legacy", "3", VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS, TURN_SIGNAL_STALK, true}
  };
  for (const auto &s : specs) if ((uint8_t)s.id == id) return &s;
  return nullptr;
}

static inline bool vehicleProfileValid(uint8_t id) {
  return vehicleProfileSpec(id) != nullptr;
}

static inline VehicleCanTopology vehicleProfileDefaultTopology(uint8_t id) {
  const VehicleProfileSpec *s = vehicleProfileSpec(id);
  return s ? s->defaultTopology : VEHICLE_TOPOLOGY_NONE;
}

static inline bool vehicleProfileTopologyValid(uint8_t id, uint8_t topology) {
  if (!vehicleProfileValid(id)) return false;
  if (id == VEHICLE_MODEL_YL)
    return topology == VEHICLE_TOPOLOGY_YL_PARTY_VH;
  return topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS ||
         topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS;
}

// Compatibility helper: this is the legacy/default topology for a model, not
// the selected runtime topology. Runtime code should use activeVehicleTopology.
static inline VehicleCanTopology vehicleProfileTopology(uint8_t id) {
  return vehicleProfileDefaultTopology(id);
}

static inline TurnSignalVariant vehicleProfileDefaultTurn(uint8_t id) {
  const VehicleProfileSpec *s = vehicleProfileSpec(id);
  return s ? s->defaultTurn : TURN_SIGNAL_UNSET;
}

static inline bool vehicleProfileCanAIsParty(uint8_t id, uint8_t topology) {
  return vehicleProfileTopologyValid(id, topology) &&
         (topology == VEHICLE_TOPOLOGY_YL_PARTY_VH ||
          topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS);
}

static inline bool vehicleProfileCanAIsBody(uint8_t id, uint8_t topology) {
  return vehicleProfileTopologyValid(id, topology) &&
         topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
}

static inline bool vehicleProfileCanBIsChassis(uint8_t id, uint8_t topology) {
  return vehicleProfileTopologyValid(id, topology) &&
         (topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS ||
          topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS);
}

static inline bool vehicleProfileNagSupported(uint8_t id, uint8_t topology) {
  return vehicleProfileCanAIsParty(id, topology);
}

static inline bool vehicleProfileAdvancedEapSupported(uint8_t id, uint8_t topology) {
  if (!vehicleProfileTopologyValid(id, topology)) return false;
  // YL keeps its existing Party+VH split-bus implementation. Standard 3/Y
  // requires Body CAN on CAN A for Advanced EAP/Auto Blinker.
  return id == VEHICLE_MODEL_YL || topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
}

static inline bool vehicleProfileEuUnlockSupported(uint8_t id, uint8_t topology) {
  // Current Universal v3.2 hotfix EU Unlock/Summon-R79 path is available on the
  // supported YL Party+VH layout and on both Standard 3/Y Chassis layouts.
  return vehicleProfileTopologyValid(id, topology);
}

static inline bool vehicleProfileBodyControlsSupported(uint8_t id, uint8_t topology) {
  return vehicleProfileTopologyValid(id, topology) &&
         topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
}

// 0x334 UI_powertrainControl / UI_pedalMap routing. Model Y L exposes the
// validated frame on VH CAN (CAN B). Standard Model 3/Y exposes the same
// pedal-map field on Vehicle/Body CAN, so it is available only when CAN A is
// wired as Body. Party+Chassis intentionally has no Body 0x334 path.
static inline bool vehicleProfilePedalMapSupported(uint8_t id, uint8_t topology) {
  if (!vehicleProfileTopologyValid(id, topology)) return false;
  if (id == VEHICLE_MODEL_YL) return topology == VEHICLE_TOPOLOGY_YL_PARTY_VH;
  return topology == VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS;
}



// On Standard Party+Chassis, NAG injection is on CAN A but the AP gate is
// supplied by Chassis CAN B (0x399/921). A CAN B recovery therefore must
// invalidate the latched NAG AP authorization before TX can resume.
static inline bool vehicleProfileNagGateDependsOnCanB(uint8_t id, uint8_t topology) {
  return vehicleProfileNagSupported(id, topology) &&
         vehicleProfileCanBIsChassis(id, topology);
}

static inline bool vehicleProfileTlsscRestoreSupported(uint8_t id) {
  const VehicleProfileSpec *s = vehicleProfileSpec(id);
  return s && s->tlsscRestoreSupported;
}

// Banned Car is intentionally hidden/blocked on Model Y L in v3.2 hotfix.
// Keep this separate from TLSSC Restore capability so it can be re-enabled
// deliberately later without weakening the current backend guard.
static inline bool vehicleProfileBannedCarSupported(uint8_t id) {
  return id != VEHICLE_MODEL_YL && vehicleProfileTlsscRestoreSupported(id);
}

static inline bool vehicleProfileTurnValid(uint8_t id, uint8_t topology, uint8_t turn) {
  if (!vehicleProfileTopologyValid(id, topology)) return false;
  if (topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS)
    return turn == TURN_SIGNAL_UNSET;
  if (id == VEHICLE_MODEL_3_HIGHLAND)
    return turn == TURN_SIGNAL_STALK || turn == TURN_SIGNAL_STALKLESS;
  return turn == TURN_SIGNAL_STALK;
}

static inline TurnSignalVariant vehicleProfileResolvedTurn(uint8_t id, uint8_t topology, uint8_t storedTurn) {
  if (!vehicleProfileTopologyValid(id, topology)) return TURN_SIGNAL_UNSET;
  if (topology == VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS) return TURN_SIGNAL_UNSET;
  if (id == VEHICLE_MODEL_3_HIGHLAND)
    return vehicleProfileTurnValid(id, topology, storedTurn) ? (TurnSignalVariant)storedTurn : TURN_SIGNAL_UNSET;
  return TURN_SIGNAL_STALK;
}

static inline const char *vehicleProfileName(uint8_t id) {
  const VehicleProfileSpec *s = vehicleProfileSpec(id);
  return s ? s->name : "UNSET";
}

static inline const char *vehicleProfileShortName(uint8_t id) {
  const VehicleProfileSpec *s = vehicleProfileSpec(id);
  return s ? s->shortName : "--";
}

static inline const char *vehicleProfileTopologyName(uint8_t topology) {
  switch (topology) {
    case VEHICLE_TOPOLOGY_YL_PARTY_VH: return "PARTY + VH";
    case VEHICLE_TOPOLOGY_STANDARD_BODY_CHASSIS: return "BODY + CHASSIS";
    case VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS: return "PARTY + CHASSIS";
    default: return "DISABLED";
  }
}

static inline const char *vehicleProfileCanAName(uint8_t id, uint8_t topology) {
  if (!vehicleProfileTopologyValid(id, topology)) return "DISABLED";
  return vehicleProfileCanAIsParty(id, topology) ? "PARTY" : "BODY";
}

static inline const char *vehicleProfileCanBName(uint8_t id, uint8_t topology) {
  if (!vehicleProfileTopologyValid(id, topology)) return "DISABLED";
  return topology == VEHICLE_TOPOLOGY_YL_PARTY_VH ? "VH" : "CHASSIS";
}

static inline const char *turnSignalVariantName(uint8_t turn) {
  switch (turn) {
    case TURN_SIGNAL_STALK: return "STALK";
    case TURN_SIGNAL_STALKLESS: return "STALKLESS";
    default: return "N/A";
  }
}

#ifdef ARDUINO
static constexpr const char *VEHICLE_PROFILE_NAMESPACE = "v3profile";
static constexpr const char *VEHICLE_PROFILE_KEY = "profile";
static constexpr const char *VEHICLE_TOPOLOGY_KEY = "topology";
static constexpr const char *VEHICLE_TURN_KEY = "turn";
static constexpr const char *VEHICLE_UNIVERSAL_INIT_KEY = "univInit";
static constexpr const char *VEHICLE_MIGRATION_NOTICE_KEY = "migNotice";

static volatile uint8_t activeVehicleProfile = VEHICLE_PROFILE_NONE;
static volatile uint8_t activeVehicleTopology = VEHICLE_TOPOLOGY_NONE;
static volatile uint8_t activeTurnSignalVariant = TURN_SIGNAL_UNSET;
static volatile bool vehicleProfileSetupMode = true;
static volatile bool vehicleProfileMigrationNotice = false;
static volatile bool vehicleProfileNvsError = false;

static inline bool activeProfileIsYl() {
  return activeVehicleProfile == VEHICLE_MODEL_YL;
}

static inline bool activeCanAIsParty() {
  return vehicleProfileCanAIsParty(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeCanAIsBody() {
  return vehicleProfileCanAIsBody(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeCanBIsChassis() {
  return vehicleProfileCanBIsChassis(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfileIsStandard3Y() {
  return activeVehicleProfile != VEHICLE_MODEL_YL && vehicleProfileValid(activeVehicleProfile);
}

static inline bool activeProfileNagSupported() {
  return vehicleProfileNagSupported(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfileAdvancedEapSupported() {
  return vehicleProfileAdvancedEapSupported(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfileBodyControlsSupported() {
  return vehicleProfileBodyControlsSupported(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfilePedalMapSupported() {
  return vehicleProfilePedalMapSupported(activeVehicleProfile, activeVehicleTopology);
}



static inline bool activeProfileNagGateDependsOnCanB() {
  return vehicleProfileNagGateDependsOnCanB(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfileEuUnlockSupported() {
  return vehicleProfileEuUnlockSupported(activeVehicleProfile, activeVehicleTopology);
}

static inline bool activeProfileTlsscRestoreSupported() {
  return vehicleProfileTlsscRestoreSupported(activeVehicleProfile);
}

static inline bool activeProfileBannedCarSupported() {
  return vehicleProfileBannedCarSupported(activeVehicleProfile);
}

static inline const char *activeProfileCanAName() {
  return vehicleProfileCanAName(activeVehicleProfile, activeVehicleTopology);
}

static inline const char *activeProfileCanBName() {
  return vehicleProfileCanBName(activeVehicleProfile, activeVehicleTopology);
}

static bool vehicleProfileWriteBootstrapMarker(bool migrationNotice) {
  Preferences p;
  if (!p.begin(VEHICLE_PROFILE_NAMESPACE, false)) return false;
  const bool ok1 = p.putBool(VEHICLE_UNIVERSAL_INIT_KEY, true) > 0;
  const bool ok2 = p.putBool(VEHICLE_MIGRATION_NOTICE_KEY, migrationNotice) > 0;
  p.end();
  return ok1 && ok2;
}

static bool vehicleProfileLoadFromNvs() {
  Preferences p;
  if (!p.begin(VEHICLE_PROFILE_NAMESPACE, true)) return false;
  const uint8_t id = p.getUChar(VEHICLE_PROFILE_KEY, VEHICLE_PROFILE_NONE);
  const uint8_t topologyStored = p.getUChar(VEHICLE_TOPOLOGY_KEY, VEHICLE_TOPOLOGY_NONE);
  const uint8_t turnStored = p.getUChar(VEHICLE_TURN_KEY, TURN_SIGNAL_UNSET);
  const bool initialized = p.getBool(VEHICLE_UNIVERSAL_INIT_KEY, false);
  const bool notice = p.getBool(VEHICLE_MIGRATION_NOTICE_KEY, false);
  p.end();

  vehicleProfileMigrationNotice = notice;
  if (!initialized || !vehicleProfileValid(id)) return false;

  // v3.0 compatibility: previously configured profiles have no topology key.
  // Resolve those to the exact topology v3.0 used before this feature.
  const uint8_t topology = topologyStored == VEHICLE_TOPOLOGY_NONE
                         ? (uint8_t)vehicleProfileDefaultTopology(id)
                         : topologyStored;
  if (!vehicleProfileTopologyValid(id, topology)) return false;

  const TurnSignalVariant resolved = vehicleProfileResolvedTurn(id, topology, turnStored);
  if (!vehicleProfileTurnValid(id, topology, resolved)) return false;

  activeVehicleProfile = id;
  activeVehicleTopology = topology;
  activeTurnSignalVariant = resolved;
  vehicleProfileSetupMode = false;
  return true;
}

static bool vehicleProfileSave(uint8_t id, uint8_t topology, uint8_t requestedTurn) {
  if (!vehicleProfileTopologyValid(id, topology)) return false;
  const TurnSignalVariant turn = vehicleProfileResolvedTurn(id, topology, requestedTurn);
  if (!vehicleProfileTurnValid(id, topology, turn)) return false;

  Preferences p;
  if (!p.begin(VEHICLE_PROFILE_NAMESPACE, false)) return false;
  const bool ok1 = p.putBool(VEHICLE_UNIVERSAL_INIT_KEY, true) > 0;
  const bool ok2 = p.putUChar(VEHICLE_PROFILE_KEY, id) > 0;
  const bool ok3 = p.putUChar(VEHICLE_TOPOLOGY_KEY, topology) > 0;
  const bool ok4 = p.putUChar(VEHICLE_TURN_KEY, (uint8_t)turn) > 0;
  const bool ok5 = p.putBool(VEHICLE_MIGRATION_NOTICE_KEY, false) > 0;
  p.end();
  return ok1 && ok2 && ok3 && ok4 && ok5;
}

static bool vehicleProfileClearOnly() {
  Preferences p;
  if (!p.begin(VEHICLE_PROFILE_NAMESPACE, false)) return false;
  const bool ok1 = p.remove(VEHICLE_PROFILE_KEY);
  const bool ok2 = p.remove(VEHICLE_TOPOLOGY_KEY);
  const bool ok3 = p.remove(VEHICLE_TURN_KEY);
  p.end();
  activeVehicleProfile = VEHICLE_PROFILE_NONE;
  activeVehicleTopology = VEHICLE_TOPOLOGY_NONE;
  activeTurnSignalVariant = TURN_SIGNAL_UNSET;
  vehicleProfileSetupMode = true;
  return ok1 || ok2 || ok3;
}
#endif
