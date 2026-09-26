# T2CAN Universal v3.7
## Cumulative Changes Since v3.5a1

`v3.7` is a major evolution from the `v3.5a1` baseline, spanning 47 named development revisions across the v3.6 beta / c / d / e / f branches.

This document summarizes the cumulative differences from `v3.5a1` to the final `v3.7` production architecture. Intermediate experiments are included only where they explain the final design, and features that were tested but later retired are listed separately.

---

## 1. R79 / Summon — Transport Architecture Rebuilt

### v3.5a1 baseline

`v3.5a1` used the V2.6-style model:

- Immediate R79 transmission after a real `0x3FD MUX1`
- Independent 500 ms periodic refresh
- No active pending/coalescing/retry service
- No destructive SUMMON_FULL queue flush/retry architecture

### Development through v3.6

The v3.6 development line extensively tested:

- Fast Reactive Echo
- 0 ms vs 2 ms post-MUX1 queue admission
- 487 ms phase walking
- MUX2 Quiet Window scheduling
- PRE-MUX1 injection
- POST-MUX2 injection
- MUX0 collision scheduling
- Multi-shot R79 transmission
- ROAMING mirror transport
- V2.6 legacy transport
- Summon-priority queue admission/retry behavior
- Microsecond-level timing capture
- Video-sync logging

### Final v3.7 production policy

`v3.7` removes the experimental transport matrix and replaces it with one fixed production policy:

- Accepted stock `MUX1` → modified R79 echo with a bounded **2 ms queue wait**
- Accepted stock `MUX2` → one **+150 ms refresh**
- `bit19 = 0`
- `bit47 = 1`

`SmartSummonOnly / bit18` is now the only user-selectable R79 payload policy:

- **FORCE 0** — default
- **STOCK**

The following experimental controls were removed from the production UI/runtime:

- PRE-MUX1
- POST-MUX2
- Quiet Window scheduling
- 487 ms phase-walk
- MUX0 Collision
- ROAMING transport selection
- V2.6 transport selection
- R79 Timing Capture

Manual Drive/Reverse suppression and CAN administrative safety gates remain integrated into the production transport.

**Net change:** R79 evolved from a relatively simple v3.5a1 immediate + periodic transmitter, through a large timing-research platform, into a smaller fixed stock-synchronized production transport.

---

## 2. Mode H — Expanded into a Full Profile System

### v3.5a1

Mode H was essentially the original Human Interaction implementation:

- Production default peak: **1.50–2.00 Nm**
- LAB range: **1.00–3.00 Nm**
- WAIT: **1.2–3.0 s**
- REFRACTORY: **0.8–1.8 s**
- Comparatively simple Hands-On handling

### v3.7

Mode H now has a full profile architecture developed through Rev.1–Rev.4.

Final Rev.4 production profile:

- Primary Peak: **1.80–2.60 Nm**
- WAIT: **0.90–3.00 s**
- REFRACTORY: **0.50–1.50 s**
- Opposite Carrier: **0.10–0.60 Nm**
- Primary direction bias: **80% negative / 20% positive**
- TIERED Hands-On policy
- HO=1 threshold: **0.40 Nm**
- HO=2 threshold: **2.00 Nm**
- Generated HO capped at **HO=2**
- Visual Warning Rescue: **ON**
- Rescue Delay: **0.5 s**
- Stationary behavior: **HARD PAUSE**

Rev.4 carrier behavior is tied to every eligible real stock `0x370` RX:

- Meaningful positive stock torque → negative opposite carrier
- Meaningful negative stock torque → positive opposite carrier
- Transmitted magnitude = `|stock| + carrier`
- Near-zero stock torque remains stock-relative
- Carrier remains active through WAIT and REFRACTORY

Additional work since v3.5a1 includes:

- Configurable WAIT / REFRACTORY timing
- Hands-On probability experiments
- Full 2-bit HO handling
- Rev.2 Natural Grip research
- Rev.3 Human Interaction + Natural Grip carrier
- Configurable HO policies and thresholds
- Visual Warning-triggered recovery
- Stock-preserving stopped-state transport experiments
- Persistent per-profile configuration and migration

