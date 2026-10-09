from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PKG = ROOT / "T2CAN-Universal-v3.28.0-LP_YL"


assert PKG.is_dir(), "v3.28.0 package must exist"
ino = PKG / "T2CAN-Universal-v3.28.0-LP_YL.ino"
assert ino.is_file(), "v3.28.0 sketch name must match its directory"
text = ino.read_text()
assert text.startswith("// T2CAN Universal v3.28.0")
assert '#define FW_VERSION "v3.28.0"' in text
assert (PKG / "CHANGELOG.md").read_text().startswith("# T2CAN Universal v3.28.0\n")
assert (PKG / "VALIDATION.md").read_text().startswith(
    "# T2CAN Universal v3.28.0 — Validation\n"
)

print("PASS v3.28.0 package identity")
