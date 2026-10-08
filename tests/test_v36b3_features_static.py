from pathlib import Path

root = Path(__file__).resolve().parents[1]
logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
api = (root / 'web_api.h').read_text(encoding='utf-8')
ble = (root / 's3xy_ble.h').read_text(encoding='utf-8')
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
ino_files = list(root.glob('*.ino'))
assert len(ino_files) == 1
ino = ino_files[0].read_text(encoding='utf-8')

# Version / package contract.

# TLSSC disable/gate transitions must have an explicit one-shot clear path.
for symbol in ('tlsscInjectedActive', 'tlsscClearPending', 'setTlsscEnabled', 'tlsscBlockInNoa'):
    assert symbol in logic or symbol in api, f'missing TLSSC b3 symbol: {symbol}'
assert 'setBit(out.data, 38, false)' in logic
assert 'setBit(out.data, 39, false)' in logic
assert 'tlsscBlockInNoa' in api and 'tlsscBlockInNoa' in html

# S3XY must expose TLSSC and Auto Blinker toggle actions.
for token in ('S3XY_ACTION_TLSSC_TOGGLE', 'S3XY_ACTION_AUTO_BLINKER_TOGGLE'):
    assert token in ble, f'missing S3XY action {token}'
for code in ('tlssc_toggle', 'auto_blinker_toggle'):
    assert code in ble and code in html, f'missing S3XY action code {code}'

# AP Pedal / Regen Profile still forces CHILL on 0x334. Regen target/topology support is
# covered by the newer v3.6b7 contract.
assert 'writeBitsLE(outData,5,2,0)' in logic.replace(' ', ''), 'AP profile must force CHILL raw 0'
assert 'AP Accel / Regen' in html
assert 'apDriveProfileToggle' in html

# HOME Injection State layout: no Summon Ready tile; R79 + CAN A/B remain.
assert 'Summon Ready' not in html
assert 'R79 TX' in html and 'homeCanAName' in html and 'homeCanBName' in html

# Main-tab motion and its no-overflow behavior are exercised in the real
# browser regression. Detail-panel motion remains part of this static contract.
assert 'v38PanelForward' in html and 'v38PanelExitBack' in html
assert 'animationend' in html, 'panel polling should be deferred until animation end'

# Once firmware has asserted TLSSC, a later stock 1/1 frame must not silently
# discard ownership before the requested one-shot OFF transition can be sent.
stock_on = logic.index('if (!getBit(out.data, 38) || !getBit(out.data, 39))')
stock_on_end = logic.index('    setBit(out.data, 38, true)', stock_on)
assert 'tlsscInjectedActive = false' not in logic[stock_on:stock_on_end], 'TLSSC ownership must survive stock 1/1 after our ON injection'

# Removed HOME Summon card must not leave live JS dereferences behind.
for removed_id in ('gatePill','gateMode','sumHomeSession','gPark','gSpr','gAca','gQueue'):
    assert f"$('{removed_id}')" not in html, f'removed HOME Summon id still dereferenced: {removed_id}'

print('v3.6b3 feature contract OK')
