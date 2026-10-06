# T2CAN Universal v3.21.0 — Validation

## LAB R79 manual-driving injection — 2026-10-05
- Final completion passes **128/128 Python/static files**, **63/63 C++ host binaries**, and **14/14 JavaScript/browser files** on the frozen production source.
- Valid RED was confirmed before implementation for manual D/R authorization: the unchanged policy still blocked a request with the proposed permission enabled. Final pure and extracted-runtime tests cover D and R, permission OFF/ON, both AP-control modes and master states, immediate/periodic/retry paths, cached authorization, runtime status and the fast reactive gate.
- `Allow R79 During Manual Driving` is persisted in `r79/apctl` bit2 and defaults OFF for missing and valid older packed values. Extracted NVS tests cover reboot restoration, all AP mode/delay/manual combinations, invalid packed data, barrier acquisition failure, namespace-open failure and write failure. Failed transactions leave live policy and generations unchanged; successful changes immediately recompute cached authorization and cancel pending work under the shared TX barrier.
- ON bypasses only the latched manual D/R clause. AP block/delay and stale/unknown DAS handling still close transmission. Administrative hold, CAN offline, missing/invalid stock, configuration generation, cancellation and final enqueue protections remain effective. The manual latch stays active so turning the option OFF suppresses all authorization consumers immediately.
- Compiled API tests cover **72 valid configuration combinations**, strict required `0/1` parsing for `allowManualDriving`, malformed/missing input, LAB-disabled 403, persistence 503 and actual stats JSON. Source and embedded browser tests cover default OFF, independence from the AP master, reboot restoration, save rollback, late-poll rejection, busy/LAB-disabled controls and 320/390/430px light/dark layouts. Source 320px dark and 390px light renders were visually inspected; physical mobile rendering remains unverified.
- Independent review found one immediate-timeout recovery race: a policy generation could change after failed enqueue but before an unguarded whole-queue flush. A separate valid transport RED reproduced the obsolete-generation flush. The final guarded clear holds the TX barrier and rechecks administrative state, CAN readiness, AP generation and current authorization before clearing. Regression coverage requires flush 0/send 1 after manual permission cancellation and preserves authorized timeout recovery at flush 1/send 2. Re-review found no remaining Critical or Important issue.
- Two dashboard generations are byte-identical: **352,319 source / 343,074 minified / 75,102 Zopfli gzip bytes**. Source SHA-256 `ebfa4c464e842b47a8a48821119e7c17a221fc8f0a167155918823e5850eb118`; generated header SHA-256 `f755357345c3e290289553505a71ecac3a4fe62dbbbc1bc9628682e7afb8c685`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,529,791 / 3,145,728 bytes (48%)**; global RAM **72,288 / 327,680 bytes (22%)**, leaving **255,392 bytes**.
- OTA `releases/T2CAN-Universal-v3.21.0-LP_YL/T2CAN-Universal-v3.21.0-LP_YL.bin`: **1,529,936 bytes**, SHA-256 `c8537def13dadde25c6df040c47ba95741d430830f57998f337b5b1466ac9406`. Deterministic source/full ZIP packaging is produced by the no-flash release workflow. No commit, push, device flash, serial/CAN access, vehicle or road testing was performed.
- Host simulation and target compilation do not establish actual Tesla acceptance or real-time behavior. In particular, frames already accepted by the controller/driver queue before a policy transition cannot be recalled individually.

# T2CAN Universal v3.20.0 — Validation

## Retired 0x3F8 speed experiments — 2026-10-05
- Final completion passes **127/127 Python/static files**, **62/62 C++ host binaries**, and **14/14 JavaScript/browser files**. The two experiments are retired after reported vehicle ineffectiveness; their historical host/browser/build evidence below never established vehicle acceptance.
- Removes the previous `Vision Speed Control` (`0x3F8` bits20–21, `UI_visionSpeedType`) and `Adaptive Set Speed` (`0x3F8` bit39, `UI_adaptiveSetSpeedEnable`) LAB rows, panels, render/save/poll callbacks, HTTP handlers/routes, runtime selections/gates/counters and compositor transforms. Removes `vision_speed_pure.h` and six feature-exclusive runtime/pure/browser test files from this new package, replacing them with removal regressions. Completed older packages retain recoverable source copies.
- Preserves `Vision Speed Control — 0x3FD` and its independent `labv3fd/selection` persistence, five supported profiles/nine valid topologies, fresh AP/stock requirements, selected-bus routing and raw0 no-extra-TX behavior. Its runtime block is unchanged; `can_core.h`, `can_runtime.h` and `vision_speed_control_pure.h` are byte-identical to v3.19.0. The only retained-panel text change removes references to the now-absent 0x3F8 settings.
- Preserves the shared 0x3F8 single compositor and ALC/off-highway/blind-spot/confirm transformations. Shared cancellation generation and standard-ID/DLC8/freshness helpers use generic `lab3f8` names rather than removed-feature names. Final transport admission, actual receive timestamp, 3,000 ms source age, epoch, controller state and cancellation checks remain in place.
- Setup calls `retiredLabSpeedSettingsCleanup()` instead of the removed two loaders, then continues loading the retained 0x3FD setting. Cleanup removes only `lab3f8/visionDisabled` and `lab3f8/adaptiveOff`. Actual Preferences-boundary tests cover missing keys, open failure, deletion failure and next-boot retry while preserving other lab3f8 values, `labv3fd/selection`, feature/ULC settings and BLE pairing. A failed cleanup cannot reactivate removed transforms even when saved ON values remain.
- Valid pre-removal C++ RED reaches the stock-preservation assertion with both retired selectors enabled while four surviving transforms are selected. Final actual-runtime and pure tests verify all **65,536 byte2/byte4 combinations**, original stock preservation and the four remaining field transforms. Runtime extraction tests retain source-age boundaries/rollover, malformed frame rejection, final AP/epoch/cancellation changes and transport holds. Compiled HTTP registration-boundary RED confirms old routes previously existed; final tests require their absence while retained routes remain registered.
- Source and decompressed embedded browser tests require the retired menus/panels/controls to be absent and the retired API paths never to be polled, while 0x3FD navigation, saving, routes and diagnostics continue working. LAB menu and retained panel pass **320/390/430px light/dark** viewport checks. Screenshots: `/private/tmp/t2can-v320-speed-ui-final/`. Source 320px dark and embedded 390px light LAB menus and retained control panels were visually inspected. Physical mobile rendering remains unverified.
- Two dashboard generations are byte-identical: **351,626 source / 342,397 minified / 74,971 Zopfli gzip bytes**. Source SHA-256 `a00020a78e8cc81b68e13a22d8458c4f29200373f415bd80744ec352342b5b94`; header SHA-256 `a634afdffacc31971a37d3992df0e371a30fa0e9247ecd3a95aaa2991f957705`.
- Independent read-only source review found no remaining actionable finding. The first completion run passed Python 126/127, C++ 62/62 and JavaScript 14/14; `test_v36c3_cleanup_static.py` falsely classified four active removal regressions as obsolete because their names contain `retired`. A separate valid classifier RED reproduced the issue. The test-only correction distinguishes explicit archive markers/locations, covers five active names and six archive names, and checks archive directories as well as files. Deprecated API and backup protections remain intact. Focused GREEN, re-review and the final full matrix pass; this prevents the same naming false-positive recurring in later removal work.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,529,199 / 3,145,728 bytes (48%)**; global RAM **72,288 / 327,680 bytes (22%)**, leaving **255,392 bytes**. Host C++ checks use Apple clang **21.0.0**.
- OTA `releases/T2CAN-Universal-v3.20.0-LP_YL/T2CAN-Universal-v3.20.0-LP_YL.bin`: **1,529,344 bytes**, SHA-256 `2dd04e5c0cd6188a57f92783fe2cf8cdf73617b21b41a7f3803919fc8dcf7aae`. BIN strings confirm `v3.20.0` and both retained vision-control routes; old vision-speed/adaptive-speed routes are absent. The old key names intentionally remain only for migration cleanup. Production was reviewed and frozen before compilation; subsequent test-only classifier and documentation changes do not alter this BIN.
- Completed v3.19.0 source/header/BIN hashes still match its validation record. No ZIP/release-note packaging, commit, push, device flashing, serial/CAN access, actual device NVS modification, vehicle or road testing was performed. Vehicle acceptance and braking effects of the retained 0x3FD experiment remain unverified; no new DAS speed/acceleration/AEB command changes were introduced.

# T2CAN Universal v3.19.0 — Validation

## LAB Vision Speed Control — 0x3FD — 2026-10-05
- Final completion passes **127/127 Python/static files**, **63/63 C++ host binaries**, and **15/15 JavaScript/browser files** on the frozen production source. Historical evidence below applies only to the named previous packages.
- Adds an independent, persisted default-OFF request for `UI_enableVisionSpeedControl=0`: standard `0x3FD`, DLC8, MUX1, bit49 (byte6 mask `0x02`). Coordinates are from community DBCs pinned to `dzid26/ESP32-DualCAN` commit `2b4d7404ccbb984d57150e9f12421c30e1327bec`, `dbc/Model3_CH.dbc:6252` and `dbc/Model3_VEH.dbc:12667`. These definitions do not prove vehicle acceptance or compatibility with every software version.
- Model YL retains fixed CAN B/VH. Juniper, Y Legacy, 3 Highland and 3 Legacy default to CAN B/Chassis; CAN A/Body is selectable only with Body + Chassis topology. All five profiles and nine valid profile/topology combinations are covered in host simulation. Source templates and TX remain on the selected physical bus, with no cross-bus fallback. Existing `0x3F8` Vision/Adaptive requests remain independent.
- LAB enabled, supported route, valid fresh AP states 3–6, same-profile/topology/epoch stock within 3,000 ms of actual reception, controller readiness, transport freshness and administrative admission are required. Four final hardware enqueue entrypoints compose only bit49 with existing R79/DMS/ULC/Lane Graph policies. Same-bus Lane Graph and Vision changes share one enqueue; original stock bit49=0 creates no additional TX and reports `STOCK_ZERO`.
- A compiled behavioral RED against unchanged v3.18 production reaches the bit49 assertion, then final GREEN exercises actual production final senders with hardware/RTOS doubles. Pure tests cover all 256 byte6 values, all MUX pages, whole-frame preservation, malformed input, AP gates, 3,000/3,001 ms boundaries and uint32 rollover. Runtime tests cover every profile/topology/bus/AP combination, independent routes, claimed/pending stock priority, final admission changes, failure counters and receive-to-observer delays.
- Actual atomic NVS/load/apply/stats extraction tests cover `labv3fd/selection` defaults and reboot, invalid saved values, packed toggle+bus writes, mutex/open/write failures, profile changes at barrier entry, YL normalization, saved unsupported Body recovery and prepared-send cancellation. API tests cover strict and duplicate arguments, LAB 403, unsupported 409, persistence 503 and both reset paths; BLE preservation remains intact. Existing isolated host fixtures share an explicit new-option-OFF boundary helper instead of duplicating stubs; new feature tests execute the actual implementation.
- Independent read-only review identified a support-after-lock race and a stats transport freshness discrepancy. Both are corrected and covered by regression tests; the stats correction was tested after implementation, not claimed as an additional pre-implementation RED. Diagnostic transport epoch/mask/readiness/hold are captured under a nonblocking shared TX barrier; snapshot failure reports WAIT_STOCK with gate closed. Root-added tests also cover lock contention, administrative hold and controller-not-ready diagnostics. Final review found no remaining actionable source finding. Already-enqueued hardware/driver frames are not selectively retractable.
- Source and decompressed embedded browser tests cover default OFF, reload, independent settings, supported routes, unsupported saved Body recovery, save/read failure rollback, late polling, WAIT_AP/WAIT_STOCK/STOCK_ZERO, and 320/390/430px light/dark bounds and bottom-card scrolling. Screenshots: `/private/tmp/vision-control-ui-final/`. Embedded 390px light and source 320px dark bottom renders were visually inspected against existing LAB styling. Physical mobile rendering remains unverified.
- Two dashboard builds are byte-identical: **360,962 source / 351,485 minified / 75,909 Zopfli gzip bytes**. Source SHA-256 `6a5d7c782d391e3d4d8ca8e0402cf04c50fc4f9931dde52901d635b45f67bcfb`; header SHA-256 `df7a7e04b911403b4cb00e46e5ca718fdc367f10054692279983f19bfdb3b82d`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,534,295 / 3,145,728 bytes (48%)**; global RAM **72,304 / 327,680 bytes (22%)**, leaving **255,376 bytes**. Host C++ checks use Apple clang **21.0.0**.
- OTA `releases/T2CAN-Universal-v3.19.0-LP_YL/T2CAN-Universal-v3.19.0-LP_YL.bin`: **1,534,448 bytes**, SHA-256 `3e6e824130f0e1dee6c37c83f3569f5c4b6f9424b3461c0b71e84bf64fa282b8`. BIN strings confirm `v3.19.0`, `labv3fd`, `selection` and both vision-control API routes. The full host matrix and OTA compile ran after independent review and production freeze; only this documentation changed afterward.
- Completed v3.18.0 is preserved; its dashboard source/header hashes still match its recorded evidence. No ZIP/release-note packaging, commit, push, device flashing, serial/CAN access, vehicle or road testing was performed. Vehicle acceptance, sign-specific slowing and phantom-braking reduction remain unverified. No DAS speed/acceleration/AEB command transforms were added.

