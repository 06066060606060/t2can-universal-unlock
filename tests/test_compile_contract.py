from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
forward = (root / 't2can_forward.h').read_text(encoding='utf-8')

# t2can_forward.h is included before vehicle_logic.h. The R79 type therefore
# must be declared before the forward prototype that returns it by value.
proto = 'static R79RuntimeStatus r79RuntimeStatusSnapshot(uint32_t now);'
assert proto in forward, 'R79 runtime snapshot prototype missing'
proto_pos = forward.index(proto)
prior = forward[:proto_pos]
assert re.search(r'\bstruct\s+R79RuntimeStatus\s*;', prior), (
    'R79RuntimeStatus must be forward-declared before r79RuntimeStatusSnapshot prototype'
)
print('compile contract OK')


# b14 Mode H reads ALC state from can_core.h, which is included before vehicle_logic.h.
# Shared state must therefore be defined in t2can_core_state.h, not first declared later.
ino = next(root.glob('*.ino')).read_text(encoding='utf-8')
core_state = (root / 't2can_core_state.h').read_text(encoding='utf-8')
vehicle_logic = (root / 'vehicle_logic.h').read_text(encoding='utf-8')
assert ino.index('#include "t2can_core_state.h"') < ino.index('#include "can_core.h"') < ino.index('#include "vehicle_logic.h"')
for token in (
    'static portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;',
    'static volatile uint8_t dasAutoLaneChangeState = 0xFF;',
    'static volatile bool dasAutoLaneChangeStateValid = false;',
):
    assert token in core_state, f'shared declaration must precede can_core.h: {token}'
    assert token not in vehicle_logic, f'duplicate late declaration remains in vehicle_logic.h: {token}'
print('b14 shared-state include-order contract OK')


# v3.7 web API calls the administrative TX hold helper during profile/reset
# transitions. It must be defined in can_core.h before web_api.h is included.
core = (root / 'can_core.h').read_text(encoding='utf-8')
web = (root / 'web_api.h').read_text(encoding='utf-8')
assert 'static void setCanTxAdministrativeHold(bool hold)' in core
assert 'setCanTxAdministrativeHold(true);' in web
assert ino.index('#include "can_core.h"') < ino.index('#include "web_api.h"')
print('v3.7 administrative TX hold compile contract OK')
