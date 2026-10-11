# Python Regression Coverage

The immutable baseline is `T2CAN-Universal-v3.28.5-LP_YL`. This package physically combines 103 static/contract files into 17 feature suites, and three Country/Map host files into one suite with three independent compilation methods. Another 31 version-named host or browser-bridge files have feature names. No unique regression was deleted. Sixty-one repeated identity assertions share one dynamic package-identity check; each relocation is recorded below.

Python files decrease from **161 to 74**, including support modules. Test entrypoints decrease from **158 to 70**. `contract_test_support.py` is the only added support module. All pre-existing host fixtures remain separate except the explicitly mapped Country/Map group.

## Execution and isolation

- Each mapped source body is ordinary Python inside a named `unittest.TestCase` method. There is no generated string catalog, `exec`, or `runpy` layer for these suites.
- Source reads, helper definitions, parser classes, and mutable dictionaries are local to each method. Importing a suite does not read firmware files, mutate fixtures, or execute checks.
- The 55 methods in pre-existing unittest classes remain inside their original classes. `run_contract_cases` explicitly loads these classes, requires at least the baseline method count and checks that every currently discovered method executes, propagates failures/errors, and rejects skipped/expected failures.
- The separate NAG host fixture retains its existing source-snippet loader; its support filename and profile-PREAMBLE imports were updated to the new feature filenames. The consolidation adds no such mechanism.
- Version identity is checked once by `PackageIdentityContracts.test_package_version_identity`: numeric package directory, exactly one matching sketch, a unique FW_VERSION define, and matching sketch/CHANGELOG/VALIDATION identities. Three identity-only duplicate methods were removed; feature-specific checks remain in their own methods.

## Preservation evidence

- Before consolidation, all 103 selected baseline static/contract files passed (102 execution units under the existing runner), and the three baseline Country/Map host files passed.
- The 106 combined baseline sources contain **1,317 `assert` statements** and **246 unittest assertion/fail calls**. After explicitly centralizing 61 identity assertions, **1,256 original assert statements and all 246 unittest assertion calls remain**. Eight central identity assertions cover the relocated predicates. All Country/Map C++ fixture literals remain intact.
- Before identity deduplication, all **106 original source bodies** had normalized assertion-expression AST multisets matching their destination methods exactly. Normalization covers only the approved header/file relocations and current version identity.
- Before identity deduplication, **105 of 106 bodies** also had complete normalized statement ASTs matching exactly after moving the main-guard body into its callable and explicitly loading old unittest classes. This verifies fixture data and multiline C++ strings, not merely assertion counts.
- The one intentional structural exception is the old compatibility-cleanup test: two ULC owners now have the same filename, so its dictionary became a list of `(owner, checks)` pairs to prevent duplicate-key loss. Its self-exclusion now removes only its own callable source rather than excluding an entire consolidated suite. All seven original assertion expressions were retained before identity deduplication; its version assertion is now covered by the central owner.
- The manifest guard in `test_package_hygiene_contracts.py` verifies mapped callables and nested-class execution wiring. Assertion counts below are migration evidence, not frozen limits: future assertion additions and new methods are allowed. This guard is additional coverage beyond the baseline checks.
- AST assertion counts are source-level counts, not the number of loop iterations or runtime scenarios. Each Country/Map C++ compilation and each existing unittest method still runs independently.

## Consolidated callable mapping