# T2CAN Universal v3.18.0 — Validation

## LAB Adaptive Set Speed — 2026-10-05
- Final completion passes **124/124 Python/static files**, **62/62 C++ host binaries**, and **14/14 JavaScript/browser files**. Historical entries below apply only to their named versions.
- Adds a persisted default-OFF independent request for `UI_adaptiveSetSpeedEnable=0` in CAN B `0x3F8` bit39 across all five supported profiles and nine valid profile/topology combinations. This is not compatibility evidence for every Tesla or software version. NVS `lab3f8/adaptiveOff` is independent of `visionDisabled`; setup restores the selection.
- Valid compiled behavioral RED and GREEN cover all 256 byte4 values with selected/gate combinations and whole-frame preservation, raw0 no-op, independent Vision/Adaptive requests and counters, single-frame composition, malformed/extended/RTR inputs, all profile/topology and AP enum combinations, LAB/AP policy, 3,000/3,001 ms age boundaries, receive delays, uint32 rollover, recovery epoch, configuration cancellation, failed locks/NVS and strict HTTP arguments. Production runtime/API/NVS/final admission functions are executed with hardware/RTOS boundary doubles.
- Read-only independent review found a field-counter corner case: after final recomposition removes Adaptive but leaves ALC, a TWAI transport failure must not increment Adaptive failures. An additional runtime RED reproduced LAB closure and AP-freshness expiry cases. The final-composed flag now distinguishes admission rejection from an actual transport attempt; targeted GREEN and the final full matrix pass. Re-review found no remaining actionable finding.
- Source and decompressed embedded browser tests cover default OFF, persistence/reload, save/read failure rollback, late polls, supported profile/topology combinations, LAB/AP gating and independence from Vision. Both panels passed 320/390/430px light/dark bounds checks; screenshots are in `/private/tmp/t2can-v318-adaptive-ui/`. Source 320px dark and embedded 390px light Adaptive renders were visually checked against existing Vision styling. Physical mobile rendering remains unverified.
- Two dashboard builds were byte-identical: **354,806 source / 345,467 minified / 75,277 Zopfli gzip bytes**. Source SHA-256 `2bda48dfedda37b2e08191f042f1bd076c6352cc218279d137fcddc15c28754d`; header SHA-256 `f6208ce90d27b7f3257d81acf8d35a2c3963641b023d7202394acc1a6ec36a35`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,530,655 / 3,145,728 bytes (48%)**; global RAM **72,240 / 327,680 bytes (22%)**, leaving **255,440 bytes**. Host C++ checks use Apple clang 21.0.0.
- Final OTA `releases/T2CAN-Universal-v3.18.0-LP_YL/T2CAN-Universal-v3.18.0-LP_YL.bin`: **1,530,800 bytes**, SHA-256 `2231701091cc1a1a388ae92cee3475b8e9325b7d9e86c247c2dafdfc2351fbb8`. Embedded version `v3.18.0`, `adaptiveOff` and both adaptive-speed API routes were verified in the BIN.
- The first successful BIN is superseded after the reviewed counter correction and retained at `/private/tmp/tesla-v318-superseded-KU3bsQ/`. To avoid repeated full validation/builds, close independent review findings and freeze production sources before launching the completion matrix and OTA compile together; any later production edit invalidates the previous BIN. Final verification here was rerun on the frozen corrected source.
- Completed v3.17.0 is preserved; its dashboard source/header hashes still match the prior validation record. No ZIP/release-note packaging, commit, push, device flashing, serial/CAN access, vehicle or road testing was performed. Vehicle acceptance, sign-specific slowing and phantom-braking reduction remain unverified. No DAS speed/acceleration/AEB command transformations were added.

# T2CAN Universal v3.17.0 — Validation

## LAB Vision Speed Control — 2026-10-05
- Final completion passes **123/123 Python/static files**, **61/61 C++ host binaries**, and **13/13 JavaScript/browser files**. Historical results below apply only to their named versions.
- Adds a persisted default-OFF request for `UI_visionSpeedType=0` in CAN B `0x3F8` bits 20–21 across all five supported profiles and nine valid profile/topology combinations. This is not a claim of compatibility with every Tesla model or firmware version.
- Compiled pre-implementation RED and final GREEN cover byte2 preservation, stock raw0 no-op, LAB/AP gates, 3,000 ms receive-age boundary, uint32 rollover, receive-to-observer delays, malformed frames, transport holds, recovery epochs, configuration cancellation, persistence failures, strict HTTP arguments and simultaneous single-compositor policies. An additional RED/GREEN covers the early-admission TX failure counter.
- Existing country runtime extraction fixtures required the newly referenced vision cancellation generation; repeated scenarios also required resetting this fixture state. Initial full runs reported 121/123 and 122/123 Python files; after those test-only fixes the full completion command passed without exclusions. Production firmware did not change for these fixture repairs.
- Source and decompressed embedded dashboards cover saved-state reload, failed-save rollback, late polling, all supported profile/topology combinations and LAB/AP gating. Layout bounds were checked at 320/390/430px in light/dark themes; representative source and embedded screenshots were visually inspected. Physical mobile rendering remains unverified.
- Two dashboard generations were byte-identical: **350,099 source / 340,884 minified / 74,821 Zopfli gzip bytes**. Source SHA-256 `ad181d1f593597e8453570c1f98ae82b3476d9803056ee0789611d28bf3d9e5b`; header SHA-256 `f8726d7eaddda77d195e453d8d38f6cccc6da3c9f1f89243dd62537255eb4f6c`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,528,135 / 3,145,728 bytes (48%)**; global RAM **72,232 / 327,680 bytes (22%)**. Host C++ checks use Apple clang 21.0.0.
- Final OTA `releases/T2CAN-Universal-v3.17.0-LP_YL/T2CAN-Universal-v3.17.0-LP_YL.bin`: **1,528,288 bytes**, SHA-256 `3e5a6b9684713292349c0297027cda739ece10af0986d260f816761c71c5c8ae`. Embedded version and both vision-speed API routes were verified in the BIN. The pre-counter-fix BIN was moved to a recoverable temporary archive and is superseded.
- Completed v3.16.0 is preserved. No ZIP/release-note packaging, commit, push, device flashing, serial/CAN access, vehicle or road testing was performed. The DBC enum identifies raw0 as DISABLED, but vehicle acceptance, sign-specific slowing effects and phantom-braking reduction remain unverified.

# T2CAN Universal v3.16.0 — Validation

## LAB Lane Graph injection bus — 2026-10-05
- Final verification covers **122/122 Python/static files**, **60/60 C++ host binaries**, and **12/12 JavaScript/browser files**. Completion passed all Python/C++ and 11/12 JavaScript files; the one inherited Settings layout test passed a focused source/embedded rerun after correcting its navigation wait. This test-only correction did not change the compiled firmware or dashboard.
- Valid pre-implementation RED checks cover bus-only HTTP updates, Body selection preserving CAN B Lane bit45, and stale-frame rejection following final-admission preemption. Production functions compiled normally before the assertion failures.
- New regressions cover all mode/bus combinations, non-YL profile/topology support, independent raw caches and same-bus transmission, bit45-only Body transformation, absent/stale stock, AP freshness, maintenance/recovery epochs, route-change races, failed lock/NVS persistence, reboot migration, strict API parsing and selected-bus TX diagnostics.
- YL keeps fixed VH, including normalization of a previously saved Body selection on profile change. Non-YL Party + Chassis with saved Body stays blocked until a supported selection is saved. Body transmission does not depend on CAN B readiness or its RX freshness.
- Body and independent Chassis clones capture timestamps immediately after hardware/driver receive. Their final admission rejects age over 3,000 ms, including delay before the observer and inside the TX barrier; boundary and uint32 rollover regressions pass. Existing R79/DMS/ULC transmission policy remains unchanged.
- Independent source review covered routing, storage, cancellation, diagnostics and the final RX timestamp path. Physical CAN A freshness, YL migration and selected-bus diagnostics findings were corrected; no actionable finding remains in the final scoped review.
- Source and decompressed embedded dashboard checks cover Body/Chassis selection, saved-mode independence, failed-save rollback, late polls, YL selector hiding and unsupported-topology recovery. Lane Graph renders at 320/390/430px in light/dark themes were checked; source 320px dark and embedded 320px light screenshots were visually inspected. Physical mobile rendering remains unverified.
- Browser-test reliability: a controlled 5 ms point in the 200 ms Settings entry animation reproduced a 0.000015258789 px coordinate discrepancy in source and embedded pages. The test now waits for navigation animation completion and checks the settled transform before the unchanged bounds assertion, rather than changing product CSS or loosening the bounds. Boot-ready and visible-panel checks also prevent hidden-page false passes.
- Two dashboard generations are byte-identical: **345,531 source / 336,440 minified / 74,126 Zopfli gzip bytes**. Source SHA-256 `79f92179c450a343643142cfdedbf31d6bc2cdaff7673e8ec351a643eae84eeb`; header SHA-256 `f51b30005ace01df02915114251c86459a1609ccadadb9667c4373d3f2299d61`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,524,419 / 3,145,728 bytes (48%)**; global RAM **72,192 / 327,680 bytes (22%)**. Host C++ checks use Apple clang 21.0.0.
- Final OTA `releases/T2CAN-Universal-v3.16.0-LP_YL/T2CAN-Universal-v3.16.0-LP_YL.bin`: **1,524,576 bytes**, SHA-256 `b573d89cd1349418e7c53a7de037bdffb5f28a5d469581d6acb05a3fab4d2e2c`. Embedded version is `v3.16.0`. The earlier successful build was superseded after the RX-age fix and is not the delivery artifact.
- Original v3.15.0 contents match their pre-work manifest (**270 files**). Final production sources and generated dashboard match the compile-time manifest. Auxiliary Arduino exports are preserved outside the source package. The accompanying source ZIP excludes build/cache artifacts and is checked for deterministic bytes, CRC and member equality.
- No controller flashing, serial/CAN access, vehicle or road test has been performed. Whether the selected physical bus carries the expected stock frame, and whether the vehicle accepts the visualization flag, remain unverified.

# T2CAN Universal v3.15.0 — Validation

## Independent map region and LAB Lane Graph — 2026-10-04
- Final verification covers **119/119 Python/static files**, **59/59 C++ host binaries**, and **11/11 JavaScript/browser files**. The completion lane passed 118 Python, all C++ and all JavaScript files. One new Lane Graph settings host file was added after that run enumerated Python tests; a final focused workflow run passed **1/1**. The final file inventory confirms this was the only additional Python file. Firmware production sources were unchanged during and after the successful target build.
- Valid pre-implementation RED checks compiled successfully and failed at independent Country STOCK + Map US transmission and final Lane Graph TX composition. New pure/runtime regressions cover all country/map combinations, original-byte preservation, AP freshness boundary/rollover, OFF/AP release, ALWAYS without AP, transport gating, prepared-clone preservation, NVS defaults/legacy migration, strict API parsing, and persistence failures.
- Persistence regression exposed save-before-lock ordering: a failed TX barrier acquisition could return failure after changing reboot state. Both new apply paths now acquire the barrier before persistence and publish only after successful writes. Actual mutex-acquisition failure tests confirm unchanged runtime and reboot settings.
- Independent source review covered Country/Map persistence/migration and Lane Graph composition across immediate, delayed, retry, DMS-only and ULC MUX1 paths. Duplicate failure traces discovered during review were removed; trace records use the final transmitted payload.
- New dashboard regression reached an assertion RED before implementation. Focused source/embedded browser verification passes independent Country/Map saves, reload, failed POST/read rollback and stale-poll rejection; Lane Graph OFF/AP ACTIVE/ALWAYS, LAB gating, AP release/staleness and ALWAYS without fresh AP state.
- Source and embedded Country/Lane Graph panels plus Settings summaries are checked at 320/390/430px in light/dark themes. Final embedded Lane Graph 320px dark and Settings 320px light were visually inspected. Existing Playwright browser tests were used because the in-app browser was unavailable. Physical mobile rendering remains unverified.
- Two dashboard builds are byte-identical: **343,735 source / 334,700 minified / 73,758 Zopfli gzip bytes**. Source SHA-256 `b56d8a8f3555b7f271d060d6e4f5b5601c1e9c187e72afd55f4b964e2ff97138`; header SHA-256 `bc237ada1394a66f6c9f43b497deabfd646f7ee94e21349d0b853c5afc846cee`.
- UI review corrected a browser fixture that incorrectly required fresh AP state in ALWAYS mode; regression now distinguishes ALWAYS from AP ACTIVE with stale/unknown AP.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,521,879 / 3,145,728 bytes (48%)**; global RAM **72,136 / 327,680 bytes (22%)**. The first target attempt exposed declaration-order and NVS reset-namespace-count errors, both corrected. A second attempt was interrupted before artifact creation when the persistence-order finding required source changes; the final attempt completed successfully.
- OTA `releases/T2CAN-Universal-v3.15.0-LP_YL/T2CAN-Universal-v3.15.0-LP_YL.bin`: **1,522,032 bytes**, SHA-256 `1c72509d7ab353eae4efea221fea02b2f758ae7431bac40e4b59fafef4c412b4`. The application contains `v3.15.0`. No source/full ZIP or release notes were requested.
- Original v3.14.0 contents match the pre-work hash manifest (**262 files**). CLI-generated auxiliary exports were preserved in a temporary directory outside the source package; the required OTA remains in `releases/`.
- No controller flashing, serial/CAN access, vehicle or road testing was performed. Actual visualization and vehicle acceptance of country/map settings remain unverified.