**Net change:** Mode H is no longer one tunable waveform. It became a multi-generation steering-interaction engine with distinct carrier, event, Hands-On, warning-recovery and persistence logic.

---

## 3. Auto Blinker — Major Behavioral Redesign

Added since `v3.5a1`:

- Independent **NOA-entry stabilization timer**
  - Range: **1–20 s**
  - Default: **10 s**
- Auto Blinker will not initiate until NOA has remained eligible for the full stabilization period
- Shared **Cancel Pause** state
  - Door Open cancel
  - S3XY Auto Blinker Cancel
  - Range: **10–100 s**
  - Default: **20 s**
- Pressing Cancel again releases the pause early
- Auto Blinker and direct S3XY blinker actions are logically separated
- Production Auto Blinker transmission moved to a **single-TX policy**
- Legacy 350 ms burst remains only as a volatile LAB A/B comparison

**Net change:** fewer unnecessary repeated CAN transmissions, clearer cancellation behavior, and better separation between automatic and direct user-triggered turn-signal actions.

---

## 4. S3XY Button Support Expanded

S3XY integration grew substantially beyond the v3.5a1 baseline.

Added or revised actions include:

- Auto Blinker Toggle
- Auto Blinker Cancel
- TLSSC Toggle
- Acceleration Mode actions
- Performance Mode
- Left Blinker
- Right Blinker

Direct Left/Right Blinker actions:

- No longer require Auto Blinker to be enabled
- Do not require NOA/ALC eligibility
- Use the selected vehicle's actual stalk/stalkless transport
- On stalk vehicles, synchronize with the next real stock `0x249`
- Consume the direct request once instead of using a free-running burst
- Yield to real physical stalk input

S3XY mappings also survive vehicle-profile changes even when an action is temporarily unsupported by the selected topology.

---

## 5. Lane-Change Cancel Behavior Corrected

The current v3.7 package also changes the S3XY and Door Open lane-change cancel path.

Previously, manual cancel required instantaneous:

`DAS_behaviorType = LANE_CHANGE_LEFT / LANE_CHANGE_RIGHT`

This could block cancellation at the **first Tesla lane-change notification** while the planner was still reporting `IN_LANE`.

Current v3.7 behavior:

- **S3XY Cancel and Door Open Cancel no longer use `DAS_behaviorType` as an eligibility gate**
- Existing NOA/context freshness checks remain
- Existing CAN admission protections remain
- `UI_ulcSnooze` can therefore be armed while the planner still reports `IN_LANE`

Auto Blinker itself continues to use `DAS_behaviorType` normally for automatic lane-change detection.

---

## 6. AP Right Scroll — New Production Feature

A feature that did not exist in v3.5a1.

Originally developed as a YL LAB experiment, AP Right Scroll is now a production Settings feature.

- Supported on Model YL and standard Model 3/Y profiles
- Routed through **CHASSIS / CAN B**
- Injects an UP/DOWN right-scroll pair using real stock frames
- Physical right-scroll input always has priority

Visual-warning recovery:

- Newly detected Hands-On visual warning → immediate UP/DOWN pair
- Persistent warning → repeat every **1–5 s**
- Default: **2 s**
- Warning clear, AP exit, CAN recovery, or administrative hold cancels pending input

---

## 7. AP Drive Profile / PedalMap Matured

AP Drive Profile moved from YL-only LAB research into a production Settings feature.

Supported behavior includes:

- Automatic `0x334` ownership while AP is active
- CHILL / Comfort pedal-map application
- Regenerative braking selection:
  - STANDARD `20`
  - REDUCED `10`
  - MINIMAL `1`
- Model YL VH/CAN-B routing
- Standard Model 3/Y Body+Chassis routing
- Stock-follow transmission
- Counter/checksum regeneration
- Immediate configuration updates while AP remains active

Manual PedalMap control was rebuilt as a drive-session override:

- STOCK
- CHILL
- SPORT
- PERFORMANCE
- RAM-only session semantics
- Park / STOCK / reboot release behavior
- Manual selection survives temporary AP Drive Profile ownership

---

## 8. `0x3F8` Driver-Assist Architecture Consolidated

A large amount of `0x3F8` research occurred through v3.6.

