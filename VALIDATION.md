# T2CAN Universal v3.7.2 — Validation

## LAB DMS/NAG bit43 — all supported profiles + NVS persistence

1. Capability is gated by `activeProfileDmsNagSupported()`, which follows the existing Universal `0x3FD` / R79 route. It is not tied to Model YL identity.
2. Current supported matrix: Model Y L on Party+VH; Model Y Juniper, Model Y Legacy, Model 3 Highland, and Model 3 Legacy on either Standard Body+Chassis or Standard Party+Chassis. Invalid topology/profile combinations remain unsupported.
3. The user selection is stored as `features/dmsNag43` in Preferences/NVS. Missing key defaults OFF. Reboot reloads the saved selection. LAB OFF immediately deactivates injection through the runtime gate without clearing the saved selection.
4. The bit overlay is unchanged: only `0x3FD mux1 bit43` is cleared, and it is composed into Fast Echo, periodic/retry, and ULC-snooze mux1 clone paths. Existing R79 bit18/19/47 behavior is unchanged.
5. Dashboard/API expose stock and last-effective bit43 values and use `SAVED` wording. Real-vehicle behavioral validation of NAG suppression is still required separately per vehicle generation/HW/software branch.
6. Verification results are updated after the local regression suite below; no ESP32-S3 target compile/link claim is made unless target tooling is present.

## Fresh v3.7.2 verification

- Blinker TX profile default under test: **YL = SINGLE TX; all non-YL supported 3/Y profiles = LEGACY 350 ms BURST**. LAB remains a volatile override and LAB OFF/reboot restores the profile default.

- Python/static regression suite: **67/67 PASS**.
- Host C++ pure suite: **34/34 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- HOME Blinker countdown regression: **8/8 PASS**.
- Dashboard JavaScript syntax: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Final dashboard is **390,662 bytes source / 381,213 bytes minified / 78,280 bytes gzip**, below the strict **84,000-byte** limit. `index_html.h` SHA-256: `e69ed8d6f9e167c2e61daa770eb8f6a809262d4aebc78f91540f751bc04fddee`.
- `index_html.h` minimal host compile with an Arduino/PROGMEM stub: **PASS**.
- `arduino-cli` and PlatformIO are not installed in this environment, so no ESP32-S3 target compile/link or `.bin` claim is made for this package.

# T2CAN Universal v3.7.1 — Validation

## P0 — 0x399 AP / Hands-On decode correction
1. DBC vector `03 06 FF 80 B0 44 60 78` decodes AP state **3** and Hands-On state **1** from independent fields.
2. Host vectors cover AP/Hands-On values `(5,3)`, `(14,1)`, and `(6,5)`. Raw **14** is not AP-active; raw **5** remains AP-active and NOA-active.
3. DLC below 6 produces an invalid decoder result without modifying selector outputs. Both production bus admission paths also reject 0x399 below DLC 6.
4. NAG defaults use AP byte 0 / shift 0 / mask 0x0F and Hands-On byte 5 / shift 2 / mask 0x0F through the existing general selector path.
5. Visual warning coverage confirms AP 3 + Hands-On 1 is inactive, entry into Hands-On 3 creates the warning edge, and 3 -> 4 -> 5 does not create a second edge.
6. Fresh host regression: **65/65 Python/static test files PASS**, **32/32 C++ pure test binaries PASS**, **8/8 HOME Blinker countdown cases PASS**, **5 source + 5 embedded JavaScript blocks PASS**, and generated `index_html.h` host compile PASS.
7. ESP32-S3 compile/link PASS with Arduino CLI 1.5.1, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1 and `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. Result: **1,510,067 bytes (48%)** program storage and **71,840 bytes (21%)** global RAM.

## Auto Blinker NOA session gate follow-up
1. NOA stabilization and Cancel Pause use separate pure state objects. NOA transitions through `INACTIVE -> STABILIZATION -> READY`; a valid non-NOA observation enters `EXIT WAIT`, and only **2,000 ms** of continuous non-NOA state closes the session.
2. Pure tests cover sub-2-second recovery from both STABILIZATION and READY, exact 2-second exit confirmation, a new timer after confirmed re-entry, `millis()` rollover, immediate invalid-state reset, and cancel-pause independence.
3. Dashboard timing changes replace only an in-progress stabilization deadline and only when the value actually changes. A READY session is not re-closed, and changing Cancel Pause does not touch the NOA session.
4. HOME fast/full snapshots and the detailed Auto Blinker API expose the same NOA session phase and remaining stabilization/exit times. The top Blinker metric renders stabilization as a rounded-up `10s` through `1s` countdown, then changes to `READY`; `PAUSED`, `EXIT WAIT`, `STANDBY`, and `OFF` retain priority through the shared renderer.
5. Fresh regression: **64/64 Python/static contract scripts PASS**, **31/31 C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`, **8/8 HOME Blinker countdown behavior cases PASS**, **5 source + 5 embedded JavaScript blocks PASS**, and generated `index_html.h` host compile PASS. Final dashboard is **388,959 bytes source / 379,512 bytes minified / 80,805 bytes gzip**, below the strict **84,000-byte** limit.
6. ESP32-S3 compile/link PASS with Arduino CLI 1.5.1, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1 and `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. Result: **1,509,959 bytes (48%)** program storage and **71,840 bytes (21%)** global RAM.

## Lane-change cancel hotfix
1. S3XY NOA Lane Change Cancel and Door Open Lane Change Cancel share the same manual cancel helper and no longer read `visualBehaviorType` / `DAS_behaviorType` for eligibility or direction.
2. The manual cancel helper still requires a fresh NOA gate and a fresh `0x24A` context frame before arming `UI_ulcSnooze`; the existing 1 s pending-request timeout and CAN-B transmit admission remain unchanged.
3. Auto Blinker planner logic is unchanged and still decodes `DAS_behaviorType` LEFT/RIGHT for automatic lane-change confirmation.
4. Regression coverage includes a static contract that rejects any reintroduction of `visualBehaviorType`, `behavior == 2/3`, `dir != 0`, or `BLOCKED: no lane-change request` inside the shared manual cancel helper.

## Baseline v3.7 validation

1. Firmware identity and production defaults are covered by static and pure-logic tests: Rev.4 uses **1.80–2.60 Nm**, **0.90–3.00 s**, **0.50–1.50 s**, carrier **0.10–0.60 Nm**, TIERED **0.40/2.00 Nm**, Visual Rescue **ON / 0.5 s**, and **HARD PAUSE**.
2. R79 call-site and deleted-symbol audits confirm one production policy: accepted stock MUX1 fast echo with a bounded **2 ms** queue wait plus one MUX2-anchored **+150 ms** refresh. Both paths apply `bit19=0` and `bit47=1`; bit 18 is selected only as **STOCK** or **FORCE 0** (default). Retired transport, timing, scheduler, capture, and writable LAB-control symbols are absent from product code.
3. Auto Blinker coverage verifies the independent **1–20 s / 10 s default** NOA stabilization timer and shared **10–100 s / 20 s default** cancel-pause toggle. Driver-door-open and S3XY Cancel share the same toggle state, a second action releases it, and direct S3XY Left/Right requests bypass both gates.
4. AP Right Scroll coverage verifies CAN-B-only routing for YL and standard profiles, immediate UP/DOWN on a new visual-warning epoch, **1–5 s / 2 s default** repeats while the warning remains active, physical-input priority, and failure/disable/recovery reset behavior.
5. Persistence schema **3** tests cover ordered write/readback/marker/cleanup migration, interruption retry, production defaults, legacy-value promotion, obsolete-key deletion, and Settings reset without clearing retained BUS OFF evidence.
6. Fresh host regression: **64/64 Python/static contract scripts PASS** and **31/31 C++ pure tests PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`. The generated `index_html.h` host compile also passes.
7. Dashboard verification: **5 source + 5 embedded inline scripts PASS** with `node --check`; two deterministic Zopfli builds are byte-identical. Final sizes are **387,754 bytes source / 378,309 bytes minified / 77,341 bytes gzip**, below the strict **84,000-byte** limit. `index_html.h` SHA-256: `d8352b82721b004a0296fd1a1900a400e2dadca02170c9a784f1418424eaa32e`.
8. Mobile browser QA at **390×844** covers Settings Auto Blinker, Summon/R79, AP Right Scroll, and the LAB R79 status card: no horizontal overflow was found, the bit-18 selector synchronized correctly, and the LAB card contains **zero interactive controls**.
9. Full ESP32-S3 compile/link was not run because `arduino-cli`, PlatformIO, and the Xtensa ESP32-S3 target compiler are unavailable in this environment. No target-binary or target-build claim is made.

