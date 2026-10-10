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
        self.assertIn('if (!reader.begin("nag", true))', prefix)
        self.assertIn('if (!reader.isKey("v"))', prefix)
        self.assertIn('nagCfgLoadSafeDefaults();', prefix)
        self.assertIn('nagCfgDefaultsModeH(nagCfg);', prefix)
        self.assertIn('nagCfgSetCommonDefaults(c);', body('nagCfgDefaultsModeH'))

    def test_missing_enabled_key_defaults_off(self):
        load = body('nagCfgLoad')
        self.assertRegex(load, r'nagCfg\.enabled\s*=\s*reader\.getBool\("en",\s*false\);')

    def test_existing_saved_true_and_false_are_read_without_override(self):
        load = body('nagCfgLoad')
        assignments = re.findall(r'nagCfg\.enabled\s*=\s*([^;]+);', load)
        self.assertEqual(assignments, ["savedEnabled", 'reader.getBool("en", false)'])
        self.assertIn('const bool savedEnabled = reader.getBool("en", false);', load)

    def test_save_keeps_enabled_snapshot(self):
        self.assertIn('stored.config = config;', SOURCE)
        self.assertIn('nvsCheckedWriteBlob("nag", "snapshot", &stored, sizeof(stored))', SOURCE)


if __name__ == '__main__':
    unittest.main()