Development included:

- Confirm-Free Lane Change
- PRE-AP Confirm-Free timing
- CAN A / CAN B target experiments
- ULC Speed Config
- ULC Off-Highway
- ULC Blind Spot
- ACC Follow Distance
- AP Right Scroll
- Dual-bus `0x3F8` comparison

By v3.7/f3, production `0x3F8` modifications were consolidated into a **single CAN-B stock-frame compositor**.

Production/promoted functionality includes:

- Confirm-Free
- ULC Off-Highway
- ULC Blind Spot
- PedalMap Control
- AP Right Scroll
- Mode H Profiles

Research that did not justify remaining in the production interface was removed or returned to LAB.

Removed from the production configuration surface:

- ACC Follow Distance
- ULC Speed Config
- Old `0x3F8` target-bus selector

---

## 9. `0x293` Auto Lane Change Research Added

`v3.5a1` did not include the later `0x293` research stack.

v3.6 added:

- Independent CAN A / CAN B observation
- `UI_autoLaneChangeEnable` decoding
- Same-bus injection
- Counter advancement
- Checksum regeneration
- Selectable CAN A / CAN B / BOTH target
- Independent `0x293` and `0x3F8` routing

In final v3.7 this remains an explicit **LAB research feature**, rather than being promoted to normal Settings.

---

## 10. Driver Monitoring Capture Expanded to All Profiles

Originally introduced as a YL-only RX research recorder, it was later generalized to every supported vehicle profile.

Tracked research frames include:

- `0x389`
- `0x5D9`
- `0x247`
- `0x399`
- `0x370`

on both physical buses.

Capture output preserves:

- Physical CAN side
- Logical bus role
- PRE / POST phase
- Raw payload
- Decoded driver interaction state where available
- DAS Hands-On
- EPAS Hands-On
- EPAS torsion-bar torque

The recorder remains **RX-only**.

---

## 11. CAN Research / Diagnostics Expanded

Since v3.5a1, diagnostics expanded to include:

- Independent CAN-A and CAN-B TX traces
- CAN-A MCP2515 BUS-OFF snapshots
- CAN-B TWAI BUS-OFF snapshots
- Queue-depth telemetry
- Recovery-cause tracking
- Task heartbeat monitoring
- CAN A / CAN B / BOTH heartbeat timeout attribution
- Driver Monitoring CSV
- Targeted ULC / Confirm-Free RAW capture
- Physical-bus vs logical-role capture
- Persistent previous-boot BUS-OFF evidence
- Bounded pre-failure TX history

BUS-OFF evidence is stored in verified dual-slot NVS records and survives reboot until explicitly cleared.

---

## 12. CAN Runtime and TX Safety Hardened

The CAN transport layer gained substantially more protection since v3.5a1:

- Centralized TX admission/barrier handling
- Administrative TX hold
- Recovery epoch invalidation
- Stale-template invalidation
- Same-bus stock-template enforcement
- Queue-headroom management
- Physical-input priority
- Pending-action cancellation across CAN recovery
- Controlled profile/reset transitions
- Application-TX serialization around administrative state changes

This is one of the largest architectural changes even where visible feature behavior appears unchanged.

---

## 13. Vehicle-Profile Awareness Expanded

Profile-dependent routing is more deeply integrated across the firmware.

Examples:

- Model YL `PARTY + VH`
- Standard Model 3/Y `BODY + CHASSIS`
- `PARTY + CHASSIS`
- Stalk / stalkless turn-signal paths
- Profile-specific `0x334`, `0x3F8`, `0x293`, DAS-warning and Driver Monitoring routing

Unsupported controls are dynamically removed from the dashboard instead of simply failing at runtime.

---

## 14. Dashboard / Mobile UI Reworked

The v3.5 generation introduced the dark mobile dashboard, but v3.6 → v3.7 continued substantial UI work:

- Profile-aware Quick Controls
- One-row supported-control layouts
- Adaptive bottom navigation
- Correct hidden-state CSS hierarchy
- Compact Firmware Profile
- Redesigned Injection State
- Improved submenu animation
- Disabled-control visual states
- Dirty/focus protection for editable LAB inputs
- Prevention of background polling overwriting user edits
- iOS/WebKit scroll restoration
- Screenshot/viewport stabilization
- Dark-mode select-arrow fixes
- Correct CSV download completion handling
- Reduced dashboard polling during large downloads

