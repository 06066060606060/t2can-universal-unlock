# T2CAN Universal v3.7.3 Blinker Promotion and Cleanup Design

## Scope

Promote the existing LAB-only blinker transmit policy to a supported Auto Blinker setting, persist it in NVS, and complete the reviewed dead-code, duplication, stale-dashboard, packaging, and validation cleanup. The deliverable is a verified `T2CAN-Universal-v3.7.3-LP_YL-source.zip`; the supplied v3.7.2 archive remains unchanged.

## Supported behavior

- Settings > Auto Blinker exposes one selector with `Single TX` and `350ms Burst` choices.
- The selected policy governs the shared Auto Blinker and S3XY direct-blinker transmitter.
- The setting is available independently of the LAB menu.
- A valid selection is stored as `features/blinkTx` in Preferences/NVS and restored on boot.
- If `features/blinkTx` is absent or invalid, Model YL defaults to `Single TX`; every other supported Model 3/Y profile defaults to `350ms Burst`.
- Changing from burst to single cancels any active legacy burst before the new mode becomes active. A pending single request is not converted into a burst.
- Persistence completes before the runtime mode changes. If NVS cannot be opened or written, the API returns an error and runtime behavior stays unchanged.
- Resetting the `features` namespace naturally removes the key and restores the profile default on the next load.

## API and UI

- Add production `POST /api/blinkA/tx-mode?mode=0|1`.
- Include `txMode`, `txModeName`, `txActiveSource`, `txLastDirection`, `txLastSource`, `txLastResult`, `txRequests`, `txOk`, `txFail`, and `txBlocked` in `/api/blinkA/stats`.
- Delete the LAB blinker panel, LAB polling/binding code, and `/api/lab/blinker-tx/stats|update` routes rather than retaining compatibility aliases.
- Place the selector and its operational counters in the Auto Blinker settings panel. Copy states that the choice also applies to S3XY blinker buttons.

## Cleanup boundaries

- Remove R79 telemetry that is not consumed by a decision, API, dashboard, serial diagnostic, or retained test: 0x7FF observation, unused bit 18/19/47 mirrors, unused last-kind/effective/3FD timestamps, unused periodic and fast-latency histograms, and the success-probe subsystem. Retain bit-43 behavior and all counters currently exposed or required for safety decisions.
- Remove stale forward declarations and unused private helpers/constants identified by the review. Delete tests that exist only to keep dead production helpers alive, while retaining tests of user-observable behavior.
- Remove dashboard writes/bindings to nonexistent element IDs.
- Deduplicate only low-risk pure/common preparation: stalk-frame template setup and result bookkeeping while leaving CAN A/B send calls distinct, CAN trace raw-byte formatting, repeated profile/feature capability JSON emission, and Nag V3/V4 common JSON fields. Preserve JSON key order and CAN timing/order.
- Exclude Python bytecode/cache artifacts from source and package output.

## Versioning and documentation

- Firmware identity, sketch/directory name, changelog, validation title, and version contract become `v3.7.3`.
- Regenerate `index_html.h` from `dashboard_source.html` using the supplied builder.
- Record final raw/embedded/gzip sizes and SHA-256 values in `VALIDATION.md` from the actual generated artifacts.

## Verification

- Pure C++ tests compile with C++17, warnings-as-errors, and execute successfully.
- Python/static tests pass; JavaScript tests run when Node is available and otherwise the missing runtime is reported explicitly.
- The generated dashboard header compiles against the host Arduino stub.
- New regression coverage proves profile defaults, invalid-value fallback, mode-transition cancellation, official API/NVS contracts, LAB removal, Settings placement, and package hygiene.
- A final clean extraction of the ZIP is retested to catch path, filename, cache, or packaging drift.

## Non-goals

- No change to turn-signal frame contents, checksum math, transmit cadence, arbitration order, or profile capability policy beyond selecting the already-existing single/burst paths.
- No full firmware build is claimed without an installed ESP32 Arduino toolchain.
