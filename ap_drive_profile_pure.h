#pragma once
#include <stdint.h>

// Production AP Drive Profile regen targets on UI_powertrainControl 0x334
// byte[2]. Vehicle/topology availability is controlled by the active profile.
// The selector exposes STANDARD=20, REDUCED=10 and MINIMAL=1.
static constexpr uint8_t AP_DRIVE_REGEN_STANDARD_RAW = 20;
static constexpr uint8_t AP_DRIVE_REGEN_REDUCED_RAW = 10;
static constexpr uint8_t AP_DRIVE_REGEN_MINIMAL_RAW = 1;

static inline bool apDriveRegenRawSelectablePure(uint8_t raw) {
  return raw == AP_DRIVE_REGEN_STANDARD_RAW ||
         raw == AP_DRIVE_REGEN_REDUCED_RAW ||
         raw == AP_DRIVE_REGEN_MINIMAL_RAW;
}

static inline const char *apDriveRegenNamePure(uint8_t raw) {
  switch (raw) {
    case AP_DRIVE_REGEN_STANDARD_RAW: return "STANDARD";
    case AP_DRIVE_REGEN_REDUCED_RAW: return "REDUCED";
    case AP_DRIVE_REGEN_MINIMAL_RAW: return "MINIMAL";
    default: return "UNKNOWN";
  }
}
