> ⚠️ **Research / educational firmware only**
>
> This project interacts with a Tesla vehicle CAN bus. It is intended for controlled bench testing, code review, and research environments only.
>
> It sends signals directly to the controller, not a physical command to the steering wheel. **Do not use this on public roads or in any situation where unsafe behavior could put people or property at risk.**
>
> You are responsible for your own testing, wiring, configuration, and compliance with local laws.

--- 

# T2CAN Universal Unlock v3.7  

**Major Universal Release**  
**Release date:** 26 September 2026  
**[Dashboard view](https://06066060606060.github.io/t2can-universal-unlock/)**

## 📋v3.7 changes  
**[More detail in changelog](https://github.com/06066060606060/t2can-universal-unlock/CHANGELOG.md)**

- R79 / Summon rebuilt  
- Improved  Nag Killer Mode H — Human Interaction  
- Auto Blinker improved  
- S3XY Button expanded  
- Lane Change Instant Cancel fixed  
- AP Right Scroll added  
- AP Drive Profile / PedalMap  
- 0x3F8 architecture consolidated  
- CAN runtime hardened  
- Dashboard & mobile UI updated   


## 📋Highlights  

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
- **Off-Highway ALC BETA** added as a normal vehicle feature.
- **TLSSC Highway Block** added: TLSSC can optionally turn **OFF on highways / controlled-access roads**.
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

<img width="270" height="554" alt="Screenshot_2026-09-09-13-56-20-841_com microsoft emmx" src="https://github.com/user-attachments/assets/14d20866-690a-4b62-97b5-031ebcd4b972" /><img width="270" height="494" alt="Screenshot_2026-09-09-13-56-34-391_com microsoft emmx" src="https://github.com/user-attachments/assets/5c513560-1266-4a15-832d-32a417b6c451" />

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
