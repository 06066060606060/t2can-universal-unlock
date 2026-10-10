#pragma once

#include <nvs.h>
#include <stddef.h>
#include <stdint.h>

// Missing keys retain legacy defaults; every other NVS error stays observable.
class CheckedPreferences {
 public:
  CheckedPreferences() = default;
  CheckedPreferences(const CheckedPreferences &) = delete;
  CheckedPreferences &operator=(const CheckedPreferences &) = delete;
  ~CheckedPreferences() { end(); }

  bool begin(const char *name, bool readOnly = true) {
    end();
    error_ = ESP_OK;
    missing_ = false;
    const esp_err_t result = nvs_open(name,
        readOnly ? NVS_READONLY : NVS_READWRITE, &handle_);
    if (readOnly && result == ESP_ERR_NVS_NOT_FOUND) {
      missing_ = true;
      return true;
    }
    opened_ = result == ESP_OK;
    if (!opened_) error_ = result;
    return opened_;
  }

  void end() {
    if (opened_) nvs_close(handle_);
    opened_ = false;
  }
  bool ok() const { return error_ == ESP_OK; }
  esp_err_t error() const { return error_; }
  bool exists() const { return opened_; }

  bool isKey(const char *key) {
    if (!readable()) return false;
    nvs_type_t type;
    return accepted(nvs_find_key(handle_, key, &type));
  }

  uint8_t getUChar(const char *key, uint8_t fallback = 0u) {
    uint8_t value = fallback;
    if (readable() && !accepted(nvs_get_u8(handle_, key, &value)))
      value = fallback;
    return value;
  }
  bool getBool(const char *key, bool fallback = false) {
    return getUChar(key, fallback ? 1u : 0u) != 0u;
  }
  uint16_t getUShort(const char *key, uint16_t fallback = 0u) {
    uint16_t value = fallback;
    if (readable() && !accepted(nvs_get_u16(handle_, key, &value)))
      value = fallback;
    return value;
  }
  uint32_t getUInt(const char *key, uint32_t fallback = 0u) {
    uint32_t value = fallback;
    if (readable() && !accepted(nvs_get_u32(handle_, key, &value)))
      value = fallback;
    return value;
  }
  size_t getBytesLength(const char *key) {
    size_t size = 0u;
    if (readable() && !accepted(nvs_get_blob(handle_, key, nullptr, &size)))
      size = 0u;
    return size;
  }
  size_t getBytes(const char *key, void *value, size_t capacity) {
    size_t size = capacity;
    if (!readable() || !accepted(nvs_get_blob(handle_, key, value, &size)))
      return 0u;
    return size;
  }

 private:
  bool readable() {
    if (opened_) return true;
    if (!missing_ && error_ == ESP_OK) error_ = ESP_FAIL;
    return false;
  }
  bool accepted(esp_err_t result) {
    if (result == ESP_OK) return true;
    if (result != ESP_ERR_NVS_NOT_FOUND && error_ == ESP_OK) error_ = result;
    return false;
  }
  nvs_handle_t handle_ = 0;
  esp_err_t error_ = ESP_OK;
  bool opened_ = false;
  bool missing_ = false;
};

// Absence is compatible with legacy keys. Present malformed records are errors.
static inline bool nvsCheckedReadBlob(const char *name, const char *key,
                                      void *value, size_t expectedSize,
                                      bool &found) {
  found = false;
  CheckedPreferences p;
  if (!p.begin(name, true)) return false;
  found = p.isKey(key);
  if (!p.ok()) return false;
  if (!found) return true;
  if (p.getBytesLength(key) != expectedSize || !p.ok()) return false;
  return p.getBytes(key, value, expectedSize) == expectedSize && p.ok();
}

static inline bool nvsCheckedWriteBlob(const char *name, const char *key,
                                       const void *value, size_t size) {
  nvs_handle_t handle;
  if (nvs_open(name, NVS_READWRITE, &handle) != ESP_OK) return false;
  const bool ok = nvs_set_blob(handle, key, value, size) == ESP_OK &&
      nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  return ok;
}
