from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
web=(ROOT/"web_api.h").read_text()
ino=next(ROOT.glob("*.ino")).read_text()
assert '#define FW_VERSION "v3.28.0"' in ino
const=web.index('static constexpr uint32_t CAN_TRAFFIC_UI_FRESH_MS = 1500;')
struct=web.index('struct CanTrafficUiSnapshot')
helper=web.index('static CanTrafficUiSnapshot canTrafficUiSnapshot()')
first_use=web.index('static String canTrafficStatsToJson()')
assert const < struct < helper < first_use
for fn in ('canTrafficStatsToJson','systemStatsToJson','homeFastSnapshotToJson','homeSnapshotToJson'):
    pos=web.index('static String '+fn+'()')
    assert struct < pos and helper < pos
assert web.count('struct CanTrafficUiSnapshot') == 1
assert web.count('CAN_TRAFFIC_UI_FRESH_MS = 1500') == 1
print('v3.6d9a2 CAN traffic UI snapshot compile-order contract: PASS')
