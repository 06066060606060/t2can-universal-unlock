from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
html = (root / 'dashboard_source.html').read_text(encoding='utf-8')
match = re.search(r'function researchCsvIsDataRow\(line\)\{[^}]+\}', html)
assert match, 'researchCsvIsDataRow helper missing'
fn = match.group(0)
js = fn + r'''
const cases = [
  ['', false],
  ['#segment_summary,segment_id=1', false],
  ['segment_id,label_slot,label,phase', false],
  ['1,A,"LEFT",RAW_PRE,-200,1000,800,0,PARTY,RX,-1,0x239,8,"00"', true],
];
for (const [line, expected] of cases) {
  const actual = researchCsvIsDataRow(line);
  if (actual !== expected) throw new Error(`row classification failed for ${line}: ${actual} != ${expected}`);
}
'''
r = subprocess.run(['node', '-e', js], capture_output=True, text=True)
assert r.returncode == 0, r.stderr
print('research CSV progress row classifier OK')