<!-- PYTHON-MAPPING-START -->
| Original file | Destination file | Callable | Baseline assert | Retained assert | Centralized identity | Unittest assertion calls | Retained unittest methods |
|---|---|---|---:|---:|---:|---:|---:|
| `test_compile_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_compile` | 8 | 8 | 0 | 0 | 0 |
| `test_v36b17_version_static.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_current_version_and_preserved_features` | 8 | 5 | 3 | 0 | 0 |
| `test_v371_version_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_package_version_identity` | 4 | 0 | 4 | 0 | 0 |
| `test_v37_version_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_current_version_and_release_files` | 7 | 6 | 1 | 0 | 0 |
| `test_v38_release_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_release` | 24 | 20 | 4 | 0 | 0 |
| `test_v3913_package_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_package_version_identity` | 6 | 0 | 6 | 0 | 0 |
| `test_v391_package_contract.py` | `test_package_identity_contracts.py` | `PackageIdentityContracts.test_package_version_identity` | 6 | 0 | 6 | 0 | 0 |
| `test_modes_removed.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_modes_removed` | 10 | 10 | 0 | 0 | 0 |
| `test_v36b2_cleanup_static.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_removed_nag_controls` | 7 | 7 | 0 | 0 | 0 |
| `test_v36b5_cleanup_static.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_removed_dashboard_compatibility` | 14 | 13 | 1 | 0 | 0 |
| `test_v36c3_cleanup_static.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_removed_compatibility_and_archives` | 7 | 6 | 1 | 0 | 0 |
| `test_v373_cleanup_contract.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_removed_experimental_modes` | 12 | 12 | 0 | 0 | 0 |
| `test_can_shared_ownership_contract.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_can_shared_ownership` | 0 | 0 | 0 | 5 | 1 |
| `test_retired_controls_runtime_contract.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_retired_controls_runtime` | 9 | 9 | 0 | 0 | 0 |
| `test_v36d7_contract_static.py` | `test_package_hygiene_contracts.py` | `PackageHygieneContracts.test_fixed_r79_heartbeat_viewport` | 10 | 9 | 1 | 0 | 0 |
| `test_dashboard_compact_profile_static.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_dashboard_compact_profile` | 10 | 10 | 0 | 0 | 0 |
| `test_dashboard_feature_promotion_static.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_dashboard_feature_promotion` | 4 | 4 | 0 | 0 | 0 |
| `test_dashboard_profile_visibility_regression.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_dashboard_profile_visibility` | 12 | 12 | 0 | 0 | 0 |
| `test_dashboard_v3130_promotion.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_production_nag_dashboard_promotion` | 6 | 6 | 0 | 0 | 0 |
| `test_dashboard_v3220_removed_lab_static.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_removed_lab` | 6 | 6 | 0 | 0 | 0 |
| `test_party_chassis_tsl9_hidden.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_party_chassis_tsl9_hidden` | 7 | 7 | 0 | 0 | 0 |
| `test_v382_body_tsl9_ui_regression.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_body_tsl9_ui` | 14 | 14 | 0 | 0 | 0 |
| `test_v382_nag_right_scroll_relocation.py` | `test_dashboard_profiles_contracts.py` | `DashboardProfilesContracts.test_nag_right_scroll_relocation` | 8 | 8 | 0 | 0 | 0 |
| `test_home_screen_icon.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_home_screen_icon` | 0 | 0 | 0 | 11 | 2 |
| `test_v382_mobile_select_guard.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_mobile_select_guard` | 0 | 0 | 0 | 8 | 1 |
| `test_v38_demo_layout_parity.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_demo_layout_parity` | 38 | 38 | 0 | 0 | 0 |
| `test_v38_lab_reference_parity.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_lab_reference_parity` | 25 | 25 | 0 | 0 | 0 |
| `test_v37_dashboard_contract.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_production_controls_and_fixed_status` | 0 | 0 | 0 | 12 | 6 |
| `test_v390_dashboard_contract.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_dashboard_owner_visibility` | 0 | 0 | 0 | 27 | 5 |
| `test_browser_harness_contract.py` | `test_dashboard_layout_contracts.py` | `DashboardLayoutContracts.test_browser_harness` | 11 | 11 | 0 | 0 | 0 |
| `test_blinker_tx_policy_static.py` | `test_blinker_contracts.py` | `BlinkerContracts.test_blinker_tx_policy` | 11 | 11 | 0 | 0 | 0 |
| `test_blinker_tx_production_contract.py` | `test_blinker_contracts.py` | `BlinkerContracts.test_blinker_tx_production` | 24 | 24 | 0 | 0 | 0 |
| `test_v37_auto_blinker_timers_static.py` | `test_blinker_contracts.py` | `BlinkerContracts.test_auto_blinker_timers` | 19 | 19 | 0 | 0 | 0 |
| `test_v37_cancel_behavior_independent_static.py` | `test_blinker_contracts.py` | `BlinkerContracts.test_cancel_behavior_independent` | 7 | 7 | 0 | 0 | 0 |
| `test_v36b3_features_static.py` | `test_blinker_contracts.py` | `BlinkerContracts.test_blinker_behavior_and_feature_controls` | 16 | 15 | 1 | 0 | 0 |
| `test_ulc_single_sender_contract.py` | `test_ulc_contracts.py` | `UlcContracts.test_ulc_single_sender` | 4 | 4 | 0 | 0 | 0 |
| `test_v36b10_autolc_static.py` | `test_ulc_contracts.py` | `UlcContracts.test_autolc` | 22 | 21 | 1 | 0 | 0 |
| `test_v36b11_ulc_policy_static.py` | `test_ulc_contracts.py` | `UlcContracts.test_ulc_policy` | 13 | 13 | 0 | 0 | 0 |
| `test_v36b17_ulc_stalk_confirm_timing_static.py` | `test_ulc_contracts.py` | `UlcContracts.test_ulc_stalk_confirm_timing` | 13 | 12 | 1 | 0 | 0 |
| `test_v36c1_ulc_confirm_capture_static.py` | `test_ulc_contracts.py` | `UlcContracts.test_ulc_confirm_capture` | 15 | 14 | 1 | 0 | 0 |
| `test_v36b22_autolc_bus_target_static.py` | `test_ulc_contracts.py` | `UlcContracts.test_autolc_bus_target` | 14 | 13 | 1 | 0 | 0 |
| `test_mode_h_module_contract.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_mode_h_module` | 12 | 12 | 0 | 0 | 0 |
| `test_v313_nag_default_off.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_nag_default_off` | 0 | 0 | 0 | 11 | 5 |
| `test_v36b20_contract_static.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_single_mode_h_tuning_and_retired_variants` | 18 | 17 | 1 | 0 | 0 |
| `test_v36c2_contract_static.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_mode_h_hands_on_and_fixed_r79` | 17 | 16 | 1 | 0 | 0 |
| `test_v36d4_defaults_static.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_defaults` | 10 | 9 | 1 | 0 | 0 |
| `test_v36d5_mode_h_stop_carrier_static.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_mode_h_stop_carrier` | 16 | 15 | 1 | 0 | 0 |
| `test_v36e2_mode_h_dashboard_static.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_mode_h_dashboard` | 13 | 13 | 0 | 0 | 0 |
| `test_v36f2_mode_h_visual_rescue_contract.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_mode_h_visual_rescue` | 14 | 14 | 0 | 0 | 0 |
| `test_v36f4_rev4_defaults_ho_contract.py` | `test_mode_h_contracts.py` | `ModeHContracts.test_rev4_defaults_ho` | 15 | 15 | 0 | 0 | 0 |
| `test_legacy_y_body_scroll_runtime.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_legacy_y_body_scroll_runtime` | 11 | 11 | 0 | 0 | 0 |
| `test_tsl9_legacy_body_runtime_static.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_legacy_body_runtime` | 28 | 28 | 0 | 0 | 0 |
| `test_tsl9_runtime_static.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_runtime` | 47 | 47 | 0 | 0 | 0 |
| `test_tsl9_scroll_assist_static.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_scroll_assist` | 0 | 0 | 0 | 22 | 3 |
| `test_v36f1_scroll_burst_static.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_scroll_burst` | 17 | 16 | 1 | 0 | 0 |
| `test_v37_ap_right_scroll_routes_static.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_ap_right_scroll_routes` | 0 | 0 | 0 | 11 | 5 |
| `test_v390_tsl9_input_runtime_contract.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_input_runtime` | 0 | 0 | 0 | 30 | 5 |
| `test_v390_tsl9_isa_runtime_contract.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_isa_runtime` | 1 | 1 | 0 | 16 | 3 |
| `test_v390_tsl9_legacy_route_contract.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_tsl9_legacy_route` | 0 | 0 | 0 | 13 | 3 |
| `test_v392_nag_right_scroll_contract.py` | `test_nag_tsl9_contracts.py` | `NagTsl9Contracts.test_nag_right_scroll` | 0 | 0 | 0 | 19 | 3 |
| `test_r79_dashboard_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_dashboard` | 8 | 8 | 0 | 0 | 0 |
| `test_r79_mode2_runtime_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_mode2_runtime` | 31 | 31 | 0 | 0 | 0 |
| `test_r79_runtime_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_runtime` | 29 | 29 | 0 | 0 | 0 |
| `test_v36b12_static.py` | `test_r79_contracts.py` | `R79Contracts.test_fixed_ulc_route_and_driver_monitor` | 14 | 14 | 0 | 0 | 0 |
| `test_v36d2_r79_fast_path_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_fast_path` | 21 | 20 | 1 | 0 | 0 |
| `test_v36d3_followup_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_followup_and_can_read_budget` | 28 | 27 | 1 | 0 | 0 |
| `test_v36d9a2_post_mux1_mode_static.py` | `test_r79_contracts.py` | `R79Contracts.test_post_mux1_mode` | 12 | 11 | 1 | 0 | 0 |
| `test_v37_r79_fixed_runtime_static.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_fixed_runtime` | 33 | 33 | 0 | 0 | 0 |
| `test_v37_r79_retired_absent.py` | `test_r79_contracts.py` | `R79Contracts.test_r79_retired_absent` | 2 | 2 | 0 | 0 | 0 |
| `test_v36b9_driver_monitor_static.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_driver_monitor` | 19 | 17 | 2 | 0 | 0 |
| `test_v36b21_driver_monitor_all_profiles_static.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_driver_monitor_all_profiles` | 15 | 14 | 1 | 0 | 0 |
| `test_v372_dms_nag_lab_static.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_dms_nag_lab` | 18 | 17 | 1 | 0 | 0 |
| `test_v372_dms_nag_profiles_static.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_dms_nag_profiles` | 11 | 11 | 0 | 0 | 0 |
| `test_v390_r79_dms_ownership_contract.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_r79_dms_ownership` | 1 | 1 | 0 | 19 | 4 |
| `test_v3913_driver_window_runtime_contract.py` | `test_driver_monitor_contracts.py` | `DriverMonitorContracts.test_driver_window_runtime` | 32 | 32 | 0 | 0 | 0 |
| `test_v36b4_pedalmap_static.py` | `test_drive_profile_contracts.py` | `DriveProfileContracts.test_pedalmap` | 13 | 12 | 1 | 0 | 0 |
| `test_v36b7_drive_profile_static.py` | `test_drive_profile_contracts.py` | `DriveProfileContracts.test_drive_profile` | 25 | 23 | 2 | 0 | 0 |
| `test_v36b19_ap_drive_dashboard_scope.py` | `test_drive_profile_contracts.py` | `DriveProfileContracts.test_ap_drive_dashboard_scope` | 6 | 6 | 0 | 0 | 0 |
| `test_v36b8_features_static.py` | `test_drive_profile_contracts.py` | `DriveProfileContracts.test_drive_and_lane_feature_controls` | 30 | 28 | 2 | 0 | 0 |
| `test_v392_ap_aggressiveness_removed_static.py` | `test_drive_profile_contracts.py` | `DriveProfileContracts.test_ap_aggressiveness_removed` | 6 | 4 | 2 | 0 | 0 |
| `test_can_busoff_persistence_contract.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_can_busoff_persistence` | 11 | 11 | 0 | 0 | 0 |
| `test_can_usb_logger_contract.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_can_usb_logger` | 6 | 6 | 0 | 0 | 0 |
| `test_dashboard_diagnostics_visibility_static.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_dashboard_diagnostics_visibility` | 10 | 10 | 0 | 0 | 0 |
| `test_v36b6_diagnostics_static.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_diagnostics` | 34 | 32 | 2 | 0 | 0 |
| `test_v36d9a1_can_traffic_snapshot_static.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_can_traffic_snapshot` | 5 | 4 | 1 | 0 | 0 |
| `test_v3911_diagnostic_contract.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_diagnostic` | 13 | 11 | 2 | 0 | 0 |
| `test_v3211_twai_alert_diagnostics_static.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_twai_alert_diagnostics` | 18 | 18 | 0 | 0 | 0 |
| `test_v3914_research_capture_mode_contract.py` | `test_diagnostics_contracts.py` | `DiagnosticsContracts.test_research_capture_mode` | 12 | 11 | 1 | 0 | 0 |
| `test_v37_schema3_migration_static.py` | `test_settings_nvs_contracts.py` | `SettingsNvsContracts.test_schema3_migration` | 0 | 0 | 0 | 18 | 5 |
| `test_v390_nvs_keep_ble_reset_contract.py` | `test_settings_nvs_contracts.py` | `SettingsNvsContracts.test_nvs_keep_ble_reset` | 0 | 0 | 0 | 24 | 4 |
| `test_feature_domain_contract.py` | `test_settings_nvs_contracts.py` | `SettingsNvsContracts.test_feature_domain` | 12 | 12 | 0 | 0 | 0 |
| `test_promoted_feature_api_contract.py` | `test_settings_nvs_contracts.py` | `SettingsNvsContracts.test_promoted_feature_api` | 15 | 15 | 0 | 0 | 0 |
| `test_v36c6_binary_size_static.py` | `test_web_serialization_contracts.py` | `WebSerializationContracts.test_binary_size` | 21 | 20 | 1 | 0 | 0 |
| `test_v36c7_api_key_contract.py` | `test_web_serialization_contracts.py` | `WebSerializationContracts.test_api_key` | 5 | 5 | 0 | 0 | 0 |
| `test_v36c7_json_writer_static.py` | `test_web_serialization_contracts.py` | `WebSerializationContracts.test_json_writer` | 11 | 10 | 1 | 0 | 0 |
| `test_v36c7_snapshot_table_static.py` | `test_web_serialization_contracts.py` | `WebSerializationContracts.test_snapshot_table` | 4 | 4 | 0 | 0 | 0 |
| `test_s3xy_stock_synced_stalk_static.py` | `test_ble_contracts.py` | `BleContracts.test_s3xy_stock_synced_stalk` | 13 | 13 | 0 | 0 | 0 |
| `test_v36e2_s3xy_direct_blinker_static.py` | `test_ble_contracts.py` | `BleContracts.test_s3xy_direct_blinker` | 15 | 15 | 0 | 0 | 0 |
| `test_v37_s3xy_action_ids_static.py` | `test_ble_contracts.py` | `BleContracts.test_s3xy_action_ids` | 4 | 4 | 0 | 0 | 0 |
| `test_v36d9a6_s3xy_blinker_disabled_ui_static.py` | `test_ble_contracts.py` | `BleContracts.test_s3xy_blinker_disabled_ui` | 18 | 17 | 1 | 0 | 0 |
| `test_country_override_runtime_contract.py` | `test_country_contracts.py` | `CountryContracts.test_country_override_runtime` | 60 | 60 | 0 | 0 | 0 |
| `test_v38_ap_display_hold_static.py` | `test_ap_session_contracts.py` | `ApSessionContracts.test_ap_display_hold` | 16 | 16 | 0 | 0 | 0 |
| `test_v36b14_static.py` | `test_ap_session_contracts.py` | `ApSessionContracts.test_fixed_confirm_free_route` | 9 | 8 | 1 | 0 | 0 |
| `test_country_override_runtime_host.py` | `test_country_override_host.py` | `CountryOverrideHost.test_country_guarded_transport` | 0 | 0 | 0 | 0 | 0 |
| `test_v315_country_map_runtime_host.py` | `test_country_override_host.py` | `CountryOverrideHost.test_independent_country_map_runtime` | 0 | 0 | 0 | 0 | 0 |
| `test_country_override_nz_api_host.py` | `test_country_override_host.py` | `CountryOverrideHost.test_country_map_api_persistence` | 1 | 1 | 0 | 0 | 0 |
<!-- PYTHON-MAPPING-END -->

