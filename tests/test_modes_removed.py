from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
files = {
    name: (root / name).read_text(encoding='utf-8')
    for name in ['can_core.h', 'runtime_gate_pure.h', 't2can_forward.h', 'web_api.h', 'dashboard_source.html']
}
all_text = '\n'.join(files.values())

# Removed production modes must not remain as selectable/runtime symbols.
for symbol in ('MODE_D', 'MODE_E', 'MODE_F'):
    assert not re.search(rf'\b{symbol}\b', all_text), f'{symbol} still present'
for phrase in ('Mode D', 'Mode E', 'Mode F', 'D/E/F'):
    assert phrase not in all_text, f'{phrase} still present'
for element_id in ('modeD', 'modeE', 'modeF'):
    assert element_id not in files['dashboard_source.html'], f'{element_id} still in dashboard'

# Preserve public/persisted IDs of A/B/C/H while leaving removed gaps unused.
core = files['can_core.h']
assert re.search(r'MODE_A\s*=\s*0', core)
assert re.search(r'MODE_B\s*=\s*1', core)
assert re.search(r'MODE_C\s*=\s*3', core)
assert re.search(r'MODE_H\s*=\s*7', core)

# Old persisted D/E/F IDs must fail safe to Mode A when loaded.
assert 'nagModePersistedIdSupported' in core, 'missing persisted-mode sanitizer helper'

# HTTP mode switching must expose only A/B/C/H presets.
api = files['web_api.h']
for fn in ('nagCfgDefaultsModeD', 'nagCfgDefaultsModeE', 'nagCfgDefaultsModeF'):
    assert fn not in api
assert 'nagCfgDefaultsModeH' in api

print('removed-mode contract OK')
