"""The Actions-tab Settings row is presentation only.

Turning the tab off in Settings must not affect preheat. The engine is governed
by its own toggle (`preheatEnabled`) and by nothing else, so hiding the tab
leaves a running preheat running and does not clear the flag it would resume
from after a reboot. `preheatMenuEnabled` survives as a nav-visibility flag for
the dashboard's feature-policy payload.

These assertions are textual on purpose: the coupling lived in three separate
places (boot load, the Settings setter, and the tick gate), and any one of them
silently reintroduces it.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
rt = (ROOT / "preheat_runtime.h").read_text()
web = (ROOT / "web_api.h").read_text()


def _body(source, start_marker, end_marker="\n}\n"):
    """The text from `start_marker` up to the function's closing brace."""
    start = source.index(start_marker)
    return source[start:source.index(end_marker, start)]


def _code(source):
    """Drop // comments, so the checks test code and not prose."""
    return "\n".join(line.split("//")[0] for line in source.splitlines())


# --- the tick gate ---------------------------------------------------------
# The engine reads its own flag alone: the tab may be hidden while the car
# keeps heating.
tick = _code(_body(rt, "static inline void preheatRuntimeTick() {"))
assert "if (!preheatEnabled) {" in tick
assert "preheatMenuEnabled" not in tick

# --- the Settings setter ---------------------------------------------------
# Visibility only. It must not stop the engine, clear the engine flag, or drop
# the template, or hiding the tab would silently cancel a running preheat.
menu_set = _code(_body(rt, "static bool preheatMenuSetEnabled(bool enabled) {"))
assert "preheatNvsWrite(PREHEAT_NVS_MENU_KEY, enabled)" in menu_set
assert "preheatMenuEnabled = enabled;" in menu_set
assert "PREHEAT_NVS_ENABLED_KEY" not in menu_set
assert "preheatEnabled" not in menu_set
assert "preheatRuntimeResetUnderTxBarrier" not in menu_set

# --- boot restore ----------------------------------------------------------
# The two flags are read independently, so a preheat that was running when the
# tab was hidden resumes after a reboot exactly as it was left.
load = _code(_body(rt, "static void preheatMenuLoad() {"))
assert "preheatMenuEnabled = menu;" in load
assert "preheatEnabled = enabled;" in load
assert "preheatEnabled = menu && enabled;" not in load
assert "menu = prefs.getBool(PREHEAT_NVS_MENU_KEY, false);" in load
assert "enabled = prefs.getBool(PREHEAT_NVS_ENABLED_KEY, false);" in load
# Both default to false, so a failed NVS read still cannot arm the transmitter.
assert "bool menu = false;" in load
assert "bool enabled = false;" in load

# --- the capability report -------------------------------------------------
# Follows the profile, not the menu flag: the tab is reachable only when the
# menu flag is set, so a report that followed it would contradict the tab the
# moment it is shown again.
supported = _code(_body(rt, "static inline bool preheatFeatureSupported() {"))
assert "return preheatProfileSupported();" in supported
assert "preheatMenuEnabled" not in supported
assert "out.supported = preheatFeatureSupported();" in rt

# --- the only stops --------------------------------------------------------
# Three now: the tab's own toggle, the auto-off target, and the stator stall
# criterion that came with the flat revision. Hiding the tab is still not a stop,
# so the count is the forward declaration plus those three calls.
assert rt.count("preheatRuntimeResetUnderTxBarrier();") == 4
setter = _code(_body(rt, "static bool preheatSetEnabled(bool enabled) {"))
assert "if (!enabled) preheatRuntimeResetUnderTxBarrier();" in setter

# --- the HTTP surface ------------------------------------------------------
# The visibility endpoint persists the menu key and reports the policy payload;
# the engine stays behind the tab's own endpoints.
visibility = _code(_body(web, "static void httpFeaturePreheat() {"))
assert "preheatMenuSetEnabled(enabled)" in visibility
assert "v3FeaturePolicyJson()" in visibility
assert "preheatSetEnabled" not in visibility
assert 'jw.boolean("preheat", preheatMenuEnabled);' in web
assert "preheatSetEnabled(enabled)" in web
assert "preheatSetTargetCx10(targetCx10)" in web
