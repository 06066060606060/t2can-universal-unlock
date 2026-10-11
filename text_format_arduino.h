#pragma once
#include "fixed_point_pure.h"
#include "text_format_pure.h"

// fixed point arduino
static inline String fixedPointString(int32_t scaled, uint8_t decimals) {
  char buf[24] = {};
  if (!formatFixedPure(scaled, decimals, buf, sizeof(buf))) return String("0");
  return String(buf);
}

static inline String centiString(int32_t centi) {
  return fixedPointString(centi, 2u);
}

// text format arduino
static inline String escapedTextArduino(const char *src, size_t len, JsonTextPolicy policy) {
  String out;
  out.reserve(len + 8);
  appendJsonEscapedPure(out, src, len, policy);
  return out;
}
static inline bool httpBoolValue(const String &value, HttpBoolPolicy policy) {
  bool result = false;
  parseHttpBoolPure(value.c_str(), value.length(), result, policy);
  return result;
}

static inline String canFrameHexArduino(const uint8_t *data, uint8_t dlc) {
  char buffer[24];
  formatHexBytesPure(data, dlc > 8u ? 8u : dlc, buffer, sizeof(buffer));
  return String(buffer);
}
