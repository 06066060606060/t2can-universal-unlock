"""The auto-off target grid is one fact, and it lives in four places.

The window the dropdown offers IS the window the engine clamps to: 20.0..50.0 C in
5 C steps, seven stops, 40.0 C by default. A dropdown that offered a
value the API rejects would render blank when chosen, and an entry the clamp refuses
would be a control that cannot work. So the four places are pinned together here:

  * the constants and the clamp in preheat_pure.h;
  * the <select> in dashboard_source.html, in markup order;
  * the client-side gate that refuses to post a value outside the window;
  * the range the API names in its 400 body, which is what a caller reads.

The assertions are textual on purpose, exactly like the neighbouring preheat pins: the
engine and the browser disagreeing is the failure the driver cannot see.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
pure = (ROOT / "preheat_pure.h").read_text(encoding="utf-8")
web = (ROOT / "web_api.h").read_text(encoding="utf-8")
dash = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

# --- the engine's window -----------------------------------------------------
assert "static constexpr int16_t PREHEAT_TARGET_MIN_CX10 = 200;" in pure
assert "static constexpr int16_t PREHEAT_TARGET_MAX_CX10 = 500;" in pure
assert "static constexpr int16_t PREHEAT_TARGET_DEFAULT_CX10 = 400;" in pure
assert "static constexpr int16_t PREHEAT_TARGET_STEP_CX10 = 50;" in pure
# 55.0 C is gone from the header, comment included: a constant left at 550 would
# accept a value the dropdown no longer offers.
assert "550" not in pure.split("PREHEAT_REFERENCE_FRAME")[0]
clamp = pure[pure.index("static inline int16_t preheatClampTargetCx10"):]
clamp = clamp[:clamp.index("\n}\n")]
assert "value < PREHEAT_TARGET_MIN_CX10" in clamp
assert "value > PREHEAT_TARGET_MAX_CX10" in clamp

# --- the dropdown, in markup order -------------------------------------------
sel = dash[dash.index('id="preheatTargetC"'):]
sel = sel[:sel.index("</select>")]
options = re.findall(r'<option value="([\d.]+)"(?: selected)?>', sel)
assert options == ["20.0", "25.0", "30.0", "35.0", "40.0", "45.0", "50.0"], options
assert 'value="40.0" selected' in sel, "the shipped default is not pre-selected"
# The number of stops follows from the window, so the two can never drift.
lo, hi, step = 200, 500, 50
assert len(options) == (hi - lo) // step + 1
assert [float(o) for o in options] == [x / 10 for x in range(lo, hi + 1, step)]
assert "55.0" not in dash

# --- the client's own gate and the API's own text ----------------------------
assert "value<20||value>50" in dash, "the JS gate no longer matches the window"
assert "value<20||value>55" not in dash
assert '\\"targetC must be 20.0..50.0\\"' in web, \
    "the API's 400 body no longer names the window the clamp enforces"
assert "20.0..55.0" not in web