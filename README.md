> ⚠️ **Research / educational firmware only**
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.
>
> It sends signals directly to vehicle controllers; it is **not** a physical steering-wheel command. **Do not use it on public roads or in any situation where unsafe behavior could put people or property at risk.**
>
> You are responsible for your own testing, wiring, configuration, and compliance with local laws.

--- 

# T2CAN Universal Unlock v3.30.3

**Major Universal Release for Lilygo T2CAN**  
**Release date:** 20 11 2026  
**[Dashboard view](https://06066060606060.github.io/t2can-universal-unlock/)**  

**[仪表板模拟](https://06066060606060.github.io/t2can-universal-unlock/index-zh-cn.html)** 

### Discord
Join Discord server for more info:
https://discord.gg/euPbYG8Npc

---
**v3.30.3 **
- Restore TLSSC for Model YL
- Added Chinese Language variant
- Added cached `0x331` replay for TLSSC Restore on CAN B / Chassis profiles to avoid the MCU reboot timing race (ported from P4can PR #15).

**v3.30.2 **	
- Added Model S/X MCU2 HW3 profile (Compatibility with HW2.5) 
- Fix R79 injection after AP disengage. 
- Added experimental FSD Visualisation panel.(This will block AP Activation and cause autopark error!)
- Added Service Mode entry.
- Reorganized settings for a better overview. 
- Auto lane change enable for Standard AP 

**v3.28+ **	
- Continuous AP Experimental    
- Independent ISA chime Suppression 
- Increased OTA receive idle timeout from 5 to 30 seconds.
- Added detailed upload errors and improved retry handling.
- Prevented duplicate uploads and false success messages.
- The new timeout applies after installing and booting this version.  
- Clearer feature names and short descriptions  

**v3.26** 
 - Fixed initial setup recovery, added clear save-error messages, and prevented duplicate saves.
 - Added USB CAN logging and a persistent listen-only mode. (not for everyone)
 - Made ISA Suppression independent of Nag Killer.
 - Improved long diagnostic text layout on mobile screens.

**v3.23**  
 - Improved CAN reliability, including safer BUS-OFF recovery and expanded error diagnostics.  
 - Simplified CAN A reception with immediate single-frame processing.  
 - Removed completed or obsolete LAB tools, including Lane Graph, Parked 0x3FD Injection, and Blind Spot Injection Monitor.  
 - Fixed dashboard API recovery and mobile dropdown touch handling.  
 - Made Disable Driver Monitoring a standalone production feature, independent of Nag Killer.  
 - Promoted Visual Speed Control from LAB to Settings and removed 0x3FD from its name.  
 - Preserved existing AP, CAN health, signal freshness, routing, and transport safety checks.  
 - And a lot more check [Changelog](https://github.com/06066060606060/t2can-universal-unlock/blob/main/CHANGELOG.md)


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

## 🚙 Supported profiles

| Configuration | NAG Killer | Advanced EAP | Pedal Map | EU Unlock |
|---|:---:|:---:|:---:|:---:|
| **Model YL · Party + Body** | ✅ | ✅ | ✅ | ✅ |
| **Standard Model 3/Y · Body + Chassis** | ❌✅ | ✅ | ✅ | ✅ | 
| **Standard Model 3/Y · Party + Chassis** | ✅ | ❌ | ❌ | ✅ |

> Model 3 Highland requires selecting the installed turn-control type during profile setup. Other supported profiles automatically use the Stalk configuration.

### TLSSC Restore anti-restart

For profiles where `0x331` is handled on **CAN B / Chassis**, TLSSC Restore now caches the last observed patched 8-byte frame in NVS (`features/eapFrame`). After CAN B is ready, the cached frame is replayed at 10 Hz for up to 60 seconds or 600 frames, so the MCU can read `SELF_DRIVING` during its own boot instead of waiting for the first stock frame.

- No cached frame means no transmission; the first power cycle still needs to observe a stock `0x331`.
- Fresh stock `0x331` traffic within 2 seconds makes replay yield to the existing read-modify-retransmit path.
- Replay uses the existing TX barrier, epoch, administrative-hold, USB-passive and Summon admission gates.
- Model YL's CAN-A `0x331` route is unchanged and is not covered by this replay fix.
- Cache and replay counters are available under `eap` in `/api/lab/service-mode/stats`.

This reduces the boot-time race but cannot guarantee recovery if the MCU reads its configuration before this controller's CAN B is ready.

## 🔧 Hardware Target

This code is adapted for the **LilyGO / T-2Can** and the **Waveshare ESP32-S3-CAN-2CH**.

| Device | CAN Interface | CAN Bus | Function |
|---|---|---|---| 
| LilyGO / T-2Can | CAN A — MCP2515 | Body CAN | 
| LilyGO / T-2Can | CAN B — TWAI | Chassis CAN | 
| Waveshare ESP32-S3-CAN-2CH | CAN A — XL2515 (`CAN_2` terminal) | Body CAN |
| Waveshare ESP32-S3-CAN-2CH | CAN B — TWAI (`CAN_1` terminal) | Chassis CAN |

### Waveshare ESP32-S3-CAN-2CH

The [Waveshare ESP32-S3-CAN-2CH](https://docs.waveshare.net/ESP32-S3-CAN-2CH/)
(onboard-antenna version; the `-U` external-antenna version is the same board)
is a dual isolated-CAN ESP32-S3 board that maps 1:1 onto the T-2CAN layout:

| Firmware bus | Board terminal | Controller | Pins |
|---|---|---|---|
| CAN A | `CAN_2` | XL2515QF20 (MCP2515-compatible, 16 MHz crystal) | CS=17, SCK=21, MOSI=41, MISO=42 |
| CAN B | `CAN_1` | ESP32-S3 TWAI through the isolator | TX=15, RX=16 |

- The XL2515 has **no reset pin** — every reset (boot, recovery, OTA hold) goes
  over SPI. Its INT line (GPIO 18) is unused; the driver polls.
- ESP32-S3R8: 16 MB flash + 8 MB octal PSRAM, USB-CDC on boot — same 16 MB
  flash / 3 MB app / FAT partition scheme as the T-2CAN build; the Waveshare
  variant selects OPI PSRAM explicitly.
- Pins verified against Waveshare's schematic / BSP
  ([waveshareteam/ESP32-S3-CAN-2CH](https://github.com/waveshareteam/ESP32-S3-CAN-2CH)
  `HARDWARE_REFERENCE.md`).
- Both channels are isolated; mind the **120 Ω termination jumpers** (see the
  hardware note below) and connect the isolated side's ground to the tap.
- **Not yet confirmed on real hardware.** Start in Listen-Only and check that
  both buses receive before enabling injection.

Build it with the variant script (Arduino CLI 1.5.1 + ESP32 core 3.3.12,
`autowp-mcp2515@1.3.1` library):

```powershell
.\Compile-T2CAN-Variants.ps1 -Board waveshare-s3-can-2ch
```

The default `-Board t2can` build is unchanged. Waveshare binaries are named
`*.waveshare-s3-can-2ch.OTA.bin` / `*.waveshare-s3-can-2ch.FULLFLASH.merged.bin`
(plus `.zh-CN` language variants) in `Final-Build`.

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

## ☕ Support the Project

<p align="left">
  <a href="https://tesla-mods.fkey.id/">
    <img height="30" alt="Donate with Crypto" src="https://img.shields.io/badge/Crypto-Donate-F7931A?style=for-the-badge&amp;logo=bitcoin&amp;logoColor=white">
  </a>
  <a href="https://www.paypal.com/donate/?hosted_button_id=Q2YMMMJYTAPJE">
    <img height="30" alt="Donate with PayPal" src="https://img.shields.io/badge/PayPal-Donate-00457C?style=for-the-badge&amp;logo=paypal&amp;logoColor=white">
  </a>
  <a href="https://www.buymeacoffee.com/xbmod">
    <img height="30" alt="Buy Me a Coffee" src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png">
  </a>
</p>

  **各位，开发这个很不容易，费了好大劲，请大家适当赞赏，让我有持续更新的动力**  
<p align="left">
<img width="250" alt="368" src="https://github.com/user-attachments/assets/003791f1-f670-44b9-8728-73e8b48a24a6" />

</p>
