from pathlib import Path

root = Path(__file__).resolve().parents[1]
dash = (root / 'dashboard_source.html').read_text(errors='ignore')

# b18 root cause: setApDriveRegen lived in the first/global script but called
# applyFeatures()/refreshFeatures(), which are local to the later profile IIFE.
# A successful backend POST therefore fell into the catch path with
# ReferenceError: applyFeatures is not defined and showed the generic failure.
iife_marker = '<script>(() => {'
iife_start = dash.index(iife_marker, dash.index('<script>\nconst $='))
global_script = dash[dash.index('<script>\nconst $='):iife_start]
iife_script = dash[iife_start:dash.index('</script>', iife_start)]

assert 'async function setApDriveRegen' not in global_script, \
    'AP Drive regen handler must not live outside applyFeatures scope'
assert 'function applyFeatures' in iife_script
assert 'async function refreshFeatures' in iife_script
assert 'async function setApDriveRegen' in iife_script
assert "q('apDriveRegenSelect').onchange=e=>setApDriveRegen(e.target.value)" in iife_script
assert 'featurePostJson' in iife_script or 'featurePostJson' in global_script
print('v3.6b19 AP Drive dashboard scope regression passed')
