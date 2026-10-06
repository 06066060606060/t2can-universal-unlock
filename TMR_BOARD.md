# TMR board port

TMR is an ESP32-S3 board with three external MCP2515 / XL2515 CAN controllers.
There is no ESP32 internal CAN controller controller in this port.

## CAN hardware

| Connector | Logical bus | Controller | CS | STBY |
|---|---|---|---:|---:|
| J2 | BODY | MCP2515 / XL2515 | GPIO10 | GPIO18 |
| J3 | CHASSIS | MCP2515 / XL2515 | GPIO9 | GPIO14 |
| J4 | PARTY | MCP2515 / XL2515 | GPIO8 | GPIO21 |

Shared SPI:

- SCK: GPIO12
- MOSI: GPIO11
- MISO: GPIO13
- Bitrate: 500 kbit/s on all three buses
- MCP2515 crystal: 16 MHz on all three controllers

## GPIO9

GPIO9 is **J3 CHASSIS chip-select only**.

It is never configured as an MCU reset output and is never used in a reset
sequence. MCP2515 controller recovery uses the MCP2515 SPI `RESET` command.

## Vehicle topology

Every vehicle profile uses:

`VEHICLE_TOPOLOGY_PARTY_BODY_CHASSIS`

Physical mapping is fixed:

- J2 = BODY
- J3 = CHASSIS
- J4 = PARTY

`VH` is an alias of `BODY` for every profile. Therefore legacy VH TX paths
are routed to J2 BODY rather than to J3 CHASSIS.

## Runtime architecture

- `canTaskMcp` services J2 BODY and J4 PARTY.
- `canTaskChassis` services J3 CHASSIS.
- All three MCP2515 controllers share one SPI peripheral and have independent
  CS lines.
- Each MCP2515 is configured for `CAN_500KBPS` with `MCP_16MHZ`.
- STBY is LOW during normal operation and HIGH during maintenance/recovery
  shutdown.

The TMR source contains no ESP32 CAN controller driver dependency.
