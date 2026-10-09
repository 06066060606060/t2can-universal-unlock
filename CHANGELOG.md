# T2CAN Universal v3.28.0

## Continuous AP — 2026-10-10
- Adds default-off Continuous AP with selectable Stalk Down (two pulls), Scroll Click and Scroll Double Click under Settings. The chosen method must match the vehicle's Autopilot activation setting.
- Supports Legacy Model 3/Y stalk input on Body+Chassis and Highland/Juniper scroll input on Body+Chassis or Party+Chassis. Excludes YL and unverified Legacy Party+Chassis stalk routing; no method fallback.
- Observes unmodified Chassis AP/indicator/brake/gear and route-qualified physical controls/steering torque. Requires a fresh AP-active-to-ready transition during a single indicated lane change, then 1,000 ms after signal off; limits an episode to three attempts and 10 seconds.
- Persists one versioned `features/contApCfg` key. Strict GET/POST configuration rejects unsupported ON requests, keeps live state on NVS failure, cancels an old episode on saved settings changes, and reconciles lost HTTP replies with GET without repeated POST.
- Serializes final MCP/TWAI admission, settings publication and cancellation under the existing TX barrier. Stops on physical input, brake, unknown/stale state, fault/non-Drive gear, signal reactivation, route/epoch changes and transport holds; retains stop events across drained RX batches.
- Releases an accepted input through its original adapter with bounded cleanup. Separates Nag rotation and composes shared 0x3C2 from qualified newest idle stock rather than replaying a cached click. Exact echoes remain ambiguous and do not prove vehicle acceptance.
- Preserves the completed v3.27.0 package. Host/browser verification and target build evidence are recorded in VALIDATION.md; physical gesture acceptance, OEM competition and vehicle timing/torque applicability remain unverified.

# T2CAN Universal v3.27.0

## Independent ISA Suppression — 2026-10-10
- Separates ISA profile capability and transport selection from Nag/TSL9, including Standard Party+Chassis.
- ISA uses Party CAN A 0x399 on YL and Chassis CAN B 0x399 on every valid Standard topology, independent of the Legacy Nag Body 0x39B selection.
- Removes the TSL9 Hands-On/CHIME 1 dependency: valid active-AP frames can suppress speed warnings in every Hands-On state. Preserves the saved standalone setting and guarded TX; combines ISA and Nag mutations into one frame only when they share a route.
- Leaves Nag support, Hands-On downgrade windows and Legacy Body 0x39B behavior unchanged; ISA-only frames do not mutate Nag session state or counters.
- Keeps completed v3.26.4 unchanged.

# T2CAN Universal v3.26.4

## Diagnostics BUS OFF display hotfix — 2026-10-09
- Fixes the repeated Diagnostics flicker between a blank age and persisted BUS OFF evidence, which also changed row heights and shifted nearby rows.
- Uses one formatter for BUS OFF age, snapshots and CAN A/B TX trace evidence when system telemetry arrives. Keeps current/previous-boot labels, event uptime, persistence status and errors accurate.
- Preserves existing dashboard styling, polling intervals, recovery controls, APIs, NVS storage and all CAN behavior. Keeps completed v3.26.3 unchanged.

# T2CAN Universal v3.26.3

## Dashboard clarity and compact Interval — 2026-10-09
- Applies the accepted English dashboard review: simpler feature names and brief functional descriptions that remain stable after status refresh.
- Explains that Auto Blinker is inactive while Confirm-Free Lane Change is on. Removes positive restart-persistence annotations while retaining warnings for data/settings that clear after restart.
- Reuses the common field design for the Nag Killer Interval control with a compact label/value row and right-aligned number. Keeps the 1–600-second range and existing update API.
- Preserves all CAN policies, feature settings, NVS schema and API behavior. Keeps completed v3.26.2 unchanged.

# T2CAN Universal v3.26.2

## Factory-reset profile commit hotfix — 2026-10-08
- Fixes profile confirmation after Factory Reset, Reset NVS and a clean first boot on the bundled ESP32 core 3.3.12 / ESP-IDF 5.5.5. The uninstalled TWAI driver returns ESP_ERR_INVALID_ARG (0x102), which previously caused maintenance preparation to reject profile saving before NVS was reached.
- Accepts this missing-driver result only in web-only profile setup/NVS-error mode with no CAN task or supervisor handles and neither controller ready. Running-controller shutdown, barrier ownership, task quiescence, uninstall failures and unknown errors remain fail-closed.
- Keeps prior controller/task observations across maintenance retries so partially initialized states cannot become eligible for the web-only exception after readiness flags are cleared.
- Adds production-function host regressions with realistic SDK error values and setup-to-profile-save/reboot coverage. Keeps completed v3.26.1 and its dashboard unchanged.

# T2CAN Universal v3.26.1

## Initial Profile Setup recovery hotfix — 2026-10-08
- Retries initial profile-status requests after temporary Wi-Fi/HTTP failures and provides a visible recovery action when initialization cannot complete.
- Shows profile-save and initialization errors on the setup screen so a rejected or failed request does not silently leave users in vehicle selection.
- Keeps profile saving disabled when the controller reports unavailable NVS, while preserving the existing fail-closed CAN setup policy and reset recovery paths.
- Requires Check Status after an uncertain save response instead of repeating POST automatically; retains that recovery action when an existing-profile change is cancelled and reopened.
- Prevents overlapping profile-save requests while a selection is being committed or the controller is rebooting. Vehicle/CAN topology selection, confirmation and existing dashboard styling remain intact.
- Limits the patch to dashboard startup/recovery behavior and firmware identity. Profile/reset APIs, NVS storage, CAN routing and feature policies remain unchanged; completed v3.26.0 is preserved.

# T2CAN Universal v3.26.0

## Standalone ISA Suppression — 2026-10-08
- Moves `ISA Suppression` out of Nag Killer into its own Settings toggle, `features/isaSuppress` storage key and `/api/isa-suppression/config` endpoint. The saved selection no longer depends on the Nag master or selected Nag method.
- Keeps the existing supported 0x399/0x39B routes, valid active-AP gate and CHIME 1-only policy. When Nag TSL9 and ISA Suppression are both enabled, both changes compose into one frame with one rolling-counter/checksum update.
- Migrates the previous `nag/tsl9isa` value only when the new standalone key is absent. Failed NVS transactions leave live behavior unchanged and retry migration on a later boot.
- Separates ISA modification/TX diagnostics from Nag session and echo counters, and removes the old ISA field from Nag configuration, update and reset ownership.
- Adds pure transform, persistence, API/static and source/embedded responsive browser regressions. Keeps completed v3.25.1 unchanged.

# T2CAN Universal v3.25.1

## Dashboard long-row layout hotfix — 2026-10-08
- Preserves short label/value rows and gives long diagnostic labels and values their own unbroken line when the available width is insufficient. Exceptionally long values remain fully readable through horizontal scrolling.
- Applies the common row fix throughout dashboard diagnostics and LAB panels; preserves data, controls, CAN behavior and existing visual tokens.

# T2CAN Universal v3.25.0

## LAB Summon Heartbeat Override — 2026-10-08
- Adds a LAB-only, session-scoped override for `0x3F8 UI_driverAssistControl` `UI_summonHeartbeat` bits 2–3. Raw values 0, 1, 2 and 3 are selectable and applied to every valid received stock frame while enabled, including frames whose stock value already matches.
- Extends the existing single 0x3F8 compositor, preserving every unrelated bit and composing independently with Confirm-Free bit1 and the retained ALC/ULC fields. The original RX frame remains untouched.
- Uses the shared TX barrier, receive epoch, 3-second stock freshness, administrative hold, controller-ready checks, cancellation generation and Summon-aware non-R79 queue admission. LAB OFF, CAN full-recovery and reboot disable the override; its toggle and selected value are never written to NVS.
- Adds strict `0|1|2|3` HTTP parsing, live stock/selected/last-applied diagnostics and applied/fail/blocked counters under `/api/lab/summon-heartbeat/*`.
- Adds a matching LAB panel using the existing dashboard controls and responsive layout, plus pure, extracted-runtime, API and source/embedded browser regressions.
- Keeps completed v3.24.0 unchanged. No controller flash, serial/CAN access, live vehicle transmission or road testing was performed.

# T2CAN Universal v3.24.0

## USB full-frame logging and receive-only investigation — 2026-10-08
- Adds default-OFF USB CAN capture on Hardware CDC/JTAG. A bounded 512-frame queue decouples CAN reception from USB writes; shared sequence numbers and software microsecond timestamps preserve enqueue order across A/B, including standard, extended and RTR frames.
- `LOGGER START ACTIVE` retains the existing feature policy while recording RX. `LOGGER START PASSIVE` asks the CAN supervisor to quiesce both tasks and switch MCP2515/TWAI to hardware listen-only; existing saved feature/profile choices remain intact. A single connected bus does not trigger missing-second-bus recovery in PASSIVE.
- Persists only the new `usblog/passive` mode. PASSIVE survives USB unplug, STOP, reset and power cycling; explicit ACTIVE restores normal mode after checked controller initialization. Mode/readback/quiesce failures retain a TX hold and return an error. A legacy direct TSL9 send now shares the TX barrier.
- Reports logger queue counters plus controller epoch, MCP overflow observations and TWAI missed/overrun counters. USB disconnect abandons the old queue/partial command and starts fresh framing on reconnect.
- Adds the macOS collector at `tools/can_usb_logger/`: raw USB bytes, frame/event/controller CSV, metadata and completeness summary. Reported timestamps are software observations, not on-wire arrival; detected gaps/errors are retained and surfaced.
- Keeps completed v3.23.0 and dashboard design unchanged. No flash, serial/CAN device access or vehicle testing is implied.

# T2CAN Universal v3.23.0

## Standalone Driver Monitoring and production Visual Speed Control — 2026-10-08
- Moves `Disable Driver Monitoring` out of Nag Killer into its own Settings toggle, storage key and API. Its runtime policy now depends only on the standalone selection, supported vehicle route and valid active AP state; Nag master, method and torque mode no longer participate.
- Migrates the prior `features/dmsNag43` or `nag/dms43` selection into `features/dmsDisable` on first boot. A failed migration remains safely OFF and retries on a later boot; updates publish to live runtime only after the standalone value is durably saved.
- Promotes `Visual Speed Control` from LAB to Settings, removes `0x3FD` from the feature name and makes its API/runtime independent of the LAB master. Existing supported routes, saved selection, stock-template freshness, active-AP gating, final transport checks and default-OFF behavior remain unchanged.
- Adds canonical `/api/driver-monitoring/config` and `/api/vision-control/*` endpoints. The former LAB Vision endpoint aliases remain registered for compatibility, while the dashboard uses only the canonical production endpoints.
- Adds pure, extracted-runtime, API/static and source/embedded browser regressions for standalone ownership, Nag/LAB independence, persistence and responsive Settings layouts. Technical diagnostics continue identifying the underlying CAN fields without using protocol identifiers as feature names.

# T2CAN Universal v3.22.0

## Retired LAB controls and fixed immediate CAN A receive — 2026-10-07
- Removes Lane Graph selection, storage loaders, stock override, UI, API and feature-exclusive tests. Stock lane data is preserved; surviving Vision Control cache helpers retain equivalent freshness/epoch/profile/topology checks under independent names.
- Removes Parked Injection Test sessions, parked AP bypasses, UI, APIs and dedicated CAN3FD timing capture rings/transport instrumentation. Ordinary AP and final transport gates remain active; normal BUS-OFF diagnostics/persistence/recovery remain intact.
- Fixes CAN A RX to immediate single-frame processing. Removes the 4-frame batch option, saved-mode load/update API and LAB selector; previously saved values cannot reactivate batch processing.
- Removes Blind Spot Injection Monitor UI, API and monitor-only capture fields. Existing Blind Spot injection policy remains unchanged.
- Preserves previous DMS/Nag dependency and Vision Control LAB gating; standalone driver-monitoring disable and Vision promotion/renaming are outside the implemented scope.
- Preserves completed v3.21.4. Updates retained regressions and offline preview; adds removal/fixed-RX and normal-AP gating regressions.

# T2CAN Universal v3.21.4

## Touch dropdown activation hotfix — 2026-10-07
- Keeps native select suppression on pointerdown but opens the custom choice sheet only on completed click. This prevents the opening touch's click from hitting the newly inserted backdrop and immediately dismissing the sheet.
- Supports browsers without PointerEvent by activating on completed single-touch touchend, cancelling moved, cancelled or multi-touch gestures and suppressing the subsequent compatibility click.
- Preserves keyboard activation, independent backdrop/cancel dismissal, existing dashboard design, saved settings and all CAN behavior. Completed v3.21.3 remains unchanged.
- Adds real Chromium touch-pipeline regressions for short/long touches, option selection/change ownership, drag/cancel, disabled controls, mouse/keyboard and legacy fallback. Updates the obsolete native-select guard contract to require completed activation.

# T2CAN Universal v3.21.3

## Parked test activation JSON hotfix — 2026-10-06
- Fixes the parked status API's duplicated opening JSON brace, which prevented the dashboard from decoding state and left every parked-test toggle disabled.
- Adds a regression that compiles the production status serializer with the real JSON writer and parses its output across ready, active, moving and LAB-disabled states.
- Initializes the parked panel's polling clock and clears loading/error text after successful automatic retry; obsolete failed polls cannot overwrite a newer session action. Browser regressions exercise actual menu entry and timer-driven recovery.
- Keeps all CAN injection policies, payloads, timing, masks and vehicle safety gates unchanged. Completed v3.21.2 remains unchanged.

# T2CAN Universal v3.21.2

## Parked 0x3FD injection diagnostic — 2026-10-06
- Adds a volatile LAB parked test session with independent TLSSC/R79/DMS/Lane Graph/Vision selections. Fresh P and zero speed, inactive Summon, valid topology and healthy CAN are required; session expires after ten minutes and is not persisted.
- Preserves actual AP state and existing feature choices. Selection changes, automatic stop, latest invalid gear/speed, pending work, final enqueue and post-send result ownership are protected by cancellation generations and the shared TX barrier.
- Captures CAN B 0x3FD RX-dequeue, driver enqueue request/result and error-alert timestamps in a 256-event microsecond RAM ring. BUS-OFF freezes the ring; manual capture and CSV download are available in LAB.
- Timestamps represent software observation and queue admission, not on-wire start/end or ACK. Parked traffic differs from active AP traffic; non-reproduction in P cannot exclude driving-only timing faults.

# T2CAN Universal v3.21.1

## CAN B BUS-OFF recovery race fix and alert evidence — 2026-10-06
- Prevents synchronous BUS-OFF persistence from making newer CAN task heartbeats appear almost `2^32` ms old. The supervisor re-samples time after persistence and uses rollover-safe half-range age checks before requesting a full CAN reinitialization.
- Captures whether `TX_FAILED`, `ERR_PASS` and `BUS_ERROR` alerts were observed before BUS-OFF, which relevant alerts shared the BUS-OFF read batch, and each alert's last age. These diagnostics do not change CAN IDs, payloads, timing, queues or transmit flags.
- Extends the dual-slot BUS-OFF persistence record to v2 for the new alert evidence while retaining v1 decode compatibility.
- Keeps completed v3.21.0 unchanged. The initial TX-side cause still requires controller/vehicle evidence; generic `BUS_ERROR` cannot distinguish ACK, bit, stuff, CRC or form errors.

# T2CAN Universal v3.21.0

## LAB R79 manual-driving injection — 2026-10-05
- Adds `Allow R79 During Manual Driving` inside LAB `R79 AP Control`, saved across reboots and defaulting OFF. The option is independent of the existing AP policy master.
- ON bypasses only the established manual D/R suppression clause. The manual-driving latch remains observable, while existing AP block/delay, administrative hold, CAN readiness, valid-stock, freshness, generation, cancellation and queue-admission protections remain active.
- Applies the same policy to cached country/R79 authorization, runtime status, receive-synchronized immediate sends, periodic sends and retries. A successful setting change updates authorization immediately and invalidates work prepared under the previous policy.
- Stores the new option in the previously unused `r79/apctl` bit2. Existing saved values load with the option OFF; failed mutex, NVS-open or NVS-write operations leave live behavior unchanged.
- Keeps completed v3.20.0 unchanged. Local validation and OTA evidence are recorded in `VALIDATION.md`; no physical controller or vehicle behavior is implied.

# T2CAN Universal v3.20.0

## Retired 0x3F8 speed experiments — 2026-10-05
- Removes the failed LAB `Vision Speed Control` (`UI_visionSpeedType`, bits20–21) and `Adaptive Set Speed` (`UI_adaptiveSetSpeedEnable`, bit39) experiments from the dashboard, API, runtime and compositor. Those stock fields are no longer overridden by these experiments.
- Retires their saved `lab3f8/visionDisabled` and `lab3f8/adaptiveOff` keys without clearing the shared namespace or other saved settings. Failed NVS cleanup cannot reactivate the removed functions.
- Keeps `Vision Speed Control — 0x3FD` (`UI_enableVisionSpeedControl`, MUX1 bit49), its independent saved settings and vehicle-specific routes unchanged.
- Preserves existing ALC/ULC, R79, Lane Graph and final transport-admission policies. Completed source and OTA packages remain unchanged; only the new package removes these experiments.
- Marks the two earlier experiments as retired after reported vehicle ineffectiveness. Host success in their historical validation entries was never proof of vehicle acceptance.

# T2CAN Universal v3.19.0

## LAB Vision Speed Control — 0x3FD — 2026-10-05
- Adds an independent saved default-OFF experiment requesting `UI_enableVisionSpeedControl=0` in stock `0x3FD` MUX1 bit49.
- Uses fixed VH on Model YL; Chassis by default on Juniper, Y Legacy, 3 Highland and 3 Legacy, with Body selectable only on Body + Chassis topology. Received stock and transmission stay on the selected physical bus.
- Requires LAB enabled, fresh active AP and fresh valid stock. Other fields retain their existing policies; acceptance and sign-specific slowing effects are unverified.
- Signal-coordinate reference: `dzid26/ESP32-DualCAN` commit `2b4d7404ccbb984d57150e9f12421c30e1327bec`, `dbc/Model3_CH.dbc:6252` and `dbc/Model3_VEH.dbc:12667` (`m1 : 49|1@1+`). These community definitions do not establish support on every Tesla software version or a sign-only behavior.
- Completed v3.18.0 is preserved. Validation and local OTA evidence follow in VALIDATION.md.

# T2CAN Universal v3.18.0

## LAB Adaptive Set Speed — 2026-10-05
- Adds a saved default-OFF LAB experiment requesting `UI_adaptiveSetSpeedEnable=0` in live CAN B `0x3F8` bit39 across all supported vehicle profiles and valid topologies.
- Independent of the existing Vision Speed option; applies only with LAB enabled and fresh active AP state. Uses same-bus received stock and the existing single compositor, preserving unrelated fields and speed/acceleration/AEB commands.
- Actual vehicle acceptance, sign-related slowing and phantom-braking reduction are unverified. This is an experimental request, not a verified sign-only bypass.
- Preserves completed v3.17.0. Local validation and OTA evidence are recorded in VALIDATION.md.

# T2CAN Universal v3.17.0

## LAB Vision Speed Control — 2026-10-05
- Adds a saved default-OFF LAB experiment to request UI_visionSpeedType=0 in live CAN B 0x3F8 UI_driverAssistControl frames across all valid Universal vehicle profiles and topologies.
- Applies only with LAB enabled and fresh active AP state, using same-bus received stock and the existing single compositor. Other 0x3F8 fields and DAS speed/acceleration commands retain their existing policies.
- The DBC identifies raw 0 as DISABLED; a sign-only interpretation and actual vehicle acceptance are not established. This option does not promise to remove phantom braking.
- Preserves completed v3.16.0. Local validation and OTA evidence are recorded in VALIDATION.md.

# T2CAN Universal v3.16.0

