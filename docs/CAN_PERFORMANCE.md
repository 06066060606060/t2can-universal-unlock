# CAN scheduling and -Os / -O2 comparison

Hardware reported for this board: three XL2515-TSS controllers and MCP2562-E/SN
transceivers. Xinluda specifies a 10 MHz SPI interface for XL2515-TSS:
[XL2515-TSS specifications](https://www.xinluda.com/en/CAN-Bus-controller/20250103619.html).
The MCP2562 is the physical CAN transceiver, not the SPI controller:
[MCP2562 specifications](https://www.microchip.com/en-us/product/MCP2562).
The existing MCP2515-compatible driver names are retained. SPI stays at 10 MHz,
CAN at 500 kbit/s, and controller oscillator configuration at 16 MHz.

## Scheduling

The three existing tasks remain: `canA` receives BODY/J2 and PARTY/J4;
`canB` receives CHASSIS/J3; `canSup` supervises recovery on core 0.
Both receive tasks use priority 4 on core 1, including after recovery.

- BODY and PARTY alternate first service each pass; neither gets a 32-frame
  drain before the other is checked.
- Each bus receives a slice of at most four frames. A 600 us cooperative time
  limit is checked between frames/batches. Already-prefetched BODY frames are
  all processed. A decoder, preemption, SPI operation or recovery can exceed
  this limit; it is not a hard real-time bound.
- CHASSIS reads are nonblocking, removing the former extra 2 ms empty-read wait.
- Receive tasks yield to their equal-priority peer between slices and continue
  while traffic is being received. A one-tick pause occurs on an empty pass or
  after 2 ms of elapsed burst time, allowing idle/watchdog tasks to execute.
  At the installed framework's 1 kHz tick rate one tick is nominally 1 ms;
  scheduler delays can be longer. Idle polling latency remains.
- RX overflow flags are observed for all three controllers each pass. Only
  RX0OVR/RX1OVR are cleared, preserving CANINTF.
- Existing frame routing, feature authorization and recovery epoch checks
  remain in place. All existing production TX calls pass through measurement.
  CHASSIS controller saturation is reported as `ESP_ERR_TIMEOUT`, separately
  from controller send errors.

## Dashboard measurement

Open **Settings > Diagnostics & CAN Recovery > CAN PERFORMANCE**. The current
compiler optimization and separate BODY, CHASSIS and PARTY readings are shown.
Use the existing **System Stats JSON** button to save the complete measurements.
The same data is available under `canPerformance` in `/api/system/stats`.

- `rxToEnqueue`: completion of software RX read to successful `sendMessage()`
  return for that frame, attributed only to the decoding task on the same bus.
  This is an observed upper bound on submission latency: it includes the
  driver's post-enqueue verification and any intervening preemption.
  Periodic TX, TX by another task and other-bus TX are not attributed to it.
  It excludes controller receive residence, CAN arbitration, transmission and
  acknowledgement. No physical RX-to-TX latency claim is made.
- `rxDispatch`: read completion to decoder entry; includes BODY prefetch delay.
- `rxRead`, `rxProcess`, `txCall`: wall-clock durations, including preemption.
- `pollGap`: gap between software receive polls, including pauses/recovery.
- Timing `count`, `totalUs`, `meanUs`, `maxUs`, histogram and p95 upper bound are
  accumulated since boot or **Reset Stats**. p95 is an approximate histogram
  upper bound, not an exact percentile. Missing samples appear as `--`.
- `rxOverflowEvents`: observations of latched overflow flags. Several dropped
  frames can produce one event. **This is not an exact lost-frame count.** Exact
  loss requires known transmitted frame sequences/counts from a CAN bench.
- `txBusy`: send attempts rejected because all three TX buffers were occupied.
  `txQueued` means accepted by the controller, not acknowledged on CAN.
- `txPendingLast`/`txPendingMax`: sampled hardware TXREQ occupancy, 0 to 3.
  `txFullSamples`/`txStatusSamples` is a sample ratio, not a time percentage;
  activity-dependent sampling may miss short saturation events.

Measurement adds a small CPU/SPI cost, identical in the two comparison builds.
No serial logging or per-frame allocation was added. The dashboard does its
formatting in the browser; firmware records integers and fixed histograms.
One API snapshot is coherent per bus, but snapshots of different buses are not
simultaneous. Resetting measurements can bisect an in-flight frame's timing.

For the manual comparison:

1. Build/load `-Os` with your board's current, known-working flash/PSRAM/partition
   settings. Allow boot/warmup to finish. Use **Reset Stats**, then allow flash
   activity from clearing stored diagnostics to settle before evaluating CAN.
2. Reproduce the same traffic, feature settings, capture mode, BLE/Wi-Fi activity
   and dashboard polling for a fixed duration. Save **System Stats JSON**.
3. Repeat with `-O2`. Compare per-bus latency mean/p95/max, RX overflow events,
   TX busy attempts relative to TX attempts, hardware occupancy, free heap and
   watchdog/recovery behavior. Repeat runs to reduce traffic-dependent noise.
4. Prefer the build with better measured CAN behavior, not simply the larger
   optimization flag. The default Arduino release setting remains `-Os` until
   measurements justify changing it.

For Arduino IDE, `build_opt.h` currently contains `-fno-exceptions`. Adding a
separate `-O2` line applies to rebuilt source, including the local CAN driver;
also add `-DTMR_PERF_BUILD_O2=1` so the dashboard labels that experiment `-O2`.
Remove both added lines to return to the default `-Os`. Fully rebuild each
variant. This does not rebuild the bundled ESP-IDF static libraries or change
their sdkconfig cache settings.

## Reproducible build comparison

Run `python tools/compare_can_builds.py`. It freezes the source, builds both
variants, verifies their production compiler flags, records flash/RAM/image
sizes and SHA-256 hashes, and checks that source did not change during builds.
It never uploads firmware. Outputs go into `.build/can-performance/<timestamp>`.

The default compilation profile is a generic ESP32-S3, 240 MHz, QIO 80 MHz,
16 MB flash, OPI PSRAM and the existing 3 MB application partition scheme.
This is a compilation profile, not detection of your board. Pass `--fqbn` with
your real board options when they differ; binaries must match the installed
partition table and hardware. Only the application `.ino.bin` is for dashboard
OTA, not `.merged.bin` or a bootloader.

Host tests in `tests/can_performance*_test.*` cover timing wraparound, limits,
all 256 hardware status values, metrics attribution, overflow accounting,
reset, empty samples and dashboard script syntax/rendering. They do not prove
physical CAN throughput, loss, latency or XL2515 electrical behavior.

## Validation on 13 October 2026

Both final builds passed with Arduino-ESP32 3.3.12 and the profile above. The
production compiler flags for the sketch and local CAN driver were verified
from the compilation database, including cached objects. The source manifest
matched the workspace at completion.

- `-Os`: 1,529,059 bytes program storage; 73,696 bytes static RAM;
  1,529,216-byte application BIN.
- `-O2`: 1,603,959 bytes program storage; 73,704 bytes static RAM;
  1,604,112-byte application BIN.
- `-O2` adds 74,900 bytes of program storage (about 4.90%) and 8 bytes static RAM.
  This is a size comparison, not a measured CAN speed improvement.
- Pure policy/metrics and runtime instrumentation tests passed under GCC with
  `-Os` and `-O2`, ASan and UBSan. All 256 TX status combinations, time wrap,
  first-poll fairness despite preemption, saturation versus send errors,
  task/bus timestamp attribution, early returns and recovery/reset were checked.
- Dashboard rendering checks and syntax checks for every script passed.
  Embedded gzip matches the editable source and occupies 78,690 bytes, below
  the project's 100,000-byte budget. `git diff --check` passed.

Build logs, source hashes, image hashes and the machine-readable comparison are
in `.build/can-performance/20261013-144811/`. The default `build_opt.h` remains
`-fno-exceptions`; no `-O2` override has been added to the normal release build.
No firmware was uploaded, and physical CAN latency/loss/throughput was not
measured. Hardware verification will be performed manually via the dashboard.

The subsequent [Home dashboard fixes](DASHBOARD_FIXES.md) correct the NAG
switch, traffic field names, three-bus layout and polling timeout. The compiler
comparison binaries above predate those fixes; use a new build of the current
source or the corrected `.build/dashboard-fix/` application image.
