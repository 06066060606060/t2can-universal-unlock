# T2CAN Universal v3.7.3 Blinker Promotion and Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship a verified v3.7.3 source package with the blinker TX selector promoted into Auto Blinker, persisted in NVS, and the reviewed dead/duplicate code removed safely.

**Architecture:** Keep the existing single and legacy transmit paths, but replace the LAB gate with a validated production policy and a focused Preferences write. Fold the operational state into the existing Auto Blinker status object and panel. Perform cleanup behind existing behavior tests, extracting only pure/common pieces while leaving bus-specific timing and sends intact.

**Tech Stack:** ESP32 Arduino C++, Preferences/NVS, WebServer JSON API, embedded HTML/CSS/JavaScript, Python contract tests, host C++17 tests.

**Spec:** `docs/superpowers/specs/2026-09-27-v373-blinker-promotion-cleanup-design.md`

## Global Constraints

- Preserve the supplied v3.7.2 ZIP and every file under the project `sources/` mirror.
- Persist mode at `features/blinkTx`; valid values are `0` Single TX and `1` 350ms Burst.
- Missing or invalid stored values default to YL Single and non-YL Burst.
- Persist successfully before applying a runtime mode transition.
- Keep CAN A/B send operations separate and preserve transmit ordering, frame bytes, cadence, checksum behavior, and JSON key order.
- Do not ship `.git`, `__pycache__`, `.pyc`, compiled tests, or temporary logs.

## Review Focus

- Preferences open/write failure must not change the active runtime TX policy; covered by Task 1 contract assertions around save-before-apply flow.
- An invalid persisted byte must not silently select Single on non-YL vehicles; covered by Task 1 pure fallback tests.
- Burst-to-single changes during an active burst must cancel the burst atomically; covered by Task 1 transition tests.
- Moving the control out of LAB must not leave hidden pollers/routes or reset the production selection when LAB is disabled; covered by Task 2 UI/API contract tests.
- Cleanup must not change CAN frame content, JSON key order, or externally visible counters; covered by Tasks 3 and 4 characterization/compile tests.

---

### Task 1: Production blinker TX policy and NVS/API contract

**Files:**
- Modify: `blinker_tx_policy_pure.h`
- Modify: `vehicle_logic.h`
- Modify: `web_api.h`
- Modify: `t2can_forward.h`
- Modify: `T2CAN-Universal-v3.7.2-LP_YL.ino`
- Modify: `tests/test_blinker_tx_policy_pure.cpp`
- Replace: `tests/test_blinker_tx_lab_contract.py`

**Interfaces:**
- Produces: `blinkerTxStoredModePure(uint8_t stored, bool isModelYl) -> uint8_t`, `blinkerTxModeTransitionPure(uint8_t current, uint8_t requested) -> BlinkerTxModeTransitionPure`, `blinkerTxModePersist(uint8_t requested) -> bool`, `POST /api/blinkA/tx-mode`, and Auto Blinker status `tx*` fields.

- [ ] Add pure tests for YL/non-YL defaults, invalid stored fallback, valid stored retention, invalid transition rejection, and burst-to-single cancellation; run and observe expected compile/assert failure.
- [ ] Add the production static/API test asserting `features/blinkTx`, save-before-apply ordering, official route/status fields, and LAB route absence; run and observe failure on the LAB implementation.
- [ ] Implement the pure policy, focused NVS load/persist functions, boot load, official handler, and status fields. Remove LAB gating and LAB handlers.
- [ ] Run the two focused tests and the complete C++/Python suites; expect all applicable tests to pass.
- [ ] Commit `feat: promote persisted blinker tx policy`.

### Task 2: Move the selector into Auto Blinker settings

**Files:**
- Modify: `dashboard_source.html`
- Modify: `tests/test_blinker_tx_lab_contract.py`
- Modify: `tests/test_dashboard_feature_promotion_static.py`

**Interfaces:**
- Consumes: Task 1 `/api/blinkA/stats` `tx*` fields and `POST /api/blinkA/tx-mode`.
- Produces: Auto Blinker `blinkTxMode` selector/render/save flow with no LAB blinker panel or poller.

