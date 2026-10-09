from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
CAPTURE = (ROOT / "can_research_capture.h").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")
HTML = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

assert '#define FW_VERSION "v3.28.0"' in INO

# SNAPSHOT and RAW TRANSITION share one fixed PSRAM allocation. A mode change
# must not free the working buffers and attempt a larger fragmented allocation.
assert "RESEARCH_CAPTURE_RAW_ARCHIVE_CAPACITY = 163840" in CAPTURE
assert "RESEARCH_CAPTURE_FIXED_MAIN_BYTES" in CAPTURE
assert "RESEARCH_CAPTURE_FIXED_AUX_BYTES" in CAPTURE
set_mode = CAPTURE[
    CAPTURE.index("static bool researchCaptureSetMode(uint8_t mode)"):
    CAPTURE.index("static inline uint16_t researchCaptureStateIndex", CAPTURE.index("static bool researchCaptureSetMode(uint8_t mode)"))
]
assert "researchCaptureReleaseModeBuffers();" not in set_mode

# Only the two supported user modes remain selectable and accepted by the API.
select = re.search(r'<select[^>]+id="researchCapMode"[^>]*>(.*?)</select>', HTML, re.S)
assert select
options = re.findall(r'<option[^>]+value="([^"]+)"', select.group(1))
assert options == ["SNAPSHOT", "RAW_TRANSITION"]

mode_handler = API[
    API.index("static void httpResearchCaptureMode()"):
    API.index("static void httpResearchCaptureReset()", API.index("static void httpResearchCaptureMode()"))
]
assert "RAW_AUTO_ALC" not in mode_handler
assert "ULC_CONFIRM" not in mode_handler
assert "mode must be SNAPSHOT or RAW_TRANSITION" in mode_handler

# A rejected mode change must be surfaced instead of silently snapping the UI
# back to the previous server value.
save_mode = HTML[
    HTML.index("async function saveResearchMode()"):
    HTML.index("async function saveAllResearchLabels()")
]
assert "capture mode change failed" in save_mode
assert "alert(" in save_mode

print("PASS v3.28.0 stable two-mode research capture contract")
