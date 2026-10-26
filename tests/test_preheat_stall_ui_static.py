"""A gate holding the engine back must not be painted as "Heating" - and a stall is
no longer painted at all.

The field report was "preheat does not turn on", and the strip could not answer
it: the status showed `Heating` while the tick returned above the transmit path
on every slot, so a preheat that never started looked exactly like one that was
running. The skip counters did not close the gap either - they are history, and
a BOOT_DELAY skip from boot still reads BOOT_DELAY minutes later.

Three things have to stay in step for the strip to name the live gate, and all
three are textual: the tick publishes the cascade on every tick (above the
periodic slot), the snapshot and the JSON expose that live value, and the tab
maps the device's own reason names instead of inventing one. The fourth claim
pinned here is unrelated to the gate: `Heating not needed now` was read off
0x212 bit5, which is 0 in all 268 frames of the reference capture taken while
the pack heated at 8 kW, so that sentence asserted a verdict the car never
delivered.

The stator-stall criterion is pinned here too, and its pins are inverted on
purpose: the reason still reaches the status chip, while the red row and the
count-up of the silence must not appear. See the block below for why.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
pure = (ROOT / "preheat_pure.h").read_text(encoding="utf-8")
rt = (ROOT / "preheat_runtime.h").read_text(encoding="utf-8")
web = (ROOT / "web_api.h").read_text(encoding="utf-8")
dash = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
doc = (ROOT / "docs" / "actions_tab" / "README.md").read_text(encoding="utf-8")


def _body(source, start_marker, end_marker="\n}\n"):
    """The text from `start_marker` up to the function's closing brace."""
    start = source.index(start_marker)
    return source[start:source.index(end_marker, start)]


def _code(source):
    """Drop comments, so the checks test code and not prose."""
    source = re.sub(r"/\*[\s\S]*?\*/", "", source)
    return "\n".join(line.split("//")[0] for line in source.splitlines())


# --- the predicate ---------------------------------------------------------
# Armed and gated is a stall; DISABLED and UNSUPPORTED are not, because the tab
# renders those two itself and the cascade only returns them when the matching
# flag is false.
stalled = _code(_body(pure, "static inline bool preheatStalledPure("))
assert "if (!enabled || !supported) return false;" in stalled
assert "return reason != PREHEAT_SKIP_NONE;" in stalled

# --- the tick publishes it on EVERY tick -----------------------------------
# Above the transmission gate, or the value would lag the state it describes -
# and a stalled engine is exactly what the strip has to see immediately. The gate
# is the flat revision's own: one cadence, one shape, so the whole decision is the
# periodic slot and there is no second branch to step before it.
tick = _code(_body(rt, "static inline void preheatRuntimeTick() {"))
eval_at = tick.index("preheatEvalReason = reason;")
gate_at = tick.index("if (!preheatDuePure(now, lastTxMs, PREHEAT_PERIOD_MS)) return;")
count_at = tick.index("preheatRuntimeCountSkip(reason);")
assert eval_at < gate_at, "the cascade must be published before the transmission gate"
# Counting stays below the gate, so the counters keep counting opportunities to
# transmit rather than ticks.
assert gate_at < count_at
assert "preheatLastEvalMs = now;" in tick

# --- immediate reassertion -------------------------------------------------
# The answer to a disagreeing car frame goes out ahead of the periodic slot, after
# the cascade is published, and only when the cascade is NONE - it can never
# bypass a gate the periodic frame obeys.
reply_at = tick.index("preheatRuntimeTransmit(now, true);")
assert eval_at < reply_at < gate_at, "the answer must sit between the cascade and the periodic slot"
reply_block = tick[tick.index("if (replyArmed) {"):reply_at]
assert "if (reason != PREHEAT_SKIP_NONE) {" in reply_block
assert "preheatReplyDuePure(replyArmed, now, lastReplyMs)" in reply_block
# The rate breaker has the last word before every answer.
assert "preheatReplyBreakerAllowPure(preheatReplyBreaker, now)" in reply_block
assert reply_block.index("preheatReplyBreakerAllowPure") > reply_block.index("preheatReplyDuePure")
# Armed on the RX side, from the car's frame only, and only while the engine is on.
observe = _code(_body(rt, "static inline void preheatRuntimeObserve("))
assert "preheatReplyArmPure(data, preheatTripFramePure(tripId), dlc)" in observe
assert "preheatEnabled &&" in observe
assert "preheatRuntimeTransmit" not in observe, "the receive path must not transmit"

