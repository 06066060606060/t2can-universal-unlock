from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
ino = next(root.glob('*.ino')).read_text(errors='ignore')
core = (root/'can_core.h').read_text(errors='ignore')
api = (root/'web_api.h').read_text(errors='ignore')
dash = (root/'dashboard_source.html').read_text(errors='ignore')
forward = (root/'t2can_forward.h').read_text(errors='ignore')
v2 = (root/'nag_human_v2_pure.h').read_text(errors='ignore')
v3 = (root/'nag_human_v3_pure.h').read_text(errors='ignore')
v4 = (root/'nag_human_v4_pure.h').read_text(errors='ignore')
variants = (root/'nag_mode_h_variant_pure.h').read_text(errors='ignore')

assert '#define FW_VERSION "v3.28.0"' in ino
assert '#include "nag_human_v4_pure.h"' in ino
assert '#include "nag_mode_h_variant_pure.h"' in ino

# One production Mode H with stable persisted slot 1 and existing h4 tuning.
assert 'H_VARIANT_REV4 = 1' in variants
for token in ['H_VARIANT_REV1', 'H_VARIANT_REV2', 'H_VARIANT_REV3']:
    assert token not in variants + core + api, token
assert 'prefs.putUChar("hv"' in core
retired_handler=api.split('static void httpNagSetHumanVariant()',1)[1].split('static void httpNagUpdate()',1)[0]
assert 'server.send(410' in retired_handler and 'Mode H profiles retired' in retired_handler
assert 'nagHumanVariant =' not in retired_handler
for key in ['h4pmin','h4pmax','h4wmin','h4wmax','h4rmin','h4rmax','h4cmin','h4cmax','h4hop','h4ho1','h4ho2','h4vres','h4vdly']:
    assert '"'+key+'"' in core, key
assert 'nagHumanV4RuntimeSetLabTuning' in api and 'nagHumanV4RuntimeConfigSnapshot' in api

# Mode H is one promoted production profile; historical selector is removed.
for token in ['id="modeHVariantWrap"', 'id="modeHRev1"', 'id="modeHRev3"', 'id="modeHRev4"', 'setModeHVariant']:
    assert token not in dash, token
assert 'async function fetchNagHumanLab' in dash

# Production runtime dispatch is exclusively the preserved Mode H engine.
assert 'nagHumanV4StepPure' in core
for retired in ['nagHumanV1StepPure','nagHumanV2StepPure','nagHumanV3StepPure']:
    assert retired not in core, retired
assert 'nagHumanV4DefaultConfigPure' in v4
# b20 Rev.2 is a clean Natural Grip Peak engine; the failed b18 warning-reactive
# branch/presets are intentionally not restored.
for retired in ['NagHumanV2PresetPure', 'PREWARN', 'VISUAL_RETRY', 'warningLeftBiasPct']:
    assert retired not in v2, retired

# b19 production work remains present.
assert 'id="apDriveProfileSettingsRow"' in dash
assert 'body.t2-2027 .quickMini .toggle' in dash

print('PASS v3.6b20 static contract')
