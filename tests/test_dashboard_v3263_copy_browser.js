// Production source and embedded HTML regression. Synthetic fixture only; no controller access.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const { launchBrowser } = require('./browser_test_runtime');
const root = path.resolve(__dirname, '..');
const output = process.env.DASHBOARD_COPY_SCREENSHOT_DIR || '/private/tmp/t2can-v3264-copy-browser';
const descriptions = [
  {
    "id": "setNagDesc",
    "assignments": 1,
    "text": "Reduce steering-wheel reminders during Autopilot."
  },
  {
    "id": "isaSuppressionDesc",
    "assignments": 2,
    "text": "Reduce ISA warning chimes during Autopilot."
  },
  {
    "id": "driverMonitoringDesc",
    "assignments": 2,
    "text": "Request cabin-camera monitoring off during Autopilot."
  },
  {
    "panel": "panelVisionControl",
    "text": "Request visual speed control off."
  },
  {
    "id": "setBlinkDesc",
    "assignments": 2,
    "text": "Signal automatically for NOA lane changes."
  },
  {
    "id": "setSumDesc",
    "assignments": 2,
    "text": "View the current Summon session and status."
  },
  {
    "panel": "panelR79",
    "text": "Choose the R79 override mode and timing."
  },
  {
    "panel": "panelCountry",
    "text": "Change country and map signals."
  },
  {
    "panel": "panelTlssc",
    "text": "Request traffic-light and stop-sign control during AP."
  },
  {
    "id": "setAlcDesc",
    "assignments": 2,
    "text": "Auto Blinker is inactive while Confirm-Free Lane Change is on."
  },
  {
    "id": "pedalMapSettingsDesc",
    "assignments": 1,
    "text": "Change acceleration response for this drive."
  },
  {
    "id": "apDriveProfileDesc",
    "assignments": 1,
    "text": "Use Chill acceleration and chosen regen during AP."
  },
  {
    "id": "s3xyFeatureDesc",
    "assignments": 1,
    "text": "Use S3XY buttons for assigned vehicle actions."
  },
  {
    "id": "labMenuDesc",
    "assignments": 1,
    "text": "Show experimental controls and capture tools."
  },
  {
    "panel": "panelLabR79ApControl",
    "text": "Block or delay R79 during AP."
  },
  {
    "panel": "panelLabR79",
    "text": "View R79 send status."
  },
  {
    "panel": "panelLabSummonHeartbeat",
    "text": "Test Summon heartbeat signals."
  },
  {
    "panel": "panelLabAutoLaneChange",
    "text": "Test lane-change enable signals."
  },
  {
    "panel": "panelLabDriverWindow",
    "text": "Test window opening in Park."
  },
  {
    "panel": "panelLabCapture",
    "text": "Record and download CAN signals."
  },
  {
    "panel": "panelLabDriverMonitor",
    "text": "Record driver-monitoring signals."
  },
  {
    "id": "doorCancelDesc",
    "assignments": 1,
    "text": "Cancel a lane change with the front door-open button."
  },
  {
    "id": "bannedCarDesc",
    "assignments": 1,
    "text": "Show restore controls for already-banned vehicles."
  },
  {
    "id": "tlsscRestoreDesc",
    "assignments": 1,
    "text": "Try the TLSSC restore override on a banned vehicle."
  }
];
const report = { status: 'running', variants: {}, rendering: [], pageErrors: [], requests: [] };
function array(header, name) {
  const match = header.match(new RegExp(name + '\\[\\][^{]*\\{([\\s\\S]*?)\\};'));
  assert(match, name + ' byte array missing');
  return Buffer.from(match[1].match(/0x[\da-f]+/gi).map(v => parseInt(v, 16)));
}
const source = fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8');
const embedded = zlib.gunzipSync(array(fs.readFileSync(path.join(root, 'index_html.h'), 'utf8'), 'INDEX_HTML_GZ')).toString();
const fontHeader = fs.readFileSync(path.join(root, 'lab_fonts.h'), 'utf8');
const fonts = {
  '/fonts/geist-lab.woff2': array(fontHeader, 'LAB_GEIST_FONT'),
  '/fonts/geist-mono-lab.woff2': array(fontHeader, 'LAB_GEIST_MONO_FONT'),
};
async function navigate(page, target) {
  await page.evaluate(name => { closePanels(); if (name.startsWith('panel')) openPanel(name); else showPage(name); forcePoll(); }, target);
  await page.waitForTimeout(370);
}
async function refresh(page) {
  await page.evaluate(async () => {
    await Promise.all([loadNag(), fetchBlink(), fetchSum(), fetchLab(), fetchPedalMap(), fetchDriverMonitoringControl(), fetchIsaSuppressionControl(), fetchCountryOverrideLab(), fetchVisionControlLab()]);
    forcePoll();
  });
}
async function snapshotDescriptions(page) {
  return page.evaluate(items => items.map(item => {
    const el = item.id ? document.getElementById(item.id) : document.querySelector('.listrow[data-panel="'+item.panel+'"] .listdesc');
    return { key: item.id || item.panel, text: el?.textContent.trim() };
  }), descriptions);
}
async function assertDescriptions(page) {
  assert.deepEqual(await snapshotDescriptions(page), descriptions.map(item => ({ key: item.id || item.panel, text: item.text })));
}
async function geometry(page, target) {
  return page.evaluate(name => {
    const root = name.startsWith('panel') ? document.getElementById(name) : document.querySelector('.page.active');
    const issues = [];
    if (root.scrollWidth > root.clientWidth + 1 || document.documentElement.scrollWidth > innerWidth + 1) issues.push('horizontal overflow');
    for (const row of root.querySelectorAll('.listrow, .settingToggleRow')) {
      const rr = row.getBoundingClientRect(), rs = getComputedStyle(row);
      if (!rr.width || !rr.height || rs.display === 'none') continue;
      const desc = row.querySelector('.listdesc');
      if (!desc) continue;
      const dr = desc.getBoundingClientRect(), range = document.createRange(); range.selectNodeContents(desc);
      const rects = [...range.getClientRects()].filter(r => r.width && r.height);
      const control = row.querySelector('.toggle, .rowRight, .value');
      const cr = control?.getBoundingClientRect();
      if (desc.scrollWidth > desc.clientWidth + 1 || rects.some(r => r.left < rr.left - 1 || r.right > rr.right + 1)) issues.push('description overflow: '+desc.textContent);
      if (cr && rects.some(r => r.left < cr.right && r.right > cr.left && r.top < cr.bottom && r.bottom > cr.top)) issues.push('description/control overlap: '+desc.textContent);
      if (rects.some(r => r.bottom > rr.bottom + 1)) issues.push('description vertical clipping: '+desc.textContent);
      if (name === 'settings' && new Set(rects.map(r => Math.round(r.top))).size > 2) issues.push('Settings description exceeds two lines: '+desc.textContent);
    }
    return issues;
  }, target);
}
async function intervalMetrics(page) {
  return page.locator('#torqueRightScrollInterval').evaluate(input => {
    const field = input.closest('.field'), label = field.querySelector('label');
    const ir = input.getBoundingClientRect(), fr = field.getBoundingClientRect(), lr = label.getBoundingClientRect();
    return { height: fr.height, inputWidth: ir.width, textAlign: getComputedStyle(input).textAlign, rightInset: fr.right-ir.right,
      sameLine: lr.top < ir.bottom && lr.bottom > ir.top, overlap: lr.right > ir.left, min: input.min, max: input.max, step: input.step };
  });
}
async function saveChecks(page, variant) {
  await navigate(page, 'panelNag');
  for (const [method, input, key] of [
    [0,'torqueRightScrollInterval','torqueRightScrollIntervalSeconds'],
    [1,'tsl9RightPeriodicInterval','tsl9RightPeriodicIntervalSeconds'],
  ]) {
    await page.locator('#nagMethod').selectOption(String(method));
    await page.waitForFunction(expected => nagCfg.method === expected, method);
    if (method === 1) { await page.locator('#tsl9InputMode').selectOption('1'); await page.waitForFunction(() => nagCfg.tsl9InputMode === 1); }
    const inputEl = page.locator('#'+input);
    assert.equal(await inputEl.getAttribute('min'), '1'); assert.equal(await inputEl.getAttribute('max'), '600');
    assert.equal(await inputEl.getAttribute('step'), '1');
    for (const value of [1,600,45,30]) {
      await page.evaluate(() => window.__t2CopyRequests = []);
      await inputEl.fill(String(value)); await inputEl.dispatchEvent('change');
      await page.waitForFunction(({key,value,input}) => nagCfg[key] === value && document.getElementById(input).value === String(value), {key,value,input});
      const requests = await page.evaluate(() => window.__t2CopyRequests.filter(r => r.method === 'POST'));
      assert.deepEqual(requests, [{method:'POST',url:'/api/nag/update?'+key+'='+value}], 'Each valid interval sends its existing API field once');
      await refresh(page); assert.equal(await inputEl.inputValue(), String(value));
      report.requests.push({variant,input,value,requests});
    }
    for (const value of ['0','601','1.5','']) {
      await page.evaluate(() => window.__t2CopyRequests = []);
      await inputEl.fill(value); await inputEl.dispatchEvent('change');
      assert.equal(await inputEl.inputValue(), '30', 'Invalid interval restores saved value');
      assert.deepEqual(await page.evaluate(() => window.__t2CopyRequests.filter(r => r.method === 'POST')), []);
    }
  }
}
async function validate(browser, base, variant) {
  const page = await browser.newPage({viewport:{width:390,height:1100}});
  page.on('pageerror', error => report.pageErrors.push({variant,error:error.message}));
  const fontRequests = new Set();
  await page.addInitScript({path:path.join(__dirname,'dashboard_v3263_copy_fixture.js')});
  await page.route('**/*', async route => {
    const url = new URL(route.request().url());
    if (url.origin !== base) return route.abort();
    const font=fonts[url.pathname];
    if(font) fontRequests.add(url.pathname);
    return route.fulfill({status:200,contentType:font?'font/woff2':'text/html; charset=utf-8',body:font || (variant==='embedded'?embedded:source)});
  });
  try {
    await page.goto(base+'/'+variant+'?allFeatures=1');
    await page.waitForFunction(() => typeof universalBootReady !== 'undefined' && universalBootReady);
    // Fonts are used on LAB/details; the Home screen uses the system font.
    await navigate(page,'lab');
    await page.evaluate(async () => { await Promise.all([document.fonts.load('400 14px \"Geist\"'),document.fonts.load('400 12px \"Geist Mono\"')]); await document.fonts.ready; });
    assert.deepEqual([...fontRequests].sort(),Object.keys(fonts).sort(),'Both firmware font URLs must be requested');
    assert.deepEqual(await page.evaluate(() => [...document.fonts].map(face => ({family:face.family,status:face.status})).sort((a,b)=>a.family.localeCompare(b.family))),[{family:'Geist',status:'loaded'},{family:'Geist Mono',status:'loaded'}],'Both actual firmware WOFF2 fonts must load');
    await navigate(page,'settings'); await assertDescriptions(page);
    for (const ap of ['OFF','AUTOSTEER','NOA']) {
      await page.evaluate(value => window.__t2PreviewSetAp(value), ap); await refresh(page); await assertDescriptions(page);
    }
    for (const [key,id] of [['s3xy','s3xyFeatureToggle'],['lab','labMenuToggle'],['doorCancel','doorCancelToggle'],['banned','bannedCarToggle'],['tlsscRestore','tlsscRestoreToggle'],['apDriveProfile','apDriveProfileToggle']]) {
      for (const value of [false,true]) {
        await page.locator('#'+id).evaluate((input,value) => { input.checked=value; input.dispatchEvent(new Event('change',{bubbles:true})); },value);
        // Production enable flows require their existing confirmation dialogs.
        if(value && key==='banned') await page.locator('#confirmBannedEnable').evaluate(button=>button.click());
        if(value && key==='tlsscRestore') await page.evaluate(() => { const input=document.getElementById('restoreConfirmCheck'); input.checked=true; input.dispatchEvent(new Event('change')); document.getElementById('confirmRestoreEnable').click(); });
        await page.waitForFunction(({key,value}) => window.__t2PreviewState.features[key] === value, {key,value});
        await page.waitForFunction(({id,value}) => document.getElementById(id).checked === value, {id,value});
        if (key === 'lab' || key === 's3xy') assert.equal(await page.evaluate(key=>key==='lab'?labEnabled:s3xyEnabled,key),value,'Production feature global must match saved setting');
        if (key === 'lab' || key === 's3xy') assert.equal(await page.locator('#'+(key==='lab'?'labNavBtn':'devicesNavBtn')).evaluate(button=>button.classList.contains('hiddenByProfile')),!value,'Production applyFeatures must update navigation visibility');
        await refresh(page); await assertDescriptions(page);
      }
    }
    for (const width of [320,390,430]) for (const theme of ['light','dark']) {
      await page.setViewportSize({width,height:1100});
      await page.evaluate(mode => window.t2Theme.setMode(mode), theme); await page.waitForTimeout(650);
      for (const target of ['settings','lab','panelNag','panelUlc']) {
        await navigate(page,target);
        const issues = await geometry(page,target);
        assert.deepEqual(issues, [], variant+'/'+width+'/'+theme+'/'+target);
        const item = {variant,width,theme,target,issues};
        if (target === 'panelNag') {
          const metrics = await intervalMetrics(page); item.interval = metrics;
          assert.equal(metrics.height,43,'Compact Interval field height');
          assert.equal(metrics.textAlign,'right'); assert(metrics.sameLine && !metrics.overlap,'Label/value must share one line without overlap');
          assert(metrics.inputWidth >= 60 && metrics.rightInset <= 14,'Right aligned usable input');
        }
        if (target === 'panelUlc') assert.equal(await page.locator('#labUlcNoConfirmRow .settingToggleRow:first-child .listdesc').textContent(), 'Auto Blinker is inactive while Confirm-Free Lane Change is on.');
        report.rendering.push(item);
        await page.screenshot({path:path.join(output,variant+'-'+target+'-'+width+'-'+theme+'.png'),fullPage:false});
      }
    }
    await saveChecks(page,variant);
    assert.deepEqual(await page.evaluate(() => window.__t2DemoUnhandled),[],'No unhandled APIs before reload');
    await page.reload(); await page.waitForFunction(() => typeof universalBootReady !== 'undefined' && universalBootReady);
    await refresh(page); await assertDescriptions(page);
    assert.deepEqual(await page.evaluate(() => window.__t2DemoUnhandled),[],'All dashboard APIs handled by fixture');
    report.variants[variant] = {fixedDescriptions:24,apStates:3,featureToggles:12,layoutScreens:24,validIntervalSaves:8,invalidIntervalEdits:8,reloadStable:true};
  } finally { await page.close(); }
}
(async () => {
  fs.mkdirSync(output,{recursive:true});
  let browser;
  try {
    for (const html of [source,embedded]) {
      assert.doesNotMatch(html,/__t2Demo|__t2Preview|t2-demo-control|mock\.js|bridge\.js|Preview-only API simulator/,'Production must not contain demo fixture/bridge');
      for (const title of ['Summon Status','Pedal Response','Lane Change','AP Accel / Regen','Country / Map','Data Refresh Interval','Reset Feature Settings','Factory Reset','Auto Speed Scroll']) assert(html.includes(title),title+' missing');
    }
    const base='http://t2can-copy.test';
    // macOS single-process Chromium exits when its last context closes.
    // Each variant owns a fresh browser so source/embedded runs are independent.
    for (const variant of ['source','embedded']) {
      browser=await launchBrowser();
      await validate(browser,base,variant);
      await browser.close(); browser=null;
    }
    assert.deepEqual(report.pageErrors,[]);
    report.status='passed';
    process.stdout.write('v3.28.0 copy source/embedded: PASS (48 fixed descriptions, 48 mobile/theme screens, 16 valid saves, 16 invalid edits)\n');
  } catch(error) { report.status='failed'; report.failure=error.stack; process.stderr.write(error.stack+'\n'); process.exitCode=1; }
  finally {
    if(browser) await browser.close();
    fs.writeFileSync(path.join(output,'validation.json'),JSON.stringify(report,null,2)+'\n');
  }
})();