# T2CAN Universal v3.14.0 — Validation

## Legacy V12/V13 HW3 R79 option — 2026-10-04
- Host verification covers **115/115 Python/static files**, **57/57 C++ host binaries**, and **10/10 JavaScript/browser files**. The completion run passed 114 Python files, all C++ and all JavaScript files; its one remaining historical static assertion (`test_v36d9a2_post_mux1_mode_static.py`) referenced the old two-argument payload call. Updating that assertion to the new policy argument and a focused workflow rerun passed **1/1**. Firmware production sources were unchanged by this final test correction.
- A valid pre-implementation runtime RED compiled normally and failed at stock bit47=0 preservation; the dashboard RED reached its visibility assertion. New pure tests cover all 256×256 MUX1 byte2/byte5 combinations with stock/forced bit47, both Mode 1 bit18 policies and Mode 2 change reporting. Defaults retain FORCE 1.
- Extracted production runtime checks cover both modes' immediate and delayed sends, Mode 1 emergency queue-clear and scheduled retries, newer stock templates, unsupported profiles, and a policy-generation change after payload preparation. Extracted NVS/HTTP checks cover absent keys/default OFF, reboot/reset, both Legacy models and both standard topologies, failed writes, standalone strict scalar parsing, mixed-request rejection, unsupported ON rejection, descriptor cancellation, and AP session preservation.
- Source and decompressed embedded dashboards pass OFF/ON saving/restoration, failed-save rollback, delayed polling and 320/390/430px light/dark layout checks. The entire new card is hidden outside supported Legacy profiles. Source 320px dark and embedded 320px dark / 390px light renders were visually inspected. The in-app browser was unavailable; the existing Playwright host browser was used. Physical mobile rendering remains unverified.
- Two dashboard generations are byte-identical: **338,201 source / 329,293 minified / 72,864 Zopfli gzip bytes**. Source SHA-256 `ee84a9c1bc3588d2bd8b62fcb27da22db9be4b255ba49cf47c4cd714f1d5587d`; header SHA-256 `f5b18611cabedff9dfda5c19863ae7070f4512cb1c61d1c02872197b011415a2`.
- Independent read-only review found no actionable defect within the requested bit47, persistence, cancellation and profile/UI boundaries.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, the 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,517,711 / 3,145,728 bytes (48%)**; global RAM **72,120 / 327,680 bytes (22%)**. Host C++ checks used Apple clang **21.0.0**.
- OTA `releases/T2CAN-Universal-v3.14.0-LP_YL/T2CAN-Universal-v3.14.0-LP_YL.bin`: **1,517,856 bytes**, SHA-256 `4ad5abe76ecbcdc730b87e656547e8243a289089056f94ba946ac257de76aba3`. No source/full ZIP packaging or release notes were requested.
- Original v3.13.0 contents match the pre-work hash manifest (**269 files**). No device flashing, serial/CAN access, vehicle or road testing was performed. This option's effect on actual FSD/R79 behavior remains unverified.

# T2CAN Universal v3.13.0 — Validation

## NAG settings and Country promotion — 2026-10-04
- Final completion validation passes **113/113 Python/static files**, **56/56 C++ host binaries**, and **9/9 JavaScript/browser files**. Historical entries below apply only to their named versions.
- Valid RED was confirmed for the torque AP eligibility option, single Mode H selection, default OFF policy, LAB-independent country runtime, NVS migration/persistence boundaries, HTTP save-failure behavior, dashboard placement and bypass state rendering. The pure AP gate covers 256 input combinations without relaxing non-AP conditions.
- Extracted production NVS/default/load/save and HTTP functions verify missing keys, explicit enabled values, old revision IDs, preserved custom h4 tuning, mode-switch state preservation, strict boolean parsing, failed NVS open/write and input-cleanup rejection. Explicit en/iap keys also survive a missing schema marker. AP opt-in is persisted before runtime publication.
- Torque A/B/C/H alone consume Ignore AP State. AP transitions do not reset the Mode H session while the option is enabled. Actual AP diagnostics remain unchanged; the UI reports BYPASS when AP is inactive. TSL9, right-scroll and DMS AP conditions remain unchanged. A valid speed observation and the existing stop policy are still required by Mode H.
- Country tests retain the stock transforms/routing checks and verify LAB OFF and a LAB transition during TX admission. R79 authorization, mode changes, recovery/maintenance cancellation, NVS countrylab keys and existing country numeric identities remain in effect. New /api/country routes coexist with the legacy aliases.
- Source and embedded dashboards pass 320/390/430px light/dark layout checks, setting persistence/restoration, failed-save rollback, method changes, and country navigation with LAB OFF. Embedded 320px dark and source 430px light NAG/Country screenshots were visually inspected, along with the single Mode H card. Physical mobile rendering is unverified.
- Repeated dashboard builds are byte-identical: **336,003 source / 327,148 minified / 72,369 Zopfli gzip bytes**. Source SHA-256 `f22d4704cfbe5f5905335c18c8df1c34d387c505dc29734ae4073b149d402165`; header SHA-256 `99f548e2e5dff6ef7bf2c52bd509f6b11bd474fd1386b66130d7113cca5f2031`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,515,843 / 3,145,728 bytes (48%)**; global RAM **72,120 / 327,680 bytes (22%)**.
- Final OTA: `releases/T2CAN-Universal-v3.13.0-LP_YL/T2CAN-Universal-v3.13.0-LP_YL.bin`, **1,516,000 bytes**, SHA-256 `ada138ba3978fe933507ae3b8636fd792f46b526225b4acfe3206395c2b5cd0a`. Source/full ZIP packaging and release notes were not requested.
- Test reliability fixes: the layout test now reads the current source instead of a copied historical preview. A controlled late toggle response reproduced the interval test race (45 overwritten by the saved value 30); the test now awaits the saved configuration before editing the next input. No arbitrary timeout increase or unrelated production UI change was used.
- Original v3.12.0 contents match the pre-work hash manifest (274 files). No controller flashing, serial/CAN access, vehicle or road testing was performed. Bench/vehicle confirmation of NAG behavior and country acceptance remains outstanding; local tests and target compilation do not establish those outcomes.

# T2CAN Universal v3.12.0 — Validation

## New Zealand country preset — 2026-10-04
- Fresh completion passes **108/108 Python/static files**, **55/55 C++ host binaries**, and **8/8 JavaScript/browser files**. Historical entries apply only to their named versions.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,529,255 / 3,145,728 bytes (48%)**; global RAM **72,568 / 327,680 bytes (22%)**.
- OTA `releases/T2CAN-Universal-v3.12.0-LP_YL/T2CAN-Universal-v3.12.0-LP_YL.bin`: **1,529,408 bytes**, SHA-256 `b5659edb2eef1b49f799602ea082c1babfc1be2509450e9ede2ec7d55f8d2285`. No source/full ZIP packaging requested.
- Valid RED was confirmed before implementation for pure NZ transformation, extracted runtime routing, HTTP/NVS handling and the dashboard option. Focused country validation passes 3 Python files and 1 C++ binary.
- Coverage includes NZ numeric 554 and GTW NZ little-endian bytes, all 256 values of MUX1 byte1 (including handedness), complete MUX3 preservation/no replacement TX, malformed frames, rolling checksum/counter, 840 runtime routing/mode/race cases, strict mode parsing, NVS reload, save-failure rollback and authorization.
- Source and embedded dashboards pass NZ selection/save/restoration/failure tests and 320/390/430px light/dark checks. Embedded 320px dark and source 430px light screenshots were visually inspected; physical phone rendering is unverified.
- Repeated dashboard builds are byte-identical: **346,035 source / 336,882 minified / 74,475 Zopfli gzip bytes**. Source SHA-256 `650e844cd835c0705e506e639edf39eb1da657b170515578bae72b434d87a0d3`; header SHA-256 `3eccb81a7b04ab0856035c7d0986715690a631d6b9969aa1b9f3ccd599660717`.
- The preset preserves stock mapRegion because no NZ encoding was verified. The ISO code is sourced in CHANGELOG.md; this is not evidence of a Tesla market-specific feature outcome.
- No flashing, live CAN, vehicle or road testing was performed. FSD activation and R79 behavior are not validated by these host checks.

# T2CAN Universal v3.11.1 — Validation

## OTA/reboot maintenance — 2026-10-04
- Fresh completion validation passes **107/107 Python/static files**, **55/55 C++ host binaries**, and **8/8 JavaScript/browser files**. Historical evidence below applies only to its named version.
- Actual OTA/reboot handler regression was observed failing before implementation, then passes with the maintenance barrier. Extracted production shutdown tests exercise ordering, idempotence, supervisor/task/mutex timeout, controller failures, BUS_OFF/RECOVERING handling, startup/self-supervisor calls, and sticky hold release rejection.
- The source and embedded reconnect UI regression passes across 320/390/430px, light/dark and AP/NOA states (24 combinations), including AP updates during the overlay, background restoration and OTA error/abort messaging. Embedded 390px dark and source 320px light screenshots were visually checked. Native iPhone status-bar rendering is not verified by Chromium.
- A pre-existing 200 ms navigation-animation test intermittently missed the animation between browser round trips under build load. Its click and initial observation now share one browser task; production navigation behavior was not changed.
- Repeated dashboard generation is byte-identical: **345,835 source / 336,684 minified / 74,396 Zopfli gzip bytes**. Source SHA-256 `f82db952a9ed0fab54f7b30ef27c205c5703d23eb198034a773f7e1ec9b180fa`; embedded header SHA-256 `b24ba08909171f284f50722cdaaa24194579dabab579ffdc71bdccb229406e13`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program **1,529,087 / 3,145,728 bytes (48%)**; global RAM **72,568 / 327,680 bytes (22%)**.
- OTA `releases/T2CAN-Universal-v3.11.1-LP_YL/T2CAN-Universal-v3.11.1-LP_YL.bin`: **1,529,232 bytes**, SHA-256 `24bbc78b95e8265b11f64ddb04f2458188773a7c2ea2b62c09e454c3a96ea1bd`. No source/full ZIP packaging requested.
- No controller flash, live CAN, vehicle or road test was performed. Shutdown success is checked before flash writes; frames transmitted before the shutdown handshake completes cannot be recalled. An unconfirmed stop refuses OTA, and a software restart waits for confirmed shutdown instead of bypassing it. Watchdog, power-loss and hardware resets are outside this software-request path.

# T2CAN Universal v3.11.0 — Validation

## LAB R79 AP Control — 2026-10-04
- Fresh completion validation passes **105/105 Python/static files**, **55/55 C++ host binaries**, and **7/7 JavaScript/browser files**. Historical results below apply only to their named versions.
- AP control coverage includes default-OFF legacy authorization, block/delay modes, 2–10 second bounds, AP disengage/reengage, stale/unknown DAS states, timer rollover, final-enqueue configuration races, periodic cancellation, strict HTTP validation, failed NVS writes, and reboot restoration using extracted production code.
- Source and decompressed embedded dashboards pass browser checks at 320/390/430px in light/dark themes. Final source screenshots at 320px light and 390px dark were visually inspected. Physical phone testing was not performed.
- Repeated Zopfli dashboard builds are byte-identical: **345,550 source / 336,491 minified / 74,335 gzip bytes**. Source SHA-256: `8c47a58cc0f97b9d09458c345757910de712eddd6beed2bfd74adce5ab64ea03`; generated header SHA-256: `6f42e6cdf51f05d4eb9de2c8572346c7f36d3bcc37cc1a862e257d5145945134`.
- ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, 3 MB app partition, OPI PSRAM and CDC-on-boot. Program: **1,528,179 / 3,145,728 bytes (48%)**; global RAM: **72,560 / 327,680 bytes (22%)**.
- OTA: `releases/T2CAN-Universal-v3.11.0-LP_YL/T2CAN-Universal-v3.11.0-LP_YL.bin`, **1,528,336 bytes**; SHA-256 `290efa31300ad1024349a0627cb52fb0924bf9e081572f248d4dd8fc96adc0fa`. Source/full ZIP packaging was not requested.
- `tools/build_ota.py` successfully produced this artifact using an explicit temporary build directory, preventing the observed sandbox failure at Arduino's default user-cache directory. It preserves existing OTA files and performs no device access.
- Master OFF preserves legacy authorization and payload policy. The shared final-enqueue barrier is new; identical physical real-time timing is not established. Frames already accepted by the driver/hardware queue cannot be individually retracted by this gate.
- No device flash, live CAN, vehicle or road test has been performed. The reported HW3 AP disengagement cause and this feature's effect on that vehicle remain unverified.

