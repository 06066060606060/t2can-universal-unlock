const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { launchBrowser } = require('./browser_test_runtime');

const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(process.env.DASHBOARD_HTML_PATH || path.join(root, 'dashboard_source.html'));

const profile = {
  setupMode: false,
  migrationNotice: false,
  profile: 1,
  topology: 1,
  turn: 1,
  topologyName: 'Party + VH',
  canA: 'PARTY',
  canB: 'VH',
  nagSupported: true,
  nagTorqueSupported: true,
  nagTsl9Supported: true,
  advancedEapSupported: true,
  euUnlockSupported: true,
  pedalMapSupported: true,
};

const nagConfig = {
  enabled: true,
  method: 0,
  tsl9Sequence: 0,
  tsl9Window: 0,
  tsl9InputMode: 0,
  torqueRightScrollEnabled: false,
  torqueRightScrollIntervalSeconds: 30,
  torqueRightScrollPattern: 1,
  tsl9RightPeriodicEnabled: false,
  tsl9RightPeriodicIntervalSeconds: 30,
  tsl9LegacyRoute: 0,
  tsl9LegacyRouteSelectable: false,
  mode: 7,
  humanVariant: 3,
};

const driverMonitoring = {
  enabled: false,
  supported: true,
  active: false,
};

const isaSuppression = {
  enabled: false,
  supported: true,
  active: false,
};

const features = {
  lab: true,
  s3xy: true,
  doorCancel: false,
  bannedSupported: false,
  banned: false,
  tlsscRestoreSupported: false,
  tlsscRestore: false,
  pedalMapSupported: true,
  apDriveProfileSupported: true,
  apDriveProfile: false,
  apDriveProfileRegenRaw: 10,
};

const r79 = {
  fixedPolicy: true,
  mode: 1,
  modeName: 'Mode 1',
  transport: 'D9_MUX1_2MS_WAIT',
  periodic: 'MUX2_PLUS_150MS_1X',
  mode1TxWaitMode: 1,
  mode1ReinjectEnabled: true,
  mode1DelayMs: 150,
  mode2ReinjectEnabled: false,
  mode2DelayMs: 150,
  bit18Mode: 1,
  bit18ModeName: 'FORCE_0',
};

const homeSnapshot = {
  fast: {
    nag: { torque: 0.03, handsOnState: 4, apActive: true, canAState: 1 },
    blink: { dasState: 5, dasStateValid: true, enabled: true },
    summon: { canState: 1, canStateName: 'RUNNING' },
    cantraffic: {
      mcpTrafficSeen: true,
      mcpTrafficOnline: true,
      mcpTrafficAgeMs: 18,
      twaiTrafficSeen: true,
      twaiTrafficOnline: true,
      twaiTrafficAgeMs: 22,
    },
    r79: {
      txState: 'ACTIVE',
      txReason: 'AUTOPILOT',
      txOk: 42,
      txFail: 0,
      lastTxAgeMs: 12,
      smartMode: 0,
    },
  },
  slow: {
    blink: { enabled: true, delayMs: 300 },
    summon: { tlssc: false },
    s3xy: { bluetoothEnabled: true, pairedCount: 2, connectedCount: 2 },
    r79: { smartMode: 0 },
    lab3f8: { alcMode: 0 },
  },
};

const s3xy = {
  bluetoothEnabled: true,
  initialized: true,
  autoEnabled: true,
  pairedCount: 1,
  connectedCount: 1,
  maxDevices: 3,
  devices: [{
    id: 1,
    name: 'Driver Button',
    address: 'AA:BB:CC:DD:EE:FF',
    state: 'READY',
    connected: true,
    secure: true,
    subscribed: true,
    autoConnect: true,
    rssi: -48,
    notifyCount: 27,
    singleAction: 'left_blinker',
    doubleAction: 'right_blinker',
    longAction: 'noa_cancel',
  }],
  ulcTxOk: 0,
  ulcTxFail: 0,
  ulcAccepted: 0,
  ulcBlocked: 0,
  ulcLastDir: 0,
};

const requests = [];
async function waitForRequest(expected, timeoutMs = 2000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (requests.some(request => request === expected)) return;
    await new Promise(resolve => setTimeout(resolve, 20));
  }
  assert.fail(`request not observed: ${expected}`);
}
const testOrigin = 'http://t2can.test';

function json(value, status = 200) {
  return {
    status,
    contentType: 'application/json',
    headers: { 'Cache-Control': 'no-store' },
    body: JSON.stringify(value),
  };
}