## LAB Lane Graph injection bus — 2026-10-05
- Adds a saved Injection Bus selector for non-YL Lane Graph: Chassis (default) or Body. Body is available only with a valid Body + Chassis topology; Party + Chassis cannot select Body. YL keeps its VH route and hides the selector.
- Preserves the existing OFF / During AP / Always policy independently of the selected bus. Existing saved modes load with Chassis as the default route.
- Uses stock 0x3FD MUX1 frames from the selected bus and transmits on that same bus. There is no cross-bus template fallback. A saved Body selection on an unsupported non-YL topology remains blocked; switching to YL normalizes the route to fixed VH.
- Body and independent Chassis display sends reject stock older than 3,000 ms at final admission, using timestamps captured immediately after reception.
- Displays selected-bus receive state and age, including waiting for stock. Route changes invalidate pending display work; existing R79 behavior and manual D/R policy remain unchanged.
- v3.15.0 is preserved. Local validation and OTA evidence are recorded in VALIDATION.md; actual Body-bus visualization remains unverified.

# T2CAN Universal v3.15.0

## Independent map region and LAB Lane Graph — 2026-10-04
- Separates Country and Map Region selections in the existing Settings panel. Country retains STOCK/US/KOREA/NEW ZEALAND; Map Region offers STOCK/US/KOREA. Existing saved presets migrate to equivalent country/map combinations. Japan is omitted because its map-region encoding is unverified.
- Adds saved LAB Lane Graph modes OFF (default), AP ACTIVE, and ALWAYS. OFF preserves stock; AP ACTIVE applies the display flag only with fresh active AP state. Existing transport and maintenance protections remain in effect.
- Preserves the R79 manual D/R blocking policy. No manual-drive bypass is added.
- v3.14.0 is preserved. Host, browser and target-build evidence is recorded in VALIDATION.md; no physical vehicle or visualization outcome is implied.

# T2CAN Universal v3.14.0

## Legacy V12/V13 HW3 R79 option — 2026-10-04
- Adds a saved default-OFF V12/V13 HW3 toggle to Settings → R79 Mode, visible only for Model Y Legacy and Model 3 Legacy with either supported CAN topology.
- When selected on a supported Legacy profile, both R79 modes preserve the received MUX1 bit47 instead of forcing it to 1, including reinjection and retry paths. Existing bit19, bit18, DMS, authorization and timing policies remain in effect.
- Setting publication follows successful NVS persistence and invalidates pending R79 work at the TX admission barrier. Unsupported profiles cannot activate the override.
- v3.13.0 is preserved. Local test/build evidence is recorded in VALIDATION.md; no device or vehicle outcome is implied.

# T2CAN Universal v3.13.0

## NAG settings and Country promotion — 2026-10-04
- Adds saved torque-only Ignore AP State, default OFF, for A/B/C/H. TSL9, scroll, and DMS keep their original AP conditions. Other torque eligibility, speed, transport and maintenance gates remain in force. When ON, torque processing may continue after AP disengagement.
- Missing NAG enabled NVS values now default OFF; existing values are preserved. Mode changes preserve enabled and Ignore AP State.
- Mode H uses the established Rev.4 engine only. Previous revision selections normalize to persisted slot 1, and h4 tuning is retained. Shared pure algorithm helpers remain for the engine.
- Country / Map Region moves to Settings and operates independently of LAB. Country choices, stored values, R79 authorization, frame validity and CAN routing remain unchanged.
- v3.12.0 is preserved. Local validation and OTA evidence are recorded in VALIDATION.md; no device or vehicle validation is implied.

# T2CAN Universal v3.12.0

## New Zealand country preset — 2026-10-04
- Adds saved LAB Country / Map Region preset NEW ZEALAND (mode 3), preserving existing STOCK/US/KOREA numeric identities and defaults.
- Sets UI_countryCode (0x238) to ISO numeric 554 and GTW_country (0x7FF MUX1) to NZ using the established little-endian encoding. Existing route, fresh-stock, LAB, R79 and maintenance gates remain in effect.
- Preserves the complete GTW MUX3 map-region page because no NZ-specific encoding is verified. Preserves right-hand-drive and all other GTW MUX1 fields. The dashboard explicitly describes this country-only behavior.
- NZ/554 reference: https://www.ncei.noaa.gov/archive/archive-management-system/OAS/bin/prd/jquery/country/details/163 ; field locations follow research/can-logs/Model3CAN.dbc.
- No FSD enablement, R79 exemption or on-vehicle outcome is established by this preset. Existing v3.11.1 remains unchanged.

# T2CAN Universal v3.11.1

## OTA/reboot CAN shutdown and reconnect backdrop — 2026-10-04
- OTA now requires a confirmed maintenance shutdown before starting flash writes. The sticky TX hold, supervisor handshake and CAN-task quiescence prevent controller recovery or settings changes from reopening communication.
- CAN A is held in hardware reset; CAN B is stopped/uninstalled to discard queued traffic. Failed or aborted OTA leaves CAN stopped until reboot; the dashboard explains this state.
- Software reboot paths use the same shutdown, including reset/profile handlers, BLE reset, supervisor failure and startup failure. Stop failure refuses OTA and does not fall through to an uncoordinated reboot.
- Reconnect/OTA overlays override AP/NOA page backgrounds, matching html/body safe-area and browser theme color in light and dark themes.
- Existing v3.11.0 remains unchanged. No device or vehicle validation is implied.

# T2CAN Universal v3.11.0

## LAB R79 AP Control — 2026-10-04
- Adds a saved opt-in master (default OFF). OFF retains the existing R79 authorization policy, payload transforms, mode selection, and configured enqueue wait.
- ON offers Block during AP or Delay after AP for 2–10 whole seconds (default 2). The timer restarts on AP re-engagement, settings changes, DAS freshness loss, and relevant CAN recovery. AP states 3–6 share a continuous session.
- The opt-in gate fails closed on unknown/stale DAS state. Immediate, retry, and periodic R79 sends recheck authorization and cancellation generation under the shared TX barrier before enqueue.
- Existing LAB controls/styles show AP blocked, waiting with remaining seconds, and unknown state. R79 AP control stays configured when the LAB navigation menu is hidden; use its own master to restore the original policy.
- This controls new R79 enqueue admission. Frames already accepted into the hardware/driver queue are not selectively retractable; no global TX queue flush is added for AP transitions.
- Preserves completed v3.10.2. No device or vehicle validation is implied.

# T2CAN Universal v3.10.2

## Periodic Interval alignment — 2026-10-04
- Balance the existing field card with its label on the left and editable value on the right, vertically centered on one row.
- Preserve input bounds, saving, and CAN behavior.

# T2CAN Universal v3.10.1

## Periodic Interval dashboard input styling — 2026-10-04
- Replaced the unstyled browser-native TSL9 **Periodic Interval** number control with the same full-width field-card component used by Auto Blinker timing inputs.
- Preserved the existing control ID, 1–600 second bounds, default, and save behavior; CAN scheduling and runtime policy are unchanged.
- Advanced the firmware and package identity to `v3.10.1`; released `v3.10.0` remains immutable.

# T2CAN Universal v3.10.0

## Universal LAB Country / Map Region — 2026-10-03
- Based on v3.9.3, with the separate v3.9.2 country experiment ported into LAB.
- Adds STOCK, US, and KOREA selections across valid Universal vehicle profiles and CAN topologies.
- Applies country transforms only to received route-local stock frames, preserving unrelated fields and the existing R79 authorization and cancellation barriers.
- Vehicle acceptance remains unverified; host and target-build evidence is recorded separately.

# T2CAN Universal v3.9.3

## R79 and NAG right-scroll controls — 2026-10-03
- Added a saved Mode 1 Post-MUX2 reinjection toggle while preserving the existing Mode 1 wait and delay controls. Disabling it cancels pending periodic work through the transport generation barrier.
- Added Torque-mode **Auto Right Scroll**, shown directly below NAG Method. It supports configurable 1–600 second AP-active periodic injection, immediate injection on a new torque warning, and selectable `+1 → 0 → -1 → 0` or `+1 → -1` waveforms.
- Added optional periodic injection to TSL9 **Right Speed** only. TSL9 Left Volume remains warning-only and keeps its existing randomized warning repeat behavior.
- Unified Torque and TSL9 right-scroll output under the timer-owned route scheduler so 100 ms steps, physical-input priority, CENTER cleanup, configuration quiesce, and CAN recovery cannot overlap across producers.
- Advanced the firmware and package identity to `v3.9.3`; `v3.9.2` remains an immutable prior release.

# T2CAN Universal v3.9.2

## AP profile naming and failed experiment removal — 2026-10-02
- Renamed the existing Settings feature to **AP Pedal / Regen Profile** while retaining its established API and NVS keys.
- Removed the failed LAB driving-aggressiveness experiment completely, including its `0x398` detection, `0x3FD` overlays, CAN trace source, API, NVS reads/writes, dashboard controls, helper, and dedicated tests.
- Existing **AP Pedal / Regen Profile**, TLSSC, and other `0x3FD` behavior remain independent and supported.

# T2CAN Universal v3.9.1.4

## Stable two-mode CAN Research Capture — 2026-10-02
- Reduced the CAN Research Capture selector to **SNAPSHOT** and **RAW TRANSITION**. The former AUTO ALC and ULC / CONFIRM-FREE choices are no longer accepted by the mode API or shown in LAB.
- Fixed RAW TRANSITION silently reverting to SNAPSHOT. Both supported modes now share the same fixed PSRAM allocation, avoiding the previous multi-megabyte free/reallocate cycle and its fragmentation/allocation failure path.
- RAW archive capacity is 163,840 physical CAN A/B RX frames inside the fixed 2.5 MiB main block; the rolling RAW PRE ring remains 32,768 frames. Capture remains RX-only and never transmits CAN.
- A rejected mode change now displays an explicit browser alert before refreshing authoritative device state instead of silently resetting the selector.

# T2CAN Universal v3.9.1.3

## Model YL driver-window Auto Down field correction — 2026-10-02
- Replaced the unverified DBC bit-35 overlay with the field observed during a physical Model YL Auto Down second-detent capture: CAN B `0x3C2` MUX0 byte 6 bits `[3:2]` change from neutral value `1` (`0x15`) to Auto Down value `2` (`0x19`).
- The transform replaces only that two-bit field and preserves every other stock bit. Physical Auto Down input in the same field now blocks the LAB request instead of being misclassified as idle.
- Existing Model YL Party+VH, PARK, freshness, CAN epoch, LAB, timeout, duplicate-request, generation-barrier, and exactly-two-stock-synchronized-frame policies are unchanged.

# T2CAN Universal v3.9.1.2

## Model YL driver-window LAB Auto Down test — 2026-10-02
- Fixed the LAB availability gate to recognize Model YL's actual `PARTY + VH` topology directly. The earlier `activeCanBIsChassis()` check could never succeed for YL because its CAN-B route is named VH, not CHASSIS.
- Added a Model YL-only **LAB → Driver Window Test** action. A confirmed tap arms a RAM-only request; the web task never transmits CAN directly.
- The CAN B task consumes fresh stock `0x3C2` MUX0 frames, clones the complete eight-byte payload, changes only `VCLEFT_btnWindowSwPackAutoDownLF` at DBC bit 35, and submits exactly two stock-synchronized overlays before returning to untouched stock traffic.
- The action fails closed unless LAB is enabled, the active profile is Model YL Party+VH, a fresh real gear source reports PARK, the stock template is at most 250 ms old, CAN B is available in the current recovery epoch, no physical window-switch field is active, and no earlier request is pending. These gates are checked again for each transmitted frame; cached physical input also blocks arming.
- Added LAB diagnostics for availability, pending state, stock/TX raw frames, request/completion/block counts, TX results, and explicit latest-request results. A CAN-task service tick clears stale/expired work without waiting for another frame or browser poll. LAB disable advances a feature-local cancellation generation under the common TX barrier, so an earlier prepared overlay cannot enqueue after cancellation completes.

## CAN task heartbeat and CAN A overflow diagnostic build — 2026-10-02
- Added timeout snapshots for both CAN tasks: FreeRTOS state, last execution stage and stage age, heartbeat count and maximum gap, loop count and last/maximum loop duration, and stack high-water mark.
- Added CAN A receive-pressure counters: total frames processed, maximum frames per loop, 32-frame budget exhaustion count, separate RX0/RX1 overflow observations, and receive totals captured at the last overflow observation.
- Exposed the new evidence through the existing **System Stats JSON** download and included it in **Reset Stats**. CAN routing, transmit policy, task priorities, RX budgets, heartbeat thresholds, and recovery decisions are unchanged.

# T2CAN Universal v3.9.1

## DMS bit43 AP gate — 2026-10-02
- **Disable Driver Monitoring** now forces `0x3FD` MUX1 bit43 low only while a valid DAS state reports AP active (`3..6`). AP-inactive and AP-invalid states preserve the stock bit43 value.
- The gate covers both DMS-only fallback transmissions and the final DMS overlay applied to R79 Mode 1/2, retry, Post-MUX2, and ULC clone frames. R79 ownership and transmission policy are unchanged; only the optional DMS bit43 mutation is AP-gated.
- TSL9 Hands-On, ISA suppression, warning-only input assistance, and owned CENTER cleanup behavior are unchanged.

# T2CAN Universal v3.9.0

## NAG KILL integration work — 2026-10-02
- Replaced the independent AP Right Scroll feature with TSL9-warning-only input assistance. The default left volume wheel and selectable right speed wheel reproduce the V8.2 `+1 → 0 → -1 → 0` waveform at 100 ms steps, repeat at a randomized 2.0–3.0 seconds, defer to physical input, and never run during ordinary driving.
- Added an owned retry/cleanup state. A nonzero generated command cannot be discarded on warning clear, AP exit, CAN recovery, send failure, or configuration change: CENTER waits for a fresh route-local MUX1 template and 300 ms of physical-input quiet, retries failed sends at 250 ms, and blocks configuration commit until quiesce completes.
- Added a saved TSL9 ISA-chime suppression toggle and composed it with Hands-On transformation so the rolling counter and checksum are rebuilt exactly once for the selected `0x399` or `0x39B` CAN ID.
- Promoted DMS bit43 control from LAB into NAG KILL. It applies to TSL9 and Torque modes A/B/C/H, while R79 remains the sole priority owner: DMS is the final overlay on Mode 1, retry, Post-MUX2, Mode 2 immediate/delayed, and ULC MUX1 clones. DMS-only fallback is zero-wait/no-retry and is admitted only when R79 neither claims the stock generation nor has pending work.
- Added a mutually exclusive Legacy Model 3/Y TSL9 route selector. Fresh/reset configuration defaults to Body CAN A `0x39B`; Chassis CAN B `0x399` is selectable, and Chassis `0x399` AP/R79 observation remains active in either selection.
- Added **Reset NVS · Keep S3XY & BLE**. It clears the explicit application/profile/Wi-Fi/diagnostic/schema namespace allowlist while preserving S3XY registry/mappings and BLE bonds. A durable guard is written before the first clear, so an interrupted reset resumes selective cleanup at boot instead of entering the first-Universal full-partition erase path.
- Bumped the NAG schema to 19, migrated an explicit prior `features/dmsNag43` selection into the production NAG key, added API/dashboard diagnostics, and retained full factory reset plus the existing settings-only reset as separate choices.

# T2CAN Universal v3.8.4

## Legacy Model 3/Y TSL9 Body 0x39B test route — 2026-10-02
- Model Y Legacy and Model 3 Legacy/HW3 with Body + Chassis now receive, transform, and transmit the gateway-translated DAS status on Body CAN A ID `0x39B`.
- The TSL9 checksum now includes the actual CAN ID, producing the required `0x39B` checksum while preserving the existing `0x399` result.
- Model YL remains on Party CAN A `0x399`. Model Y Juniper and Model 3 Highland remain on Chassis CAN B `0x399`. Party + Chassis support policy is unchanged.
- Chassis `0x399` remains available to the existing AP/R79 state handler on Legacy profiles but no longer triggers a TSL9 transmission there. The dashboard reports `Body CAN A · 0x39B` only for the two Legacy profiles.

## Diagnostics System Stats JSON download — 2026-10-01
- Added a full-width **System Stats JSON** action under **Settings → Diagnostics → CAN B Recovery**, above the existing two-column recovery and trace controls.
- The action downloads the existing `/api/system/stats` response as `T2CAN_SYSTEM_STATS.json` through the dashboard's standard download progress flow.
- No diagnostic values, reset/recovery behavior, CAN routing, or CAN transmission behavior changed.

# T2CAN Universal v3.8.3

## Embedded dashboard build budget — 2026-10-01
- Raised the strict embedded-dashboard gzip ceiling from **71,350 bytes** to **100,000 bytes** so routine dashboard updates no longer require byte-level optimization at every change.
- Kept the ceiling exclusive: **99,999 bytes is accepted and 100,000 bytes is rejected**.
- Made both binary-size regression checks consume the build tool's canonical limit instead of maintaining separate 71,350-byte and 84,000-byte values.

## R79 Home state and Mode 1 default bugfix — 2026-10-01
- Fixed the Home and LAB R79 state label treating every active transport state as Autopilot. The label now follows the runtime reason: `AUTOPILOT` shows **AP engaged**, `SUMMON` shows **Summon**, an active default state in gear P shows **Park standby**, and other active default states show **Ready**.
- Kept the established R79 transport policy unchanged. Fresh/default configuration remains **Mode 1**, bit18 **STOCK**, MUX1 **2 ms WAIT**, and Post-MUX2 **150 ms**.
- Made the dashboard's pre-API Mode 1 wait selection match the firmware default, preventing a user action before the first status refresh from submitting the former 0 ms HTML fallback.

## Selectable TSL9 Hands-On downgrade window — 2026-09-30
- Added a saved **Entire AP session** toggle under the TSL9 settings. OFF preserves the existing first-12-seconds behavior; ON keeps the Hands-On downgrade active for the full continuous AP state 3–6 session.
- The window choice is independent from **TSL9 Sequence**: V8.2 Original still changes only 4→1, while Extended still changes 2/3/4→1. AP exit and unsupported/inactive states continue to pass stock 0x399 through unchanged.
- The choice is stored as `tsl9win`, exposed as `tsl9Window` in the config/status APIs, and applied on both the Model YL MCP2515 route and supported Standard Chassis TWAI route. Existing installations default to the original 12-second window.
- Tightened generated-dashboard HTML attribute minification without rewriting script/style bodies or self-closing tag endings, keeping the embedded dashboard inside its existing gzip ceiling.

## Standard Body + Chassis AP Right Scroll CAN A test build — 2026-09-30
- Model Y Juniper, Model Y Legacy, Model 3 Highland, and Model 3 Legacy now receive stock 0x3C2 MUX1 and send generated AP Right Scroll frames on Body CAN A when configured Body + Chassis. Their status API displays BODY.
- Model YL and Standard Party + Chassis retain their existing CAN B routes. TSL9 0x399 routing and the Party + Chassis TSL9 option policy are unchanged.
- This expands the previous Legacy Y-only Body route. The 2026 Juniper's actual 0x3C2 Body traffic and vehicle acceptance still require a raw both-bus capture; this is a test build.

## Party + Chassis TSL9 option removal — 2026-09-30
- Standard Model 3/Y Party + Chassis profiles now expose Torque NAG only. TSL9 is hidden from the NAG method selector, rejected by the method API, and blocked by the existing runtime profile gate.
- A saved TSL9 choice on this topology resolves to Torque with NAG disabled during configuration loading. YL Party + VH and Standard Body + Chassis retain TSL9 support.
- The prior Legacy Model Y Body CAN AP Right Scroll test change remains included.

## Legacy Model Y AP Right Scroll Body CAN test build — 2026-09-30
- For Legacy Model Y with Body + Chassis topology, AP Right Scroll now reads stock 0x3C2 MUX1 from Body CAN A and sends its generated scroll frame on Body CAN A. Model YL and other profiles keep their existing CAN B route; TSL9 0x399 stays on Chassis CAN B.
- The dashboard and status API show BODY for this route. The feature's saved enable state remains independent from the Nag Killer switch and still defaults OFF.
- This is a source and OTA test build. The available 2024 capture has no AP-active 0x399 samples and cannot establish whether the generated Body frame is accepted by the vehicle. Body 0x3C2 MUX1 timing in the decoded capture is sparse, so the four-step waveform timing also needs an on-vehicle capture.

