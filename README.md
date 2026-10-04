> ⚠️ **Research / educational firmware only**
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.
>
> It sends signals directly to vehicle controllers; it is **not** a physical steering-wheel command. **Do not use it on public roads or in any situation where unsafe behavior could put people or property at risk.**
>
> You are responsible for your testing, wiring, configuration, and compliance with local laws.

---

# 🚗 T2CAN Universal Unlock v3.7.2

> Universal firmware for supported Tesla Model 3 / Model Y platforms.

| 📅 Release | 👨‍💻 Firmware rewrite | 🌐 Dashboard | 📜 History |
|---|---|---|---|
| 27 September 2026 | LP_YL | [Open dashboard demo](https://06066060606060.github.io/t2can-universal-unlock/) | [CHANGELOG.md](https://github.com/06066060606060/t2can-universal-unlock/blob/pre-release/CHANGELOG.md) |

## ✨ What’s new in v3.7.2

- Profile-based Auto Blinker TX defaults:
  - **Model YL:** Single TX
  - **Other supported Model 3/Y profiles:** legacy 350 ms burst
- New persistent LAB experiment: **Disable Driver Monitoring (NAG)** via `0x3FD` mux1 bit 43.
- The LAB NAG setting is saved in NVS, defaults to OFF, and remains experimental.
- Added R79 LAB telemetry for bit 43: stock value, effective value, and RX change counters.


## 🚀 v3.7 Highlights

| Area | Changes |
|---|---|
| AP / NAG decoding | Corrected `0x399` AP and Hands-On decoding, full AP-state validation, and short-frame rejection |
| Auto Blinker | NOA stabilization (1–20 s, default 10 s), cancel pause (10–100 s, default 20 s), and clearer dashboard states |
| Lane-change cancel | Door-open and S3XY cancel work even when the live `0x24A` context remains `IN_LANE` |
| R79 | Fixed production policy: 2 ms mux1 fast echo plus one +150 ms mux2 refresh |
| Summon | SmartSummonOnly bit 18 moved to **Settings → Summon** (`FORCE 0` by default or `STOCK`) |
| NAG Killer | Mode H Rev.4 production defaults and profiles in **Settings → NAG KILLER** |
| AP Right Scroll | Warning recovery for all supported profiles, with 1–5 s repeat interval (default 2 s) |
| Dashboard | New production controls, persistent migrations, firmware identity updated to v3.7 |
| Nag Killer | New mode tsl9 using chassis/body configuration (doesn't work in all region/country)

## 🚙 Supported profiles

| Configuration | NAG Killer | Advanced EAP | Pedal Map | EU Unlock |
|---|:---:|:---:|:---:|:---:|
| **Model YL · Party + VH** | ✅ | ✅ | ✅ | ✅ |
| **Standard Model 3/Y · Body + Chassis** | ❌✅ | ✅ | ✅ | ✅ |
| **Standard Model 3/Y · Party + Chassis** | ✅ | ❌ | ❌ | ✅ |

> Model 3 Highland requires selecting the installed turn-control type during profile setup. Other supported profiles automatically use the Stalk configuration.

## 🧩 Main features

### 🛡️ Vehicle-profile safety

A saved Vehicle Profile determines the CAN topology, turn-signal route, supported features, and profile-specific restrictions.

On first boot or after a major migration, firmware remains in a fail-closed setup state until a valid profile is saved:

- CAN transmission and injection are disabled.
- CAN recovery supervision is disabled.
- Bluetooth is not started.
- Wi-Fi and the Profile Setup page remain available.

> Migrating from pre-Universal v2.x firmware intentionally erases NVS before creating the Universal bootstrap configuration.

### 🚦 Advanced EAP and Auto Blinker

Advanced EAP can automatically request a turn signal when the vehicle requests a lane change, with configurable delay and retained cancellation options through the screen, door-open action, or S3XY button.

Auto Blinker is profile-aware and validates the requested direction before transmission:

- `DAS_autoLaneChangeState` must permit the requested left or right maneuver.
- The condition is checked when the request is armed and again when it fires.
- A lane becoming unavailable during the delay cancels the pending action.
- NOA must remain eligible through the configured stabilization interval.
- Door-open or S3XY cancellation can apply a shared, configurable pause; triggering cancel again releases it early.

> Direct S3XY left/right blinker actions remain independent from the Auto Blinker enable, stabilization, and pause states.

### 🔁 R79 and Summon

The production R79 path applies a fixed policy to accepted stock frames:

- `UI_applyEceR79` bit 19: forced to `0`.
- `UI_hardCoreSummon` bit 47: forced to `1`.
- SmartSummonOnly bit 18: selectable in **Settings → Summon** as `FORCE 0` or `STOCK`.

Each accepted stock mux1 frame is echoed with a bounded 2 ms queue wait. Each accepted mux2 frame schedules one +150 ms refresh. The LAB page now exposes a read-only R79 status card rather than transport/timing experiments.

A separate always-on Summon Monitor provides state observation, gate telemetry, ACA/SPR monitoring, park-state monitoring, and queue/transport telemetry.

### 💪 NAG Killer and AP Right Scroll

NAG Killer remains profile-gated. Mode H Rev.4 is available as a production profile in **Settings → NAG KILLER** with these defaults:

- Primary: 1.80–2.60 Nm
- Wait: 0.90–3.00 s
- Refractory: 0.50–1.50 s
- Opposite carrier: 0.10–0.60 Nm
- Tiered Hands-On thresholds: 0.40 / 2.00 Nm
- Visual Warning Rescue: ON after 0.5 s
- Hard pause while stopped

AP Right Scroll is available in Settings for Model YL and standard Model 3/Y profiles. A newly detected visual warning triggers an immediate UP/DOWN pair; if the warning stays active, it repeats every 1–5 seconds (default: 2 seconds). Physical right-scroll input always has priority.

### 🔘 S3XY Button support

- Register up to **three S3XY Buttons**.
- Enable or disable Bluetooth Master at runtime without rebooting.
- Saved devices, bonds, button mappings, and Auto Connect settings are retained.
- S3XY actions can control direct blinkers, lane-change cancellation, and CAN research capture.

## 🧪 Dashboard and LAB

The mobile dashboard has four persistent sections:

| Section | Purpose |
|---|---|
| **HOME** | Status and quick controls |
| **DEVICES** | S3XY / Bluetooth devices |
| **SETTINGS** | Supported production features and system configuration |
| **LAB** | Experimental tools and CAN research |

### 🔬 LAB tools

> ⚠️ LAB is intended for controlled research only.

- Read-only R79 status and bit telemetry.
- Experimental **Disable Driver Monitoring (NAG)** control using `0x3FD` mux1 bit 43, where supported.
- ULC Blind Spot research modes: Stock, Standard, Aggressive, and Mad Max.
- Auto Lane Change experiment.
- CAN capture: Snapshot, Raw Transition, and Auto ALC Transition.

> CAN Research Capture is RX-only: it observes and exports CAN traffic but does not replay captured frames.

## 📶 Wi-Fi and resets

### 📡 Wi-Fi access point

Default access point:

| Setting | Default |
|---|---|
| SSID | `T2CAN-****` |
| Password | `12345678` |
| Dashboard | [http://192.168.4.1](http://192.168.4.1) |

SSID and password can be changed from the dashboard. Applying a Wi-Fi change restarts only the access point; CAN, BLE, and the MCU continue running.

### ♻️ Reset options

| Action | What it clears | What it preserves |
|---|---|---|
| Reset Firmware Settings | Normal firmware settings | Vehicle profile, Wi-Fi, and S3XY/Bluetooth data |
| Reset Bluetooth Data | S3XY registry, mappings, bonds, discovery/cache | Bluetooth Master and Global Auto Connect settings |
| Factory Reset | All NVS data | Nothing; returns to Vehicle Profile Setup |

## 🔧 Hardware note

> ⚠️ Remove the two **120 Ω termination resistors** if they are not appropriate for your wiring. Leaving them in place can cause CAN signal errors.

<img width="407" height="180" alt="LILYGO-T-2CAN_9" src="https://github.com/user-attachments/assets/0d272b7e-bd82-408f-9ca1-239e6dab44d5" />

## 📚 Scope

This Universal release is based on source-level comparison and consolidation of:

- LP_YL V2.0
- Advanced EAP & EU-Unlock V2.6.0 for T-2CAN
- T2CAN Universal v3.x