# T2CAN Universal v3.10.2 — Validation

## Periodic Interval alignment — 2026-10-04
- Completion passes 101/101 Python files, 54/54 C++ binaries, and 6/6 JavaScript files. The focused alignment check also passes against source and decompressed embedded HTML at 320/390/430px in light and dark themes; both 390px renders were visually inspected.
- Dashboard: 340,853 source bytes / 331,936 minified bytes / 73,436 Zopfli gzip bytes.
- ESP32-S3 compile/link passes with the inherited 16 MB flash, 3 MB application partition, OPI PSRAM and CDC-on-boot configuration. Program: 1,525,275 / 3,145,728 bytes; RAM: 72,552 / 327,680 bytes.
- OTA: `releases/T2CAN-Universal-v3.10.2-LP_YL/T2CAN-Universal-v3.10.2-LP_YL.bin`, 1,525,424 bytes; SHA-256 `1642c7d48047431f4a76d91874891172963c4067c2361b96b004ba0c3472ae25`.
- Physical phone, controller flash, CAN, vehicle and road validation were not performed. Historical evidence below applies to its named version.

# T2CAN Universal v3.10.1 — Validation

## Periodic Interval dashboard input styling — 2026-10-04
- The new mobile browser regression was first observed failing because the TSL9 Periodic Interval input had no dashboard field-card container and rendered with browser-default number-input styling.
- Fresh completion validation passes **101/101 Python/static files**, **54/54 C++ host binaries**, and **6/6 JavaScript/browser tests**. The new regression passes at 320, 390, and 430 pixel viewport widths and is included automatically in the standard dashboard lane.
- The Zopfli dashboard build is **340,575 bytes source / 331,661 bytes minified HTML / 73,377 bytes gzip**, below the strict 100,000-byte ceiling. Dashboard source SHA-256: `7091414a36e7635c6d4ec58fc56bcb23af27a08628f368a5de90ea54bf1696c6`; generated header SHA-256: `e7e66ad9849d0521a2d2a60ff21f7b590f05a6e84b9befd9a61b0dfb66346ab2`.
- Fresh ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,525,211 / 3,145,728 bytes (48%)** and global RAM is **72,552 / 327,680 bytes (22%)**.
- The OTA application BIN is **1,525,360 bytes**, SHA-256 `1fa000f8e19c2cd0f3541ef007f600e34386499da34ecaf0ecef662ee86dedf3`, stored at `releases/T2CAN-Universal-v3.10.1-LP_YL/T2CAN-Universal-v3.10.1-LP_YL.bin`. Source/full ZIP packaging was not requested.
- No controller was flashed. Live CAN, vehicle behavior, and road validation are unchanged and remain unverified hardware boundaries.

# T2CAN Universal v3.10.0 — Validation

## Universal LAB Country / Map Region — 2026-10-03
- Fresh `firmware_workflow.py completion` validation passes **101/101 Python/static files**, **54/54 C++ host binaries**, and **5/5 JavaScript/browser tests**. Historical results below apply only to their named versions.
- Country regressions cover all five profiles and nine valid topology combinations, STOCK/US/KOREA, byte preservation, checksum/counter updates, malformed frames, cancellation generations, recovery epochs, and the final CAN-B-ready check before CAN A transmission. The runtime harness exercises 720 routing/mode/race cases and 180 malformed-frame cases using extracted production handlers and guarded transports.
- Editable and embedded dashboards pass browser tests, including 320/390/430-pixel layouts. The test launches independent browser instances for source and embedded checks and serves the actual embedded font to avoid a single-process Chromium context-reuse failure.
- Two deterministic Zopfli dashboard builds are byte-identical at **340,679 bytes source / 331,759 bytes minified HTML / 73,375 bytes gzip**. Source SHA-256: `c037f63be4f47cebf6da46acc5ad0e058b85151adbfcf3e4c1e2b5e77a566cf6`; generated header SHA-256: `0d9fbc09c24c911ecbaeec642f5eab2f86c8e2883ee04889af0ef50ddbf05338`.
- Final ESP32-S3 compile/link passes with Arduino CLI **1.5.1**, ESP32 core **3.3.12**, 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage: **1,525,211 / 3,145,728 bytes (48%)**; global RAM: **72,552 / 327,680 bytes (22%)**.
- The pre-packaging compile-check application BIN is **1,525,360 bytes**, SHA-256 `cbd2f1eea5ae26134ec356baf41d2c9e20d9cb2b9211d5c04f25e661dd417ad8`. Packaged BIN/source ZIP/full ZIP belong in `releases/T2CAN-Universal-v3.10.0-LP_YL/`; the no-flash release workflow revalidates the frozen source, recompiles, checks archives, and prints final artifact hashes. Build timestamps can change the final BIN hash.
- Independent code review found a CAN B offline race at the final CAN A send check; a failing regression confirmed it, and the fix and passing regression were re-reviewed with no remaining concrete findings.
- No controller flashing, live CAN transmission, or vehicle testing has been performed for this version.

# T2CAN Universal v3.9.3 — Validation

## R79 and NAG right-scroll controls — 2026-10-03
- Regression coverage requires Torque mode to display **Auto Right Scroll** immediately below NAG Method, while TSL9 periodic controls remain visible only when TSL9 Right Speed is selected.
- Scheduler host tests cover warning and periodic ownership, both selectable Torque waveforms, 100 ms steps, warning-edge priority, warning clear, physical-input deferral, CENTER cleanup, configuration quiesce, retries, and rollover.
- Static/runtime contracts cover NVS defaults and persistence, strict 1–600 second API parsing, route-local fresh templates, the single timer-owned CAN sender, and R79 Mode 1 cancellation generation barriers.
- Fresh completion validation passes **99/99 Python/static files**, **53/53 C++ host binaries**, and **4/4 JavaScript/browser tests**.
- Two deterministic Zopfli dashboard builds are byte-identical at **334,706 bytes source / 325,958 bytes embedded / 72,097 bytes gzip**. Dashboard source SHA-256 is `83608e60f248d33cf13fa9416ee5c4a2178f14e058d27f3e99ed8a51e3839315`; generated header SHA-256 is `21e0cd678509c6d4174636de0723c45ad7453c0d2e88ff7c3ba41ceea385e840`.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1 and ESP32 core 3.3.12 using 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,519,427 / 3,145,728 bytes (48%)** and global RAM is **72,480 / 327,680 bytes (22%)**. The OTA application image is **1,519,584 bytes**, SHA-256 `f809b6786442a8433fd4092cc190597d0050aa7e267a0dd7e112cc9bf7e7a711`.
- No controller was flashed. Live CAN timing, vehicle display behavior, and road validation remain unverified hardware boundaries.

# T2CAN Universal v3.9.2 — Validation

## AP profile naming and failed experiment removal — 2026-10-02
- A removal contract was observed failing before implementation while the experiment's helper, runtime hooks, API, persistence, trace source, and dashboard remained present.
- The contract now requires those integration markers and dedicated files to be absent while preserving the existing **AP Pedal / Regen Profile** dashboard feature.
- Fresh completion validation passes **98/98 Python/static files**, **53/53 C++ host binaries**, and **4/4 JavaScript/browser tests**.
- Two deterministic Zopfli dashboard builds are byte-identical at **330,692 bytes source / 322,044 bytes embedded / 71,399 bytes gzip**. Dashboard source SHA-256 is `3070ea50bf2943fa8db76f6fe8a93cbb7b6a93850a8fc5278067523dde6c0c57`; generated header SHA-256 is `86042be5817425ce9035b4897c1876676f37e3f1ba2dc33ccb649081fc452deb`.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1 and ESP32 core 3.3.12 using 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,514,315 / 3,145,728 bytes (48%)** and global RAM is **72,424 / 327,680 bytes (22%)**. The OTA application image is **1,514,464 bytes**, SHA-256 `a6f0fb5b2e301d54a69dc67af107fe97ce12d8e8f213d97d1fdb4f5f8a266475`.
- No controller was flashed. Live CAN and vehicle behavior were not tested.

# T2CAN Universal v3.9.1.4 — Validation

## Stable two-mode CAN Research Capture — 2026-10-02
- A new source contract was observed failing before implementation because RAW used a 360,448-frame archive, mode switching released/reallocated the large PSRAM blocks, four UI modes were present, and mode-change failures were hidden.
- The focused contract and dashboard browser regression pass with the fixed-buffer, two-mode implementation. Fresh completion validation passes **97/97 Python/static files**, **53/53 C++ host binaries**, and **4/4 JavaScript/browser tests**. The Zopfli dashboard build is **330,652 bytes** source, **322,004 bytes** embedded HTML, and **71,405 bytes** gzip.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1 and ESP32 core 3.3.12 using 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,514,259 / 3,145,728 bytes (48%)** and global RAM is **72,424 / 327,680 bytes (22%)**.
- No controller was flashed. Successful RAW allocation on the controller, CSV completeness, live CAN capture rate, and physical window-switch correlation remain hardware validation boundaries.

# T2CAN Universal v3.9.1.3 — Validation

## Model YL driver-window Auto Down field correction — 2026-10-02
- The supplied four-segment snapshot contained two Normal and two physical Auto Down captures. Its `0x3C2` MUX0 samples showed neutral `00 55 55 55 00 00 15 00` and second-detent Auto Down `00 55 55 55 00 00 19 00`, replacing the earlier unverified bit-35 assumption with byte 6 bits `[3:2]`, value `1 → 2`.
- A new pure regression was observed failing against the v3.9.1.2 transform because it changed byte 4 instead of producing the captured `0x19` byte 6 value. It also verifies that a physical `0x19` Auto Down input is not idle. The focused C++ regression passes after the field correction.
- Fresh completion validation passes **95/95 Python/static files**, **53/53 C++ host binaries**, and **4/4 JavaScript/browser tests**. The Zopfli dashboard build is **330,928 bytes** source, **322,276 bytes** embedded HTML, and **71,519 bytes** gzip.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1 and ESP32 core 3.3.12 using 16 MB flash, the 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,514,639 / 3,145,728 bytes (48%)** and global RAM is **72,424 / 327,680 bytes (22%)**.
- No controller was flashed. Live CAN timing, injected-frame acceptance, and actual window movement remain on-vehicle validation boundaries.

# T2CAN Universal v3.9.1.2 — Validation

## Model YL driver-window LAB Auto Down test — 2026-10-02
- A dedicated RED→GREEN regression reproduces the shipped availability defect: Model YL `PARTY + VH` was incorrectly combined with the Standard-profile CHASSIS predicate. The pure profile gate now accepts exactly `VEHICLE_MODEL_YL + VEHICLE_TOPOLOGY_YL_PARTY_VH` and rejects Standard topologies/models.
- The pure regression was first observed failing at an assertion because the driver-window helper did not exist. It now verifies that only byte 4 bit 3 (DBC start bit 35) changes, all other stock bits are preserved, exactly two MUX0 overlays are admitted, duplicate requests do not extend the pulse, and physical window input, stale template, stale/non-PARK gear, wrong profile, CAN unavailability, epoch change, timeout, invalid MUX, and rollover fail closed.
- Additional RED→GREEN coverage verifies fresh PARK/gear authorization on every frame, cached physical-input rejection at arm time, cancellation after a 250 ms stock gap even if traffic later resumes, autonomous timeout/stale cleanup without browser polling, and a generation guard shared with the CAN TX enqueue barrier. A runtime contract verifies CAN B task ownership, VH-only freshness admission, tagged TX tracing, recovery/LAB reset hooks, latest-request result reporting, API gating, and absence of direct CAN transmission in the HTTP handler.
- The dashboard regression was first observed failing because the LAB row, panel, action, and API bindings were absent. It now covers the explicit confirmation, duplicate-click guard, blocked/ready rendering, one-shot POST, and status refresh. The generated embedded dashboard is rebuilt from `dashboard_source.html`.
- Fresh completion validation passes **95/95 Python/static files**, **53/53 C++ host binaries**, and **4/4 JavaScript/browser tests**. Two deterministic Zopfli builds are byte-identical: **330,893 bytes** source, **322,241 bytes** embedded HTML, and **71,502 bytes** gzip. Dashboard source SHA-256 is `3bc60c8c90471d1f2197e344115410823d64066a3df3cef3ac1bd824d5436aa0`; generated header SHA-256 is `2168f658152ce4d6252fce86843acf560bc99ffb3812028660d983c7e4f103d3`.
- A focused code review first found four Important safety gaps (per-frame PARK, stale-stock cancellation, cached physical input, and LAB OFF/enqueue ordering). After the fixes, focused re-review found no remaining Critical or Important issue.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1, ESP32 core 3.3.12, 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,514,599 / 3,145,728 bytes (48%)**, global RAM is **72,424 / 327,680 bytes (22%)**, and the OTA application image is **1,514,752 bytes**, SHA-256 `05bfb5e67952fe0241875f5111282cd5a6d451f1ada62678de890a9ddf684890`.
- No controller was flashed. Live CAN timing, the assumed physical Auto Down interpretation of DBC bit 35, actual window movement, and vehicle behavior remain unverified hardware boundaries.

