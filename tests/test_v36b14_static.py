from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ino=next(ROOT.glob('*.ino')).read_text()
core=(ROOT/'can_core.h').read_text()
runtime=(ROOT/'can_runtime.h').read_text()
logic=(ROOT/'vehicle_logic.h').read_text()
api=(ROOT/'web_api.h').read_text()
dash=(ROOT/'dashboard_source.html').read_text()
ulc=(ROOT/'ulc_stalk_confirm_pure.h').read_text()
assert '#define FW_VERSION "v3.26.3"' in ino
# Confirm-Free is fixed to the production CAN-B 0x3F8 compositor.
assert 'ulcNoConfirmBusSelectablePure' not in ulc
assert 'static void lab3f8ObserveCanB' in logic
assert 'static void injectDriverAssistControl' in logic
assert 'stalkConfirmRoute' in api
assert 'stalkConfirmBusSelectable' not in api
assert 'ulcNoConfirmEffectiveTargetBus()' not in api

# Mode H engine contract is covered by the current b19 regression; the retired alternate engine is intentionally absent.
assert not (ROOT/'nag_human_pure.h').exists()
assert 'Mode H' in dash and 'Mode H · Rev.' not in dash
print('fixed-route ULC contract passed')