## Mobile dashboard stability, Nag layout, and defaults
- Locked the dashboard's web viewport against pinch/double-tap page zoom on iPhone and Android while preserving normal vertical scrolling and native system accessibility magnification.
- Fixed the transient white right-side strip, bottom white line, and header/status shift seen when opening a longer page. The old page-slide transform temporarily made a 390 px viewport 402 px wide and exposed the browser's native horizontal/vertical scroll indicators. Main-menu slide/fade motion is restored inside an app-level horizontal clip, root scrolling remains instant, native scrollbars stay hidden, and the root background covers the dynamic viewport.
- Moved **AP Right Scroll** directly below **Pause at 0 km/h** in Nag Killer. **Advanced Parameters** now keeps its gray description on a separate line, and AP Right Scroll numeric fields use the dashboard card styling with right-aligned values.
- Fresh/reset/invalid configuration now defaults **R79 bit18** to **STOCK** and **TSL9 Sequence** to **Extended**. Existing valid saved choices remain unchanged.
- TSL9 hides the torque-only Mode A/B/C/H, Mode H Tuning, and Advanced Parameters content. Its Home signal now displays the decoded 0x399 Hands-On value under the exact label **Hands-On state**; Torque mode retains **Stock torque Nm**.

## R79 Mode 1 selectable MUX1 wait and Post-MUX2 timing
- Restored the Mode 1 initial-enqueue A/B choice as **FAST ECHO · 0 ms** and **2 ms WAIT**. Existing and fresh installations default to **2 ms WAIT**, preserving the v3.8.2 behavior unless the user changes it.
- Added a separately saved Mode 1 **POST-MUX2 DELAY** setting. It defaults to 150 ms and accepts strict decimal values from 0–340 ms; empty, signed, non-numeric, and out-of-range inputs are rejected. Mode 2 keeps its independent reinjection toggle and 0–2000 ms delay.
- Mode 1 now budgets each initial enqueue and queue-flush retry against the absolute quiet-window deadline, so the 340 ms edge setting cannot start or extend a transmit attempt past the existing hard safety boundary.
- Mode 1 payload policy, manual D/R suppression, authorization, queue-flush recovery, bounded retry, bit18 selection, bit19=0, bit47=1, and DMS/NAG LAB overlay are unchanged. The wait selector changes only the initial MUX1 enqueue attempt.
- Added API/status fields and NVS keys for both Mode 1 timing choices, reset pending R79 timing state when settings change, and exposed the controls only while Mode 1 is selected.
- Tightened embedded-dashboard generation by removing standalone JavaScript comments and whitespace between adjacent HTML tags from the generated payload only; the editable dashboard source remains readable.

## Mobile custom-select native-picker guard
- Fixed the mobile handoff where closing the custom dashboard select sheet synchronously focused its backing native `<select>` and could immediately open the phone's native picker.
- Closing by option, Cancel, backdrop, or Escape now blurs the active custom-sheet control and never focuses or programmatically clicks the native select.
- Select activation uses one Pointer Events press path plus a click default-action guard. Browsers without Pointer Events use `touchstart` plus the same click guard, removing the previous simultaneous pointer/mouse/touch listener stack.
- Native selects remain the saved value and `change` event source, so the existing UI design, API/NVS handlers, toggles, buttons, keyboard selection, and OTA file picker are unchanged.

## Settings navigation — AP Right Scroll
- Moved **AP Right Scroll** out of the top-level Settings > Features list and into **Settings > Nag Killer**.
- The existing AP Right Scroll controls, API routes, persistence, TSL9-aware sequence behavior, and CAN runtime are unchanged.
- The AP Right Scroll back button now returns to Nag Killer instead of the Settings root.

## Universal Body+Chassis NAG method recovery
- Fixed Standard Model 3/Y **Body + Chassis** profiles booting with the persisted/default **Torque** NAG method even though that topology has no Party-CAN torque route.
- An unavailable persisted method is now resolved against active-profile capabilities. Body+Chassis falls back to **TSL9 · 0x399** on Chassis CAN B.
- A method fallback caused by topology migration is fail-safe: NAG is set **OFF** instead of automatically arming the newly selected method. An already-valid saved TSL9 selection keeps its saved enable state.
- NAG Reset follows the same topology-aware policy, and the update path normalizes stale method state before applying an explicit user enable/disable request.
- The dashboard mirrors the effective capability: Body+Chassis shows TSL9 as the required method, leaves the NAG enable toggle usable, exposes TSL9 Sequence, and keeps torque-only controls disabled/hidden.

## LAB Driving State layout
- Fixed the LAB Driving State header where **Active · AP engaged** could overlap a long **Gear UNKNOWN** label on Android/mobile widths.
- Unknown gear now renders as **N/A**. The gear column is width-bounded and the driving-state value is clipped responsively so the two fields cannot paint over each other.

## iPhone home-screen icon
- Added the approved TU artwork as a local 180×180 opaque PNG at `/apple-touch-icon.png`, embedded in flash (26,699 bytes).
- Safari home-screen shortcuts use the icon and the title **Tesla Unlock**. No internet connection or additional filesystem upload is needed.
- Existing dashboard navigation, Android support, and CAN behavior are unchanged. No service worker, offline dashboard, or standalone-mode change is included.
- Rebuild assets with `python3 tools/build_dashboard_icon.py` and `python3 tools/build_dashboard.py` (install `tools/requirements-dashboard.txt` to reproduce release compression).
- After updating firmware, connect to its Wi-Fi and open `http://192.168.4.1` in Safari. Choose Share → Add to Home Screen. Delete and re-add an old shortcut if it retains the previous icon.

## AP Right Scroll restoration and V8.2 TSL9 waveform
- Restored the independent **Settings > AP Right Scroll** menu. Its runtime gate is again feature enabled + supported CAN-B route + **AP active** + transmitter ready; it does not require Mode H or Nag Killer to be enabled.
- Outside active TSL9 NAG, the original stock-following UP → DOWN behavior and the narrower visual-warning states **3–5** remain unchanged.
- While TSL9 NAG is active, the same saved feature switches to a separate V8.2 right-speed sequence: **+1 → 0 → -1 → 0**, with 100 ms between accepted stock-derived transmissions. The two zero stages also reproduce the V8.2 Byte 6 bit 4 center flag.
- TSL9 warning assistance uses Hands-On states **3–6 and 9–10**. Both paths remain CAN-B-only, yield to physical right-scroll input, and reset on AP exit, CAN recovery, administrative hold, feature disable, or send failure.

## TSL9 V8.2 sequence and Body + Chassis support
- Added a saved **TSL9 Sequence** selector that appears when the Nag Killer method is TSL9. **V8.2 Original** reproduces the reference behavior exactly: AP states 3–6, the first 12 seconds of the AP session, Hands-On state 4→1 only, rolling-counter increment, and checksum rebuild. **Extended** retains the current 2/3/4→1 behavior.
- Exposed Nag Killer on every valid Standard Body + Chassis profile. The TSL9 choices use Chassis CAN B; the unsupported steering-torque choice is disabled and its CAN-A injection path remains blocked. Party + Chassis and Model YL retain their existing routes.
- Shortened the Home unknown-AP title to **AP Unknown** and the LAB R79 no-template state to **Waiting**, preventing the two values from colliding with adjacent mobile-card content.
- Removed unused hidden Home R79 fields and superseded Home card CSS while keeping the visible state, reason, TX totals, and last-TX age. The embedded dashboard remains within the 71,250-byte optimized gzip contract.

## Separate R79 Mode settings menu
- Moved the R79 Mode 1 / Mode 2 selector, bit18 policy, and optional Mode 2 MUX2 reinjection timing from Summon Monitor into a dedicated **Settings > R79 Mode** panel.
- Summon Monitor is now a read-only session, gear, route, source, and queue monitor. Its panel no longer owns or refreshes the R79 configuration controls.
- Split panel polling so Summon Monitor requests Summon status only, while R79 Mode requests R79 status only. The transmit policies, persistence keys, API routes, and manual D/R suppression behavior are unchanged.

## Hidden Home Live details removal
- Removed the permanently hidden Home Live details card, its eight DOM values, toggle handler, renderer, and writes from the NAG, TSL9, Auto Blinker, Summon, and S3XY refresh paths.
- Home polling no longer sends `live=1`. Removed the corresponding `homeLiveSnapshotToJson()` builder and optional `live` response while preserving the documented `home-lite` path for cached legacy dashboards.
- Rebuilt the dashboard to **71,219 bytes gzip**, down from **71,887 bytes**. The OTA application image is **1,450,320 bytes**, down by another **1,280 bytes**, while global RAM remains **71,440 bytes**.

## Asset and no-op UI cleanup
- Subset the embedded Geist and Geist Mono WOFF2 fonts to printable ASCII plus the symbols used by the current dashboard. The two font payloads total **34,956 bytes**, down from **52,396 bytes**, without changing the font families or CSS routes.
- Removed 135 calls to the intentionally empty dashboard `toast()` function and removed the empty function itself. Visible control state, progress, confirmation sheets, and reconnect screens remain the dashboard's feedback paths.
- Removed 67 CSS rules whose class names have no production markup or JavaScript reference. The readable JavaScript source remains unminified for maintainability.
- Rebuilt the dashboard to **71,887 bytes gzip**, down from **74,320 bytes**. The final OTA application image is **1,451,600 bytes**, down by **20,000 bytes**, while global RAM remains **71,440 bytes**.

## Conservative binary optimization
- Disabled C++ exceptions for the complete Arduino target with `build_opt.h`. The firmware does not use exception handling, and no feature, font, Wi-Fi, BLE, CAN, or dashboard code was removed.
- Rebuilt the embedded dashboard with deterministic Zopfli gzip compression. Its standard gzip payload is **74,320 bytes**, down from **77,084 bytes**, and is still served by the existing firmware path without runtime decompression changes.
- The final OTA application image is **1,471,600 bytes**, down from **1,542,560 bytes** in the supplied build. `-fno-use-cxa-atexit` was deliberately excluded from the production build because its additional saving was small and it changes static-object teardown semantics.

## TSL9 NAG and selectable R79 Mode 2
- Added a saved Nag Killer method selector. **Torque · existing** keeps the current implementation; **TSL9 · 0x399 Hands-On** uses the V8.2-style AP-session window, changes Hands-On states 2/3/4 to 1, advances the rolling counter, and rebuilds the checksum. The two methods never transmit together.
- Model YL runs TSL9 on Party CAN A (MCP2515). Standard Party + Chassis profiles run it on Chassis CAN B (TWAI); profiles where Nag Killer is already unsupported remain unchanged.
- Added saved R79 **Mode 1** and **Mode 2** choices. Mode 2 performs one zero-wait clone immediately after each stock 0x3FD MUX1, preserves bit18, clears bit19, sets bit47, and does not use version detection, queue flushing, or scheduled retries.
- Mode 2 can optionally schedule one MUX1-template reinjection after a stock MUX2. The delay is user-selectable from 0 to 2000 ms and defaults to 150 ms; the option is off by default. The existing manual D/R suppression policy applies to immediate and delayed Mode 2 attempts.

## Final v3.8 firmware package
- Built the approved dashboard and vehicle code as a fresh ESP32-S3 OTA application image, `T2CAN-Universal-v3.8-LP_YL-OTA.bin`.
- The final build retains the v3.8 identity, Mode H Rev.4 fresh-install NAG default, persistent 4 FRAMES / IMMEDIATE CAN A RX choice, and the current dashboard source. The offline AP/model simulator remains preview-only.
- Refreshed the source archive and checksums alongside the OTA file. No vehicle flash or road validation is implied by this build.

## Wireless reset card and floating navigation clearance
- Increased the Reset Bluetooth Data row's vertical and horizontal padding so its wrapped description has space inside the rounded card.
- Increased bottom scroll padding for all detail panels and the pairing wizard. The floating navigation begins 86 px above the viewport bottom, while panel content now has 120 px of bottom padding plus the phone safe area.
- Regenerated the offline preview, embedded dashboard, and OTA application image.

## Wireless overview row balance
- Removed the one-off padding on Bluetooth Master. All four Wireless overview rows now share the same minimum height, vertical padding, and 18 px horizontal inset.
- The row separators follow that inset rather than spanning the full rounded card. Bluetooth Master and S3XY Auto Connect descriptions are short enough to keep the two-line title/description layout consistent at normal phone widths.
- Rebuilt the embedded dashboard, offline simulator preview, and OTA application image.

## Wireless overview and choice-button polish
- Shortened the Bluetooth Master description to "Applies instantly. Saved pairings remain." and added top space inside the first rounded Wireless card row.
- Removed the background and border from unselected Appearance and Dashboard Polling Rate choices; only the selected choice has a filled pill.
- The offline simulator now returns a Wi-Fi SSID and the BLE initialized state, so its Wireless overview no longer shows an undefined SSID or a false idle state.

## Settings choice buttons and profile separator
- Settings removes the separator above Auto Blinker when the vehicle profile hides Nag Killer.
- Appearance and Dashboard Polling Rate now show three inline choice buttons below their descriptions. The current saved choice is highlighted on load; selecting a button applies and stores the value through the existing handlers.
- The production dashboard, embedded header, offline Home simulator preview, and OTA application binary were rebuilt from the same source.

## Offline Home preview controls
- Added a preview-only panel outside the dashboard for AP OFF / AUTOSTEER / NOA and all five supported vehicle models. Standard models also expose Body + Chassis versus Party + Chassis, and Model 3 Highland Body + Chassis exposes Stalk versus Stalkless.
- AP changes update the mock Home data immediately; model and wiring changes reload the preview so the production profile gates and CAN labels are reapplied from boot. Corrected the preview Home snapshot envelope to match the production dashboard API.
- The production dashboard source, embedded HTML header, and OTA firmware remain unchanged by this preview-only addition.

## Dashboard appearance preference
- Added **Settings > System > Appearance** with **FOLLOW PHONE**, **LIGHT**, and **DARK** choices. FOLLOW PHONE remains the default and updates live when the phone's appearance changes.
- The choice is stored in the viewing browser's local storage and applied before dashboard CSS loads, including after controller reboot or firmware update on the same browser. Manual LIGHT or DARK ignores phone appearance changes until FOLLOW PHONE is selected again.
- The chosen theme now drives page colors, AP-state color palettes, the setup and reconnect screens, native form color scheme, and the browser theme color.

## Dashboard selection and firmware file controls
- Replaced the operating-system popup for all 17 dashboard setting selectors with a rounded selection sheet matching the v3.8 cards. The existing select values, change handlers, disabled states, and saved settings remain the source of truth.
- Replaced the browser's default OTA file input presentation with a matching Choose .bin file button and an updating filename label. The existing `.bin` filter and upload/flash request are unchanged.

## Dark button text contrast
- Removed the new skin's broad button text-color override, which was taking precedence over the white-on-dark styling of profile setup and other legacy button components.
- Disabled buttons now retain their own text color and use a lighter opacity so the label stays readable on dark backgrounds.

## Dashboard style cleanup and Nag Killer rebuild
- Removed 18 superseded style layers from the dashboard source. The offline preview and embedded firmware now use the same three style blocks, so preview-only legacy styling can no longer mask missing production styles.
- Rebuilt the Nag Killer detail page around the supplied demo structure: enabled and pause controls, Mode A/B/C and Mode H choices, saved Rev.1/3/4 profile choices, behavior summary, live monitor, and dedicated tuning and advanced parameter links.
- Added a shared detail-page component layer for row typography, forms, monitor values, and directional transitions. Existing firmware API calls and saved setting behavior remain connected to their original controls.

## Supplied LAB HTML match
- Removed the legacy 7 px left margin from the Settings vehicle model name so its first character aligns with the profile label and firmware version above and below it.
- Rebuilt the LAB overview against the supplied `T2CAN Dashboard Demo.html`. The driving card now uses the reference's 30 px status/gear text, three 76 px separate rounded lane cells, 6 px cell gaps, compact `ALC STATE` label, paired `0x399`/`0x239 age` footer, and matching Research Tools row typography.
- Overrode the older LAB CSS selectors, including the ID-specific `#alcStateBig` rule that kept centering and enlarging text after the previous visual update. Removed the extra side-cell lane detail lines and the Research Tools helper line from the overview.
- Changed the DMS and CAN A overview rows to the reference's value-and-chevron layout. Their saved toggle and receive-method selector remain functional in dedicated LAB detail pages.
- Embedded the supplied Latin Geist and Geist Mono font faces as local WOFF2 resources. The dashboard and the new standalone preview use the same faces without an internet connection.

## Hard Init AP display continuity
- Home now keeps the last confirmed AP presentation while CAN Hard Init is running and through a bounded 10-second recovery grace period. A newly accepted `0x399` frame replaces the cached presentation immediately.
- The cache is presentation-only. Hard Init still invalidates the functional AP/NOA state and closes NAG, Auto Blinker, R79, and other CAN-transmit gates exactly as before.
- Added separate Home snapshot fields for the display state, leaving the existing live `dasStateValid` / `dasState` fields and control logic unchanged.
- Locked the LAB overview to the approved mockup: left-aligned driving state, right-aligned gear, three separate rounded LEFT / ALC STATE / RIGHT cells, paired freshness values, and the inset Research Tools list.

## Approved-demo layout parity correction
- Removed the 50 px iPhone mock status-bar offset from production page and detail-panel spacing. Main content now begins 8 px below the real 48 px fixed header.
- Rebuilt the Home R79 summary as the approved horizontal card with state copy on the left and TX counters on the right. Removed legacy column-direction leakage and restored content-height CAN cards, value-first metrics, demo spacing, and single-color header status dots.
- Rebuilt the LAB overview with three separated direction/state cells, paired `0x399` and `0x239` freshness values, inset research-tool list separators, the compact DMS row, and the demo-style CAN A RX selector row.
- Matched the approved navigation width/gaps and applied the same inset list construction to Settings cards without changing firmware endpoints or feature gates.

## 2027 dashboard production release
- Promoted the approved 2027 dashboard to the v3.8 production UI while retaining the existing firmware APIs and vehicle-profile capability gates.
- The fixed header now shows only **TESLA UNLOCK**, Home CAN frame ages occupy their own second line, CAN A/B sit above the unified R79 card, unsupported NAG/Auto Blinker controls remain hidden, and the obsolete Home Live Details card is no longer displayed.
- Removed transient dashboard toast messages. Controls now communicate through their visible state, saved selection, progress area, confirmation sheet, or reconnect screen.
- Retained directional detail-panel transitions, reduced the Home feature-label-to-toggle gap, and removed premature capability labels from the first vehicle-selection card.

## NAG Killer production default
- Fresh NVS, invalid persisted mode/variant fallback, NAG reset, Settings reset, and Factory Reset now select **Mode H · Rev.4**.
- A valid mode and Mode H revision already saved by an existing installation remain unchanged across OTA, including a deliberately selected Rev.1 or Rev.3 profile.
- Rev.4 tuning remains the established production profile: opposite carrier, TIERED Hands-On policy, Visual Warning Rescue, and HARD PAUSE while stopped.

# T2CAN Universal v3.7.3

## 2027 full-bleed dashboard
- Replaced the four main dashboard pages with the supplied 2027 full-bleed visual system: live STANDBY / AUTOSTEER / NOA field colors, compact fixed header, floating four-tab navigation, larger state typography, tinted cards, and the new Home, Devices, Settings, and LAB layouts.
- Preserved all 77 existing dashboard API endpoints, DOM control IDs, polling groups, warning dialogs, feature panels, profile setup, OTA, and diagnostic flows. Settings and LAB navigation remain real buttons wired to the existing page controller.
- Kept the persistent LAB CAN A receive selector available across every profile with the labels **4 FRAMES** and **IMMEDIATE**.
- Added browser regression coverage for AP-state presentation, all four navigation tabs, device-card rendering, and the CAN A receive update request. Superseded style layers are omitted from the embedded build to keep the dashboard below its flash budget.

