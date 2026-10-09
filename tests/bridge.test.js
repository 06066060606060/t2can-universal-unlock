const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const { Blob } = require('node:buffer');
const script = fs.readFileSync(path.join(__dirname, '../assets/dashboard-bridge.js'), 'utf8');

async function run() {
  let page = 'home', panel = false, modal = false, sheet = false, selectSheet = false;
  const listeners = {}, alerts = [], chunks = [], messages = [], connectionStates = [];
  const context = {
    Blob, Uint8Array, Promise, String, JSON, Error,
    btoa: s => Buffer.from(s, 'binary').toString('base64'),
    alert: s => alerts.push(s),
    otaUploading: false,
    localStorage: { getItem: () => 'system' },
    setConn: good => connectionStates.push(good),
    showPage: name => { page = name; },
    addEventListener: (name, fn) => { listeners[name] = fn; },
    document: {
      documentElement: { dataset: {} },
      querySelector: selector => {
        if (selector === '.page.active') return { dataset: { page } };
        if (selector === '.panel.show .backbtn') return panel ? { click: () => { panel = false; } } : null;
        if (selector === '.warningModal.show') return modal ? {} : null;
        if (selector === '#sheetback.show') return sheet ? { click: () => { sheet = false; } } : null;
        if (selector === '#selectSheetBack.show') return selectSheet ? { click: () => { selectSheet = false; } } : null;
        return null;
      }
    },
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(script, context);
  assert.equal(typeof context.__tuNativeBusy, 'function', 'native route changes need a side-effect-free busy guard');
  assert.equal(context.__tuNativeBusy(),false);
  const installedBack = context.__tuNativeBack;
  vm.runInContext(script, context);
  assert.equal(context.__tuNativeBack, installedBack, 'installation is idempotent');
  assert.equal(context.__tuNativeBack(), 'exit');
  context.showPage('settings'); context.showPage('lab'); panel = true;
  assert.equal(context.__tuNativeBack(), 'handled'); assert.equal(panel, false);
  assert.equal(context.__tuNativeBack(), 'handled'); assert.equal(page, 'settings');
  assert.equal(context.__tuNativeBack(), 'handled'); assert.equal(page, 'home');
  context.otaUploading = true; assert.equal(context.__tuNativeBack(), 'busy'); assert.equal(context.__tuNativeBusy(),true);
  context.otaUploading = false;
  modal = true; assert.equal(context.__tuNativeBack(), 'modal'); modal = false;
  sheet = true; assert.equal(context.__tuNativeBack(), 'handled'); assert.equal(sheet, false);
  selectSheet = true; assert.equal(context.__tuNativeBack(), 'handled'); assert.equal(selectSheet, false);
  context.__tuNativeTheme(true); assert.equal(context.document.documentElement.dataset.theme, 'dark');
  context.localStorage.getItem = () => 'light';
  context.__tuNativeTheme(false); assert.equal(context.document.documentElement.dataset.theme, 'dark', 'explicit theme left to dashboard');
  context.__tuNativeConnectivity(false);
  assert.equal(connectionStates.at(-1), false, 'native network loss marks dashboard disconnected');
  context.setConn(true);
  assert.equal(connectionStates.at(-1), false, 'successful poll cannot override native network loss');
  context.__tuNativeConnectivity(true);
  context.setConn(true);
  assert.equal(connectionStates.at(-1), true, 'successful poll restores connected after native recovery');
  let fail = false;
  const port = {
    start() {}, close() {},
    postMessage(text) {
      const message = JSON.parse(text); messages.push(message);
      if (message.type === 'chunk') chunks.push(Buffer.from(message.data, 'base64'));
      if (message.type !== 'abort') queueMicrotask(() => this.onmessage({ data: JSON.stringify(
        fail ? { ok: false, error: 'Save cancelled' } : { ok: true }) }));
    }
  };
  listeners.message({ data: 'T2CAN_NATIVE_PORT', ports: [port] });
  const original = Buffer.alloc(150123); for (let i = 0; i < original.length; i++) original[i] = i % 251;
  const save = context.saveDownloadedBlob(new Blob([original]), 'test.csv');
  assert.equal(context.__tuNativeBack(), 'busy');
  assert.equal(context.__tuNativeBusy(),true,'route changes blocked during Blob save');
  await save;
  assert.equal(context.__tuNativeBusy(),false);
  assert.deepEqual(Buffer.concat(chunks), original, 'multi-chunk binary round trip');
  assert.equal(messages.filter(x => x.type === 'chunk').length, 4);
  assert.equal(messages.at(-1).type, 'end');
  assert.equal(alerts.length, 0);
  fail = true;
  await context.saveDownloadedBlob(new Blob(['cancelled']), 'cancel.csv');
  assert.equal(alerts.length, 0, 'user cancellation is quiet');
  assert.equal(messages.at(-1).type, 'abort');
  assert.equal(context.__tuNativeBack(), 'exit');
  console.log('Dashboard bridge: navigation, OTA guard, theme, blob chunking, cancellation PASS');
}
run().catch(error => { console.error(error); process.exitCode = 1; });