# T2CAN Universal v3.6f3 — Validation

## Feature-promotion / BUS OFF persistence patch

1. Full host regression: **62/62 Python/static contract scripts PASS** and **34/34 C++ pure tests PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
2. Blinker policy tests confirm the production default is profile-dependent: Model YL uses one stock-synchronized transmit, while all non-YL supported 3/Y profiles use the existing Legacy path by default. LAB can temporarily select either mode; LAB OFF/reboot restores the profile default and the override is never persisted.
3. `0x3F8` audit confirms a single CAN-B RX-follow compositor (`injectDriverAssistControl`) owns production overlays. It copies the accepted stock frame, composes Confirm-Free bit 1, ULC Off-Highway bit 15, ULC Blind Spot bits 52–53, and ALC Off-Highway bit 56, then makes one `canTxTwaiTransmit` request only when the composed frame changed. No CAN-A `0x3F8` sender remains.
4. Feature-domain migration schema **2** copies supported legacy values into production `ulc` and `alc293lab` namespaces, verifies the writes and commits the schema marker before deleting obsolete experiment keys. Dedicated migration tests cover interruption/retry and value preservation.
5. BUS OFF persistence tests cover record encoding/CRC, torn-write rejection, newest valid dual-slot selection, current/previous-boot provenance, source-tagged bounded TX traces, asynchronous retry, and clear verification. The Reset Stats API returns `bus-off-persistence-clear-failed` and preserves evidence when NVS clearing cannot be verified.
6. Dashboard verification: **5 source + 5 embedded inline scripts PASS**; generated-header host compile PASS; decompressed embedded HTML matches the minified source. Two Zopfli builds are byte-identical. Final sizes are **411,218 bytes source / 401,733 bytes minified / 82,810 bytes gzip**, inside the strict **<84,000-byte** budget. `index_html.h` SHA-256: `38f2cb1ba4626186c5028b37f628c14b95f3d2614cb0fa7608e9087a31701e77`.
7. Mobile browser QA at **390×844** found no horizontal overflow, duplicate IDs, or console errors. Firmware Profile height is **94.5 px** versus the untouched baseline's **314.5 px** (**30.0%**, below the 55% limit); the hidden S3XY/BLE label and card both collapse to zero height.
8. Deleted-symbol and call-site audits found no product-code references to ACC Follow, ULC Speed, the removed `0x3F8` target-bus state, or TLSSC Green-Light. Promoted Settings panels and the retained LAB 0x293/monitor/blinker comparison panels are covered by static and browser tests.
9. Full ESP32-S3 compile/link was not run because `arduino-cli`, PlatformIO, and the Xtensa target toolchain are unavailable in this environment. No new firmware-binary size or target-build claim is made.

## Mode H Rev.4 approved defaults + Hands-On Policy patch

1. Rev.4 defaults are Primary Peak **1.80–2.60 Nm**, WAIT **0.90–3.00 s**, REFRACTORY **0.50–1.50 s**, opposite carrier **0.10–0.60 Nm**, Visual Warning Rescue **ON**, and Rescue Delay **500 ms**.
2. Rev.4 exposes the same policy values as Rev.3 and defaults to **TIERED HO=1 / 2** with **0.40 Nm / 1.75 Nm** thresholds. Boundary tests confirm `<0.40 Nm` preserves stock HO, `0.40–1.74 Nm` generates HO=1, and `>=1.75 Nm` generates HO=2. ALWAYS and THRESHOLD modes are also covered; HO=3 is never generated.
3. Policy selection uses the final transmitted torque magnitude after Rev.4 event/carrier calculation. Confirmed-stopped STOCK CARRIER bypasses the override and retains the full accepted stock HO field.
4. NVS schema **17** persists `h4hop`, `h4ho1`, and `h4ho2`. An exact former v16 default profile migrates to the approved values; a customized profile retains its peak/timing/carrier/rescue values.
5. Local regression verification: **55/55 Python/static integration scripts PASS** and **34/34 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
6. Dashboard verification: **5 source + 5 embedded inline scripts PASS**; generated-header host compile PASS; decompressed embedded HTML matches the minified source. Final dashboard is **418,754 bytes source / 409,238 bytes minified / 83,853 bytes gzip**, inside the strict **<84,000-byte** budget. `index_html.h` SHA-256: `a6e7f51d87ede3099c7eee47c94bf13874faa8735eab9d7425cdd96e1e5ddcd5`.
7. Full ESP32-S3 compilation was not run because Arduino CLI, PlatformIO, and the Xtensa ESP32-S3 toolchain are unavailable in this environment. No new firmware-binary claim is made.

## S3XY 0x249 stock-synchronized single-shot patch

