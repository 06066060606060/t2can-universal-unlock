#pragma once
#include <stdint.h>

static constexpr uint8_t SCCM249_CKSUM_CTR[16] = {
  0x9B, 0xE8, 0x2A, 0xD3, 0xD3, 0x83, 0x4C, 0x5E,
  0x3F, 0x5E, 0xE2, 0x28, 0x3A, 0x13, 0xAF, 0xCE
};

static inline uint8_t sccm249Crc8Pure(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x2F) : (uint8_t)(crc << 1);
  }
  return crc;
}

static inline uint8_t leftStalkChecksumPure(const uint8_t frame[4], uint8_t counter) {
  const uint8_t crcInput[4] = {(uint8_t)(frame[1] & 0xF0), frame[2], frame[3], 0x00};
  return (uint8_t)(sccm249Crc8Pure(crcInput, 4) ^ SCCM249_CKSUM_CTR[counter & 0x0F]);
}


static inline uint8_t vcleftMuxPure(const uint8_t data[8]) { return data[0] & 0x03u; }
static inline uint8_t vcleftLeftButtonPure(const uint8_t data[8]) { return (data[3] >> 6) & 0x03u; }
static inline uint8_t vcleftRightButtonPure(const uint8_t data[8]) { return (data[5] >> 4) & 0x03u; }
static inline void vcleftSetLeftButtonPure(uint8_t data[8], uint8_t state) {
  data[3] = (uint8_t)((data[3] & 0x3Fu) | ((state & 0x03u) << 6));
}
static inline void vcleftSetRightButtonPure(uint8_t data[8], uint8_t state) {
  data[5] = (uint8_t)((data[5] & 0xCFu) | ((state & 0x03u) << 4));
}
static inline bool doorOpenButtonPressedPure(const uint8_t *data, uint8_t dlc) {
  return data && dlc >= 4 && ((data[3] & 0x80u) != 0);
}
