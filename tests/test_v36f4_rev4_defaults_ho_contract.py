from html.parser import HTMLParser
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")


class Rev4HoUiParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.inputs = {}
        self.selects = {}
        self.current_select = None

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        element_id = values.get("id", "")
        if tag == "input" and element_id:
            self.inputs[element_id] = values
        elif tag == "select" and element_id:
            self.current_select = element_id
            self.selects[element_id] = []
        elif tag == "option" and self.current_select:
            self.selects[self.current_select].append(values.get("value"))

    def handle_endtag(self, tag):
        if tag == "select":
            self.current_select = None


parser = Rev4HoUiParser()
parser.feed(DASHBOARD)

assert parser.selects.get("humanV4HoPolicy") == ["0", "1", "2"], (
    "Rev.4 Hands-On Policy selector must expose the same three modes as Rev.3"
)
for element_id in ["humanV4Ho1Nm", "humanV4Ho2Nm"]:
    assert element_id in parser.inputs, f"missing Rev.4 input {element_id}"
    field = parser.inputs[element_id]
    assert field.get("type") == "number"
    assert field.get("min") == "0.1"
    assert field.get("max") == "3"
    assert field.get("step") == "0.05"

for public_name in ["hoPolicy", "ho1ThresholdNm", "ho2ThresholdNm"]:
    assert public_name in API, f"Rev.4 API field missing: {public_name}"
    assert public_name in DASHBOARD, f"Rev.4 dashboard field missing: {public_name}"

assert "hoPolicyName" in API, "Rev.4 API policy label missing"

for key in ["h4hop", "h4ho1", "h4ho2"]:
    assert f'getUChar("{key}"' in CORE or f'getUShort("{key}"' in CORE
    assert f'putUChar("{key}"' in CORE or f'putUShort("{key}"' in CORE

assert 'nagCfgVersion < 18u' in CORE
assert 'prefs.putUChar("v", 20u)' in CORE
assert 'nagHumanV4MigrateV16DefaultPure(rev4Cfg)' in CORE
assert 'nagHumanV4MigrateV17DefaultPure(rev4Cfg)' in CORE

print("PASS v3.6f4 Rev.4 defaults + Hands-On integration contract")