1. A direct S3XY stalk request no longer enters `blinkATxTick()`'s 20 ms timed pulse. It is kept in a separate pending state and consumed only by the next real stock `0x249` on the active body/VH bus.
2. One request produces at most one direct overlay. The pending request expires after **250 ms**, a newer Left/Right request replaces it, and a non-idle physical-stalk frame consumes it without transmitting.
3. Auto Blinker retains the existing 350 ms / 20 ms pulse path. The stalkless `0x3C2` press/release path is unchanged. CAN recovery clears the new pending request and the direct-origin latch.
4. Local regression verification: **54/54 Python/static integration scripts PASS** and **34/34 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
5. Full ESP32-S3 compile/link was not rerun for this source patch because `arduino-cli`, PlatformIO, and the Xtensa target toolchain are unavailable in the current environment. No new binary-build claim is made; the release validation below remains the supplied v3.6f3 baseline record.

1. Baseline is the supplied **v3.6f1 LP/YL source**. v3.6f3 includes Mode H Rev.4 Visual Warning Rescue/revision ordering and the RAM-only R79 MUX0 Collision LAB transport.
2. Rescue defaults **OFF** with a **500 ms** delay. The accepted range is **0–2000 ms**; `h4vres` and `h4vdly` persist independently under Nag configuration schema **16**.
3. The warning epoch is created only by a DAS Hands-On transition from `<3` into `3/4/5`, using the transition's own `millis()` timestamp. `3 -> 4 -> 5` does not retrigger, `6 -> 3` does not falsely create an epoch, and a later transition through `<3` re-arms the feature.
4. Clearing the warning before the deadline cancels the pending Rescue. At or after the deadline, the next eligible stock `0x370` RX starts a fresh Rev.4 event directly in PRIMARY peak. The normal Rev.4 RAMP_OUT -> REFRACTORY/carrier -> WAIT/carrier flow remains unchanged, with no added CAN scheduler.
5. The visible dashboard selector is **Rev.1 -> Rev.3 -> Rev.4**. Legacy persisted Rev.2 variant data remains readable for compatibility but has no visible selection button.
6. MUX0 Collision accepts only **40–52 ms**, defaults to **40 ms**, arms on every real MUX0 with the previous stock MUX1 snapshot, and is unaffected by later MUX1/MUX2 RX. Its 256-entry reservation FIFO matches the configured TWAI RX queue; at most one due reservation is serviced per CAN task loop to avoid a self-created TX-queue purge cascade. The due request uses one zero-wait enqueue; only its `ESP_ERR_TIMEOUT` queue-full result permits a local queue clear and one immediate retry. No delayed retry exists.
7. Collision mode and delay are RAM-only. NVS save maps the active collision strategy to D9, configuration load resets delay to 40 ms, and no collision-delay key is read or written.
8. Regression verification: **53/53 Python/static integration scripts PASS** and **33/33 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
9. Dashboard verification: deterministic Zopfli output is **417,010 bytes source / 407,512 bytes minified HTML / 83,750 bytes gzip**, inside the strict **<84,000-byte** budget. `index_html.h` SHA-256: `9efbd1be6123d1651fbff5acd8d94ea42def7027f6f2d303f4ee4005ef3b84bb`.
10. Full ESP32-S3 compile and link **PASS** with ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1, and `lilygo_t_display_s3` options `USBMode=hwcdc, CDCOnBoot=cdc, PartitionScheme=app3M_fat9M_16MB, LoopCore=1, EventsCore=1`. Final result: **1,522,274 bytes (48%)** program storage and **68,504 bytes (20%)** global RAM. Application BIN SHA-256: `29719b6d303054e791412d73a494b3b97f176c5c1cea7944effa188dd3d76c48`.

# T2CAN Universal v3.6f1 — Validation

1. Baseline is the supplied **v3.6e2 LP/YL** source. v3.6f1 adds only the LAB AP Right Scroll experiment and the RAM-only ROAMING MUX1 Burst selector plus their UI/API/diagnostics, tests, and release metadata.
2. **AP Right Scroll** is limited to Model YL, CAN-B/VH `0x3C2`, MUX1, LAB enabled, feature enabled, confirmed AP-active state, TWAI ready, and no administrative hold. It clones the accepted live stock frame and changes only Byte3 bits `[5:0]`: UP `0x01`, then DOWN `0x3F` on the next accepted real MUX1 frame.
3. AP Right Scroll defaults **OFF** with a **30-second** interval, accepts **1–600 seconds**, and persists only its enable/interval values in NVS (`rsEnabled`, `rsInterval`). A physical right-scroll value wins over injection and restarts the interval. AP/LAB/feature disable, CAN recovery, or administrative hold clears any pending UP/DOWN sequence.
4. **ROAMING MUX1 Burst** is a RAM-only `1x / 2x / 3x` selector and defaults to `1x` at every boot/config load. It has no Preferences/NVS read or write. The existing ROAMING ratio decision occurs first; silence suppresses the entire cycle; Shot 1 retains the existing 5 ms ROAMING mirror path.
5. After a successful Shot 1, Shot 2/3 reuse the same corrected mux1 template. At most one extra shot is requested per CAN loop, only after the alert latch and authoritative TWAI `msgs_to_tx` status both confirm the CAN-B TX pipeline is idle, with a zero-wait TWAI enqueue and no retry. Extra shots pass through the recovery epoch/freshness barrier. Any newer real stock `0x3FD` cancels pending shots before that stock frame's normal handling. Strategy change, silence, common safety/administrative gate closure, CAN recovery, and administrative hold also cancel pending shots.
6. R79 Timing Capture includes `ROAMING_BURST_2`, `ROAMING_BURST_3`, and `ROAMING_BURST_CANCEL_STOCK`. Captured timestamps represent TX request time, and capture metadata snapshots the RAM-only burst selection.
7. Regression verification: **51/51 Python/static contract scripts PASS** and **31/31 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`. The new tests cover input validation, bit-field preservation, interval/rollover behavior, physical-input priority, burst ratio/state progression, pipeline-idle admission, stock cancellation, and no burst NVS persistence.
8. Dashboard verification: **5 source + 5 embedded inline scripts PASS** with the bundled Node runtime; generated `index_html.h` host compile PASS; decompressed embedded HTML matches the current minified `dashboard_source.html` byte-for-byte. Final dashboard: source **413,411 bytes** / embedded minified HTML **403,937 bytes** / deterministic gzip **83,036 bytes**, inside the existing strict **<84,000-byte** budget. `index_html.h` SHA-256: `fd9c30c420b4db94ca1dfa7d9c6cfded353379133c6fbc08244288e7080e527a`.
9. Two consecutive Zopfli dashboard builds are byte-identical. The checked-in output is a standard gzip stream; the build tool retains its existing stdlib zlib-9 fallback, although the release artifact was generated with deterministic Zopfli to satisfy the size budget.
10. Full ESP32-S3 compile and link **PASS** with Arduino CLI 1.5.1, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1, and generic ESP32S3 Dev Module options `FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, CDCOnBoot=cdc`. Result: **1,537,231 bytes (48%)** program storage and **64,424 bytes (19%)** global RAM. Delivery remains source-only as requested.

# T2CAN Universal v3.6e2 — Validation

1. Baseline is the supplied **v3.6e1 RATIO-FIX** source. R79 ratio/timing transport files and policy are treated as regression-sensitive and are not intentionally changed.
2. Mode H profile ID `1` now resolves to Rev.4. Rev.4 defaults are Carrier **0.10–0.60 Nm**, Primary Peak **1.50–2.10 Nm**, WAIT **0.90–2.00 s**, REFRACTORY **0.50–1.50 s**, Primary direction **80% NEG / 20% POS**, stock deadband ±**0.05 Nm**.
3. Rev.4 carrier is active in WAIT and REFRACTORY, re-samples on every eligible stock `0x370` RX, and uses the existing Mode-H RX-follow TX path with no additional scheduler. A separate carrier RNG stream prevents RX-rate-dependent changes to Primary event RNG.
4. Direct S3XY Left/Right Blinker requests bypass Auto-Blinker/NOA/ALC/Advanced-EAP/Confirm-Free policy while retaining actual route/template, physical-input, and transport safety checks. Auto Blinker retains those policy gates.
5. Mode H LAB edit-state protection uses dirty/saving/epoch guards so background polling cannot clobber unsaved numeric/select values or apply stale responses after a newer user edit/save.
6. Regression verification: **50/50 Python/static contract scripts PASS** and **29/29 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`. The Rev.4 pure test covers defaults, exact opposite-stock arithmetic, ±0.05 Nm passthrough, per-RX carrier re-sampling, 80/20 Primary direction distribution, WAIT carrier, REFRACTORY carrier, and RAMP_OUT-to-carrier transition.
7. Dashboard verification: **5 source + 5 embedded inline scripts PASS** with `node --check`; generated `index_html.h` host compile PASS. Final dashboard: source **407,540 bytes** / embedded minified HTML **398,072 bytes** / deterministic gzip **81,934 bytes**, inside the existing **<84,000 byte** budget. `index_html.h` SHA-256: `a6beb989505256c7cc81bd280836df53a5548598542420835d38041d2d7d65ef`.
8. Two consecutive dashboard builds are byte-identical. Zopfli is used when available; the checked-in output is a standard gzip stream and the build tool retains a stdlib zlib-9 fallback.
9. Baseline-diff audit confirms no changes to `can_runtime.h`, `r79_timing_capture.*`, or the R79 scheduler/pure modules. `vehicle_logic.h` changes are confined to the direct-vs-auto S3XY blinker policy/transport path.
10. Full ESP32-S3 Arduino target compilation was **not** run because `arduino-cli`, PlatformIO, and the Xtensa ESP32-S3 target compiler are not installed in this environment. No binary-build claim is made.

