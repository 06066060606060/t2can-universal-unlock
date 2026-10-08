from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILE = (ROOT / "vehicle_profile.h").read_text(encoding="utf-8")
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
RUNTIME = (ROOT / "can_runtime.h").read_text(encoding="utf-8")
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")

selectable = PROFILE.split(
    "static inline bool vehicleProfileLegacyTsl9RouteSelectable", 1
)[1].split("\n}", 1)[0]
assert "VEHICLE_MODEL_Y_LEGACY" in selectable
assert "VEHICLE_MODEL_3_LEGACY" in selectable
assert "VEHICLE_MODEL_Y_JUNIPER" not in selectable
assert "VEHICLE_MODEL_3_HIGHLAND" not in selectable
assert "vehicleProfileCanAIsBody" in selectable

mcp_start = CORE.index("static bool nagProcessTsl9Mcp(")
mcp_end = CORE.index("static bool nagProcessTsl9Twai399(", mcp_start)
mcp = CORE[mcp_start:mcp_end]
assert "nagTsl9Body39BSelected()" in mcp
assert "0x39Bu" in mcp
assert "0x399u" in mcp
assert "tsl9ApplyDasTransformForCanIdPure" in mcp
assert "canTxMcpSend" in mcp

twai_start = CORE.index("static bool nagProcessTsl9Twai399(")
twai_end = CORE.index("// ── Nag process frame from MCP2515", twai_start)
twai = CORE[twai_start:twai_end]
assert "nagTsl9Chassis399Selected()" in twai
assert "tsl9ApplyDasTransformForCanIdPure" in twai
assert "0x399u" in twai

body_start = RUNTIME.index("} else if (activeCanAIsBody()) {")
body_end = RUNTIME.index("\n      }", body_start)
body = RUNTIME[body_start:body_end]
assert "nagTsl9Body39BSelected()" in body
assert "nagProcessTsl9Mcp(rxf)" in body

case_399 = RUNTIME[RUNTIME.index("case 921:"):RUNTIME.index("case 0x331:")]
assert "handle921(f.data, f.data_length_code)" in case_399
assert "nagProcessTsl9Twai399(f)" in case_399

recover_a = RUNTIME[
    RUNTIME.index("static void invalidateCanTxStateForCanARecovery()"):
    RUNTIME.index("static void invalidateCanTxStateForCanBRecovery()")
]
assert "nagTsl9Body39BSelected()" in recover_a
assert "nagTsl9State = {}" in recover_a

assert "function nagTsl9RouteInfo()" in DASH
route_ui = DASH[
    DASH.index("function nagTsl9RouteInfo()"):
    DASH.index("function renderNagCfg()")
]
assert "p===3||p===5" in route_ui
assert "t===2" in route_ui
assert "Body CAN A · 0x39B" in route_ui
assert "Chassis CAN B · 0x399" in route_ui
assert "Party CAN A · 0x399" in route_ui
assert "tsl9LegacyRoute" in route_ui

print("PASS Legacy-only Body 0x39B TSL9 route contract")
