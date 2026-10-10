from pathlib import Path
import re


root = Path(__file__).resolve().parents[1]
dashboard = (root / "dashboard_source.html").read_text()

assert "profileCompact" in dashboard
assert dashboard.count('class="profileCompactMain"') == 1
assert dashboard.count('class="profileChips"') == 1
assert 'class="profileChangeInline" id="changeProfileBtn"' in dashboard
assert "activeProfileCaps" in dashboard
assert "activeProfileTarget" not in dashboard
assert ".profileCell" not in dashboard
assert "profileGrid" not in dashboard

mobile_rules = re.findall(r"body\.tu-mobile-only \.profile[^\{]*\{([^}]*)\}", dashboard)
assert all("grid-template-columns" not in rule for rule in mobile_rules)
assert all("margin:" not in rule or "profileCompact" in rule for rule in mobile_rules)
print("compact firmware profile contract: PASS")