# T2CAN Universal v3.6d9a6 — Validation

1. Baseline is the supplied **v3.6d9a5** source package; R79 transport behavior and d9a5 defaults are preserved.
2. S3XY persisted action IDs 0–10 are unchanged. New `Left Blinker` / `Right Blinker` actions append IDs **11 / 12** and use the existing validated one-shot turn-signal transport.
3. Direct S3XY blinker actions are independent of Auto Blinker enable/NOA/ALC state, but retain the existing profile/turn-route guard and ULC no-confirm collision protection. Stalk routes use the established 350 ms pulse; stalkless routes use the established press/release cycle.
4. Dashboard disabled-state styling is global for disabled inputs/selects/buttons and additionally dims atomic Settings/LAB/Devices rows through CSS `:has(:disabled)`. No polling or runtime JavaScript is added for the visual effect.
5. **47/47 Python/static contract files PASS; 28/28 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`. Dashboard JavaScript **5 inline scripts PASS** with `node --check`; generated `index_html.h` host compile PASS.
6. Final dashboard: source **402,864 bytes** / embedded minified HTML **393,466 bytes** / gzip **83,996 bytes**, preserving the existing **<84,000 byte** budget. `index_html.h` SHA-256 `e4bcf8a6eb171db56964f6f6dd3d78ecca2c3950a9961ccfd43309a0f17024ac`.
7. Full ESP32-S3 Arduino target compilation was not run because Arduino/PlatformIO target tooling is not installed in this environment.

## Inherited v3.6d9a5 validation

1. Baseline is the supplied/previously generated **v3.6d9a4** source; D9 CURRENT is the default and retains d9a4 runtime behavior.
2. Source verification was performed directly against the supplied archives:
   - `t2can-roaming-main(1).zip` SHA-256 `e097b645773dc117b77fe49c05523b3bd31c50f17a33b756ef7c4822a90bb774`: `fsd_apctl_build()` copies stock mux1 and changes only bit19=0 / bit47=1; CAN-B TWAI send uses a 5 ms bounded wait; no R79 periodic path is present.
   - `Summon-Unlock-main(2).zip` SHA-256 `a17b3dde7822a36a47569f14f346c3da5cf8d61e84c9dee6a2375de152bfe49c`: mux1 immediate mirror uses a 2 ms bounded wait, and `SUMMON_PERIODIC_TX_MS` is 500 ms with the latest real mux1 template while `gateSummoning` is true.
3. D9 CURRENT, ROAMING MIRROR and V2.6 LEGACY are mutually exclusive. Selecting a legacy transport disables d9-only QW/PRE/POST/bit18 controls in the dashboard and stops d9 background R79 scheduling.
4. Legacy payloads preserve stock bit18 and all stock bytes except bit19/47. No delayed retry, queue flush, QW, PRE-MUX1, POST-MUX2 or 487/500 ms d9 scheduler is executed in ROAMING MIRROR. V2.6 LEGACY adds only its source-derived Summoning-only 500 ms periodic latest-mux1 resend.
5. All strategies share the current d9 manual-D/R, TWAI-ready and administrative-hold safety gate. This is intentional A/B isolation of transport behavior and is not represented as a full clone of either source firmware's authorization gate.
6. Strategy changes reset pending schedulers/retries and fast/PRE TX-success attribution state. The selection is NVS-persistent and Timing Capture snapshots the transport name in each CSV header.
7. Dedicated timing slots are present for `ROAMING_MIRROR`, `V26_MIRROR`, and `V26_PERIODIC`; legacy API counters are separate from the D9 Fast Reactive mode counters.
8. Verification completed: **47/47 Python/static contract files PASS; 28/28 host C++ pure tests PASS** with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`; dashboard JavaScript **5 inline scripts PASS** with `node --check`; generated `index_html.h` host compile PASS.
9. Final dashboard: source 402,864 bytes / embedded minified HTML 393,466 bytes / gzip **83,996 bytes**, preserving the existing **<84,000 byte** budget. `index_html.h` SHA-256 `e4bcf8a6eb171db56964f6f6dd3d78ecca2c3950a9961ccfd43309a0f17024ac`.
10. Full ESP32-S3 Arduino target compilation was not run because Arduino/PlatformIO target tooling is not installed in this environment.

