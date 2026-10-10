from pathlib import Path


root = Path(__file__).resolve().parents[1]
api = (root / "web_api.h").read_text()
logic = (root / "vehicle_logic.h").read_text()


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


assert 'server.on("/api/nag/h-profile/stats"' in api
assert 'server.on("/api/nag/h-profile/update"' in api
assert 'server.on("/api/nag/h-profile/reset"' in api
assert "/api/nag-human-lab/" not in api
for handler in [
    "httpNagHumanProfileStats",
    "httpNagHumanProfileUpdate",
    "httpPedalMapStats",
    "httpPedalMapSet",
]:
    body = function_body(api, f"static void {handler}()")
    assert "httpRequireLab()" not in body
assert "labMenuEnabled && enabled && apActive" not in logic

for route in [
    'server.on("/api/pedalmap/stats"',
    'server.on("/api/pedalmap/set"',
]:
    assert route in api
assert 'server.on("/api/ap-right-scroll' not in api
assert 'hasArg("tsl9InputMode")' in api
assert 'hasArg("tsl9IsaChimeSuppress")' not in api
assert 'server.on("/api/isa-suppression/config", HTTP_GET' in api
assert 'server.on("/api/isa-suppression/config", HTTP_POST' in api
assert 'server.on("/api/driver-monitoring/config", HTTP_GET' in api
assert 'server.on("/api/driver-monitoring/config", HTTP_POST' in api
assert 'hasArg("dmsControlEnabled")' not in api

print("promoted Mode H, PedalMap, standalone DMS/ISA, and TSL9 API contract: PASS")
