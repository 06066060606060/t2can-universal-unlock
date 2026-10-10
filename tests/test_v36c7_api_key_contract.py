from pathlib import Path
import json, re
ROOT=Path(__file__).resolve().parents[1]
expected=json.loads((ROOT/'tests/c7_api_keys_expected.json').read_text())

# Deliberate v3.7 R79 retirement allowlist. Any future key removal still fails
# unless it is explicitly reviewed and added here.
RETIRED_R79_KEYS={
    'strategy','periodMs','refreshMode','schedulerMode','periodicShots',
    'postMux1Mode','quietAnchor','postMux2Enabled','postMux2OffsetMs',
    'preMux1Enabled','preMux1OffsetMs','mux0DelayMs','roamingMirrorRatio',
    'roamingBurst','roamingSilenceMs','timingCapture',
}
assert RETIRED_R79_KEYS.isdisjoint(expected['r79StatsToJson']['keys'])

# v3.13 promotes one Mode H engine. Only fields belonging to retired
# revision-specific tuning/telemetry may disappear from this endpoint.
RETIRED_KEYS_BY_FUNCTION={
    'nagHumanProfileStatsToJson': {
        'carrierDirectionMode','carrierDirectionName','durationMaxMs',
        'durationMinMs','excursionMaxNm','hoOverridePct','holdMaxNm',
        'holdMinNm','intervalMaxMs','intervalMinMs',
    },
}
for function,keys in RETIRED_KEYS_BY_FUNCTION.items():
    assert keys <= set(expected[function]['keys']), function

def extract(src,fn,return_type='String'):
    m=re.search(rf'static {return_type} {re.escape(fn)}\([^)]*\)\s*\{{',src,re.S)
    assert m, fn
    i=m.end(); depth=1; j=i; in_s=False; esc=False
    while j < len(src) and depth:
        c=src[j]
        if in_s:
            if esc: esc=False
            elif c=='\\': esc=True
            elif c=='"': in_s=False
        else:
            if c=='"': in_s=True
            elif c=='{': depth+=1
            elif c=='}': depth-=1
        j += 1
    return src[m.start():j]

for fn,meta in expected.items():
    src=(ROOT/meta['path']).read_text()
    body=extract(src,fn)
    for helper in re.findall(r'\b(write[A-Za-z0-9_]+Json)\(jw\b', body):
        body += extract(src,helper,'void')
    literal_keys=set(re.findall(r'\\"([A-Za-z0-9_]+)\\":', body))
    writer_keys=set(re.findall(r'\b\w+\.(?:boolean|u32|i32|fixed|string|raw|beginObject|beginArray)\(\"([A-Za-z0-9_]+)\"', body))
    current=literal_keys | writer_keys
    retired=RETIRED_KEYS_BY_FUNCTION.get(fn,set())
    assert retired.isdisjoint(current), f'{fn} retirement allowlist includes live keys: {sorted(retired & current)}'
    missing=sorted(set(meta['keys'])-current-retired)
    assert not missing, f'{fn} lost API keys: {missing}'
print('v3.6d9a2 API key preservation contract: PASS')
