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