## CAN A RX LAB comparison variant
- LAB now selects **CURRENT · 4 FRAMES** or **3.6c7 · IMMEDIATE** for the MCP2515 CAN A read loop. The latter reads one frame and runs the existing observers, decoders, and NAG logic before reading the next frame.
- The selection is saved in `features/canARxMode` and restored after reboot. Missing or invalid storage defaults to the current four-frame method. Disabling LAB makes the effective method four-frame while retaining the saved selection for the next LAB session.
- Both methods keep the existing 32-frame task yield limit. This switch is a diagnostic comparison; it does not claim to fix NAG on HW3 Model Y without vehicle testing.

## Auto Blinker TX policy promoted to Settings
- Promoted the former LAB-only **Single TX / 350 ms Burst** selector to **Settings > Auto Blinker** and made it the shared production policy for Auto Blinker and direct S3XY left/right requests.
- Persisted the selection in Preferences/NVS as `features/blinkTx` (`0` = Single TX, `1` = 350 ms Burst). A missing or invalid value defaults to **Single TX on Model YL** and **350 ms Burst on every other supported Model 3/Y profile**.
- The update API now writes and verifies NVS before changing runtime state. A persistence error leaves the active policy unchanged, and switching to Single TX atomically cancels an active burst.
- On Model 3 Highland stalkless configurations, Single TX now overlays one accepted live `0x3C2` mux1 frame with the requested ON state; the following stock frame supplies the natural release. Physical steering-wheel input retains priority.
- Moved operational TX mode/source/result/request/blocked counters into the official Auto Blinker status and removed the former LAB routes, panel, polling, and LAB-disable reset behavior.

## Focused source cleanup
- Removed unconsumed R79 0x7FF observation, dead bit mirrors/latency probes/TX-success attribution, stale dashboard writes, obsolete forward declarations, unused constants, and production helpers referenced only by tests.
- Consolidated stock-synchronized stalk frame preparation and common result bookkeeping while retaining separate CAN A and CAN B physical send paths.
- Consolidated duplicate CAN trace raw formatting, vehicle capability JSON fields, and shared Mode H Rev.3/Rev.4 JSON fields without changing public key order.
- Added host regression coverage for exact left/right stalk bytes, persistence fallback/transition behavior, official API/UI placement, and the reviewed dead-code inventory.

# T2CAN Universal v3.7.2

## Auto Blinker / S3XY profile-default TX policy
- Changed the production blinker TX default by vehicle profile: **Model YL remains SINGLE TX**, while **every other supported Model 3/Y profile defaults to LEGACY 350 ms BURST**.
- The LAB selector remains a volatile comparison override. Disabling LAB or rebooting returns to the active vehicle profile default instead of globally forcing SINGLE TX.
- Standard stalk profiles therefore use the existing 350 ms / ~20 ms legacy `0x249` pulse by default. Stalkless profiles enter the existing legacy press/release route; no new stalkless timing was introduced.

## LAB — persistent 0x3FD mux1 bit43 DMS/NAG control for all supported 3/Y profiles
- Added **Disable Driver Monitoring (NAG)** directly to LAB and expanded it to every current vehicle profile/topology that supports the Universal CAN-B `0x3FD` / R79 route: Model Y L, Model Y Juniper, Model Y Legacy, Model 3 Highland, and Model 3 Legacy. Standard 3/Y supports both Body+Chassis and Party+Chassis selections.
- Support is keyed to `vehicleProfileDmsNagSupported()`, which follows the existing `vehicleProfileEuUnlockSupported()` / 0x3FD transport capability instead of model identity.
- The toggle is now **Preferences/NVS-backed** under `features/dmsNag43`. Its ON/OFF selection survives reboot. Disabling the global LAB menu immediately makes the injection inactive through the LAB gate but does **not** erase the saved selection; re-enabling LAB restores it. Default remains OFF when no saved key exists.
- When active, T-2CAN clears **0x3FD mux1 bit43** on every T-2CAN-generated mux1 clone that can coexist with the fixed R79 path: the 2 ms MUX1 Fast Echo, +150 ms periodic/retry path, and the independent ULC-snooze mux1 TX. Existing R79 bit18/19/47 behavior and timing are unchanged.
- Added stock/last-effective bit43 telemetry to the R79 LAB card and lightweight LAB snapshot, plus RX 0/1/change counters in the API. Dashboard wording now reports **SAVED** state instead of RAM-only state.
- The control remains experimental: the firmware manipulates the documented bit position, but real-vehicle validation is still required to establish whether bit43 suppression produces identical NAG behavior across vehicle generations/HW/software branches.

# T2CAN Universal v3.7.1

## P0 — 0x399 AP / Hands-On decode correction
- Corrected `DAS_autopilotHandsOnState` to DBC bit 42..45 (`data[5] >> 2`) instead of copying the AP state from `data[0]`.
- Corrected the NAG 0x399 default selectors to AP byte 0 / shift 0 and Hands-On byte 5 / shift 2 while preserving the existing generic selector path and NVS schema.
- Unified AP state handling on the full low nibble. Only raw 3/4/5/6 remain AP-active; raw 14 (`FAULT`) can no longer be truncated to 6 and authorized.
- Both YL Party CAN A and Standard Chassis CAN B now require DLC 6 before admitting 0x399 to the shared decoder. A short frame cannot refresh AP, NOA, Hands-On warning, or NAG authorization state.
- Added host regression vectors for AP/Hands-On independence, warning edges, raw 14, raw 5 NOA, selector defaults, and short-DLC rejection.

## Auto Blinker NOA session gate follow-up
- Split NOA-entry stabilization from the manual cancel-pause state. Stabilization now runs once per confirmed NOA session; after the gate reaches **READY**, it cannot close again until NOA has been continuously inactive for **2 seconds**.
- Added a **2-second NOA exit confirmation** state. A shorter valid non-NOA interval blocks Auto Blinker TX but preserves the current stabilization/READY session, so a quick signal fluctuation does not restart the entry timer.
- Applying a different NOA Stabilization value from the dashboard now replaces an in-progress deadline from the moment of the update. An already-READY session remains open, and the new value applies on the next confirmed NOA entry.
- Added explicit `STABILIZATION`, `READY`, `EXIT WAIT`, and `PAUSED` states to both the full Auto Blinker API and lightweight HOME snapshots. During stabilization, the top HOME Blinker metric now shows the rounded-up remaining seconds (`10s` through `1s`) before changing to `READY`; both polling paths use the same renderer.
- Cancel Pause remains independent: expiry or manual release resumes the completed NOA session without another stabilization interval. CAN recovery and invalid DAS state still fail closed and reset both transient states.

## Lane-change cancel hotfix
- Removed `DAS_behaviorType` / `visualBehaviorType` LEFT/RIGHT gating from both **S3XY NOA Lane Change Cancel** and **Door Open Lane Change Cancel**. A manual cancel can now arm `UI_ulcSnooze` while the fresh `0x24A` frame still reports `IN_LANE`.
- Retained the existing fail-closed **NOA state/freshness**, fresh `0x24A` context, ULC request timeout, CAN admission, and shared Auto Blinker cancel-pause behavior.
- Manual cancel no longer writes a synthetic planner direction into `lastReqDir` / `autoRequestLastSeenMs`; Auto Blinker continues to use `DAS_behaviorType` normally for its own planner-driven LEFT/RIGHT logic.

## Production R79 policy
- Replaced the former R79 transport/timing experiment matrix with one fixed production path: each accepted stock MUX1 is echoed with a bounded **2 ms** queue wait after applying `bit19=0` and `bit47=1`, and each accepted stock MUX2 schedules one **+150 ms** refresh.
- Promoted SmartSummonOnly bit 18 to **Settings > Summon** as the only remaining R79 payload choice: **FORCE 0** is the default and **STOCK** preserves the received bit. LAB now shows a read-only R79 status card with no transport controls.
- Removed the retired transport selector, MUX0 collision, roaming/v2.6 alternatives, periodic/PRE/POST/quiet-window controls, timing capture, retry-mode selectors, and their API/NVS/UI surfaces.

## Mode H Rev.4 production profile
- Finalized Rev.4 defaults: Primary **1.80–2.60 Nm**, WAIT **0.90–3.00 s**, REFRACTORY **0.50–1.50 s**, opposite carrier **0.10–0.60 Nm**, TIERED Hands-On thresholds **0.40/2.00 Nm**, Visual Warning Rescue **ON / 0.5 s**, and **HARD PAUSE** while stopped.
- Kept Mode H Profiles as a production feature under **Settings > NAG KILLER** and removed the former experimental presentation.

## Auto Blinker stabilization and cancel pause
- Added an independent NOA-entry stabilization timer, configurable from **1–20 s** and defaulting to **10 s**. Auto Blinker cannot start until NOA has remained eligible for the complete interval.
- Added a shared Auto Blinker cancel-pause toggle for the driver-door-open and S3XY Auto Blinker Cancel actions. The pause is configurable from **10–100 s**, defaults to **20 s**, and a second cancel action releases it early.
- Direct S3XY Left/Right Blinker actions remain independent of Auto Blinker enable, stabilization, and pause state.

## AP Right Scroll warning recovery
- Promoted AP Right Scroll to Settings for both Model YL and standard Model 3/Y profiles. It uses **CHASSIS / CAN B** on all supported profiles, with the existing YL VH route retained as that profile's CAN-B transport.
- A newly observed nag visual warning now fires the configured UP/DOWN pair immediately. If the warning remains active, the pair repeats every **1–5 s**; the production default is **2 s**.
- Physical right-scroll input has priority, and disable, recovery, administrative-hold, route-failure, and warning-clear transitions cancel pending pairs.

## Persistence and dashboard
- Advanced feature-domain persistence to schema **3** with verified ordered migration for `r79/bit18`, `summon/noaStabS`, `summon/cancelPauseS`, and `features/rsWarnS`; obsolete experimental keys are removed only after the new values are read back and the schema marker commits.
- Added the production controls to Settings, retained a read-only LAB R79 status card, and preserved the embedded dashboard's strict **84,000-byte gzip** limit.
- Updated the firmware identity to **v3.7**.

# T2CAN Universal v3.6f3

## Feature-promotion / BUS OFF persistence patch
- Changed both Auto Blinker and direct S3XY blinker requests to the shared **single-TX** policy by default. LAB now offers a volatile **SINGLE TX / LEGACY 350 ms BURST** comparison; the selection always returns to SINGLE TX after reboot or LAB disable.
- Consolidated production `0x3F8` changes into one CAN-B stock-frame compositor. Confirm-Free, ULC Off-Highway, ULC Blind Spot, PedalMap Control, AP Right Scroll, and Mode H Profiles are promoted to Settings; Auto Lane Change `0x293` and the passive Blind Spot monitor remain in LAB.
- Removed ACC Follow Distance, `0x3F8` target-bus selection, ULC Speed Config, and the failed TLSSC Green-Light experiment. Auto Lane Change Enable remains available as an explicit LAB experiment.
- Moved Mode H Profiles under **Settings > NAG KILLER**, retained Rev.4 approved defaults and the Rev.4 Hands-On Policy controls, and compacted Firmware Profile to roughly one-third of its previous mobile height.
- Persisted the latest CAN-A and CAN-B BUS OFF snapshots plus bounded TX traces in verified dual-slot NVS records. Reboot retains the evidence; only a successful user Reset Stats action clears it, while a failed clear reports an error and keeps the record.
- Collapsed empty S3XY/BLE diagnostics without leaving an orphan label or gap, and added current/previous-boot BUS OFF provenance plus persistence state to diagnostics.
- Added feature-domain migration schema **2** with read-back verification before obsolete experiment keys are removed.

## Mode H Rev.4 approved defaults + Hands-On Policy patch
- Changed the Rev.4 defaults to Primary Peak **1.80–2.60 Nm**, WAIT **0.90–3.00 s**, REFRACTORY **0.50–1.50 s**, opposite carrier **0.10–0.60 Nm**, Visual Warning Rescue **ON**, and Rescue Delay **0.5 s**.
- Added the same three Hands-On policies exposed by Rev.3: **ALWAYS HO=1**, **THRESHOLD HO=1**, and **TIERED HO=1 / 2**. Rev.4 defaults to TIERED with thresholds **0.40 Nm / 1.75 Nm**.
- Hands-On policy is evaluated from the final transmitted torque magnitude. In the default TIERED mode, torque below 0.40 Nm preserves the accepted stock HO field, 0.40–1.74 Nm generates HO=1, and 1.75 Nm or more generates HO=2. HO=3 is never generated.
- Added Rev.4 dashboard controls, API fields, input validation, and NVS persistence (`h4hop`, `h4ho1`, `h4ho2`). Nag configuration schema is now **17**.
- Schema migration promotes only the exact former Rev.4 default profile to the approved defaults. Any customized Rev.4 peak/timing/carrier/rescue profile is preserved and receives the new default Hands-On policy fields.
- Confirmed that Mode H STOPPED stock-carrier behavior still preserves the complete stock HO field.

## S3XY 0x249 stock-synchronized single-shot patch
- Changed direct S3XY Left/Right Blinker actions on stalk vehicles from a free-running **350 ms / 20 ms** pulse to one overlay following the **next real stock `0x249`**.
- The direct request is consumed exactly once, expires after **250 ms**, and is cancelled if that stock frame reports a non-idle physical stalk. A newer direct request replaces the pending direction.
- Auto Blinker retains its existing timed pulse behavior. Stalkless vehicles retain the existing stock-RX-following `0x3C2` press/release cycle.
- CAN recovery now clears the direct stock-synchronized request as well as `oneShotDirect`, preventing a button request from crossing a controller epoch.
- Added pure state-machine and static integration regression coverage for single consumption, direction replacement, rollover-safe expiry, physical-stalk priority, bus-specific TX routing, and recovery reset.

## R79 MUX0 Collision LAB
- Added a fourth, mutually exclusive **MUX0 COLLISION LAB** transport. Every real stock `0x3FD` MUX0 snapshots the previously accepted stock MUX1 template and schedules one modified R79 request after a RAM-only **40–52 ms** delay (default **40 ms**).
- Later stock MUX1/MUX2 frames do not cancel or move the reservation. Stock MUX1 is observed normally for the next cycle, while this strategy does not also run D9 Fast Echo, ROAMING mirror, V2.6 mirror, periodic, PRE, POST, or delayed retry paths.
- Independent reservations use a 256-entry FIFO matching the configured TWAI RX queue. At most one due reservation is serviced per CAN task loop, preventing a delayed RX batch from self-filling and repeatedly purging the local 16-entry TX queue.
- The due request retains the existing R79 authorization gates and payload policy. If its zero-wait CAN-B enqueue reports `ESP_ERR_TIMEOUT` (no local queue slot available), the firmware clears that local queue and retries `0x3FD` exactly once. This does not affect frames already on-wire or another ECU's queue and cannot override physical CAN arbitration.
- Added dashboard selection/delay control plus arm, attempt, result, queue-purge, pre-clear queued-frame estimate and best-effort timing diagnostics. The experimental strategy and delay are never written to NVS; reboot always returns transport selection to **D9 CURRENT** and delay to **40 ms**.
- Added pure scheduler/rollover/queue-policy tests, integration contracts, and a dedicated `MUX0_COLLISION` timing-capture slot.

## Mode H Rev.4 Visual Warning Rescue
- Added optional **Visual Warning Rescue** for Mode H Rev.4. It defaults **OFF** and listens for a real DAS Hands-On transition from a value below 3 into **3/4/5**.
- Each warning epoch starts one delay from the original DAS transition timestamp. The delay defaults to **0.5 seconds**, is adjustable from **0.0–2.0 seconds** in 0.1-second steps, and is persisted in NVS together with the ON/OFF setting.
- If the DAS warning clears below 3 before the delay expires, the pending Rescue is cancelled. `3 -> 4 -> 5` remains one warning epoch and cannot retrigger; returning below 3 re-arms the next warning edge.
- When the deadline is reached, the next eligible real stock `0x370` RX enters Rev.4 directly at full PRIMARY peak, generates a fresh **1.50–2.10 Nm** event using the existing **80% NEG / 20% POS** policy, then returns through the existing RAMP_OUT, REFRACTORY/carrier and WAIT/carrier flow.
- Rescue reuses the existing stock-RX-follow Mode H transmit path. It adds no timer task, background CAN transmission, retry, or independent TX scheduler.
- Added pending/count diagnostics and dashboard controls. Mode H's visible revision selector is now numerically ordered **Rev.1 -> Rev.3 -> Rev.4**; legacy persisted Rev.2 values remain runtime-readable but Rev.2 is not presented as a selectable profile.
- Added independent NVS keys `h4vres` / `h4vdly` and advanced the Nag configuration schema to **16**.

# T2CAN Universal v3.6f1

## AP Right Scroll / ROAMING MUX1 Burst
- Added a Model YL LAB feature that, while AP is active, injects a right-scroll **UP** (`0x01`) followed by **DOWN** (`0x3F`) on consecutive real CAN-B/VH `0x3C2` MUX1 frames. The interval is configurable from **1–600 seconds**, saved in NVS, and defaults OFF at 30 seconds.
- The AP Right Scroll path clones each live stock frame, changes only Byte3 bits `[5:0]`, yields to any physical right-scroll input, and cancels on AP/LAB/feature disable, CAN recovery, or administrative hold.
- Added RAM-only **ROAMING MUX1 Burst** selection: `1x`, `2x`, or `3x`. Shot 1 is the existing source-derived mirror; shot 2/3 reuse the same corrected template, request at most one zero-wait enqueue per CAN loop, and wait for the CAN-B TX pipeline to become idle.
- Any newer real stock `0x3FD` cancels pending burst shots before normal stock handling. Strategy changes, silence, safety/administrative gate closure, and CAN recovery also cancel pending shots. Extra burst shots never use delayed retry.
- Extra burst shots use the CAN recovery epoch/freshness barrier and require both the TX-IDLE latch and live TWAI queue status to confirm an idle pipeline, preventing stale alerts or recovery/admin transitions from admitting a shot.
- AP disengagement and administrative-hold entry synchronously clear pending AP Right Scroll state; administrative hold also clears any pending ROAMING burst.
- Extended R79 Timing Capture with `ROAMING_BURST_2`, `ROAMING_BURST_3`, and `ROAMING_BURST_CANCEL_STOCK`, and snapshots the RAM-only burst selection in CSV metadata.
- Added LAB controls and diagnostics for both features while retaining the embedded dashboard's **<84,000-byte gzip** budget.

# T2CAN Universal v3.6e2

## Mode H Rev.4 / dashboard state / direct S3XY blinker
- Replaced the selectable **Mode H Rev.1 Plus** slot with **Rev.4 · Opposite Carrier** while preserving persisted variant ID `1`. Rev.1, Rev.2, and Rev.3 remain available.
- Rev.4 keeps the existing Mode-H/stock-`0x370` RX-follow transmit cadence; it does **not** add a timer, task, or extra CAN-TX scheduler.
- Rev.4 applies its carrier in both **WAIT** and **REFRACTORY**. For every eligible stock `0x370` RX, Carrier Min–Max is sampled again independently (default **0.10–0.60 Nm**; no 350–900 ms hold).
- Outside the ±**0.05 Nm** stock deadband, Rev.4 carrier output uses the opposite stock sign with `|TX| = |stock| + carrier`. Inside the deadband the stock torque is passed through unchanged.
- Rev.4 Primary events are independent of stock sign and use **80% negative / 20% positive** direction selection. Defaults: Peak **1.50–2.10 Nm**, WAIT **0.90–2.00 s**, REFRACTORY **0.50–1.50 s**.
- Rev.4 uses a dedicated carrier RNG stream so per-RX carrier sampling cannot perturb Primary event cadence, peak, or direction RNG. RAMP_OUT captures one opposite-carrier target so the Primary waveform decays toward the upcoming REFRACTORY carrier regime instead of snapping through stock first.
- Added independent Rev.4 NVS keys (`h4*`) and advanced the Nag configuration schema to **15**. Legacy Rev.1 Plus `h1*` tuning is intentionally not imported into Rev.4; an old saved variant ID `1` selects Rev.4 with Rev.4 defaults unless Rev.4 keys already exist.
- Fixed Mode H LAB polling clobber: tuning inputs now use **dirty + saving + request-epoch** protection. Unsaved edits are not overwritten after focus changes, and stale polling responses cannot roll the form back while editing/saving.
- S3XY **Left Blinker / Right Blinker** direct actions are separated from Auto Blinker policy. Direct requests no longer depend on NOA/ALC state, Auto Blinker enable, Advanced-EAP capability, or Confirm-Free; actual turn-route validity, stock-template availability, physical-input collision handling, and CAN transport admission remain enforced. Auto Blinker retains its existing policy gates.
- S3XY action UI now exposes direct left/right blink actions by actual configured turn-signal route rather than Advanced-EAP support; Auto Blinker Toggle remains Advanced-EAP-gated.
- Dashboard generation optionally uses deterministic Zopfli gzip (with the existing stdlib zlib-9 fallback) to keep the embedded dashboard below the established 84,000-byte gzip budget.

