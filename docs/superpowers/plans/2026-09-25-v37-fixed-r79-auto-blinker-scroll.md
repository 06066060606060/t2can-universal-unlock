# T2CAN Universal v3.7 Implementation Plan

> **Post-release hotfix:** Manual S3XY/door lane-change cancel no longer uses `DAS_behaviorType` LEFT/RIGHT as an eligibility or direction gate. The original v3.7 plan below documents the release baseline; runtime behavior is superseded by the hotfix recorded in `CHANGELOG.md` and `VALIDATION.md`.

> **Execution:** Use `superpowers:executing-plans` for native, task-by-task implementation in this isolated repository. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build T2CAN Universal v3.7 with one fixed production R79 transport, updated Mode H Rev.4 defaults, independent Auto Blinker stabilization/pause timers, and all-profile AP Right Scroll visual-warning recovery.

**Architecture:** Preserve the existing stock-RX-following CAN architecture and isolate timing decisions in pure headers. Collapse R79 to a fixed D9 Fast Echo plus one MUX2 Quiet Window slot, expose only bit18 as production configuration, and remove every alternative implementation. Extend the existing Auto Blinker and AP Right Scroll state machines without adding a timer-originated CAN sender.

**Tech Stack:** ESP32/Arduino C++17, MCP2515 CAN A, ESP-IDF TWAI CAN B, Preferences/NVS, embedded HTML/CSS/JavaScript dashboard, host C++ pure tests, Python static contracts.

**Spec:** `docs/superpowers/specs/2026-09-25-v37-fixed-r79-auto-blinker-scroll-design.md`

## Global Constraints

- Firmware and user-visible release version is exactly `v3.7`.
- R79 Fast Echo is fixed to stock MUX1, 2 ms enqueue wait, bit19=0 and bit47=1; bit18 is the only setting and defaults to FORCE 0.
- R79 periodic is fixed to ALWAYS, stock MUX2 anchor, Quiet Window +150 ms, one TX per cycle.
- NOA Stabilization is 1–20 s, default 10 s; Cancel Pause is 10–100 s, default 20 s.
- AP Right Scroll regular interval remains 1–600 s, default 30 s; visual-warning repeat is 1–5 s, default 2 s.
- AP Right Scroll transmits only on CAN B: VH for Model Y L and Chassis for Standard Model 3/Y.
- No new S3XY action ID is introduced; existing persisted IDs remain byte-for-byte stable.
- No new background CAN transmit task is allowed.
- Dashboard gzip must remain below 84,000 bytes and deterministic across consecutive builds.
- Existing BUS OFF persistence and reset semantics must remain unchanged.

## Review Focus

- `millis()` wraparound during NOA stabilization, Cancel Pause and AP warning repeat must not extend or prematurely end a deadline; Tasks 4 and 5 add boundary tests.
- A second cancel action while paused and without a current planner request must unpause without sending another cancel; Task 4 pins this state transition.
- Physical right-scroll input during a warning UP/DOWN sequence must cancel/postpone generated output without leaving DOWN stuck; Task 5 exercises both phases.
- Power loss after new NVS values are written but before the schema marker commits must preserve legacy keys and retry safely; Task 6 simulates each boundary.
- R79 pruning must leave exactly the fixed Fast Echo and fixed periodic transmit paths, with no hidden transport fallback; Tasks 2 and 3 audit symbols and call sites.

---

### Task 1: Version and Mode H Rev.4 v18 Defaults

**Files:**
- Modify: `T2CAN-Universal-v3.6f3-LP_YL.ino`
- Modify: `nag_human_v4_pure.h`
- Modify: `nag_mode_h_variant_pure.h`
- Modify: `can_core.h`
- Modify: `tests/test_v36f4_rev4_defaults_ho_contract.py`
- Create: `tests/test_v37_mode_h_defaults_pure.cpp`
- Create: `tests/test_v37_version_contract.py`

**Interfaces:**
- Consumes: existing `NagHumanV4ConfigPure`, `NagConfig::modeHStopBehavior`, nag NVS version 17.
- Produces: `nagHumanV4MigrateV17DefaultPure(NagHumanV4ConfigPure&) -> bool`, nag schema 18, Rev.4 HO=2 default 200 raw, Mode H stop default `H_STOP_HARD_PAUSE`.

- [ ] **Step 1: Write the failing v3.7 default and migration tests**

```cpp
// tests/test_v37_mode_h_defaults_pure.cpp
#include <cassert>
#include "../nag_human_v4_pure.h"
#include "../nag_mode_h_variant_pure.h"

int main() {
  const auto defaults = nagHumanV4DefaultConfigPure();
  assert(defaults.ho2ThresholdRaw == 200u);
  assert(nagModeHDefaultStopBehaviorPure() == H_STOP_HARD_PAUSE);

  auto old = defaults;
  old.ho2ThresholdRaw = 175u;
  assert(nagHumanV4MigrateV17DefaultPure(old));
  assert(old.ho2ThresholdRaw == 200u);

  old = defaults;
  old.base.peakMaxRaw = 255u;
  old.ho2ThresholdRaw = 175u;
  assert(!nagHumanV4MigrateV17DefaultPure(old));
  assert(old.base.peakMaxRaw == 255u);
  return 0;
}
```

The static contract must assert `FW_VERSION "v3.7"`, nag version 18, `prefs.putUChar("v", 18u)`, the 2.00 dashboard fallback, and HARD PAUSE fallback.

- [ ] **Step 2: Run the tests to verify RED**

