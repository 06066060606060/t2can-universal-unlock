# T2CAN Universal v3.5a1

- R79/Summon transport rollback toward proven Summon-Unlock V2.6 behavior.
- Real 0x3FD mux1 now triggers immediate direct R79 TX when authorized.
- Restored independent 500 ms periodic R79 timer; immediate TX does not reset it.
- Removed active R79 pending/coalescing/retry service.
- Removed destructive SUMMON_FULL TX queue flush/retry.
- Preserved existing model-specific Gear/ACA source routing.
- Added complete dark-mode overrides for the v3.5 mobile dashboard.

# T2CAN Universal Changelog

## v3.5

- Based on the compile-verified v3.4 beta 7 source package.
- Replaced the dashboard presentation layer with the supplied Claude mobile design and subsequent approved mobile refinements.
- Mobile-only portrait layout with the existing HOME / DEVICES / SETTINGS / LAB structure.
- HOME TORQUE metric is centered; Off-Highway ALC was removed from HOME Quick Controls and remains available in Settings.
- Nag Killer Settings now uses compact Mode A–F selectors, a highlighted/recommended Mode H card, and a per-mode behavior summary.
- Legacy Advanced Parameters / torque-table controls remain wired for compatibility but are hidden from the normal dashboard UI.
- Removed the user-facing Custom NAG mode; legacy persisted mode value 2 migrates to Mode A while Mode C–H numeric IDs remain unchanged.
- Banned Car controls use a high-visibility red danger surface.
- Mode H primary-event LAB range is 1.00–3.00 Nm; production default/reset remains 1.50–2.00 Nm.
- Mode H LAB timing display uses the v3.4b7 NORMAL preset values (WAIT 1.2–3.0 s, REFRACTORY 0.8–1.8 s).
- Preserved existing DOM IDs, firmware API endpoints, controls, and backend wiring.
- Added directional page/panel transitions and preserved reduced-motion fade behavior.
- Removed all standalone demo/mock API code before firmware embedding.
- HOME AUTOSTEER / NOA engaged-state title uses the dashboard green token (#10A967); FSD and other state colors are unchanged.
- Mobile fixed header content height reduced from 52 px to 26 px; page/panel offsets follow the same shared header-height variable.
- HOME status tiles reduced to 53 px and Quick Controls to 64 px while preserving their approved typography.
- AUTOPILOT state title is 40 px; AUTOSTEER / NOA retain the engaged green treatment.
- Injection State cards are approximately 10% shorter via spacing/row compaction without reducing their text sizes.
- No GitHub workflow or binary build is included in this source-only package.

### Dashboard dark theme refinement
- Rebuilt dark mode around a neutral charcoal palette for higher contrast and lower visual noise.
- Removed remaining light-theme surfaces from the mobile HOME, bottom navigation, Quick Controls, setup flow, sheets, forms, and status cards.
- Added body-scoped dark color tokens to override the higher-specificity mobile readability skin on iPhone/WebKit.
- Added explicit dark overrides for `#bottomNav` state-class combinations and `#homeQuickStrip` column-state combinations so light borders/backgrounds cannot win the CSS cascade.
- Dashboard-only change; CAN, NAG, Summon/R79, BLE, and vehicle logic are unchanged.
