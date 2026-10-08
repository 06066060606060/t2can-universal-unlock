"""Exercise the dashboard theme controller without a browser renderer."""

from pathlib import Path
import re
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
SCRIPTS = re.findall(r"<script(?:\s[^>]*)?>(.*?)</script>", HTML, re.S)
THEME_SCRIPT = next(s for s in SCRIPTS if "t2canDashboardTheme" in s)

HARNESS = r"""
const vm = require('vm');
const source = process.argv[1];
function load(saved, phoneDark) {
  let change;
  const store = new Map(saved ? [['t2canDashboardTheme', saved]] : []);
  const media = {matches: phoneDark, addEventListener(_name, callback) {change = callback}};
  const root = {dataset: {}};
  const meta = {content: ''};
  const select = {value: ''};
  const desc = {textContent: ''};
  const document = {
    documentElement: root,
    querySelector(selector) {return selector === 'meta[name="theme-color"]' ? meta : null},
    getElementById(id) {return id === 'dashboardThemeMode' ? select : id === 'dashboardThemeDesc' ? desc : null}
  };
  const window = {matchMedia() {return media}};
  const localStorage = {getItem(key) {return store.get(key) || null}, setItem(key, value) {store.set(key, value)}};
  vm.runInNewContext(source, {window, document, localStorage});
  return {window, root, meta, select, desc, media, store, change: () => change()};
}
const auto = load(null, false);
if (auto.window.t2Theme.mode !== 'system' || auto.root.dataset.theme !== 'light') throw Error('default mode');
auto.media.matches = true; auto.change();
if (auto.root.dataset.theme !== 'dark' || auto.meta.content !== '#1d1d21') throw Error('follow phone');
auto.window.t2Theme.setMode('light');
if (auto.root.dataset.theme !== 'light' || auto.select.value !== 'light' || auto.store.get('t2canDashboardTheme') !== 'light') throw Error('force light');
auto.media.matches = false; auto.change(); auto.media.matches = true; auto.change();
if (auto.root.dataset.theme !== 'light') throw Error('manual light changed with phone');
auto.window.t2Theme.setMode('dark');
if (auto.root.dataset.theme !== 'dark' || auto.desc.textContent !== 'Always dark') throw Error('force dark');
const restored = load('dark', false);
if (restored.root.dataset.theme !== 'dark' || restored.select.value !== 'dark') throw Error('restore dark');
const invalid = load('invalid', true);
if (invalid.window.t2Theme.mode !== 'system' || invalid.root.dataset.theme !== 'dark') throw Error('invalid saved value');
console.log('theme system/light/dark/persistence: PASS');
"""


class ThemeModeTest(unittest.TestCase):
    def test_system_manual_modes_and_persistence(self):
        result = subprocess.run(
            ["node", "-e", HARNESS, THEME_SCRIPT],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS", result.stdout)


if __name__ == "__main__":
    unittest.main()
