#pragma once

#include <cstddef>
#include <cstdint>

static inline uint32_t fixedPow10Pure(uint8_t decimals) {
  static constexpr uint32_t kPow10[] = {
      1u, 10u, 100u, 1000u, 10000u, 100000u, 1000000u};
  return decimals < (sizeof(kPow10) / sizeof(kPow10[0])) ? kPow10[decimals] : 0u;
}

// Formats an integer that is already scaled by 10^decimals. The result always
// contains exactly `decimals` fractional digits. No floating-point code is used.
static inline bool formatFixedPure(int32_t scaled, uint8_t decimals,
                                   char *out, size_t outLen) {
  if (!out || outLen == 0u) return false;
  const uint32_t scale = fixedPow10Pure(decimals);
  if (scale == 0u) return false;

  const bool negative = scaled < 0;
  const uint32_t mag = negative
      ? (uint32_t)(-(int64_t)scaled)
      : (uint32_t)scaled;
  const uint32_t whole = mag / scale;
  uint32_t frac = mag % scale;

  char tmp[32];
  size_t pos = 0u;
  if (negative) tmp[pos++] = '-';

  char digits[11];
  size_t digitCount = 0u;
  uint32_t value = whole;
  do {
    digits[digitCount++] = (char)('0' + (value % 10u));
    value /= 10u;
  } while (value != 0u && digitCount < sizeof(digits));
  while (digitCount != 0u) tmp[pos++] = digits[--digitCount];

  if (decimals != 0u) {
    tmp[pos++] = '.';
    uint32_t divisor = scale / 10u;
    for (uint8_t i = 0u; i < decimals; ++i) {
      tmp[pos++] = (char)('0' + (frac / divisor) % 10u);
      if (divisor > 1u) divisor /= 10u;
    }
  }

  if (pos + 1u > outLen) return false;
  for (size_t i = 0; i < pos; ++i) out[i] = tmp[i];
  out[pos] = '\0';
  return true;
}

static inline bool formatCentiPure(int32_t centi, char *out, size_t outLen) {
  return formatFixedPure(centi, 2u, out, outLen);
}

// Parses a non-negative decimal and rounds to the nearest 0.01. This mirrors
// the former strtod(value)*100 + 0.5 behavior for valid UI inputs without
// pulling floating-point parsing into the firmware image.
static inline bool parseUnsignedCentiPure(const char *text, uint16_t &out,
                                          uint16_t maxCenti) {
  if (!text || !*text) return false;
  const char *p = text;
  if (*p == '+') ++p;
  if (!*p || *p == '-') return false;

  uint64_t mantissa = 0u;
  uint8_t fracDigits = 0u;
  bool seenDigit = false;
  bool seenDot = false;
  while (*p && *p != 'e' && *p != 'E') {
    const char c = *p++;
    if (c == '.') {
      if (seenDot) return false;
      seenDot = true;
      continue;
    }
    if (c < '0' || c > '9') return false;
    seenDigit = true;
    const uint8_t d = (uint8_t)(c - '0');
    if (mantissa > 1000000000000000000ULL) return false;
    mantissa = mantissa * 10u + d;
    if (seenDot) {
      if (fracDigits == 18u) return false;
      ++fracDigits;
    }
  }
  if (!seenDigit) return false;

  int exp10 = 0;
  if (*p == 'e' || *p == 'E') {
    ++p;
    bool expNegative = false;
    if (*p == '+' || *p == '-') {
      expNegative = (*p == '-');
      ++p;
    }
    if (!*p) return false;
    int expValue = 0;
    bool expDigit = false;
    while (*p) {
      const char c = *p++;
      if (c < '0' || c > '9') return false;
      expDigit = true;
      expValue = expValue * 10 + (c - '0');
      if (expValue > 18) return false;
    }
    if (!expDigit) return false;
    exp10 = expNegative ? -expValue : expValue;
  }

  const int scaleExp = exp10 + 2 - (int)fracDigits;
  uint64_t centi = mantissa;
  if (scaleExp >= 0) {
    for (int i = 0; i < scaleExp; ++i) {
      if (centi > 0xFFFFFFFFULL / 10ULL) return false;
      centi *= 10ULL;
    }
  } else {
    uint64_t divisor = 1u;
    for (int i = 0; i < -scaleExp; ++i) divisor *= 10ULL;
    centi = (centi + divisor / 2ULL) / divisor;
  }

  if (centi > maxCenti || centi > 0xFFFFu) return false;
  out = (uint16_t)centi;
  return true;
}
