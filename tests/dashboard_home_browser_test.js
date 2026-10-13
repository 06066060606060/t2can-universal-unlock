// Run with Playwright available through NODE_PATH; mocks never contact the board.
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const {chromium} = require('playwright');
const root = path.resolve(__dirname, '..');
const editable = fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8');
const source = process.argv.includes('--embedded')
  ? require('zlib').gunzipSync(Buffer.from([...fs.readFileSync(path.join(root,'index_html.h'),'utf8').matchAll(/0x([0-9A-F]{2})/g)].map(m=>parseInt(m[1],16)))).toString('utf8')
  : editable;
for (const style of source.matchAll(/<style\b[^>]*>([\s\S]*?)<\/style>/gi))
  assert(!/<style\b/.test(style[1]), 'style elements must not be nested');
const baseline = process.argv.includes('--baseline');
const output = path.join(root, '.build', 'dashboard-fix');
fs.mkdirSync(output, {recursive: true});
const profile = {profile:3, topology:4, turn:1, setupMode:false, canA:'BODY', canB:'CHASSIS', canC:'PARTY', nagSupported:true, nagTorqueSupported:true, nagTsl9Supported:true, advancedEapSupported:true, pedalMapSupported:true};
const config = {enabled:true, method:0, mode:1, targetId:0x370, apStateId:0x399, steeringId:0x370, hoRatePct:50, burstMs:500, pauseMs:500, torque:[], tsl9EffectiveRoute:'BODY_0x39B'};
const traffic = {bodyTrafficSeen:true, bodyTrafficOnline:true, bodyTrafficAgeMs:10, chassisTrafficSeen:true, chassisTrafficOnline:true, chassisTrafficAgeMs:20, partyTrafficSeen:true, partyTrafficOnline:true, partyTrafficAgeMs:30, bodyState:1, bodyReady:true, chassisState:1, chassisReady:true, partyState:1, partyReady:true};
const fast = {nag:{torque:0, apActive:true}, blink:{enabled:true, displayState:5, displayStateValid:true, noaSessionStateName:'READY'}, cantraffic:traffic, r79:{txState:'ACTIVE', txReason:'AP engaged', txOk:733, txFail:10, lastTxAgeMs:76, lastTxValid:true}};
const slow = {blink:{enabled:true}, summon:{tlssc:false}, s3xy:{bluetoothEnabled:false}, lab3f8:{alcMode:0}};
const counterBus = {rxFrames:1234,rxErrors:1,txQueued:42,txBusy:2,txErrors:3,recoveries:4,recoveryErrors:1,rxOverflowEvents:7};
const systemStats = {fwVersion:'test',uptimeS:10,canPerformance:{body:counterBus,chassis:{...counterBus,recoveries:5},party:{...counterBus,recoveries:6,rx370:120,tx370:30}}};
// Tie the fixture to production serializer keys so a frontend-only rename fails.
const api = fs.readFileSync(path.join(root, 'web_api.h'), 'utf8');
const serializer = api.slice(api.indexOf('static String homeFastSnapshotToJson()'),api.indexOf('static String homeSlowSnapshotToJson()'));
for (const key of Object.keys(traffic).filter(key=>key.includes('Traffic')))
  assert(serializer.includes('"'+key+'"'), 'missing HOME API key: '+key);
