"""Execute the production Summon Heartbeat HTTP parser and status mapping."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
API = (ROOT / "web_api.h").read_text()
LOGIC = (ROOT / "vehicle_logic.h").read_text()


def extract_function(text, name):
    match = re.search(
        r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", text, re.M
    )
    assert match, name
    end = text.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start() : end] + "\n"


handler = extract_function(API, "httpSummonHeartbeatOverrideUpdate")
code = r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
struct String : std::string {
  using std::string::string;
  int toInt() const { return std::stoi(*this); }
};
struct Server {
  std::map<std::string, String> values;
  int status = 0;
  bool hasArg(const char *key) const { return values.count(key) != 0; }
  String arg(const char *key) { return values[key]; }
  void send(int value, const char *, const String &) { status = value; }
} server;
bool labMenuEnabled = true, supported = true, applyOk = true;
bool publishedEnabled = false;
uint8_t publishedValue = 2;
bool httpBoolArg(const char *key, bool &out) {
  if (!server.hasArg(key)) return false;
  const String value = server.arg(key);
  if (value == "0") { out = false; return true; }
  if (value == "1") { out = true; return true; }
  return false;
}
bool summonHeartbeatOverrideSupported() { return supported; }
bool summonHeartbeatOverrideApply(bool enabled, uint8_t value) {
  if (!applyOk) return false;
  publishedEnabled = enabled;
  publishedValue = value;
  return true;
}
String summonHeartbeatOverrideStatsToJson() { return "{}"; }
'''
code += handler
code += r'''
void request(const std::map<std::string, String> &args, int expected) {
  server.values = args;
  server.status = 0;
  httpSummonHeartbeatOverrideUpdate();
  assert(server.status == expected);
}
int main() {
  for (int value = 0; value <= 3; ++value) {
    const String raw(std::to_string(value).c_str());
    request({{"enabled", "1"}, {"value", raw}}, 200);
    assert(publishedEnabled && publishedValue == value);
  }
  request({{"enabled", "0"}, {"value", "3"}}, 200);
  assert(!publishedEnabled && publishedValue == 3);

  const bool beforeEnabled = publishedEnabled;
  const uint8_t beforeValue = publishedValue;
  for (const char *bad : {"4", "255", "-1", "3x", "3.0", "03", "", " 3", "3 ", "4294967296"}) {
    request({{"enabled", "1"}, {"value", bad}}, 400);
    assert(publishedEnabled == beforeEnabled && publishedValue == beforeValue);
  }
  for (const char *bad : {"2", "true", "false", "", " 1", "1 "}) {
    request({{"enabled", bad}, {"value", "2"}}, 400);
    assert(publishedEnabled == beforeEnabled && publishedValue == beforeValue);
  }
  request({}, 400);
  request({{"enabled", "1"}}, 400);
  request({{"value", "2"}}, 400);

  labMenuEnabled = false;
  request({{"enabled", "1"}, {"value", "2"}}, 409);
  request({{"enabled", "0"}, {"value", "1"}}, 200);
  assert(!publishedEnabled && publishedValue == 1);
  labMenuEnabled = true;
  supported = false;
  request({{"enabled", "1"}, {"value", "2"}}, 409);
  supported = true;
  applyOk = false;
  request({{"enabled", "1"}, {"value", "2"}}, 503);
}
'''

assert 'server.on("/api/lab/summon-heartbeat/stats", HTTP_GET' in API
assert 'server.on("/api/lab/summon-heartbeat/update", HTTP_POST' in API
stats_handler = extract_function(API, "summonHeartbeatOverrideStatsToJson")
assert "stockEpoch == epoch" in stats_handler
assert "canTxEpochSnapshot()" in stats_handler
state_block = LOGIC[
    LOGIC.index("static volatile bool summonHeartbeatOverrideEnabled") :
    LOGIC.index("// Set by the CAN-B task", LOGIC.index("static volatile bool summonHeartbeatOverrideEnabled"))
]
assert "Preferences" not in state_block
assert "summonHeartbeatOverrideEnabled = false" in state_block

with tempfile.TemporaryDirectory(prefix="summon-heartbeat-api-host-") as directory:
    source = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    source.write_text(code)
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(source),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("Summon Heartbeat API: strict 0..3 parser, LAB/topology gates and RAM-only default PASS")
