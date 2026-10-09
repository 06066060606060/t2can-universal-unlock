from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INO = next(ROOT.glob("*.ino")).read_text(encoding="utf-8")
CHANGELOG = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
VALIDATION = (ROOT / "VALIDATION.md").read_text(encoding="utf-8")


assert '#define FW_VERSION "v3.28.0"' in INO
assert INO.startswith("// T2CAN Universal v3.28.0")
assert CHANGELOG.startswith("# T2CAN Universal v3.28.0\n")
assert VALIDATION.startswith("# T2CAN Universal v3.28.0 — Validation\n")

print("PASS v3.7.3 firmware identity contract")
