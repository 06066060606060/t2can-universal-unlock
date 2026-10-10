#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct CanUsbFrame {
  uint64_t timestampUs;
  uint32_t session;
  uint64_t sequence;
  uint32_t id;
  uint8_t bus;
  uint8_t flags;
  uint8_t dlc;
  uint8_t data[8];
};

static inline size_t canUsbFormatFrame(char *out, size_t capacity, const CanUsbFrame &f) {
  if (!out || f.bus > 1 || f.flags > 3 || f.dlc > 8 ||
      f.id > ((f.flags & 1) ? 0x1FFFFFFFUL : 0x7FFUL)) return 0;
  char data[17] = {};
  static const char hex[] = "0123456789ABCDEF";
  if (!(f.flags & 2)) {
    for (uint8_t i = 0; i < f.dlc; ++i) {
      data[i * 2] = hex[f.data[i] >> 4];
      data[i * 2 + 1] = hex[f.data[i] & 15];
    }
  }
  const int n = snprintf(out, capacity, "@CAN,%lu,%llu,%llu,%c,%u,%lX,%u,%s\n",
      (unsigned long)f.session, (unsigned long long)f.sequence,
      (unsigned long long)f.timestampUs, f.bus ? 'B' : 'A',
      (unsigned)f.flags, (unsigned long)f.id, (unsigned)f.dlc, data);
  return n > 0 && (size_t)n < capacity ? (size_t)n : 0;
}

template <size_t Capacity> struct CanUsbQueue {
  CanUsbFrame frames[Capacity] = {};
  size_t head = 0, count = 0;
  uint32_t session = 0;
  uint64_t seen = 0, queued = 0, dropped = 0;
  bool active = false;
  void start() {
    if (++session == 0) ++session;
    head = count = 0; seen = queued = dropped = 0; active = true;
  }
  bool observe(CanUsbFrame frame) {
    if (!active) return false;
    frame.session = session;
    frame.sequence = ++seen;
    if (count == Capacity) { ++dropped; return false; }
    frames[(head + count) % Capacity] = frame;
    ++count; ++queued; return true;
  }
  bool pop(CanUsbFrame &frame) {
    if (!count) return false;
    frame = frames[head]; head = (head + 1) % Capacity; --count; return true;
  }
};
