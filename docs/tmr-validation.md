# Universal TMR / T-2CAN validation

The contribution starts from upstream main `fc1a47f`. One runtime-selected
firmware supports both boards; no separate board build or release is required.

## A23 support

The sanitized host adapter test covers stable/unstable board signatures,
unknown-board isolation, saved profiles, same-ID Body/Party separation, explicit
TX destinations, shared-SPI locking, Chassis FIFO completion, queue saturation,
overflow, stuck TX, recovery, YL standby and native T-2CAN TWAI delegation.

Earlier A23 USB bench runs verified clean boot, saved Highland/stalkless profile,
three-bus reception and recovery. Chassis IRQ/queued TX wake the SPI worker;
A23 decoders have equal priority. T-2CAN keeps its original priority and drivers.
Hardware validation is distinct from a simulated maneuver result.

Contribution commit 1: universal target build passed (1,518,791 program bytes,
72,624 static-data bytes); sanitized adapter tests and dashboard syntax passed.
The decoded embedded HTML changes only the profile script against upstream.

## USB diagnostics

Sanitized host tests cover fragmented, overlong and malformed input, exact GET/POST
routes, persistence rollback, partial writes, backpressure, stalled output,
disconnect, queued requests, ring wrapping and physical-bus/page separation.
The client uses an exclusive serial-port lock and ignores unrelated boot logs.
Responses report stock frames and enqueue attempts, not receiver acceptance.

Contribution commit 2: universal build passed (1,522,143 program bytes,
73,088 static-data bytes). Sanitized USB tests and a pseudo-terminal client
round trip passed, including fragmented replies, boot-log filtering, response
matching and rejection of a competing port owner before serial I/O.

## Confirm-Free country assist

The final extension keeps the existing Confirm-Free toggle/timing/compositor.
Original is the fresh-install default. The saved ISO country changes only
0x238 and 0x7FF page1 on the stock frame's physical bus; page3 is unchanged.
Temporary region probes, map-region overrides and expiration controls are omitted.

Contribution commit 3: universal build passed (1,525,287 program bytes,
73,112 static-data bytes). Sanitized tests cover every listed country, all 16
counter values/checksums, unrelated-field preservation, malformed input, legacy
Korea migration, explicit Original precedence, invalid saved values, reboot
loading, failed writes, Confirm-Free/profile gates and same-bus TX selection.
Original T-2CAN capability results remain unchanged across 32 original
profile/topology combinations. All 26 pure headers compile independently.

The actual decoded embedded UI was tested with a simulated HTTP API: country
selection saves, failed saves restore the prior selection, Original survives a
page reload, and the row is hidden on T-2CAN. JavaScript syntax and deterministic
regeneration pass; docs/embedded-dashboard.html exactly matches compressed bytes.

Earlier Korea-only firmware produced user-confirmed automatic passing on the
isolated dismantled Highland bench (reported Tesla 2026.32.7), without a manual
turn-signal confirmation. This was the persistent country override, not proof
that all country selections work. Other country selections remain experimental.

### Deferred hardware acceptance

The connected TMR is reserved by another test instance as of 2026-10-05. The
user authorized proceeding with the contribution without hardware testing in
the available time. This consolidated build has not been flashed or maneuver-
tested; the completed build, host and simulated-API checks above remain the
current validation evidence. The following checks are deferred and do not block
PR review:

- Re-identify the available TMR and active application slot before app-only flash.
- Confirm saved Highland/stalkless profile, all three buses and legacy Korea migration.
- Save Original, reboot and verify it remains Original; restore Korea and verify
  it persists beyond 180 seconds without host renewal.
- Verify Confirm-Free OFF stops country assist and ON resumes it.
- Replay the passing scenario and correlate telemetry with the user's observed
  unconfirmed crossing into an established adjacent lane.
- Repeat the universal build on T-2CAN hardware when available; host delegation
  and profile checks do not replace physical T-2CAN qualification.

Bench results do not establish behavior in an operable road vehicle.
