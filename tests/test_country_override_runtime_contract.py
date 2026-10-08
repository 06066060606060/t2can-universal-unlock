from pathlib import Path

root = Path(__file__).resolve().parents[1]
ino = next(root.glob("*.ino")).read_text()
logic = (root / "vehicle_logic.h").read_text()
runtime = (root / "can_runtime.h").read_text()
api = (root / "web_api.h").read_text()


assert 'begin("countrylab"' in logic
assert 'getUChar("mode", COUNTRY_OVERRIDE_STOCK_PURE)' in logic
assert 'putUShort("selection"' in logic

gate = logic.split("static bool countryOverrideGateOpen()", 1)[1].split("\n}", 1)[0]
assert "countryOverrideRouteAllowedPure" in gate
assert "r79RuntimeStatusSnapshot" in gate
assert "R79_TX_STATE_ACTIVE" in gate
assert "countryOverrideGateOpenPure" in gate
assert "labMenuEnabled" not in gate
lab_setter = logic.split("static bool countryOverrideSetLabEnabledWithBarrier", 1)[1].split("\n}", 1)[0]
assert "countryOverrideCancelGeneration" not in lab_setter

assert "static void countryOverrideObserve238CanB(" in logic
assert "static void countryOverrideObserve238CanA(" in logic
assert "static void countryOverrideObserve7ffCanA(" in logic
assert "static void countryOverrideObserve7ffCanB(" in logic
assert "countryOverrideApply238Pure" in logic
assert "countryOverrideApply7ffPure" in logic
assert "countryOverrideCancelGeneration" in logic
assert "canTxMcpSendTaggedGuarded" in logic
assert logic.count("canTxTwaiTransmitWithMaskTaggedGuarded") >= 2
for handler_name in ("countryOverrideObserve238CanB", "countryOverrideObserve7ffCanB"):
    handler = logic.split(f"static void {handler_name}(", 1)[1].split("\n}", 1)[0]
    assert "twaiNonSummonAdmissionOpen()" in handler
    assert "canTxCancellationGenerationSnapshot" in handler
    assert "canTxTwaiTransmitWithMaskTaggedGuarded" in handler
    assert handler.index("canTxCancellationGenerationSnapshot") < handler.index("countryOverrideGateOpen()")
    assert handler.index("canTxCancellationGenerationSnapshot") < handler.index("countryOverrideModeSnapshot")
    assert handler.index("countryOverrideModeSnapshot") < handler.index("countryOverrideApply")
    assert "uint32_t txEpoch" in handler
    assert handler.index("countryOverrideGateOpen()") < handler.index("canTxTwaiTransmitWithMaskTaggedGuarded")
handler_a = logic.split("static void countryOverrideObserve7ffCanA(", 1)[1].split("\n}", 1)[0]
assert "canTxCancellationGenerationSnapshot" in handler_a
assert "canTxMcpSendTaggedGuarded" in handler_a
assert handler_a.index("canTxCancellationGenerationSnapshot") < handler_a.index("countryOverrideGateOpen()")
assert handler_a.index("canTxCancellationGenerationSnapshot") < handler_a.index("countryOverrideModeSnapshot")
assert handler_a.index("countryOverrideGateOpen()") < handler_a.index("canTxMcpSendTaggedGuarded")
assert "countryOverrideUpdateR79AuthorizationLocked" in logic
assert "countryOverrideAuthorizationBarrierLock" in logic
assert "countryOverrideAuthorizationBarrierUnlock" in logic
assert "refreshSummonDerivedStateLocked(now);" not in logic
for function_name in ("refreshSummonState", "handle280", "handle390", "handle1016"):
    body = logic.split(f"static void {function_name}(", 1)[1].split("\n}", 1)[0]
    assert "countryOverrideAuthorizationBarrierLock" in body, function_name
    assert body.index("countryOverrideAuthorizationBarrierLock") < body.index("portENTER_CRITICAL(&stateMux)")
    assert "countryOverrideAuthorizationBarrierUnlock" in body, function_name
handle921 = logic.split("static void handle921(", 1)[1].split("\n}", 1)[0]
assert "countryOverrideAuthorizationBarrierLock" in handle921
assert handle921.index("countryOverrideAuthorizationBarrierLock") < handle921.index("portENTER_CRITICAL(&stateMux)")
assert "countryOverrideAuthorizationBarrierUnlock" in handle921

party = runtime.split("const uint16_t partyId", 1)[1].split("if (activeCanAIsParty())", 1)[0]
assert "partyId == 0x7FF" in party
assert "countryOverrideObserve7ffCanA" in party

case238 = runtime.split("case 0x238:", 1)[1].split("break;", 1)[0]
assert "countryOverrideObserve238CanB(f, countryRxEpoch)" in case238
case7ff = runtime.split("case 0x7FF:", 1)[1].split("break;", 1)[0]
assert "countryOverrideObserve7ffCanB(f, countryRxEpoch)" in case7ff

assert "countryOverrideObserve238CanA(rxf, countryRxEpoch)" in party
assert runtime.index("const uint32_t countryRxEpoch = canTxEpochSnapshot()") < runtime.index("Can_A.readMessage")
assert runtime.index("uint32_t countryRxEpoch = canTxEpochSnapshot()", runtime.index("static void canTaskTwai")) < runtime.index("twai_receive")
update = api.split("static void httpCountryOverrideUpdate()", 1)[1].split("\n}", 1)[0]
assert 'arg != "0" && arg != "1" && arg != "2" && arg != "3"' in update
assert "vehicleProfileTopologyValid" in update
assert '"KOREA"' in api

assert 'server.on("/api/lab/country/stats", HTTP_GET, httpCountryOverrideStats);' in api
assert 'server.on("/api/lab/country/update", HTTP_POST, httpCountryOverrideUpdate);' in api
for handler in ("httpCountryOverrideStats", "httpCountryOverrideUpdate"):
    assert "httpRequireLab()" not in api.split("static void " + handler + "()", 1)[1].split("\n}", 1)[0]
assert 'server.on("/api/country/stats", HTTP_GET, httpCountryOverrideStats);' in api
assert 'server.on("/api/country/update", HTTP_POST, httpCountryOverrideUpdate);' in api
assert "countryOverrideApplySelection" in api
assert "countryOverrideApplySelection" in api
assert "countryOverrideSetLabEnabledWithBarrier" in api.split("static void httpFeatureLab()", 1)[1].split("\n}", 1)[0]

for key in (
    '"countryMode"', '"countryGateOpen"', '"countryRx238"',
    '"countryRx7ffA"', '"countryRx7ffB"', '"countryBlocked"',
    '"countryTxOk"', '"countryTxFail"', '"countryLastBus"',
    '"countryLastPage"', '"countryLastRaw"', '"countryLastResult"'
):
    assert key in api, key

print("PASS Settings country override runtime/API contract")
