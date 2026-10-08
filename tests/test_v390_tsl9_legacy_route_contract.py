import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class Tsl9LegacyRouteContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.core = (ROOT / "can_core.h").read_text()
        cls.runtime = (ROOT / "can_runtime.h").read_text()
        cls.api = (ROOT / "web_api.h").read_text()

    def test_route_is_saved_and_reported(self):
        self.assertIn("uint8_t  tsl9LegacyRoute;", self.core)
        self.assertIn('getUChar("tsl9rt"', self.core)
        self.assertIn('putUChar("tsl9rt"', self.core)
        self.assertIn('"tsl9LegacyRoute"', self.api)
        self.assertIn('hasArg("tsl9LegacyRoute")', self.api)

    def test_dispatch_is_exclusive_but_chassis_state_decode_remains(self):
        self.assertIn("nagTsl9Body39BSelected()", self.runtime)
        case_399 = self.runtime.split("case 921:", 1)[1].split("break;", 1)[0]
        self.assertIn("nagProcessTsl9Twai399(f)", case_399)
        self.assertIn("handle921(f.data", case_399)
        self.assertIn("nagTsl9Body39BSelected()", self.core)
        self.assertIn("nagTsl9Chassis399Selected()", self.core)

    def test_route_change_quiesces_old_transport(self):
        update = self.api.split("static void httpNagUpdate()", 1)[1].split(
            "static void httpNagReset()", 1)[0]
        self.assertIn("tsl9InputQuiesceForConfig()", update)
        self.assertIn("nagTsl9RuntimeReset()", update)
        self.assertLess(update.index("tsl9InputQuiesceForConfig()"),
                        update.index("nagCfgCommit(nc)"))


if __name__ == "__main__":
    unittest.main()
