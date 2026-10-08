from pathlib import Path


root = Path(__file__).resolve().parents[1]
api = (root / "web_api.h").read_text()
logic = (root / "vehicle_logic.h").read_text()
forward = (root / "t2can_forward.h").read_text()
ino = next(root.glob("*.ino")).read_text()
dashboard = (root / "dashboard_source.html").read_text()


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[brace : pos + 1]
    raise AssertionError(f"unclosed function: {signature}")


assert 'server.on("/api/blinkA/tx-mode", HTTP_POST, httpBlinkATxMode)' in api
assert '/api/lab/blinker-tx/' not in api
assert 'httpBlinkerTxLab' not in api + forward

load = function_body(logic, "static void featureCfgLoad()")
persist = function_body(logic, "static bool blinkerTxModePersist(uint8_t requested)")
update = function_body(api, "static void httpBlinkATxMode()")
feature_lab = function_body(api, "static void httpFeatureLab()")
stats = function_body(api, "static String blinkAStatsToJson()")

assert 'getUChar("blinkTx", 0xFFu)' in load
assert "blinkerTxStoredModePure" in load
assert 'begin("features", false)' in persist
assert 'putUChar("blinkTx", requested)' in persist
assert 'blinkerTxModePersist(requested)' in update
assert 'blinkerTxSetMode(requested)' in update
assert update.index('blinkerTxModePersist(requested)') < update.index('blinkerTxSetMode(requested)')
assert "NVS write failed" in update
assert "blinkerTxCancelLegacyLocked();" not in feature_lab
assert "blinkerTxDefaultModePure(activeProfileIsYl())" not in ino

for key in (
    "txMode",
    "txModeName",
    "txActiveSource",
    "txLastDirection",
    "txLastSource",
    "txLastResult",
    "txRequests",
    "txOk",
    "txFail",
    "txBlocked",
):
    assert f'"{key}"' in stats

assert "AUTO_BLINKER" in api and "S3XY_BUTTON" in api

blink_panel = dashboard[
    dashboard.index('id="panelBlink"') : dashboard.index('id="panelSummon"')
]
assert 'id="blinkTxMode"' in blink_panel
assert 'id="blinkTxSource"' in blink_panel
assert 'id="blinkTxLast"' in blink_panel
assert 'id="blinkTxRequests"' in blink_panel
assert 'id="blinkTxCounts"' in blink_panel
assert "S3XY blinker buttons" in blink_panel
assert "/api/blinkA/tx-mode?mode=" in dashboard
assert "txMode" in dashboard and "txActiveSource" in dashboard
for retired in (
    "panelLabBlinkerTx",
    "labBlinkerTxMode",
    "fetchBlinkerTxLab",
    "updateBlinkerTxLab",
):
    assert retired not in dashboard
print("production Auto Blinker TX policy contract: PASS")