# T2CAN Universal v3.6d9a4 — Validation

1. Based directly on v3.6d9a3. POST-MUX2 is default OFF, so the existing runtime behavior is preserved until explicitly enabled.
2. POST-MUX2 delay accepts 0–149 ms and is independent from the Quiet Window +150 ms hard-start guard.
3. Any newer real stock 0x3FD cancels a pending POST-MUX2 reservation before it fires; no delayed POST-MUX2 retry exists.
4. Timing Capture includes `POST_MUX2` slot tags and capture-start POST-MUX2 ON/OFF + delay metadata.
5. Dashboard polling cannot clobber the POST-MUX2 numeric input while it is focused or dirty.
6. Host verification: 45 Python/static contracts PASS; 28 C++ pure-logic tests PASS; dashboard JavaScript syntax PASS; generated `index_html.h` host compile PASS.
7. Dashboard gzip size after final build: 83,990 bytes (existing 84,000-byte budget preserved).
8. Final generated `index_html.h` SHA-256: `6ecfb8639e2f895df5b2da978cd26296581298d69341a67cd4a8b44d1e3c499f`.
9. Full ESP32-S3 Arduino target compilation was not run because the Arduino ESP32 toolchain is not installed in this environment.

# T2CAN Universal v3.6d9a3 — Validation

1. Based directly on v3.6d9a2. Existing R79 post-mux1 A/B modes and defaults are unchanged.
2. Quiet Window Event Anchor is NVS-persistent with default MUX2. MUX2 behavior remains the compatibility baseline.
3. MUX0/MUX1 anchor experiments keep later muxes in the same stock cluster as recent-stock guard updates instead of cancelling the selected-anchor cycle. Unsafe early slots are still guard-skipped; changing the anchor does not bypass collision protection.
4. R79 Timing Capture is diagnostic-only: max 10 minutes / 16,384 compact 20-byte events, VIDEO SYNC marker, stock 0x3FD RX, R79 TX/guard result and queue snapshot. It does not originate CAN TX.
5. Capture hooks tag QW1/QW2/QW3, PRE_MUX1, PHASE487, FREE1/FREE2/FREE3 and FAST_REACTIVE. Capture-start scheduler/config values are snapshotted into the CSV header, and R79 settings are locked while recording.
6. CSV download uses a fixed 2 KiB chunk buffer plus a hidden browser download frame; no giant CSV String is built in RAM and the dashboard document is not navigated away.
7. Host verification: 44 Python/static contracts PASS; 27 C++ pure-logic tests PASS; R79 timing module host compile/runtime smoke test PASS.
8. Full ESP32-S3 Arduino firmware compilation was not run in this environment because the Arduino toolchain is not installed.

# T2CAN Universal v3.6d9a2 — Validation

v3.6d9a2 is a narrow A/B experiment over d9a1. Only the initial stock-mux1-triggered R79 TWAI queue wait is selectable.

## d9a2 contract

1. `FAST ECHO · 0 ms` uses the d9a1 initial `twai_transmit(&out, 0)` path.
2. `V2.6 STYLE · 2 ms WAIT` uses the same modified R79 mux1 payload and authorization path but calls `twai_transmit(&out, pdMS_TO_TICKS(2))` for the initial enqueue.
3. A mode switch does not alter bit18/19/47 policy, Periodic/PRE scheduling, emergency queue flush, or the existing bounded retry sequence.
4. The selected mode is NVS-persistent and defaults to 0 ms for existing installations.
5. Aggregate Fast Echo diagnostics remain intact; additional counters separate 0 ms and 2 ms initial attempts/OK/FAIL and RX-dequeue→TX-request latency.

## Fresh d9a2 verification

- Python/static regression suite: **43/43 PASS**, including a dedicated post-mux1 mode contract.
- Host C++ pure suite: **26/26 PASS**.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Dashboard is **397,950 bytes source / 388,559 bytes embedded minified HTML / 83,115 bytes gzip**.
- `index_html.h` SHA-256: `6a48ea46473773274f64401ec56f76f785b4dcd16b28f269075c0edd9b6528a5`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---
# T2CAN Universal v3.6d9a1 — Validation

v3.6d9a1 is a compile-only hotfix over v3.6d9. The d9 R79/PRE-MUX1 behavior is intentionally unchanged.

## d9a1 contract

1. `CAN_TRAFFIC_UI_FRESH_MS`, `CanTrafficUiSnapshot`, and `canTrafficUiSnapshot()` exist in `web_api.h` before `canTrafficStatsToJson()` and all later API consumers.
2. CAN traffic freshness semantics remain the prior working behavior: 1500 ms threshold, separate CAN A/CAN B seen/age/online fields.
3. R79 PRE-MUX1, Guarded 487 Phase-Walk, Quiet/Phased schedulers, numeric-input dirty protection, Mode H, CAN routing, and all unrelated runtime behavior remain unchanged from d9.

## Fresh d9a1 verification

- Python/static regression suite: **42/42 PASS**, including a new compile-order contract for `CanTrafficUiSnapshot`.
- Host C++ pure suite: **26/26 PASS**.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Dashboard is **395,628 bytes source / 386,239 bytes embedded minified HTML / 82,605 bytes gzip**.
- `index_html.h` SHA-256: `dedfa2942f1402ea0c601c26cbcf632aac1b4991734c3c5a57796c3ccbcef11f`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---
# T2CAN Universal v3.6d9 — Validation

v3.6d9 is the next R79 timing experiment after the d8 46-minute MUX2 Quiet 2x result: CAN-B transport was stable but Summon-range flicker persisted. The release therefore preserves existing schedulers, adds guarded 487 ms phase diversity and an independent pre-mux1 timing experiment, while fixing the observed Quiet multi-shot boundary and LAB numeric-edit issues.

## d9 contract

