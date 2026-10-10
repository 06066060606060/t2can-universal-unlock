import gzip
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
BUILD_DASHBOARD_SPEC = importlib.util.spec_from_file_location(
    "build_dashboard", ROOT / "tools" / "build_dashboard.py"
)
BUILD_DASHBOARD = importlib.util.module_from_spec(BUILD_DASHBOARD_SPEC)
BUILD_DASHBOARD_SPEC.loader.exec_module(BUILD_DASHBOARD)


class BinaryOptimizationContract(unittest.TestCase):
    def test_build_flags_reject_exception_code(self):
        flags = ROOT / "build_opt.h"
        self.assertTrue(flags.exists(), "Arduino build_opt.h is missing")
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "exception_probe.cpp"
            output = Path(temp_dir) / "exception_probe.o"
            source.write_text(
                "int probe() { try { throw 1; } catch (...) { return 1; } }\n",
                encoding="utf-8",
            )
            result = subprocess.run(
                [os.environ.get("CXX", "g++"), f"@{flags}", "-c", str(source), "-o", str(output)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertNotEqual(result.returncode, 0, "exception syntax unexpectedly compiled")
        diagnostic = (result.stdout + result.stderr).lower()
        self.assertTrue("exceptions disabled" in diagnostic or "exception handling disabled" in diagnostic)

    def test_embedded_dashboard_budget_is_strictly_below_100000_bytes(self):
        self.assertEqual(BUILD_DASHBOARD.MAX_EMBEDDED_GZIP_BYTES, 100_000)
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "dashboard_source.html"
            header = Path(temp_dir) / "index_html.h"
            source.write_text("<!doctype html><title>TESLA UNLOCK</title>", encoding="utf-8")
            exact_limit_compressor = SimpleNamespace(compress=lambda _payload: b"x" * 100_000)
            with (
                mock.patch.object(BUILD_DASHBOARD, "SOURCE", source),
                mock.patch.object(BUILD_DASHBOARD, "HEADER", header),
                mock.patch.object(BUILD_DASHBOARD, "zopfli_gzip", exact_limit_compressor),
            ):
                with self.assertRaises(SystemExit):
                    BUILD_DASHBOARD.main()

    def test_embedded_dashboard_accepts_99999_bytes(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "dashboard_source.html"
            header = Path(temp_dir) / "index_html.h"
            source.write_text("<!doctype html><title>TESLA UNLOCK</title>", encoding="utf-8")
            below_limit_compressor = SimpleNamespace(compress=lambda _payload: b"x" * 99_999)
            with (
                mock.patch.object(BUILD_DASHBOARD, "SOURCE", source),
                mock.patch.object(BUILD_DASHBOARD, "HEADER", header),
                mock.patch.object(BUILD_DASHBOARD, "zopfli_gzip", below_limit_compressor),
            ):
                BUILD_DASHBOARD.main()
            self.assertTrue(header.exists())

    def test_embedded_dashboard_gzip_stays_within_build_budget(self):
        header = (ROOT / "index_html.h").read_text(encoding="utf-8")
        match = re.search(r"INDEX_HTML_GZ\[\].*?=\s*\{(.*?)\};", header, re.S)
        self.assertIsNotNone(match, "embedded dashboard array is missing")
        payload = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))
        self.assertLess(len(payload), BUILD_DASHBOARD.MAX_EMBEDDED_GZIP_BYTES)
        html = gzip.decompress(payload)
        self.assertIn(b"TESLA UNLOCK", html)

    def test_embedded_lab_fonts_stay_under_35000_bytes(self):
        header = (ROOT / "lab_fonts.h").read_text(encoding="utf-8")
        payloads = []
        for name in ("LAB_GEIST_FONT", "LAB_GEIST_MONO_FONT"):
            match = re.search(rf"{name}\[\].*?=\s*\{{(.*?)\}};", header, re.S)
            self.assertIsNotNone(match, f"{name} array is missing")
            payload = bytes(int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", match.group(1)))
            self.assertEqual(payload[:4], b"wOF2", f"{name} is not a WOFF2 font")
            payloads.append(payload)
        self.assertLessEqual(sum(map(len, payloads)), 35_000)


if __name__ == "__main__":
    unittest.main()
