from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
VARIANT = (ROOT / "nag_mode_h_variant_pure.h").read_text(encoding="utf-8")
DASH = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
WEB = (ROOT / "web_api.h").read_text(encoding="utf-8")
CHANGELOG = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
VALIDATION = (ROOT / "VALIDATION.md").read_text(encoding="utf-8")


assert INO.startswith("// T2CAN Universal v3.26.3")
assert '#define FW_VERSION "v3.26.3"' in INO
assert CHANGELOG.startswith("# T2CAN Universal v3.26.3\n")
assert VALIDATION.startswith("# T2CAN Universal v3.26.3 — Validation\n")

# Fresh NVS, invalid values, and explicit NAG reset all converge on Mode H Rev.4.
assert "nagModeHDefaultVariantPure" in VARIANT
assert "return H_VARIANT_REV4;" in VARIANT
assert "static volatile uint8_t nagHumanVariant = nagModeHDefaultVariantPure();" in CORE
assert CORE.count("nagCfgDefaultsModeH(nagCfg);") >= 2
assert 'prefs.getUChar("mode", MODE_H)' in CORE
assert 'prefs.getUChar("hv", nagModeHDefaultVariantPure())' in CORE

# Approved production dashboard refinements.
assert '<div class="logo">TESLA UNLOCK</div>' in DASH
assert 'NAG ✓ · ADV EAP ✓ · EU UNLOCK ✓' not in DASH
assert 'function toast(t)' not in DASH
assert 'id="toast"' not in DASH
assert 'liveDetailsCard' not in DASH
assert 'homeLiveDetails' not in DASH
assert '&live=1' not in DASH
assert 'homeLiveSnapshotToJson' not in WEB
assert 'if (groups == "home-lite")' in WEB
assert 'body.t2-2027 .quickMini .toggle{margin-top:15px}' in DASH
assert "+'\\n'+fmtMs(age)" in DASH
assert "white-space:pre-line" in DASH

print("PASS v3.26.3 release identity, Rev.4 defaults, and dashboard contract")