Run:

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v37_mode_h_defaults_pure.cpp -o /private/tmp/test_v37_mode_h_defaults && /private/tmp/test_v37_mode_h_defaults
python3 tests/test_v37_version_contract.py
```

Expected: compile/static failures because v3.7 defaults and migration helpers do not exist.

- [ ] **Step 3: Implement the v18 defaults and exact-old-default migration**

In `nag_human_v4_pure.h`, change the default and add an exact migration predicate:

```cpp
c.ho2ThresholdRaw = 200u;

static inline bool nagHumanV4MigrateV17DefaultPure(NagHumanV4ConfigPure &c) {
  const bool exactV17 =
      c.base.peakMinRaw == 180u && c.base.peakMaxRaw == 260u &&
      c.base.waitMinMs == 900u && c.base.waitMaxMs == 3000u &&
      c.base.refractoryMinMs == 500u && c.base.refractoryMaxMs == 1500u &&
      c.carrierMinRaw == 10u && c.carrierMaxRaw == 60u &&
      c.hoPolicy == H3_HO_TIERED_1_2 &&
      c.ho1ThresholdRaw == 40u && c.ho2ThresholdRaw == 175u &&
      c.visualRescueEnabled && c.visualRescueDelayMs == 500u;
  if (!exactV17) return false;
  c = nagHumanV4DefaultConfigPure();
  return true;
}
```

Add `nagModeHDefaultStopBehaviorPure()` returning `H_STOP_HARD_PAUSE`. Use it in defaults, invalid-value fallbacks, missing-key reads and reset paths. In `nagCfgLoad()`, migrate STOCK CARRIER to HARD PAUSE only when nag version is below 18 and the exact v17 Rev.4 default migration fired. Save nag version 18 only after all values are written.

- [ ] **Step 4: Update version strings and existing contracts**

Set `FW_VERSION` to `v3.7`. Replace v17 assertions in `test_v36f4_rev4_defaults_ho_contract.py` with v18 assertions while retaining all Rev.4 UI and persistence coverage.

- [ ] **Step 5: Run focused tests to verify GREEN**

Run the commands from Step 2 plus:

```bash
python3 tests/test_v36f4_rev4_defaults_ho_contract.py
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v36e2_mode_h_rev4_pure.cpp -o /private/tmp/test_v36e2_mode_h_rev4 && /private/tmp/test_v36e2_mode_h_rev4
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v36f2_mode_h_visual_rescue_pure.cpp -o /private/tmp/test_v36f2_mode_h_visual_rescue && /private/tmp/test_v36f2_mode_h_visual_rescue
```

Expected: all pass.

- [ ] **Step 6: Commit**

```bash
git add T2CAN-Universal-v3.6f3-LP_YL.ino nag_human_v4_pure.h nag_mode_h_variant_pure.h can_core.h tests/test_v36f4_rev4_defaults_ho_contract.py tests/test_v37_mode_h_defaults_pure.cpp tests/test_v37_version_contract.py
git commit -m "feat: set v3.7 mode h defaults"
```

### Task 2: Fixed R79 Policy and Single Production Scheduler

**Files:**
- Create: `r79_fixed_policy_pure.h`
- Modify: `T2CAN-Universal-v3.6f3-LP_YL.ino`
- Modify: `vehicle_logic.h`
- Modify: `can_runtime.h`
- Modify: `web_api.h`
- Create: `tests/test_v37_r79_fixed_policy_pure.cpp`
- Create: `tests/test_v37_r79_fixed_runtime_static.py`

**Interfaces:**
- Consumes: accepted stock `0x3FD` MUX1/MUX2, existing R79 runtime authorization and retry helpers, `R79QuietWindowStatePure`.
- Produces: `R79Bit18PolicyPure`, `R79FixedQuietStatePure`, `r79FixedApplyBitsPure(uint8_t[8], uint8_t)`, `r79FixedQuietObserveStockPure(...)`, `r79FixedQuietStepPure(...)`, `r79FixedFastEcho(const twai_message_t&,int64_t)`, `r79FixedObserveStock(uint8_t,uint32_t)`, `r79FixedTick()`, and one `r79Bit18Policy` runtime setting.

- [ ] **Step 1: Write failing pure policy tests**

```cpp
// tests/test_v37_r79_fixed_policy_pure.cpp
#include <cassert>
#include <cstring>
#include "../r79_fixed_policy_pure.h"