## CAN task heartbeat and CAN A overflow diagnostic build — 2026-10-02
- A new pure host regression covers never-started versus active task snapshots, execution-stage age, heartbeat gaps, loop timing including 32-bit microsecond rollover, CAN A loop load, budget exhaustion, and separate RX0/RX1 overflow observations.
- The runtime integration freezes task state, execution stage, timing, and stack evidence before a heartbeat-triggered hard reinitialization. CAN A receive-pressure evidence is exported in the existing System Stats JSON and cleared by Reset Stats.
- CAN routing, frame transforms, transmit authorization, task priorities, RX budgets, heartbeat thresholds, and recovery decisions are unchanged. Physical flash and live CAN/vehicle validation are outside this host build.
- Fresh host verification passes **93/93 Python/static files**, **52/52 C++ host binaries**, and **3/3 JavaScript/Chromium tests**. The two new diagnostic regressions were observed failing before their implementations and passing afterward.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1 and ESP32 core 3.3.12 using the 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot options. Program storage is **1,510,367 / 3,145,728 bytes (48%)**, global RAM is **72,336 / 327,680 bytes (22%)**, and the OTA application image is **1,510,512 bytes**, SHA-256 `0d5e21e0f03defd1a5b74d56c4db31d5507b07b5db294f5ac763fd1561545742`.

# T2CAN Universal v3.9.1 — Validation

## DMS bit43 AP gate — 2026-10-02
- The new pure regression was first observed failing because `r79DmsPolicyActivePure()` accepted no AP-state inputs, while the runtime contract failed because `r79DmsNagActive()` never read the AP gate. Both now require a valid active AP state before the DMS policy can force bit43 low, while retaining the existing NAG master, DMS toggle, profile, method, and torque-mode conditions.
- R79 ownership and transport authorization remain independent of this new gate. Outside AP, any R79-generated MUX1 frame retains the stock bit43 value instead of disabling Driver Monitoring.
- Fresh host verification passes **92/92 Python/static files**, **51/51 C++ host binaries**, and **3/3 JavaScript/Chromium tests**.
- Two deterministic Zopfli builds are byte-identical: **326,979 bytes** dashboard source, **318,451 bytes** embedded HTML, and **70,863 bytes** gzip. Dashboard source SHA-256: `2df3dae18e7980a8077e7bac9c84506d0fefd6698c3bd20bf0e8b5597581ee4d`; generated header SHA-256: `4623a34d3e37410c8014e60bc48faf0f759a1ba6bbd2a44d786d7bd6effbca74`.
- ESP32-S3 compile/link passes with Arduino CLI 1.5.1, ESP32 core 3.3.12, `autowp-mcp2515` 1.3.1, 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,507,559 / 3,145,728 bytes (47%)** and global RAM is **72,152 / 327,680 bytes (22%)**. The OTA application image is **1,507,712 bytes**, SHA-256 `c49245139e22b8de24a394974930bb3827eee3771134336db6a75a0a81749c82`.
- No controller was flashed. Bench CAN timing, live vehicle behavior, and road validation remain unverified hardware boundaries.

# T2CAN Universal v3.9.0 — Validation

## NAG KILL integration and release package — 2026-10-02
- New pure regressions cover the left-volume default and optional right-speed waveform, 100 ms ordering, randomized 2.0–3.0 second repeats, 250 ms retry, 300 ms physical-input quiet period, stale-template refusal, failed-CENTER ownership, AP/CAN/config cleanup, step and repeat rollover, and the cross-core issued-command/quiesce interleaving. The route-owning CAN task is the sole scheduler/TX executor.
- R79/DMS regressions verify DMS as the final overlay on Mode 1 immediate/retry/Post-MUX2, Mode 2 immediate/delayed, and ULC MUX1 clones. DMS-only fallback is admitted only with no R79 claim or pending work and remains zero-wait/no-retry. TSL9 transform tests cover Hands-On-only, ISA-only, combined, actual-ID checksum, and single counter/checksum composition.
- Legacy route tests lock the fresh/reset default to Body CAN A `0x39B`, the alternative to Chassis CAN B `0x399`, exclusive TSL9 transmission, and continuous Chassis `0x399` AP/R79 observation. NVS reset tests lock the exact 13-namespace clear allowlist, S3XY/BLE exclusions, durable guard ordering, and failure/reboot recovery after every namespace boundary plus bootstrap and guard-clear failures.
- Fresh host verification passes **92/92 Python/static files**, **51/51 C++ host binaries**, and **3/3 JavaScript/Chromium tests** against the editable and embedded dashboards. A focused post-fix code review found no remaining Critical or Important issue in cleanup/quiesce or BLE-preserving reset sequencing.
- Two deterministic Zopfli builds are byte-identical: **326,979 bytes** dashboard source, **318,451 bytes** embedded HTML, and **70,863 bytes** gzip, below the strict 100,000-byte ceiling. Dashboard source SHA-256: `2df3dae18e7980a8077e7bac9c84506d0fefd6698c3bd20bf0e8b5597581ee4d`; generated header SHA-256: `4623a34d3e37410c8014e60bc48faf0f759a1ba6bbd2a44d786d7bd6effbca74`.
- Fresh ESP32-S3 compile/link passes with Arduino CLI 1.5.1, ESP32 core 3.3.12, `autowp-mcp2515` 1.3.1, 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot. Program storage is **1,507,455 / 3,145,728 bytes (47%)** and global RAM is **72,152 / 327,680 bytes (22%)**. The OTA application image is **1,507,600 bytes**, SHA-256 `7ac6229640715734bd2405ec1f8cdfad05186df26ec7fa201360b77bcd908298`.
- No controller was flashed. Bench CAN timing, real BLE-bond preservation, live vehicle behavior, and road validation remain unverified hardware boundaries.

# T2CAN Universal v3.8.4 — Validation

## Legacy Model 3/Y TSL9 Body 0x39B test route — 2026-10-02
- The new route/checksum tests were first observed failing because the Legacy-only selector, ID-aware checksum, Body dispatch, and CAN-A recovery reset did not exist. They now verify Body `0x39B` only for Model Y Legacy and Model 3 Legacy/HW3, including the captured AP-active sample transforming to `05 1A DF 80 B0 C4 81 11`.
- Regression coverage locks Model YL to Party `0x399`, Model Y Juniper and Model 3 Highland to Chassis `0x399`, and keeps Chassis `0x399` AP/R79 parsing on Legacy while suppressing its TSL9 transmit path.
- Fresh host verification passes all **85/85 Python/static files**, **45/45 C++ host binaries**, and all **3/3 JavaScript/browser tests**. The dashboard rebuild is **330,800 bytes source / 322,166 bytes embedded HTML / 74,160 bytes gzip**, below the strict 100,000-byte limit.
- LilyGO T-Display-S3 compile/link passes with ESP32 core 3.3.12 and the existing 16 MB flash, 3 MB app partition, HW CDC, LoopCore=1 and EventsCore=1 options. Program storage is **1,486,974 / 3,145,728 bytes (47%)** and static RAM is **71,928 / 327,680 bytes (21%)**. The OTA application image is **1,487,120 bytes**, SHA-256 `71c558282057e0add1299823d9f8acdc94bd1cca8fc5560dd843b45dcc31b7fe`. No controller flash or live-vehicle test was performed.

## Diagnostics System Stats JSON download — 2026-10-01
- The browser regression first failed because `#diagSystemStatsJson` did not exist. It now verifies that the control is visible inside the CAN B Diagnostics action grid, spans the grid width, requests `GET /api/system/stats`, and downloads as `T2CAN_SYSTEM_STATS.json`.
- Fresh host verification passes all **84/84 Python/static files**, **44/44 C++ pure binaries**, generated-header host compilation, source and embedded Chromium dashboard behavior, the generated mobile NAG preview, and **8/8** Home Blinker countdown cases.
- Two deterministic dashboard rebuilds produce **330,395 bytes source / 321,769 bytes embedded HTML / 74,044 bytes gzip** with zlib level 9. Both generated headers have SHA-256 `617ca68e0ee18af6671f780fae27233ba0e301b2484db9db46d4d24c934efeff` and remain below the strict 100,000-byte gzip limit.
- ESP32-S3 compile/link passes from the final `T2CAN-Universal-v3.8.4-LP_YL` package layout with Arduino CLI and ESP32 core 3.3.12 using the existing 16 MB flash, 3 MB app partition, HW CDC, LoopCore=1 and EventsCore=1 options. Program storage is **1,486,606 / 3,145,728 bytes (47%)** and static RAM is **71,928 / 327,680 bytes (21%)**. The temporary OTA application image SHA-256 is `5495ef10b35e55bdb957e1d04c3215e0fbc4ca7712a2f87edcb91c75651df1db`; no controller flash or live-vehicle test was performed.

# T2CAN Universal v3.8.3 — Validation

## Embedded dashboard build budget — 2026-10-01
- The new boundary regression first failed on the former **71,350-byte** constant, then failed again because the exact **100,000-byte** boundary was still accepted. The build now accepts **99,999 bytes** and rejects **100,000 bytes**.
- Both binary-size contracts read `MAX_EMBEDDED_GZIP_BYTES` from `tools/build_dashboard.py`; the previous independent 71,350-byte and 84,000-byte literals are removed from active tests.
- Fresh host verification passes all **84/84 Python/static files** with **30 unittest cases**, **44/44 C++ pure binaries**, source and embedded Chromium dashboard behavior, the generated mobile NAG preview, **8/8** Home Blinker countdown cases, and generated-header host compilation.
- Two deterministic Zopfli rebuilds produce **330,164 bytes source / 321,542 bytes embedded HTML / 71,347 bytes gzip** and the same generated-header SHA-256 `21f4dc40fc3df8dad10c05d2447c21b6cc5e98a09cf8f2c64d2691b9f21981a6`. The generated header is byte-identical to the previously validated R79 bugfix input.
- Firmware inputs are unchanged; only the host build tool, its size contracts, and release documentation changed. The existing R79 bugfix OTA image remains `2ae28ee625abc6a484acff679648db37d6f7a326afbe0fd619ed366c103116a7` SHA-256. No additional controller build, flash, or live-vehicle test was required for this host-side ceiling change.

## R79 Home state and Mode 1 default bugfix — 2026-10-01
- The browser regression first reproduced both defects: the raw Mode 1 wait selector opened at `0` before its API refresh, and an `ACTIVE / DEFAULT / gear=P / DAS=0` R79 snapshot rendered **Active · AP engaged**. It now verifies the requested Mode 1 / STOCK / 2 ms / 150 ms initial defaults, **Park standby** while parked, and **AP engaged** only for the `AUTOPILOT` reason.
- Fresh host verification passes all **84/84 Python/static files**, **44/44 C++ pure binaries**, source and embedded Chromium dashboard behavior, the NAG mobile layout, **8/8** Home Blinker countdown cases, and generated-header host compilation.
- Deterministic Zopfli generation produces **330,164 bytes source / 321,542 bytes embedded HTML / 71,347 bytes gzip**, within the 71,350-byte ceiling.
- ESP32-S3 compile/link with Arduino CLI 1.5.1 and ESP32 core 3.3.12 passes using the existing 16 MB flash, 3 MB app partition, OPI PSRAM, HW CDC, LoopCore=1 and EventsCore=1 options. Program storage is **1,505,763 / 3,145,728 bytes (47%)**, static RAM is **72,112 / 327,680 bytes (22%)**, and the OTA application image is **1,505,920 bytes**. No controller flash or live-vehicle test was performed.

