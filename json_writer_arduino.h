#pragma once
#include "fixed_point_pure.h"

// Shared compact JSON emitter for dashboard/API status payloads.
//
// The firmware previously repeated String concatenation sequences in every
// status function. On ESP32-S3 that duplicated a significant amount of flash
// code. Keep these helpers out-of-line so the linker retains one formatting
// implementation shared by all API builders.

#if defined(__GNUC__)
#define T2CAN_JSON_NOINLINE __attribute__((noinline))
#else
#define T2CAN_JSON_NOINLINE
#endif

class JsonWriterArduino {
public:
  explicit JsonWriterArduino(String &out, bool resumeStartedObject = false)
      : out_(out), first_(!resumeStartedObject) {
    if (!resumeStartedObject) out_ += '{';
  }

  T2CAN_JSON_NOINLINE void boolean(const char *key, bool value) {
    prefix(key);
    out_ += value ? "true" : "false";
  }

  T2CAN_JSON_NOINLINE void u32(const char *key, uint32_t value) {
    prefix(key);
    out_ += (unsigned long)value;
  }

  T2CAN_JSON_NOINLINE void i32(const char *key, int32_t value) {
    prefix(key);
    out_ += (long)value;
  }


  T2CAN_JSON_NOINLINE void fixed(const char *key, int32_t scaled, uint8_t decimals) {
    char buf[32] = {};
    if (!formatFixedPure(scaled, decimals, buf, sizeof(buf))) {
      raw(key, "0");
      return;
    }
    prefix(key);
    out_ += buf;
  }

  T2CAN_JSON_NOINLINE void string(const char *key, const char *value) {
    prefix(key);
    out_ += '"';
    out_ += value ? value : "";
    out_ += '"';
  }

  T2CAN_JSON_NOINLINE void string(const char *key, const String &value) {
    prefix(key);
    out_ += '"';
    out_ += value;
    out_ += '"';
  }

  // Append an already-formatted JSON scalar/object/array without quotes.
  T2CAN_JSON_NOINLINE void raw(const char *key, const String &value) {
    prefix(key);
    out_ += value;
  }

  T2CAN_JSON_NOINLINE void raw(const char *key, const char *value) {
    prefix(key);
    out_ += value ? value : "null";
  }

  T2CAN_JSON_NOINLINE void beginObject(const char *key) {
    prefix(key);
    out_ += '{';
    // The first field inside a nested object must not start with a comma.
    first_ = true;
  }

  T2CAN_JSON_NOINLINE void endObject() {
    out_ += '}';
    // The nested container itself is already a field in its parent.
    first_ = false;
  }

  T2CAN_JSON_NOINLINE void beginArray(const char *key) {
    prefix(key);
    out_ += '[';
    first_ = true;
  }

  T2CAN_JSON_NOINLINE void endArray() {
    out_ += ']';
    first_ = false;
  }

  T2CAN_JSON_NOINLINE void finish() {
    out_ += '}';
  }

private:
  String &out_;
  bool first_;

  T2CAN_JSON_NOINLINE void prefix(const char *key) {
    if (!first_) out_ += ',';
    first_ = false;
    out_ += '"';
    out_ += key;
    out_ += "\":";
  }
};

#undef T2CAN_JSON_NOINLINE
