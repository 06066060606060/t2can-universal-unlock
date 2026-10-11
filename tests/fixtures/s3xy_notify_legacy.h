#pragma once
#include <stdint.h>
#include <stddef.h>

static int s3xyFindToken2(const uint8_t *data, size_t len, uint8_t a, uint8_t b) {
  if (!data || len < 2) return -1;
  for (size_t i = 0; i + 1 < len; i++) {
    if (data[i] == a && data[i + 1] == b) return (int)i;
  }
  return -1;
}

static int s3xyFindToken3(const uint8_t *data, size_t len, uint8_t a, uint8_t b, uint8_t c1, uint8_t c2) {
  if (!data || len < 3) return -1;
  for (size_t i = 0; i + 2 < len; i++) {
    if (data[i] == a && data[i + 1] == b && (data[i + 2] == c1 || data[i + 2] == c2)) return (int)i;
  }
  return -1;
}