## Preserved baseline
- Built directly on the supplied **v3.6e1 RATIO-FIX** source. R79 ratio/timing transport behavior is intentionally untouched by the Rev.4, Mode-H UI, and S3XY policy changes.

# T2CAN Universal v3.6d9a6

- Dashboard: disabled controls now receive a consistent dimmed/desaturated visual state at both control and row level.
- S3XY Button: added Left Blinker and Right Blinker one-shot actions using the existing validated stalk/stalkless turn-signal transport.
- Existing S3XY action IDs remain unchanged; new actions append IDs 11/12 for NVS compatibility.

## Previous v3.6d9a5 changes
## Selectable R79 transport A/B — D9 / Roaming / v2.6
- Added persistent **R79 Transport** selection in LAB with three mutually exclusive strategies. Existing installations default to **D9 CURRENT** and keep all d9a4 behavior unless explicitly changed.
- **D9 CURRENT** preserves the existing d9a4 Fast Reactive / QW / PRE-MUX1 / POST-MUX2 / periodic / retry behavior and all current R79 experiment controls.
- **ROAMING MIRROR** ports the R79 transport behavior from the supplied `t2can-roaming` source: every accepted real stock `0x3FD` mux1 is copied, bit19 is cleared, bit47 is set, bit18 and all other bits remain stock, and the mirror uses the source firmware's bounded `twai_transmit(..., 5 ms)` queue wait. There is no R79 periodic/background scheduler or delayed retry in this strategy.
- **V2.6 LEGACY** ports the R79 transport behavior from the supplied `Summon-Unlock` v2.6 source: every accepted stock mux1 is mirrored with bit19=0 / bit47=1 using a bounded 2 ms queue wait, plus the latest real mux1 template is re-sent every **500 ms while the Summoning gate is active**. There is no d9 QW/PRE/POST/487 scheduler or d9 retry path in this strategy.
- Legacy strategies deliberately preserve the **current d9 common safety/administrative gate** (manual D/R suppression, TWAI-ready and administrative-hold checks) so the A/B isolates R79 transport timing rather than importing unrelated whole-firmware gate policy. They are therefore source-faithful transport ports, not full firmware gate clones.
- Legacy strategy payloads deliberately preserve stock **bit18**. The d9 bit18 experiment and all d9 scheduler/PRE/POST controls are disabled in the dashboard while a legacy transport is selected.
- Strategy switches cancel/reset pending d9 schedulers, retries and TX-success attribution probes before the new transport becomes active. The selected strategy is NVS-persistent.
- R79 Timing Capture records the selected transport in the CSV header and adds dedicated slots: `ROAMING_MIRROR`, `V26_MIRROR`, and `V26_PERIODIC`.
- Added separate Roaming mirror, v2.6 mirror and v2.6 periodic counters to the R79 API while keeping existing aggregate immediate/periodic counters.
- Existing POST-MUX2 input focus/dirty protection and the 84,000-byte dashboard gzip budget are preserved.

# T2CAN Universal v3.6d9a4

## Independent POST-MUX2 timing experiment
- Added persistent **POST-MUX2 TX** with independent ON/OFF control. It uses the latest valid real stock mux1 template and is armed by each real stock MUX2.
- Added direct **0–149 ms POST-MUX2 Delay** input. This path is intentionally independent from the Quiet Window +150 ms hard-start guard so the newly identified post-MUX2/pre-QW1 window can be A/B tested directly.
- Any newer real stock `0x3FD` arriving before the configured due time cancels the pending POST-MUX2 request. POST-MUX2 never uses delayed retry, preventing a stale experimental TX from drifting into a newer stock cluster.
- R79 Timing Capture now tags POST-MUX2 as its own slot and snapshots POST-MUX2 ON/OFF + delay in the CSV config header. Added arm/cancel/attempt/OK/FAIL/blocked/no-template and MUX2→TX-request timing telemetry.
- The POST-MUX2 numeric field uses focus + dirty protection: background dashboard polling does not overwrite the value while the user is typing, and the setting is committed only on the input change event.
- POST-MUX2 defaults **OFF**. Existing MUX0/MUX1/MUX2 Quiet Window anchor behavior, 150 ms Quiet Window hard guard, Fast Reactive Echo, PRE-MUX1, payload policy, and retry semantics are otherwise unchanged.

# T2CAN Universal v3.6d9a3

- Added R79 Timing Capture: up to 10 minutes / 16,384 compact events with VIDEO SYNC markers, stock 0x3FD RX, actual R79 TX attempts, slot tags, guard skips, queue snapshot, and streamed CSV download.
- Added selectable Quiet Window Event Anchor: MUX0 / MUX1 / MUX2. Default remains MUX2 for backward-compatible behavior.
- Quiet Window now preserves the selected anchor cycle across later muxes in the same stock 0→1→2 cluster while those frames still refresh the last-stock safety guard.
- R79 Timing Capture is diagnostic-only and does not transmit CAN frames.

# T2CAN Universal v3.6d9a2

## R79 post-mux1 enqueue-wait A/B
- Added one persistent LAB selector: **`R79 Post-Mux1 TX Mode`**.
  - `FAST ECHO · 0 ms` keeps the current d9a1 zero-wait `twai_transmit(&out, 0)` behavior.
  - `V2.6 STYLE · 2 ms WAIT` uses `twai_transmit(&out, pdMS_TO_TICKS(2))` for the initial stock-mux1-triggered R79 enqueue.
- This A/B changes **only the initial post-mux1 queue-admission wait**. The stock mux1 trigger, R79 bit18/19/47 payload policy, fail-open/manual-D/R authorization, TX_SUCCESS probe, emergency queue flush, and existing bounded retry path are unchanged.
- The 2 ms mode intentionally does **not** import V2.6's repeated/unbounded retry behavior. If the selected initial request fails, d9a1 recovery semantics continue unchanged.
- Added NVS persistence (`r79lab/postTx`) with existing d9a1 behavior (`FAST ECHO · 0 ms`) as the migration/default value.
- Added per-mode diagnostics for attempts / OK / FAIL and TWAI-dequeue→TX-request latency while preserving all existing aggregate Fast Echo counters and TX_SUCCESS telemetry. The RX→request metric is measured before any optional 2 ms queue wait.
- Periodic/PRE-MUX1/Quiet/Phased/Guarded-487 schedulers, Mode H, CAN routing, and unrelated vehicle behavior are unchanged.

---
# T2CAN Universal v3.6d9a1

## Compile hotfix — restore CAN traffic UI snapshot helper
- Fixed an Arduino compile regression in `web_api.h`: d9 still referenced `CanTrafficUiSnapshot`, `canTrafficUiSnapshot()`, and `CAN_TRAFFIC_UI_FRESH_MS`, but their shared definition block had been accidentally dropped during the d9 API/R79 edit.
- Restored the exact CAN-traffic UI snapshot helper used by the prior working line: 1500 ms freshness threshold plus CAN A/CAN B seen/age/online fields.
- No R79, PRE-MUX1, scheduler, Mode H, CAN routing, injection, dashboard behavior, or NVS semantics are intentionally changed by this hotfix.
- Added a regression test that requires the traffic snapshot constant/type/helper to be defined before the first API use.

---
# T2CAN Universal v3.6d9

## R79 PRE-MUX1 / guarded 487 phase-walk / Quiet Window margin follow-up
- Preserved the existing d8 Periodic controls and schedulers, then added a third selectable scheduler: **`GUARDED 487 PHASE-WALK`**. It retains an exact **487 ms** due cadence; unsafe slots are skipped without re-anchoring, so the phase keeps walking. The latest stock mux1 and latest stock `0x3FD` are safety references only.
- Guarded 487 currently runs **1x per 487 ms cycle**. The safety window requires mux1-relative phase **+90..+410 ms** and at least **30 ms** since the newest stock `0x3FD`, leaving margin around the measured mux0/mux1/mux2 cluster.
- Added independent **PRE-MUX1 TX**. A real stock mux0 arms one PRE request using the previous accepted stock mux1 template; default is **mux0 +20 ms**, configurable **10–30 ms**. PRE is independent from Periodic Refresh, so `Periodic OFF + PRE ON` is a clean PRE+Fast-Echo experiment.
- PRE-MUX1 has **no delayed retry**: a retry could drift into the upcoming stock mux1. A stock mux1 arriving before the due time cancels the PRE request.
- Added PRE telemetry: arm/cancel/attempt/OK/FAIL/blocked/no-template, mux0→PRE-request latency, previous-template age/raw/diff count, conservative PRE `TX_SUCCESS` attribution, and `TX_SUCCESS`→next-stock-mux1 delta/ambiguity.
- Fixed the d8 Quiet Window multi-shot edge condition. The multi-shot **target end is now +300 ms after mux2** with a separate **+340 ms hard cutoff**, instead of placing the last slot directly on the old +320 ms cutoff. With the default +200 ms first slot: `2x = +200/+300`, `3x = +200/+250/+300`.
- Quiet first-slot maximum now adapts to shot count to maintain slot spacing: **1x ≤300 ms, 2x ≤260 ms, 3x ≤220 ms**.
- Added **per-slot Periodic diagnostics** for Slot 1/2/3 due/fire/guard counts in addition to scheduler-wide counters.
- Fixed the iPhone LAB numeric-input polling-clobber bug. `Mux2 Quiet Start Delay / Mux1 Phase Offset / Phase-Walk offset` and PRE offset are protected by focus/dirty state so background `fetchR79()` polling no longer rewrites a value while the user is editing it. Scheduler/shot changes clamp the numeric value to the selected mode's valid range instead of reverting the selection.
- Periodic policy remains independent (`ALWAYS / SUMMON ONLY / OFF`). Fast Reactive Echo remains unchanged. PRE remains independently switchable even when Periodic is OFF.
- Preserved d7/d8 heartbeat A/B/BOTH diagnostics, iPhone viewport/scroll stabilization, d5 Mode H STOCK CARRIER/HARD PAUSE, Rev.3 defaults, CAN-A prefetch, CSV download cleanup and dark-mode fixes.

---
# T2CAN Universal v3.6d6

## Preserved d6 platform changes
- Migrated the legacy 487 ms value once to a 200 ms stock-phase delay while retaining Periodic mode selection.
- Added CAN task-heartbeat timeout snapshots that identify **CAN A / CAN B / BOTH**, preserve both heartbeat ages at the timeout, and count each cause independently.
- Added iOS/WebKit viewport stabilization with `overflow-anchor: none`, `100svh`, and separate window/fixed-panel scroll restoration that yields to recent touch, pointer, or wheel input.

---
# T2CAN Universal v3.6d5

## Mode H stationary transport A/B experiment
- Added a persistent **Mode H Stop Behavior** control in LAB with two choices:
  - **STOCK CARRIER** — new default. When Mode H is positively in `PAUSED_STOPPED`, the event generator remains paused but T-2CAN keeps the 0x370 transport continuous. The injected frame preserves the accepted stock torque and complete stock 2-bit Hands-On field; only the rolling counter and checksum are advanced.
  - **HARD PAUSE · 0-TX** — preserves the v3.6d4 behavior and transmits no Mode H frame while positively stopped.
- STOP CARRIER is deliberately limited to the confirmed STOPPED block reason. **Speed stale** and **move-confirming** states remain hard-paused, so the experiment does not convert uncertain motion state into active NAG transmission.
- Added Stop Carrier diagnostics: current stop behavior, active stop-carrier state, and successful stop-carrier TX count. HOME/LAB display `CARRIER` / `STOCK · STOP CARRIER` while the event engine itself remains `PAUSED_STOPPED`.
- Fixed the lightweight HOME snapshot's Rev.3 paused-state lookup to read the Rev.3 runtime state rather than the Rev.1 state, so the new stationary transport state is represented consistently on HOME.
- NVS schema advances to **v14** with `STOCK CARRIER` as the default for fresh installs and upgrades that do not yet have a stop-behavior key. The setting is immediately switchable for same-session A/B testing and is NVS-persistent.
- R79, CAN routing, Mode H Rev.3 tuning values, 487 ms R79 periodic default interval, d3 diagnostics/download fixes, Auto Blinker, PedalMap, 0x293/0x3F8, TLSSC, S3XY and capture behavior are otherwise unchanged.

---
# T2CAN Universal v3.6d4

- Changed the R79 Periodic Refresh default interval from **500 ms to 487 ms**. Periodic mode itself remains default **OFF**; saved user values are preserved.
- Changed the fresh/default Mode H profile to **Rev.3** while preserving an existing saved Mode H profile selection.
- Updated Mode H Rev.3 default tuning to the approved profile:
  - Primary peak: **1.50–2.10 Nm**
  - WAIT: **1.2–2.5 s**
  - Refractory: **0.8–1.8 s**
  - WAIT carrier: **+0.30–0.40 Nm**
  - Carrier direction: **FOLLOW STOCK**
  - Hands-On policy: **TIERED HO=1 / HO=2**
  - HO=1 threshold: **0.40 Nm**
  - HO=2 threshold: **1.75 Nm**
- Preserved the d3 R79 Fast Reactive Echo, TX_SUCCESS timing diagnostics, CSV-download cleanup, CAN-A burst prefetch, diagnostics-rate changes, and dark-mode select-arrow fix.
- Existing arbitrary custom Rev.3 tuning stored in NVS is preserved. On first d4 boot, the untouched d3 default profile and the supplied pre-d4 screenshot profile are migrated to the new d4 Rev.3 defaults; **Reset Current Profile** also applies them.

---

# T2CAN Universal v3.6d3

## R79 Diagnostics / Dashboard / CAN Runtime Follow-up
- Kept the v3.6d2 **Fast Reactive Echo** and existing fail-open R79 authorization behavior. The default independent Periodic Refresh mode remains OFF; no new manual-driving gate or R79 payload policy is introduced.
- Changed the R79 Periodic Refresh interval control from a fixed preset selector to a **direct integer millisecond input**. Valid values are **20–5000 ms** in 1 ms steps and remain NVS-persistent. The UI explicitly warns that very short independent periods can destabilize CAN B.
- Added conservative R79 completion telemetry based on TWAI `TX_SUCCESS` alerts. Samples are armed only when the application TX pipeline was already observed idle and are discarded as ambiguous if another CAN-B enqueue occurs first. Reported completion timing is explicitly a **software-observed upper bound**, not a hardware wire timestamp.
- Fixed generic CSV downloads (including CAN A/B TX trace, BLE, and Driver Monitoring) so download state is managed by `fetch()`/Blob completion and cleared in `finally`. This removes the dashboard deadlock where `downloadInProgress` stopped polling before the old completion path could clear itself.
- Improved CAN A MCP2515 burst handling by prefetching up to four RX frames from the two hardware receive buffers before running heavier observers/decoders, while retaining the existing 32-frame bounded yield budget and task priority.
- Replaced the unstable per-dashboard-poll `Errors/min` extrapolation with a deterministic **session average per minute** derived from cumulative TWAI counters and firmware uptime.
- Fixed dark-mode selector corruption on iOS/WebKit. The dark theme no longer uses a `background:` shorthand that resets `background-repeat`; custom select arrows now explicitly remain single, no-repeat, correctly positioned images.
- Expanded R79 reset handling so d2/d3 fast-path and TX_SUCCESS timing counters reset with the rest of the R79 diagnostics epoch.

---

# T2CAN Universal v3.6d2

## R79 Fast Reactive Echo Experiment
- Reworked the stock-triggered `0x3FD mux1` immediate R79 reassertion into a **receive-synchronized fast path** on CAN B / TWAI. The current stock frame is handled immediately after the TWAI driver dequeues it and before normal CAN-B RX-gap accounting, boot capture, research capture, driver-monitor capture, and decoder work.
- The fast path preserves the existing v3.6 **fail-open authorization policy**. Positively confirmed manual Drive/Reverse still suppresses R79; AP, confirmed Summon, Park/Neutral, and unknown/stale gear/DAS state remain default-on exactly as in d1. No new conservative/fail-closed gear gate was added.
- The first reactive TX request uses `twai_transmit(..., 0)` so it never waits up to the legacy 2 ms enqueue timeout before issuing the request. The fixed production overlay remains bit19=`0`, bit47=`1`, with LAB bit18 behavior unchanged.
- The d1 Summon transport hardening remains as **failure recovery only** after the fast request: READY/ACTIVE timeout handling may clear the pending TWAI TX queue, retry immediately, then use the bounded `+5 / +15 / +30 ms` sequence if needed. NORMAL/PARK_STANDBY still cannot use destructive queue flush.
- Removed the duplicate immediate TX from the later `r79LabObserve3fdMux1()` accounting path, so each stock mux1 frame has only one normal reactive request. The observer now records the stock template/statistics after the fast request.
- Added microsecond telemetry using `esp_timer_get_time()`: fast attempts / OK / FAIL / blocked, plus last/min/average/max and latency buckets. The metric is explicitly **TWAI dequeue → TX request**, not physical wire RX→TX latency because the classic TWAI API does not expose a hardware RX timestamp.
- To isolate this experiment from the independent periodic scheduler, the **first d2 boot migrates Periodic Refresh to OFF once**, including upgrades carrying a previous d1 NVS setting. Users may re-enable ALWAYS or SUMMON ONLY afterward from LAB. The 500 ms interval selection itself is retained.
- No intended change to NAG/Mode H, PedalMap, Auto Blinker, `0x293`, `0x3F8`, TLSSC, S3XY BLE, capture formats, Summon state detection, d1 queue-admission thresholds, or the d1 manual-suppression recovery behavior.

---

# T2CAN Universal v3.6d1

## R79 Summon Transport Hardening
- Added explicit CAN-B transport priority states: **NORMAL**, **PARK_STANDBY**, **SUMMON_READY**, and **SUMMON_ACTIVE**. PARK_STANDBY requires a fresh decoded real PARK observation and never uses the 0x118-stale compatibility PARK fallback.
- **SUMMON_READY** begins as soon as fresh ACA-active or non-zero SPR startup evidence appears; **SUMMON_ACTIVE** begins on the existing confirmed ACA+SPR Summon session. R79 is the highest-priority T2CAN CAN-B transmission from READY onward.
- Added non-R79 queue admission control: fresh PARK softly reserves the last 2 slots of the 16-entry TWAI TX queue (`queue >= 14` sheds lower-priority traffic), while READY/ACTIVE retain the stronger `queue >= 6` shedding threshold.
- Reintroduced destructive TWAI TX-queue flush **only** for an R79 `ESP_ERR_TIMEOUT` while READY/ACTIVE. NORMAL and PARK_STANDBY can never flush. A successful clear immediately retries the newest stock-derived R79 template.
- Added bounded Summon-priority R79 recovery retries at **+5 ms / +15 ms / +30 ms**. Retries always rebuild from the latest captured stock 0x3FD mux1 template, stop on any successful R79 TX, cancel if R79 authorization is lost or Summon priority ends, and cannot loop indefinitely.
- Changed periodic R79 cadence so the normal interval advances from the **last successful periodic TX** rather than the request attempt. Failed READY/ACTIVE requests enter the bounded recovery sequence instead of consuming the full 500 ms success cadence. After retry exhaustion, a bounded full-period hold prevents a high-rate failure loop.
- Preserved the v3.6 authorization policy: default R79 ACTIVE, bit19=0, bit47=1, manual confirmed D/R suspension, stock mux1 immediate reassertion, Periodic Refresh ALWAYS / SUMMON ONLY / OFF, default 500 ms.
- Added R79/Summon telemetry for priority state, PARK/READY/ACTIVE shed counters, retry scheduled/OK/FAIL/exhausted, last retry age, emergency queue flush count, flush-triggered retry results, queue depth, and periodic request vs successful periodic TX age.
- No intended behavior change to NAG/Mode H, Auto Blinker codec, PedalMap values, 0x293 payload logic, 0x3F8 overlay payload logic, TLSSC payload bits, S3XY BLE mapping, or capture formats outside the new CAN-B admission priority applied during PARK/READY/ACTIVE.