1. Existing Periodic Refresh modes (`ALWAYS / SUMMON ONLY / OFF`) and d8 schedulers remain available. `GUARDED_487_PHASE_WALK` is an additional scheduler, not a replacement.
2. Guarded 487 advances its due time by exactly 487 ms on every due slot, including unsafe/disallowed slots. Guard skips **do not re-anchor** the phase. The latest real mux1 only updates the phase-safety reference.
3. Guarded 487 fires only when the newest stock `0x3FD` is at least 30 ms old and the current latest-mux1-relative phase is 90–410 ms. It is fixed to one periodic shot per 487 ms cycle.
4. PRE-MUX1 is an independent ON/OFF experiment. A real mux0 arms exactly one request 10–30 ms later (20 ms default) only when a previous real stock mux1 template exists. Any subsequent stock mux frame before due cancels the pending PRE.
5. PRE uses the previous stock mux1 template and the normal R79 bit policy. It is zero-wait TWAI enqueue and deliberately has no +5/+15/+30 ms retry, preventing delayed PRE from entering the stock mux1 window.
6. Quiet Window target slots no longer sit on the hard safety edge: target end is +300 ms after mux2 and hard end is +340 ms. Default +200 ms plans are 1x `200`, 2x `200/300`, 3x `200/250/300` ms.
7. LAB/API expose scheduler-wide and per-slot due/fire/guard counters plus PRE timing/template/TX_SUCCESS telemetry.
8. R79 numeric editors use focused/dirty protection against dashboard polling, preventing an in-progress 150/other value from being overwritten by the previous server value.
9. Fast Reactive Echo, fail-open R79 authorization, d7 heartbeat diagnostics, WebKit scroll restoration, Mode H Stop Carrier, Rev.3 tuning and unrelated CAN features are not intentionally changed.

## Fresh d9 verification

- Python/static regression suite: **41/41 PASS**.
- Host C++ pure suite: **26/26 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- Dedicated d9 pure coverage verifies 487 ms phase advancement across guard skips, PRE mux0 arming/cancellation, and the new Quiet 2x/3x target plans.
- Dedicated d9 static coverage verifies PRE integration/API/UI telemetry, guarded phase-walk presence, per-slot counters and numeric-input dirty protection.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Dashboard is **395,626 bytes source / 386,237 bytes embedded minified HTML / 82,604 bytes gzip**.
- `index_html.h` SHA-256: `174c37ac56843459bb0d2cfea8f2fc751e473dfb449116099aeede4c2fd5cd73`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---
# T2CAN Universal v3.6d5 — Validation

v3.6d5 is a focused Mode H stationary-transport A/B experiment based on v3.6d4. It does not intentionally change the Mode H event waveform while moving, R79 transport, CAN routing, or unrelated vehicle-control features.

## d5 contract

1. Fresh/upgraded d5 defaults Mode H Stop Behavior to **STOCK CARRIER**, with **HARD PAUSE · 0-TX** retained as an immediate NVS-persistent A/B option.
2. In STOCK CARRIER, only a positively confirmed Mode H STOPPED block is converted from 0-TX to a transport-continuity frame. Speed-stale and move-confirming blocks remain 0-TX.
3. The stop-carrier frame starts from the accepted stock 0x370 frame, preserves stock torque and the complete stock 2-bit Hands-On field, and changes only the normal rolling counter/checksum used by the existing injection transport.
4. The Mode H runtime phase/event engine remains `PAUSED_STOPPED`; no interaction event, WAIT carrier magnitude, HO=1 or HO=2 override is generated while stop-carrier mode is active.
5. LAB exposes the current stop behavior, active state and successful stop-carrier TX count. HOME distinguishes `CARRIER` from the legacy `PAUSED` state.
6. NVS schema is v14 and stores the stop behavior under its own key without changing existing Rev.3 tuning or the d4 487 ms R79 interval defaults.
7. The HOME lightweight snapshot resolves Rev.3 pause state from the Rev.3 runtime object.

## Fresh d5 verification

- Python/static regression suite: **39/39 PASS**.
- Host C++ pure suite: **24/24 PASS**, including dedicated stop-behavior validity/selection coverage.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Dashboard is **384,931 bytes source / 375,568 bytes minified input / 79,934 bytes gzip**.
- `index_html.h` SHA-256: `ebcfacb1f8ab1a01bea74e872a77538af866a7dfac3f249314089b76221e51e9`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---
# T2CAN Universal v3.6d4 — Validation

v3.6d4 is a default-profile tuning release based on v3.6d3. It does not intentionally change R79 transport, CAN routing, or the d3 diagnostics/download fixes.

## d4 contract

1. R79 Periodic Refresh mode remains default OFF; the default interval changes from 500 ms to 487 ms.
2. Direct interval editing remains 20–5000 ms and existing saved interval values remain persistent.
3. Fresh/default Mode H profile selection is Rev.3; an existing valid saved profile selection remains persistent.
4. Rev.3 defaults are 1.50–2.10 Nm primary peak, 1.2–2.5 s WAIT, 0.8–1.8 s refractory, +0.30–0.40 Nm WAIT carrier, FOLLOW STOCK carrier direction, TIERED HO=1/2, HO1 0.40 Nm, HO2 1.75 Nm.
5. Arbitrary custom Rev.3 NVS tuning is preserved. The untouched d3 default and the supplied pre-d4 screenshot profile are migrated to the d4 defaults; Reset Current Profile resolves to the same values.
6. d3 Fast Reactive Echo, fail-open R79 authorization, TX_SUCCESS timing diagnostics, download cleanup, CAN-A burst prefetch, and dark-mode select-arrow behavior remain unchanged.

## Fresh d4 verification

- Python/static regression suite: **38/38 PASS**.
- Host C++ pure suite: **23/23 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- Dedicated d4 coverage verifies the 487 ms R79 default/migration, fresh Rev.3 selection, exact Rev.3 tuning defaults, d3-default/pre-d4-profile migration, and preservation of unrelated custom Rev.3 tuning.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Dashboard is **382,848 bytes source / 373,489 bytes embedded minified HTML / 79,372 bytes gzip**.
- `index_html.h` SHA-256: `db3b31d1aba1d23b4772820405357fc13cfb07718dbf614d14424cc4a24d30c7`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---

# T2CAN Universal v3.6d3 — Validation

v3.6d3 is a follow-up to the d2 R79 timing experiment. It preserves the d2 Fast Reactive Echo/control policy and focuses on measurement quality, dashboard reliability, CAN-A burst tolerance, and the requested R79 interval/dark-mode UI changes.

## d3 contract

1. R79 Periodic Refresh remains default OFF and Fast Reactive Echo behavior/payload policy is unchanged from d2.
2. Periodic interval accepts any integer from 20 through 5000 ms and is edited with a numeric input rather than a fixed select list.
3. R79 TX_SUCCESS timing is diagnostic-only and is labeled as a software-observed upper bound; ambiguous/interleaved CAN-B TX sequences are not reported as valid samples.
4. Generic dashboard downloads always clear `downloadInProgress` in a `finally` path and resume polling after completion/failure.
5. Dark-mode select arrows use `background-color` plus explicit no-repeat/position/size rules; the dark theme must not reset the custom-arrow background with shorthand `background:`.
6. CAN A retains the 32-frame bounded task budget but prefetches a small raw MCP2515 batch before downstream observer/decoder work.
7. TWAI error-rate UI reports session-average counts/minute using cumulative counters and uptime instead of extrapolating one dashboard polling interval.
8. Existing NAG/Mode H, Auto Blinker, PedalMap, 0x293, 0x3F8, TLSSC, S3XY, Summon detection, and d1/d2 R79 authorization semantics are not intentionally changed.

