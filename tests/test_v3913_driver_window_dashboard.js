const assert = require('assert');
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8');

assert(html.includes('id="driverWindowLabRow"'));
assert(html.includes('data-panel="panelLabDriverWindow"'));
assert(html.includes('id="panelLabDriverWindow"'));
assert(html.includes('id="driverWindowOpenButton"'));
assert(html.includes('OPEN DRIVER WINDOW · AUTO DOWN'));
assert(html.includes('0x3C2 MUX0 · byte 6 bits [3:2]'));

assert(html.includes("async function fetchDriverWindowLab("));
assert(html.includes("jget('/api/lab/driver-window/stats')"));
assert(html.includes("post('/api/lab/driver-window/open')"));
assert(html.includes('let driverWindowOpening=false'));
assert(html.includes('if(driverWindowOpening)return'));
assert(html.includes('driverWindowOpening=true'));
assert(html.includes('driverWindowOpening=false'));
assert(html.includes("confirm('Open the driver window with a two-frame Auto Down test pulse?')"));
assert(html.includes("driverWindowOpenButton').disabled=driverWindowOpening||!s.available"));
assert(html.includes("p==='panelLabDriverWindow'"));

const row = html.indexOf('id="driverWindowLabRow"');
const panel = html.indexOf('id="panelLabDriverWindow"');
assert(row >= 0 && panel > row, 'LAB row must open a later detail panel');

console.log('PASS v3.28.0 driver-window LAB dashboard contract');