---

# T2CAN Universal v3.6 c7a1

## JSON writer compile hotfix
- Fixed five malformed `JsonWriterArduino::u32()` calls in `r79LabStatsToJson()` that still contained legacy Arduino `String` concatenation expressions after the c7 serializer refactor.
- Restored the intended independent keys: `bit18Rx0/1`, `bit19Rx0/1`, `bit47Rx0/1`, `immediateTxOk/Fail`, and `periodicTxOk/Fail`.
- Added a regression that rejects `String(...)` concatenation inside numeric JsonWriter calls so this class of refactor error cannot recur.
- No CAN runtime, R79 policy, dashboard key contract, NAG/Mode H, Summon, TLSSC, S3XY, or capture behavior is intentionally changed from c7.

---

# T2CAN Universal v3.6 c7

## Binary size optimization — shared JSON/API serializer
- Reworked high-cost dashboard/API JSON builders around a shared out-of-line `JsonWriterArduino` emitter instead of independently compiling repeated Arduino `String` numeric/boolean/string concatenation sequences in every endpoint.
- Converted the largest status builders first (`nag`, `summon`, `0x3F8`, R79, system, research capture, Mode H LAB, and Driver Monitoring), then expanded the same serializer to Auto Blinker, DAS telemetry, TLSSC Green-Light, CAN traffic, HOME fast/slow/live/full snapshots, Settings/LAB lite snapshots, PedalMap, S3XY status, vehicle-profile status, Wi-Fi AP status, and NAG configuration output.
- Added nested-object/array support so HOME and S3XY payload structure is preserved while sharing one formatting implementation. S3XY device and scan-result objects use the same emitter without changing registry, BLE, action, or connection logic.
- Replaced the repeated `/api/snapshot` group-dispatch chain with a compact static builder table while preserving group names and response order.
- Added API-key preservation regression generated from the v3.6c6 baseline plus host tests for flat and nested JSON emission. No API key is intentionally removed or renamed.
- CAN runtime, injection policy, NAG/Mode H engines, R79, Summon, TLSSC behavior, capture logic, and dashboard source are not intentionally changed by this pass.
- Exact ESP32-S3 `.bin` reduction is intentionally not claimed until v3.6c7a1 is compiled with the same Arduino target settings and compared against the supplied v3.6c6 `.map/.elf/.bin` baseline.

---

# T2CAN Universal v3.6 c6

## Binary size optimization
- Reworked user-code numeric formatting/parsing around **integer fixed-point** values. NAG torque, injected torque, steering angle, Mode H LAB telemetry, driver-monitor EPAS torque, research geometry, and capture utilization no longer require float/double formatting in the project runtime/API path.
- Replaced the Mode H LAB `strtod()` parser with a no-float decimal/scientific parser that preserves the normal web/API input forms and nearest-centi-Nm rounding. Removed the confirmed write-only `lastModeCTorqueNm` field and fixed `steeringScale` / `steeringOffset` storage.
- Added `T2CAN_SERIAL_DIAGNOSTICS` as a compile-time switch, **OFF by default** for production. Serial logging can be restored with `-DT2CAN_SERIAL_DIAGNOSTICS=1` without changing functional CAN behavior. Diagnostic-only boot reset decoding, heartbeat accounting, NAG TX-failure rate limiting, and AP-IP lookup are also excluded from the default build.
- Tightened the existing `S3XY_DIAGNOSTICS_ENABLED=0` build path so diagnostic-only temporary `String` construction and GATT-enumeration work are excluded rather than merely passing their finished strings into a no-op logger.
- Kept `dashboard_source.html` readable and unchanged while adding a deterministic production embed step that minifies CSS / safe outer whitespace before gzip. Embedded dashboard gzip is reduced from **81,039 B to 77,904 B** (**-3,135 B / -3.87%**) in the generated `index_html.h`.
- No intentional change to CAN routing, NAG/Mode H policy, R79, Summon, TLSSC, S3XY actions, capture behavior, dashboard API keys, or LAB feature behavior.
- The exact ESP32-S3 `.bin` delta is intentionally not claimed without an Arduino/ESP-IDF target build and linker map.

---

# T2CAN Universal v3.6 c5

## TLSSC 0x25D DLC=6 hotfix
- Corrected the YL/Party `0x25D APP_trafficControl` experiment to use the observed **DLC=6** frame instead of incorrectly requiring DLC>=8.
- Test 1 and Test 2 now preserve and retransmit the live **6-byte same-bus template**. An unrelated DLC=8 `0x25D` is rejected fail-closed.
- LAB raw telemetry now displays exactly the six valid bytes for stock and injected `0x25D` frames.
- Added regression coverage using the captured GREEN+STOPPING frame `13 1C 3C 02 06 04`: Test 1 -> `1B 1C 3C 02 06 04`; Test 2 -> `1B 1C 3C 12 07 04`.

---

# T2CAN Universal v3.6 c4

## TLSSC Green-Light causal experiments
- Added **LAB → TLSSC Green-Light Experiment** for controlled 0x25D `APP_trafficControl` research on profiles where CAN A is physically/logically **PARTY**. The experiment is **RAM-only**, defaults OFF on every boot, and is forced OFF when LAB or TLSSC is turned OFF.
- Added mutually exclusive **TEST 1 · StateMachine only**: when the live stock 0x25D frame is `FeatureState=ACTIVE`, `ControlType=TRAFFIC_LIGHT`, `LightState=GREEN`, and `StateMachine=STOPPING`, the same-bus overlay changes only `APP_tcStateMachine` to `CONTINUING`.
- Added mutually exclusive **TEST 2 · Pedal tuple**: under the identical strict gate, the overlay sets `StateMachine=CONTINUING`, `ContinuationReason=USER_INPUT_ON_GREEN`, and `ConfirmationType=PEDAL`, matching the tuple observed in the supplied accelerator-confirmation captures.
- Both tests require **LAB ON + TLSSC ON + AP active + PARTY CAN** and respect the existing TLSSC Highway and NOA blocks. RED, YELLOW, stop-sign, inactive-feature, and already-CONTINUING frames are never modified.
- Added live LAB telemetry for stock 0x25D state/light/reason/confirmation, raw stock frame, last injected frame, RX/eligible/blocked counters, and TX success/failure. Selecting a test mode starts a fresh experiment counter session.
- 0x25D is treated as a live same-bus template; all non-target bits are preserved byte-for-byte. No application checksum/counter is synthesized because the referenced Party-CAN DBC does not define one for `APP_trafficControl`.

---

# T2CAN Universal v3.6 c3

## Conservative cleanup
- Removed confirmed unused production helpers and constants with zero active runtime references, including stale vehicle-profile short-name data, an unused Summon bus-name formatter, unused Mode H v1 compatibility helpers, legacy Driver Monitoring bus aliases, and unused ULC/0x293 symbolic constants.
- Removed the pre-b14/pre-b17 Confirm-Free compatibility wrappers. Active regression tests now exercise the current profile-aware bus selector and timing-aware AP gate directly. Runtime Confirm-Free behavior is unchanged.
- Updated the legacy Mode H regression to use the explicit Rev.1 Plus factory directly instead of keeping a production-only default wrapper alive. Rev.1 / Rev.1 Plus / Rev.2 / Rev.3 behavior is unchanged.
- Removed the historical `backup/mode_h_v1` source snapshot and five `*.retired` test files from the shipping source package.
- No CAN routing, injection timing, NVS migration, dashboard API contract, R79 policy, Mode H behavior, capture behavior, or S3XY behavior is intentionally changed from v3.6c2.

---

# T2CAN Universal v3.6 c2

## Mode H Rev.3 — Human Interaction + Natural Grip
- Added **Mode H Rev.3**, based on the existing **Rev.1 Plus** event engine. Rev.1 Plus peak/timing behavior remains the event foundation while Rev.3 adds a low stock-relative Natural Grip carrier during `WAIT`.
- Rev.3 WAIT carrier defaults to **+0.30–0.40 Nm relative to the live stock torque**. The carrier magnitude is held for a short bounded dwell instead of changing every frame, then refreshed to keep the grip feel irregular but continuous.
- Added carrier direction policy: **FOLLOW STOCK** follows the sign of the live stock torque; **RANDOM + / −** chooses and holds an independent random sign for each carrier decision.
- Added Rev.3 Hands-On policy with three selectable modes: **ALWAYS HO=1**, **THRESHOLD HO=1**, and **TIERED HO=1 / HO=2**. Default tiered thresholds are **0.50 Nm → HO=1** and **1.60 Nm → HO=2**, both configurable in LAB.
- Rev.3 writes the complete 2-bit hands-on field when overriding it instead of OR-ing a single bit. Generated HO is explicitly capped at **HO=2**; **HO=3 is never generated**. Below the first threshold in threshold/tiered modes, the accepted stock HO field is retained. Existing protection against real stock `ho > 1` remains in the common NAG admission path.
- Rev.3 settings are NVS-persistent and have their own peak, timing, carrier, direction, HO policy, and HO-threshold keys. Existing Rev.1 / Rev.1 Plus / Rev.2 profiles are preserved.

## R79 Periodic Refresh policy
- Added three NVS-persistent periodic refresh modes under **LAB → R79 Control**: **ALWAYS**, **SUMMON ONLY**, and **OFF**.
- The new policy affects only the independent periodic R79 scheduler. The existing stock-triggered immediate 0x3FD reassert path remains active in every refresh mode and still does not reset the periodic timer.
- **SUMMON ONLY** allows periodic refresh only while the existing confirmed Summon session state is active. **OFF** disables periodic refresh without disabling immediate stock-follow reassertions.

## Dashboard / persistence
- Added **REV.3** to Settings → Nag Killer and expanded the profile selector to a mobile-friendly 2×2 layout.
- Added Rev.3 LAB controls for WAIT carrier range, carrier direction, HO policy, HO=1 threshold, and HO=2 threshold.
- Added R79 Periodic Refresh Mode selector and live mode telemetry.
- NAG configuration schema version advances to **12**; older installs receive Rev.3 defaults without changing their currently selected legacy Mode H profile.

## Preserved from v3.6c1
- ULC / Confirm-Free targeted RAW capture, all b23/c1 CAN routing behavior, Confirm-Free/0x293 research paths, Auto Blinker, TLSSC, S3XY, PedalMap/AP Drive Profile, and Driver Monitoring Capture remain present.

---

# T2CAN Universal v3.6 c1

## ULC / Confirm-Free targeted RAW capture
- Added **ULC / CONFIRM-FREE** as a dedicated CAN Research Capture mode for the YL/Standard cross-platform state-machine investigation. The mode is strictly **RX-only** and watches only `0x247`, `0x3F8`, `0x3E9`, `0x24A`, `0x3FD`, and `0x293`.
- The same six-ID filter is applied independently to **physical CAN A and CAN B**. Missing IDs remain valid evidence; the firmware does not assume that Model 3 Tesla CAN Explorer packing or bus placement is valid on Model YL.
- Capture timing is fixed at **PRE 3 s + POST 7 s**. The existing RAW rolling ring and segmented archive are reused, so completed captures remain downloadable without adding another CAN observer/task.
- Research CSV now preserves **physical_bus** (`CAN A` / `CAN B`) separately from the active profile **bus_role** (`PARTY`, `VH`, `BODY`, `CHASSIS`, etc.) for both RAW and snapshot research exports.
- ULC / CONFIRM-FREE intentionally stores raw bytes only; `0x247` fork/lane-change-state and other candidate-signal decoding is performed offline so unverified YL packing is not baked into firmware behavior.
- No 0x293 injection change is included in c1. In particular, the planned stock-ON suppression removal remains deferred; CAN injection, NAG, R79, Auto Blinker, 0x3F8 Confirm-Free routing, and other b23 runtime behavior are unchanged.

---

# T2CAN Universal v3.6 beta 23

## Quick Controls profile-visibility regression fix
- Fixed a mobile CSS cascade regression that could re-expose a Quick Controls tile after JavaScript correctly marked it `hiddenByProfile`. The stale 2×2 mobile skin still forced `.quickMini { display:block!important; }`, which outranked the global `.hiddenByProfile { display:none!important; }` contract because the mobile selector was more specific.
- Removed the stale forced `display` declaration instead of adding another selector-specific hide workaround. The later one-row Quick Controls layout remains authoritative.
- Body + Chassis now keeps unsupported **Nag Killer** hidden, leaving **Auto Blinker + TLSSC** in one two-column row. Party + Chassis keeps unsupported **Auto Blinker** hidden, leaving **Nag Killer + TLSSC** in one two-column row. Model YL keeps its supported three-control row.
- Strengthened the dashboard regression test so **any** `display:* !important` declaration on the mobile Quick Control / nav element itself fails the visibility contract; the previous test only rejected `display:flex!important` and missed `display:block!important`.
- No CAN routing, injection, NAG engine, 0x3F8/0x293 research logic, Driver Monitoring Capture, S3XY, or R79 behavior is changed from v3.6b22.

---

# T2CAN Universal v3.6 beta 22

## Independent 0x293 TX target
- Added an NVS-persistent **0x293 TX Target** selector under LAB → Confirm-Free / Auto Lane Change research. `0x3F8` Confirm-Free and `0x293 UI_chassisControl` can now be routed independently.
- 0x293 target choices are **CAN A**, **CAN B**, or **BOTH**. Labels follow the active profile roles (`BODY / PARTY / CHASSIS / VH`). `BOTH` is the default and preserves v3.6b21 behavior.
- Stock 0x293 is still observed independently on both physical buses. Injection remains same-bus only: the selected bus must provide its own live stock 0x293 template; cross-bus copying is never used.
- Non-selected-bus frames remain RX-visible for comparison but are never transmitted and do not increment AP-gate-blocked counters.
- This enables the requested research combination **0x3F8 → CHASSIS / 0x293 → BODY** on Body+Chassis profiles without changing the 0x3F8 selector.
- Existing AP-active gate, 0x293 counter/checksum refresh, NVS enable state, and all v3.6b21 Driver Monitoring behavior are preserved.

---

# T2CAN Universal v3.6 beta 21

## Driver Monitoring Capture — all vehicle profiles
- Expanded **LAB → Driver Monitoring Capture** from Model Y L only to every supported vehicle profile. The recorder remains strictly **RX-only** and adds no CAN TX/injection path.
- The capture observer now watches the existing research IDs `0x389`, `0x5D9`, `0x247`, `0x399`, and `0x370` on **both physical CAN A and CAN B** instead of assuming the Model Y L VH/Party placement.
- Removed the Model Y L profile gates from capture initialization, observation, manual A–F trigger requests, post-window ticking, reset, and CSV export. When LAB is enabled, Driver Monitoring buffers are allocated regardless of the selected vehicle profile.
- Live telemetry records the physical side on which each tracked ID was most recently observed and labels it with the current profile bus role, for example `CAN A · BODY`, `CAN A · PARTY`, `CAN B · CHASSIS`, or `CAN B · VH`.
- CSV export now includes separate `physical_bus` and `bus_role` columns for every raw record. This allows Standard Model 3/Y captures to be compared without assuming that an ID uses the same bus placement as Model Y L.
- Existing decoded fields (`DAS_driverInteractionLevel`, DAS hands-on, EPAS hands-on, EPAS torsion-bar torque) are retained as **best-effort by CAN ID** when the corresponding frame is actually observed. Raw bytes and the recorded physical bus remain the authoritative capture data.
- Updated LAB copy from `YL ONLY` to `ALL VEHICLES · READ ONLY`; the Driver Monitoring LAB row is no longer hidden on non-YL profiles. Fixed PRE 2 s + POST 5 s capture behavior and A–F experiment labels are unchanged.

## Preserved from v3.6b20
- Mode H Rev.1 / Rev.1 Plus / Rev.2 profiles and Settings selector.
- AP Drive Profile production Settings feature, Confirm-Free Pre-AP experiment, Quick Controls alignment, S3XY fixes, R79 policy, and existing CAN Research Capture behavior.

---

# T2CAN Universal v3.6 beta 20

## Mode H — three selectable profiles
- Reintroduced Mode H as three NVS-persistent profiles selectable directly in **Settings → Nag Killer**. Switching a profile resets only the active Mode H session and starts the newly selected engine; Mode A/B/C are unaffected.
- **Rev.1** restores the b18 Human Interaction defaults: `WAIT → RAMP_IN → INTERACT → RAMP_OUT → REFRACTORY`, primary peak **1.50–2.00 Nm**, existing timing, correction behavior and 100% HO override.
- **Rev.1 Plus** keeps the b19 Human Interaction lineage but moderates the default primary peak to **1.50–2.20 Nm**. WAIT/RAMP/INTERACT/REFRACTORY timing and HO policy remain identical to Rev.1.
- **Rev.2** restores the Natural Grip concept with a new fixed b20 waveform: balanced **±0.15–0.45 Nm** normal hold, occasional excursions to **±0.65 Nm**, periodic **±1.80–2.20 Nm** interaction peaks every **3–6 s**, and total interaction duration **0.33–0.70 s**.
- Rev.2 peak direction uses a bounded five-event bag: each complete block contains exactly **4 negative peaks + 1 positive peak**, with the positive slot shuffled per block. The low Natural Grip hold remains direction-balanced. This avoids relying on long-run RNG convergence to approximate the requested 80/20 peak-direction bias.
- Rev.2 deliberately does **not** restore the b18 PRE-WARN/VISUAL reactive recovery branch or its curve/ALC tap suppression. In b20 it is a single known Natural Grip waveform admitted by the common Mode H AP/speed/real-driver protections.

## Mode H Settings / LAB UI
- Added a compact three-way segmented selector under Mode H: **REV.1 / REV.1 PLUS / REV.2**. HOME Quick Controls identifies the selected profile as `Mode H · R1`, `Mode H · R1+`, or `Mode H · R2`.
- Mode H summary copy and badges update immediately with the selected profile.
- LAB is profile-aware: Rev.1/Rev.1 Plus expose the existing Human Interaction peak/timing/HO tuning; Rev.2 shows a concise fixed-profile monitor for hold, excursion, peak, interval, duration, direction bias, live phase/event and engine output.

## OTA / persistence
- Added NVS `nag/hv` for the Mode H profile. Existing b19 installations without the key default to **Rev.1 Plus** so the Human Interaction engine remains selected after OTA.
- Existing b19 `h1*` tuning keys become the Rev.1 Plus namespace. Only the untouched b19 **1.80/2.40 Nm** pair migrates to the new **1.50/2.20 Nm** Rev.1 Plus default; customized peak pairs remain unchanged.
- Rev.1 receives an independent `hr1*` tuning namespace with **1.50/2.00 Nm** defaults. Rev.2 uses fixed b20 profile values rather than persistent LAB tuning.

## Preserved from v3.6b19
- Quick Controls mobile centering fix.
- AP Drive Profile production Settings feature and the b18 regen-update JavaScript scope fix.
- Confirm-Free Lane Change AP-only / Pre-AP timing experiment.
- S3XY action-map preservation/API validation, R79 policy, PedalMap session behavior, profile-dependent visibility, and existing CAN routing contracts.

---

# T2CAN Universal v3.6 beta 19

## Quick Controls alignment
- Fixed the mobile Quick Controls switch alignment regression. The final compact mobile toggle rule now centers each switch horizontally inside its tile with `margin: 4px auto 0`, preserving the one-row YL and Standard layouts.

