> ⚠️ **Research / educational firmware only**
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.
>
> It sends signals directly to the controller, not a physical command to the steering wheel. **Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk.**
>
> You are responsible for your own testing, wiring, configuration, and compliance with local laws.

--- 

# TMR Universal Unlock v3.21 Pre release

**Major Universal Release**  
**Release date:** 6 October 2026  
**code entirely rewritten by LP_YL**  

**v3.21 Highlights**
- Added Vision Speed Control — 0x3FD, which requests UI_enableVisionSpeedControl=0.
- Independent Country and Map Region settings
- Improved CAN task diagnostics and Research Capture reliability.
- Fixed several CAN synchronization and transmission safety issues.
- And a lot more check [Changelog](https://github.com/06066060606060/t2can-universal-unlock/blob/pre-release/CHANGELOG.md)

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

# 🚙 Universal Vehicle Architecture

The selected profile determines:

- CAN A / CAN B / CAN C topology.
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
| **Standard 3/Y · Body + Chassis** | ✅ | ✅ | ✅ | ✅ |
| **Standard 3/Y · Party + Chassis** | ✅ | ✅ | ✅ | ✅ |

# 11. Configurable Wi-Fi

Universal v3.0 adds persistent Wi-Fi AP configuration.  
Default:
- SSID: TMR-****
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


This release note was prepared from source-level comparison of:

- **LP_YL V3.21**
- **Advanced EAP & EU-Unlock V2.6.0 for T-2Can**
- **T2CAN Universal v3.0**