## Feature filename renames

These files keep their existing execution boundary and assertion bodies. Only approved production include paths and internal test imports/filenames change.

| Original file | Destination file |
|---|---|
| `test_v313_nag_api_host.py` | `test_nag_api_host.py` |
| `test_v313_nag_persistence_host.py` | `test_nag_legacy_persistence_host.py` |
| `test_v313_torque_ap_gate_host.py` | `test_nag_torque_ap_gate_host.py` |
| `test_v319_vision_control_api_host.py` | `test_vision_control_api_host.py` |
| `test_v320_retired_speed_routes_host.py` | `test_retired_speed_routes_host.py` |
| `test_v323_driver_monitoring_persistence_host.py` | `test_driver_monitoring_persistence_host.py` |
| `test_v325_summon_heartbeat_api_host.py` | `test_summon_heartbeat_api_host.py` |
| `test_v325_summon_heartbeat_session_host.py` | `test_summon_heartbeat_session_host.py` |
| `test_v326_isa_persistence_host.py` | `test_isa_persistence_host.py` |
| `test_v327_isa_runtime_host.py` | `test_isa_runtime_host.py` |
| `test_v3281_continuous_ap_diagnostics_host.py` | `test_continuous_ap_diagnostics_host.py` |
| `test_v3281_continuous_ap_runtime_host.py` | `test_continuous_ap_lane_change_host.py` |
| `test_v3282_feature_nvs_host.py` | `test_feature_nvs_host.py` |
| `test_v3282_lane_settings_api_host.py` | `test_lane_settings_api_host.py` |
| `test_v3282_nag_nvs_host.py` | `test_nag_nvs_host.py` |
| `test_v3282_profile_nvs_host.py` | `test_profile_nvs_host.py` |
| `test_v3282_reset_guard_host.py` | `test_reset_guard_host.py` |
| `test_v3282_scroll_restore_host.py` | `test_nag_scroll_restore_host.py` |
| `test_v3282_settings_api_host.py` | `test_settings_api_host.py` |
| `test_v3283_ota_receive_host.py` | `test_ota_receive_host.py` |
| `test_v3285_ble_ownership_host.py` | `test_ble_ownership_host.py` |
| `test_v328_continuous_ap_api_host.py` | `test_continuous_ap_api_host.py` |
| `test_v328_continuous_ap_config_host.py` | `test_continuous_ap_config_host.py` |
| `test_v328_continuous_ap_nag_host.py` | `test_continuous_ap_nag_host.py` |
| `test_v328_continuous_ap_review_host.py` | `test_continuous_ap_admission_races_host.py` |
| `test_v328_continuous_ap_runtime_host.py` | `test_continuous_ap_runtime_host.py` |
| `test_v328_continuous_ap_tx_host.py` | `test_continuous_ap_tx_host.py` |
| `test_v328_host_fixture_setup.py` | `test_continuous_ap_fixture_setup_host.py` |
| `test_v36c7_json_writer_host.py` | `test_json_writer_host.py` |
| `test_v38_preview_home_simulator.py` | `test_preview_home_simulator.py` |
| `test_v38_theme_mode.py` | `test_theme_mode.py` |

