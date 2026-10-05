#!/usr/bin/env python3
"""Update the embedded profile script; requires zopfli (upstream's compressor)."""
import gzip
import re
import zopfli.gzip
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = root / "index_html.h"
raw = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", header.read_text()))
embedded = gzip.decompress(raw).decode()
source = (root / "docs/index.html").read_text()
pattern = r"<script(?:\s[^>]*)?>(.*?)</script>"
def profile_script(html):
    return next(s for s in re.findall(pattern, html, re.S) if "const PROFILES={" in s)

script = "\n".join(line.strip() for line in profile_script(source).splitlines())
embedded = embedded.replace(profile_script(embedded), script)
(root / "docs/embedded-dashboard.html").write_text(embedded)
data = zopfli.gzip.compress(embedded.encode())
assert len(data) < 84000
text = "#pragma once\n#include <Arduino.h>\n\n"
text += "// Upstream dashboard; profile script updated by tools/update_profile_ui.py.\n"
text += f"// Embedded HTML: {len(embedded.encode())} bytes | gzip: {len(data)} bytes\n"
text += "static const uint8_t INDEX_HTML_GZ[] PROGMEM = {\n"
text += "".join("  " + ", ".join(f"0x{b:02X}" for b in data[i:i+16]) + ",\n"
                for i in range(0, len(data), 16))
text += "};\nstatic const size_t INDEX_HTML_GZ_LEN = sizeof(INDEX_HTML_GZ);\n"
header.write_text(text)
