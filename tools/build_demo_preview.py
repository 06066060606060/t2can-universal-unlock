#!/usr/bin/env python3
"""Build an offline dashboard preview from the current production source."""

import argparse
import base64
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    html = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
    for filename, url in (
        ("geist-latin.woff2", "/fonts/geist-lab.woff2"),
        ("geist-mono-latin.woff2", "/fonts/geist-mono-lab.woff2"),
    ):
        data = base64.b64encode((ROOT / "fonts" / filename).read_bytes()).decode("ascii")
        html = html.replace(url, "data:font/woff2;base64," + data)

    mock = (ROOT / "tools" / "v38_preview_mock.js").read_text(encoding="utf-8")
    controls_css = (ROOT / "tools" / "v38_preview_controls.css").read_text(encoding="utf-8")
    controls_html = (ROOT / "tools" / "v38_preview_controls.html").read_text(encoding="utf-8")
    controls_js = (ROOT / "tools" / "v38_preview_controls.js").read_text(encoding="utf-8")
    html = html.replace(
        "</head>",
        '<style id="v38-preview-controls-style">\n' + controls_css + "</style>\n"
        + '<script id="v38-production-preview-api">\n' + mock + "</script>\n</head>",
        1,
    )
    html, body_count = re.subn(r"(<body\b[^>]*>)", lambda match: match.group(1) + "\n" + controls_html, html, count=1)
    if body_count != 1:
        raise RuntimeError("Dashboard body tag not found")
    html = html.replace("</body>", '<script id="v38-preview-controls-script">\n' + controls_js + "</script>\n</body>", 1)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(html, encoding="utf-8")
    print(f"preview={args.output} bytes={args.output.stat().st_size}")


if __name__ == "__main__":
    main()