- [ ] Extend the dashboard contract test to require the selector/counters inside `panelBlink`, official endpoint usage, S3XY scope text, and absence of LAB IDs/functions; run and observe failure.
- [ ] Move markup and rendering into `fetchBlink`, bind the production update handler, remove LAB polling/reset code, and keep save errors visible without changing the selector optimistically.
- [ ] Regenerate `index_html.h`, then run dashboard/static and generated-header compile tests; expect pass.
- [ ] Commit `feat: move blinker tx policy into settings`.

### Task 3: Remove unconsumed R79 telemetry and stale UI writes

**Files:**
- Modify: `vehicle_logic.h`
- Modify: `web_api.h`
- Modify: `t2can_forward.h`
- Modify: `dashboard_source.html`
- Modify: R79 and dashboard tests that pin removed internals
- Create: `tests/test_v373_cleanup_contract.py`

**Interfaces:**
- Produces: reduced R79 state retaining bit-43 control, safety counters, and every field emitted by live APIs; dashboard code with no writes/bindings to missing IDs.

- [ ] Add a cleanup regression test that inventories the retained API/safety symbols and rejects the reviewed unconsumed telemetry and missing DOM IDs; run and observe failure.
- [ ] Remove 0x7FF observation, unused bit mirrors, unused latency/success-probe state and resets, stale declarations/names, and missing-ID dashboard calls while preserving retained R79 behavior.
- [ ] Regenerate the dashboard header and run focused R79/dashboard tests plus the complete suites; expect pass.
- [ ] Commit `refactor: remove dead r79 telemetry and ui hooks`.

### Task 4: Remove dead helpers and deduplicate safe common paths

**Files:**
- Modify: `vehicle_logic.h`
- Modify: `web_api.h`
- Modify: `t2can_forward.h`
- Modify: pure headers containing test-only helpers
- Modify: affected C++ tests
- Create: `stalk_frame_pure.h`
- Create: `tests/test_stalk_frame_pure.cpp`
- Extend: `tests/test_v373_cleanup_contract.py`

**Interfaces:**
- Produces: `stalkFramePreparePure(...)` for common template preparation only; shared trace raw formatter; shared profile/feature capability writer; shared Nag V3/V4 base writer.

- [ ] Add a host test with literal expected stalk-frame bytes for both turn-signal variants and a contract covering unchanged API key order; run and observe failure before the helper exists.
- [ ] Extract pure/common preparation and formatting helpers while keeping `sendStalkFrameCanA` and `sendStalkFrameCanB` as separate send functions.
- [ ] Remove reviewed unused declarations/constants/functions and tests whose only purpose was to call dead production helpers.
- [ ] Run focused frame/JSON/compile tests and the complete suites; expect pass.
- [ ] Commit `refactor: consolidate common paths and remove dead helpers`.

### Task 5: Version, generated assets, validation, and source package

**Files:**
- Rename: `T2CAN-Universal-v3.7.2-LP_YL.ino` to `T2CAN-Universal-v3.7.3-LP_YL.ino`
- Modify: version-bearing tests
- Modify: `CHANGELOG.md`
- Modify: `VALIDATION.md`
- Regenerate: `index_html.h`
- Create outside source directory: `T2CAN-Universal-v3.7.3-LP_YL-source.zip` and `.sha256`

**Interfaces:**
- Consumes: all previous tasks.
- Produces: installable Arduino sketch directory and clean source archive.

- [ ] Update version tests first to require v3.7.3 identity and matching sketch name; run and observe failure.
- [ ] Rename/update firmware identity, tests, and changelog; regenerate the dashboard and calculate actual size/hash facts for validation.
- [ ] Run all C++ tests with `-std=c++17 -O2 -Wall -Wextra -Werror -pedantic`, all Python tests, available JavaScript tests, and generated-header host compilation.
- [ ] Build the ZIP excluding Git/cache/build artifacts, extract it into a fresh temporary directory, rerun the verification suite there, and verify the archive manifest and SHA-256.
- [ ] Commit `release: package t2can universal v3.7.3`.
