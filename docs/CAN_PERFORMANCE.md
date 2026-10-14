# CAN scheduling and -Os / -O2 comparison

Hardware reported for this board: three XL2515-TSS controllers and MCP2562-E/SN
transceivers. Xinluda specifies a 10 MHz SPI interface for XL2515-TSS:
[XL2515-TSS specifications](https://www.xinluda.com/en/CAN-Bus-controller/20250103619.html).
The MCP2562 is the physical CAN transceiver, not the SPI controller:
[MCP2562 specifications](https://www.microchip.com/en-us/product/MCP2562).
The existing MCP2515-compatible driver names are retained. SPI stays at 10 MHz,
CAN at 500 kbit/s, and controller oscillator configuration at 16 MHz.

## Scheduling

Each physical bus now has its own receive task: `canA` receives BODY/J2,
`canB` receives CHASSIS/J3, and `canC` receives PARTY/J4. `canSup` remains
a separate recovery supervisor on core 0 (four CAN-related tasks total).
All three receive tasks use priority 4 on core 1, including after recovery.
The controllers still share a 10 MHz SPI bus; the Arduino SPI transaction lock
serializes bus access. A dedicated task does not make SPI transfers parallel.

- PARTY reads, processing, overflow checks, TX sampling and local bus-off
  recovery belong to `canC`; no BODY loop services PARTY. All three tasks
  participate in maintenance/OTA quiescence and full recovery stop/recreation.
  Partial task creation is cleaned up before any receiver starts SPI traffic.
  CHASSIS recovery wrappers explicitly compare their result to `ESP_OK`; a
  successful zero-valued result must not be interpreted as boolean failure.
- PARTY has an independent task heartbeat, including during quiet CAN periods.
  `/api/system/stats` adds `stackCanC`, timeout count/age and snapshot C fields.
  Existing A/B fields remain available. The historical `Both` timeout counter
  still denotes events affecting A and B together; C counts any event involving C.
- Each bus receives a slice of at most four frames. A 600 us cooperative time
  limit is checked between frames/batches. Already-prefetched BODY frames are
  all processed. A decoder, preemption, SPI operation or recovery can exceed
  this limit; it is not a hard real-time bound.
- CHASSIS reads are nonblocking, removing the former extra 2 ms empty-read wait.
- Receive tasks yield to their equal-priority peers between slices and continue
  while traffic is being received. After reception, empty passes yield and retry
  for up to 250 us before a one-tick pause. A periodic one-tick pause remains
  after 4 ms of elapsed burst time, allowing idle/watchdog tasks to execute.
  At the installed framework's 1 kHz tick rate one tick is nominally 1 ms;
  scheduler delays can be longer. Idle polling latency remains.
- RX overflow flags are observed for all three controllers each pass. Only
  RX0OVR/RX1OVR are cleared, preserving CANINTF.
- Existing frame routing, feature authorization and recovery epoch checks
  remain in place. All existing production TX calls pass through measurement.
  CHASSIS controller saturation is reported as `ESP_ERR_TIMEOUT`, separately
  from controller send errors.

## RX ages, buffer service and rollover

Each XL2515/MCP2515 has two hardware receive buffers, RX0 and RX1. Three
controllers provide six buffers total, with two dedicated to each bus; a busy
BODY controller cannot use unused PARTY buffers. These are distinct from the
three transmit buffers in each controller.

The supervisor captures all three published heartbeat timestamps before
refreshing its clock after recovery service. It no longer compares a newly
published heartbeat to a clock sampled earlier on the other core. Diagnostic
age calculation clamps future skews of up to 1000 ms to zero while retaining
unsigned clock wraparound and genuinely old timestamps. Missing heartbeat
sentinels and actual timeouts still trigger recovery. Dashboard traffic ages,
timeout snapshots and controller event ages use the same correction.

The driver now rotates RX0/RX1 priority when both receive flags are pending.
A sole pending buffer is always read. A malformed frame still drains its
selected buffer and rotates priority, preventing it from monopolizing reads.
The choice is local to each controller; bus routing and payloads are unchanged.
`reset()` reads back RXB0CTRL/RXB1CTRL and reports failure if BUKT is absent or
the expected standard/extended filter mode was not written. All three checked
initialization paths use this result before publishing readiness.

The compact dashboard cards are retained. `/api/system/stats` adds, for each
`canPerformance` bus, `rx0Frames`, `rx1Frames`, `rxConfigSamples`, `rx0Ctrl`,
`rx1Ctrl`, `rxConfigValid`, `rxRolloverEnabled`, and `rxFilterModeEnabled`.
Control registers are read by the bus's RX owner at most once per second,
without clearing pending frames or error flags. `rxConfigValid` requires a
sample, BUKT and the expected receive mode; filter masks are initialized to zero
by reset, but are not reread by this diagnostic. A zero RX1 overflow count alone
does not prove RX1 is unused: check the actual `rx1Frames` counter.

The new retry window and burst limit are cooperative scheduling settings,
not physical latency guarantees. They reduce immediate sleeps after short
traffic gaps, while preserving bounded periodic pauses. SPI contention,
processing/preemption and the two-frame capacity of each controller can still
cause overflow. Compare overflow increments, RX rates and poll gaps over equal
time intervals with the same bench traffic; cumulative counts do not show a
before/after rate without their elapsed interval.

Run `tests/run_can_rx_diagnostics_tests.py` under Linux/WSL. It exercises the
actual driver against a synthetic SPI register model for three controllers,
including continuous RX0 refill, RX1 service, malformed DLC, reset readback
failure and preservation of interrupt/overflow flags. It also extracts the
dashboard traffic snapshot and tests future publication, real stale/missing
heartbeats, clock wraparound, quiet retry and periodic pause boundaries.
These checks and existing task/lifecycle and TX-contention regressions pass
under `-Os`/`-O2` with ASan/UBSan. Isolated driver and diagnostics ESP32-S3
compilation results are saved in `.build/can-rx-diagnostics/target/results.json`.
No complete application was linked or uploaded. Physical readback and any
overflow reduction remain to be confirmed after loading the updated sources.

## Essential statistics (default since 2026-10-14)

`TMR_CAN_DETAILED_STATS` now defaults to `0`. All buses retain RX/TX counters,
RX read errors, TX busy/errors, RX overflow, recovery successes/failures and
the RX0/RX1/BUKT diagnostics. PARTY retains raw `0x370` RX/TX/error counters.
Closing the dashboard alone does not stop instrumentation; this compile-time
setting actually removes its timing work from the receive/transmit path.

In this essential build, `CanPerfRxScope` is empty; timing clock reads,
per-frame timing contexts and histogram updates are omitted. The optional
`readUs` output is zero and is consumed only by that disabled timing scope.
The statistics lock remains around the small counter updates so dashboard
snapshots and Reset Stats remain synchronized with RX/TX tasks.
RX configuration sampling no longer takes the statistics lock on every pass:
each receive owner schedules its own once-per-second readback. After Reset
Stats, configuration is republished at the next scheduled sample.

For a diagnostic comparison, add `-DTMR_CAN_DETAILED_STATS=1` to `build_opt.h`
alongside its existing flags. In this detailed build, the driver's existing
RX `READ STATUS` observation also supplies TXREQ occupancy. The three RX
tasks no longer issue an extra `READ STATUS` solely for TX statistics at the
end of every service pass. Sampling now occurs per RX poll; occupancy sample
counts/percentages from the former per-pass implementation are not directly
comparable. TX enqueue/busy/error counters remain independent of sampling.

`canPerformance.schemaVersion` is `2`, and `detailedStatsEnabled` explicitly
states the build mode. Timing/histogram and TX occupancy fields are omitted
when disabled, so missing measurements are not presented as zero latency or
zero saturation. The compact dashboard cards continue to use the essential
counters and require no UI changes.

The CHASSIS compatibility receive shim now copies its `can_frame` directly
instead of clearing and rebuilding an identical struct. This preserves the
driver's extended/RTR flags, allowing the existing receive task's frame-type
guard to work; padding beyond short payloads stays zero. Output remains
untouched on empty reads, invalid DLC and unavailable controllers.

See [CHASSIS trace analysis](CHASSIS_TRACE_ANALYSIS.md) for the input log,
measured software-operation reductions and verification scope. These changes
do not establish a physical overflow or latency improvement without bench
measurements after loading the updated sources.

## TX barrier contention

The common BODY/PARTY/CHASSIS transport entry points now wait up to one
FreeRTOS tick for the TX barrier instead of rejecting an occupied barrier
immediately. At the configured 1 kHz tick rate this is nominally 1 ms;
scheduler latency is additional. An uncontended take proceeds immediately.
The wait yields execution to the current owner, including when it is finishing
a SPI submission. A timeout still rejects the frame: there is no unbounded
wait or backlog that could later send obsolete frames.

`canTxEpochSnapshot()` reads an atomic mirror without taking the TX mutex,
so BODY TX cannot turn an otherwise valid PARTY epoch snapshot into zero or
delay the RX polling path. Recovery publishes every epoch change while holding
the barrier. The authoritative epoch, bus readiness, freshness and applicable
administrative hold, cancellation generation or validation callback are checked
after taking the mutex. PARTY also rechecks administrative hold after waiting.
Existing feature-local admission checks retain their own policies.

`MUTEX_BUSY` from these transport entry points now denotes failure to acquire
the barrier within the wait budget, or an unavailable mutex. Driver TX failure
counts still exclude rejections before the controller is called.

Run `tests/run_can_tx_contention_tests.py` under Linux/WSL. It extracts the
transport functions, uses synthetic frames, and tests contention release,
timeout, nonblocking epoch reads, recovery and epoch wrap, administrative hold,
freshness/readiness changes, cancellation and final validation during the wait.
It also checks unchanged controller busy/error accounting, with `-Os`/`-O2`,
ASan/UBSan and 1 kHz/100 Hz tick configurations. These host checks do not
measure physical CAN latency, RX overflow or throughput.

For this correction, the TX contention, RX lifecycle and existing performance
tests passed under `-Os` and `-O2` with ASan/UBSan. The extracted transport and
unchanged local MCP2515 driver also compiled to ESP32-S3 objects with both
optimization levels using Arduino-ESP32 3.3.12 and its real FreeRTOS/SPI headers.
Results are in `.build/can-tx-contention/target/objects/results.json`.
This isolated target check did not link a complete application or upload
firmware. Dashboard measurements under bench traffic are still required.

## Dashboard measurement

Open **Settings > Diagnostics & CAN Recovery > CAN COUNTERS**. Separate BODY,
CHASSIS and PARTY cards show RX, TX, FAIL RX/TX, recovery OK/FAIL and RX overflow.
TX failures include busy-buffer rejections. RX overflow remains a separate
latched-event count, not a failed software read or an exact lost-frame count.
Buffer/configuration and compiler readings remain available in JSON. Detailed
timing and TX occupancy readings require `TMR_CAN_DETAILED_STATS=1`.
Use the existing **System Stats JSON** button to save the complete measurements.
The same data is available under `canPerformance` in `/api/system/stats`.

- `recoveryAttempts`, `recoveries`, `recoveryErrors`: runtime controller
  recovery attempts, successful reinitializations and failed reinitializations
  on each physical bus. Initial boot setup is excluded; local and full subsystem
  recoveries are included. These counters reset with **Reset Stats**.
- PARTY `rx370`: standard data frames with ID `0x370` and nonzero DLC read
  directly from J4, before profile and NAG eligibility checks.
- PARTY `tx370`: successful controller submissions of those frames through the
  PARTY TX path; this is not a physical CAN acknowledgement counter.
  `tx370Errors` counts unsuccessful submissions, including busy buffers.
  The PARTY card displays `0x370 RX` and `0x370 TX` independently of NAG method.

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
comparison binaries above predate those fixes. The later simplified CAN cards
and per-bus recovery / PARTY `0x370` counters are in the current source and
`.build/dashboard-counters/Os/TMR_Dashboard.ino.bin`. The older
`.build/dashboard-fix/` image contains only the earlier Home fixes.

### Dedicated PARTY receive task validation

Run `tests/run_can_receive_task_tests.py` under Linux/WSL with Python 3 and g++.
The tests extract the current task/lifecycle functions and run them against
synthetic CAN frames and FreeRTOS fakes, without vehicle feature handlers.
They pass under `-Os` and `-O2` with ASan/UBSan: PARTY-only reception, bounded
draining, quiet-task heartbeat, bus-off recovery success/failure, maintenance
waiting for PARTY, all three allocation-failure positions, stop/recreation,
all seven task-timeout masks, timestamp wraparound and CHASSIS return codes.
Existing performance policy and runtime instrumentation tests also pass.

No complete ESP32 firmware build or upload was performed for this task split.
The binaries listed above predate it. Overflow reduction and physical CAN
latency remain unmeasured and must be compared under equivalent bench traffic.