The dashboard build system was also optimized to keep the embedded gzip below **84,000 bytes**.

---

## 15. Firmware Size / Code Architecture Cleanup

The v3.6 line included major internal cleanup:

- Removal of dead helpers and retired compatibility wrappers
- Removal of unused diagnostics state
- Removal of retired Mode D/E/F NAG engines
- Fixed-point integer formatting in high-cost API paths
- Removal of unnecessary floating-point formatting/parsing
- Shared JSON writer implementation
- Snapshot/API serialization consolidation
- Compile-time serial diagnostics exclusion
- Reduced S3XY diagnostic code in production builds
- Dashboard minification before gzip
- Removal of retired development source snapshots

The current production source package additionally removes host-test and internal planning artifacts while retaining required runtime source.

---

## 16. Persistence / Migration Redesigned

Feature persistence now uses ordered schema migration with read-back verification.

By v3.7, feature-domain migration includes items such as:

- R79 `bit18`
- Auto Blinker NOA stabilization time
- Cancel-pause duration
- AP Right Scroll warning-repeat interval

Obsolete experimental keys are deleted **only after**:

1. New values are written
2. Values are read back successfully
3. The new schema marker is committed

Mode H received separate schema migrations as Rev.2 / Rev.3 / Rev.4 configuration evolved.

---

## 17. Features / Experiments Retired Since v3.5a1

A substantial amount of development was intentionally **not** carried into the final production configuration.

Removed or retired:

- NAG Mode D
- NAG Mode E
- NAG Mode F
- R79 free-running experimental schedulers
- R79 PRE-MUX1
- R79 POST-MUX2
- Quiet Window scheduler
- 487 ms phase-walk
- MUX0 Collision
- ROAMING transport selector
- V2.6 transport selector
- R79 Timing Capture
- ACC Follow Distance
- ULC Speed Config
- `0x3F8` target-bus experiment
- TLSSC Green-Light Test 1 / Test 2
- Old Mode H Rev.1 Plus slot, replaced by Rev.4

This is important because **v3.7 is not simply v3.5a1 plus every experiment added during development**. Much of the v3.6 work was used to determine what should *not* remain in the production path.

---

## 18. Reliability / Build Fixes

Across the development line:

- Multiple Arduino declaration-order regressions were fixed
- Dashboard/API JSON compile issues were corrected
- CAN traffic snapshot helpers were restored where refactors dropped them
- Retired R79 symbol references were removed
- Administrative CAN TX hold handling was restored and serialized through the TX barrier
- Dashboard source/embedded gzip consistency checks were added
- Host-side pure logic and static regression coverage expanded significantly

---

# Short Release Summary

## v3.5a1

Dark-dashboard Universal firmware with V2.6-style R79 immediate + 500 ms periodic behavior and the earlier Mode H / Auto Blinker architecture.

## v3.7

Profile-aware production firmware with:

- Fixed stock-synchronized R79 transport
- Rev.4 Mode H
- Stabilized Auto Blinker
- Shared lane-change cancel/pause handling
- Stock-synchronized S3XY blinkers
- AP Right Scroll warning recovery
- Consolidated `0x3F8` injection
- Expanded diagnostics/capture
- Persistent BUS-OFF forensics
- Verified NVS migration
- Hardened CAN TX/recovery infrastructure

---

## Overall

`v3.5a1 → v3.7` is substantially more than a minor update.

The biggest changes are:

1. **R79 transport was researched, simplified, and rebuilt around stock synchronization**
2. **Mode H became a full Rev.4 interaction profile system**
3. **Auto Blinker / S3XY / Door Open cancel behavior was redesigned**
4. **The CAN transport/recovery layer became much more defensive**
5. **Profile-specific routing and production UI behavior became significantly more mature**
6. **A large number of v3.6 experiments were intentionally removed rather than carried forward**

In architectural terms, v3.7 represents a new production generation built on top of the v3.5a1 baseline.