int main() {
  uint8_t stock[8] = {0xA5,0x5A,0xFF,0x12,0x34,0x00,0x78,0x9A};
  uint8_t out[8];
  std::memcpy(out, stock, 8);
  r79FixedApplyBitsPure(out, R79_BIT18_STOCK_PURE);
  assert(((out[2] >> 2) & 1u) == ((stock[2] >> 2) & 1u));
  assert(((out[2] >> 3) & 1u) == 0u);
  assert(((out[5] >> 7) & 1u) == 1u);

  std::memcpy(out, stock, 8);
  r79FixedApplyBitsPure(out, R79_BIT18_FORCE_0_PURE);
  assert(((out[2] >> 2) & 1u) == 0u);
  assert(r79Bit18PolicySanitizePure(99u) == R79_BIT18_FORCE_0_PURE);
  static_assert(R79_FIXED_FAST_WAIT_MS_PURE == 2u, "2 ms Fast Echo");
  static_assert(R79_FIXED_QUIET_DELAY_MS_PURE == 150u, "MUX2 +150 ms");

  R79FixedQuietStatePure quiet = {};
  assert(!r79FixedQuietObserveStockPure(quiet, 1u, 1000u).armed);
  assert(r79FixedQuietObserveStockPure(quiet, 2u, 1050u).armed);
  assert(r79FixedQuietStepPure(quiet, 1199u, true) == R79_FIXED_QUIET_WAIT_PURE);
  assert(r79FixedQuietStepPure(quiet, 1200u, true) == R79_FIXED_QUIET_FIRE_PURE);
  assert(r79FixedQuietStepPure(quiet, 1201u, true) == R79_FIXED_QUIET_WAIT_PURE);
  return 0;
}
```

Add re-anchor, next-cycle cancellation, authorization-block, hard-window and `uint32_t` rollover cases. One MUX2 cycle may yield at most one FIRE.

Add a static test requiring one MUX1 Fast Echo dispatcher, one MUX2 quiet-window arm, one fixed periodic tick and no runtime read of strategy/post-mode/scheduler/anchor/shots.

- [ ] **Step 2: Run focused tests to verify RED**

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v37_r79_fixed_policy_pure.cpp -o /private/tmp/test_v37_r79_fixed_policy && /private/tmp/test_v37_r79_fixed_policy
python3 tests/test_v37_r79_fixed_runtime_static.py
```

Expected: missing header/functions and legacy selectable runtime references.

- [ ] **Step 3: Add the fixed pure policy**

```cpp
enum R79Bit18PolicyPure : uint8_t {
  R79_BIT18_STOCK_PURE = 0,
  R79_BIT18_FORCE_0_PURE = 1
};
static constexpr uint8_t R79_BIT18_DEFAULT_PURE = R79_BIT18_FORCE_0_PURE;
static constexpr uint16_t R79_FIXED_FAST_WAIT_MS_PURE = 2u;
static constexpr uint16_t R79_FIXED_QUIET_DELAY_MS_PURE = 150u;

static inline void r79FixedApplyBitsPure(uint8_t data[8], uint8_t policy) {
  if (!data) return;
  if (r79Bit18PolicySanitizePure(policy) == R79_BIT18_FORCE_0_PURE)
    data[2] = (uint8_t)(data[2] & ~(1u << 2));
  data[2] = (uint8_t)(data[2] & ~(1u << 3));
  data[5] = (uint8_t)(data[5] | (1u << 7));
}
```

Ensure the helper changes no other payload bits.

- [ ] **Step 4: Rewire Fast Echo and periodic scheduling**

Replace `r79TransportHandleMux1()` with a fixed handler that clones MUX1, applies the fixed bits and uses `twai_transmit(..., pdMS_TO_TICKS(2))`. The accepted MUX1 also refreshes the sole periodic template. Replace selectable periodic observation with a fixed stock observer: MUX2 arms one slot at +150 ms, while subsequent accepted stock frames update/cancel the Quiet Window guard as appropriate. `r79FixedTick()` clones the latest fresh accepted MUX1 template, applies the same bit policy and consumes at most one slot per MUX2 cycle. Reuse the existing R79 authorization, stock-template freshness, administrative hold, recovery epoch and bounded retry functions.

Capture the millisecond timestamp before R79 dispatch so the observer never refers to the later heartbeat-local `frameNow` declaration. The CAN-B fast dispatch becomes structurally equivalent to:

```cpp
const uint32_t r79FrameNowMs = (uint32_t)millis();
if (timingMux == 1u) r79FixedFastEcho(f, frameRxDequeueUs);
r79FixedObserveStock(timingMux, r79FrameNowMs);
```

- [ ] **Step 5: Expose production bit18 status/update**

Add `GET /api/r79/stats` and `POST /api/r79/update?bit18Mode=0|1`. Reject any other number with HTTP 400. The stats payload includes fixed policy labels and only core counters. Mutation is not LAB-gated.

- [ ] **Step 6: Run focused tests to verify GREEN**

Run Step 2 plus:

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_r79_policy.cpp -o /private/tmp/test_r79_policy && /private/tmp/test_r79_policy
python3 tests/test_r79_runtime_static.py
python3 tests/test_v36d2_r79_fast_path_static.py
```

Update the older static tests to assert the fixed function names and 2 ms path rather than the removed transport selector.

- [ ] **Step 7: Commit**

```bash
git add r79_fixed_policy_pure.h T2CAN-Universal-v3.6f3-LP_YL.ino vehicle_logic.h can_runtime.h web_api.h tests/test_v37_r79_fixed_policy_pure.cpp tests/test_v37_r79_fixed_runtime_static.py tests/test_r79_runtime_static.py tests/test_v36d2_r79_fast_path_static.py
git commit -m "feat: fix r79 production transport"
```

### Task 3: Delete Retired R79 Experiments and Timing Capture

**Files:**
- Delete: `r79_phase_walk_pure.h`
- Delete: `r79_phased_scheduler_pure.h`
- Delete: `r79_periodic_scheduler_pure.h`
- Delete: `r79_quiet_window_pure.h`
- Delete: `r79_pre_mux1_pure.h`
- Delete: `r79_post_mux2_pure.h`
- Delete: `r79_roaming_burst_pure.h`
- Delete: `r79_mux0_collision_pure.h`
- Delete: `r79_timing_capture.h`
- Delete: `r79_timing_capture.cpp`
- Modify: `T2CAN-Universal-v3.6f3-LP_YL.ino`
- Modify: `vehicle_logic.h`
- Modify: `can_runtime.h`
- Modify: `web_api.h`
- Modify: `summon_state_pure.h`
- Delete or replace: legacy R79 tests listed in Step 5
- Create: `tests/test_v37_r79_retired_absent.py`
- Create: `tests/test_v37_r79_safety_pure.cpp`

**Interfaces:**
- Consumes: fixed R79 interfaces from Task 2.
- Produces: a production tree with no alternative R79 transport/scheduler/capture implementation or route.

- [ ] **Step 1: Write the failing removal contract**

```python
# tests/test_v37_r79_retired_absent.py
from pathlib import Path