## Centralized identity assertions

Each removed assertion below is owned by `PackageIdentityContracts.test_package_version_identity`. Equivalent source variables were resolved to the same package sketch or document before predicate deduplication. The central checks are at least as strong: exactly one matching define/sketch also implies every former containment/existence check.

| Former callable | Removed assertion | Central rule |
|---|---|---|
| `DiagnosticsContracts.test_diagnostics` | `assert len(ino_files) == 1` | single matching sketch |
| `DiagnosticsContracts.test_diagnostics` | `assert '#define FW_VERSION "' in ino` | FW_VERSION |
| `DiagnosticsContracts.test_can_traffic_snapshot` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DiagnosticsContracts.test_diagnostic` | `assert INO.startswith('// T2CAN Universal v3.28.6')` | sketch comment identity |
| `DiagnosticsContracts.test_diagnostic` | `assert '#define FW_VERSION "v3.28.6"' in INO` | FW_VERSION |
| `DiagnosticsContracts.test_research_capture_mode` | `assert '#define FW_VERSION "v3.28.6"' in INO` | FW_VERSION |
| `PackageIdentityContracts.test_current_version_and_preserved_features` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `PackageIdentityContracts.test_current_version_and_preserved_features` | `assert changelog.startswith('# T2CAN Universal v3.28.6\n')` | CHANGELOG title |
| `PackageIdentityContracts.test_current_version_and_preserved_features` | `assert validation.startswith('# T2CAN Universal v3.28.6 — Validation\n')` | VALIDATION title |
| `PackageIdentityContracts.test_current_version_and_das_decoder` | `assert '#define FW_VERSION "v3.28.6"' in INO` | FW_VERSION |
| `PackageIdentityContracts.test_current_version_and_das_decoder` | `assert INO.startswith('// T2CAN Universal v3.28.6')` | sketch comment identity |
| `PackageIdentityContracts.test_current_version_and_das_decoder` | `assert CHANGELOG.startswith('# T2CAN Universal v3.28.6\n')` | CHANGELOG title |
| `PackageIdentityContracts.test_current_version_and_das_decoder` | `assert VALIDATION.startswith('# T2CAN Universal v3.28.6 — Validation\n')` | VALIDATION title |
| `PackageIdentityContracts.test_current_version_and_release_files` | `assert '#define FW_VERSION "v3.28.6"' in INO` | FW_VERSION |
| `PackageIdentityContracts.test_release` | `assert INO.startswith('// T2CAN Universal v3.28.6')` | sketch comment identity |
| `PackageIdentityContracts.test_release` | `assert '#define FW_VERSION "v3.28.6"' in INO` | FW_VERSION |
| `PackageIdentityContracts.test_release` | `assert CHANGELOG.startswith('# T2CAN Universal v3.28.6\n')` | CHANGELOG title |
| `PackageIdentityContracts.test_release` | `assert VALIDATION.startswith('# T2CAN Universal v3.28.6 — Validation\n')` | VALIDATION title |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert ROOT.name == 'T2CAN-Universal-v3.28.6-LP_YL'` | numeric package directory identity |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert INO.is_file()` | single matching sketch |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert text.startswith('// T2CAN Universal v3.28.6')` | sketch comment identity |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert '#define FW_VERSION "v3.28.6"' in text` | FW_VERSION |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert (ROOT / 'CHANGELOG.md').read_text().startswith('# T2CAN Universal v3.28.6\n')` | CHANGELOG title |
| `PackageIdentityContracts.test_current_package_driver_window_identity` | `assert (ROOT / 'VALIDATION.md').read_text().startswith('# T2CAN Universal v3.28.6 — Validation\n')` | VALIDATION title |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert PKG.is_dir(), 'v3.28.6 package must exist'` | numeric package directory identity |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert ino.is_file(), 'v3.28.6 sketch name must match its directory'` | single matching sketch |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert text.startswith('// T2CAN Universal v3.28.6')` | sketch comment identity |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert '#define FW_VERSION "v3.28.6"' in text` | FW_VERSION |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert (PKG / 'CHANGELOG.md').read_text().startswith('# T2CAN Universal v3.28.6\n')` | CHANGELOG title |
| `PackageIdentityContracts.test_current_package_diagnostic_identity` | `assert (PKG / 'VALIDATION.md').read_text().startswith('# T2CAN Universal v3.28.6 — Validation\n')` | VALIDATION title |
| `DriveProfileContracts.test_pedalmap` | `assert len(ino_files) == 1` | single matching sketch |
| `DriveProfileContracts.test_drive_profile` | `assert len(ino_files) == 1` | single matching sketch |
| `DriveProfileContracts.test_drive_profile` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DriveProfileContracts.test_drive_and_lane_feature_controls` | `assert len(ino_files) == 1` | single matching sketch |
| `DriveProfileContracts.test_drive_and_lane_feature_controls` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DriveProfileContracts.test_ap_aggressiveness_removed` | `assert len(ino_files) == 1` | single matching sketch |
| `DriveProfileContracts.test_ap_aggressiveness_removed` | `assert '#define FW_VERSION "v3.28.6"' in production` | FW_VERSION |
| `R79Contracts.test_r79_fast_path` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `R79Contracts.test_r79_followup_and_can_read_budget` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `R79Contracts.test_post_mux1_mode` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `ApSessionContracts.test_fixed_confirm_free_route` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DriverMonitorContracts.test_driver_monitor` | `assert len(ino_files) == 1` | single matching sketch |
| `DriverMonitorContracts.test_driver_monitor` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DriverMonitorContracts.test_driver_monitor_all_profiles` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `DriverMonitorContracts.test_dms_nag_lab` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `ModeHContracts.test_single_mode_h_tuning_and_retired_variants` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `ModeHContracts.test_mode_h_hands_on_and_fixed_r79` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `ModeHContracts.test_defaults` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `ModeHContracts.test_mode_h_stop_carrier` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `UlcContracts.test_autolc` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `UlcContracts.test_ulc_stalk_confirm_timing` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `UlcContracts.test_ulc_confirm_capture` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `UlcContracts.test_autolc_bus_target` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `PackageHygieneContracts.test_removed_dashboard_compatibility` | `assert len(ino_files) == 1` | single matching sketch |
| `PackageHygieneContracts.test_removed_compatibility_and_archives` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `PackageHygieneContracts.test_fixed_r79_heartbeat_viewport` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `BlinkerContracts.test_blinker_behavior_and_feature_controls` | `assert len(ino_files) == 1` | single matching sketch |
| `WebSerializationContracts.test_binary_size` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `WebSerializationContracts.test_json_writer` | `assert '#define FW_VERSION "v3.28.6"' in text, 'firmware version must be v3.28.6'` | FW_VERSION |
| `NagTsl9Contracts.test_scroll_burst` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |
| `BleContracts.test_s3xy_blinker_disabled_ui` | `assert '#define FW_VERSION "v3.28.6"' in ino` | FW_VERSION |

## Header-owner relocation

Python source readers, static include checks, and embedded C++ host includes follow these production owners. Separate concerns remain separate test callables.

| Former header | Current owner |
|---|---|
| `continuous_ap_types.h` | `continuous_ap_fsm_pure.h` |
| `continuous_ap_frames_pure.h` | `continuous_ap_fsm_pure.h` |
| `continuous_ap_policy_pure.h` | `continuous_ap_fsm_pure.h` |
| `stalk_frame_pure.h` | `auto_blinker_pure.h` |
| `blinker_tx_policy_pure.h` | `auto_blinker_pure.h` |
| `ulc_policy_pure.h` | `ulc_compositor_pure.h` |
| `ulc_stalk_confirm_pure.h` | `ulc_compositor_pure.h` |
| `r79_mode2_pure.h` | `r79_fixed_policy_pure.h` |
| `r79_ap_gate_pure.h` | `r79_fixed_policy_pure.h` |
| `fixed_point_arduino.h` | `text_format_arduino.h` |
| `ap_drive_profile_pure.h` | `pedal_map_session_pure.h` |
| `can_a_rx_mode_pure.h` | `runtime_gate_pure.h` |
| `ap_right_scroll_pure.h` | `tsl9_input_scheduler_pure.h` |

The retired scroll engine is preserved by the C++ legacy-scroll oracle suite; current interval constants/functions live in `tsl9_input_scheduler_pure.h`. Mode H immutable oracle fixtures and their hashes are unchanged.

## Verification status

Focused baseline static contracts: 103 original files / 102 execution units passed. Baseline Country/Map host: 3/3 files passed. After consolidation and header/filename relocation, all 48 host-Python files passed. After final identity centralization and shared COVERAGE.md integration, all 17 feature suites passed (report: tesla-unlock-host-yqhiwpe2.json). The final retained assertions plus the 61-entry centralization ledger reproduce all 106 original assertion-expression AST multisets exactly after documented path relocation. All 18 consolidated suites have no source reads or mutable fixture state at import time. Full completion, ESP32-S3 compilation, and artifact generation are owned by the root workflow. No hardware access is performed by these tests.


---

# C++ regression coverage mapping

Every original assertion is retained in its named case, with helper/global names isolated by namespace. Each case is called by the suite main. Mode H oracle fixtures remain unchanged.

| Original file | Suite | Callable | Assertion sites |
|---|---|---|---:|
| test_ap_drive_profile_pure.cpp | test_powertrain_suite.cpp | case_ap_drive_profile_pure::run | 15 |
| test_pedal_map_session.cpp | test_powertrain_suite.cpp | case_pedal_map_session::run | 20 |
| test_blinker_tx_policy_pure.cpp | test_blinker_suite.cpp | case_blinker_tx_policy_pure::run | 35 |
| test_stalk_frame_pure.cpp | test_blinker_suite.cpp | case_stalk_frame_pure::run | 8 |
| test_v37_auto_blinker_timers_pure.cpp | test_blinker_suite.cpp | case_auto_blinker_timers_pure::run | 54 |
| test_ulc_compositor_pure.cpp | test_ulc_suite.cpp | case_ulc_compositor_pure::run | 14 |
| test_ulc_policy_pure.cpp | test_ulc_suite.cpp | case_ulc_policy_pure::run | 8 |
| test_ulc_stalk_confirm_pure.cpp | test_ulc_suite.cpp | case_ulc_stalk_confirm_pure::run | 10 |
| test_v36b17_ulc_stalk_confirm_timing_pure.cpp | test_ulc_suite.cpp | case_ulc_stalk_confirm_timing_pure::run | 13 |
| test_auto_lane_change_enable_pure.cpp | test_auto_lane_change_suite.cpp | case_auto_lane_change_enable_pure::run | 15 |
| test_v36b22_autolc_bus_target_pure.cpp | test_auto_lane_change_suite.cpp | case_autolc_bus_target_pure::run | 11 |
| test_r79_ap_gate_pure.cpp | test_r79_suite.cpp | case_r79_ap_gate_pure::run | 25 |
| test_r79_hw3_policy.cpp | test_r79_suite.cpp | case_r79_hw3_policy::run | 7 |
| test_r79_manual_drive_pure.cpp | test_r79_suite.cpp | case_r79_manual_drive_pure::run | 10 |
| test_r79_mode2_pure.cpp | test_r79_suite.cpp | case_r79_mode2_pure::run | 26 |
| test_r79_policy.cpp | test_r79_suite.cpp | case_r79_policy::run | 18 |
| test_v37_r79_fixed_policy_pure.cpp | test_r79_suite.cpp | case_r79_fixed_policy_pure::run | 69 |
| test_v37_r79_safety_pure.cpp | test_r79_suite.cpp | case_r79_safety_pure::run | 17 |
| test_v326_isa_standalone_pure.cpp | test_isa_suite.cpp | case_isa_standalone_pure::run | 13 |
| test_v327_isa_independent_pure.cpp | test_isa_suite.cpp | case_isa_independent_pure::run | 13 |
| test_v390_tsl9_isa_transform_pure.cpp | test_isa_suite.cpp | case_tsl9_isa_transform_pure::run | 17 |
| test_v328_continuous_ap_frames.cpp | test_continuous_ap_suite.cpp | case_continuous_ap_frames::run | 26 |
| test_v328_continuous_ap_fsm.cpp | test_continuous_ap_suite.cpp | case_continuous_ap_fsm::run | 51 |
| test_v328_continuous_ap_policy.cpp | test_continuous_ap_suite.cpp | case_continuous_ap_policy::run | 8 |
| test_country_override_pure.cpp | test_country_suite.cpp | case_country_override_pure::run | 46 |
| test_v315_country_map_pure.cpp | test_country_suite.cpp | case_country_map_pure::run | 11 |
| test_legacy_y_body_scroll_route.cpp | test_tsl9_routes_suite.cpp | case_legacy_y_body_scroll_route::run | 3 |
| test_tsl9_legacy_body_route_pure.cpp | test_tsl9_routes_suite.cpp | case_tsl9_legacy_body_route_pure::run | 13 |
| test_tsl9_profile_capability_pure.cpp | test_tsl9_routes_suite.cpp | case_tsl9_profile_capability_pure::run | 9 |
| test_v390_tsl9_legacy_route_pure.cpp | test_tsl9_routes_suite.cpp | case_tsl9_legacy_route_pure::run | 7 |
| test_tsl9_downgrade_window_pure.cpp | test_tsl9_hands_on_suite.cpp | case_tsl9_downgrade_window_pure::run | 6 |
| test_tsl9_hands_on_pure.cpp | test_tsl9_hands_on_suite.cpp | case_tsl9_hands_on_pure::run | 39 |
| test_v36b21_driver_monitor_all_profiles_pure.cpp | test_driver_monitoring_suite.cpp | case_driver_monitor_all_profiles_pure::run | 5 |
| test_v372_dms_nag_profiles_pure.cpp | test_driver_monitoring_suite.cpp | case_dms_nag_profiles_pure::run | 6 |
| test_v390_r79_dms_composition_pure.cpp | test_driver_monitoring_suite.cpp | case_r79_dms_composition_pure::run | 15 |
| test_v313_mode_h_single_pure.cpp | test_mode_h_suite.cpp | case_mode_h_single_pure::run | 3 |
| test_v36d5_mode_h_stop_carrier_pure.cpp | test_mode_h_suite.cpp | case_mode_h_stop_carrier_pure::run | 6 |
| test_v36e2_mode_h_rev4_pure.cpp | test_mode_h_suite.cpp | case_mode_h_rev4_pure::run | 56 |
| test_v36f2_mode_h_visual_rescue_pure.cpp | test_mode_h_suite.cpp | case_mode_h_visual_rescue_pure::run | 37 |
| test_v37_mode_h_defaults_pure.cpp | test_mode_h_suite.cpp | case_mode_h_defaults_pure::run | 20 |
| test_v36b16_mode_h_rev1_pure.cpp | test_legacy_mode_h_suite.cpp | case_mode_h_rev1_pure::run | 13 |
| test_v36b20_mode_h_variants_pure.cpp | test_legacy_mode_h_suite.cpp | case_mode_h_variants_pure::run | 29 |
| test_v36c2_mode_h_rev3_pure.cpp | test_legacy_mode_h_suite.cpp | case_mode_h_rev3_pure::run | 34 |
| test_v36d4_defaults_pure.cpp | test_legacy_mode_h_suite.cpp | case_defaults_pure::run | 18 |
| test_v36f1_ap_right_scroll_pure.cpp | test_legacy_scroll_suite.cpp | case_ap_right_scroll_pure::run | 26 |
| test_v37_ap_right_scroll_visual_pure.cpp | test_legacy_scroll_suite.cpp | case_ap_right_scroll_visual_pure::run | 42 |
| test_tsl9_right_scroll_sequence_pure.cpp | test_legacy_scroll_suite.cpp | case_tsl9_right_scroll_sequence_pure::run | 16 |
| test_tsl9_scroll_warning_pure.cpp | test_legacy_scroll_suite.cpp | case_tsl9_scroll_warning_pure::run | 10 |


---

# Dashboard browser coverage mapping

The feature suites below retain every scenario and assertion from the completed
v3.28.5 tests. Each scenario function keeps its independent browser, page, route
mocks, mutable server state and cleanup. The suites share real source/embedded
artifact loading and font extraction through `browser_test_runtime.js`; they do
not evaluate historical test files or store tests in source-string containers.

Both `dashboard_source.html` and the actual `INDEX_HTML_GZ` payload are checked
by default. `DASHBOARD_SOURCE_ONLY=1` is for targeted source-only iteration.
Existing feature-specific source-only environment variables remain aliases.

| Previous file | Current suite and case function | Preserved behavior |
| --- | --- | --- |
| `test_country_override_dashboard.js` | `test_dashboard_country.js` / `checkCountryModes` | All five profiles; STOCK/US/KOREA/NZ mode saves; reload from server; stock bit19/47 and raw telemetry; CAN A/B role labels; rejected writes and unsupported state; LAB independence; 320/390/430 px light/dark layout and optional screenshots. |
| `test_dashboard_v3150_country_lane_graph.js` | `test_dashboard_country.js` / `checkCountryMapAndLayout` | Independent country/map selections and persistence; rollback and unknown-result recovery; stale poll ordering; LAB independence; removed panel absence; map-only summary; navigation settling; Settings and panel responsive checks; page errors. |
| `test_dashboard_v323_standalone_features.js` | `test_dashboard_settings_controls.js` / `checkDriverMonitoring` | Standalone DMS with Nag unsupported; one correct endpoint write; no Nag config dependency; persisted reload; mobile themes and optional screenshots. |
| `test_dashboard_v326_isa_standalone.js` | `test_dashboard_settings_controls.js` / `checkIsaSuppression` | Standalone ISA on Party+Chassis with TSL9 unavailable; endpoint ownership; saved reload; write failure reconciliation; mobile themes and optional screenshots. |
| `test_dashboard_settings_transactions.js` | `test_dashboard_settings_controls.js` / `checkSettingsTransactions` and `checkSteadyPolling` | Initial DMS double-failure regression; 46 browser scenarios across DMS/ISA/Country/Continuous AP (confirmed state, GET recovery, lost reply, duplicate write, malformed reply, support gates, stale success/failure, coalesced polling, post-save refresh and GET-only recovery); four sustained-poll latency cases; uncertain-state layouts and screenshots. |
| `test_dashboard_v328_continuous_ap.js` | `test_dashboard_continuous_ap.js` / `checkContinuousApSettings` | Method capabilities and selector sheet; enable/method persistence; rejected and lost POST replies; unknown-result lock and recovery; no repeated writes; reload; saved unsupported ON can turn OFF; unavailable initial GET; mobile themes and reference screenshots. |
| `test_dashboard_v3281_continuous_ap.js` | `test_dashboard_continuous_ap.js` / `checkContinuousApReadiness` | Live RX readiness independent of engagement; cancellation evidence; stale/invalid/missing signal states; failed GET removes green claim; collapsed/expanded row behavior; mobile detail rendering and optional screenshots. |

The seven previous test files become three discovered feature suites, reducing
`test_*.js` entries from 26 to 22 without removing any behavior checks. Common
fixtures are added to the existing runtime helper, so no extra JS helper file is
introduced. Every former function body is retained apart from its descriptive
function name; artifact loading and the top-level invocation are consolidated.

Runner discovery still uses top-level `tests/test_*.js`. Workspace scheduling and
browser-optimization allowlists must name the new suites; a safe fallback is to
execute the suites unchanged with their own isolated browser lifetimes.


## Production header ownership

Pure implementations retain their function bodies; runtime and transport boundaries remain separate.

| Former header | Current owner |
|---|---|
| `continuous_ap_types.h` | `continuous_ap_fsm_pure.h` |
| `continuous_ap_frames_pure.h` | `continuous_ap_fsm_pure.h` |
| `continuous_ap_policy_pure.h` | `continuous_ap_fsm_pure.h` |
| `stalk_frame_pure.h` | `auto_blinker_pure.h` |
| `blinker_tx_policy_pure.h` | `auto_blinker_pure.h` |
| `ulc_policy_pure.h` | `ulc_compositor_pure.h` |
| `ulc_stalk_confirm_pure.h` | `ulc_compositor_pure.h` |
| `r79_mode2_pure.h` | `r79_fixed_policy_pure.h` |
| `r79_ap_gate_pure.h` | `r79_fixed_policy_pure.h` |
| `fixed_point_arduino.h` | `text_format_arduino.h` |
| `ap_drive_profile_pure.h` | `pedal_map_session_pure.h` |
| `can_a_rx_mode_pure.h` | `runtime_gate_pure.h` |
| `ap_right_scroll_pure.h` | `tsl9_input_scheduler_pure.h` |

The retired scroll engine is embedded only in `test_legacy_scroll_suite.cpp`; current interval validity and sanitization are compared for all 65,536 uint16 values. Mode H oracle fixtures are unchanged.
