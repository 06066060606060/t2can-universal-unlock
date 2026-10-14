# CHASSIS trace and essential statistics — 2026-10-14

Input: `C:/Users/xmick/Desktop/tesla log/chassis-can.csv`, 710,534 bytes.
SHA-256: `308b8a476336ab23923a2b86d565ae3186071bd635448f414df05429346052bc`.
The original file was read without modification. It contains no header or
timestamp-unit declaration. Calculations below assume milliseconds, consistent
with the repeated 10/16/20/40 ms intervals in the intact records; the file alone
does not prove this unit or that timestamps represent physical arrival times.

The first and last complete records bracket timestamps 13039..33752. Excluding
malformed rows, invalid standard identifiers and timestamps outside this
explicitly selected interval yields:

- 18,019 usable frames and 118 identifiers over 20.713 seconds: about 870
  usable frames per second, with an average spacing of about 1.15 ms.
- Peak recorded counts are 9 frames in a 1 ms window, 13 in 2 ms, 23 in 5 ms,
  38 in 10 ms, and 900 in 1 second. The timestamp resolution and logger batching
  prevent sub-millisecond arrival/latency conclusions.
- Common IDs include `0x129` (1,775 intact records), `0x261` (1,157), `0x2E1`
  (1,130), `0x489` (977), and `0x488` (945). Captured rates undercount any frames
  represented by damaged rows. The trace contains no usable `0x370`, consistent
  with its CHASSIS source; this does not characterize the separate PARTY bus.
- 927 of 18,946 CSV records are excluded: 776 have the wrong column count
  (including blank/metadata records), 120 have inconsistent DLC/payload, six
  have malformed timestamp/ID text, one has an invalid standard ID, and 24 have
  timestamps outside the selected interval. Several lines contain merged or
  truncated frame text. None are reconstructed or treated as CAN loss counts.
- Usable records have no timestamp inversions after those exclusions. The log
  does not provide received-frame sequence numbers or a complete transmitter
  count, so exact losses cannot be calculated from it.

These recorded bursts motivate promptly draining both receive buffers, but
one CHASSIS log cannot establish three-bus scheduling requirements or justify
an arbitrary larger RX slice, a disabled watchdog pause, or a hardware filter
allowlist. Existing four-frame/600 us slices and three independent RX tasks
are retained. A software `switch` ignoring an ID occurs after reception and
does not protect the controller's two hardware RX buffers from that traffic.

## Implemented reductions

The default build keeps essential RX/TX/busy/error/overflow/recovery counters,
the requested PARTY `0x370` counts and RX0/RX1/BUKT checks. Detailed timing,
histograms and TX occupancy measurement are disabled with
`TMR_CAN_DETAILED_STATS=0`; set it to `1` for a diagnostic build. Simply closing
the dashboard would not have disabled the firmware's per-frame instrumentation.

The detailed build now reuses the RX driver's `READ STATUS` TXREQ bits instead
of issuing an additional statistics-only SPI transaction at every RX service
pass. Both builds remove that redundant per-pass read. Controller completion,
bus-off recovery, TX transport validation and watchdog pause policies remain
in place. This reduction is in SPI calls, not a measured end-to-end CAN speedup.

The CHASSIS receive shim avoids rebuilding an identical frame and retains the
extended/RTR flags for the existing decoder admission guard. Standard payloads
are preserved exactly, bytes beyond short DLC remain zero, and failed reads
do not overwrite the output frame.

## Verification and practical limits

An isolated synthetic SPI-register replay feeds all 18,019 intact records
through the actual MCP2515 driver, performance wrapper and extracted CHASSIS
receive shim. Every output ID, DLC and all eight data bytes are checked, with
short-frame padding, RX0/RX1 accounting and separate extended/RTR cases.
Additional cases cover invalid DLC, unavailable/empty controllers, TX enqueue,
busy and error separation, PARTY `0x370`, overflow, recovery and statistics
reset/configuration sampling. No vehicle feature handler is invoked.

Host checks pass in both essential/detailed modes under `-Os` and `-O2` with
ASan/UBSan. During this replay alone, the detailed instrumentation performs
72,076 `micros()` calls and 54,057 statistics critical-section entries; the
essential instrumentation performs zero such clock calls and 18,019 entries.
Thus four clock calls and two critical-section entries are removed per received
frame in this instrumentation. Scheduling/watchdog clocks elsewhere still run.
Both modes perform exactly one SPI `READ STATUS` per RX poll. Timing in this
register model is synthetic and is not a hardware benchmark.

Per-bus statistics storage is 88 bytes in essential mode versus 488 bytes in
detailed mode in the host fixture using the target's 32-bit CAN frame ABI:
1,200 fewer bytes for the three statistics structs. The ESP32-S3 object check
confirms this too: a three-struct probe array is 264 bytes in essential mode
versus 1,464 bytes in detailed mode. This is not a complete firmware RAM-size
comparison.

The isolated driver, receive shim and statistics JSON also compile with the
real ESP32-S3 Arduino headers/toolchain in both modes and optimization levels.
No complete application was linked, no firmware uploaded, and physical RX
loss/latency or CAN transmission completion was not measured. Compare overflow
increments and RX counts over the same time span and three-bus workload after
loading the updated sources. Temporary verification artifacts are under
`.build/can-trace/` (analysis JSON, excluded-row details, replay and target logs).

Reproduce the CSV analysis with `tools/analyze_can_log.py`, passing the source
path, `--time-unit ms --timestamp-start 13039 --timestamp-end 33752`, and an
`--output` JSON path. Unit/bound choices are recorded in the output.