async function installDashboardRoutes(page) {
  await page.route(`${testOrigin}/**`, async route => {
  const request = route.request();
  const url = new URL(request.url());
  requests.push(`${request.method()} ${url.pathname}${url.search}`);
  if (url.pathname === '/') {
    return route.fulfill({
      status: 200,
      contentType: 'text/html; charset=utf-8',
      headers: { 'Cache-Control': 'no-store' },
      body: html,
    });
  }
  let response;
  if (url.pathname === '/api/profile/status') response = json(profile);
  else if (url.pathname === '/api/features/status') response = json(features);
  if (url.pathname === '/api/nag/config') {
    response = json(nagConfig);
  } else if (url.pathname === '/api/nag/update') {
    if (url.searchParams.has('method')) nagConfig.method = Number(url.searchParams.get('method'));
    if (url.searchParams.has('tsl9Sequence')) nagConfig.tsl9Sequence = Number(url.searchParams.get('tsl9Sequence'));
    if (url.searchParams.has('tsl9Window')) nagConfig.tsl9Window = Number(url.searchParams.get('tsl9Window'));
    if (url.searchParams.has('tsl9InputMode')) nagConfig.tsl9InputMode = Number(url.searchParams.get('tsl9InputMode'));
    if (url.searchParams.has('torqueRightScrollEnabled')) nagConfig.torqueRightScrollEnabled = url.searchParams.get('torqueRightScrollEnabled') === '1';
    if (url.searchParams.has('torqueRightScrollIntervalSeconds')) nagConfig.torqueRightScrollIntervalSeconds = Number(url.searchParams.get('torqueRightScrollIntervalSeconds'));
    if (url.searchParams.has('torqueRightScrollPattern')) nagConfig.torqueRightScrollPattern = Number(url.searchParams.get('torqueRightScrollPattern'));
    if (url.searchParams.has('tsl9RightPeriodicEnabled')) nagConfig.tsl9RightPeriodicEnabled = url.searchParams.get('tsl9RightPeriodicEnabled') === '1';
    if (url.searchParams.has('tsl9RightPeriodicIntervalSeconds')) nagConfig.tsl9RightPeriodicIntervalSeconds = Number(url.searchParams.get('tsl9RightPeriodicIntervalSeconds'));
    if (url.searchParams.has('tsl9LegacyRoute')) nagConfig.tsl9LegacyRoute = Number(url.searchParams.get('tsl9LegacyRoute'));
    response = json(nagConfig);
  } else if (url.pathname === '/api/driver-monitoring/config') {
    if (url.searchParams.has('enabled')) driverMonitoring.enabled = url.searchParams.get('enabled') === '1';
    response = json(driverMonitoring);
  } else if (url.pathname === '/api/isa-suppression/config') {
    if (url.searchParams.has('enabled')) isaSuppression.enabled = url.searchParams.get('enabled') === '1';
    isaSuppression.active = isaSuppression.enabled;
    response = json(isaSuppression);
  } else if (url.pathname === '/api/r79/stats') response = json(r79);
  else if (url.pathname === '/api/r79/update') {
    if (url.searchParams.has('mode')) r79.mode = Number(url.searchParams.get('mode'));
    if (url.searchParams.has('bit18Mode')) r79.bit18Mode = Number(url.searchParams.get('bit18Mode'));
    if (url.searchParams.has('mode1TxWaitMode')) r79.mode1TxWaitMode = Number(url.searchParams.get('mode1TxWaitMode'));
    if (url.searchParams.has('mode1Reinject')) r79.mode1ReinjectEnabled = url.searchParams.get('mode1Reinject') === '1';
    if (url.searchParams.has('mode1DelayMs')) r79.mode1DelayMs = Number(url.searchParams.get('mode1DelayMs'));
    if (url.searchParams.has('mode2Reinject')) r79.mode2ReinjectEnabled = url.searchParams.get('mode2Reinject') === '1';
    if (url.searchParams.has('mode2DelayMs')) r79.mode2DelayMs = Number(url.searchParams.get('mode2DelayMs'));
    response = json(r79);
  } else if (url.pathname === '/api/s3xy/stats') response = json(s3xy);
  else if (url.pathname === '/api/snapshot') {
    const groups = url.searchParams.get('groups');
    if (groups === 'home-fast') response = json(homeSnapshot);
    else if (groups === 'lab-lite') {
      response = json({
        r79: homeSnapshot.fast.r79,
        alc: { alcValid: true, alcRaw: 8, alcAgeMs: 10, lane239Valid: false },
        dmsNag: {},
      });
    } else if (groups === 'settings-lite') {
      response = json({
        system: { fwVersion: '3.8.4' },
        blink: { delayMs: 300 },
        summon: { sessionActive: false },
        s3xy,
        lab3f8: { alcMode: 0 },
      });
    }
  }
  return route.fulfill(response || json({}));
  });
}