root = Path(__file__).resolve().parents[1]
product = "\n".join(
    p.read_text(encoding="utf-8")
    for p in root.iterdir()
    if p.suffix in {".h", ".cpp", ".ino", ".html"} and p.name != "index_html.h"
)
for token in (
    "R79_TRANSPORT_ROAMING_MIRROR", "R79_TRANSPORT_V26_LEGACY",
    "R79_TRANSPORT_MUX0_COLLISION", "r79PostMux2", "r79PreMux1",
    "R79_REFRESH_SUMMON_ONLY", "R79_REFRESH_OFF",
    "R79_SCHED_GUARDED_487_PHASE_WALK", "r79PhasedScheduler",
    "r79RoamingBurst", "r79TimingCapture", "/api/r79timing/",
):
    assert token not in product, token
for removed in (
    "r79_phase_walk_pure.h", "r79_phased_scheduler_pure.h",
    "r79_periodic_scheduler_pure.h", "r79_quiet_window_pure.h",
    "r79_pre_mux1_pure.h", "r79_post_mux2_pure.h",
    "r79_roaming_burst_pure.h", "r79_mux0_collision_pure.h",
    "r79_timing_capture.h", "r79_timing_capture.cpp",
):
    assert not (root / removed).exists(), removed
```

- [ ] **Step 2: Run the removal contract to verify RED**

```bash
python3 tests/test_v37_r79_retired_absent.py
```

Expected: multiple retired symbols/files are present.

- [ ] **Step 3: Remove runtime implementations and compile dependencies**

Delete alternative enums, state, counters, functions, timing slots and includes. In `can_runtime.h`, remove roaming cancellation, POST/PRE/MUX0 observers and timing-capture calls. Keep only fixed MUX1/MUX2 dispatch and `r79FixedTick()`.

- [ ] **Step 4: Remove obsolete API fields and routes**

Delete LAB mutation handlers, roaming silence, timing capture start/sync/stop/download, strategy-specific counters and reset-stat assignments. Keep `/api/r79/stats`, `/api/r79/update`, core R79 counters and BUS OFF trace source naming.

- [ ] **Step 5: Remove superseded tests and replace retained contracts**

Delete these feature-presence tests because their production features are deliberately removed:

```text
tests/test_v36c2_r79_refresh_pure.cpp
tests/test_v36d1_r79_transport_pure.cpp
tests/test_v36d1_r79_transport_static.py
tests/test_v36d7_r79_phased_scheduler_pure.cpp
tests/test_v36d9_r79_phasewalk_pre_pure.cpp
tests/test_v36d9_r79_phasewalk_pre_static.py
tests/test_v36d9a3_r79_anchor_pure.cpp
tests/test_v36d9a3_r79_timing_anchor_static.py
tests/test_v36d9a4_post_mux2_pure.cpp
tests/test_v36d9a4_post_mux2_static.py
tests/test_v36d9a5_r79_transport_sources_static.py
tests/test_v36e1_r79_diagnostics_static.py
tests/test_v36f1_roaming_burst_pure.cpp
tests/test_v36f3_r79_mux0_collision_contract.py
tests/test_v36f3_r79_mux0_collision_pure.cpp
```

Replace the retained priority/retry assertions from `test_v36d1_r79_transport_pure.cpp` with `tests/test_v37_r79_safety_pure.cpp`. Split `test_v36f1_scroll_burst_static.py`: retain AP Right Scroll coverage in the v3.7 scroll contract and remove ROAMING assertions. Update `test_v36c2_contract_static.py`, `test_v36d2_r79_fast_path_static.py`, `test_v36d3_followup_static.py`, `test_v36d7_contract_static.py`, `test_r79_runtime_static.py` and `test_r79_dashboard_static.py` to preserve unrelated coverage while asserting only the fixed v3.7 R79 dispatch and counters.

- [ ] **Step 6: Run removal and compile contracts**

```bash
python3 tests/test_v37_r79_retired_absent.py
python3 tests/test_compile_contract.py
python3 tests/test_v36d7_contract_static.py
python3 tests/test_r79_runtime_static.py
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v37_r79_safety_pure.cpp -o /private/tmp/test_v37_r79_safety && /private/tmp/test_v37_r79_safety
```

Expected: all pass; `rg` finds no retired token in product code.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "refactor: remove retired r79 experiments"
```

### Task 4: Auto Blinker NOA Stabilization and Cancel Pause

**Files:**
- Modify: `auto_blinker_pure.h`
- Modify: `vehicle_logic.h`
- Modify: `can_runtime.h`
- Modify: `web_api.h`
- Create: `tests/test_v37_auto_blinker_timers_pure.cpp`
- Create: `tests/test_v37_auto_blinker_timers_static.py`
- Create: `tests/test_v37_s3xy_action_ids_static.py`
- Modify: `tests/test_blinker_tx_policy_pure.cpp`

**Interfaces:**
- Consumes: `handle921()` NOA state, existing planner request session, door/S3XY cancel inputs.
- Produces: `AutoBlinkerTimingStatePure`, the exact pure interfaces below, persisted durations and dashboard/API telemetry.

