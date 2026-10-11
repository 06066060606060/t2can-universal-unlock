#pragma once
#include <stdint.h>
#include <stddef.h>

struct S3xyNotifyTokensPure {
  bool single;
  bool dbl;
  bool lng;
  bool ack;
};

// Recognize every established token anywhere in the payload. Duplicate tokens
// remain one match; multiple gesture kinds remain ambiguous at the caller.
static inline S3xyNotifyTokensPure s3xyParseNotifyTokensPure(const uint8_t *data, size_t len) {
  S3xyNotifyTokensPure tokens = {};
  if (!data || len < 2) return tokens;
  for (size_t i = 0; i + 1 < len; ++i) {
    switch (data[i]) {
      case 0xC1:
        tokens.single |= data[i + 1] == 0x01;
        tokens.dbl |= data[i + 1] == 0x02;
        break;
      case 0xC3:
        tokens.lng |= data[i + 1] == 0x01;
        break;
      case 0xC7:
        if (data[i + 1] == 0x00 && i + 2 < len)
          tokens.ack |= data[i + 2] == 0x01 || data[i + 2] == 0xC3;
        break;
      default:
        break;
    }
  }
  return tokens;
}