## AP Drive Profile — production Settings feature
- Promoted **AP Drive Profile** from LAB to **Settings → Features**. The production panel owns enable/disable plus the persistent regenerative-braking selector: `STANDARD · 20`, `REDUCED · 10`, or `MINIMAL · 1`.
- Removed the LAB authorization requirement from `/api/features/ap-drive-profile`. Supported routes remain Model YL VH/CAN B and Standard Model 3/Y Body+Chassis; Party+Chassis remains unsupported because there is no supported Body `0x334` route.
- Regen changes are accepted while AP Drive Profile is active. The currently selected regen raw value is re-read for every live stock `0x334`, so a successful setting change is applied from the next stock-follow overlay without disabling/re-enabling the feature.
- Fixed the b18 Regen-update failure at its actual front-end root cause. `setApDriveRegen()` lived in the global dashboard script but called `applyFeatures()` / `refreshFeatures()` that were private to the later profile IIFE. Even after a successful backend POST, JavaScript raised `ReferenceError: applyFeatures is not defined`, the local `catch` converted it to the generic `AP Drive Regen update failed` toast, and the selected value appeared not to update. The handler now lives in the same feature-IIFE scope as those functions.
- The dashboard also parses and surfaces the server's actual JSON error text for genuine non-2xx responses instead of reducing every server rejection to the same generic toast.
- Removed the AP Drive Profile controls from LAB/PedalMap. Manual PedalMap remains a RAM-only drive-session override; when AP Drive Profile owns `0x334`, the manual target stays armed in RAM and resumes after AP ownership ends.

## Mode H — single Human Interaction engine
- Removed **Mode H Rev.2 / Natural Grip** from the production firmware, including its alternate engine, warning-feedback recovery path, Current/Stronger preset, revision selector, Rev.2 LAB monitor, and production headers. Mode H is again a single **Human Interaction** event generator.
- Mode H keeps the proven Rev.1 state machine: `WAIT → RAMP_IN → INTERACT → RAMP_OUT → REFRACTORY`. Timing defaults remain WAIT 1.2–3.0 s, RAMP_IN 260–520 ms, INTERACT 700–1300 ms, RAMP_OUT 320–680 ms, and REFRACTORY 0.8–1.8 s. Default HO override remains 100%.
- Increased only the default primary interaction peak from **1.50–2.00 Nm** to **1.80–2.40 Nm**. LAB retains the **1.00–3.00 Nm** tuning envelope so torque magnitude can be tested independently from the state-machine timing.
- Added a conservative NVS migration: only an untouched legacy `1.50 / 2.00 Nm` default pair is upgraded to `1.80 / 2.40 Nm`; any user-customized Rev.1 peak values are preserved. Old Rev.2 selection/preset keys are no longer read or written.

## Preserved from v3.6b18
- Confirm-Free Lane Change AP-only / Pre-AP timing experiment remains unchanged.
- S3XY action-map preservation/API validation, profile-dependent dashboard visibility, R79 policy, PedalMap session behavior, and the existing CAN routing contracts remain unchanged.

---

# T2CAN Universal v3.6 beta 18

## Mode H Rev.2 response A/B + live monitor
- Added **Rev.2 Response Preset** in LAB: `Current · b17` preserves the v3.6b17 Natural Grip torque/cadence values; `Stronger` increases ordinary interaction density and warning-recovery torque without changing Mode H Rev.1.
- Stronger preset: Natural Hold 0.20–0.55 Nm, excursions to 0.80 Nm at 10%, HO delta trigger 0.35 Nm with 150–250 ms pulse, Natural Tap 1.20–1.60 Nm every 3–6 s, PRE-WARN 1.60–2.00 Nm, VISUAL 2.10–2.60 Nm.
- Split Rev.2 tap admission into **Natural** and **Reactive** gates: Natural taps remain blocked during ALC and above 10° steering; PRE-WARN / VISUAL recovery remains available through ALC and normal curves, blocking only above 25° steering. During active ALC, reactive peaks are bounded to **1.80 Nm PRE-WARN** and **2.20 Nm VISUAL/Retry**.
- Added a lightweight **Rev.2 Live Monitor** in LAB showing DAS warning state/source/age, current engine event and peak, engine output/HO, last successful 0x370 TX torque/HO/age, Natural/Reactive gate state, and event counters.
- Rev.2 preset choice is NVS-persistent (`nag/h2preset`). Existing installs default to `Current` so updating does not silently strengthen Mode H.

## Preserved from v3.6b17
- Confirm-Free Lane Change AP-only / Pre-AP timing experiment remains unchanged.
- Mode H Rev.1 / Rev.2 selection, S3XY action-map preservation/API validation, and dashboard visibility fixes remain intact.

---

# T2CAN Universal v3.6 beta 17

- Added an NVS-persistent **Confirm-Free Injection Timing** control for `UI_ulcStalkConfirm` (`0x3F8` byte0 bit1). **AP ACTIVE ONLY** remains the default and preserves v3.6b16 behavior; **PRE-AP + AP ACTIVE** deliberately bypasses only the AP-state gate so bit1 can be cleared on any live selected-bus `0x3F8` before AP engagement and remain cleared while AP is active.
- The PRE-AP experiment keeps the existing same-bus stock-template contract: no synthetic `0x3F8` is generated, no cross-bus template is used, and every unrelated bit is preserved. The selected bus must still provide a live stock `0x3F8` frame before an overlay can be transmitted.
- Added LAB diagnostics for the active timing policy. AP-only mode reports `AP ONLY · OPEN/BLOCKED`; PRE-AP mode reports `PRE-AP · OPEN` and does not increment the AP-gate-blocked counters. This isolates the Legacy HW4 latch-timing hypothesis without changing `0x293`, ULC Speed, ULC Off-Highway, or other research signals.
- Existing v3.6b16 Mode H Rev.1/Rev.2 selection, S3XY mapping preservation/API validation, and mobile profile-visibility cleanup are retained unchanged.
- Regenerated the embedded gzip dashboard and added pure/static regression coverage for AP-only vs PRE-AP Confirm-Free timing, persistence, API/UI exposure, and fail-closed invalid timing values.

# T2CAN Universal v3.6 beta 16

- Split Mode H into two NVS-persistent revisions selectable from NAG Settings. **Rev.1 · Human Interaction** restores the v3.6b13 WAIT → RAMP_IN → INTERACT → RAMP_OUT → REFRACTORY stochastic engine from the preserved source, while **Rev.2 · Natural Grip** retains the v3.6b15 behavior. Existing installations default/migrate to Rev.2 so an update does not silently change the active waveform.
- Restored revision-aware Mode H LAB tuning. Rev.1 exposes primary peak range, WAIT range, REFRACTORY range, and HO=1 override percentage; Rev.2 keeps the current Natural Grip hold/tap/warning tuning. Revision changes reset the active Mode H session before the newly selected engine starts.
- Preserved S3XY button action mappings across vehicle-profile changes. Actions unsupported by the current CAN topology remain stored in NVS and are blocked only at runtime/UI, so returning to a compatible profile restores the original mapping.
- Hardened the S3XY action API: unknown action strings now return HTTP 400 instead of silently becoming NONE, and unsupported-action responses identify the actual requested action rather than always naming Acceleration Mode Toggle.
- Consolidated the mobile profile-visibility contract by removing layout-level `display:flex!important` overrides from Quick Controls and bottom navigation. The single global `.hiddenByProfile` rule is authoritative, while adaptive 2/3/4-column bottom-nav layout remains intact.
- Regenerated the embedded gzip dashboard and added regression coverage for both Mode H revisions, revision persistence/dispatch, S3XY mapping preservation/API validation, and the mobile visibility cascade.

# T2CAN Universal v3.6 beta 15

- Fixed non-YL mobile Quick Controls profile visibility so unsupported controls remain hidden and the supported controls stay on a single row.
- Fixed the mobile DEVICES navigation visibility cascade: when S3XY Bluetooth is disabled, the DEVICES tab now follows the existing profile/runtime hidden state instead of being forced visible by the mobile nav CSS.
- Added adaptive mobile bottom-navigation column counts when DEVICES and/or LAB are hidden.
- Regenerated the embedded gzip dashboard from the corrected `dashboard_source.html`; CAN/BLE runtime behavior is otherwise unchanged from v3.6b14a1.

# T2CAN Universal v3.6 beta 14a1

- Hotfix for Arduino-ESP32 target compilation: Mode H v2 in `can_core.h` referenced `stateMux`, `dasAutoLaneChangeState`, and `dasAutoLaneChangeStateValid` before `vehicle_logic.h` declared them. The shared mutex and ALC snapshot now live in `t2can_core_state.h`, which is included before `can_core.h`, preserving the same runtime state while fixing declaration order.
- Added a compile-order regression contract so this dependency cannot silently regress in host/static validation.
- No Mode H waveform, Confirm-Free routing, CAN timing, or feature behavior changes from v3.6b14.

# T2CAN Universal v3.6 beta 14

- Reworked `Confirm-Free Lane Change · EXPERIMENTAL` bus selection. **Model YL keeps its fixed, real-car-validated CAN-B/VH route with no A/B selector. Every non-YL dual-CAN profile can now select physical CAN A or CAN B in LAB**, with CAN B / Chassis as the default/fail-safe target. Model 3 Highland no longer force-routes 0x3F8 to Party.
- Confirm-Free remains a same-bus stock-template overlay: the selected bus must first provide a live `0x3F8 UI_driverAssistControl`; only `UI_ulcStalkConfirm` byte0 bit1 is cleared, and the modified frame is transmitted back onto that same physical bus. If no selected-bus stock frame exists, no Confirm-Free TX is generated.
- Updated 0x3F8 diagnostics/UI for the generalized selector. LAB reports the actual profile role for CAN A/B (for example Party/Chassis or Body/Chassis), shows `—` until a real stock bit1 has been observed, and fixes the mobile CSS path that could expose a hidden selector on YL. The selected non-YL A/B target remains NVS-persistent.
- Replaced Mode H's long stochastic interaction waveform with **Mode H v2 · Natural Grip**. Natural Hold now moves through small human-like steering-torque targets, normally ±0.15–0.45 Nm with occasional excursions up to the configured ±0.65 Nm, reselecting targets every 180–600 ms with a 70% opposite-sign preference. Transitions mix smooth 80–180 ms moves with 35% quick 30–80 ms rebalances, allowing realistic cross-zero jumps.
- Mode H no longer forces HO=1 in random held blocks. Natural Hold preserves stock HO; a hold-target delta of approximately 0.55 Nm or more creates a short 120–220 ms HO=1 pulse, while all tap events assert HO=1. Existing real stock HO >1 remains a fail-closed driver-interaction gate.
- Added periodic **Natural Tap** events at a default randomized 4–10 s interval with 0.80–1.20 Nm peaks. Removed the old correction-event/direction-persistence/long WAIT-INTERACT-REFRACTORY behavior from active Mode H v2.
- Added DAS warning-reactive Mode H taps. Fresh hands-on state `2 (REQD_NOT_DETECTED)` triggers a 1.20–1.70 Nm PRE-WARN tap; state `3 (VISUAL)` triggers a stronger 1.80–2.40 Nm VISUAL RECOVERY tap. Warning taps default to **90% LEFT / negative torque**, and the LAB visual-recovery range may be tuned up to 3.00 Nm; there is no 1.8 Nm hard cap. A still-active visual warning may receive one bounded retry after 350–600 ms with a 0.20–0.40 Nm boost, never an unbounded retry loop.
- Added profile-aware DAS warning feedback for Mode H: YL uses Party `0x399`; Model 3 Highland / Juniper use Chassis `0x39B` with `0x399` fallback; Legacy 3/Y use the existing Chassis `0x399` path. If warning feedback is unavailable/stale, Natural Hold and Natural Tap continue while warning-reactive taps stay disabled. New tap starts are inhibited during active lane change (ALC states 9/10) or fresh steering angle above 10 degrees.
- Backed up the complete prior Mode H implementation under `backup/mode_h_v1/`, while the full v3.6b13 source package remains the canonical rollback baseline. No unrelated driver-assist feature bits were added in this release.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this package is host/static/JavaScript/gzip validated in this environment.

# T2CAN Universal v3.6 beta 13

- Stabilized `Confirm-Free Lane Change · EXPERIMENTAL` routing for **Model 3 Highland + PARTY + CHASSIS**. The effective `0x3F8 UI_driverAssistControl` source/target is now forced to **CAN A · PARTY** for that exact profile/topology, using the live same-bus stock frame and clearing only `UI_ulcStalkConfirm` byte0 bit1.
- Added continuous CAN-A/Party `0x3F8` observation for that Highland route even while Confirm-Free is OFF, so LAB can show Party RX count, rate, age, raw payload, and stock bit1 before enabling injection.
- Kept every other Confirm-Free route unchanged: Model YL and non-Body+Chassis profiles retain their existing CAN-B route; Body+Chassis retains its selectable CAN-A/Body vs CAN-B/Chassis research target.
- No new driver-assist feature bits were added. This patch is limited to Confirm-Free CAN routing/diagnostics stability.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this package is host/static/JavaScript/gzip validated in this environment.

# T2CAN Universal v3.6 beta 12

- Extended LAB **Mode H Tuning**. `HO=1 Override Rate` is now selectable from 0–100% (5% UI step): selected held-random blocks force HO=1, while all non-selected blocks preserve the stock 0x370 HO bits instead of forcing HO=0. Random decisions are held for 150–250 ms so HO does not reroll on every CAN frame. Default remains 100% for b11-compatible behavior.
- Added LAB Mode H event-cadence controls for `WAIT Min/Max` and `REFRACTORY Min/Max`. These tune Human Interaction event spacing only; the stock-follow 0x370 CAN cadence, counter/checksum policy, AP gate, speed gate, and ramp/interact waveform structure remain unchanged. Existing 1.00–3.00 Nm torque-range tuning remains available.
- Persisted Mode H torque range, HO probability, WAIT, and REFRACTORY tuning in the existing `nag` NVS namespace. Invalid saved timing falls back to the current Mode H defaults as a coherent preset.
- Changed LAB `UI_ulcBlindSpotConfig` injection from AUTOSTEER-only to the AP-active DAS states 3/4/5/6, so the selected blind-spot policy remains eligible during NOA as well. ACC Follow Distance and production bit56 Off-Highway ALC keep their existing AUTOSTEER-only behavior.
- Expanded YL-only read-only **Driver Monitoring Capture** with Party-CAN `0x399 DAS_status` and `0x370 EPAS_sysStatus` alongside the existing 0x389/0x5D9/0x247 set. CSV/live telemetry now includes DAS hands-on raw, EPAS hands-on raw, and decoded EPAS torsion-bar torque. Capture capacity was increased to accommodate the additional higher-rate frames.
- Added **Body + Chassis 0x3F8 Dual-Bus Monitor**. LAB now observes `UI_driverAssistControl` independently on CAN A/Body and CAN B/Chassis and reports RX count, rate, age, raw payload, bit1, payload equality, and bytewise XOR.
- Added a Body+Chassis-only Confirm-Free target selector: `CAN A · BODY` or `CAN B · CHASSIS`. Each route uses that same bus's live stock 0x3F8 as its template, clears only `UI_ulcStalkConfirm` bit1, and transmits back onto the same physical bus. Cross-bus template copying is never used. Default target remains CAN B.
- Model YL keeps the already real-car-verified CAN B/VH Confirm-Free route; outside Body+Chassis the effective target remains CAN B even if an old saved bus value exists.
- Added per-bus Confirm-Free TX/fail/gate-blocked telemetry and last-TX bus reporting.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this package is host/static/JavaScript/gzip validated in this environment.

# T2CAN Universal v3.6 beta 11

- Added LAB `UI_ulcSpeedConfig · EXPERIMENTAL` under `0x3F8 Research`, using the attached T-CAN mapping `0x3F8 bits 50–51`. Selectable values are `STOCK`, `DISABLED (raw 0)`, `MILD (raw 1)`, `AVERAGE (raw 2)`, and `MAD MAX (raw 3)`.
- Added LAB `UI_ulcOffHighway · EXPERIMENTAL` using `0x3F8 bit 15`, with `STOCK / OFF (0) / ON (1)` selection. This is separate from the existing production-BETA `UI_alcOffHighwayEnable` at bit 56.
- Both new 0x3F8 controls are NVS-persistent and only become active while LAB is enabled and DAS is in an AP-active state 3/4/5/6. They share the existing stock-copy CAN-B overlay, preserve unrelated payload bits, and emit no duplicate frame when the selected raw value already matches stock.
- Added per-feature LAB telemetry for stock value, AP gate, TX success/failure, gate-blocked count, and last forced TX value/age.
- Kept the existing `UI_autoLaneChangeEnable · EXPERIMENTAL` 0x293 control unchanged: stock A/B observation, same-bus override, AP-active gate, counter/checksum refresh, and NVS persistence remain as in v3.6b10.
- Preserved legacy 0x3F8 gate behavior: production-BETA bit56 Off-Highway ALC plus ULC Blind Spot/ACC Follow Distance remain on their existing AUTOSTEER-only path. Confirm-Free, ULC Speed Config, and ULC Off-Highway use the AP-active path.
- Added host-testable pure helpers for 0x3F8 `UI_ulcSpeedConfig` and `UI_ulcOffHighway` extraction/application, including a regression check that ULC Speed masking does not overwrite adjacent ULC Blind Spot bits.
- R79, Summon, NAG, PedalMap/AP Drive Profile, Auto Blinker, TLSSC/TLSSC Restore, CAN recovery, Driver Monitoring Capture, and CAN Research Capture behavior are otherwise unchanged from v3.6b10.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static/JavaScript/gzip regression checks only.

# T2CAN Universal v3.6 beta 10

- Expanded `Confirm-Free Lane Change · EXPERIMENTAL` from NOA-only to the existing AP-active DAS states 3/4/5/6 (AUTOSTEER / AUTOSTEER RESTRICTED / NOA / FSD). The NVS-backed user selection and 0x3F8 stock-copy bit1 clear behavior are otherwise unchanged.
- Added LAB `Auto Lane Change Enable · EXPERIMENTAL` directly below Confirm-Free. It observes stock `0x293 UI_chassisControl` on CAN A and CAN B independently and displays `UI_autoLaneChangeEnable` raw bits 24–25 for each bus.
- The new 0x293 toggle is NVS-persistent and TX is AP-gated. While selected and AP is active, any same-bus stock 0x293 whose raw value is not already `1 (ON)` is copied, `UI_autoLaneChangeEnable` is forced to raw 1, the bits52–55 counter is advanced modulo 16, byte7 checksum is regenerated, and the overlay is transmitted back onto the same physical bus.
- 0x293 checksum generation is validated against existing YL Party and VH stock captures; CAN A and CAN B TX success/failure, blocked counters, stock ages/RX counts, and last forced bus/raw/age are exposed in LAB.
- Driver Monitoring Capture remains YL-only and read-only. R79, Summon, NAG, PedalMap/AP Drive Profile, TLSSC/TLSSC Restore, CAN recovery thresholds/task cadence, and existing CAN Research Capture behavior are otherwise unchanged.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static regression checks only.

# T2CAN Universal v3.6 beta 9