```cpp
void autoBlinkerObserveNoaPure(
    AutoBlinkerTimingStatePure &state, uint32_t nowMs,
    bool stateValid, bool noaActive, uint8_t stabilizationSeconds);
bool autoBlinkerNoaReadyPure(
    const AutoBlinkerTimingStatePure &state, uint32_t nowMs);
AutoBlinkerCancelActionPure autoBlinkerCancelTogglePure(
    AutoBlinkerTimingStatePure &state, uint32_t nowMs,
    bool cancelEligible, uint8_t pauseSeconds);
bool autoBlinkerPauseActivePure(
    AutoBlinkerTimingStatePure &state, uint32_t nowMs);
void autoBlinkerTimingResetPure(AutoBlinkerTimingStatePure &state);
```

`stateValid=false` clears stabilization. A valid inactive observation clears it; a valid inactive→active transition starts a full deadline. The pause-active query may consume natural expiry. Recovery/profile changes call the explicit reset helper.

- [ ] **Step 1: Write failing pure timer tests**

```cpp
// representative cases in tests/test_v37_auto_blinker_timers_pure.cpp
AutoBlinkerTimingStatePure s = {};
autoBlinkerObserveNoaPure(s, 1000u, true, true, 10u);
assert(!autoBlinkerNoaReadyPure(s, 10999u));
assert(autoBlinkerNoaReadyPure(s, 11000u));
autoBlinkerObserveNoaPure(s, 12000u, true, false, 10u);
assert(!s.noaActive);

assert(autoBlinkerCancelTogglePure(s, 20000u, true, 20u) ==
       AUTO_BLINKER_CANCEL_START_PAUSE_PURE);
assert(autoBlinkerPauseActivePure(s, 20999u));
assert(autoBlinkerCancelTogglePure(s, 21000u, false, 20u) ==
       AUTO_BLINKER_CANCEL_RELEASE_PAUSE_PURE);
assert(!autoBlinkerPauseActivePure(s, 21000u));
```

Add rollover cases near `0xFFFFFFFF`, invalid duration sanitization, NOA exit/re-entry, recovery reset, natural pause expiry and ineligible first-cancel rejection. Add a static S3XY contract pinning IDs 0–12 and their existing code strings, asserting that no Nag Killer action/token was added.

- [ ] **Step 2: Run tests to verify RED**

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v37_auto_blinker_timers_pure.cpp -o /private/tmp/test_v37_auto_blinker_timers && /private/tmp/test_v37_auto_blinker_timers
python3 tests/test_v37_auto_blinker_timers_static.py
```

- [ ] **Step 3: Implement the pure timing state**

Add exact constants and a state that stores NOA entry/deadline plus pause deadline. All deadline checks use signed deltas. `autoBlinkerCancelTogglePure` must check an existing active pause before evaluating `cancelEligible`, so the second press always releases.

```cpp
static constexpr uint8_t BLINKA_NOA_STABILIZE_MIN_S_PURE = 1u;
static constexpr uint8_t BLINKA_NOA_STABILIZE_MAX_S_PURE = 20u;
static constexpr uint8_t BLINKA_NOA_STABILIZE_DEFAULT_S_PURE = 10u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_MIN_S_PURE = 10u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_MAX_S_PURE = 100u;
static constexpr uint8_t BLINKA_CANCEL_PAUSE_DEFAULT_S_PURE = 20u;
```

- [ ] **Step 4: Integrate NOA stabilization into planner gating**

Update `handle921()` to observe valid NOA transitions. Require `autoBlinkerNoaReadyPure()` in `autoBlinkerCurrentRequestDir()`, `evaluateAutoBlinker()` and the pre-fire decision. Clear timing state on profile/recovery boundaries. Do not use the stabilization gate in direct S3XY Left/Right request paths.

- [ ] **Step 5: Unify door and S3XY cancel handling**

Create one helper used by `handle102LaneChangeCancel()` and `handleS3xySingleAction()`:

```cpp
static AutoBlinkerCancelActionPure handleAutoBlinkerCancelToggle(
    uint8_t source, bool edgeTriggered) {
  // release active pause first; otherwise validate fresh NOA/request,
  // clear pending Auto Blinker state, arm snooze, then start pause.
}
```

The release branch returns before arming `ulcSnoozePending`. The start branch retains all existing request freshness/direction checks.

- [ ] **Step 6: Add production configuration and telemetry APIs**

Extend the Auto Blinker production stats/update API with `noaStabilizationSeconds`, `cancelPauseSeconds`, `noaStabilized`, `noaStabilizationRemainingMs`, `cancelPaused` and `cancelPauseRemainingMs`. Validate ranges before changing runtime state or writing NVS.

- [ ] **Step 7: Run focused tests to verify GREEN**

Run Step 2 plus:

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_blinker_tx_policy_pure.cpp -o /private/tmp/test_blinker_tx_policy && /private/tmp/test_blinker_tx_policy
python3 tests/test_blinker_tx_policy_static.py
python3 tests/test_s3xy_stock_synced_stalk_static.py
python3 tests/test_v36e2_s3xy_direct_blinker_static.py
python3 tests/test_v37_s3xy_action_ids_static.py
```

- [ ] **Step 8: Commit**

```bash
git add auto_blinker_pure.h vehicle_logic.h can_runtime.h web_api.h tests/test_v37_auto_blinker_timers_pure.cpp tests/test_v37_auto_blinker_timers_static.py tests/test_v37_s3xy_action_ids_static.py tests/test_blinker_tx_policy_pure.cpp
git commit -m "feat: add auto blinker stabilization and pause"
```

