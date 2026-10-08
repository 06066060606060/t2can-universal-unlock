from pathlib import Path

root = Path(__file__).resolve().parents[1]
product = "\n".join(
    p.read_text(encoding="utf-8")
    for p in root.iterdir()
    if p.suffix in {".h", ".cpp", ".ino", ".html"} and p.name != "index_html.h"
)

for token in (
    "R79_TRANSPORT_ROAMING_MIRROR",
    "R79_TRANSPORT_V26_LEGACY",
    "R79_TRANSPORT_MUX0_COLLISION",
    "R79LAB_TX_PRE_MUX1",
    "R79LAB_TX_POST_MUX2",
    "R79LAB_TX_LEGACY_MIRROR",
    "R79LAB_TX_V26_PERIODIC",
    "R79LAB_TX_MUX0_COLLISION",
    "r79PostMux2",
    "r79PreMux1",
    "R79_REFRESH_SUMMON_ONLY",
    "R79_REFRESH_OFF",
    "R79_SCHED_GUARDED_487_PHASE_WALK",
    "r79PhasedScheduler",
    "r79RoamingBurst",
    "r79TimingCapture",
    "/api/r79timing/",
):
    assert token not in product, token

for removed in (
    "r79_phase_walk_pure.h",
    "r79_phased_scheduler_pure.h",
    "r79_periodic_scheduler_pure.h",
    "r79_quiet_window_pure.h",
    "r79_pre_mux1_pure.h",
    "r79_post_mux2_pure.h",
    "r79_roaming_burst_pure.h",
    "r79_mux0_collision_pure.h",
    "r79_timing_capture.h",
    "r79_timing_capture.cpp",
):
    assert not (root / removed).exists(), removed

print("v3.7 retired R79 experiments absent: PASS")
