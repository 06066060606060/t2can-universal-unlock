from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INO = ROOT / "T2CAN-Universal-v3.28.0-LP_YL.ino"

assert ROOT.name == "T2CAN-Universal-v3.28.0-LP_YL"
assert INO.is_file()
text = INO.read_text()
assert text.startswith("// T2CAN Universal v3.28.0")
assert '#define FW_VERSION "v3.28.0"' in text
assert (ROOT / "CHANGELOG.md").read_text().startswith("# T2CAN Universal v3.28.0\n")
assert (ROOT / "VALIDATION.md").read_text().startswith(
    "# T2CAN Universal v3.28.0 — Validation\n"
)

print("PASS v3.28.0 package identity")
