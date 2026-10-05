#!/usr/bin/env python3
"""Update embedded profile/Confirm-Free UI; requires upstream's zopfli compressor."""
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
# Keep unrelated upstream embedded markup intact; update only our feature card
# and its small additions to the existing main script. Re-running is idempotent.
card = r'<section class="card" id="labUlcNoConfirmRow">.*?</section>'
embedded = re.sub(card, lambda _: re.search(card, source, re.S)[0], embedded, flags=re.S)
toggle = next(line.strip() for line in source.splitlines()
              if line.strip().startswith("if($('labUlcNoConfirmToggle')){"))
render = "\n".join(line.strip() for line in source.splitlines()
                   if line.strip().startswith(("for(const id of ['confirmCountryRow'", "if($('confirmCountry')){")))
if "for(const id of ['confirmCountryRow'" in embedded:
    embedded = re.sub(r"for\(const id of \['confirmCountryRow'[^\n]+\nif\(\$\('confirmCountry'\)\)\{[^\n]+",
                      lambda _: render, embedded)
else:
    assert embedded.count(toggle) == 1
    embedded = embedded.replace(toggle, toggle + "\n" + render)
country_pattern = r'let confirmCountrySaving=false;\nasync function updateConfirmCountry[^\n]+'
country_js = re.search(country_pattern, source)[0]
if "let confirmCountrySaving=false;" in embedded:
    embedded = re.sub(country_pattern, lambda _: country_js, embedded)
else:
    anchor = re.search(r'async function updateUlcNoConfirmTiming[^\n]+', embedded)[0]
    embedded = embedded.replace(anchor, anchor + "\n" + country_js)
handler = "if($('confirmCountry'))$('confirmCountry').onchange=updateConfirmCountry;"
if handler not in embedded:
    anchor = "if($('labUlcNoConfirmToggle'))$('labUlcNoConfirmToggle').onchange=e=>toggleUlcNoConfirm(e.target.checked);"
    assert embedded.count(anchor) == 1
    embedded = embedded.replace(anchor, anchor + handler)
(root / "docs/embedded-dashboard.html").write_text(embedded)
data = zopfli.gzip.compress(embedded.encode())
assert len(data) < 84000
text = "#pragma once\n#include <Arduino.h>\n\n"
text += "// Upstream dashboard; profile/Confirm-Free UI updated by tools/update_profile_ui.py.\n"
text += f"// Embedded HTML: {len(embedded.encode())} bytes | gzip: {len(data)} bytes\n"
text += "static const uint8_t INDEX_HTML_GZ[] PROGMEM = {\n"
text += "".join("  " + ", ".join(f"0x{b:02X}" for b in data[i:i+16]) + ",\n"
                for i in range(0, len(data), 16))
text += "};\nstatic const size_t INDEX_HTML_GZ_LEN = sizeof(INDEX_HTML_GZ);\n"
header.write_text(text)