### Task 5: All-Profile AP Right Scroll Visual-Warning Recovery

**Files:**
- Modify: `ap_right_scroll_pure.h`
- Modify: `can_core.h`
- Modify: `vehicle_profile.h`
- Modify: `vehicle_logic.h`
- Modify: `can_runtime.h`
- Modify: `web_api.h`
- Modify: `tests/test_v36f1_ap_right_scroll_pure.cpp`
- Create: `tests/test_v37_ap_right_scroll_visual_pure.cpp`
- Create: `tests/test_v37_ap_right_scroll_routes_static.py`

**Interfaces:**
- Consumes: accepted stock CAN-B `0x3C2` MUX1, AP-active state and shared Hands-On warning epoch/active state.
- Produces: extended `ApRightScrollStatePure`, the exact scheduler interface below, `vehicleProfileApRightScrollSupported()`, warning repeat setting and all-profile CAN-B routing.

```cpp
ApRightScrollActionPure apRightScrollStepPure(
    ApRightScrollStatePure &state, uint32_t nowMs, bool gateOpen,
    uint16_t regularIntervalSeconds, uint8_t warningRepeatSeconds,
    uint32_t warningEpoch, bool warningActive, bool isMux1,
    uint8_t physicalRightValue);
void apRightScrollTxResultPure(
    ApRightScrollStatePure &state, uint32_t nowMs,
    ApRightScrollActionPure attempted, bool txOk);
```

`apRightScrollStepPure()` only decides an action for an accepted stock MUX1 frame. `apRightScrollTxResultPure()` commits successful UP/DOWN phase changes and fully resets a failed generated pair so DOWN cannot remain latched.

- [ ] **Step 1: Write failing warning scheduler tests**

Test these exact sequences:

```cpp
ApRightScrollStatePure s = {};
assert(apRightScrollStepPure(s, 1000u, true, 30u, 2u, 0u, false, true, 0u) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
auto action = apRightScrollStepPure(s, 1100u, true, 30u, 2u, 1u, true, true, 0u);
assert(action == AP_RIGHT_SCROLL_ACTION_UP_PURE);
apRightScrollTxResultPure(s, 1100u, action, true);
action = apRightScrollStepPure(s, 1120u, true, 30u, 2u, 1u, true, true, 0u);
assert(action == AP_RIGHT_SCROLL_ACTION_DOWN_PURE);
apRightScrollTxResultPure(s, 1120u, action, true);
assert(apRightScrollStepPure(s, 3119u, true, 30u, 2u, 1u, true, true, 0u) == AP_RIGHT_SCROLL_ACTION_NONE_PURE);
assert(apRightScrollStepPure(s, 3120u, true, 30u, 2u, 1u, true, true, 0u) == AP_RIGHT_SCROLL_ACTION_UP_PURE);
```

Add warning clear/resume, physical input during UP and pending DOWN, AP gate close, TX failure reset, regular due collision, repeat sanitization and rollover cases.

- [ ] **Step 2: Run tests to verify RED**

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v37_ap_right_scroll_visual_pure.cpp -o /private/tmp/test_v37_ap_right_scroll_visual && /private/tmp/test_v37_ap_right_scroll_visual
python3 tests/test_v37_ap_right_scroll_routes_static.py
```

- [ ] **Step 3: Extend the pure scheduler**

Add warning interval constants and state fields for warning epoch, warning active, warning due and the source of a pending DOWN. Serialize pairs: a warning edge sets warning due to now; UP sets DOWN pending; DOWN completion sets the next warning repeat deadline to `now + repeatSeconds*1000`. Physical input clears pending generated output and restarts both applicable intervals.

- [ ] **Step 4: Share warning observation across topologies**

Extract the Hands-On transition update in `can_core.h` into `nagObserveHandsOnState(uint8_t ho, uint32_t now)`. Keep Party-CAN behavior unchanged and invoke the same observer from accepted Standard Chassis `0x399` handling. Edge semantics remain `<3 -> 3/4/5`; 3->4->5 is one epoch.

- [ ] **Step 5: Generalize profile support and CAN-B dispatch**

Add:

```cpp
static inline bool vehicleProfileApRightScrollSupported(uint8_t id, uint8_t topology) {
  return vehicleProfileTopologyValid(id, topology) &&
         (id == VEHICLE_MODEL_YL || vehicleProfileCanBIsChassis(id, topology));
}
```

Call `handle3C2OnCanBRightScroll()` for every supported profile. The handler must reject any non-MUX1/non-8-byte frame, never call a CAN-A sender and label the route from `activeProfileIsYl() ? "VH" : "CHASSIS"`.

- [ ] **Step 6: Extend configuration and APIs**

Persist `rsWarnS` with default 2. Extend stats/update with `visualRepeatSeconds`, `visualWarningActive`, `visualWarningEpoch`, `warningRemainingMs`, and `routeName`. Replace the Model-YL-only 409 guard with `activeProfileApRightScrollSupported()`.

- [ ] **Step 7: Run focused tests to verify GREEN**

Run Step 2 plus:

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_v36f1_ap_right_scroll_pure.cpp -o /private/tmp/test_v36f1_ap_right_scroll && /private/tmp/test_v36f1_ap_right_scroll
python3 tests/test_v36f1_scroll_burst_static.py
python3 tests/test_v36b21_driver_monitor_all_profiles_static.py
```

