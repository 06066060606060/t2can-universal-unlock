from pathlib import Path
r = Path(__file__).resolve().parents[1]
ino = next(r.glob('*.ino')).read_text(errors='ignore')
vl = (r/'vehicle_logic.h').read_text(errors='ignore')
web = (r/'web_api.h').read_text(errors='ignore')
html = (r/'dashboard_source.html').read_text(errors='ignore')
pure = (r/'ulc_stalk_confirm_pure.h').read_text(errors='ignore')

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'ULC_NO_CONFIRM_TIMING_AP_ACTIVE_ONLY_PURE' in pure
assert 'ULC_NO_CONFIRM_TIMING_PRE_AP_PURE' in pure
assert 'ulcNoConfirmGateOpenWithTimingPure' in pure
assert 'ulcNoConfirmTimingMode' in vl
assert 'getUChar("timing"' in vl
assert 'putUChar("timing"' in vl
assert 'stalkConfirmTimingMode' in web
assert 'server.hasArg("timing")' in web
assert 'const String arg = server.arg("timing")' in web
assert 'arg != "0" && arg != "1"' in web
assert 'ulcNoConfirmGateOpenWithTimingPure' in vl
assert 'selected.confirmFreeEnabled && !gates.confirmFreeOpen' in vl
print('PASS production Confirm-Free timing contract')
