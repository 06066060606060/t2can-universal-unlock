"""Catch missing iOS metadata, invalid PNGs, and stale embedded icon bytes."""
import gzip
from html.parser import HTMLParser
from pathlib import Path
import re
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]


def embedded_bytes(path):
    return bytes(int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", path.read_text()))


class Head(HTMLParser):
    def __init__(self):
        super().__init__()
        self.icons = []
        self.title = None

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "link" and attrs.get("rel") == "apple-touch-icon":
            self.icons.append(attrs)
        if tag == "meta" and attrs.get("name") == "apple-mobile-web-app-title":
            self.title = attrs.get("content")


class HomeScreenIcon(unittest.TestCase):
    def test_source_and_shipped_dashboard_advertise_local_icon(self):
        for html in ((ROOT / "dashboard_source.html").read_text(),
                     gzip.decompress(embedded_bytes(ROOT / "index_html.h")).decode()):
            with self.subTest(shipped=html.startswith("<!DOCTYPE html>\n<html")):
                head = Head()
                head.feed(html)
                self.assertEqual(len(head.icons), 1, "Safari cannot discover the home-screen icon")
                self.assertEqual(head.icons[0].get("href"), "/apple-touch-icon.png")
                self.assertEqual(head.icons[0].get("sizes"), "180x180")
                self.assertEqual(head.title, "Tesla Unlock")

    def test_embedded_icon_is_exact_opaque_180px_png(self):
        asset = ROOT / "assets" / "apple-touch-icon.png"
        header = ROOT / "dashboard_icon.h"
        self.assertTrue(asset.is_file(), "home-screen PNG is missing")
        self.assertTrue(header.is_file(), "embedded PNG is missing")
        png = asset.read_bytes()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        self.assertEqual(struct.unpack(">II", png[16:24]), (180, 180))
        self.assertEqual(png[25], 2, "icon must use opaque RGB")
        self.assertLessEqual(len(png), 40_000)
        self.assertEqual(embedded_bytes(header), png, "firmware icon is stale or truncated")


if __name__ == "__main__":
    unittest.main()
