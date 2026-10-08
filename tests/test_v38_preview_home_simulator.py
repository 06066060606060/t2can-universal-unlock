"""Preview-only AP and vehicle presets must match the production Home snapshot contract."""

from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
MOCK = (ROOT / "tools" / "v38_preview_mock.js").read_text(encoding="utf-8")
CONTROLS = (ROOT / "tools" / "v38_preview_controls.js").read_text(encoding="utf-8")

HARNESS = r"""
const vm = require('vm');
const mock = process.argv[1], controls = process.argv[2];
async function snapshot(query) {
  const location = {search: query, href: 'file:///preview.html' + query};
  const window = {};
  vm.runInNewContext(mock, {window, location, URL, URLSearchParams, Response, Promise});
  const profile = await (await window.fetch('/api/profile/status')).json();
  const data = await (await window.fetch('/api/snapshot?groups=home-fast&slow=1')).json();
  return {profile, data, state: window.__t2PreviewState, window};
}
(async () => {
  const cases = [
    ['?previewAp=OFF&previewModel=1', 1, 1, true, true, 0, 'SUSPENDED'],
    ['?previewAp=AUTOSTEER&previewModel=2&previewTopology=2', 2, 2, true, true, 3, 'ACTIVE'],
    ['?previewAp=NOA&previewModel=3&previewTopology=3', 3, 3, true, false, 5, 'ACTIVE'],
    ['?previewAp=AUTOSTEER&previewModel=4&previewTopology=2&previewTurn=2', 4, 2, true, true, 3, 'ACTIVE'],
    ['?previewAp=NOA&previewModel=5&previewTopology=3', 5, 3, true, false, 5, 'ACTIVE']
  ];
  for (const [query, model, topology, nag, advanced, das, r79] of cases) {
    const s = await snapshot(query);
    if (s.profile.profile !== model || s.profile.topology !== topology) throw Error('profile: ' + query);
    if (s.profile.nagSupported !== nag || s.profile.advancedEapSupported !== advanced) throw Error('capabilities: ' + query);
    if (s.data.fast?.blink?.displayState !== das || s.data.fast?.r79?.txState !== r79) throw Error('AP Home snapshot: ' + query);
    if (!s.data.slow?.blink || !s.data.fast?.cantraffic) throw Error('Home snapshot envelope: ' + query);
  }
  const highland = await snapshot('?previewModel=4&previewTopology=2&previewTurn=2');
  if (highland.profile.turn !== 2 || highland.profile.canA !== 'BODY') throw Error('Highland turn/bus');
  highland.window.__t2PreviewSetAp('OFF');
  const changed = await (await highland.window.fetch('/api/snapshot?groups=home-fast')).json();
  if (changed.fast?.blink?.displayState !== 0 || changed.fast?.r79?.txState !== 'SUSPENDED') throw Error('live AP change');
  const dms = await (await highland.window.fetch('/api/driver-monitoring/config?enabled=1', {method:'POST'})).json();
  if (!dms.enabled || dms.active) throw Error('standalone Driver Monitoring preview');
  const vision = await (await highland.window.fetch('/api/vision-control/update?disabled=1', {method:'POST'})).json();
  if (!vision.requestDisabled || vision.gateOpen || vision.state !== 'WAIT_AP') throw Error('production Visual Speed Control preview');
  const turnWrap = {hidden: false};
  const makeButton = (key, value) => ({dataset: {[key]: String(value)}, setAttribute(name, value) {this[name] = value}});
  const apButtons = ['OFF','AUTOSTEER','NOA'].map(value => makeButton('previewAp', value));
  const modelButtons = [1,2,3,4,5].map(value => makeButton('previewModel', value));
  const topologyButtons = [1,2,3].map(value => makeButton('previewTopology', value));
  const turnButtons = [1,2].map(value => makeButton('previewTurn', value));
  let destination = '';
  const location = {href: 'file:///preview.html?previewAp=NOA&previewModel=4&previewTopology=2&previewTurn=2', assign(url) {destination = url}};
  const document = {getElementById(id) {return id === 'previewTurnWrap' ? turnWrap : null}, querySelectorAll(selector) {
    if (selector.includes('preview-ap')) return apButtons;
    if (selector.includes('preview-model')) return modelButtons;
    if (selector.includes('preview-topology')) return topologyButtons;
    if (selector.includes('preview-turn')) return turnButtons;
    return [];
  }};
  const window = {__t2PreviewState: {model: 4, topology: 2, turn: 2, ap: 'NOA'}, __t2PreviewSetAp(ap) {this.__t2PreviewState.ap = ap}};
  let page = '';
  const history = {replaceState(_state, _unused, url) {location.href = url}};
  vm.runInNewContext(controls, {window, document, location, URL, history, showPage(name) {page = name}});
  if (modelButtons[3]['aria-pressed'] !== 'true' || topologyButtons[1]['aria-pressed'] !== 'true' || turnButtons[1]['aria-pressed'] !== 'true' || turnWrap.hidden) throw Error('controls initialization');
  apButtons[0].onclick();
  if (window.__t2PreviewState.ap !== 'OFF' || page !== 'home' || new URL(location.href).searchParams.get('previewAp') !== 'OFF') throw Error('live AP control');
  topologyButtons[2].onclick();
  if (new URL(destination).searchParams.get('previewTopology') !== '3' || new URL(destination).searchParams.get('previewTurn') !== '0') throw Error('topology navigation');
  modelButtons[0].onclick();
  if (new URL(destination).searchParams.get('previewTopology') !== '1') throw Error('YL topology lock');
  console.log('preview AP/model/topology/turn and Home snapshot: PASS');
})().catch(error => {console.error(error); process.exitCode = 1});
"""


class PreviewHomeSimulatorTest(unittest.TestCase):
    def test_preview_controls_and_home_snapshot(self):
        result = subprocess.run(
            ["node", "-e", HARNESS, MOCK, CONTROLS],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS", result.stdout)


if __name__ == "__main__":
    unittest.main()
