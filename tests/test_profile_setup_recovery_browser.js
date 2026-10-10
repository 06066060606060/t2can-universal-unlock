const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const { launchBrowser } = require('./browser_test_runtime');
const root = path.resolve(__dirname, '..');
const setup = { setupMode: true, profile: 0, topology: 0, turn: 0, migrationNotice: false, nvsError: false };

async function check(html, label) {
  const browser = await launchBrowser();
  try {
    const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
    const errors = [], posts = [], resetPosts = [];
    let statusCalls = 0, mode = 'initial503', postMode = 'failure', postRelease = null, selected = null;
    page.on('pageerror', e => errors.push(e.message));
    await page.route('http://profile-recovery.test/**', async route => {
      const req = route.request(), url = new URL(req.url());
      if (url.pathname === '/') return route.fulfill({ contentType: 'text/html', body: html });
      let body = {};
      if (req.method() === 'POST' && url.pathname.includes('reset')) resetPosts.push(url.pathname);
      if (url.pathname === '/api/profile/status') {
        statusCalls++;
        if (mode === 'initial503' && statusCalls === 1) return route.fulfill({ status: 503, body: '{}' });
        if (mode === 'permanent503') return route.fulfill({ status: 503, body: '{}' });
        body = mode === 'configured' ? { ...setup, setupMode: false, ...selected } : { ...setup, nvsError: mode === 'nvsError' };
      } else if (url.pathname === '/api/profile/select') {
        posts.push(url.search);
        if (postMode === 'blocked') await new Promise(resolve => { postRelease = resolve; });
        if (postMode === 'networkLoss') return route.abort('failed');
        if (postMode !== 'success') return route.fulfill({ status: 503, contentType: 'application/json', body: JSON.stringify({ error: 'profile_store_failed' }) });
        selected = { profile: Number(url.searchParams.get('profile')), topology: Number(url.searchParams.get('topology')), turn: Number(url.searchParams.get('turn')) };
        mode = 'configured';
        body = { ok: true, rebooting: true };
      }
      return route.fulfill({ contentType: 'application/json', body: JSON.stringify(body) });
    });
    const load = () => page.goto('http://profile-recovery.test/', { waitUntil: 'domcontentloaded' });
    async function ready() {
      await page.locator('[data-vehicle="MODEL_YL"]').click({ timeout: 12000 });
      await page.waitForFunction(() => !document.querySelector('#saveProfileBtn').disabled && document.querySelector('#setupError').getBoundingClientRect().height === 0, null, { timeout: 12000 });
      assert.equal(await page.locator('#setupTitle').innerText(), 'Select your vehicle.');
    }
    const errorVisible = () => page.waitForFunction(() => { const e = document.querySelector('#setupError'); return e && e.getBoundingClientRect().height > 0 && e.textContent.trim(); });
    async function choose(profile, topology, turn) {
      const keys = { 1: 'MODEL_YL', 2: 'MODEL_Y_JUNIPER', 3: 'MODEL_Y_LEGACY', 4: 'MODEL_3_HIGHLAND', 5: 'MODEL_3_LEGACY' };
      await page.locator(`[data-vehicle="${keys[profile]}"]`).click();
      if (topology !== 1) await page.locator(`[data-topology="${topology === 2 ? 'BODY_CHASSIS' : 'PARTY_CHASSIS'}"]`).click();
      if (profile === 4 && topology === 2) await page.locator(`[data-turn="${turn === 2 ? 'STALKLESS' : 'STALK'}"]`).click();
      assert.equal(await page.locator('#saveProfileBtn').isDisabled(), false);
    }
    async function confirm() {
      await page.locator('#saveProfileBtn').click();
      await page.locator('#confirmProfileSave').click();
    }

    await load();
    await ready();
    assert(statusCalls >= 2, `${label}: initial status failure must retry automatically`);
    assert.equal(await page.locator('#setupError').getAttribute('role'), 'status');
    assert.equal(await page.locator('#setupError').getAttribute('aria-live'), 'polite');

    // Simulate a fetch which only settles when the caller aborts it.
    await page.addInitScript(() => {
      const original = window.fetch; let hung = false;
      window.fetch = (url, options = {}) => {
        if (sessionStorage.getItem('hangProfileOnce') === '1' && !hung && String(url) === '/api/profile/status') {
          hung = true; sessionStorage.removeItem('hangProfileOnce');
          return new Promise((resolve, reject) => options.signal?.addEventListener('abort', () => reject(new DOMException('Timed out', 'AbortError')), { once: true }));
        }
        return original(url, options);
      };
    });
    mode = 'normal'; statusCalls = 0;
    await page.evaluate(() => sessionStorage.setItem('hangProfileOnce', '1'));
    await load();
    await ready();
    assert(statusCalls >= 1, `${label}: hung initial fetch must time out and retry`);

    mode = 'permanent503'; await load(); await errorVisible();
    assert.equal(await page.locator('#retryProfileStatusBtn').isVisible(), true);
    mode = 'normal'; await page.locator('#retryProfileStatusBtn').click(); await ready();

    mode = 'nvsError'; await load(); await errorVisible();
    assert.match(await page.locator('#setupError').innerText(), /NVS|storage/i);
    await page.locator('[data-vehicle="MODEL_YL"]').click();
    assert.equal(await page.locator('#saveProfileBtn').isDisabled(), true, `${label}: NVS unavailable must prevent selection save`);
    assert.equal(await page.locator('#setupFactoryResetBtn').isVisible(), true);
    await page.locator('#setupFactoryResetBtn').click();
    assert.equal(await page.locator('#factoryWarningModal').isVisible(), true);
    assert.equal(resetPosts.length, 0, `${label}: NVS recovery must require deliberate reset confirmation`);
    await page.locator('#cancelFactoryReset').click();

    mode = 'normal'; await load(); await ready();
    await choose(3, 2, 1); await confirm(); await errorVisible();
    assert.match(await page.locator('#setupError').innerText(), /profile_store_failed/);
    assert.equal(await page.locator('[data-vehicle="MODEL_Y_LEGACY"]').evaluate(e => e.classList.contains('selected')), true);
    assert.equal(await page.locator('[data-topology="BODY_CHASSIS"]').evaluate(e => e.classList.contains('selected')), true);
    assert.equal(await page.locator('#saveProfileBtn').isDisabled(), false);
    const beforeRetry = posts.length; await confirm(); await errorVisible();
    assert.equal(posts.length, beforeRetry + 1, `${label}: explicit retry must issue one save`);

    // A delayed save must lock both confirmation and the underlying save action.
    postMode = 'blocked'; await page.locator('#saveProfileBtn').click();
    const beforeDouble = posts.length;
    await page.evaluate(() => { const b = document.querySelector('#confirmProfileSave'); b.click(); b.click(); });
    await page.waitForFunction(() => document.querySelector('#saveProfileBtn').disabled);
    await page.waitForTimeout(100); assert.equal(posts.length, beforeDouble + 1, `${label}: repeated confirmation must issue only one POST`);
    postMode = 'failure'; postRelease(); await errorVisible();

    postMode = 'networkLoss'; const beforeLoss = posts.length; await confirm(); await errorVisible();
    assert.match(await page.locator('#setupError').innerText(), /reconnect|connection|check|confirm/i);
    await page.waitForTimeout(800);
    assert.equal(posts.length, beforeLoss + 1, `${label}: ambiguous network failure must never automatically repeat POST`);
    assert.equal(await page.locator('#saveProfileBtn').isDisabled(), true);
    assert.equal(await page.locator('#retryProfileStatusBtn').isVisible(), true);
    await page.locator('#retryProfileStatusBtn').click(); await ready();
    await choose(3, 2, 1);
    assert.equal(posts.length, beforeLoss + 1, `${label}: manual status read must not repeat POST`);

    // Reload clears the ambiguous outcome before deliberate new selections.
    postMode = 'failure'; await load(); await ready();
    let combinations = 0;
    for (const profile of [1, 2, 3, 4, 5]) for (const topology of profile === 1 ? [1] : [2, 3]) for (const turn of profile === 4 && topology === 2 ? [1, 2] : [topology === 3 ? 0 : 1]) {
      await choose(profile, topology, turn); await confirm(); await errorVisible();
      assert.equal(posts.at(-1), `?profile=${profile}&topology=${topology}&turn=${turn}`); combinations++;
    }
    assert.equal(combinations, 10);
    postMode = 'success'; await choose(1, 1, 1); await confirm();
    await page.waitForURL(/resume=/, { timeout: 10000 });
    await page.waitForFunction(() => universalBootReady, null, { timeout: 5000 });
    assert.equal(await page.locator('#vehicleSetup').isVisible(), false);

    // Leaving and reopening profile change must retain the way to resolve an uncertain save.
    postMode = 'networkLoss';
    await page.getByRole('button', { name: 'Settings', exact: true }).click();
    await page.locator('#changeProfileBtn').click();
    await choose(3, 2, 1); await confirm(); await errorVisible();
    await page.locator('#cancelProfileChangeBtn').click();
    await page.locator('#changeProfileBtn').click();
    assert.equal(await page.locator('#saveProfileBtn').isDisabled(), true);
    assert.equal(await page.locator('#retryProfileStatusBtn').isVisible(), true, `${label}: reopening uncertain profile change must retain Check Status`);
    assert.match(await page.locator('#setupError').innerText(), /check|confirm|reconnect/i);
    const beforeReopenCheck = posts.length;
    await page.locator('#retryProfileStatusBtn').click();
    await page.waitForFunction(() => !document.body.classList.contains('setup-active'));
    assert.equal(posts.length, beforeReopenCheck, `${label}: reopened status check must not repeat save`);
    assert.deepEqual(errors, [], `${label}: unexpected JavaScript errors`);
    console.log(`PASS ${label}: boot recovery, timeout, NVS error, save failure/retry, duplicate guard, ambiguous response and 10 selections`);
  } finally { await browser.close(); }
}

(async () => {
  await check(fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8'), 'source');
  if (!process.env.PROFILE_SETUP_SOURCE_ONLY) {
    const array = fs.readFileSync(path.join(root, 'index_html.h'), 'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
    await check(zlib.gunzipSync(Buffer.from(array.match(/0x[\da-f]+/gi).map(x => parseInt(x, 16)))).toString(), 'embedded');
  }
})().catch(e => { console.error(e); process.exitCode = 1; });