The revised scroll static test must cover only AP Right Scroll and must not reintroduce deleted ROAMING assertions.

- [ ] **Step 8: Commit**

```bash
git add ap_right_scroll_pure.h can_core.h vehicle_profile.h vehicle_logic.h can_runtime.h web_api.h tests/test_v36f1_ap_right_scroll_pure.cpp tests/test_v36f1_scroll_burst_static.py tests/test_v37_ap_right_scroll_visual_pure.cpp tests/test_v37_ap_right_scroll_routes_static.py
git commit -m "feat: add visual warning right scroll recovery"
```

### Task 6: Schema-3 Production Configuration Migration

**Files:**
- Modify: `feature_config_migration_pure.h`
- Modify: `vehicle_logic.h`
- Modify: `web_api.h`
- Modify: `tests/test_feature_config_migration_pure.cpp`
- Create: `tests/test_v37_schema3_migration_static.py`

**Interfaces:**
- Consumes: legacy `r79lab/smart18`, new R79/Auto Blinker/AP Right Scroll runtime configuration from Tasks 2, 4 and 5.
- Produces: feature schema 3, verified production namespaces/keys and post-marker obsolete-key cleanup.

- [ ] **Step 1: Write failing pure migration tests**

Extend the migration pure module with:

```cpp
struct FeatureConfigV37Pure {
  uint8_t r79Bit18Mode;
  uint8_t noaStabilizationSeconds;
  uint8_t cancelPauseSeconds;
  uint8_t rightScrollWarningSeconds;
};
```

Test legacy STOCK and FORCE-0 preservation, invalid/legacy FORCE-1 mapping to the new FORCE-0 default, and 10/20/2 defaults. Simulate failure at value write, read-back verification and marker commit; cleanup must be false in all three cases and true only after the marker is durable.

- [ ] **Step 2: Run migration tests to verify RED**

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. tests/test_feature_config_migration_pure.cpp -o /private/tmp/test_feature_config_migration && /private/tmp/test_feature_config_migration
python3 tests/test_v37_schema3_migration_static.py
```

- [ ] **Step 3: Implement schema-3 ordered writes and verification**

Use production keys:

```text
r79/bit18
summon/noaStabS
summon/cancelPauseS
features/rsWarnS
```

`featureConfigMigrateToSchema3()` reads legacy `r79lab/smart18`, preserving values 0=STOCK and 1=FORCE-0 while mapping removed/invalid values (including legacy FORCE-1) to FORCE-0. It writes/sanitizes the four new values, reads each back, then commits `t2meta/schema=3`. It must be idempotent and return false without cleanup on any failure.

- [ ] **Step 4: Remove obsolete keys only after marker durability**

After schema 3 is committed, remove the legacy R79 keys:

```text
smart18 strategy period refresh sched shots postTx qAnchor
post2En post2Off preEn preOff d2FastEcho d4Period d6Quiet
```

Delete the old `r79LabCfgLoad/Save` interface. Production `r79CfgLoad/Save` owns only `bit18`.

- [ ] **Step 5: Cover Settings Reset and Factory Reset**

Settings Reset writes FORCE 0 / 10 / 20 / 2 and clears volatile timer state without touching BUS OFF evidence. Factory Reset continues to erase all configuration and evidence per existing behavior.

- [ ] **Step 6: Run migration tests to verify GREEN**

Run Step 2 plus:

```bash
python3 tests/test_feature_domain_contract.py
python3 tests/test_can_busoff_persistence_contract.py
```

- [ ] **Step 7: Commit**

```bash
git add feature_config_migration_pure.h vehicle_logic.h web_api.h tests/test_feature_config_migration_pure.cpp tests/test_v37_schema3_migration_static.py
git commit -m "feat: migrate v3.7 production settings"
```

### Task 7: v3.7 Dashboard and Read-Only R79 Card

**Files:**
- Modify: `dashboard_source.html`
- Regenerate: `index_html.h`
- Create: `tests/test_v37_dashboard_contract.py`
- Modify: `tests/test_dashboard_feature_promotion_static.py`
- Modify: `tests/test_v36c7_api_key_contract.py`
- Modify: `tests/c7_api_keys_expected.json`

**Interfaces:**
- Consumes: production APIs from Tasks 2, 4, 5 and 6.
- Produces: Settings controls for bit18/timers, read-only LAB R79 status, updated API key contract and embedded dashboard.

- [ ] **Step 1: Write the failing dashboard contract**

Require:

```python
for token in (
    'R79 bit18', 'STOCK', 'FORCE 0',
    'NOA Stabilization', 'Cancel Pause',
    'Visual Warning Repeat', 'Fixed R79 Policy',
):
    assert token in dashboard
for retired in (
    'R79 Transport', 'POST-MUX2 TX', 'PRE-MUX1 TX',
    'Periodic Scheduler', 'Quiet Window Event Anchor',
    'ROAMING Mirror Ratio', 'ROAMING MUX1 Burst', 'MUX0 → TX Delay',
):
    assert retired not in dashboard
