/*
 * @Description: T-2CAN pin map
 * @Author: LILYGO_L
 * @Date: 2023-06-05 13:01:59
 * @LastEditTime: 2026-09-15
 */
#pragma once

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