## Selectable TSL9 Hands-On downgrade window — 2026-09-30
- The new pure regression was observed failing on the absent window policy and expanded transform signature. It now verifies the upgrade-safe 12-second default, rejection at exactly 12,000 ms, continuous downgrade after 60 seconds, Extended and V8.2 sequence independence, and AP-inactive pass-through.
- Persistence/API/runtime contracts cover the `tsl9win` NVS value, `tsl9Window` config and status fields, strict 0/1 API validation, preservation across torque-mode changes, and explicit application on both MCP2515 and TWAI 0x399 paths.
- Chromium passes against both editable and embedded dashboards. It verifies the TSL9-only **Entire AP session** toggle, default OFF state, saved update request, mobile rendering, and existing NAG controls. The separate NAG layout and HOME Blinker browser tests also pass.
- Fresh host verification passes all **84 Python/static files** with **28 unittest cases**, **44/44 root C++ host binaries**, generated-header host compilation, all **3 JavaScript/browser files**, and **5 source + 5 embedded** inline-script syntax checks.
- Deterministic Zopfli regeneration produces **330,188 bytes source / 321,564 bytes embedded HTML / 71,314 bytes gzip**, within the existing 71,350-byte ceiling. ESP32-S3 compile/link passes at **1,505,699 / 3,145,728 bytes** program storage and **72,112 / 327,680 bytes** RAM; the OTA application image is **1,505,856 bytes**. No controller flash or live-vehicle test was performed.

## Standard Body + Chassis AP Right Scroll CAN A test build — 2026-09-30
- Pure profile coverage checks Body CAN A routing for all four standard models, and CAN B preservation for Model YL and Standard Party + Chassis. Runtime contract checks confirm the Body receive/MCP transmit and Chassis receive/TWAI transmit dispatch; preview and API route names follow the same selector.
- LilyGO T-Display-S3 compilation and link pass at 1,483,354 / 3,145,728 bytes program storage and 71,920 / 327,680 bytes static RAM. Targeted tests pass; 81 of 84 Python test files pass, with the remaining three unable to start because Node is absent from this shell.
- 2026 Model Y Juniper vehicle traffic was not captured. The prior screenshot's CHASSIS label came from the Legacy Y-only Body route; this build changes Juniper to BODY by profile policy, but on-vehicle function remains unverified.

## Party + Chassis TSL9 option removal — 2026-09-30
- Profile capability and UI/API gating tests verify Party + Chassis exposes Torque only, while YL and Body + Chassis retain TSL9. Existing saved TSL9 settings use the profile fallback that disables NAG during migration.
- Dashboard source regenerated with Zopfli at 71,349 bytes gzip, within the 71,350-byte budget. This section supersedes historical Party + Chassis TSL9 support claims below.
- LilyGO T-Display-S3 target compilation and link pass at 1,483,362 / 3,145,728 bytes program storage and 71,920 / 327,680 bytes static RAM. 81 of 84 Python test files pass; three require Node, which is unavailable in this shell. Targeted C++ profile and Hands-On tests pass. No physical controller or vehicle test was run.

## Legacy Model Y Body CAN AP Right Scroll test build — 2026-09-30
- Profile route, RX/TX dispatch, TSL9 sequence, and shared scroll integration tests pass. The shared scroll state machine remains unchanged; only Legacy Model Y Body + Chassis uses MCP2515 CAN A for 0x3C2, while YL and other profiles retain TWAI CAN B.
- Dashboard rebuilt from source with Zopfli: 71,336 bytes gzip, below the 71,350-byte budget. The source ZIP and OTA image from the original release remain untouched.
- The LilyGO T-Display-S3 target compiles and links at 1,483,310 / 3,145,728 bytes program storage and 71,920 / 327,680 bytes static RAM. Of 83 Python test files, 80 pass using the installed Python; the remaining 3 require Node, which is unavailable in this shell. Bun runs one of those three, while the other two fail because Bun does not match their Node VM harness. No physical controller, AP-active session, or live vehicle transmission was tested.

## Mobile dashboard polish and defaults — 2026-09-30
- Frame inspection of the supplied recording showed a native vertical scroll indicator plus a bottom horizontal indicator while entering a longer page. The pre-fix browser regression measured **402 px document width in a 390 px viewport** during the slide transform and also tracked the header position. The restored slide/fade now runs inside an app-level horizontal clip; every sampled animation frame stays within the viewport and the header remains fixed.
- Chromium behavior passes against both editable source and decompressed embedded dashboards. It verifies visible main-menu slide/fade motion, the no-zoom viewport/gesture contract, clipped horizontal overflow, hidden native scrollbars, AP Right Scroll placement, Advanced Parameters line layout, restyled/right-aligned inputs, exact **Hands-On state** Home rendering, and TSL9 hiding all torque-only controls. The generated preview Nag layout also passes at **320 / 390 / 430 px**.
- Fresh host verification passes **82/82 Python/static test files**, **42/42 C++ pure test binaries**, **5/5 source + 5/5 embedded inline scripts**, generated-header host compilation, and two byte-identical deterministic Zopfli rebuilds.
- ESP32-S3 compile/link passes with the existing 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot options: **1,504,751 / 3,145,728 bytes (47%)** program storage and **72,112 / 327,680 bytes (22%)** global RAM. The OTA application image is **1,504,896 bytes**.
- Final dashboard: **329,593 bytes source / 326,073 bytes embedded minified HTML / 71,340 bytes gzip**, inside the existing **71,350-byte** ceiling. Dashboard source SHA-256: `0693eebff623e96a298afa554dd0f43a3a5e59816f5c550a7646477b6d188eb2`; generated header SHA-256: `6802efea77fc56f20e05b13d9b770ebad82ce5d508304ce9e8d0933761d7cea5`.
- Physical iPhone/Android interaction, controller flashing, live CAN transmission, vehicle behavior, and road validation were not performed.

## R79 Mode 1 selectable timing — 2026-09-30
- The new pure regression was observed failing before implementation on the absent Mode 1 wait-mode and configurable-delay interfaces. It now verifies **FAST ECHO = 0 ms**, **2 ms WAIT = 2 ms**, the 2 ms upgrade-safe default, 0/340 ms delay boundaries, invalid-value fallback to 150 ms, a real 275 ms deadline, one-shot consumption, rollover, cancellation, authorization, and the existing 340 ms hard guard.
- Runtime/API contracts cover the new `m1wait` and `m1delay` NVS values, strict numeric input validation (including empty/signed/non-numeric rejection), status fields, selected initial enqueue wait, Mode 1 delayed scheduling, state reset on updates, and isolation from Mode 2's zero-wait/no-retry path. Deadline tests exercise 340/341 ms and rollover cases, and confirm initial enqueue plus queue-flush retry waits are capped by the remaining hard-deadline budget.
- Chromium behavior passes against both the editable source dashboard and the decompressed embedded dashboard. In Mode 1 it shows **FAST ECHO · 0 ms** and **2 ms WAIT**, accepts and posts a 275 ms Post-MUX2 delay, and keeps Mode 2-only controls hidden.
- Fresh host verification passes **82/82 Python/static test files**, **42/42 C++ pure test binaries**, **5/5 source + 5/5 embedded inline scripts**, generated-header host compilation, and deterministic Zopfli regeneration.
- ESP32-S3 compile/link passes with the existing 16 MB flash, 3 MB app partition, OPI PSRAM, and CDC-on-boot options: **1,504,567 / 3,145,728 bytes (47%)** program storage and **72,112 / 327,680 bytes (22%)** global RAM.
- Final dashboard: **329,002 bytes source / 325,482 bytes embedded minified HTML / 71,180 bytes gzip**, inside the existing **71,350-byte** ceiling. Dashboard source SHA-256: `43634a66a8076655951d9a5617d5f6b9418503765588a952bb239147e7d7f614`; generated header SHA-256: `e0299aa52280e44015dc8a0aaf2b7e9ef422ce60011f0344d96707f15863df1c`.
- No controller flashing, physical CAN transmission, live-vehicle behavior, or road validation was performed.

## Mobile custom-select native-picker guard — 2026-09-30
- Root-cause reproduction instrumented `HTMLSelectElement.focus()`: the original dashboard called it once from `closeSelectSheet()` while the Cancel click still carried trusted mobile user activation, matching the reported custom-sheet → native-picker chain. The new regression failed `1 !== 0` before the fix and passes with zero native-select focus calls after option, Cancel, and backdrop close paths.
- The activation contract confirms both touch `pointerdown` and its following `click` are default-prevented. A second mobile context removes Pointer Events and confirms the legacy `touchstart` + `click` fallback is also default-prevented. The custom sheet and its existing choice UI remain in use.
- The Chromium behavior harness passes against both the source and decompressed embedded dashboard. It covers the custom NAG selector, resulting `change`/API behavior, ordinary Settings navigation, toggles, buttons, and AP Right Scroll navigation. The custom sheet stays within **320 / 390 / 430 px** mobile viewports; the existing generated-preview NAG layout harness also passes at all three widths.
- Fresh host verification: **82/82 Python/static test files PASS**, **42/42 C++ pure test binaries PASS**, **5/5 source + 5/5 embedded inline scripts PASS** `node --check`, generated-header host compile PASS, and deterministic Zopfli rebuild PASS.
- Rebuilt dashboard: **327,483 bytes source / 324,940 bytes embedded minified HTML / 71,349 bytes gzip**, within the existing **71,350-byte** ceiling. `index_html.h` SHA-256: `495f6ac06234c8b7bd42acad90bd090f39e73fb35ca294a6bddb3f0b5ad9ec18`.
- The pre-fix archive contained one stale AP Right Scroll static assertion that expected the old panel tag without `data-back-target="panelNag"`; the assertion now matches the already-shipped NAGScroll structure. ESP32-S3 target compile/link, physical iOS/Android picker behavior, controller flashing, and live-vehicle validation were not run.

## AP Right Scroll navigation relocation
- Top-level Settings no longer exposes an AP Right Scroll row.
- Settings > Nag Killer exposes exactly one AP Right Scroll row.
- AP Right Scroll carries a `panelNag` back target, so Back returns to Nag Killer instead of the Settings root.
- Runtime/API/persistence code is unchanged; this is a dashboard navigation-only relocation.
- Targeted source + embedded-dashboard relocation contracts PASS; all 5 inline dashboard scripts PASS `node --check`; deterministic Zopfli dashboard rebuild is **71,350 bytes gzip**, at the existing binary optimization ceiling.
- The Chromium behavior harness was not executed in this environment because the Node `playwright` module is unavailable.

## v3.8.2 targeted regression
- Body+Chassis capability policy resolves an impossible persisted/default Torque method to TSL9 and de-arms NAG on that fallback. Valid TSL9 selections are preserved.
- NAG Reset applies the same active-profile policy; explicit enable/disable updates normalize the method first.
- Dashboard NAG method rendering has a single-method state for Body+Chassis, with TSL9 selected/locked and Enabled still controllable.
- LAB R79 gear fallback renders `N/A` instead of `UNKNOWN`; the driving-state and gear columns have bounded overflow rules for mobile widths.
- Host regression after the fix: **80/80 Python/static test files PASS**, **42/42 C++ pure test binaries PASS**, and **5/5 production dashboard inline scripts PASS** `node --check`.
- Rebuilt production dashboard: **327,528 bytes source / 324,985 bytes embedded minified HTML / 71,350 bytes gzip**, exactly at the existing binary optimization contract ceiling.
- Arduino ESP32-S3 target compile/link and live-vehicle validation were **not run in this environment**; no new hardware-runtime claim is made by this patch.

## iPhone home-screen icon — 2026-09-29
- New regression tests first failed on the absent home-screen metadata and icon, then passed after implementation. Source and decompressed embedded HTML both advertise `/apple-touch-icon.png` (180×180) and the name **Tesla Unlock**.
- The icon is an opaque RGB PNG, **26,699 bytes**; the generated PROGMEM array matches the asset byte-for-byte. The compiled OTA also contains the exact PNG and route string.
- Full Python unittest discovery passes **27 cases**, including existing static-module assertions. The initial run exposed a missing Node executable on PATH and the old HTML byte budget; Node PATH was supplied and the budget increased by 100 bytes specifically for the new metadata. Release HTML is **71,307 bytes gzip**, a **68-byte** increase over the previous release.
- The existing Chromium dashboard behavior test passes. Its first launch was blocked by sandbox loopback restrictions; the approved local-server run passed.
- LilyGO T-Display-S3 build succeeds: **1,481,438 / 3,145,728 bytes** program storage; **71,920 / 327,680 bytes** static RAM (unchanged). OTA application image: **1,481,584 bytes**. No CAN runtime or control-policy source was changed.
- Physical iPhone home-screen installation, physical controller flashing, and live vehicle behavior were not tested. No standalone/offline PWA behavior is claimed.

