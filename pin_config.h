/* TMR: ESP32-S3 + three MCP2515 / XL2515 controllers. */
#pragma once

#define TMR_BOARD 1

// Shared SPI bus.
#define SPI_SCLK 12
#define SPI_MOSI 11
#define SPI_MISO 13

// Physical connector map. All buses use 500 kbit/s and MCP_16MHZ.
#define MCP2515_BODY_CS       10  // J2 BODY (legacy CAN A)
#define MCP2515_CHASSIS_CS     9  // J3 CHASSIS (legacy CAN B)
#define MCP2515_PARTY_CS       8  // J4 PARTY

// CAN-transceiver standby pins; LOW selects normal (non-standby) operation.
#define MCP2515_BODY_STBY     18
#define MCP2515_CHASSIS_STBY  14
#define MCP2515_PARTY_STBY    21

// Compatibility aliases for code that still identifies J2 as CAN A.
#define MCP2515_CS   MCP2515_BODY_CS
#define MCP2515_SCLK SPI_SCLK
#define MCP2515_MOSI SPI_MOSI
#define MCP2515_MISO SPI_MISO
