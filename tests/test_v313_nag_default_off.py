"""Default-off persistence contract; preserve explicitly stored enabled values."""
from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / 'can_core.h').read_text()


def body(name):
    start = SOURCE.index('static void ' + name + '(')
    begin = SOURCE.index('{', start)
    depth = 1
    end = begin + 1
    while depth:
        depth += (SOURCE[end] == '{') - (SOURCE[end] == '}')
        end += 1
    return SOURCE[begin + 1:end - 1]


class NagDefaultOff(unittest.TestCase):
    def test_common_default_is_off(self):
        common = body('nagCfgSetCommonDefaults')
        self.assertRegex(common, r'c\.enabled\s*=\s*false\s*;')

    def test_new_namespace_and_missing_schema_use_common_defaults(self):
        load = body('nagCfgLoad')
        prefix = load[:load.index('const uint8_t nagCfgVersion')]
        self.assertIn('if (!prefs.begin("nag", true))', prefix)
        self.assertIn('if (!prefs.isKey("v"))', prefix)
        self.assertEqual(prefix.count('nagCfgDefaultsModeH(nagCfg);'), 2)
        self.assertIn('nagCfgSetCommonDefaults(c);', body('nagCfgDefaultsModeH'))

    def test_missing_enabled_key_defaults_off(self):
        load = body('nagCfgLoad')
        self.assertRegex(load, r'nagCfg\.enabled\s*=\s*prefs\.getBool\("en",\s*false\);')

    def test_existing_saved_true_and_false_are_read_without_override(self):
        load = body('nagCfgLoad')
        assignments = re.findall(r'nagCfg\.enabled\s*=\s*([^;]+);', load)
        self.assertEqual(assignments, ["savedEnabled", 'prefs.getBool("en", false)'])
        self.assertIn('const bool savedEnabled = prefs.getBool("en", false);', load)

    def test_save_keeps_enabled_snapshot(self):
        self.assertRegex(SOURCE, r'prefs\.putBool\("en",\s*snapshot\.enabled\);')


if __name__ == '__main__':
    unittest.main()
