const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const { launchBrowser } = require('./browser_test_runtime');
const root = path.resolve(__dirname, '..');
const fontHeader = fs.readFileSync(path.join(root, 'lab_fonts.h'), 'utf8');
const fontBytes = symbol => Buffer.from(fontHeader.match(new RegExp(`${symbol}\\[\\][^{]*\\{([\\s\\S]*?)\\};`))[1].match(/0x[\da-f]+/gi).map(x => parseInt(x, 16)));
const fonts = {
  '/fonts/geist-lab.woff2': fontBytes('LAB_GEIST_FONT'),
  '/fonts/geist-mono-lab.woff2': fontBytes('LAB_GEIST_MONO_FONT'),
};

// Detect a dropped KOREA selection, a YL-only UI gate, incorrect bus labels,
// stale stock-monitor values, and rejection paths that re-enable unsupported TX.
async function check(html, label) {
  // Darwin's single-process Chromium exits when its sole context is closed.
  // Give each artifact its own browser lifetime instead of reusing that process.
  const browser = await launchBrowser();
  const page = await browser.newPage({ viewport: { width: 320, height: 844 } });
  let stats = { countrySupported: true, countryMode: 0, countryGateOpen: false };
  let reject = false;
  const writes = [];
  await page.route('http://country.test/**', async route => {
    const u = new URL(route.request().url());
    if (u.pathname === '/') return route.fulfill({ contentType: 'text/html', body: html });
    if (fonts[u.pathname]) return route.fulfill({ contentType: 'font/woff2', body: fonts[u.pathname] });
    let body = {};
    let status = 200;
    if (u.pathname === '/api/country/update') {
      writes.push({ method: route.request().method(), mode: u.searchParams.get('countryMode') });
      if (reject) { status = 409; body = { error: 'unsupported' }; }
      else { stats = { ...stats, countryMode: Number(u.searchParams.get('countryMode')) }; body = stats; }
    } else if (u.pathname === '/api/country/stats') body = stats;
    await route.fulfill({ status, contentType: 'application/json', body: JSON.stringify(body) });
  });
  try {
    await page.goto('http://country.test/', { waitUntil: 'domcontentloaded' });
    assert.equal(await page.locator('#countryOverrideMode').count(), 1, `${label}: country control missing`);
    assert.deepEqual(await page.locator('#countryOverrideMode option').evaluateAll(es => es.map(e => [e.value, e.textContent])), [['0', 'STOCK'], ['1', 'US'], ['2', 'KOREA'], ['3', 'NEW ZEALAND']]);
    for (const profile of [1, 2, 3, 4, 5]) {
      await page.evaluate(p => { window.__t2ProfileId = p; renderCountryOverrideLab({ countrySupported: true, countryMode: 2, countryGateOpen: true }); }, profile);
      assert.equal(await page.locator('#countryOverrideMode').isDisabled(), false);
      assert.equal(await page.locator('#countryOverrideSettingsRow').evaluate(e => e.classList.contains('hiddenByProfile')), false);
      assert.equal(await page.locator('#countryOverrideRowState').textContent(), 'KOREA / STOCK');
      assert.equal(await page.locator('#countryOverrideGate').textContent(), 'OPEN · R79 ACTIVE');
    }
    for (const [mode, text] of [[0, 'STOCK'], [1, 'US'], [2, 'KOREA'], [3, 'NEW ZEALAND']]) {
      await page.evaluate(async m => { document.querySelector('#countryOverrideMode').value = String(m); await updateCountryOverrideMode(); }, mode);
      assert.equal(await page.locator('#countryOverrideRowState').textContent(), (text==='NEW ZEALAND'?'NZ':text)+' / STOCK');
    }
    assert.deepEqual(writes, [{method: 'POST', mode: '0'}, {method: 'POST', mode: '1'}, {method: 'POST', mode: '2'}, {method: 'POST', mode: '3'}]);
    await page.evaluate(async () => { document.querySelector('#countryOverrideMode').value = '0'; await fetchCountryOverrideLab(); });
    assert.equal(await page.locator('#countryOverrideMode').inputValue(), '3', 'saved NZ selection must restore from server');
    await page.evaluate(() => renderCountryOverrideLab({ countrySupported: true, countryMode: 2, countryGateOpen: false, r79StockValid: true, r79StockBit19: 1, r79StockBit47: 0, r79StockAgeMs: 125, r79StockRx: 42, r79StockRaw: '01 02 03 04 05 06 07 08', countryLastValid: true, countryLastBus: 2, countryLastId: 568, countryLastPage: -1, countryLastResult: 'OK', countryLastAgeMs: 5 }));
    assert.equal(await page.locator('#countryOverrideGate').textContent(), 'BLOCKED');
    assert.equal(await page.locator('#countryR79StockBit19').textContent(), '1');
    assert.equal(await page.locator('#countryR79StockBit47').textContent(), '0');
    assert.match(await page.locator('#countryR79StockMeta').textContent(), /VALID.*RX 42/);
    assert.match(await page.locator('#countryOverrideLast').textContent(), /CAN B.*0x238/);
    assert.doesNotMatch(await page.locator('#countryOverrideLast').textContent(), /VH/);
    for (const [bus, canA, canB, expected] of [[1, 'BODY', 'CHASSIS', 'CAN A · BODY'], [2, 'PARTY', 'CHASSIS', 'CAN B · CHASSIS'], [2, 'PARTY', 'VH', 'CAN B · VH']]) {
      await page.evaluate(s => renderCountryOverrideLab(s), { countrySupported: true, countryMode: 1, countryLastValid: true, countryLastBus: bus, countryCanAName: canA, countryCanBName: canB, countryLastId: 2047, countryLastPage: 3 });
      assert.ok((await page.locator('#countryOverrideLast').textContent()).includes(expected));
    }
    stats = { countrySupported: false, countryMode: 0, r79StockValid: false };
    reject = true;
    await page.evaluate(async () => { document.querySelector('#countryOverrideMode').value = '2'; await updateCountryOverrideMode(); });
    assert.equal(await page.locator('#countryOverrideMode').isDisabled(), true, 'rejection refresh must retain authoritative unsupported state');
    assert.equal(await page.locator('#countryOverrideMode').inputValue(), '0');
    assert.equal(await page.locator('#countryR79StockBit19').textContent(), '—');
    assert.equal(await page.locator('#countryR79StockBit47').textContent(), '—');
    assert.equal(await page.locator('#countryR79StockRaw').textContent(), '—');
    stats={countrySupported:true,countryMode:3,countryGateOpen:true};
    await page.evaluate(() => { labEnabled=false;document.body.classList.remove('setup-active');openPanel('panelCountry'); });
    assert.equal(await page.locator('#panelCountry').isVisible(), true);
    await page.evaluate(() => renderCountryOverrideLab({countrySupported:true,countryMode:3,countryGateOpen:true}));
    assert.match(await page.locator('#panelCountry').textContent(), /554.*NZ/);
    for (const width of [320, 390, 430]) {
      await page.setViewportSize({ width, height: 844 });
      for (const theme of ['light','dark']) {
      await page.evaluate(t => {window.t2Theme.setMode(t);},theme);
      await page.waitForTimeout(250);
      assert.equal(await page.locator('#panelCountry').evaluate(e => e.scrollWidth <= window.innerWidth), true, `${label}: country panel overflow at ${width}`);
      const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`country-${label}-${width}-${theme}.png`),fullPage:true,animations:'disabled'});}
      }
    }
  } finally { await browser.close(); }
}

(async () => {
    await check(fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8'), 'source');
    if (!process.env.COUNTRY_DASHBOARD_SOURCE_ONLY) {
      const header = fs.readFileSync(path.join(root, 'index_html.h'), 'utf8');
      const data = header.match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
      const html = zlib.gunzipSync(Buffer.from(data.match(/0x[\da-f]+/gi).map(x => parseInt(x, 16)))).toString();
      await check(html, 'embedded');
    }
})().then(() => console.log('PASS country dashboard source/embedded behavior'), e => { console.error(e); process.exitCode = 1; });
