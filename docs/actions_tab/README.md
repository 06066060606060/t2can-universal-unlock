# Actions tab: battery preheat and Thermal view

Author: **3dfx**

The Actions tab lets the driver start battery preconditioning from the device, without
navigating to a Supercharger. It also shows a live redraw of the car's service-mode
Thermal screen. The feature is read-mostly. It transmits exactly one frame, and only
while the user has switched it on.

## What it does

- **Engine** (`preheat_pure.h`, `preheat_runtime.h`). While enabled, the device sends the
  car's own trip-planning "precondition" frame at 10 Hz on the ID that car's trip planning
  uses:
  - `0x082 UI_tripPlanning` on the HW3 cars tested (firmware 2026.20.3):
    `AB 50 C8 13 FF 03 8E 14`;
  - `0x347` on HW4 (LFP, rear motor only), which carries no `0x082`:
    `AF 24 9D 1D FF 03 F2 07`.

  The ID is latched from the car's own frames and nothing is sent until one is seen
  (`NO_TRIP_ID`). The frame has no counter and no checksum, so a constant payload is valid.
- **Immediate reassertion.** The car keeps sending its own "no preconditioning" trip frame
  about every 2 s. Every car frame that differs from ours is answered at once (storm guard
  20 ms), so the car's frame is the last word on the bus for milliseconds instead of up to
  100 ms. A breaker turns the answers off for the session after more than 5 in one second.
  Our own frame can never arm an answer.
- **Gates.** The engine waits for the boot delay (15 s), MCP warm-up, a live bus and a
  supported profile. It needs the Vehicle/Body bus: CAN A on `STANDARD_BODY_CHASSIS`,
  CAN B on `YL_PARTY_VH`. It is unavailable on Party+Chassis, gated like PedalMap
  (`vehicleProfilePreheatSupported`).
- **Auto-off.** The engine switches itself off in two cases:
  - the car's own pack average (`0x332` mux 0 `BMS_thermistorTAvg`) reaches the target
    (20–50 °C in 5 °C steps, default 40 °C, stored in the `preheat` NVS namespace);
  - both stators read 0 kW for 30 s after the first frame went out (`STATOR_STALL`).
- **S3XY action** `battery_preheat` (ID 13, appended) toggles the same engine through
  `togglePreheatEnabledFromButton()`.
- **Settings row** `actionsMenuToggle` only shows or hides the tab. It never stops a
  running preheat. Both reset paths clear the `preheat` namespace.

## The tab

- **Strip:** PACK TEMP alternates every 3 s between the pack average and the min–max
  range. HEAT POWER is the drive units' measured stator heat, front plus rear
  (`0x2E5` / `0x266`). COOLANT shows the battery loop flow. STATOR HEAT shows the
  front / rear pair.
