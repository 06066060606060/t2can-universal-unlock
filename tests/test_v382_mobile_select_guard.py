import gzip
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


def embedded_dashboard() -> str:
    header = (ROOT / "index_html.h").read_text(encoding="utf-8")
    match = re.search(r"INDEX_HTML_GZ\[\].*?=\s*\{(.*?)\};", header, re.S)
    if match is None:
        raise AssertionError("embedded dashboard array is missing")
    payload = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))
    return gzip.decompress(payload).decode("utf-8")


class MobileSelectGuardContract(unittest.TestCase):
    def test_source_and_embedded_dashboard_block_native_reopen(self):
        documents = {
            "source": (ROOT / "dashboard_source.html").read_text(encoding="utf-8"),
            "embedded": embedded_dashboard(),
        }
        for name, html in documents.items():
            with self.subTest(document=name):
                self.assertNotIn("previous.focus", html)
                self.assertIn("document.activeElement.blur()", html)
                self.assertNotIn("['pointerdown','mousedown','touchstart','click']", html)
                # Native suppression remains, but opening must follow completed activation.
                self.assertTrue("select.addEventListener('pointerdown',suppress" in html,
                                f"{name}: pointerdown must only suppress native selection")
                self.assertTrue("select.addEventListener('click',activate" in html,
                                f"{name}: completed click must activate the sheet")
                suppress = re.search(r"const suppress=event=>\{([^}]+)\}", html)
                self.assertIsNotNone(suppress, f"{name}: native select guard missing")
                self.assertNotIn("openSelectSheet", suppress.group(1))
                self.assertTrue("select.addEventListener('touchend'" in html,
                                f"{name}: legacy touch needs completed activation")


if __name__ == "__main__":
    unittest.main()
