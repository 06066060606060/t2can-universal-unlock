# Android 1.2.1 test results

Current evidence and physical-device limitations: [VALIDATION.md](../VALIDATION.md).

Run host tests with tools/run_tests.py. For a disposable API36 emulator only, run tests/launch_emulator_test.py then tests/retry_emulator_test.py. They seed a non-existent AP without real credentials. The retry test counts actual platform network requests across two real timeout callbacks, rather than assuming that Back dismisses the system search dialog.

Emulator support waits up to 30 seconds for the explicitly selected emulator to finish cold boot and verifies ro.kernel.qemu before fixture changes.
