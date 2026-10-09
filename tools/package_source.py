#!/usr/bin/env python3
"""Package only app sources, approved artwork and verification documentation."""
import hashlib
import os
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
output = Path(os.environ.get("T2CAN_OUTPUT", root / "artifacts")) / "Tesla-Unlock-1.2.1-source.zip"
output.parent.mkdir(parents=True, exist_ok=True)
files = [root / name for name in ["AndroidManifest.xml", "README.md", "VALIDATION.md", ".gitignore",
                                "design/tu-icon-concept-v1.png"]]
for folder in ["src", "assets", "res", "tools", "tests", "docs"]:
    files.extend(path for path in (root / folder).rglob("*")
                 if path.is_file() and "__pycache__" not in path.parts)
with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(files):
        archive.write(path, Path("Tesla-Unlock-1.2.1") / path.relative_to(root))
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    assert all("/private/" not in name and "/.toolchain/" not in name for name in archive.namelist())
print(output)
print("Bytes:", output.stat().st_size)
print("SHA256:", hashlib.sha256(output.read_bytes()).hexdigest())