- Added `Driver Monitoring Capture · YL ONLY` under LAB as a strictly RX-only research tool. It never transmits CAN and is available only when the active vehicle profile is Model Y L.
- Captures the YL routing observed in real-car logs: `0x389 DAS_status2` from VH/CAN B and `0x5D9 DAS_carLog` plus `0x247 DAS_autopilotDebug` from Party/CAN A.
- Added six fixed experiment labels with identical capture timing: A `FRONT · NORMAL`, B `SCREEN GLANCE`, C `SIDE LOOK`, D `LOOK DOWN`, E `TORQUE + LOOK AWAY`, and F `FRONT · NO TORQUE`.
- Each A–F trigger copies up to the latest 2 seconds of filtered PRE frames and then records 5 seconds of POST frames. Up to 12 segments and 1024 archived filtered frames are retained until reset.
- Added live LAB telemetry for `DAS_driverInteractionLevel`, 0x389/0x5D9/0x247 frame age/count/raw payload, last interaction transition, PRE-buffer readiness, capture state, requests/ignored/dropped, and storage type.
- `DAS_driverInteractionLevel` is decoded read-only from 0x389 start bit 38, length 2: raw 0 `DRIVER_INTERACTING`, raw 1 `DRIVER_NOT_INTERACTING`, raw 2 `CONTINUED_DRIVER_NOT_INTERACTING`; raw 3 remains displayed as reserved.
- Added `Driver_Monitoring_Capture.csv` export with segment, A–F label, PRE/POST phase, relative time, uptime, bus, CAN ID, raw payload, and decoded 0x389 interaction state.
- The recorder allocates a 4 KiB filtered PRE ring plus a 24 KiB archive (PSRAM first, internal-RAM fallback) only when LAB is enabled on Model Y L. Non-YL profiles do not allocate the recorder.
- Existing CAN Research Capture remains independent and unchanged. R79, Summon, NAG, Auto Blinker, Confirm-Free Lane Change, PedalMap/AP Drive Profile, TLSSC/TLSSC Restore, and CAN recovery policy are unchanged from the corrected v3.6b8 baseline.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static regression checks only.

# T2CAN Universal v3.6 beta 8
- v3.6b8 correction: Confirm-Free Lane Change support now follows the CAN B 0x3F8 route on all supported dual-CAN topologies, including Standard Model 3/Y Body + Chassis; NOA-only gating and NVS persistence are unchanged.

- Added a best-effort immediate PedalMap one-shot for every accepted LAB/S3XY acceleration-mode change. The newest real stock 0x334 template is copied, the requested PedalMap is applied, counter/checksum are regenerated, and one CAN A/Body or CAN B/VH TX is attempted immediately before the existing stock-follow overlay continues.
- Selecting STOCK also releases the volatile manual session immediately and attempts one latest-stock 0x334 one-shot when the cached template and CAN gates are valid. AP Drive Profile ownership, Summon load shedding, recovery barrier, and stale-template protection are not bypassed.
- Cached full 0x334 stock templates are invalidated at CAN recovery boundaries while the existing manual drive-session target remains armed exactly as before.
- Added PedalMap diagnostics separating `Immediate one-shot TX OK / FAIL` from `Stock-follow TX OK / FAIL`, while retaining the total TX counters.
- Added LAB `Confirm-Free Lane Change · EXPERIMENTAL` on all supported dual-CAN topologies with 0x3F8 on CAN B (`PARTY + VH`, `PARTY + CHASSIS`, and `BODY + CHASSIS`). The control is stored in NVS and restores its ON/OFF selection after reboot; LAB OFF suspends runtime injection without erasing the saved selection.
- While that experiment is enabled and DAS state is NOA, the live stock 0x3F8 `UI_driverAssistControl` frame is copied and `UI_ulcStalkConfirm` (byte[0] bit1) is forced to 0. Existing ULC Blind Spot / ACC Follow / Off-Highway ALC research gates are not widened.
- Existing Auto Blinker configuration is preserved, but automatic fake-stalk TX paths (0x249/0x3C2) are suppressed only while the saved Confirm-Free selection is operational (LAB ON + supported dual-CAN topology), so the two approaches cannot intentionally drive the same lane-change confirmation event at once. Door Open Lane Change Cancel remains independent.
- Added LAB diagnostics for stock/forced stalk-confirm bit, NOA gate, dedicated 0x3F8 TX success/failure, blocked count, and last forced TX age.
- The `UI_ulcStalkConfirm=0` behavior is not claimed as real-car validated in this release; it is intentionally a LAB experiment.
- R79, Summon, NAG, TLSSC/TLSSC Restore, CAN recovery thresholds/task cadence, and CAN Research Capture behavior are unchanged from v3.6b7 except for 0x334 cached-template invalidation already tied to recovery.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static regression checks only.

# T2CAN Universal v3.6 beta 7

- Expanded LAB `AP Drive Profile` from Model Y L-only to every supported 0x334 topology: Model Y L `PARTY + VH` and Standard Model 3/Y `BODY + CHASSIS`. Standard `PARTY + CHASSIS` remains blocked because the required Body 0x334 route is unavailable.
- Kept the AP-state gate unchanged: the profile owns outgoing 0x334 only in AUTOSTEER nominal/restricted and NOA. FSD remains excluded.
- Added persistent AP Drive Profile regen selection: `STANDARD = raw 20`, `REDUCED = raw 10`, and `MINIMAL = raw 1`. `REDUCED` remains the default for backward compatibility. Body+Chassis regen mapping and raw 1 are explicitly experimental pending paired real-car validation.
- AP Drive Profile continues to force CHILL/Comfort pedal map raw 0 while applying the selected regen byte[2] target; manual PedalMap session ownership remains armed underneath and resumes after AP exits.
- Added AP Drive Profile TX success/failure accounting for the CAN A / Body path in addition to the existing VH/CAN B path.
- Changed LAB `PedalMap Control` so selecting `STOCK / CHILL / SPORT / PERFORMANCE` applies immediately; the former `Apply PedalMap` button was removed.
- Reworked the LAB `PERFORMANCE` control as a visible ON/OFF session toggle. ON arms the volatile Performance session; pressing it again releases the manual session to STOCK. CHILL/SPORT/STOCK selections keep the button state synchronized.
- Preserved the existing PedalMap session rules: manual targets are RAM-only, no PedalMap target is written to NVS, confirmed Park or reboot releases the session, and CAN recovery does not silently erase it.
- Existing S3XY `Acceleration Mode Toggle` and `Performance Mode` behavior is unchanged.
- CAN A/B diagnostics, R79, Summon, NAG, Auto Blinker, recovery policy, CAN Research Capture, TLSSC, and TLSSC Restore are unchanged from v3.6b6.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static regression checks only.

# T2CAN Universal v3.6 beta 6

- Added CAN A / MCP2515 forensic diagnostics without changing CAN A admission, NAG, Auto Blinker, PedalMap, R79, Summon, TLSSC or recovery policy.
- Added a 64-entry rolling CAN A application-TX trace. On a newly observed CAN A BUS-OFF condition (`EFLG_TXBO` or the existing `mcpTxFailConsecutive > 5` fallback), the recent TX window is frozen with MCP result/gate reason, EFLG, TX-fail sequence, CAN A RX age and RX-overflow count.
- Added `SETTINGS -> Diagnostics -> CAN A RECOVERY` and `CAN A TX CSV`. The existing CAN B recovery diagnostics and CAN B TX CSV remain unchanged.
- Added CAN Research CSV download progress in the fixed connection header (`Downloading… XX%`). Research CSV now advertises the number of exported data rows; the dashboard reads the response stream, counts completed CSV data rows and updates the percentage before saving the finished file.
- Dashboard polling is paused while a managed CAN Research CSV download is active so background snapshot requests do not queue behind the synchronous CSV stream. Other native downloads retain the existing lightweight `Downloading…` behavior.
- Final source-level dead-code cleanup: removed four uncalled wrappers/helpers (`autoBlinkerEligibleRequestDir`, `researchCaptureObserveTxParty`, `canTxFreshMaskSnapshot`, `sccm249Crc8` wrapper), the unreachable RAW Party-TX capture bus branch/constants, one unused LAB local, and unused T-2CAN-FD/interrupt/ESP-BOOT pin definitions.
- Kept S3XY diagnostics compile-out code, compatibility/NVS migrations, full diagnostic APIs, CAN task cadence, Research Capture cadence, task stack sizing, R79/Summon behavior and TLSSC Restore unchanged.
- Arduino-ESP32 target compilation still requires the user-side board toolchain; this source package is validated with host/static regression checks only.

# T2CAN Universal v3.6 beta 5

- Zero-behavior cleanup release based on v3.6b4; CAN timing, R79/Summon policy, recovery, task sizing, NVS migration, PedalMap session behavior, AP Drive Profile behavior, and TLSSC Restore are unchanged.
- Removed write-only diagnostic state/counters that had no readers or API/UI consumers, including retired ALC transition mirrors, unused stalkless/door counters, the unused 0x249 timestamp mirror, and an unused CAN Research raw first-frame mirror.
- Reduced the HOME fast snapshot to the fields consumed by the current embedded HOME renderer, removing redundant state copies and JSON serialization from the 250/500/1000 ms hot path.
- Removed dead dashboard `fetchCanTraffic()` code; HOME CAN traffic continues to use the existing consolidated HOME snapshot.
- Removed the Blink panel's redundant 1.5 s `/api/das/stats` polling. `visualDebugRx` is now returned with the existing 800 ms Auto Blinker stats response; the full `/api/das/stats` endpoint remains available for diagnostics/compatibility.
- Removed the retired Mode D/E/F-only HOME rendering branch. Supported A/B/C/H behavior remains AP-state driven. Persisted unsupported NAG mode IDs are still normalized to Mode A by the existing runtime load path.
- No CAN-loop throttling, Summon refresh timing, Research Capture timing, FreeRTOS stack sizing, or compatibility/NVS migration removal is included in this release.

# T2CAN Universal v3.6 beta 4

- Added LAB `PedalMap Control` for direct runtime selection of `STOCK / CHILL / SPORT / PERFORMANCE` using the existing model-aware 0x334 routing. A dedicated LAB `PERFORMANCE` button is also provided for one-tap session activation.
- Reworked manual PedalMap control as a volatile drive-session override. CHILL/SPORT/PERFORMANCE targets are RAM-only, are never written as PedalMap values to NVS, and boot always starts with no manual PedalMap override armed.
- Added confirmed-Park release on both real gear observation paths (0x118 and 0x186). Entering P clears the active manual PedalMap session and returns 0x334 to pure STOCK passthrough.
- Preserved the existing S3XY `Acceleration Mode Toggle` behavior (`CHILL <-> SPORT`, with PERFORMANCE falling back to CHILL on the legacy toggle) and added a separate S3XY `Performance Mode` action that arms raw PedalMap 2 for the current drive session.
- Manual PedalMap targets now remain armed across stock 0x334 echo and transient CAN recovery instead of silently disappearing. Explicit STOCK, positively observed Park, or T-2CAN reboot releases the session.
- Changed AP Drive Profile ownership so AUTOSTEER/NOA may temporarily supersede outgoing 0x334 without deleting the manual drive-session target. When AP Drive Profile exits, the still-armed manual target resumes until Park/STOCK/reboot.
- Standard Model 3/Y Party+Chassis remains unsupported/hidden for PedalMap because Body 0x334 is unavailable on that topology. Existing Model Y L VH/CAN B and Standard Model 3/Y Body+Chassis routing is unchanged.
- TLSSC Restore remains completely untouched.

# T2CAN Universal v3.6 beta 3

- Added a Model Y L-only LAB `AP Drive Profile` toggle. During AUTOSTEER nominal/restricted or NOA, VH/CAN B 0x334 is overlaid to CHILL/Comfort (`UI_pedalMap` raw 0) and the live-validated YL Reduced regen raw (`byte[2] = 0x0A`). AP exit returns immediately to pure STOCK 0x334 instead of forcing Standard.
- Kept the AP Drive Profile disabled/hidden on Standard Model 3/Y until their regen field is independently validated.
- Fixed TLSSC disable behavior: after this firmware has asserted TLSSC, user OFF now sends one explicit 0x3FD mux0 clear (`bit38=0`, `bit39=0`) and then returns to STOCK passthrough.
- Applied the same one-shot TLSSC clear path to AP exit, Highway / Controlled-Access blocking, and the new `Block TLSSC in NOA` option.
- Added `Block TLSSC in NOA` while retaining TLSSC operation in ordinary AUTOSTEER when the option is enabled.
- Added S3XY Button actions `TLSSC Toggle` and `Auto Blinker Toggle`; Web UI and BLE toggles now share the same enable/disable helpers.
- Kept Auto Blinker HOME state semantics unchanged (`STANDBY` outside NOA).
- Simplified HOME Injection State: removed the obsolete Summon injection card and moved CAN A / CAN B health beside the larger R79 TX card. Summon remains monitor/evidence state, not an R79 authorization gate.
- Unified internal submenu motion with the directional page animation and deferred forced polling until the panel animation completes, reducing WebKit/iPhone transition stutter.
- Preserved the v3.6b2 R79 default-on/manual-latch policy and existing vehicle CAN routing.

# T2CAN Universal v3.6 beta 2

- Fixed R79 manual-driving instability: Tesla DAS state 2 is now treated consistently as an AP-OFF/manual state alongside 0/1/8/9/14.
- Added a latched manual D/R suppression state so transient DAS/gear uncertainty no longer makes R79 oscillate ACTIVE/SUSPENDED once manual driving is positively established.
- Manual suppression is released immediately by confirmed AP, confirmed Summon, Park, or Neutral. Temporary unknown/stale state preserves the latch instead of toggling R79.
- Split confirmed Summon (`ACA + latched SPR`) from one-sided remote-start evidence. Fresh ACA or SPR alone may delay entry into manual suppression during remote startup, but never labels the R79 reason as SUMMON.
- Fixed R79 reason precedence: AUTOPILOT now wins over SUMMON if both states are simultaneously visible.
- R79 remains default-on; only positively confirmed manual Drive/Reverse suspends TX.
- Removed the legacy positive Summon authorization, gate-grace, PARK_STANDBY/SUMMON_FULL priority state, destructive queue-flush/retry telemetry, and retired R79 pending/fresh-mask helpers.
- Simplified CAN-B load shedding to one rule: reserve queue headroom only while the confirmed Summon session is active.
- Renamed priority-specific gear state to generic 0x118/0x186 gear state and made the R79 runtime snapshot read-only.
- Decoupled the generic CAN TX recovery barrier helpers from Summon-specific pure helpers.
- Reduced `injectSummon()` to its remaining responsibility and renamed it `injectUlcSnooze3fdMux1()`.
- Removed obsolete Summon diagnostics/API/dashboard fields and replaced them with confirmed session, source freshness, remote-start evidence, manual-latch, load-shedding, queue, and non-R79 shed telemetry.
- NAG Modes remain A / B / C / H only. Mode D/E/F remain removed.
- Existing vehicle-profile routing is unchanged for Model Y L and Standard Model 3/Y.

# T2CAN Universal v3.6 beta 1

- Hotfix: fixed the Arduino compile-order error where `t2can_forward.h` referenced `R79RuntimeStatus` before the type was declared.
- Removed experimental NAG Modes D, E, and F from the firmware runtime, presets, API selection path, diagnostics, and dashboard.
- NAG mode selection is now A / B / C / H only. Existing public/persisted IDs remain A=0, B=1, C=3, H=7; legacy stored IDs 2/4/5/6 safely fall back to Mode A.
- Removed D/E/F-only portable gating, cadence-learning, EPAS-faithful waveform, and cadence diagnostics while retaining Mode H exact-echo handling.
- Reworked R79 control from positive authorization gating to a default-on transmitter policy.
- After the first real 0x3FD mux1 template is captured, R79 TX remains active by default.
- R79 TX is suspended only when manual Drive or Reverse is positively confirmed while AP and Summon/remote-control state are inactive.
- Summon/remote-control evidence takes precedence over D/R so Smart Summon can drive forward or reverse without suppressing R79.
- Park, Neutral, AP, Summon, unknown gear, and stale/invalid state keep R79 TX active.
- Stock 0x3FD mux1 RX triggers immediate V2.6-like bounded direct TWAI reassertion.
- Independent periodic R79 refresh remains separate from immediate TX; immediate reassertions never reset the periodic clock.
- Removed R79-specific recovery-epoch, fresh-mask, barrier-mutex, pending/coalescing/retry, and positive-gate dependencies from the active R79 TX path.
- Retained minimum transport safeguards: TWAI must be ready and administrative TX hold must be clear. CAN-B TX trace remains active.
- Preserved existing model-specific Gear/ACA/AP/SPR source routing for both Model Y L and Standard Model 3/Y.
- Updated HOME/LAB dashboard terminology from AP/R79 GATE to R79 TX states: ACTIVE, SUSPENDED, WAIT TEMPLATE, CAN OFFLINE, and ADMIN HOLD.
- Dashboard exposes the current R79 reason/gear and manual-suspension counters while preserving the approved neutral-charcoal automatic dark theme.

# T2CAN Universal v3.5a1

- R79/Summon transport rollback toward proven Summon-Unlock V2.6 behavior.
- Real 0x3FD mux1 now triggers immediate direct R79 TX when authorized.
- Restored independent 500 ms periodic R79 timer; immediate TX does not reset it.
- Removed active R79 pending/coalescing/retry service.
- Removed destructive SUMMON_FULL TX queue flush/retry.
- Preserved existing model-specific Gear/ACA source routing.
- Added complete dark-mode overrides for the v3.5 mobile dashboard.

# T2CAN Universal Changelog

## v3.5

- Based on the compile-verified v3.4 beta 7 source package.
- Replaced the dashboard presentation layer with the supplied Claude mobile design and subsequent approved mobile refinements.
- Mobile-only portrait layout with the existing HOME / DEVICES / SETTINGS / LAB structure.
- HOME TORQUE metric is centered; Off-Highway ALC was removed from HOME Quick Controls and remains available in Settings.
- Nag Killer Settings now uses compact Mode A–F selectors, a highlighted/recommended Mode H card, and a per-mode behavior summary.
- Legacy Advanced Parameters / torque-table controls remain wired for compatibility but are hidden from the normal dashboard UI.
- Removed the user-facing Custom NAG mode; legacy persisted mode value 2 migrates to Mode A while Mode C–H numeric IDs remain unchanged.
- Banned Car controls use a high-visibility red danger surface.
- Mode H primary-event LAB range is 1.00–3.00 Nm; production default/reset remains 1.50–2.00 Nm.
- Mode H LAB timing display uses the v3.4b7 NORMAL preset values (WAIT 1.2–3.0 s, REFRACTORY 0.8–1.8 s).
- Preserved existing DOM IDs, firmware API endpoints, controls, and backend wiring.
- Added directional page/panel transitions and preserved reduced-motion fade behavior.
- Removed all standalone demo/mock API code before firmware embedding.
- HOME AUTOSTEER / NOA engaged-state title uses the dashboard green token (#10A967); FSD and other state colors are unchanged.
- Mobile fixed header content height reduced from 52 px to 26 px; page/panel offsets follow the same shared header-height variable.
- HOME status tiles reduced to 53 px and Quick Controls to 64 px while preserving their approved typography.
- AUTOPILOT state title is 40 px; AUTOSTEER / NOA retain the engaged green treatment.
- Injection State cards are approximately 10% shorter via spacing/row compaction without reducing their text sizes.
- No GitHub workflow or binary build is included in this source-only package.

### Dashboard dark theme refinement
- Rebuilt dark mode around a neutral charcoal palette for higher contrast and lower visual noise.
- Removed remaining light-theme surfaces from the mobile HOME, bottom navigation, Quick Controls, setup flow, sheets, forms, and status cards.
- Added body-scoped dark color tokens to override the higher-specificity mobile readability skin on iPhone/WebKit.
- Added explicit dark overrides for `#bottomNav` state-class combinations and `#homeQuickStrip` column-state combinations so light borders/backgrounds cannot win the CSS cascade.
- Dashboard-only change; CAN, NAG, Summon/R79, BLE, and vehicle logic are unchanged.


## v3.7 compile repair — R79 telemetry / administrative TX hold
- Removed stale `r79LabTxKindName()` cases for retired R79 experiment TX kinds (`PRE_MUX1`, `POST_MUX2`, `LEGACY_MIRROR`, `V26_PERIODIC`, `MUX0_COLLISION`). v3.7 runtime only defines `IMMEDIATE`, `PERIODIC`, and `RETRY`.
- Restored `setCanTxAdministrativeHold(bool)` in `can_core.h`. The helper serializes hold transitions with the CAN TX barrier mutex so profile/reset/feature state changes cannot race an in-flight application TX.
- Added regression coverage so retired R79 TX-kind symbols cannot reappear and the administrative-hold helper must exist before `web_api.h` is compiled.
- Firmware version remains `v3.7`.
