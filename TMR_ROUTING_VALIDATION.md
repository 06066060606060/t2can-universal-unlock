# TMR CAN routing validation

## Fixed routing

- J2 BODY / CAN A: GPIO CS10, STBY18
- J3 CHASSIS / CAN B: GPIO CS9, STBY14
- J4 PARTY / CAN C: GPIO CS8, STBY21
- Shared SPI: SCK12 / MOSI11 / MISO13
- 500 kbit/s, MCP2515 16 MHz

## NAG Killer

- NAG torque target `0x370` is received on J4 PARTY / CAN C.
- NAG torque modified `0x370` is transmitted on J4 PARTY / CAN C.
- DAS/AP/hands-on gate `0x399` is received on J3 CHASSIS / CAN B.
- `0x399` is decoded before the PARTY `0x370` NAG eligibility decision.
- TSL9-modified `0x399`, when applicable, is transmitted back on J3 CHASSIS / CAN B.
- TMR does not consume `0x399` from J2 BODY or J4 PARTY.
- Legacy BODY `0x39B` TSL9 compatibility is disabled on the fixed TMR topology.

## Auto Blinker / stalk

- `0x24A` `DAS_visualDebug.behaviorType` is read on J3 CHASSIS / CAN B.
- `0x249` stalk state/template is read on J2 BODY / CAN A.
- Auto Blinker `0x249` is transmitted on J2 BODY / CAN A.
- The legacy CAN-B `0x249` handler remains only as compatibility code and is not used by TMR BODY routing.


## TMR stalk routing correction

The TMR port does not consume 0x249 on CHASSIS/J3. The stalk state is read from BODY/J2 and Auto Blinker 0x249 is transmitted back on BODY/J2. CHASSIS/J3 supplies 0x24A planner/behavior state.
