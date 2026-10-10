# LAB Country / Map Region

This feature ports the separate v3.9.2 country experiment onto v3.9.3 and adds
KOREA plus Universal profile routing. STOCK is the default and disables the
overlay. US selects country 840 / US / map region 0. KOREA selects country
410 / KR / map region 7. The selection is an overlay, not an ECU configuration
write: stock frames are copied only when they arrive on an eligible route.

## Routing policy

| Selected topology | CAN A | CAN B |
| --- | --- | --- |
| Model YL Party + VH | Party: 0x7FF | VH: 0x238, 0x7FF |
| Standard Body + Chassis | Body: 0x238, 0x7FF | Chassis: 0x238, 0x7FF |
| Standard Party + Chassis | Party: 0x7FF | Chassis: 0x238, 0x7FF |

Standard covers Model Y Juniper, Model Y Legacy, Model 3 Highland and Model 3
Legacy. Invalid profile/topology combinations are rejected. Each overlay uses
the received frame's bus and ID; absent frames do not trigger synthetic traffic.

## Evidence and limits

- `research/can-logs/Model3CAN.dbc` defines 0x238 country as little-endian
  start bit 16, length 10; counter at 52, length 4; checksum at 56, length 8.
  Country 0 is UNKNOWN and 1023 is SNA.
- The same DBC defines 0x7FF page 1 country at bit 16, length 16, and page 3
  map region at bit 8, length 4, with declared region range 0 through 10.
- The 2026 Model YL BODY capture contains `B0 23 9A DD 1F 00 F6 99` for
  0x238, `01 A5 52 4B 62 A5 E8 01` for 0x7FF page 1, and
  `03 07 DC AD E1 20 30 64` for page 3. These ground KR=410, the KR byte
  encoding, region 7, and the additive 0x238 checksum in the earlier experiment.
- The 2024 Model Y decoded BODY and CHASSIS captures contain both messages;
  PARTY contains 0x7FF. Their France country/map-region values demonstrate that
  the original Korea-only source predicate does not cover all supplied vehicles.
- These captures and the existing profile topology define the software routing
  policy. They do not prove injected-frame acceptance on every model. In
  particular, Juniper and Highland acceptance has not been measured here.

## Verification scope

Host regressions exercise transformations, unrelated-bit preservation, checksum
and counter behavior, invalid input rejection, and profile/route decisions.
Dashboard and API tests cover the three selections. Controller flash, live CAN
acceptance and road behavior require separate validation. See `VALIDATION.md`
for evidence produced for this exact package.
