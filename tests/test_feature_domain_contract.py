from pathlib import Path


root = Path(__file__).resolve().parents[1]
logic = (root / "vehicle_logic.h").read_text()
api = (root / "web_api.h").read_text()
ino = next(root.glob("*.ino")).read_text()
runtime = (root / "can_runtime.h").read_text()
product = logic + api + ino + runtime


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


for required in [
    'begin("ulc"',
    '"alcOff"',
    '"blind"',
    '"ulcOff"',
    '"confirm"',
    '"timing"',
    'begin("alc293lab"',
    '"enabled"',
    '"bus"',
    'server.on("/api/ulc/stats"',
    'server.on("/api/ulc/update"',
    'server.on("/api/lab/auto-lane-change/stats"',
    'server.on("/api/lab/auto-lane-change/update"',
]:
    assert required in product, f"missing production domain contract: {required}"

assert "/api/lab3f8/" not in api
for obsolete in ["ulcBus", "accfd", "ulcspd"]:
    assert f'getUChar("{obsolete}"' not in product
    assert f'putUChar("{obsolete}"' not in product

assert "static constexpr uint16_t NVS_SCHEMA_ULC = 2;" in logic
assert "static constexpr uint16_t NVS_SCHEMA_CURRENT = 3;" in logic
migration = function_body(logic, "static bool featureConfigMigrateToSchema2()")
verify_pos = migration.index("featureConfigSchema2ReadBackMatches(")
schema_pos = migration.index('putUShort("schema", NVS_SCHEMA_ULC)')
assert verify_pos < schema_pos

ulc_update = function_body(api, "static void httpUlcUpdate()")
assert "httpRequireLab()" not in ulc_update
alc_update = function_body(api, "static void httpAutoLaneChangeLabUpdate()")
assert "httpRequireLab()" in alc_update
assert "httpUlcMonitorLabStats" not in api
assert "/api/lab/ulc-monitor/" not in api

for removed in [
    "lab3f8AccFollowRaw",
    "lab3f8UlcSpeedMode",
    "ulcNoConfirmTargetBus",
    "ulcNoConfirmInjectCanA",
    "tlsscGreen",
]:
    assert removed not in product, f"removed feature remains: {removed}"

print("production ULC domains, schema migration, and experiment removal: PASS")
