const assert = require('assert');
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const dashboard = fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8');

function extractFunction(name) {
  const marker = `function ${name}(`;
  const start = dashboard.indexOf(marker);
  assert.notStrictEqual(start, -1, `${name} must exist in dashboard_source.html`);

  const bodyStart = dashboard.indexOf('{', start);
  let depth = 0;
  let quote = null;
  let escaped = false;
  for (let i = bodyStart; i < dashboard.length; i += 1) {
    const ch = dashboard[i];
    if (quote !== null) {
      if (escaped) escaped = false;
      else if (ch === '\\') escaped = true;
      else if (ch === quote) quote = null;
      continue;
    }
    if (ch === '"' || ch === "'" || ch === '`') {
      quote = ch;
      continue;
    }
    if (ch === '{') depth += 1;
    else if (ch === '}') {
      depth -= 1;
      if (depth === 0) return dashboard.slice(start, i + 1);
    }
  }
  throw new Error(`unterminated function ${name}`);
}

const element = {textContent: '', className: ''};
global.$ = (id) => id === 'blinkerState' ? element : null;
eval(extractFunction('blinkerNoaSessionName'));
eval(extractFunction('renderHomeBlinkerState'));

const cases = [
  [{enabled: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 10000}, '10s'],
  [{enabled: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 9999}, '10s'],
  [{enabled: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 9000}, '9s'],
  [{enabled: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 1}, '1s'],
  [{enabled: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 0}, '1s'],
  [{enabled: true, noaSessionStateName: 'READY'}, 'Ready'],
  [{enabled: true, cancelPaused: true, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 8000}, 'Paused'],
  [{enabled: false, noaSessionStateName: 'STABILIZATION', noaStabilizationRemainingMs: 8000}, 'Off'],
];

for (const [state, expected] of cases) {
  element.textContent = '';
  element.className = '';
  renderHomeBlinkerState(state);
  assert.strictEqual(element.textContent, expected, JSON.stringify(state));
  if (state.noaSessionStateName === 'STABILIZATION' && state.enabled && !state.cancelPaused) {
    assert.strictEqual(element.className, 'v warn', 'countdown must use the normal large metric size');
  }
}

console.log(`${cases.length}/${cases.length} HOME Blinker countdown cases PASS`);
