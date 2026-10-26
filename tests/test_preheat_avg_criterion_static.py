"""The auto-off acts on the pack average, and the strip shows that same number.

The criterion moved from the coldest cell to the mean of the two cells 0x332
carries, because no frame in the DBC exposes a true pack average: min and max
are all the car sends. The strip alternates between that average and the range
behind it, and the average is computed on the device - the tab does no
arithmetic on sensor readings anywhere else.

These assertions are textual on purpose. The rule lives in four places that can
drift apart silently: the predicate, its two call sites, the JSON field the tab
reads, and the renderer. The engine and the browser disagreeing is the one
failure mode the driver cannot see.
"""
"""The auto-off acts on the car's own pack average, and the strip shows that number.

The criterion moved twice. It began on the coldest cell, then on the mean of the
two cells 0x332 mux 0 carries, and now it reads BMS_thermistorTAvg
from that same mux - the average the BMS computes over its own thermistors. The
mean was an invention of ours and disagreed with the car in 111 of the 180 mux-0
frames of the eight captures, by up to 0.5 C, so the number the engine stopped on
was not the number the car was acting on. Now the cell on the strip, the value the
engine compares and the value the BMS publishes are one field, and there is no
derived quantity left to drift.

These assertions are textual on purpose. The rule lives in four places that can
drift apart silently: the predicate, its two call sites, the JSON field the tab
reads, and the renderer. The engine and the browser disagreeing is the one
failure mode the driver cannot see.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
pure = (ROOT / "preheat_pure.h").read_text(encoding="utf-8")
rt = (ROOT / "preheat_runtime.h").read_text(encoding="utf-8")
web = (ROOT / "web_api.h").read_text(encoding="utf-8")
dash = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")


def _body(source, start_marker, end_marker="\n}\n"):
    """The text from `start_marker` up to the function's closing brace."""
    start = source.index(start_marker)
    return source[start:source.index(end_marker, start)]


def _code(source):
    """Drop comments, so the checks test code and not prose."""
    source = re.sub(r"/\*[\s\S]*?\*/", "", source)
    return "\n".join(line.split("//")[0] for line in source.splitlines())


# --- the predicate ---------------------------------------------------------
# The comparison is direct, because the car's own average already arrives as the
# tenth of a degree the strip prints. Validity and freshness still gate it: a
# stale reading must never stop the heat.
reached = _code(_body(pure, "static inline bool preheatTargetReachedPure("))
assert "return avgCx10 >= targetCx10;" in reached
assert "if (!avgValid) return false;" in reached
assert "if (avgAgeMs > maxAgeMs) return false;" in reached
# No derived average is left anywhere in the firmware: the helper that produced one
# was deleted with the mean-based rule, and a comparison of two cells coming back
# would mean the criterion had moved off the car's own number again.
assert "preheatAvgCx10Pure" not in pure
assert "packMinCx10" not in reached and "packMaxCx10" not in reached

# --- both call sites -------------------------------------------------------
# The snapshot the API serves and the predicate the tick acts on stay the same
# one, and both read preheatPackAvgCx10 - the value preheatDecodeBrickTempAvgPure
# writes. The cell on the strip is fed from the same field through the JSON below.
assert rt.count("preheatTargetReachedPure(") == 2
# The snapshot call passes the decoded average and its own age, not the pack min/max.
assert "preheatAvgValid, preheatAgeMs(now, preheatAvgMs), preheatPackAvgCx10," in rt
assert "preheatTargetCx10, PREHEAT_PACK_FRESH_MS);" in rt
now = _code(_body(rt, "static inline bool preheatTargetReachedNow(uint32_t now) {"))
assert "avgCx10 = preheatPackAvgCx10;" in now
assert "avgCx10, targetCx10, PREHEAT_PACK_FRESH_MS);" in now
# The average travels with the snapshot the JSON is built from, under the same
# ever-seen flag and age the raw row uses.
assert "out.packAvgCx10 = preheatPackAvgCx10;" in rt
assert "out.avgSeen = preheatAvgValid;" in rt
assert "out.avgAgeMs = preheatAgeMs(now, preheatAvgMs);" in rt
assert "int16_t packAvgCx10;" in rt

# --- the JSON contract -----------------------------------------------------
# The field name is what the tab reads; renaming it breaks the strip silently. It
# has to be the car's average and not a computed one.
assert 'jw.fixed("packTempAvgC", s.packAvgCx10, 1);' in web
assert "s.packTempAvgCx10" not in web

# --- the renderer ----------------------------------------------------------
# The cell alternates between the average and the range, and the average comes
# from the device: a client-side (min+max)/2 would be the first arithmetic on
# sensor readings anywhere in the dashboard.
bms = _code(_body(dash, "function renderPreheatBms(s){"))
assert "s.packTempAvgC" in bms
assert "(lo+hi)/2" not in bms and "(lo + hi) / 2" not in bms
assert "preheatAltPhase" in bms
# The swap is a text change, so CSS cannot stop it: the gate has to be in JS.
assert "prefers-reduced-motion: reduce" in dash
assert "preheatMotionReduced" in dash
timer = _body(dash, "setInterval(()=>{\n  if(preheatMotionReduced())return;")
assert "preheatAltPhase=!preheatAltPhase;" in timer
assert "},3000);" in timer
# The auto-off is the average, so the label has to say so - in both the renderer
# and the markup that shows before the first poll.
assert "'Reached · heat off':'Stop at average'" in dash
assert 'id="preheatTargetDesc">Stop at average<' in dash
assert "Stop at coldest cell" not in _code(dash)
assert "Stop at this cell temperature" not in dash
