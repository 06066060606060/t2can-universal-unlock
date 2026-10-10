from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
web=(ROOT/'web_api.h').read_text()
assert 'struct SnapshotGroupSpec' in web
assert 'SNAPSHOT_GROUP_SPECS' in web
for name in ('nag','blink','das','summon','s3xy','system','cantraffic','ulc','r79','researchcapture'):
    assert f'{{"{name}",' in web, name
assert 'for (size_t i = 0; i < SNAPSHOT_GROUP_SPEC_COUNT; ++i)' in web
print('v3.6d9a2 snapshot table contract: PASS')
