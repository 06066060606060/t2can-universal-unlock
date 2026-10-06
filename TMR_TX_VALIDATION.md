# TMR TX validation

## Physical buses
- CAN A = J2 BODY
- CAN B = J3 CHASSIS
- CAN C = J4 PARTY

## TX admission
- BODY TX (`Can_A`) requires `CAN_TX_FRESH_BODY` only.
- CHASSIS TX (`Can_B`) requires CHASSIS freshness when using the default chassis wrapper.
- PARTY TX (`Can_C`) requires `CAN_TX_FRESH_PARTY` only.
- Legacy `0x03` PARTY+CHASSIS freshness remains available only to explicit legacy masked callers.
- BODY freshness is now preserved by the recovery barrier (`CAN_TX_FRESH_ALL`).

## Important fixes
- Removed the hard-coded `freshMask == 0x03` requirement from the active BODY/PARTY TX paths.
- `mcpChassisTransmit()` now physically transmits through `Can_B` (J3 CHASSIS), not `Can_A`.
- TMR `0x249` remains BODY/J2 RX and TX.
- NAG `0x370` remains PARTY/J4 RX and TX.
- NAG `0x399` remains CHASSIS/J3 RX and TX when modified.