- **HW4:** the strip shows HEAT REQ (the car's own `0x3A1` mux 1 heating target) instead
  of the stator sum, because the rear unit of that car reports about 1.4 kW even with no
  route.
- **BMS Heating State row** (`0x2E1` mux 0 bit 63, `0x312` mux 0). It opens a fold of raw
  witness rows: the car's last trip frame, the transmitted frame, TX and reply counters,
  heat drops and the reply breaker.
- **Status chip:** it names the live gate (`CAN warmup`, `Bus silent`, `No trip frame`
  and so on) instead of claiming `Heating`, and names a self-stop. There is no alarm
  colour.

**Thermal fold.** An inline SVG redraw of Service Mode → Thermal: radiator and fan, LCC,
chiller, the coolant valve, both pumps, loop inlets, the powertrain blocks
(PCS / DCDC / charger, front / rear drive units) and the battery-loop blocks (Autopilot,
infotainment, HV battery).

The pipes follow the coolant valve:

| Label | When | Drawing |
|---|---|---|
| Series Radiator Bypass | valve SERIES, heat-pump request SERIES_BYPASS | one loop, radiator bypassed |
| Ambient Source | valve AMBIENT_SOURCE | PT loop + separate battery loop through radiator and chiller |
| Series | valve SERIES, any other request (BLEND is drawn as Series) | one loop with the radiator |
| Parallel | valve PARALLEL | PT loop with radiator and LCC; battery loop with chiller |
| Off | both pumps at 0 rpm, or no valve frame | all pipes idle |

The valve is labelled "Octovalve" on a heat-pump car (`0x3A1` mux 1 bit 5) and
"Coolant valve" otherwise. Each value has a status dot. A value is fresh for 5 s from
10 Hz frames and for 15 s from slow frames. Animations stop under
`prefers-reduced-motion`.

| On the drawing | Frame, signal | Scale |
|---|---|---|
| Valve / heat-pump request | `0x201` index 1, 32\|2 / 51\|3 | enum |
| Battery / PT pump, radiator fan | `0x201` index 0, 3\|8 / 11\|8 / 19\|8 | x30 / x30 / x40 rpm |
| Battery / PT inlet | `0x321` 0\|10 / 10\|11 | x0.125 − 40 °C |
| Battery / PT flow | `0x241` 0\|9 / 22\|9 | x0.1 L/min |
| PCS DC-DC / charger | `0x2A4` 33\|11 / hottest of 0, 11, 22 \|11, signed | x0.1 + 40 °C |
| Front / Rear Drive Inverter | `0x376` / `0x315` index 0, 24\|8 `DI*_statorT` | −40 °C |
| Autopilot | `0x3FC` byte 0 (Chassis bus) | −128 °C |
| Infotainment | `0x353` byte 7, signed | +40 °C |
| Ambient, HV battery max / min | `0x321` 24\|8, `0x332` mux 0 | |

`0x201`, `0x315` and `0x376` are indexed: the low bits are an index the 2026.26 DBC
does not tag. `0x321` and `0x353` are read from either channel, with the same content
on both.

## HTTP API

| method | path | note |
|---|---|---|
| GET | `/api/preheat/stats` | polled every 1500 ms by the tab |
| POST | `/api/preheat/set?enabled=0\|1&targetC=20.0..50.0` | either or both; returns the same object; `400` otherwise |

Field names are the contract. Renaming one breaks the tab silently. The main fields:

| field | meaning |
|---|---|
| `supported`, `enabled`, `active` | toggle availability, state, engine running |
| `blocked`, `blockedReason` | the live gate: `BOOT_DELAY`, `MCP_WARMUP`, `BUS_SILENT`, `NO_TRIP_ID`, `DISABLED`, `UNSUPPORTED_PROFILE`, or `NONE` |
| `evalAgeMs` | age of the last gate evaluation (stale = the CAN service stopped) |
| `tripId` | `"0x082"`, `"0x347"` or `"unknown"` |
| `srcRaw`, `srcAgeMs`, `lastTxRaw` | the car's last trip frame and the frame sent, as hex (witnesses) |
| `txOk`, `txFail`, `lastReason`, `maxTxGapMs` | transmit counters |
| `carConflict`, `replyTx`, `replyFail`, `replyLatLastMs`, `replyLatMaxMs`, `replyBreaker` | immediate reassertion |
| `heatDrops`, `heatDropsNearCar` | stator heat dropping to 0 while enabled, and how many came within 3 s of a car frame |
| `packSeen`, `packTempMinC`, `packTempMaxC`, `packTempAvgC` | pack temperature (`0x332`) |
| `statorHeatFrontKw`, `statorHeatRearKw`, `statorHeatKw` | measured stator heat; the verdict for "does it heat" |
| `targetC`, `targetReached`, `autoOffCount`, `autoOffReason` | auto-off (`TARGET_REACHED`, `STATOR_STALL`) |
| `bmsSeen`, `bmsPreconditionAllowed`, `bmsHeatingWorthwhile`, `heatingActive` | BMS state |
| `th*` | Thermal values, each with a `th…AgeMs` of its source frame (`UINT32_MAX` = never seen) |

The profile/settings payload carries `preheatSupported`, which drives the nav button
and the S3XY action list.

## Tests

- `tests/test_preheat_pure.cpp`: decoders and policies against real frames. Build with
  `g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/test_preheat_pure.cpp`; a
  silent exit 0 means pass.
- `tests/test_preheat_*_static.py`: pins between firmware, JSON and tab, checked as text.
  Run each with `python`; a silent exit means pass.
- Existing suites touched: BLE action IDs, settings/NVS reset allowlists.

## Checking on a car

Captures are RX-only, so a transmitted frame never appears in a log. Judge a run by
`GET /api/preheat/stats` instead:

1. `blocked: false` and `tripId` set.
2. `txOk` rising by about 10 per second.
3. `statorHeatFrontKw` + `statorHeatRearKw` > 0, typically 2–7 kW.
4. `packTempAvgC` climbing towards `targetC`.

Park cold and switch the toggle on, with no navigation route set.
