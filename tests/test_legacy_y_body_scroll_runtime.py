from pathlib import Path

root = Path(__file__).resolve().parents[1]
runtime = (root / "can_runtime.h").read_text()
logic = (root / "vehicle_logic.h").read_text()
api = (root / "web_api.h").read_text()
profile = (root / "vehicle_profile.h").read_text()

body_route = profile.split("static inline bool vehicleProfileTsl9InputOnBodyCanA", 1)[1].split("\n}", 1)[0]
assert "return vehicleProfileCanAIsBody(id, topology);" in body_route
assert "vehicleProfileTsl9InputSupported" in profile

body_rx = runtime.split("} else if (activeCanAIsBody()) {", 1)[1].split("\n      }", 1)[0]
assert "tsl9InputObserveCanA(rxf)" in body_rx
assert "tsl9InputServiceCanA();" in runtime

chassis_rx = runtime.split("case VCLEFT_SWITCH_ID:", 1)[1].split("break;", 1)[0]
assert "activeProfileTsl9InputSupported()" in chassis_rx
assert "!activeProfileTsl9InputOnBodyCanA()" in chassis_rx
assert "tsl9InputObserveCanB(f)" in chassis_rx
assert "tsl9InputServiceCanB();" in runtime

assert "tsl9InputSendCanA" in logic and "canTxMcpSend" in logic
assert "tsl9InputSendCanB" in logic and "canTxTwaiTransmitWithMask" in logic
assert 'server.on("/api/ap-right-scroll' not in api

print("PASS TSL9-only input scheduler Body/Chassis route integration")