```

Parse inputs and assert NOA 1..20/default 10, pause 10..100/default 20 and warning 1..5/default 2. Assert the R79 LAB card has no input/select/button descendants.

- [ ] **Step 2: Run the dashboard contract to verify RED**

```bash
python3 tests/test_v37_dashboard_contract.py
```

- [ ] **Step 3: Implement Settings controls and live states**

Add the bit18 selector under Summon Monitor, two Auto Blinker duration inputs, and AP Right Scroll warning interval/route. Update fetch/render/save code to use the production routes and show remaining timers without overwriting focused numeric inputs.

- [ ] **Step 4: Replace R79 LAB controls with one read-only card**

Remove all legacy R79 form controls and action buttons. Render fixed copy and core counters from `/api/r79/stats`. The card must state the active bit18 choice rather than implying FORCE 0 when STOCK is selected.

- [ ] **Step 5: Update API key preservation contracts**

Remove only the explicitly retired R79 keys from `c7_api_keys_expected.json`; add the v3.7 fields. Keep every unrelated API key unchanged. Update the contract scripts with an explicit retired-key allowlist so deletion is deliberate and reviewed rather than silently accepted.

- [ ] **Step 6: Build and run dashboard checks**

```bash
python3 tests/test_v37_dashboard_contract.py
python3 tests/test_dashboard_feature_promotion_static.py
python3 tools/build_dashboard.py
python3 tests/test_v36c6_binary_size_static.py
python3 tests/test_dashboard_profile_visibility_regression.py
```

Extract all source and embedded `<script>` blocks and run `node --check`. Expected: five source plus five embedded scripts pass; gzip is below 84,000 bytes.

- [ ] **Step 7: Verify mobile layout in the in-app browser**

At 390×844 verify Auto Blinker, Summon Monitor/R79, AP Right Scroll and LAB R79 panels have no clipping or horizontal overflow. Confirm removed controls leave no blank rows and the read-only card has no interactive descendant.

- [ ] **Step 8: Commit**

```bash
git add dashboard_source.html index_html.h tests/test_v37_dashboard_contract.py tests/test_dashboard_feature_promotion_static.py tests/test_v36c7_api_key_contract.py tests/c7_api_keys_expected.json
git commit -m "feat: add v3.7 production controls"
```

### Task 8: Full Regression, Metadata, Audit, and v3.7 Source Package

**Files:**
- Modify: `CHANGELOG.md`
- Modify: `VALIDATION.md`
- Verify: all production/test files
- Create outside source tree: `T2CAN-Universal-v3.7-LP_YL-source.zip`

**Interfaces:**
- Consumes: all prior task outputs.
- Produces: one reviewed v3.7 source tree and verified source archive.

- [ ] **Step 1: Run every Python/static contract**

```bash
python_count=0
for test_file in tests/test_*.py; do
  python3 "$test_file"
  python_count=$((python_count + 1))
done
echo "Python/static tests passed: $python_count"
```

- [ ] **Step 2: Compile and run every host C++ test**

```bash
cpp_count=0
mkdir -p /private/tmp/t2can-v37-host-tests
for test_file in tests/*.cpp; do
  test_name="$(basename "$test_file" .cpp)"
  c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -I. "$test_file" -o "/private/tmp/t2can-v37-host-tests/$test_name"
  "/private/tmp/t2can-v37-host-tests/$test_name"
  cpp_count=$((cpp_count + 1))
done
echo "Host C++ tests passed: $cpp_count"
```

- [ ] **Step 3: Audit deleted R79 symbols and CAN transmit call sites**

```bash
python3 tests/test_v37_r79_retired_absent.py
rg -n '0x3FD|twai_transmit|canTxTwaiTransmit' vehicle_logic.h can_runtime.h web_api.h
rg -n '0x3C2|handle3C2OnCanBRightScroll|canTxMcpSend' vehicle_logic.h can_runtime.h
```

Expected: R79 output is limited to fixed Fast Echo, fixed periodic and their bounded retry path; AP Right Scroll has no CAN-A sender.

- [ ] **Step 4: Rebuild twice and prove deterministic output**

```bash
python3 tools/build_dashboard.py
first_hash="$(shasum -a 256 index_html.h | awk '{print $1}')"
python3 tools/build_dashboard.py
second_hash="$(shasum -a 256 index_html.h | awk '{print $1}')"
test "$first_hash" = "$second_hash"
python3 tests/test_v36c6_binary_size_static.py
```

- [ ] **Step 5: Compile the generated header and target when available**

```bash
c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Itests/host_generated_header -I. tests/host_generated_header/test_index_html_compile.cpp -o /private/tmp/t2can-v37-host-tests/test_index_html_compile
/private/tmp/t2can-v37-host-tests/test_index_html_compile
if command -v arduino-cli >/dev/null 2>&1; then
  arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc' .
else
  echo 'arduino-cli unavailable; no target-build claim will be recorded'
fi
```

- [ ] **Step 6: Record exact release evidence**

Add top v3.7 sections to CHANGELOG and VALIDATION with exact Python/C++ counts, dashboard source/minified/gzip sizes, `index_html.h` SHA-256, R79 call-site audit, migrations, mobile measurements, and target-build result. Do not claim an ESP32-S3 binary build when the toolchain is absent.

- [ ] **Step 7: Commit metadata and confirm a clean tree**

```bash
git add CHANGELOG.md VALIDATION.md index_html.h
git commit -m "docs: record v3.7 validation"
git diff --check
git status --short
git log --oneline --decorate -10
```

- [ ] **Step 8: Create and verify the v3.7 archive**

Create a temporary staging directory whose archive root is exactly `T2CAN-Universal-v3.7-LP_YL/`, copy the reviewed source tree into it while excluding `.git`, `.superpowers`, `__pycache__`, `.pyc` and temporary build output, then create `T2CAN-Universal-v3.7-LP_YL-source.zip`. Verify with `unzip -t`, inspect archive names and version strings, compute SHA-256, and copy the archive to the project root without modifying `sources/`.
