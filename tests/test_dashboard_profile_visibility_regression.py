from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / 'dashboard_source.html').read_text()

# Profile visibility must remain authoritative in the current dashboard skin.
assert '.hiddenByProfile{display:none!important}' in DASH, \
    'global profile-hidden contract must remain authoritative'

# Layout rules must not force display with !important, or they can override
# the profile-hidden contract through selector specificity.
quick_blocks = re.findall(r'body\.t2-2027 \.quickMini\{([^}]*)\}', DASH)
nav_blocks = re.findall(r'body\.t2-2027 \.navbtn\{([^}]*)\}', DASH)
assert quick_blocks and any('display:flex' in b for b in quick_blocks)
assert nav_blocks and any('display:grid' in b for b in nav_blocks)
# Any !important display declaration on the profile-hideable tile/button itself
# can outrank .hiddenByProfile because the mobile selector is more specific.
display_important = re.compile(r'display\s*:\s*[^;!}]+\s*!important', re.I)
assert all(not display_important.search(b) for b in quick_blocks), \
    'mobile quick controls must not force any display value with !important'
assert all(not display_important.search(b) for b in nav_blocks), \
    'mobile nav buttons must not force any display value with !important'
assert '#homeQuickStrip .quickMini.hiddenByProfile' not in DASH, \
    'selector-specific Quick Control hide workaround should not return'
assert '#bottomNav .navbtn.hiddenByProfile' not in DASH, \
    'selector-specific nav hide workaround should not return'

# The floating nav uses flex sizing and hidden profile entries are removed.
assert 'body.t2-2027 .bottomnav.labHidden,body.t2-2027 .bottomnav.devicesHidden' in DASH
assert 'body.t2-2027 .hiddenByProfile' in DASH

# Empty S3XY registry remains a normal user-facing state.
assert "if(!devs.length){line.textContent='Ready to add an S3XY Button'" in DASH
assert 'No paired S3XY buttons.<br>Tap + Add Button to get started.' in DASH
assert 'Bluetooth is off.<br>Saved button settings remain preserved.' in DASH

print('dashboard profile/BLE visibility regression checks passed')
