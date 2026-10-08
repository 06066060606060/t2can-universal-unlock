# T2CAN Universal v3.7 Fixed R79, Auto Blinker Timers, and AP Right Scroll Design

> **Post-release hotfix:** Manual S3XY/door lane-change cancel no longer uses `DAS_behaviorType` LEFT/RIGHT as an eligibility or direction gate. The original v3.7 plan below documents the release baseline; runtime behavior is superseded by the hotfix recorded in `CHANGELOG.md` and `VALIDATION.md`.

## Goal

Produce T2CAN Universal **v3.7** from the verified v3.6f3 feature-promotion source. The release replaces the experimental R79 matrix with one production transport, updates Mode H Rev.4 defaults, adds two independent Auto Blinker timing policies, and extends AP Right Scroll to visual-warning recovery and every supported vehicle profile.

Success means the firmware has fewer R79 transmit paths, every retained transmit remains stock-RX-following or uses the existing bounded R79 scheduler, old settings migrate safely, and the dashboard exposes only production choices.

## Confirmed Product Decisions

### Mode H Rev.4 defaults

Rev.4 defaults are:

- Primary Peak: 1.80–2.60 Nm
- WAIT: 0.90–3.00 s
- REFRACTORY: 0.50–1.50 s
- Opposite Carrier: 0.10–0.60 Nm
- Hands-On Policy: TIERED HO=1 / 2
- HO=1 threshold: 0.40 Nm
- HO=2 threshold: **2.00 Nm**
- Visual Warning Rescue: ON
- Rescue Delay: 0.5 s
- Stop Behavior: **HARD PAUSE · 0-TX**

Nag configuration schema advances to v18. An exact prior Rev.4 default profile migrates to these values. Customized Rev.4 tuning is preserved. Reset Current Profile restores the v3.7 values.

### Fixed production R79 transport

R79 keeps one transport only:

1. Every accepted real stock `0x3FD` MUX1 may produce the existing D9 Fast Echo.
2. Fast Echo copies the stock frame, applies the selected bit18 policy, forces bit19=0 and bit47=1, and uses a bounded 2 ms enqueue wait.
3. Every accepted stock MUX2 anchors one Quiet Window cycle.
4. The cycle has one slot at MUX2 +150 ms.
5. Periodic refresh is always enabled, but the existing R79 authorization, manual-driving suppression, freshness, administrative-hold, queue and recovery gates remain mandatory.
6. The existing bounded retry/emergency queue-recovery policy remains. No removed transport may be used as a fallback.

The only production choice is bit18:

- `STOCK`: preserve stock bit18.
- `FORCE 0`: write bit18=0 on both Fast Echo and periodic frames.

The default is `FORCE 0`. The former SmartSummonOnly selection is migrated into the production setting and preserved when valid.

### R79 code removal

Delete the following implementations, not merely their dashboard controls:

- Transport selection and every non-D9 transport
- ROAMING MIRROR and mirror ratio
- ROAMING MUX1 Burst and TX Silence
- V2.6 LEGACY mirror and periodic transport
- MUX0 Collision transport, reservation queue and delay
- POST-MUX2 TX and delay
- PRE-MUX1 TX and offset
- Guarded 487 ms Phase Walk
- Generic phased scheduler and 2x/3x periodic shots
- Selectable refresh, scheduler, anchor, period and shot settings
- R79 timing-capture experiment, routes and UI
- Obsolete strategy-specific counters, API fields, NVS keys, timing slots and tests

Keep only counters required to explain the production path: accepted stock templates, Fast Echo attempts/OK/fail, periodic attempts/OK/fail, retry/queue recovery, authorization blocks, last request/result and selected bit18 policy.

### R79 UI and API

- `Settings > Summon Monitor` contains the production `R79 bit18` selector (`STOCK` / `FORCE 0`).
- LAB contains one read-only `Fixed R79 Policy` card.
- The card shows bit18 policy, fixed bit19/47 policy, Fast Echo wait, periodic schedule, runtime authorization state and core counters.
- LAB has no R79 selectors, number inputs, start/stop/sync/download actions or mutation routes.
- Production R79 status/update routes replace the LAB mutation interface.

## Auto Blinker v3.7 State Model

### Independent timers

Auto Blinker keeps the existing per-request delay. Two new settings are independent of it:

- NOA Stabilization: 1–20 seconds, default 10 seconds.
- Cancel Pause Duration: 10–100 seconds, default 20 seconds.

Both durations persist in NVS and appear under Settings > Auto Blinker.

### NOA stabilization

- The first valid transition into NOA starts the stabilization deadline.
- Booting or recovering while the first valid observed state is already NOA is treated as a new NOA entry.
- Planner-driven Auto Blinker requests cannot arm or fire before the deadline.
- NOA exit, AP-state invalidation, relevant CAN recovery or profile change clears the deadline.
- Re-entering NOA starts the full configured delay again.
- The existing per-request delay begins only when the request is otherwise eligible after stabilization.
- Direct S3XY Left/Right Blinker actions are unaffected.

### Cancel Pause toggle

The driver-door cancel action and the S3XY `NOA Lane Change Cancel` action share one runtime pause state.

- When not paused, an action must pass the existing fresh-NOA, fresh planner-request and direction gates. It sends the existing snooze/cancel request and starts the configured pause.
- While paused, the next action immediately clears the pause even if no planner request is active. It does not send another cancel frame.
- Natural expiry clears the pause.
- While paused, planner-driven Auto Blinker cannot arm or fire. Pending planner state is cancelled when pause begins.
- Direct S3XY Left/Right Blinker actions remain available.
- Pause state is volatile and never survives reboot, profile change or the applicable CAN recovery boundary.