# --- the stator stall criterion --------------------------------------------
# A second auto-off, and it is a state change like the target one, so it sits
# above the periodic gate as well: switching the engine off must not wait for a
# slot. Its clock is armed on the session's first successful frame, which is what
# keeps the boot grace and the MCP warmup from tripping it.
#
# The stop itself is SILENT: the status chip naming the reason is the whole of
# the presentation and a stall is read from
# GET /api/preheat/stats. The pins below are therefore negative: they fail if the
# alarm comes back, which is the only way a future revision can reintroduce it
# without noticing.
stall_at = tick.index("preheatAutoOffReason = PREHEAT_AUTOOFF_STATOR_STALL;")
assert stall_at < gate_at, "the stall auto-off must sit above the transmission gate"
assert "if (lastTxMs != 0u) {" in tick
assert "preheatStatorStallElapsedPure(now, preheatStatorZeroSinceMs)) {" in tick
assert 'jw.string("autoOffReason", preheatAutoOffReasonName(s.autoOffReason));' in web
# The chip still names the reason: a self-stopped engine must not read as a
# manual toggle. That is the one surface of it and it is measured by the preview.
assert "s.autoOffReason==='STATOR_STALL'?'Stall auto-off'" in dash

# The alarm row and its count-up are gone, including the CSS rule that had to
# outrank the theme's own .listdesc for the red to be visible at all. Asserted on
# code, not on prose: the removal is documented in a comment on the tab, and a
# comment is not a rule the browser applies.
dash_code = _code(dash)
for gone in ("ptAlarm", "stators 0 for", "AUTO-OFF:", "statorZeroAge",
             "statorZeroAgeMs", "preheatStatorZeroFrozenMs",
             "preheatStatorZeroAgePure"):
    assert gone not in dash_code, "the stall alarm is back on the tab: " + gone
assert "preheatStatorZeroFrozenMs" not in rt, (
    "the frozen silence clock lived only to feed the count-up that was removed")
assert "statorZeroAgeMs" not in rt and "statorZeroAgeMs" not in web, (
    "statorZeroAgeMs is no longer served: nothing on the tab counts the silence")
assert "preheatStatorZeroAgePure" not in pure
# The clock itself stays: it is the criterion, and only the clock's snapshot for
# the screen went with the screen.
assert "uint32_t preheatStatorZeroSinceMs = 0;" in rt
reset = _code(_body(rt, "static inline void preheatRuntimeResetUnderTxBarrier() {"))
assert "preheatStatorZeroSinceMs = 0;" in reset
session = _code(_body(rt, "static inline void preheatSessionRestart() {"))
assert "preheatStatorZeroSinceMs = 0;" in session
assert "preheatAutoOffReason = PREHEAT_AUTOOFF_NONE;" in session

# --- the snapshot carries it -----------------------------------------------
assert "uint8_t evalReason;" in rt
assert "uint32_t evalAgeMs;" in rt
assert "bool blocked;" in rt
assert "out.evalReason = preheatEvalReason;" in rt
assert "out.blocked = preheatStalledPure(out.enabled, out.supported, out.evalReason);" in rt
# evalAgeMs is the one stall the counters cannot show: they freeze with the
# service instead of moving, so the tab needs the age of the last evaluation.
assert "out.evalAgeMs = lastEvalMs ? (uint32_t)(now - lastEvalMs) : 0u;" in rt

# --- the JSON contract -----------------------------------------------------
# Field names are what the tab reads; renaming any breaks the strip silently.
assert 'jw.boolean("blocked", s.blocked);' in web
assert 'jw.string("blockedReason", preheatSkipReasonName(s.evalReason));' in web
assert 'jw.u32("evalAgeMs", s.evalAgeMs);' in web

