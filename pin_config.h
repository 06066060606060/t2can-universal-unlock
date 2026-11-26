/*
 * @Description: T-2CAN pin map
 * @Author: LILYGO_L
 * @Date: 2023-06-05 13:01:59
 * @LastEditTime: 2026-09-15
 */
#pragma once

#if defined(BOARD_WAVESHARE_S3_CAN_2CH)

// Waveshare ESP32-S3-CAN-2CH (onboard-antenna; -U version identical).
// CAN_1 terminal = ESP32-S3 TWAI through the isolator -> CAN B.
// CAN_2 terminal = XL2515QF20 (MCP2515-compatible, 16 MHz crystal) -> CAN A.
// Pins from Waveshare's schematic / BSP (waveshareteam/ESP32-S3-CAN-2CH
// HARDWARE_REFERENCE.md). The XL2515 has no reset pin: MCP2515_RST = -1 and
// every reset goes over SPI (Can_A.reset()). INT (GPIO 18) is unused — polled.

// CAN B / ESP32-S3 TWAI (CAN_1 terminal)
#define CAN_TX 15
#define CAN_RX 16

// Shared SPI pins
#define SPI_SCLK 21
#define SPI_MOSI 41
#define SPI_MISO 42

// CAN A / XL2515 (CAN_2 terminal)
#define MCP2515_CS 17
#define MCP2515_SCLK SPI_SCLK
#define MCP2515_MOSI SPI_MOSI
#define MCP2515_MISO SPI_MISO
#define MCP2515_RST -1

#else

// CAN B / ESP32-S3 TWAI
#define CAN_TX 7
#define CAN_RX 6

// Shared SPI pins
#define SPI_SCLK 12
#define SPI_MOSI 11
#define SPI_MISO 13

// CAN A / MCP2515
#define MCP2515_CS 10
#define MCP2515_SCLK SPI_SCLK
#define MCP2515_MOSI SPI_MOSI
#define MCP2515_MISO SPI_MISO
#define MCP2515_RST 9

#endif