// Use Chromium's input pipeline: dispatchEvent cannot reproduce backdrop retargeting.
async function verifyTouchSelect(page, selector, fallback = false) {
  const cdp = await page.context().newCDPSession(page);
  const shown = () => page.locator('#selectSheet').evaluate(el => el.classList.contains('show'));
  const center = async target => {
    const box = await (typeof target === 'string' ? page.locator(target).first() : target).boundingBox();
    assert(box, `missing touch target ${target}`);
    return { x: box.x + box.width / 2, y: box.y + box.height / 2 };
  };
  const touch = async (target, hold = 30) => {
    const point = await center(target);
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [point] });
    await page.waitForTimeout(hold);
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
    await page.waitForTimeout(300);
  };
  try {
    await page.waitForTimeout(300);
    for (const hold of [30, 100, 700]) {
      await touch(selector, hold);
      assert.equal(await shown(), true, `${fallback ? 'fallback' : 'pointer'} ${selector} ${hold}ms touch dismissed opening sheet`);
      // A new independent touch outside the sheet must still dismiss it.
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ x: 5, y: 100 }] });
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
      await page.waitForTimeout(300);
      assert.equal(await shown(), false, 'independent backdrop touch did not close sheet');
    }
    const point = await center(selector);
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [point] });
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchCancel', touchPoints: [] });
    await page.waitForTimeout(300);
    assert.equal(await shown(), false, 'cancelled touch opened sheet');
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [point] });
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x: point.x, y: point.y + 40 }] });
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
    await page.waitForTimeout(300);
    assert.equal(await shown(), false, 'drag gesture opened sheet');

    // Real option taps must emit one change, while choosing the current value emits none.
    const original = await page.locator(selector).evaluate(el => ({ value: el.value, label: el.selectedOptions[0].textContent.trim() }));
    await page.locator(selector).evaluate(el => {
      window.__touchChangeCount = 0;
      window.__touchChangeListener = () => window.__touchChangeCount++;
      el.addEventListener('change', window.__touchChangeListener);
    });
    await touch(selector);
    await touch('#selectSheet .selectChoice[aria-selected="true"]');
    assert.equal(await shown(), false);
    assert.equal(await page.evaluate(() => window.__touchChangeCount), 0);
    await touch(selector);
    await touch('#selectSheet .selectChoice[aria-selected="false"]');
    assert.equal(await shown(), false);
    assert.equal(await page.evaluate(() => window.__touchChangeCount), 1);
    assert.notEqual(await page.locator(selector).inputValue(), original.value);
    await touch(selector);
    await touch(page.locator('#selectSheet').getByRole('option', { name: original.label, exact: true }));
    assert.equal(await page.locator(selector).inputValue(), original.value);
    assert.equal(await page.evaluate(() => window.__touchChangeCount), 2);
    await page.locator(selector).evaluate(el => el.removeEventListener('change', window.__touchChangeListener));
    if (!fallback) {
      await page.locator(selector).click();
      assert.equal(await shown(), true, 'mouse click did not open sheet');
      await page.locator('#selectSheetCancel').click();
      assert.equal(await shown(), false);
    }
    assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0, 'touch/mouse activation focused native select');
    for (const key of ['Enter', ' ', 'ArrowDown', 'ArrowUp']) {
      await page.locator(selector).press(key);
      assert.equal(await shown(), true, `keyboard ${key} did not open sheet`);
      await page.keyboard.press('Escape');
      assert.equal(await shown(), false);
    }
    await page.locator(selector).evaluate(el => el.disabled = true);
    await touch(selector);
    assert.equal(await shown(), false, 'disabled select opened sheet');
    await page.locator(selector).evaluate(el => el.disabled = false);
    if (fallback) {
      // Moving out and back must remain cancelled, rather than inspecting end distance only.
      const point = await center(selector);
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [point] });
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x: point.x, y: point.y + 40 }] });
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [point] });
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
      await page.waitForTimeout(300);
      assert.equal(await shown(), false, 'out-and-back drag opened sheet');
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ ...point, id: 1 }, { x: point.x - 20, y: point.y, id: 2 }] });
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
      await page.waitForTimeout(300);
      assert.equal(await shown(), false, 'multi-touch opened sheet');
    }
    // Test-only native focus tracking excludes our explicit keyboard focus above.
    await page.evaluate(() => window.__nativeSelectFocusCalls = 0);
    await touch(selector);
    await touch('#selectSheetCancel');
    assert.equal(await shown(), false, 'cancel button touch did not close sheet');
  } finally {
    await cdp.detach();
  }
}

