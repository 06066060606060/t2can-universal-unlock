from pathlib import Path


root = Path(__file__).resolve().parents[1]
logic = (root / "vehicle_logic.h").read_text()
runtime = (root / "can_runtime.h").read_text()
api = (root / "web_api.h").read_text()
forward = (root / "t2can_forward.h").read_text()
dashboard = (root / "dashboard_source.html").read_text()
firmware = logic + runtime + api + forward
all_source = firmware + "".join(
    path.read_text()
    for path in (
        root / "s3xy_ble.h",
        root / "fixed_point_pure.h",
        root / "fixed_point_arduino.h",
        root / "feature_config_migration_pure.h",
        root / "nag_human_v1_pure.h",
        root / "nag_human_v2_pure.h",
        root / "runtime_gate_pure.h",
        root / "ulc_policy_pure.h",
        root / "ulc_stalk_confirm_pure.h",
    )
)

for retired in (
    "r79LabObserve7ff",
    "r79Lab7ffSeen",
    "r79Lab7ffBus",
    "r79Lab7ffPage",
    "r79Lab7ffRx",
    "r79Lab7ffLastMs",
    "r79Lab7ffRaw",
    "r79LabBit18Rx0",
    "r79LabBit18Rx1",
    "r79LabBit19Rx0",
    "r79LabBit19Rx1",
    "r79LabBit47Rx0",
    "r79LabBit47Rx1",
    "r79LabBit18Changes",
    "r79LabBit19Changes",
    "r79LabBit47Changes",
    "r79LabStockApply",
    "r79LabStockSmart",
    "r79LabStockHardCore",
    "r79LabEffectiveApply",
    "r79LabEffectiveSmart",
    "r79LabEffectiveHardCore",
    "r79LabLastTxKind",
    "r79LabLast3fdMs",
    "r79LabLastEffectiveRaw",
    "r79LabLastAttemptMs",
    "r79LabLastPeriodicRequestMs",
    "r79LabLastPeriodicTxMs",
    "r79LabNoTemplateSkip",
    "r79LabManualSuspendSkip",
    "r79LabCanOfflineSkip",
    "r79LabAdminHoldSkip",
    "r79LabAppliedFrames",
    "r79FastEchoBlocked",
    "r79FastEchoLatency",
    "r79FastEchoLt100Us",
    "r79FastEchoLt250Us",
    "r79FastEchoLt1000Us",
    "r79FastEchoGe1000Us",
    "r79FastSuccess",
    "r79LabLastBlockReason",
    "r79LabLastBlockMs",
    "r79QuietCancelCount",
    "r79QuietDueCount",
    "r79QuietDisallowedSkip",
    "r79PeriodicRetryCancelledByStock",
    "r79PeriodicSlotDue",
    "r79PeriodicSlotFire",
    "r79PeriodicSlotGuard",
):
    assert retired not in firmware, f"dead R79 telemetry remains: {retired}"

for retained in (
    "r79DmsOnlyTxOk",
    "r79DmsOnlyTxFail",
    "r79DmsOnlyBlockedByR79",
    "r79LabBit43Rx0",
    "r79LabBit43Rx1",
    "r79LabBit43Changes",
    "r79LabStockCabinCamera",
    "r79LabEffectiveCabinCamera",
    "r79LabStockValid",
    "r79LabLastTxValid",
    "r79Lab3fdRx",
    "r79LabTxOk",
    "r79LabTxFail",
    "r79FastEchoAttempts",
    "r79FastEchoTxOk",
    "r79FastEchoTxFail",
    "r79LabPeriodicTxOk",
    "r79LabPeriodicTxFail",
    "r79QuietArmCount",
    "r79QuietFireCount",
    "r79QuietGuardSkip",
):
    assert retained in logic, f"required R79 state removed: {retained}"

for key in (
    "bit43Rx0",
    "bit43Rx1",
    "bit43Changes",
    "stockMux1Rx",
    "txOk",
    "txFail",
    "fastAttempts",
    "fastTxOk",
    "fastTxFail",
    "periodicTxOk",
    "periodicTxFail",
    "quietArm",
    "quietFire",
    "quietGuardSkip",
):
    assert f'"{key}"' in api, f"required R79 API key removed: {key}"

for missing_id in (
    "labAcc",
    "labNagHumanDesc",
    "labUlcOffBlocked",
    "labUlcOffLast",
    "labUlcPolicyGate",
    "labUlcPolicyRxAge",
    "retiredConfirmBus",
    "retiredUlcSpeed",
):
    assert f"$('" + missing_id + "')" not in dashboard
    assert f'q("{missing_id}")' not in dashboard

for retired in (
    "r79LabPeriodValid",
    "r79PeriodicObserveStock",
    "r79PreObserveStock",
    "r79LabStatsToJson",
    "httpR79LabStats",
    "httpR79LabUpdate",
    "httpR79LabStock",
    "r79LabTxKindName",
    "s3xyParseAction",
    "LAB3F8_ALC_FORCE_OFF",
    "deciString",
    "r79SmartOverrideActivePure",
    "formatDeciPure",
    "FeatureConfigSchema3CommitPure",
    "featureConfigSchema3CommitPure",
    "NAG_HUMAN_V1_B19_DEFAULT_MIN_RAW",
    "NAG_HUMAN_V1_B19_DEFAULT_MAX_RAW",
    "nagHumanV1MigrateB19DefaultToRev1PlusPure",
    "nagHumanV2ConfigValidPure",
    "uiUlcOffHighwayApplyRawPure",
    "ulcNoConfirmApplyByte0Pure",
):
    assert retired not in all_source, f"dead helper remains: {retired}"

assert "stalkFramePreparePure" in logic
assert logic.count("stalkFramePreparePure") == 2
assert api.count("canTxTraceRawHex") == 3
assert "writeProfileCapabilityJson" in api
assert api.count("writeNagHumanV1ConfigJson") == 2  # definition + sole Mode H engine
assert api.count("writeNagHumanV1StateJson") == 2  # definition + sole Mode H engine

print("v3.7.3 dead telemetry and stale dashboard cleanup contract: PASS")
