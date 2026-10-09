from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text()
core = (root / 'can_core.h').read_text()
runtime = (root / 'can_runtime.h').read_text()
api = (root / 'web_api.h').read_text()
html = (root / 'dashboard_source.html').read_text()

assert '#include "tsl9_hands_on_0x399_pure.h"' in ino
assert 'uint8_t  method;' in core
assert 'uint8_t  tsl9Sequence;' in core
assert 'uint8_t  tsl9DowngradeWindow;' in core
assert 'nagMethodSanitizePure' in core
assert 'prefs.getUChar("method"' in core
assert 'prefs.putUChar("method"' in core
assert 'prefs.getUChar("tsl9seq"' in core
assert 'prefs.putUChar("tsl9seq"' in core
assert 'prefs.getUChar("tsl9win"' in core
assert 'prefs.putUChar("tsl9win"' in core

# YL 0x399 and Legacy Body 0x39B use their profile-selected MCP2515 path.
assert 'static bool nagProcessTsl9Mcp(' in core
mcp_start = core.index('static bool nagProcessTsl9Mcp(')
mcp_end = core.index('static bool nagProcessTsl9Twai399(', mcp_start)
mcp = core[mcp_start:mcp_end]
assert 'tsl9ApplyDasTransformForCanIdPure' in mcp
assert 'canTxMcpSend' in mcp
assert 'activeProfileIsYl()' in mcp
assert 'nagTsl9Body39BSelected()' in mcp
assert '0x39Bu' in mcp

# Juniper and Highland keep their 0x399 TSL9 route on Chassis CAN B.
twai_start = core.index('static bool nagProcessTsl9Twai399(')
twai_end = core.index('// ── Nag process frame from MCP2515', twai_start)
twai = core[twai_start:twai_end]
assert 'activeCanBIsChassis()' in twai
assert 'canTxTwaiTransmitWithMaskTaggedGuarded(' in twai
assert '&isaSuppressionGeneration' in twai
assert 'tsl9ApplyDasTransformForCanIdPure' in twai
assert 'nagProcessTsl9Twai399(f)' in runtime
assert 'const bool handsOnRoute = nagTsl9Chassis399Selected();' in twai
assert 'handsOnRoute && enabled && method == NAG_METHOD_TSL9_PURE' in twai
selector = core.split('static bool nagTsl9Chassis399Selected()',1)[1].split('\n}',1)[0]
assert 'activeProfileNagTsl9Supported()' in selector
assert 'activeProfileIsaSuppressionSupported()' in twai
assert 'nagTsl9Chassis399Selected()' in twai

# The method switch is exclusive: TSL9 can still observe the AP-status frame,
# but the legacy torque injection path exits when TSL9 is selected.
assert 'nagProcessTsl9Mcp(rxf)' in core
assert 'if (methodNow != NAG_METHOD_TORQUE_PURE) return;' in core

for key in ('"method"', '"methodName"', '"tsl9Sequence"',
            '"tsl9SequenceName"', '"tsl9Window"', '"tsl9WindowName"',
            '"tsl9Rx"', '"tsl9TxOk"',
            '"tsl9TxFail"'):
    assert key in api
assert 'server.hasArg("method")' in api
assert 'server.hasArg("tsl9Window")' in api
update_start = api.index('static void httpNagUpdate()')
update_end = api.index('\nstatic void ', update_start + 1)
assert 'nagTsl9State = {}' not in api[update_start:update_end], (
    'method/enable changes must not restart the AP-active 12-second window'
)
assert 'id="nagMethod"' in html
assert 'id="tsl9LegacyRoute"' in html
assert 'id="tsl9SequenceWrap"' in html
assert 'id="tsl9Sequence"' in html
assert 'id="tsl9WindowWrap"' in html
assert 'id="tsl9Window"' in html
assert 'Entire AP session' in html
assert 'V8.2 Original · 4 → 1' in html
assert 'Extended · 2 / 3 / 4 → 1' in html
assert 'updateNagMethod' in html
assert 'updateTsl9Sequence' in html
assert "toggleNag(e.target.checked,'tsl9Window')" in html

print('TSL9 runtime/dashboard contract OK')
