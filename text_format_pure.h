#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Preserve each existing endpoint's control-character policy.
enum class JsonTextPolicy { Wifi, Research, Ble };
template<class Sink>
static inline void appendJsonEscapedPure(Sink &out, const char *src, size_t len,
                                  JsonTextPolicy policy) {
  if (!src) return;
  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = (unsigned char)src[i];
    if (c == '\\' || c == '"') { out += '\\'; out += (char)c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') { if (policy != JsonTextPolicy::Ble) out += "\\r"; }
    else if (c == '\t' && policy != JsonTextPolicy::Ble) out += "\\t";
    else if (policy != JsonTextPolicy::Research || c >= 0x20) out += (char)c;
  }
}

// Like the former snprintf loop, a short buffer contains a terminated prefix.
static inline void formatHexBytesPure(const uint8_t *data, size_t len,
                               char *out, size_t capacity) {
  if (!out || !capacity) return;
  size_t used = 0;
  static const char digits[] = "0123456789ABCDEF";
  if (data) for (size_t i = 0; i < len; ++i) {
    const char token[3] = {i ? ' ' : digits[data[i] >> 4],
                          i ? digits[data[i] >> 4] : digits[data[i] & 15],
                          digits[data[i] & 15]};
    const size_t count = i ? 3 : 2;
    for (size_t k = 0; k < count; ++k) {
      if (used + 1 >= capacity) { out[used] = '\0'; return; }
      out[used++] = token[k];
    }
  }
  out[used] = '\0';
}

enum class HttpBoolPolicy { Strict01, StrictAliases, LegacyOne, LegacyOneTrue, LegacyAliases };
static inline bool parseHttpBoolPure(const char *value, size_t len, bool &out,
                              HttpBoolPolicy policy) {
  const auto equal = [value, len](const char *s) {
    return value && strlen(s) == len && memcmp(value, s, len) == 0;
  };
  const bool aliases = policy == HttpBoolPolicy::StrictAliases || policy == HttpBoolPolicy::LegacyAliases;
  const bool trueWord = aliases || policy == HttpBoolPolicy::LegacyOneTrue;
  if (equal("1") || (trueWord && equal("true")) || (aliases && equal("on"))) {
    out = true; return true;
  }
  if (policy == HttpBoolPolicy::Strict01 || policy == HttpBoolPolicy::StrictAliases) {
    if (!(equal("0") || (aliases && (equal("false") || equal("off"))))) return false;
  }
  out = false; return true;
}
