"""Exercise production NAG snapshot restore at persisted range boundaries."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def extract(source, name):
    match = re.search(r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", source, re.M)
    assert match, name
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


class ScrollRestoreHostTest(unittest.TestCase):
    def test_persisted_scroll_restore_sanitizes_ranges_and_retains_valid_values(self):
        core = (ROOT / "can_core.h").read_text().replace('#include "mode_h_runtime.h"', (ROOT / "mode_h_runtime.h").read_text())
        logic = (ROOT / "vehicle_logic.h").read_text()
        stored = re.search(r"^struct NagScrollStoredSnapshot \{.*?^\};", core, re.M | re.S)
        assert stored
        code = r'''
#include <cassert>
#include <cstdint>
#include "tsl9_input_scheduler_pure.h"
#define portMUX_TYPE int
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
'''
        code += stored.group(0) + "\n"
        start = logic.index("static constexpr uint8_t NAG_RIGHT_SCROLL_PATTERN_FOUR_STEP")
        end = logic.index("static inline uint8_t nagRightScrollPatternSanitize")
        code += logic[start:end] + extract(logic, "nagRightScrollPatternSanitize")
        code += extract(logic, "nagScrollRuntimeRestore")
        code += r'''
int main() {
  // Existing legacy loaders sanitize the same values to 30 seconds / Pair.
  nagScrollRuntimeRestore({true, 0u, 255u, true, 65535u});
  assert(nagTorqueRightScrollEnabled && nagTsl9RightPeriodicEnabled);
  assert(nagTorqueRightScrollIntervalSeconds == 30u);
  assert(nagTsl9RightPeriodicIntervalSeconds == 30u);
  assert(nagTorqueRightScrollPattern == NAG_RIGHT_SCROLL_PATTERN_PAIR);
  nagScrollRuntimeRestore({false, 1u, 0u, false, 600u});
  assert(!nagTorqueRightScrollEnabled && !nagTsl9RightPeriodicEnabled);
  assert(nagTorqueRightScrollIntervalSeconds == 1u);
  assert(nagTsl9RightPeriodicIntervalSeconds == 600u);
  assert(nagTorqueRightScrollPattern == NAG_RIGHT_SCROLL_PATTERN_FOUR_STEP);
}
'''
        with tempfile.TemporaryDirectory(prefix="v3282-scroll-restore-") as scratch:
            source = Path(scratch) / "probe.cpp"
            binary = Path(scratch) / "probe"
            source.write_text(code)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", "-Wno-unused-variable", "-I", str(ROOT),
                            str(source), "-o", str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
