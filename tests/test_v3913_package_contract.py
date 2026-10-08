from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INO = ROOT / "T2CAN-Universal-v3.26.3-LP_YL.ino"

assert ROOT.name == "T2CAN-Universal-v3.26.3-LP_YL"
assert INO.is_file()
text = INO.read_text()
assert text.startswith("// T2CAN Universal v3.26.3")
assert '#define FW_VERSION "v3.26.3"' in text
assert (ROOT / "CHANGELOG.md").read_text().startswith("# T2CAN Universal v3.26.3\n")
assert (ROOT / "VALIDATION.md").read_text().startswith(
    "# T2CAN Universal v3.26.3 — Validation\n"
)

print("PASS v3.26.3 package identity")
