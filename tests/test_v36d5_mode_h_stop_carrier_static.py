from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
ino = next(ROOT.glob('*.ino')).read_text()
core = (ROOT/'can_core.h').read_text()
web = (ROOT/'web_api.h').read_text()
dash = (ROOT/'dashboard_source.html').read_text()
variant = (ROOT/'nag_mode_h_variant_pure.h').read_text()

assert '#define FW_VERSION "v3.26.3"' in ino
assert 'H_STOP_HARD_PAUSE' in variant and 'H_STOP_STOCK_CARRIER' in variant
assert 'c.modeHStopBehavior = nagModeHDefaultStopBehaviorPure();' in core
assert 'const uint8_t storedStopDefault = nagCfgVersion < 18u' in core
assert 'prefs.getUChar("hsb", storedStopDefault)' in core
assert 'prefs.putUChar("hsb"' in core
assert 'prefs.putUChar("v",     20u);' in core
assert 'nagModeHUseStopCarrierPure(modeHStopBehavior, confirmedStopped)' in core
assert 'humanDecision.raw = tRaw;' in core
assert 'humanDecision.hoOverrideValid = false;' in core
assert 'modeHStopCarrier = true;' in core
assert 'if (modeHStopCarrier) nagStopCarrierTxOk++;' in core
assert 'modeHStopBehavior' in web and 'stopCarrierTxOk' in web
assert 'id="humanStopBehavior"' in dash
assert 'STOCK CARRIER' in dash and 'HARD PAUSE · 0-TX' in dash
assert "modeHStopBehavior='+v" in dash
print('v3.6d9a2 Mode H stop-carrier static contract: PASS')