## AP Right Scroll restoration and V8.2 TSL9 waveform — 2026-09-29
- Pure coverage verifies the independent AP gate and the complete TSL9 **+1 → 0 → -1 → 0** waveform at 100 ms steps, including the Byte 6 bit 4 center state. Physical input cancels an in-progress generated sequence.
- Runtime contracts verify that AP Right Scroll no longer depends on Nag Killer or Mode H. Active TSL9 NAG selects its V8.2 waveform and warning range **3–6 / 9–10**; every other NAG state retains the established UP → DOWN path and visual-warning range **3–5**.
- Browser coverage opens the restored standalone Settings panel, changes its saved switch, confirms the TSL9 panel has no duplicate controls, and passes mobile layout checks. The embedded dashboard is **71,239 bytes gzip**, below the **71,250-byte** optimization contract.
- Fresh host verification passes all **78** Python/static files, **25** unittest cases, **42/42** C++ pure test binaries, three JavaScript/browser tests, and the generated-header checks. A LilyGO T-Display-S3 compile/link passes at **1,454,474 / 3,145,728 bytes (46%)** program storage and **71,920 / 327,680 bytes (21%)** static RAM; the OTA application image is **1,454,624 bytes**.
- OTA SHA-256: `9f3e60f3415763e2d705a5320a51c2d8ad0baf56513fd848e9a28bf78dfc0a4a`. Dashboard source SHA-256: `f0ce397666f1dde81b0ab40621b07693bf0de04edaf2de4b6bc8166353e16e17`. Generated header SHA-256: `295ed5fbf391e355b6a056c878d50d94e3a27692319f8ba49251108f6467037a`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## TSL9 V8.2 sequence, Body + Chassis, and compact status copy — 2026-09-29
- Pure-logic coverage verifies that **V8.2 Original** passes Hands-On states 2 and 3 unchanged, changes only 4→1, preserves unrelated bits, increments the 0x399 rolling counter, rebuilds the checksum, and retains the reference AP-state 3–6 / 12-second session window. The independently selectable **Extended** sequence still changes 2/3/4→1.
- Profile coverage verifies Model YL Party + VH, every Standard Party + Chassis profile, and every Standard Body + Chassis profile. Body + Chassis allows TSL9 on Chassis CAN B while rejecting the Party-CAN steering-torque method in both the UI and runtime path.
- Source and decompressed embedded dashboards pass the full Chromium behavior test and the 320/390/430 px Nag layout test. Mobile captures confirm **AP Unknown** and **Waiting** fit without the prior title/gear collision. Embedded dashboard size is **71,181 bytes gzip**, below the **71,250-byte** optimization contract.
- Fresh host verification passes all **77** Python/static files, **21** unittest cases, and **40/40** C++ pure test binaries. A LilyGO T-Display-S3 compile/link passes at **1,453,418 / 3,145,728 bytes (46%)** program storage and **71,880 / 327,680 bytes (21%)** static RAM; the OTA application image is **1,453,568 bytes**.
- OTA SHA-256: `909eb5fcb80e02841e5661aba2e17b8006938c22343fdf82bd0451bc8d3a54c2`. Dashboard source SHA-256: `c7b9534ed8b6ba92f846c22507c86e4ec09984d8fb00b73b1dd2aac668bd20a0`. Generated header SHA-256: `d1bc0099e424a8d4b164e7a28cd762b0ecec38e7b4a08bf717d84701110e9e3d`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## Separate R79 Mode settings menu — 2026-09-29
- A browser regression opens Settings, verifies the independent **R79 Mode** row, confirms Summon Monitor contains the read-only session monitor but no R79 selector, and confirms the new R79 panel owns the Mode 1 / Mode 2 and MUX2-delay controls.
- The source dashboard and decompressed firmware dashboard both pass the full Chromium behavior test and the 320/390/430 px NAG layout test. The embedded dashboard is **71,233 bytes gzip**, below the **71,250-byte** optimization contract.
- Fresh host verification passes **77/77** Python/static test files and **39/39** C++ pure test binaries. A LilyGO T-Display-S3 compile/link passes at **1,452,642 / 3,145,728 bytes (46%)** program storage and **71,880 / 327,680 bytes (21%)** static RAM; the OTA application image is **1,452,800 bytes**.
- OTA SHA-256: `96e48ba393bc0de7a81e857aaa9b771d11abc4bfcf6259c22cc177bde031db50`. Dashboard source SHA-256: `8ac9de552eeb67013ead823e8eb8be74eeec385233b5b981b4afc044f040f4a5`. Generated header SHA-256: `531930b112f27c86f723e61c2f5a80edf423a7dc493071a5ababadf217bfc6c5`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## Hidden Home Live details removal
- A fresh LilyGO T-Display-S3 compile/link passes. Program storage is **1,450,162 / 3,145,728 bytes (46%)**, global RAM is **71,440 / 327,680 bytes (21%)**, and the OTA application image is **1,450,320 bytes**.
- Relative to the preceding asset-optimized build, program storage fell by **1,292 bytes** and the OTA image fell by **1,280 bytes**. Dashboard gzip is **71,219 bytes** from **326,842 bytes** source / **324,303 bytes** embedded HTML.
- Fresh verification passes: **77/77** Python/static test files, **39/39** C++ pure test binaries, generated-header host compilation, **17/17** source/embedded/preview JavaScript syntax checks, both Chromium dashboard layout tests, and **8/8** Home blinker cases. The browser contract verifies that Home has no Live details DOM and never requests `live=1`.
- The legacy `home-lite` response remains available. ESP32-S3 image inspection reports a valid checksum and validation hash. OTA SHA-256: `c150979c5b8e084879abbcaa9b48ec12fa80c7767c91aba5f1e9fa19f9435d85`. Dashboard source SHA-256: `6943dd604cb24e244558e4738fb685078cbfc01f7c6fedf120f33008cb9da845`. Generated header SHA-256: `3417f2a60a5563a27b5cc4435520c2d26985264a6a51cfcb708c86b34a3bcd99`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## Asset and no-op UI cleanup
- A fresh LilyGO T-Display-S3 compile/link passes. Program storage is **1,451,454 / 3,145,728 bytes (46%)**, global RAM is **71,440 / 327,680 bytes (21%)**, and the OTA application image is **1,451,600 bytes**.
- Relative to the preceding conservative build, program storage fell by **19,992 bytes** and the OTA image fell by **20,000 bytes**. Dashboard gzip is **71,887 bytes** from **329,765 bytes** source / **327,210 bytes** embedded HTML; the two embedded WOFF2 fonts total **34,956 bytes**.
- Fresh verification passes: **77/77** Python/static test files, **21/21** unittest cases, **39/39** C++ pure test binaries, generated-header host compilation, **17/17** source/embedded/preview JavaScript syntax checks, both Chromium dashboard layout tests, and **8/8** Home blinker cases.
- ESP32-S3 image inspection reports a valid checksum and validation hash. OTA SHA-256: `88f95e430bcd1d3514ae1387ecf7a9796c88cc3780f04cf9027c44bf48306aa4`. Dashboard source SHA-256: `2ebb86a551baab7d6915f9e1e1979553a3b76920f20b08ebe99d5c384cd08ee6`. Generated header SHA-256: `a6555d84ea25b88f3c5e059c9eef4fffe7171d9412e5b3c9d8847f2311a281f4`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## Conservative binary optimization
- A fresh LilyGO T-Display-S3 compile/link with the same 16 MB / 3 MB app partition options passes. Program storage is **1,471,446 / 3,145,728 bytes (46%)**, global RAM is **71,440 / 327,680 bytes (21%)**, and the OTA application `.bin` is **1,471,600 bytes**.
- Compared with the supplied build, program storage fell by **70,956 bytes (4.60%)** and the OTA application image fell by **70,960 bytes (4.60%)**. The final build uses only `-fno-exceptions`; the tested `-fno-use-cxa-atexit` experiment is not included.
- Zopfli rebuilt the embedded dashboard to **74,320 bytes gzip** from the same **341,303-byte** dashboard source and **338,743-byte** minified HTML. The result decompresses successfully and retains the production `TESLA UNLOCK` content.
- Fresh regression passes: **77/77** Python/static test files, **39/39** C++ pure test binaries, **8/8** Home blinker cases, and the NAG settings browser layout at **320, 390, and 430 px**. The ESP32-S3 image has a valid checksum and validation hash.
- OTA SHA-256: `366769aa26c28338729794475c5814b119969e5822455efaadf69934cf4f146c`. Dashboard source SHA-256: `ce63df161b7e1d88bccd47753aef5479b215a53e4a0cc25a79b7368465468e07`. Generated header SHA-256: `552a41c3bccc93493364e616b1f9352ce4976ce02d7113b3799a5d6e5777faf0`.
- No physical controller flash, live CAN transmission, vehicle behavior, or road validation was performed.

## TSL9 NAG and selectable R79 Mode 2
- **76/76** Python/static test files and **39/39** C++ pure test binaries pass with warnings treated as errors. The embedded dashboard rebuild is **77,084 bytes gzip**, below the **84,000-byte** limit.
- Dedicated tests cover the TSL9 12-second AP window, Hands-On 2/3/4→1 conversion, counter rollover, checksum, pass-through cases, route integration, and the method's mutual exclusion with torque injection.
- Dedicated tests cover R79 Mode 2's exact bit mask, stock bit18 preservation, zero-wait transmit contract, MUX1 one-shot behavior, configurable MUX2 delay, cancellation, one-shot manual-D/R suppression, rollover handling, and mutual exclusion with Mode 1.
- The Nag settings page renders without overflow or page errors at 320, 390, and 430 px browser widths. The Home blinker JavaScript regression test remains **8/8 PASS**.
- This host does not have the ESP32-S3 Arduino toolchain, so no new target binary was compiled. No live vehicle transmission, bus-load behavior, or road behavior is claimed by these host tests.

## Final v3.8 firmware package
- Fresh ESP32-S3 compile/link PASS with `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. The OTA application image is **1,558,608 bytes**; program storage is **1,558,463 bytes (49%)** and static RAM is **71,576 bytes (21%)**.
- **74/74** standalone Python/static test files and **37/37** C++ pure test binaries PASS. The production dashboard has **5/5** syntactically valid JavaScript blocks and **593** unique element IDs; the offline preview has **7/7** valid blocks and **598** unique IDs. Embedded dashboard gzip is **75,691 bytes**, below the **84,000-byte** limit.
- OTA SHA-256: `64d24a5a7c2e6ef6ad3d7b47a05008c9c41d673f95822689611cf9493e7cc930`. Dashboard source SHA-256: `4139767698fac4a28cf11c30d80688f5f1b6bdfe7580e9fbb0bc540f9cc43d0b`. Generated header SHA-256: `279916c2d999bb91cfb75700c8f5e244e090cb5c9a880de209612a2e161edc66`.
- No physical hardware, live vehicle behavior, or pixel-level phone rendering was verified in this environment.

## Wireless reset card and floating navigation clearance
- Source inspection confirms `#s3ResetAll` uses 20 px vertical / 18 px horizontal padding. All **22** detail panels use either `.panelin` (**21**) or `.wizardIn` (**1**), both now receiving 120 px of bottom padding plus the phone safe area. The 56 px floating menu sits 30 px above the bottom, leaving 34 px of clearance at the end of scroll.
- **74/74** standalone Python/static test files and **18/18** unittest cases pass. Production dashboard **5/5** and offline preview **7/7** inline scripts pass syntax checks; element IDs are unique.
- Embedded dashboard is **75,691 bytes gzip**, below the **84,000-byte** limit. ESP32-S3 compile/link passes at **1,558,463 bytes (49%)** program storage and **71,576 bytes (21%)** static RAM. Browser visual rendering and physical-device behavior remain unverified.
- `dashboard_source.html` SHA-256: `4139767698fac4a28cf11c30d80688f5f1b6bdfe7580e9fbb0bc540f9cc43d0b`; generated `index_html.h` SHA-256: `279916c2d999bb91cfb75700c8f5e244e090cb5c9a880de209612a2e161edc66`.

## Wireless overview row balance
- Source inspection confirms all four Wireless overview rows share one `min-height:72px; margin:0 18px; padding:14px 0` rule, the previous Bluetooth-only padding is gone, and the existing sibling separators now stop at the row margin.
- **74/74** standalone Python/static test files and **18/18** unittest cases pass. Production dashboard **5/5** and offline preview **7/7** inline scripts pass syntax checks; their element IDs are unique.
- Embedded dashboard is **75,629 bytes gzip** under the **84,000-byte** limit. ESP32-S3 compile/link passes at **1,558,415 bytes (49%)** program storage and **71,576 bytes (21%)** static RAM. Browser visual rendering and physical-device behavior remain unverified.
- `dashboard_source.html` SHA-256: `ef58a838ad1f3791fe5d89eef791c9f798c8d2511410252606a545d98a60a8ca`; generated `index_html.h` SHA-256: `5d42c2b0fe70df1735a00f8c917f1be2c5fdeb5f00795208575b027542b60242`.

