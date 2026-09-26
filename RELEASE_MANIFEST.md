# T2CAN Universal v3.7 — Production Source Package

This package is the production/source distribution derived from the validated v3.7 compile-repair build.

## Removed development/test artifacts
- `tests/` host C++ tests, Python/static contracts, generated host stubs, expected snapshots
- `docs/superpowers/` internal design/implementation planning documents
- `VALIDATION.md` development validation report
- Python bytecode/cache artifacts (`__pycache__`, `*.pyc`)

## Retained source/build files
- Firmware `.ino` / `.h` source required by the Arduino sketch
- `dashboard_source.html`
- `tools/build_dashboard.py`
- generated `index_html.h`
- `CHANGELOG.md`

No production runtime feature was removed as part of this packaging cleanup. In particular, source modules used by the firmware at runtime are retained even when their logic is also host-testable. The v3.7 lane-change-cancel change (manual S3XY / Door Open cancel does not gate on `DAS_behaviorType`) and the compile repairs are preserved.
