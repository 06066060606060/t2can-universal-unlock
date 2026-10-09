from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
cap = (root / 'can_research_capture.h').read_text(encoding='utf-8')
pure = (root / 'can_research_capture_pure.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')

assert '#define FW_VERSION "v3.28.0"' in ino
assert 'RESEARCH_CAPTURE_MODE_ULC_CONFIRM = 3' in cap
assert 'RESEARCH_CAPTURE_ULC_CONFIRM_PRE_MS = 3000' in cap
assert 'RESEARCH_CAPTURE_ULC_CONFIRM_POST_MS = 7000' in cap
assert 'researchUlcConfirmTargetIdPure' in pure
for token in ['0x247', '0x3F8', '0x3E9', '0x24A', '0x3FD', '0x293']:
    assert token in pure

# ULC_CONFIRM is a RAW mode but records only the six targeted RX IDs.
assert 'mode == RESEARCH_CAPTURE_MODE_ULC_CONFIRM' in cap
assert '!researchUlcConfirmTargetIdPure(id)' in cap
assert 'RESEARCH_CAPTURE_ULC_CONFIRM_PRE_MS' in cap
assert 'RESEARCH_CAPTURE_ULC_CONFIRM_POST_MS' in cap

# The legacy targeted recorder implementation remains available for historical
# CSV decoding, but v3.28.0 deliberately retires it from the public API/UI.
mode_handler = api[api.index('static void httpResearchCaptureMode()'):
                   api.index('static void httpResearchCaptureReset()')]
assert 'raw == "ULC_CONFIRM"' not in mode_handler
mode_select = html[html.index('id="researchCapMode"'):html.index('</select>', html.index('id="researchCapMode"'))]
assert 'ULC_CONFIRM' not in mode_select

# CSV must retain physical CAN side and logical profile bus role independently.
assert 'physical_bus,bus_role' in api
assert 'researchCapturePhysicalBusName' in api
assert 'researchCaptureProfileBusName' in api

print('v3.6d9a2 ULC Confirm Capture contract OK')