## Fresh d3 verification

- Python/static regression suite: **37/37 PASS**.
- Host C++ pure suite: **22/22 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- Dedicated d3 static coverage verifies direct integer R79 period validation, dark-mode non-repeating select arrows, generic download `finally` cleanup, CAN-A MCP2515 burst prefetch, TX_SUCCESS alert correlation/upper-bound labeling, and unified d2/d3 R79 diagnostic reset.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Current dashboard is **382,768 bytes source / 373,409 bytes embedded minified HTML / 79,313 bytes gzip**. `index_html.h` SHA-256 is `d14a4a813217df07ab67cca5bb0d87bc6c36ad96b34d78052b50d7bbebcd0385`.
- Minimal generated `index_html.h` host compile with Arduino/PROGMEM stub: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---

# T2CAN Universal v3.6d2 — Validation

v3.6d2 is a focused R79 timing experiment based on v3.6d1. It changes the stock-triggered immediate 0x3FD mux1 path from the normal post-observer transmitter into an early receive-synchronized fast request while deliberately preserving d1 authorization and failure-recovery semantics.

## d2 fast reactive contract

1. A standard 8-byte `0x3FD mux1` frame invokes `r79FastReactiveEcho()` immediately after TWAI dequeue and before normal CAN-B accounting/capture/decoder work.
2. The fast authorization gate preserves v3.6/d1 fail-open semantics and uses the existing manual D/R suppression latch rather than introducing a fresh-gear/fail-closed gate.
3. The first reactive request uses zero queue wait: `twai_transmit(&out, 0)`.
4. `r79LabObserve3fdMux1()` becomes accounting/template capture only and cannot issue a second immediate R79 request.
5. d1 READY/ACTIVE emergency flush and bounded `+5/+15/+30 ms` recovery remain available only after a failed fast request; the existing periodic/retry path is otherwise retained.
6. First d2 boot performs a one-time NVS migration to `Periodic Refresh = OFF` to isolate reactive behavior. Later user changes are persistent and are not overwritten again.
7. Fast-path microsecond telemetry is defined as **TWAI dequeue → TX request**. It is not claimed to be physical wire RX→TX latency.
8. Existing d1 queue-priority thresholds and the d1 fail-open CAN-recovery behavior remain unchanged.
9. Existing API keys remain preserved; d2 adds fast-reactive diagnostic keys only.

## Fresh d2 verification

- Python/static regression suite: **36/36 PASS**.
- Host C++ pure suite: **22/22 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- Dedicated d2 static contract verifies early call ordering, zero-wait initial TWAI request, no duplicate observer TX, unchanged fail-open gating, one-time Periodic-OFF migration, and microsecond telemetry keys.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`. Dashboard generation is deterministic across consecutive runs. Embedded dashboard is **78,811 bytes gzip** / **370,454 bytes minified HTML**; `index_html.h` SHA-256 is `06e30e73c2c31a4943d7c7ce405fbdc510edcab7ab9da979d9651d514c5f13a0`. Minimal generated-header host compile: **PASS**.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** unless a compatible target toolchain is present.

---

# T2CAN Universal v3.6d1 — Validation

v3.6d1 is an R79/Summon CAN-B transport hardening release based on v3.6c7a1. The v3.6 default-on R79 authorization policy is preserved; changes are scoped to CAN-B application queue priority, bounded recovery, periodic success cadence, and diagnostics.

## d1 transport contract

1. PARK_STANDBY requires fresh real P gear; `gateParked` compatibility fallback is not a priority input.
2. SUMMON_READY starts from fresh ACA-active or non-zero SPR evidence; SUMMON_ACTIVE uses the existing confirmed session.
3. Non-R79 CAN-B traffic uses queue limits 14 in PARK_STANDBY and 6 in READY/ACTIVE.
4. `twai_clear_transmit_queue()` is reachable only from the R79 timeout path while READY/ACTIVE.
5. Recovery retries are bounded to +5/+15/+30 ms and always use the latest stock 0x3FD mux1 template.
6. Retry success cancels remaining retries; loss of Summon priority or R79 runtime authorization cancels the sequence.
7. Normal periodic timing is anchored to successful periodic TX; request time remains independent telemetry.
8. Retry exhaustion enters a full-period hold so persistent failure cannot create an unbounded transmit loop.
9. c7a1 JSON serializer architecture and existing API keys remain preserved; d1 adds diagnostic keys only.

## Fresh d1 verification

- Python/static regression suite: **35/35 PASS**.
- Host C++ pure suite: **22/22 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- New d1 pure coverage verifies NORMAL/PARK_STANDBY/SUMMON_READY/SUMMON_ACTIVE priority ordering, PARK queue limit 14, READY/ACTIVE queue limit 6, flush authorization, and retry delays 5/15/30 ms.
- New d1 static coverage verifies priority is based on fresh real gear rather than `gateParked`, queue flush is scoped to the R79 transport path, latest-template retry is present, retry tick precedes periodic scheduling, and the new telemetry/API/dashboard keys exist.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic across consecutive runs: **PASS**. Embedded dashboard is **78,545 bytes gzip** / **369,467 bytes minified HTML**; `index_html.h` SHA-256 is `199c0569d934d74e8b8a61c194f4c6dda6ae573c3a453c1a8003109f1f2a1048`.
- `index_html.h` minimal host compile with an Arduino/PROGMEM stub: **PASS**.
- Existing c7 API-key preservation contract: **PASS**; d1 adds diagnostic keys without removing the established contract.
- ESP32-S3 Arduino target compilation and `.bin` generation are **not claimed** in this environment because Arduino/PlatformIO target tooling is unavailable.

---

# T2CAN Universal v3.6c7a1 — Validation

v3.6c7a1 is a compile-only hotfix for the c7 shared JSON serializer refactor. The only production-code change is the corrected R79 JSON writer calls in `web_api.h`; the c7 optimization architecture is otherwise unchanged.

## c7a1 hotfix contract

1. Numeric `JsonWriterArduino` methods receive only numeric values, never legacy `String` concatenation expressions.
2. R79 JSON preserves separate `Rx0/Rx1`, `immediateTxOk/Fail`, and `periodicTxOk/Fail` fields.
3. No CAN/runtime behavior is intentionally changed.

## Fresh c7a1 verification

- Root cause reproduced from c7: the new numeric-writer regression fails at `web_api.h:1079` because a legacy `String` concatenation was passed as the second argument to `JsonWriterArduino::u32()`.
- Fixed all five malformed calls by emitting the paired JSON fields separately.
- Python/static regression suite: **34/34 PASS**.
- Host C++ pure suite: **21/21 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic`.
- Numeric JsonWriter malformed-call scan: **PASS**.
- Dashboard JavaScript: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generation deterministic: **PASS**; embedded gzip remains **77,904 bytes**.
- `index_html.h` minimal host compile: **PASS**.
- ESP32-S3 Arduino target compilation is not claimed in this environment because the target toolchain is not installed; c7a1 specifically addresses the exact compiler errors reported from the target build.

