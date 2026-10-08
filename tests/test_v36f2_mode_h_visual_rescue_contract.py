from html.parser import HTMLParser
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DASHBOARD = (ROOT / "dashboard_source.html").read_text(encoding="utf-8")
CORE = (ROOT / "can_core.h").read_text(encoding="utf-8")
API = (ROOT / "web_api.h").read_text(encoding="utf-8")


class RescueUiParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.variant_buttons = []
        self.inputs = {}
        self.selects = {}

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        element_id = values.get("id", "")
        if tag == "button" and element_id.startswith("modeHRev"):
            self.variant_buttons.append(element_id)
        if tag == "input" and element_id:
            self.inputs[element_id] = values
        if tag == "select" and element_id:
            self.selects[element_id] = values


parser = RescueUiParser()
parser.feed(DASHBOARD)

# The visible selector is numerical even though persisted IDs stay 0/3/1.
assert parser.variant_buttons == []

# Rev.4 exposes a persistent switch and a bounded 0.0..2.0 second delay.
assert "humanV4VisualRescue" in parser.selects
delay = parser.inputs["humanV4RescueDelay"]
assert delay["type"] == "number"
assert delay["min"] == "0"
assert delay["max"] == "2"
assert delay["step"] == "0.1"

# Dashboard request/response fields use the same public API names.
for token in ["visualRescueEnabled", "visualRescueDelayMs"]:
    assert token in DASHBOARD
    assert token in API

# The actual DAS transition timestamp is carried into Rev.4, rather than
# starting the delay on a later stock 0x370 frame.
for token in [
    "visualWarningEpoch",
    "visualWarningEnterMs",
    "visualWarningActive",
]:
    assert token in CORE

assert "nagHumanV4StepPure(nagHumanV4State" in CORE
assert "visualWarningEpoch, visualWarningEnterMs, visualWarningActive" in CORE

# NVS defaults/migration/save must cover both new settings.
for key in ["h4vres", "h4vdly"]:
    assert f'getBool("{key}"' in CORE or f'getUShort("{key}"' in CORE
    assert f'putBool("{key}"' in CORE or f'putUShort("{key}"' in CORE

assert 'prefs.putUChar("v", 20u)' in CORE

print("PASS v3.6f3 Mode H Visual Warning Rescue integration contract")
