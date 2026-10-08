from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PKG = ROOT / "T2CAN-Universal-v3.26.3-LP_YL"


assert PKG.is_dir(), "v3.26.3 package must exist"
ino = PKG / "T2CAN-Universal-v3.26.3-LP_YL.ino"
assert ino.is_file(), "v3.26.3 sketch name must match its directory"
text = ino.read_text()
assert text.startswith("// T2CAN Universal v3.26.3")
assert '#define FW_VERSION "v3.26.3"' in text
assert (PKG / "CHANGELOG.md").read_text().startswith("# T2CAN Universal v3.26.3\n")
assert (PKG / "VALIDATION.md").read_text().startswith(
    "# T2CAN Universal v3.26.3 — Validation\n"
)

print("PASS v3.26.3 package identity")