## Wireless overview and choice-button polish
- Source inspection confirms the first Wireless row uses 22 px top padding, its Bluetooth description is shortened, and unselected Appearance/Polling Rate buttons have transparent backgrounds with no border. The selected state keeps a filled pill and the existing setting handlers.
- The offline mock provides `/api/wifi/status` with a defined SSID and `bleInitialized` using the production API field name.
- **74/74** standalone Python/static test files and **18/18** unittest cases pass. The production source has **5/5** valid inline JavaScript blocks and **593** unique IDs; the regenerated preview has **7/7** valid inline scripts and **598** unique IDs.
- Embedded dashboard is **75,646 bytes gzip** under the **84,000-byte** limit. ESP32-S3 compile/link passes at **1,558,431 bytes (49%)** program storage and **71,576 bytes (21%)** static RAM. Browser visual rendering and physical-device behavior remain unverified.
- `dashboard_source.html` SHA-256: `3b334c45f07f63cb7bd2d3052589ee1487b7fa8a146c835b653e8e19e27a58b8`; generated `index_html.h` SHA-256: `5498980ac6a847ac0c7c17fe5594a01e3f6f6aab0e87ff4dce92b4c90feaab76`.

## Settings choice buttons and profile separator
- Verified the Nag-hidden profile CSS removes the first visible Auto Blinker separator, while the normal Nag-visible list retains its row separators. Appearance and Dashboard Polling Rate keep their original persisted setting values and handlers behind the new button groups.
- **74/74** standalone Python/static test files and **18/18** unittest cases pass. Source dashboard **5/5** and offline preview **7/7** inline JavaScript blocks pass syntax checks; IDs are unique in both files.
- Embedded dashboard is **75,635 bytes gzip**, below the **84,000-byte** limit. ESP32-S3 compile/link passes at **1,558,415 bytes (49%)** program storage and **71,576 bytes (21%)** static RAM. Browser visual rendering and physical-device behavior remain unverified.
- `dashboard_source.html` SHA-256: `e4a9c7f590e3f0bcbe73838e756e891a6b5fa234f8cb5c3cfb6f46428eb42bba`; generated `index_html.h` SHA-256: `a7ad0a10775ed09be5da2a7e7801ec310215c35ced7c6e2ac3dad778652b6f13`.

## Offline Home preview controls
- Preview mock contract checks cover AP OFF / AUTOSTEER / NOA, five model IDs, both standard CAN topologies, Highland stalkless selection, profile capability gates, the Home snapshot envelope, and live AP switching. The preview builder injects the control panel only into the offline HTML.
- Production `dashboard_source.html` SHA-256 remains `1c02451d31a74d701d16cf7165b21e448f924c18af7202a97629d9ff368406e6`; generated `index_html.h` SHA-256 remains `ddd35f1df47614cbdd28b18a94b9947d2817cadef333df037add4bdba4a53d85`.
- The host unittest discovery suite passes after the new preview test. Browser visual rendering remains unverified in this environment.

## Dashboard appearance preference
- A dedicated runtime test covers the default FOLLOW PHONE mode, live phone dark-mode change, forced LIGHT despite phone dark mode, forced DARK, saved-mode restoration, and invalid saved-value fallback. **73/73** standalone Python/static test files pass.
- Dashboard HTML has **592 unique IDs** and all **5** inline JavaScript blocks pass syntax checks. The rebuilt embedded dashboard is **75,303 bytes gzip** under the **84,000-byte** limit.
- ESP32-S3 compile/link passes at **1,558,079 bytes (49%)** program storage and **71,576 bytes (21%)** static RAM. The offline preview and OTA application binary were regenerated. Visual rendering and live phone/device behavior have not been checked on physical hardware.

## Dashboard selection and firmware file controls
- The original 17 native `<select>` values remain in place; the new selection sheet reads each live option list and dispatches the original `change` event after a changed selection. Disabled options are omitted, and keyboard navigation, Escape, cancel, and focus return are implemented.
- OTA still reads `otaFile.files[0]` and posts the same `FormData` to `/update`; the custom Choose .bin file button opens that input and the filename label follows its `change` event. The operating system's file picker remains native.
- Host Python/static test files: **72/72 PASS**. Inline dashboard JavaScript: **5/5 syntax PASS**. HTML element IDs: **590 unique**. ESP32-S3 compile/link: **PASS**, **1,557,567 bytes (49%)** flash and **71,576 bytes (21%)** static RAM. Embedded dashboard: **74,785 bytes gzip**, below the **84,000-byte** limit.
- A production-source offline preview and OTA application binary were generated. Browser visual rendering and physical device interaction remain unverified in this environment.

## Dark button text contrast
- Root cause: `body.t2-2027 button{color:inherit}` outranked `.setupPrimary{color:var(--bg)}`. The shared disabled rule also forced muted text fill and low whole-button opacity.
- The new source and embedded dashboard were rebuilt. Host checks: **72/72 PASS**. ESP32-S3 compile: **PASS**, using **1,555,951 bytes (49%)** flash and **71,576 bytes (21%)** static RAM. Embedded dashboard gzip: **73,180 bytes**, below the 84,000-byte limit.
- CSS color calculation for the profile button: active contrast **17.02:1 light / 17.85:1 dark**; disabled contrast at the new opacity **7.46:1 light / 9.56:1 dark**. Browser rendering remains unverified due the local-file browser policy.

## Dashboard style cleanup and Nag Killer rebuild
- Host Python/static checks: **72/72 PASS**. Inline dashboard JavaScript: **4/4 syntax PASS**. Preview mock JavaScript: **syntax PASS**. Nag Killer HTML subtree has balanced tags and the full dashboard has no duplicate IDs. The generated `index_html.h` compiled and ran with the host Arduino stub.
- Embedded dashboard: **324,151 bytes source / 321,988 bytes minified / 73,169 bytes gzip**, below the **84,000-byte** budget. The preview and firmware generation now read the same three style blocks without the former build-time CSS removal.
- ESP32-S3 target compilation: **PASS** with Arduino ESP32 core 3.3.11. Application uses **1,555,951 / 3,145,728 bytes (49%)** of program storage and **71,576 / 327,680 bytes (21%)** of static RAM. An OTA application binary was produced.
- Browser rendering could not be checked in this execution environment: headless Chrome launch was blocked, and the in-app browser refused a local `file:` preview. A local preview file was generated for manual visual review.

## v3.8 release validation

1. Firmware identity is **v3.8**. Fresh NVS, invalid stored NAG mode/revision values, NAG reset, Settings reset, and Factory Reset select **Mode H · Rev.4**. Valid user selections already stored by an older firmware remain unchanged after OTA.
2. The approved 2027 dashboard is embedded in the release: the header is **TESLA UNLOCK**, CAN frame ages render on their own second line, CAN A/B precede the unified R79 card, unsupported feature controls remain profile-gated, Home Live Details is hidden, top toast messages are absent, and detail panels retain forward/back transitions.
3. Home AP display continuity has a dedicated pure-state test and integration contract. Hard Init keeps the last confirmed AP presentation while the CAN subsystem is busy and for a bounded 10-second recovery grace period, while the functional AP/NOA state remains invalid and all transmit authorization stays closed until a new valid frame arrives.
4. Full host regression: **72/72 Python/static test files PASS** and **37/37 C++ pure test binaries PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`. The generated `index_html.h` host contract, supplied LAB reference contract, and approved-demo layout parity contract also pass.
5. Dashboard JavaScript: **5 source + 5 embedded + 6 standalone preview inline scripts PASS** with `node --check`. The LAB contract covers the three separate rounded cells, legacy style override, value-and-chevron control rows, dedicated functional controls, local font routes, and inset Research Tools list. Browser automation was not rerun because the app browser blocks local `file://` previews; a pixel-level render comparison remains unverified.
6. Final dashboard size is **420,457 bytes source / 312,364 bytes embedded minified HTML / 71,663 bytes gzip**, below the strict **84,000-byte** limit. `index_html.h` SHA-256: `8d86fd56286ccc1b99b3b0de817e2c92488af0f53a6ae30d9afc157a7b41e82b`.
7. ESP32-S3 compile/link PASS with Arduino CLI 1.5.1, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1, and `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. The application uses **1,554,447 bytes (49%)** program storage and **71,576 bytes (21%)** global RAM. Local LAB fonts add roughly 53.7 kB of program storage over the preceding v3.8 build while global RAM is unchanged.
8. No live-vehicle NAG result is claimed. HW3 Model Y Rev.3 verification should compare **4 FRAMES** and **IMMEDIATE** using the same v3.8 build before selecting a production receive method.

# T2CAN Universal v3.7.3 — Validation

## 2027 full-bleed dashboard

1. The supplied handoff is implemented as an offline, single-file dashboard with system fonts and no external resources. AP state changes update `body[data-ap]` and the Home title/copy for STANDBY, AUTOSTEER, and NOA.
2. A real Chromium test at **390×844** verifies the Home presentation, clickable Settings/LAB/Devices navigation, one rendered S3XY device card, and persistent LAB CAN A selection through `POST /api/lab/can-a-rx/update?mode=1`. The same test passes against source HTML and the decompressed embedded HTML with no page errors.
3. Endpoint comparison against the CAN A RX LAB baseline reports **77 retained / 0 removed / 0 added** dashboard API paths.
4. Fresh regression: **68/68 Python test files PASS**, **36/36 C++ test binaries PASS**, and **8/8 HOME Blinker behavior cases PASS**.
5. Generated dashboard size is **414,622 bytes source / 306,558 bytes embedded minified HTML / 70,928 bytes gzip**, below the strict **84,000-byte** limit.
6. ESP32-S3 compile/link PASS with Arduino CLI, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1, and `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. The application uses **1,500,327 bytes (47%)** program storage and **71,568 bytes (21%)** global RAM.
7. No live-vehicle behavior is claimed by this visual replacement; on-vehicle verification remains required for layout, touch ergonomics, and CAN feature behavior.

## CAN A RX LAB comparison variant

1. LAB selects the existing four-frame CAN A MCP2515 read batch or one-frame immediate processing. Both retain the 32-frame task yield budget; the receive observers, decoders, and NAG injection path are shared.
2. `features/canARxMode` is read at boot and written before the live mode changes. A missing or invalid value selects four-frame prefetch. LAB OFF also makes four-frame prefetch effective while retaining the saved choice.
3. Fresh regression: **68/68 Python test files PASS** and **36/36 C++ pure tests PASS**. The new pure test was observed failing before implementation, then passed after implementation.
4. Five dashboard script blocks pass `node --check`; regenerated embedded dashboard is **81,374 bytes gzip**, below the 84,000-byte limit. The LAB selector displays `4 FRAMES` and `IMMEDIATE`.
5. ESP32-S3 compile/link PASS with Arduino CLI 1.5.1, ESP32 core 3.3.11, `autowp-mcp2515` 1.3.1, and `USBMode=hwcdc, CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, LoopCore=1, EventsCore=1`. Application sketch uses **1,510,775 bytes (48%)** program storage and **71,568 bytes (21%)** global RAM.
6. No live HW3 Model Y / Rev.3 NAG result is claimed; the LAB switch enables an A/B comparison on the same firmware.

## Auto Blinker TX policy and cleanup contract

1. The production selector is located under **Settings > Auto Blinker** and controls both Auto Blinker and direct S3XY left/right requests. The retired LAB routes, panel, poller, and LAB-disable reset are absent.
2. The selection persists at `features/blinkTx`: `0` is **Single TX** and `1` is **350 ms Burst**. Missing or invalid storage defaults to Single TX for Model YL and 350 ms Burst for every other supported Model 3/Y profile.
3. The update path persists before applying the runtime transition. A failed Preferences open/write leaves the active mode unchanged; a valid Burst-to-Single transition cancels an active burst atomically.
4. CAN A and CAN B retain separate physical send functions. Host vectors verify exact stock-preserving left/right `0x249` bytes after common rolling-counter, turn-nibble, and checksum preparation, plus exact one-frame left/right `0x3C2` overlays for stalkless Single TX. Static coverage confirms the stalkless live-frame handler consumes the pending Single request and records its result.
5. Cleanup contracts reject the reviewed unconsumed R79 telemetry, stale DOM writes, obsolete declarations/constants, and test-only production helpers while preserving live API/safety fields and public JSON keys.
6. Real-vehicle validation is still required before treating the new selectable transmit policy as validated for a particular vehicle generation, hardware revision, or vehicle software branch.

## Fresh v3.7.3 verification

- Host C++ suite: **35/35 PASS**, compiled with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I.`.
- Python/static suite: **67/67 applicable test files PASS**. The single Node-dependent JavaScript progress test is explicitly skipped because Node is not installed in this environment.
- Generated `index_html.h` minimal host compile with the Arduino/PROGMEM stub: **PASS**.
- Dashboard generation is deterministic across consecutive stdlib zlib-9 builds: **PASS**. Final dashboard is **388,789 bytes source / 379,354 bytes minified / 80,990 bytes gzip**, below the strict **84,000-byte** limit. `index_html.h` SHA-256: `09fba8b495b65ebe3d32ed774edc9a3555dba5f395f1568b48ab8f0960fecfcb`.
- Node, `arduino-cli`, PlatformIO, and the Xtensa ESP32-S3 compiler are not installed, so no JavaScript syntax, ESP32-S3 target compile/link, firmware binary, or on-vehicle result is claimed for this package.

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