---

# T2CAN Universal v3.6c7 — Validation

v3.6c7 is a behavior-preserving second binary-size optimization pass on v3.6c6. The target is the linker-map hotspot in repeated dashboard/API JSON construction, not CAN/runtime behavior.

## c7 optimization contract

1. High-cost JSON/status builders share `JsonWriterArduino` instead of each compiling independent `String(value)` / boolean / fixed-point formatting sequences.
2. Nested HOME, S3XY, torque-array, and research-label payloads preserve their object/array structure.
3. `/api/snapshot` preserves the same group names and order through a compact static dispatch table.
4. API keys are checked against a v3.6c6 baseline contract; no key is intentionally removed or renamed.
5. `dashboard_source.html`, CAN injection/routing, Mode H/NAG, R79, Summon, TLSSC, S3XY BLE behavior, capture logic, and NVS behavior are not intentionally changed.
6. Exact ESP32-S3 `.bin` savings require a fresh c7 Arduino target build; host/static tests cannot establish the final linker delta.

## c7 TDD / regression additions

- `tests/test_v36c7_json_writer_host.py` verifies flat and nested JSON formatting through the shared writer.
- `tests/test_v36c7_json_writer_static.py` locks the c7 version and requires the targeted high-cost builders to use the shared writer.
- `tests/test_v36c7_api_key_contract.py` compares JSON key presence against `tests/c7_api_keys_expected.json`, generated from the c6 source baseline.
- `tests/test_v36c7_snapshot_table_static.py` verifies the compact snapshot group table and preserved group order.

## Verification environment

Host/static regression, pure C++ tests, JavaScript syntax checks, dashboard generation/gzip integrity, and generated-header host compilation are available in this environment. ESP32-S3 target compilation still requires Arduino/PlatformIO/ESP-IDF on a target-capable environment.

## Fresh verification

- Python/static regression suite: **34/34 PASS**.
- Host C++ pure suite: **21/21 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic`.
- Shared JsonWriter host contract covers flat fields, nested objects, array-contained child objects, and post-container sibling fields.
- API-key preservation contract covers **24** refactored JSON builders against the v3.6c6 source baseline.
- Dashboard JavaScript syntax: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generator is deterministic across consecutive runs. `dashboard_source.html` and generated `index_html.h` remain byte-identical to v3.6c6.
- Embedded dashboard remains **77,904 bytes gzip** / **367,482 bytes minified HTML**; c7 does not claim dashboard savings beyond c6.
- `index_html.h` host compile with a minimal Arduino/PROGMEM stub: **PASS**.
- Core CAN/NAG injection files (`can_runtime.h`, `can_core.h`, `t2can_forward.h`, `can_research_capture.h`, Mode H pure engines, 0x293/0x3F8/TLSSC pure policy) are byte-identical to v3.6c6.
- The refactored standalone functions accounted for approximately **88.3 KiB of `.flash.text` in the supplied v3.6c6 linker map** before this pass. This is the optimization target, not a claimed c7 saving.
- ESP32-S3 target compile and exact c7 `.bin` delta are **not claimed** because the target toolchain is not installed in this environment. A same-settings c7 `.bin/.elf/.map` build is required for the final linker-size comparison.

---

## v3.6c6 validation history


v3.6c6 is a behavior-preserving binary-size optimization pass on v3.6c5. The goal is to remove project-level float/diagnostic/embedded-dashboard overhead without changing CAN policy or user-visible API contracts.

## c6 optimization contract

1. Runtime/API formatting uses integer fixed-point helpers instead of project-level float/double formatting where the source data is already fixed-point raw CAN data.
2. Mode H LAB torque parsing no longer pulls `strtod()` into the project path and retains ordinary decimal/scientific input plus nearest-centi rounding.
3. Production serial diagnostics are compile-time disabled by default and remain opt-in with `T2CAN_SERIAL_DIAGNOSTICS=1`.
4. S3XY diagnostic-only temporary strings/work are excluded when diagnostics are disabled.
5. `dashboard_source.html` remains the readable source of truth; only the generated embedded payload is minified before deterministic gzip.
6. Existing CAN routing, injection gates/timing, NVS schema, Mode H behavior, R79 policy, TLSSC, S3XY actions, and capture behavior are not intentionally changed.
7. ESP32-S3 target `.bin` size is not claimed without the target toolchain/linker map.

## TDD / regression additions

- `tests/test_v36c6_fixed_point_pure.cpp` covers fixed-point formatting and the no-float unsigned centi parser, including decimal rounding and scientific notation.
- `tests/test_v36c6_binary_size_static.py` locks the c6 version, no-`strtod` contract, removed dead float state, production diagnostics switch, and an embedded dashboard gzip smaller than c5's 81,039-byte payload.

## Verification environment

The repository includes host/static regression, JavaScript syntax, deterministic dashboard generation, gzip integrity, and host compilation checks for the generated dashboard header. ESP32-S3 target compilation still requires Arduino/PlatformIO/ESP-IDF on a target-capable environment.

## Fresh host/static verification

- Python/static regression suite: **30/30 PASS**.
- Host C++ pure suite: **21/21 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic`.
- Dashboard JavaScript syntax: **5 source + 5 embedded scripts PASS** with `node --check`.
- Dashboard generator is deterministic across consecutive runs.
- Readable `dashboard_source.html`: **376,769 bytes** and byte-identical to v3.6c5.
- Generated embedded HTML: **367,482 bytes**.
- Embedded gzip: **77,904 bytes**, down from c5's **81,039 bytes** by **3,135 bytes (3.87%)**.
- Embedded gzip SHA-256: `fd4cf7fa8f79982b70786fa02ec88c6cd5f54767ffa85a2e19e673a66e0f7973`.
- `index_html.h` host compile with a minimal Arduino/PROGMEM stub: **PASS**.
- Project runtime/API fixed-point scan: no active `float` / `double` declarations or `strtod()` / `atof()` calls in the converted paths.
- ESP32-S3 target compile and exact `.bin` delta are **not claimed** because the target toolchain is not installed in this environment.
