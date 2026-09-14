> ⚠️ **Research / educational firmware only**
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.
>
> It sends signals directly to the controller, not a physical command to the steering wheel. **Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk.**
>
> You are responsible for your own testing, wiring, configuration, and compliance with local laws.

--- 

# T2CAN Universal Unlock v3.5a1 by LP_YL  

**Major Universal Release**  
**Release date:** September 2026  
**code entirely rewritten by LP_YL**  
**[Dashboard view](https://06066060606060.github.io/t2can-universal-unlock/)**

**v3.5 Highlights**
- New Nag Killer Mode H — Human Interaction
- Improved Mode D / E / F
- Auto Blinker retry logic
- Summon / R79 reliability improvements
- 1.00–3.00 Nm Mode H torque range
- Full mobile dashboard redesign
- Performance and resource optimizations

## 📋3.0 Release Highlights  

- **One Universal firmware** supporting five Model 3/Y/YL vehicle profiles.
- **multiple operating modes:**  
  - Nag-killer + Eu-Unlock  
  - Advanced EAP + Eu-Unlock 
  - Nag-killer + Advanced EAP + Eu-Unlock (Model YL only)
- **Profile-based CAN topology and turn-signal routing** instead of separate firmware branches or manual transport selection.
- **Direct S3XY Button Bluetooth support** for up to **3 registered buttons**.
- **Runtime Bluetooth Master ON/OFF without MCU reboot**, while preserving saved devices, bonds, mappings, and Auto Connect settings.
- **Reworked Auto Blinker** with profile-aware routing and direction-specific ALC availability gating.
- **Independent R79 engine** with immediate reassertion and independent periodic refresh.
- **Summon Monitor** separated from R79 ownership.
- **Off-Highway ALC BETA** added as a normal vehicle feature.
- **TLSSC Highway Block** added: TLSSC can optionally turn **OFF on highways / controlled-access roads**.
- **CAN Research Capture** with Snapshot, RAW Transition, and Auto ALC Transition modes.
- **Dedicated LAB research area** for R79, ULC Blind Spot, ACC Follow Distance, and CAN capture.
- **New mobile dashboard architecture** with HOME, DEVICES, SETTINGS, and LAB.
- **Configurable Wi-Fi AP**, tiered reset behavior, and expanded CAN/BLE diagnostics.
- **Stronger CAN recovery safety** through the new TX recovery barrier / epoch model.
- **Added optional Pause NAG at 0 km/h** — default OFF   
- **Added topology-aware feature gating** for Advanced EAP, Body controls, NAG and EU Unlock  

**Advanced EAP**
- Automatically activates the turn signal.
- Delay can be configured from the dashboard.
- All lane-change safety features are maintained.
- The turn signal starts only when the vehicle requests a lane change.
- Lane changes can always be cancelled: On screen, using the open-door button or using s3xy button.

**EU Unlock**
- Bypass R79 EU restriction in AP.
- Expand Summon range to ±85 m.
- Expanded lateral acceleration limits.  
- Lane changes near forks are not disabled (EAP).  
- Instantaneous lane change on blinker (EAP).  
- No lane-change timeout once initiated (EAP).  
- Automatically takes forks and exits (EAP).  
- Toggle to activate TLSSC (EAP).  

**NAG-Killer**
- eliminate the "hands on the wheel" prompt while using Autopilot/FSD*

---

# 🚙 1. Universal Vehicle Architecture

### Universal v3.0
Universal v3.0 replaces both separate branches with a persistent **Vehicle Profile** system.

The selected profile determines:

- CAN A / CAN B topology.
- Turn-signal transport.
- Feature availability.
- Vehicle-specific CAN routing.
- Nag Killer availability.
- TLSSC Restore capability.
- Profile-specific safety restrictions.

## ✅ Supported Profiles

| Configuration | NAG Killer | Advanced EAP | Pedal Map | EU Unlock |
|---|:---:|:---:|:---:|:---:|
| **YL · Party + VH** | ✅ | ✅ | ✅ | ✅ |
| **Standard 3/Y · Body + Chassis** | ❌ | ✅ | ✅ | ✅ |
| **Standard 3/Y · Party + Chassis** | ✅ | ❌ | ❌ | ✅ |

Model 3 Highland requires the user to select the physically installed turn-control type during profile setup. Other profiles resolve to Stalk automatically.

---

# 🔧 2. Safe Profile Setup and Migration

Universal v3.0 adds a fail-closed setup state for first boot and major migration.

Before a valid Vehicle Profile is saved:

- CAN transmission is disabled.
- CAN injection is disabled.
- CAN recovery supervision is disabled.
- Bluetooth is not started.
- Wi-Fi and the Profile Setup page remain available.

This prevents an old or incompatible v2.x configuration from authorizing CAN traffic on the wrong vehicle topology.

The one-time migration from the pre-Universal v2.x architecture intentionally performs a **full NVS erase** before the new Universal bootstrap state is written.

---

# 🛠️ 3. CAN Runtime Safety and Recovery

Universal v3.0 introduces a stricter **CAN TX Recovery Barrier / Epoch Model**.

When a CAN controller recovery or reinitialization boundary occurs, transient authorization from the previous CAN session is invalidated.

This includes state used by:

- Auto Blinker.
- R79.
- Pending S3XY actions.
- Temporary vehicle actions.
- Cached stock-frame templates.
- Other transient TX state.

## Recovery Behavior

- Previous-session state cannot silently authorize new TX.
- Required stock templates must be captured again after a relevant recovery boundary.
- Pending actions are cancelled across recovery epochs.
- TX resumes only after the current CAN epoch satisfies the required bus/runtime conditions.

## Expanded CAN Diagnostics

Universal v3.0 adds significantly deeper runtime evidence:

- BUS OFF / STOPPED counters.
- Recovery start / outcome counters.
- Restart success / failure information.
- RX gap telemetry.
- TEC / REC and CAN error information.
- TX queue state.
- BUS OFF snapshots.
- CAN B TX trace export.
- Boot timing / capture diagnostics.

---

# 4. Direct S3XY Button Bluetooth Support — NEW

Direct S3XY Button integration is a new subsystem compared with both predecessor firmware lines.

Universal v3.0 supports **up to 3 registered S3XY Buttons**.

## Device Management

- Persistent button registry.
- Per-device naming.
- Pair / Connect / Disconnect / Forget.
- Secure BLE connection and handshake.
- Persistent identity information.
- Global Auto Connect.
- Per-device Auto Connect.
- Automatic reconnection.
- BLE identity / connection diagnostics.
- BLE CSV logging.

## Button Actions

Single, Double, and Long press mappings can use:

- None
- NOA Lane Change Cancel
- Acceleration Mode Toggle
- Research Capture A
- Research Capture B
- Research Capture C
- Research Capture D
- Research Capture Reset

## Runtime Bluetooth Master

Bluetooth can now be enabled or disabled without rebooting the T-2CAN controller.

### OFF → ON
- No MCU reboot.
- BLE runtime initializes.
- Eligible saved buttons re-enter the normal Auto-Reconnect path.

### ON → OFF
- No MCU reboot.
- Active scanning stops.
- BLE clients disconnect.
- Pending BLE commands/actions are cleared.
- BLE runtime is reversibly deinitialized.

Saved registry, bonds, mappings, and Auto Connect settings are preserved.

---

# 5. Auto Blinker — REWORKED

Auto Blinker existed in the previous firmware lines, but Universal v3.0 significantly changes routing and authorization.

## Profile-Aware Routing

### Model Y L
Uses the Party CAN + VH CAN topology required by the long-body YL platform.

### Standard Model 3/Y
Uses the CAN A / CAN B topology selected by the active Vehicle Profile.
- Body + Chassis (Advanced-EAP + EU-Unlock) 
- Party + Chassis (Nag-Killer + EU-Unlock) 

## Direction-Specific ALC Gate

Universal v3.0 evaluates `DAS_autoLaneChangeState` from 0x399 before authorizing the delayed turn-signal TX.

- LEFT request requires valid LEFT or BOTH availability.
- RIGHT request requires valid RIGHT or BOTH availability.
- BLOCKED / UNAVAILABLE / mismatched direction prevents TX.
- The direction condition is checked through ARM and again at FIRE.

If the requested lane becomes unavailable during the delay, the pending turn-signal action is cancelled.

## AP State and Freshness Handling

AP mode state and transient lane-change requests are handled separately:

- Valid AP state can remain latched until a CAN recovery boundary.
- Transient lane-change requests still use freshness constraints.

---

# 6. R79 and Summon Architecture — REWORKED

Universal v3.0 separates R79 handling from Summon monitoring.

## Independent R79 Engine

The 0x3FD mux1 R79 engine owns the production R79 policy.

### Fixed Policy

- **bit 19 (`UI_applyEceR79`) → FORCE 0**
- **bit 47 (`UI_hardCoreSummon`) → FORCE 1**
- **bit 18 → STOCK by default, optional LAB override**

## Immediate + Periodic Reassertion

### Immediate
- A real stock mux1 frame is captured.
- The stock frame is used as the template.
- R79 policy is applied.
- Reassertion is attempted immediately when the gate is valid.

### Periodic
- Runs on an independent scheduler.
- Stock RX does not reset the periodic timer.
- Available periods: 20 / 100 / 250 / 500 / 1000 ms.
- A real stock template must exist in the current CAN epoch.

## Fail-Closed Gate

R79 TX is allowed only through the defined AP / Summon / Park gate logic.

Manual driving remains fail-closed.

## Summon Monitor

The previous EU-Unlock / Summon ownership model is replaced by an always-on **Summon Monitor** responsible for:

- Summon state observation.
- Gate telemetry.
- ACA / SPR monitoring.
- Park state.
- Summon TX priority state.
- Queue and transport telemetry.

R79 bit ownership is now handled separately by the R79 engine.

---

# 7. Driving Feature Improvements

## Off-Highway ALC — NEW BETA

Off-Highway ALC is promoted to a normal BETA feature and appears in:

- HOME → Quick Controls.
- SETTINGS → Features.

Behavior:

- **ON:** Off-Highway ALC override enabled.
- **OFF:** STOCK behavior.

---

## TLSSC Highway Block — NEW

Universal v3.0 adds an optional **Highway / Controlled-Access Road Block** for TLSSC.

When enabled, TLSSC can automatically remain **OFF** when the map context indicates a highway or controlled-access road.

The feature uses the available map / road-context signal path and applies hysteresis so short or unstable road-context changes do not continuously toggle TLSSC.

---

## Nag Killer 

Nag Killer remains profile-gated.

Universal v3.0 preserves the existing functionality and adds deeper diagnostics plus **Mode C**, a continuous bounded random-walk mode.

Mode C behavior:

- Approximate target range: +1.50 to +1.80 Nm.
- Maximum step: 0.15 Nm per 200 ms.
- Continuous bounded variation rather than a fixed repeating torque sequence.

---

# 8. TLSSC Restore Safety

Universal v3.0 adds stronger exposure and profile gating through a separate **Banned Car** control.

TLSSC Restore is exposed only when:

1. The selected Vehicle Profile supports it.
2. `Banned Car` is explicitly enabled.
3. The user passes the dedicated warning / confirmation flow.

---

# 9. LAB and CAN Research Capture — MAJOR NEW TOOLING

Universal v3.0 introduces a dedicated LAB area to separate research functions from normal driving controls.

## LAB Research Tools

### R79 Control
- Fixed bit 19 / bit 47 policy telemetry.
- Experimental bit 18 control.
- Immediate / Periodic TX counters.
- Stock and injected raw-frame comparison.
- Refresh-period selection.
- Gate and TX telemetry.

### ULC Blind Spot
Research options:

- STOCK
- STANDARD
- AGGRESSIVE
- MAD MAX

### ACC Follow Distance
New research-only 0x3F8 control:

- STOCK
- Values 1 through 7

## CAN Research Capture

A new RX-focused research recorder supports:

- **SNAPSHOT**
- **RAW TRANSITION**
- **AUTO ALC TRANSITION**

Capabilities:

- CAN A + CAN B observation.
- Labels A / B / C / D.
- User-defined persistent labels.
- PRE and POST windows.
- Multi-segment capture.
- RAW PRE rolling ring / archive.
- Automatic ALC transition qualification.
- CSV download.
- S3XY-triggered Capture A/B/C/D.
- S3XY-triggered Capture Reset.

The capture subsystem is **RX-only** and does not transmit or replay CAN traffic.

---

# 10. Dashboard — REBUILT

The previous scrolling dashboard architecture is reorganized into four persistent mobile sections:

- **HOME**
- **DEVICES**
- **SETTINGS**
- **LAB**

The fixed top connection header and fixed bottom navigation remain available throughout the main interface.

## Firmware Identification

SETTINGS → System identifies the running application as:

**T2CAN Universal v3.0**

The main dashboard branding remains:

**TESLA UNLOCK**

---

# 11. Configurable Wi-Fi

Universal v3.0 adds persistent Wi-Fi AP configuration.  
Default:
- SSID: T2CAN-****
- Password: 12345678
- dashboard: http://192.168.4.1  

Users can change:

- SSID. 
- Password.

Applying a Wi-Fi change restarts only the access point:

- CAN remains running.
- BLE remains running.
- MCU reboot is not required.

---

# 12. Reset Architecture

Universal v3.0 separates reset scope into dedicated operations.

## Reset Firmware Settings

Clears normal firmware settings while preserving:

- Vehicle Profile.
- Wi-Fi configuration.
- S3XY / Bluetooth data.

## Reset Bluetooth Data

Clears:

- S3XY registry.
- Button action mappings.
- BLE bonds.
- BLE discovery / cache state.

Bluetooth Master and Global Auto Connect configuration are preserved.

## Factory Reset — Erase All NVS

Erases all stored configuration and returns T-2CAN to Vehicle Profile Setup Mode.

---

### ⚠️ Important: 120 Ω resistors

Don't forget to remove the two **120-ohm resistors**, as they can cause signal errors.

<img width="407" height="180" alt="LILYGO-T-2CAN_9" src="https://github.com/user-attachments/assets/0d272b7e-bd82-408f-9ca1-239e6dab44d5" />

---


## Source Basis and Scope

This release note was prepared from source-level comparison of:

- **LP_YL V2.0**
- **Advanced EAP & EU-Unlock V2.6.0 for T-2Can**
- **T2CAN Universal v3.0**