async function main() {
  const browser = await chromium.launch({channel:process.env.DASHBOARD_BROWSER_CHANNEL || 'msedge', headless:true});
  try {
    const context = await browser.newContext({viewport:{width:360,height:780}});
    await context.addInitScript(() => localStorage.setItem('tmrDashboardTheme', 'light'));
    let failNagUpdate = false, updates = [], snapshotRequests = 0, stalledSnapshot = false;
    const page = await context.newPage();
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    page.on('dialog', dialog => dialog.dismiss());
    await page.route('http://tmr.test/**', async route => {
      const url = new URL(route.request().url());
      const reply = body => route.fulfill({status:200, contentType:'application/json', body:JSON.stringify(body)});
      if (url.pathname === '/') return route.fulfill({status:200, contentType:'text/html', body:source});
      if (url.pathname.startsWith('/fonts/')) return route.fulfill({status:404, body:''});
      if (url.pathname === '/api/profile/status') return reply(profile);
      if (url.pathname === '/api/features/status') return reply({lab:true, s3xy:false});
      if (url.pathname === '/api/nag/config') return reply(config);
      if (url.pathname === '/api/nag/update') {
        updates.push(url.searchParams.get('enabled'));
        if (failNagUpdate) return route.fulfill({status:409, contentType:'application/json', body:'{"error":"test rejected"}'});
        config.enabled = url.searchParams.get('enabled') === '1';
        return reply(config);
      }
      if (url.pathname === '/api/nag/stats') return reply({apActive:true, torque:0, canAState:1});
      if (url.pathname === '/api/system/stats') return reply(systemStats);
      if (url.pathname === '/api/snapshot') {
        ++snapshotRequests;
        if (stalledSnapshot) return; // Simulate a request that never completes.
        return reply({fast, slow});
      }
      return reply({});
    });
    await page.goto('http://tmr.test/');
    await page.waitForFunction(() => universalBootReady && document.getElementById('nagModeSub').textContent === 'Mode B');
    await page.waitForFunction(() => document.getElementById('apMode').textContent === 'Navigating');
    const inspect = () => page.evaluate(() => {
      const bounds = selector => {const r=document.querySelector(selector).getBoundingClientRect();return {x:r.x,y:r.y,width:r.width,height:r.height,bottom:r.bottom,right:r.right}};
      return {disabled:document.getElementById('nagToggle').disabled, traffic:['A','B','C'].map(bus=>document.getElementById('can'+bus+'Traffic').textContent), cards:['A','B','C'].map(bus=>bounds('.homeCan'+bus+'Card')), r79:bounds('.homeR79Card'), viewport:innerWidth, pageWidth:document.documentElement.scrollWidth};
    });
    const state = await inspect();
    if (baseline) {
      console.log(JSON.stringify({baseline:state, errors},null,2));
      await page.screenshot({path:path.join(output,'home-before.png'),fullPage:true});
      return;
    }
    assert.equal(state.disabled,false,'supported NAG toggle must stay enabled after loading config');
    assert.deepEqual(state.traffic,['LIVE','LIVE','LIVE'],'use real firmware traffic field names');
    const capabilities = await page.evaluate(() => ({hw3:r79Hw3ProfileSupported(), actions:availableActionOptions().map(([key])=>key),route:nagTsl9RouteInfo().route}));
    assert.equal(capabilities.hw3,true,'Legacy profile recognizes the fixed three-bus topology');
    for (const action of ['performance_mode','auto_blinker_toggle','left_blinker','right_blinker'])
      assert(capabilities.actions.includes(action),'available capability action: '+action);
    assert.equal(capabilities.route,'Body CAN A · 0x39B','NAG route comes from firmware configuration');
    const assertLayout = state => {
      assert(state.cards.every(card=>Math.abs(card.y-state.cards[0].y)<1),'three bus cards share a row');
      assert(state.cards[0].right <= state.cards[1].x+1 && state.cards[1].right <= state.cards[2].x+1,'bus cards do not overlap');
      assert(state.r79.y >= Math.max(...state.cards.map(card=>card.bottom)), 'R79 follows all three bus cards');
      assert.equal(state.pageWidth,state.viewport,'no horizontal overflow');
    };
    assertLayout(state);
    for (const width of [320,360,390,520,800]) {
      await page.setViewportSize({width,height:780});
      assertLayout(await inspect());
      assert(await page.locator('.homeBusVal').evaluateAll(nodes=>nodes.every(n=>n.scrollWidth<=n.clientWidth+1)),`bus labels fit at ${width}px`);
    }
    await page.setViewportSize({width:360,height:780});
    const toggle = async (id, checked) => {
      if (await page.locator('#'+id).isChecked() !== checked)
        await page.locator('label.toggle').filter({has:page.locator('#'+id)}).click();
    };
    await toggle('nagToggle', false);
    await page.waitForFunction(() => nagCfg.enabled === false);
    assert.equal(await page.locator('#nagToggle2').isChecked(),false);
    await toggle('nagToggle', true);
    await page.waitForFunction(() => nagCfg.enabled === true);
    assert.deepEqual(updates,['0','1']);
    // The settings toggle shares the same server-backed state.
    await page.evaluate(() => openPanel('panelNag'));
    await toggle('nagToggle2', false);
    await page.waitForFunction(() => nagCfg.enabled === false);
    assert.equal(await page.locator('#nagToggle').isChecked(),false);
    failNagUpdate = true;
    await toggle('nagToggle2', true);
    await page.waitForFunction(() => document.getElementById('nagToggle2').checked === false);
    failNagUpdate = false;
    await toggle('nagToggle2', true);
    await page.waitForFunction(() => nagCfg.enabled === true);
    await page.evaluate(() => closePanels());
    // Real fetch/render path must finish after retirement of detailed recovery rows.
    await page.evaluate(async () => {openPanel('panelDiag');await fetchSys()});
    assert.equal(await page.locator('#canPerf_body_Rx').textContent(),'1234');
    assert.equal(await page.locator('#canPerf_body_Fail').textContent(),'1 / 5');
    assert.equal(await page.locator('#canPerf_chassis_Recovery').textContent(),'5 / 1');
    assert.equal(await page.locator('#canPerf_party_Recovery').textContent(),'6 / 1');
    assert.equal(await page.locator('#canPerf_party_Rx370').textContent(),'120');
    assert.equal(await page.locator('#canPerf_party_Tx370').textContent(),'30');
    assert.equal(await page.locator('#sysUptime').textContent(),'10s');
    assert.equal(await page.locator('#sysMcpBusOffSnap').count(),0);
    for (const width of [320,360,390]) {
      await page.setViewportSize({width,height:780});
      assert(await page.locator('#panelDiag').evaluate(panel=>panel.scrollWidth<=panel.clientWidth+1),'diagnostics fit mobile viewport');
    }
    await page.setViewportSize({width:360,height:780});
    await page.screenshot({path:path.join(output,'can-counters-after.png'),fullPage:true});
    await page.locator('section.card').filter({has:page.locator('#canPerf_party_Rx')}).screenshot({path:path.join(output,'party-counters-after.png')});
    await page.evaluate(() => closePanels());
    // Missing configuration gets loaded before a user change, rather than ignored.
    await page.evaluate(() => {nagCfg=null});
    await toggle('nagToggle', false);
    await page.waitForFunction(() => nagCfg?.enabled === false);
    await toggle('nagToggle', true);
    await page.waitForFunction(() => nagCfg.enabled === true);
    // Exercise stale/no-frame states using the actual serializer schema.
    const staleTraffic = await page.evaluate(snapshot => {
      renderHomeFast(snapshot);
      return ['B','C'].map(bus=>document.getElementById('can'+bus+'Traffic').textContent);
    }, {...fast,cantraffic:{...traffic,chassisTrafficOnline:false,chassisTrafficAgeMs:2000,partyTrafficSeen:false,partyTrafficOnline:false}});
    assert.deepEqual(staleTraffic,['NO DATA','WAITING']);
    await page.evaluate(snapshot=>renderHomeFast(snapshot),fast);
    const controllerStates = await page.evaluate(snapshot=>{
      renderHomeFast(snapshot);
      return ['A','B','C'].map(bus=>document.getElementById('can'+bus+'Badge').textContent);
    },{...fast,cantraffic:{...traffic,bodyState:0,bodyReady:false,chassisState:2,partyState:1}});
    assert.deepEqual(controllerStates,['STOPPED','BUS OFF','RUNNING']);
    await page.evaluate(snapshot=>renderHomeFast(snapshot),fast);
    // Unsupported profiles must remain disabled; supported ones can recover.
    await page.evaluate(() => {window.__tmrNagSupported=false;renderNagCfg()});
    assert.equal(await page.locator('#nagToggle').isDisabled(),true);
    await page.evaluate(() => {window.__tmrNagSupported=true;renderNagCfg()});
    // Timed-out snapshot must release the guard and allow polling to reconnect.
    stalledSnapshot = true;
    const beforeStall = snapshotRequests;
    await page.evaluate(() => forcePoll());
    await page.waitForFunction(() => pageSnapshotBusy);
    await page.waitForTimeout(4300);
    assert(snapshotRequests>beforeStall,'stalled snapshot was requested');
    stalledSnapshot = false;
    await page.waitForFunction(() => !pageSnapshotBusy,{},{timeout:6000});
    await page.evaluate(() => forcePoll());
    await page.waitForFunction(() => document.querySelector('[data-conn]').textContent === 'Connected' && !pageSnapshotBusy);
    await page.screenshot({path:path.join(output,'home-after.png'),fullPage:true});
    await page.evaluate(() => window.t2Theme.setMode('dark'));
    await page.waitForTimeout(700); // Let the dashboard's theme color transitions finish.
    await page.screenshot({path:path.join(output,'home-after-dark.png'),fullPage:true});
    assert.deepEqual(errors,[],'no uncaught browser errors');
    console.log('PASS: NAG home/settings toggles, rejected update, missing config, three-bus API states, responsive layout, polling timeout/reconnect; no browser errors');
  } finally {await browser.close()}
}
main().catch(error=>{console.error(error);process.exitCode=1});
