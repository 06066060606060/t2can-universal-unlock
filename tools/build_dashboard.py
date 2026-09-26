#!/usr/bin/env python3
from pathlib import Path
import gzip
import re

try:
    import zopfli.gzip as zopfli_gzip
except ImportError:
    zopfli_gzip = None

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "dashboard_source.html"
HEADER = ROOT / "index_html.h"


def minify_css(css: str) -> str:
    out = []
    i = 0
    quote = None
    while i < len(css):
        c = css[i]
        if quote is not None:
            out.append(c)
            if c == "\\" and i + 1 < len(css):
                i += 1
                out.append(css[i])
            elif c == quote:
                quote = None
            i += 1
            continue
        if c in ("'", '"'):
            quote = c
            out.append(c)
            i += 1
            continue
        if c == "/" and i + 1 < len(css) and css[i + 1] == "*":
            end = css.find("*/", i + 2)
            i = len(css) if end < 0 else end + 2
            continue
        if c.isspace():
            if out and out[-1] not in " {}:;,":
                out.append(" ")
            i += 1
            while i < len(css) and css[i].isspace():
                i += 1
            continue
        if c in "{}:;,":
            if out and out[-1] == " ":
                out.pop()
            out.append(c)
            i += 1
            while i < len(css) and css[i].isspace():
                i += 1
            continue
        out.append(c)
        i += 1
    return "".join(out).strip()


def minify_dashboard(html: str) -> str:
    parts = []
    last = 0
    for m in re.finditer(r"<style([^>]*)>(.*?)</style>", html, re.S | re.I):
        parts.append(html[last:m.start()])
        parts.append("<style" + m.group(1) + ">" + minify_css(m.group(2)) + "</style>")
        last = m.end()
    parts.append(html[last:])
    html = "".join(parts)

    lines = []
    for line in html.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("<!--") and stripped.endswith("-->"):
            continue
        lines.append(stripped)
    return "\n".join(lines) + "\n"


def render_header(payload: bytes, raw_len: int, minified_len: int) -> str:
    rows = []
    for i in range(0, len(payload), 16):
        rows.append("  " + ", ".join(f"0x{b:02X}" for b in payload[i:i+16]) + ",")
    return (
        "#pragma once\n#include <Arduino.h>\n\n"
        "// Generated from dashboard_source.html. Do not edit this byte array directly.\n"
        f"// Raw HTML: {raw_len} bytes | embedded minified HTML: {minified_len} bytes | gzip: {len(payload)} bytes\n"
        "static const uint8_t INDEX_HTML_GZ[] PROGMEM = {\n"
        + "\n".join(rows)
        + "\n};\n"
        + f"static const size_t INDEX_HTML_GZ_LEN = {len(payload)};\n"
    )


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    embedded = minify_dashboard(source)
    embedded_bytes = embedded.encode("utf-8")
    # Zopfli produces a fully standard deterministic gzip stream but usually
    # saves several KiB versus zlib -9 on this large static dashboard. Keep a
    # stdlib fallback so the source package remains rebuildable everywhere.
    payload = (zopfli_gzip.compress(embedded_bytes) if zopfli_gzip is not None
               else gzip.compress(embedded_bytes, compresslevel=9, mtime=0))
    HEADER.write_text(render_header(payload, len(source.encode("utf-8")), len(embedded_bytes)), encoding="utf-8")
    compressor = "zopfli" if zopfli_gzip is not None else "zlib-9"
    print(f"dashboard source={len(source.encode('utf-8'))} minified={len(embedded_bytes)} gzip={len(payload)} compressor={compressor}")


if __name__ == "__main__":
    main()