The dashboard reports stabilization state/remaining time and cancel-pause state/remaining time.

## S3XY Scope

No new S3XY action ID is added in v3.7. Existing persisted IDs and mappings remain unchanged. Existing `Auto Blinker Toggle`, `TLSSC Toggle`, direct blinkers and `NOA Lane Change Cancel` remain available subject to their current profile gates.

## AP Right Scroll v3.7

### Vehicle routing

- Model Y L: stock `0x3C2` MUX1 on VH / CAN B.
- Standard Model 3/Y profiles: stock `0x3C2` MUX1 on Chassis / CAN B.
- AP Right Scroll never transmits on CAN A.
- The outgoing frame copies the accepted stock CAN-B frame and modifies only the existing right-scroll field.

### Schedulers

The existing regular interval remains 1–600 seconds and defaults to 30 seconds.

A visual-warning scheduler is added:

- Hands-On state transition from below 3 into 3/4/5 starts a warning epoch.
- The next eligible stock `0x3C2` MUX1 sends UP, and the following eligible MUX1 sends DOWN.
- While the warning remains in 3/4/5, another UP/DOWN pair is eligible every 1–5 seconds.
- The visual-warning repeat default is **2 seconds**.
- Clearing the warning stops warning repetition and returns scheduling to the regular interval.
- A warning event takes precedence over the regular due time, but both share one serialized UP/DOWN state so pulses cannot overlap.
- Real non-zero driver right-scroll input always wins and postpones the next generated pair.

Visual-warning state is observed from the accepted AP/Hands-On frame on the profile's active bus. The observer is shared so Party-CAN profiles and Standard Chassis-CAN profiles use identical edge semantics.

AP exit, feature disable, profile change, administrative hold, transmission failure or relevant CAN recovery resets pending scroll state. All generated scroll frames remain stock-RX-following; no timer-originated CAN transmission is added.

### AP Right Scroll UI and persistence

Settings > AP Right Scroll contains:

- Enabled
- Regular Interval (1–600 s)
- Visual Warning Repeat (1–5 s, default 2 s)
- Active route (`VH · CAN B` or `CHASSIS · CAN B`)
- Gate, next regular pulse, warning state, next warning repeat, driver deferrals and TX results

Enabled state and both intervals persist. Runtime warning/pulse state does not.

## Persistence and Migration

Global feature schema advances from 2 to 3.

Schema 3 performs ordered, retryable migration:

1. Read and sanitize the legacy R79 SmartSummonOnly value.
2. Write production R79 bit18 policy, Auto Blinker timer defaults/current values, and AP Right Scroll warning interval.
3. Read back and verify every new value.
4. Commit the schema marker.
5. Only after the marker is durable, remove obsolete R79 experiment keys and namespaces.

A failed write, verification or marker commit leaves obsolete values available and retries on the next boot. Runtime always uses safe sanitized values. Settings Reset restores v3.7 defaults while preserving the existing BUS OFF evidence policy. Factory Reset retains its existing broader erasure behavior.

## Safety and Concurrency

- All timer comparisons use rollover-safe signed/unsigned delta checks.
- Shared Auto Blinker and AP Right Scroll state remains protected by the existing critical-section domains.
- Configuration writes occur outside CAN receive critical sections.
- R79, Auto Blinker and AP Right Scroll never block a stock RX path waiting for application state.
- Real driver inputs, administrative hold, profile validity, active CAN freshness and recovery epochs retain priority over generated frames.
- Removed R79 symbols must not remain in production code, generated dashboard code, APIs or persisted configuration paths.

## Dashboard and Versioning

- Firmware version becomes `v3.7` everywhere user-visible and in the build metadata.
- Dashboard generation remains deterministic and the embedded gzip remains below 84,000 bytes.
- The mobile layout must have no horizontal overflow, duplicate IDs or orphaned space from removed R79 controls.
- CHANGELOG and VALIDATION receive a top v3.7 section.

## Verification

Add red/green coverage for:

- Rev.4 HO=2 2.00 Nm and HARD PAUSE default/migration/reset
- R79 STOCK/FORCE-0 bit18 behavior across Fast Echo and periodic frames
- Fixed bit19=0 / bit47=1 and 2 ms Fast Echo admission
- MUX2 +150 ms, one-shot Quiet Window periodic schedule
- Complete absence of removed transports, schedulers, routes, keys and dashboard controls
- Schema-3 write/verify/marker/cleanup ordering and retry behavior
- NOA entry, exit, recovery, rollover and 10-second default stabilization
- Cancel Pause start, second-action release, natural expiry, rollover, shared door/S3XY behavior and direct-blinker independence
- AP Right Scroll immediate warning edge, 2-second default repeat, warning clear, regular-schedule resumption, physical-input priority and serialized UP/DOWN
- YL VH/CAN-B and Standard Chassis/CAN-B routing with no CAN-A AP Right Scroll sender
- Existing S3XY action ID stability
- Dashboard APIs, JavaScript syntax, generated-header compilation, gzip budget and mobile rendering

Run the complete Python/static and host C++ regression suites, rebuild the dashboard twice to prove deterministic output, audit all CAN transmit call sites and deleted R79 symbols, and verify the final v3.7 source archive before delivery.

## Out of Scope

- No new S3XY Nag Killer action.
- No change to direct S3XY Left/Right Blinker transport.
- No change to the optional LAB Single/Legacy Auto Blinker comparison.
- No new background CAN-transmit task.
- No change to BUS OFF persistence semantics.