async function main() {
  const browser = await launchBrowser();
  try {
    const page = await browser.newPage({ viewport: { width: 390, height: 844 }, hasTouch: true, isMobile: true });
    await installDashboardRoutes(page);
    await page.addInitScript(() => {
      const nativeSelectFocus = HTMLSelectElement.prototype.focus;
      window.__nativeSelectFocusCalls = 0;
      HTMLSelectElement.prototype.focus = function (...args) {
        window.__nativeSelectFocusCalls += 1;
        return nativeSelectFocus.apply(this, args);
      };
    });
    const pageErrors = [];
    page.on('pageerror', error => pageErrors.push(error.message));
    await page.goto(`${testOrigin}/`, { waitUntil: 'domcontentloaded' });

    await page.waitForFunction(() => document.body.dataset.ap === 'NOA');
    assert.equal(await page.locator('#r79Mode').inputValue(), '1');
    assert.equal(await page.locator('#r79Bit18Mode').inputValue(), '0');
    assert.equal(await page.locator('#r79Mode1TxWait').inputValue(), '1');
    assert.equal(await page.locator('#r79Mode1Reinject').isChecked(), true);
    assert.equal(await page.locator('#r79Mode1DelayMs').inputValue(), '150');
    const viewportContract = await page.evaluate(() => {
      const meta = document.querySelector('meta[name="viewport"]')?.content || '';
      const gesture = new Event('gesturestart', { bubbles: true, cancelable: true });
      window.dispatchEvent(gesture);
      const htmlStyle = getComputedStyle(document.documentElement);
      const bodyStyle = getComputedStyle(document.body);
      const scrollbarStyle = getComputedStyle(document.documentElement, '::-webkit-scrollbar');
      return {
        meta,
        gesturePrevented: gesture.defaultPrevented,
        htmlOverflowX: htmlStyle.overflowX,
        bodyOverflowX: bodyStyle.overflowX,
        scrollbarDisplay: scrollbarStyle.display,
      };
    });
    assert.match(viewportContract.meta, /maximum-scale=1/);
    assert.match(viewportContract.meta, /user-scalable=no/);
    assert.equal(viewportContract.gesturePrevented, true);
    assert(['hidden', 'clip'].includes(viewportContract.htmlOverflowX));
    assert(['hidden', 'clip'].includes(viewportContract.bodyOverflowX));
    assert.equal(viewportContract.scrollbarDisplay, 'none');
    assert.equal(await page.locator('#apMode').textContent(), 'Navigating');
    assert.equal(await page.locator('#apModeSub').textContent(), 'Navigate on Autopilot is active.');
    assert.equal(await page.locator('.logo').textContent(), 'TESLA UNLOCK');
    assert.equal(await page.locator('#homeNagSignalLabel').textContent(), 'Stock torque Nm');
    assert.equal(await page.locator('#torque').textContent(), '+0.03');
    assert.equal(await page.locator('#homeLiveDetails').count(), 0);
    assert.equal(await page.locator('label select').count(), 0);
    assert(!requests.some(request => request.includes('live=1')));

    const apR79 = {...homeSnapshot.fast.r79};
    Object.assign(homeSnapshot.fast.r79, {
      txState: 'ACTIVE',
      txReason: 'DEFAULT',
      gearName: 'P',
      dasState: 0,
      dasStateValid: true,
    });
    await page.evaluate(fast => renderHomeFast(fast), homeSnapshot.fast);
    assert.equal(await page.locator('#homeR79State').textContent(), 'Active · Park standby');
    Object.assign(homeSnapshot.fast.r79, apR79);
    await page.evaluate(fast => renderHomeFast(fast), homeSnapshot.fast);
    assert.equal(await page.locator('#homeR79State').textContent(), 'Active · AP engaged');
    if (process.env.DASHBOARD_SCREENSHOT_DIR) await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'home.png'), fullPage: true });

    const headerLeftBeforeSettings = await page.locator('.globaltop').evaluate(element => element.getBoundingClientRect().left);
    const settingsTransition = await page.evaluate(async () => {
      let maxScrollWidth = 0;
      let maxHeaderDelta = 0;
      let maxPageOffset = 0;
      let minPageOpacity = 1;
      let sawPageAnimation = false;
      const startLeft = document.querySelector('.globaltop').getBoundingClientRect().left;
      // Start observation in the click's browser task: a separate Playwright
      // round trip can miss the entire 200 ms animation under build/CI load.
      document.querySelector('.navbtn[data-page="settings"]').click();
      for (let i = 0; i < 20; i += 1) {
        if (i > 0) await new Promise(resolve => requestAnimationFrame(resolve));
        const activePage = document.querySelector('.page.active');
        const pageStyle = getComputedStyle(activePage);
        const matrix = pageStyle.transform === 'none' ? null : new DOMMatrixReadOnly(pageStyle.transform);
        maxPageOffset = Math.max(maxPageOffset, Math.abs(matrix?.m41 || 0));
        minPageOpacity = Math.min(minPageOpacity, Number(pageStyle.opacity));
        sawPageAnimation ||= activePage.getAnimations().some(animation => animation.playState === 'running');
        maxScrollWidth = Math.max(maxScrollWidth, document.documentElement.scrollWidth, document.body.scrollWidth);
        maxHeaderDelta = Math.max(maxHeaderDelta,
          Math.abs(document.querySelector('.globaltop').getBoundingClientRect().left - startLeft));
      }
      return { maxScrollWidth, maxHeaderDelta, maxPageOffset, minPageOpacity, sawPageAnimation, viewportWidth: innerWidth };
    });
    assert.equal(settingsTransition.sawPageAnimation, true, 'main page navigation has no running transition');
    assert(settingsTransition.maxPageOffset > 0.5, 'main page navigation has no visible slide movement');
    assert(settingsTransition.minPageOpacity < 0.99, 'main page navigation has no visible fade');
    assert(settingsTransition.maxScrollWidth <= settingsTransition.viewportWidth,
      `page transition creates horizontal overflow: ${settingsTransition.maxScrollWidth} > ${settingsTransition.viewportWidth}`);
    assert(settingsTransition.maxHeaderDelta < 0.5, `header shifted ${settingsTransition.maxHeaderDelta}px`);
    assert(Math.abs(await page.locator('.globaltop').evaluate(element => element.getBoundingClientRect().left) - headerLeftBeforeSettings) < 0.5);
    assert.equal(await page.locator('.page.active').getAttribute('data-page'), 'settings');
    assert.equal(await page.locator('.page.active .pageTitle').textContent(), 'Settings');
    assert.equal(await page.locator('.page.active .listrow[data-panel="panelR79"]').count(), 1);
    assert.equal(await page.locator('.page.active .listrow[data-panel="panelApRightScroll"]').count(), 0);
    assert.equal(await page.locator('#driverMonitoringToggle').count(), 1);
    assert.equal(await page.locator('#isaSuppressionToggle').count(), 1);
    assert.equal(await page.locator('.page.active .listrow[data-panel="panelVisionControl"]').count(), 1);
    await page.locator('#driverMonitoringRow label.toggle').click();
    await page.waitForFunction(() => document.querySelector('#driverMonitoringToggle')?.checked === true);
    assert(requests.some(request => request === 'POST /api/driver-monitoring/config?enabled=1'));
    await page.locator('#isaSuppressionRow label.toggle').click();
    await page.waitForFunction(() => document.querySelector('#isaSuppressionToggle')?.checked === true);
    assert(requests.some(request => request === 'POST /api/isa-suppression/config?enabled=1'));

    await page.locator('.page.active .listrow[data-panel="panelDiag"]').click();
    await page.waitForFunction(() => document.querySelector('#panelDiag')?.classList.contains('show'));
    const systemStatsButton = page.locator('#diagSystemStatsJson');
    assert.equal(await systemStatsButton.isVisible(), true);
    assert.equal(await systemStatsButton.evaluate(button =>
      button.closest('#diagCanBCard .diagButtons') !== null), true);
    const systemStatsLayout = await systemStatsButton.evaluate(button => {
      const buttonBox = button.getBoundingClientRect();
      const gridBox = button.parentElement.getBoundingClientRect();
      return { buttonWidth: buttonBox.width, gridWidth: gridBox.width };
    });
    assert(systemStatsLayout.buttonWidth >= systemStatsLayout.gridWidth * 0.9,
      'System Stats JSON should span the Diagnostics action grid');
    if (process.env.DASHBOARD_SCREENSHOT_DIR) {
      await systemStatsButton.scrollIntoViewIfNeeded();
      await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'diagnostics.png') });
    }
    const [systemStatsDownload] = await Promise.all([
      page.waitForEvent('download'),
      systemStatsButton.click(),
    ]);
    assert.equal(systemStatsDownload.suggestedFilename(), 'T2CAN_SYSTEM_STATS.json');
    await page.waitForFunction(() => !window.downloadInProgress);
    assert(requests.some(request => request === 'GET /api/system/stats'));
    await page.locator('#panelDiag .backbtn').click();
    await page.waitForFunction(() => !document.querySelector('#panelDiag')?.classList.contains('show'));

    await page.locator('.page.active .listrow[data-panel="panelNag"]').click();
    await page.waitForFunction(() => document.querySelector('#panelNag')?.classList.contains('show'));
    assert.equal(await page.locator('#panelNag .listrow[data-panel="panelApRightScroll"]').count(), 0);
    assert.equal(await page.locator('#nagDmsToggle').count(), 0);
    assert.equal(await page.locator('#tsl9InputModeWrap').isVisible(), false);
    assert.equal(await page.locator('#torqueRightScrollWrap').isVisible(), true);
    assert.equal(await page.locator('#torqueRightScrollWrap .listkey').textContent(), 'Auto Speed Scroll');
    assert.equal(await page.locator('#torqueRightScrollWrap').evaluate(wrapper =>
      wrapper.previousElementSibling === document.querySelector('#nagIgnoreApStateWrap')
      && wrapper.previousElementSibling.previousElementSibling === document.querySelector('#nagMethod')?.closest('.labControlRow')), true);
    assert.equal(await page.locator('#tsl9RightPeriodicWrap').isVisible(), false);
    assert.equal(await page.locator('#torqueRightScrollPattern').inputValue(), '1');
    await page.locator('#torqueRightScrollWrap label.toggle').click();
    // Native checked/value changes precede the save response. Wait until
    // renderNagCfg has applied that response before editing the next field.
    await page.waitForFunction(() => nagCfg?.torqueRightScrollEnabled === true
      && document.querySelector('#torqueRightScrollToggle')?.checked === true);
    assert(requests.some(request => request === 'POST /api/nag/update?torqueRightScrollEnabled=1'));
    await page.locator('#torqueRightScrollInterval').fill('45');
    await page.locator('#torqueRightScrollInterval').dispatchEvent('change');
    await waitForRequest('POST /api/nag/update?torqueRightScrollIntervalSeconds=45');
    await page.waitForFunction(() => nagCfg?.torqueRightScrollIntervalSeconds === 45);
    await page.locator('#torqueRightScrollPattern').selectOption('0');
    await page.waitForFunction(() => nagCfg?.torqueRightScrollPattern === 0
      && document.querySelector('#torqueRightScrollPattern')?.value === '0');
    assert(requests.some(request => request === 'POST /api/nag/update?torqueRightScrollPattern=0'));
    const advancedCopyLayout = await page.locator('#nagAdvancedOpen').evaluate(button => {
      const title = button.querySelector('.listkey').getBoundingClientRect();
      const description = button.querySelector('.listdesc').getBoundingClientRect();
      return { titleBottom: title.bottom, descriptionTop: description.top,
        titleLeft: title.left, descriptionLeft: description.left };
    });
    assert(advancedCopyLayout.descriptionTop >= advancedCopyLayout.titleBottom - 0.5);
    assert(Math.abs(advancedCopyLayout.descriptionLeft - advancedCopyLayout.titleLeft) < 0.5);

    const openNagMethodSheet = async () => {
      const result = await page.locator('#nagMethod').evaluate(select => {
        const pointer = new PointerEvent('pointerdown', {
          bubbles: true,
          cancelable: true,
          isPrimary: true,
          pointerType: 'touch',
        });
        const click = new MouseEvent('click', { bubbles: true, cancelable: true });
        select.dispatchEvent(pointer);
        select.dispatchEvent(click);
        return {
          pointerPrevented: pointer.defaultPrevented,
          clickPrevented: click.defaultPrevented,
        };
      });
      assert.deepEqual(result, { pointerPrevented: true, clickPrevented: true });
      await page.waitForFunction(() => document.querySelector('#selectSheet')?.classList.contains('show'));
    };

    await openNagMethodSheet();
    await page.locator('#selectSheetCancel').click();
    await page.waitForFunction(() => !document.querySelector('#selectSheet')?.classList.contains('show'));
    assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0);
    assert.equal(await page.evaluate(() => document.activeElement === document.body), true);

    await openNagMethodSheet();
    await page.locator('#selectSheet .selectChoice').filter({ hasText: 'TSL9' }).click();
    await page.waitForFunction(() => !document.querySelector('#selectSheet')?.classList.contains('show'));
    assert.equal(await page.locator('#nagMethod').inputValue(), '1');
    assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0);
    await page.waitForFunction(() => document.querySelector('#tsl9InputModeWrap')?.classList.contains('tuUiHidden') === false);
    assert.equal(await page.locator('#tsl9InputModeWrap').isVisible(), true);
    assert.equal(await page.locator('#tsl9IsaWrap').count(), 0);
    assert.equal(await page.locator('#torqueRightScrollWrap').isVisible(), false);
    assert.equal(await page.locator('#tsl9RightPeriodicWrap').isVisible(), false);
    assert.equal(await page.locator('#tsl9InputMode').inputValue(), '0');
    await page.locator('#tsl9InputMode').selectOption('1');
    await page.waitForFunction(() => document.querySelector('#tsl9InputMode')?.value === '1');
    assert(requests.some(request => request === 'POST /api/nag/update?tsl9InputMode=1'));
    await page.waitForFunction(() => document.querySelector('#tsl9RightPeriodicWrap')?.classList.contains('tuUiHidden') === false);
    assert.equal(await page.locator('#tsl9RightPeriodicWrap').isVisible(), true);
    await page.locator('#tsl9RightPeriodicWrap label.toggle').click();
    await page.waitForFunction(() => document.querySelector('#tsl9RightPeriodicToggle')?.checked === true);
    assert(requests.some(request => request === 'POST /api/nag/update?tsl9RightPeriodicEnabled=1'));
    await page.locator('#tsl9RightPeriodicInterval').fill('60');
    await page.locator('#tsl9RightPeriodicInterval').dispatchEvent('change');
    await waitForRequest('POST /api/nag/update?tsl9RightPeriodicIntervalSeconds=60');
    await openNagMethodSheet();
    await page.locator('#selectSheetBack').click({ position: { x: 5, y: 5 } });
    await page.waitForFunction(() => !document.querySelector('#selectSheet')?.classList.contains('show'));
    assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0);

    for (const width of [320, 430]) {
      await page.setViewportSize({ width, height: 844 });
      await openNagMethodSheet();
      await page.waitForTimeout(300);
      const sheetBounds = await page.locator('#selectSheet').evaluate(element => {
        const rect = element.getBoundingClientRect();
        return { left: rect.left, right: rect.right, bottom: rect.bottom, viewportWidth: innerWidth, viewportHeight: innerHeight };
      });
      assert(sheetBounds.left >= 0, `select sheet begins outside ${width}px viewport`);
      assert(sheetBounds.right <= sheetBounds.viewportWidth, `select sheet exceeds ${width}px viewport`);
      assert(sheetBounds.bottom <= sheetBounds.viewportHeight + 1,
        `select sheet exceeds viewport height at ${width}px: ${JSON.stringify(sheetBounds)}`);
      await page.locator('#selectSheetCancel').click();
      assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0);
    }
    await page.setViewportSize({ width: 390, height: 844 });

    await page.locator('#panelNag .backbtn').click();

    await page.locator('.page.active .listrow[data-panel="panelSummon"]').click();
    await page.waitForFunction(() => document.querySelector('#panelSummon')?.classList.contains('show'));
    assert.equal(await page.locator('#panelSummon #r79Mode').count(), 0);
    assert.equal(await page.locator('#panelSummon #sumSession').count(), 1);
    await page.locator('#panelSummon .backbtn').click();

    await page.locator('.page.active .listrow[data-panel="panelR79"]').click();
    await page.waitForFunction(() => document.querySelector('#panelR79')?.classList.contains('show'));
    assert.equal(await page.locator('#panelR79 #r79Mode').count(), 1);
    await verifyTouchSelect(page, '#r79Mode');
    await verifyTouchSelect(page, '#r79Bit18Mode');

    assert.deepEqual(await page.locator('#panelR79 #r79Mode1TxWait option').allTextContents(), ['FAST ECHO · 0 ms', '2 ms WAIT']);
    assert.equal(await page.locator('#panelR79 #r79Mode1Controls').isVisible(), true);
    assert.equal(await page.locator('#panelR79 #r79Mode2Controls').isVisible(), false);
    await page.locator('#r79Mode1TxWait').selectOption('0');
    await page.locator('#r79Mode1DelayMs').fill('275');
    await page.locator('#r79Mode1Controls .toggle').click();
    await page.waitForFunction(() => document.querySelector('#r79Mode1DelayMs')?.disabled === true);
    await page.locator('#r79Mode1Apply').click();
    await page.waitForFunction(() => document.querySelector('#r79Mode1DelayMs')?.value === '275');
    assert(requests.some(request => request.includes('/api/r79/update?') &&
      request.includes('mode1TxWaitMode=0') && request.includes('mode1Reinject=0') &&
      request.includes('mode1DelayMs=275')));
    assert.equal(await page.locator('#panelR79 #r79Mode2DelayMs').count(), 1);
    await page.locator('#r79Mode').selectOption('2');
    await page.waitForFunction(() => document.querySelector('#r79Mode2Controls')?.classList.contains('tuUiHidden') === false);
    assert.equal(await page.locator('#panelR79 #r79Mode1Controls').isVisible(), false);
    assert.equal(await page.locator('#panelR79 #r79Mode2Controls').isVisible(), true);
    assert.equal(await page.locator('#r79Mode2DelayMs').inputValue(), '150');
    await page.locator('#r79Mode2Controls .toggle').click();
    await page.waitForFunction(() => document.querySelector('#r79Mode2DelayMs')?.disabled === false);
    await page.locator('#r79Mode2DelayMs').fill('425');
    await page.locator('#r79Mode2Apply').click();
    await page.waitForFunction(() => document.querySelector('#r79Mode2DelayMs')?.value === '425');
    await page.locator('#r79Mode').selectOption('1');
    await page.waitForFunction(() => document.querySelector('#r79Mode1Controls')?.classList.contains('tuUiHidden') === false);
    assert.equal(await page.locator('#r79Mode1TxWait').inputValue(), '0');
    assert.equal(await page.locator('#r79Mode1Reinject').isChecked(), false);
    assert.equal(await page.locator('#r79Mode1DelayMs').isDisabled(), true);
    assert.equal(await page.locator('#r79Mode1DelayMs').inputValue(), '275');
    assert.equal(await page.locator('#panelR79 #sumSession').count(), 0);
    await page.locator('#panelR79 .backbtn').click();
    if (process.env.DASHBOARD_SCREENSHOT_DIR) await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'settings.png'), fullPage: true });

    await page.getByRole('button', { name: 'LAB' }).click();
    assert.equal(await page.locator('.page.active').getAttribute('data-page'), 'lab');
    for (const id of ['panelLabCanARx','panelLabLaneGraph','panelLabParkedInjection','panelLabUlcMonitor']) {
      assert.equal(await page.locator('#'+id).count(), 0, `removed LAB panel ${id}`);
      assert.equal(await page.locator(`[data-panel="${id}"]`).count(), 0, `removed LAB menu ${id}`);
    }
    assert.equal(await page.locator('#labUlcBlind').count(), 1, 'blind spot injection control survives monitor removal');
    assert.equal(await page.locator('#panelLabVisionControl').count(), 0, 'Visual Speed Control no longer belongs to LAB');
    assert.equal(await page.locator('#panelVisionControl').count(), 1, 'Visual Speed Control production panel remains available');
    if (process.env.DASHBOARD_SCREENSHOT_DIR) await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'lab.png'), fullPage: true });

    await page.getByRole('button', { name: 'Devices' }).click();
    assert.equal(await page.locator('.page.active').getAttribute('data-page'), 'devices');
    assert.equal(await page.locator('.device .devmeta').textContent(), 'AA:BB:CC:DD:EE:FF');
    assert.equal(await page.locator('.device [data-f="single"]').textContent(), 'Left Blinker');
    assert.match(await page.locator('.device .devEvents').textContent(), /27 events/);
    if (process.env.DASHBOARD_SCREENSHOT_DIR) await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'devices.png'), fullPage: true });

    // Legacy Body+Chassis exposes TSL9 (but not torque), including the
    // selectable Body 0x39B / Chassis 0x399 route.
    Object.assign(profile, {
      profile: 3,
      topology: 2,
      topologyName: 'Body + Chassis',
      canA: 'BODY',
      canB: 'CHASSIS',
      nagSupported: true,
      nagTorqueSupported: false,
      nagTsl9Supported: true,
    });
    Object.assign(nagConfig, { method: 1, tsl9Sequence: 0, tsl9Window: 0,
      tsl9InputMode: 0, tsl9LegacyRoute: 0,
      tsl9LegacyRouteSelectable: true });
    Object.assign(homeSnapshot.fast.blink, { dasState: 255, dasStateValid: false });
    Object.assign(homeSnapshot.fast.r79, {
      txState: 'WAIT TEMPLATE',
      txReason: 'Stock MUX1 not received',
      gearName: 'UNKNOWN',
    });
    await page.reload({ waitUntil: 'domcontentloaded' });
    await page.waitForFunction(() => document.querySelector('#apMode')?.textContent === 'AP Unknown');
    assert.equal(await page.locator('#apMode').textContent(), 'AP Unknown');
    assert.equal(await page.locator('#homeNagSignalLabel').textContent(), 'Hands-On state');
    assert.equal(await page.locator('#torque').textContent(), '4');
    if (process.env.DASHBOARD_SCREENSHOT_DIR) await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'ap-unknown.png') });

    await page.getByRole('button', { name: 'LAB' }).click();
    await page.waitForFunction(() => document.querySelector('#r79TxBig')?.textContent === 'Waiting');
    assert.equal(await page.locator('#r79TxBig').textContent(), 'Waiting');
    if (process.env.DASHBOARD_SCREENSHOT_DIR) {
      await page.waitForTimeout(700);
      await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'r79-waiting.png') });
    }

    await page.getByRole('button', { name: 'Settings' }).click();
    await page.locator('#nagSettingsRow').click();
    await page.waitForFunction(() => document.querySelector('#panelNag')?.classList.contains('show'));
    await page.waitForFunction(() => document.querySelector('#nagMethod option[value="0"]')?.disabled === true);
    assert.equal(await page.locator('#nagMethod option[value="0"]').evaluate(option => option.disabled), true);
    assert.equal(await page.locator('#nagMethod option[value="1"]').evaluate(option => option.disabled), false);
    assert.equal(await page.locator('#tsl9SequenceWrap').isVisible(), true);
    assert.equal(await page.locator('#tsl9WindowWrap').isVisible(), true);
    assert.equal(await page.locator('#nagTorqueModeControls').count(), 1);
    assert.equal(await page.locator('#nagTorqueModeControls').isVisible(), false);
    assert.equal(await page.locator('#nagTorqueLinks').count(), 1);
    assert.equal(await page.locator('#nagTorqueLinks').isVisible(), false);
    assert.equal(await page.locator('#panelNag .listrow[data-panel="panelApRightScroll"]').count(), 0);
    assert.equal(await page.locator('#tsl9ScrollAssistWrap').count(), 0);
    assert.equal(await page.locator('#tsl9InputModeWrap').isVisible(), true);
    assert.equal(await page.locator('#tsl9LegacyRouteWrap').isVisible(), true);
    assert.equal(await page.locator('#tsl9LegacyRoute').inputValue(), '0');
    await page.locator('#tsl9LegacyRoute').selectOption('1');
    await page.waitForFunction(() => document.querySelector('#tsl9LegacyRoute')?.value === '1');
    assert(requests.some(request => request === 'POST /api/nag/update?tsl9LegacyRoute=1'));
    assert.equal(await page.locator('#tsl9Sequence').inputValue(), '0');
    await page.locator('#tsl9Sequence').selectOption('1');
    await page.waitForFunction(() => document.querySelector('#tsl9Sequence')?.value === '1');
    assert(requests.some(request => request === 'POST /api/nag/update?tsl9Sequence=1'));
    assert.equal(await page.locator('#tsl9Window').isChecked(), false);
    await page.locator('#tsl9WindowWrap label.toggle').click();
    await page.waitForFunction(() => document.querySelector('#tsl9Window')?.checked === true);
    assert(requests.some(request => request === 'POST /api/nag/update?tsl9Window=1'));
    if (process.env.DASHBOARD_SCREENSHOT_DIR) {
      await page.keyboard.press('Escape');
      await page.waitForTimeout(700);
      await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'body-chassis-tsl9.png') });
    }
    assert.deepEqual(pageErrors, []);

    await page.addInitScript(() => {
      Object.defineProperty(window, 'PointerEvent', { configurable: true, value: undefined });
      const nativeSelectFocus = HTMLSelectElement.prototype.focus;
      window.__nativeSelectFocusCalls = 0;
      HTMLSelectElement.prototype.focus = function (...args) {
        window.__nativeSelectFocusCalls += 1;
        return nativeSelectFocus.apply(this, args);
      };
    });
    const fallbackErrorStart = pageErrors.length;
    await page.goto(`${testOrigin}/`, { waitUntil: 'domcontentloaded' });
    await page.waitForFunction(() => document.querySelector('#apMode')?.textContent === 'AP Unknown');
    await page.getByRole('button', { name: 'Settings' }).click();
    await page.locator('#nagSettingsRow').click();
    await page.waitForFunction(() => document.querySelector('#panelNag')?.classList.contains('show'));
    await verifyTouchSelect(page, '#tsl9Sequence', true);
    assert.equal(await page.evaluate(() => window.__nativeSelectFocusCalls), 0);
    assert.deepEqual(pageErrors.slice(fallbackErrorStart), []);
    await page.close();
  } finally {
    await browser.close();
  }
}

main().then(
  () => process.stdout.write('dashboard 2027 browser behavior: PASS\n'),
  error => {
    process.stderr.write(`${error.stack || error}\n`);
    process.exitCode = 1;
  },
);
