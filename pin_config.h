/*
 * TMR board pin map
 * ESP32-S3 + 3 x MCP2515 / XL2515
 *
 * J2 = BODY
 * J3 = CHASSIS
 * J4 = PARTY
 *
 * Shared SPI:
 *   SCK  = GPIO12
 *   MOSI = GPIO11
 *   MISO = GPIO13
 *
 * MCP2515:
 *   J2 BODY    CS=10  STBY=18
 *   J3 CHASSIS CS=9   STBY=14
 *   J4 PARTY   CS=8   STBY=21
 *
 * All controllers: 500 kbit/s, MCP_16MHZ.
 *
 * GPIO9 is a chip-select only on TMR. It is NEVER used as an MCU/MCP reset
 * output and is never toggled through a reset sequence.
 */
#pragma once

#define TMR_BOARD 1

// TMR status LED
#define TMR_STATUS_LED_GPIO 7

#define SPI_SCLK 12
#define SPI_MOSI 11
#define SPI_MISO 13

#define MCP2515_BODY_CS       10
#define MCP2515_CHASSIS_CS     9
#define MCP2515_PARTY_CS       8

#define MCP2515_BODY_STBY     18
#define MCP2515_CHASSIS_STBY  14
#define MCP2515_PARTY_STBY    21

// Legacy aliases: CAN A is now the physical BODY bus.
#define MCP2515_CS   MCP2515_BODY_CS
#define MCP2515_SCLK SPI_SCLK
#define MCP2515_MOSI SPI_MOSI
#define MCP2515_MISO SPI_MISO

// GPIO9 was the old T-2CAN reset pin. It is intentionally NOT defined as RST.