# --- the tab names the device's own reasons --------------------------------
# Every case label has to be a name preheatSkipReasonName() can actually emit,
# and the fallback must not be "Heating": the strip may never claim a
# transmission it is not making.
names = dict(re.findall(r'case (PREHEAT_SKIP_[A-Z_]+): return "([A-Z_]+)";', pure))
stall_fn = _code(_body(dash, "function preheatStallText(r){"))
labels = re.findall(r"case '([A-Z_]+)':return '([^']+)';", stall_fn)
assert labels, "preheatStallText lost its case labels"
# A duplicated label used to be invisible here: re.findall -> dict collapses the
# repeats, so two `case 'BUS_SILENT'` lines reduced to one entry and the dead one
# - whose text a user would never see - passed every other assertion. Count the
# raw labels, not the deduped ones, so a copy-paste rename cannot hide again.
seen = [name for name, _ in labels]
assert len(seen) == len(set(seen)), (
    "preheatStallText has duplicate case labels: "
    + repr(sorted({n for n in seen if seen.count(n) > 1})))
cases = dict(labels)
assert set(cases) <= set(names.values()), (
    "the tab names a reason the firmware cannot emit: "
    + repr(sorted(set(cases) - set(names.values()))))
# The four gates that can hold an armed engine, and nothing else: the source
# frame's age is no longer a gate (the payload is a constant) but the trip ID it
# reveals is (0x082 or, on HW4, 0x347), and DISABLED / UNSUPPORTED are not stalls -
# the tab names those two itself.
assert set(cases) == {"BOOT_DELAY", "MCP_WARMUP", "BUS_SILENT", "NO_TRIP_ID"}
assert "default:return 'Waiting';" in stall_fn
assert "'Heating'" not in stall_fn

# --- the status line -------------------------------------------------------
# Scoped to renderPreheat: fetchPreheat()'s error path writes the same element
# with a "Retrying…" placeholder, and that one is not part of the contract.
render = _code(_body(dash, "function renderPreheat(s){"))
status = [l for l in render.splitlines() if "$('preheatStatus').textContent=" in l]
assert len(status) == 1, "expected exactly one preheatStatus writer in renderPreheat"
status = status[0]
assert "s.blocked===true?preheatStallText(s.blockedReason)" in status
# The gate is decided before the TX-failure fallback, or an engine with one
# historical success and a stalled gate would still read "Heating".
assert status.index("s.blocked===true") < status.index("'No TX'")
assert status.index("s.blocked===true") < status.rindex("'Heating'")
# An engine that switched itself off is named by its reason. Without this the tab
# shows the same "Off" a manual toggle shows, and the cause is only readable in
# the telemetry rows - which is what a field report could not see.
assert "s.autoOffReason==='STATOR_STALL'?'Stall auto-off'" in status
assert status.index("s.autoOffReason==='STATOR_STALL'") < status.rindex("'Off'")

# --- a stale evaluation means the service itself stopped --------------------
# The one stall no counter can show: the counters freeze with the service
# instead of moving. It outranks the gate, because the gate it would name is
# itself stale by then, and it is why evalAgeMs is on the wire at all.
stale = _code(_body(dash, "function preheatServiceStalled(s){"))
assert "Number(s.evalAgeMs)" in stale
assert "PREHEAT_EVAL_STALE_MS" in stale
assert "const PREHEAT_EVAL_STALE_MS=3000;" in dash
assert "active?(preheatServiceStalled(s)?'No CAN data':" in status
assert status.index("preheatServiceStalled(s)") < status.index("s.blocked===true")

# --- the IDLE sentence the car never said ----------------------------------
bms = _code(_body(dash, "function renderPreheatBms(s){"))
assert "Heating not needed now" not in bms
assert "heating not needed now" not in bms.lower()
assert "else{text='IDLE';desc='BMS not heating'}" in bms
# The worthwhile branch stays: it is the contract, even though 0x212 bit5 never
# rises on this car and the branch is therefore unreachable.
assert "else if(worthwhile){text='READY';cls='good';desc='Heating advised'}" in bms

# --- the JSON contract is documented -------------------------------------
for field in ("`blocked`", "`blockedReason`", "`evalAgeMs`"):
    assert field in doc, f"JSON contract table is missing {field}"
