// Catches a telemetry refresh replacing persistent BUS OFF evidence with legacy age-only text.
// Runs the production source and embedded HTML, real fonts and real poll/timer handlers.
// API fixtures are synthetic: no controller, serial or CAN access.
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const zlib = require('node:zlib');
const { launchBrowser } = require('./browser_test_runtime');
const root = path.resolve(__dirname, '..');
const output = process.env.DASHBOARD_DIAGNOSTICS_SCREENSHOT_DIR || '/private/tmp/t2can-v3264-diag-ui';
const ids = ['sysMcpBusOffSnap','sysTwaiBusOffAge','sysTwaiBusOffSnap','sysCanATxTrace','sysCanBTxTrace'];
function array(header, name) {
  const match = header.match(new RegExp(name + '\\[\\][^{]*\\{([\\s\\S]*?)\\};'));
  assert(match, name + ' byte array missing');
  return Buffer.from(match[1].match(/0x[\da-f]+/gi).map(v => parseInt(v, 16)));
}
const source = fs.readFileSync(path.join(root, 'dashboard_source.html'), 'utf8');
const embedded = zlib.gunzipSync(array(fs.readFileSync(path.join(root,'index_html.h'),'utf8'),'INDEX_HTML_GZ')).toString();
const fontHeader = fs.readFileSync(path.join(root,'lab_fonts.h'),'utf8');
const fonts = {'/fonts/geist-lab.woff2':array(fontHeader,'LAB_GEIST_FONT'),'/fonts/geist-mono-lab.woff2':array(fontHeader,'LAB_GEIST_MONO_FONT')};
const saved = {busOffRecordBoot:'CURRENT_BOOT',busOffPersistState:'SAVED',busOffPersistError:'NONE'};
const previous = {busOffRecordBoot:'PREVIOUS_BOOT',busOffEventUptimeMs:25000,busOffPersistState:'LOADED',busOffPersistError:'NONE'};
const baseStats = {
  fwVersion:'3.27.0',uptimeS:2000,twaiStateName:'RUNNING',mcpState:0,mcpReady:true,twaiReady:true,
  canHardReinit:0,canHardReinitFail:0,canLastHardReason:0,mcpLastRecoverAgeMs:999999,
  mcpBusOffSnapshotValid:true,mcpBusOffSnapshotAgeMs:120000,
  twaiBusOffSnapshotValid:true,twaiBusOffSnapshotAgeMs:999999,twaiBusOffSnapshotRxGapMs:100,
  mcpBusOffCount:1,twaiBusOffCount:1,twaiStoppedCount:0,twaiRestartOkCount:1,twaiRestartFailCount:0,
  canATxTraceCount:4,canATxTraceBusOffOrdinal:1,canATxTraceAgeMs:120000,
  canBTxTraceCount:5,canBTxTraceBusOffOrdinal:1,canBTxTraceAgeMs:124100,
  canABusOffRecord:saved,canBBusOffRecord:previous,
  canTaskHeartbeatLastCause:1,canTaskHeartbeatLastCauseName:'CAN B',canTaskHeartbeatLastAgeAms:300,
  canTaskHeartbeatLastAgeBms:3200,canTaskHeartbeatTimeoutCountA:0,canTaskHeartbeatTimeoutCountB:1,canTaskHeartbeatTimeoutCountBoth:0,
};
// Expected user-visible values are hand-derived, independent of production formatters.
const mixedWant = [
  'CURRENT BOOT · age 120.0 s · SAVED',
  'PREVIOUS BOOT · event uptime 25.0 s · LOADED',
  'PREVIOUS BOOT · event uptime 25.0 s · LOADED · RX gap 100 ms',
  '4 frames · BUS OFF #1 · CURRENT BOOT · age 120.0 s · SAVED',
  '5 frames · BUS OFF #1 · PREVIOUS BOOT · event uptime 25.0 s · LOADED',
];
const cases = [
  {name:'mixed-current-and-previous',stats:baseStats,want:mixedWant},
  {name:'previous-boot-both',stats:{...baseStats,canABusOffRecord:previous},want:[
    'PREVIOUS BOOT · event uptime 25.0 s · LOADED','PREVIOUS BOOT · event uptime 25.0 s · LOADED',
    'PREVIOUS BOOT · event uptime 25.0 s · LOADED · RX gap 100 ms',
    '4 frames · BUS OFF #1 · PREVIOUS BOOT · event uptime 25.0 s · LOADED',
    '5 frames · BUS OFF #1 · PREVIOUS BOOT · event uptime 25.0 s · LOADED']},
  {name:'current-boot-saved',stats:{...baseStats,canBBusOffRecord:saved,twaiBusOffSnapshotAgeMs:1500},want:[
    'CURRENT BOOT · age 120.0 s · SAVED','CURRENT BOOT · age 1.50 s · SAVED','CURRENT BOOT · age 1.50 s · SAVED · RX gap 100 ms',
    '4 frames · BUS OFF #1 · CURRENT BOOT · age 120.0 s · SAVED','5 frames · BUS OFF #1 · CURRENT BOOT · age 1.50 s · SAVED']},
  {name:'current-boot-dirty',stats:{...baseStats,canABusOffRecord:{...saved,busOffPersistState:'DIRTY'},canBBusOffRecord:{...saved,busOffPersistState:'DIRTY'},twaiBusOffSnapshotAgeMs:500},want:[
    'CURRENT BOOT · age 120.0 s · DIRTY','CURRENT BOOT · age 500 ms · DIRTY','CURRENT BOOT · age 500 ms · DIRTY · RX gap 100 ms',
    '4 frames · BUS OFF #1 · CURRENT BOOT · age 120.0 s · DIRTY','5 frames · BUS OFF #1 · CURRENT BOOT · age 500 ms · DIRTY']},
  {name:'persistence-error',stats:{...baseStats,canABusOffRecord:{...saved,busOffPersistState:'ERROR',busOffPersistError:'NVS_OPEN'},canBBusOffRecord:{...previous,busOffPersistState:'ERROR',busOffPersistError:'WRITE'}},want:[
    'CURRENT BOOT · age 120.0 s · ERROR · NVS_OPEN','PREVIOUS BOOT · event uptime 25.0 s · ERROR · WRITE',
    'PREVIOUS BOOT · event uptime 25.0 s · ERROR · WRITE · RX gap 100 ms',
    '4 frames · BUS OFF #1 · CURRENT BOOT · age 120.0 s · ERROR · NVS_OPEN',
    '5 frames · BUS OFF #1 · PREVIOUS BOOT · event uptime 25.0 s · ERROR · WRITE']},
  {name:'no-snapshot',stats:{...baseStats,mcpBusOffSnapshotValid:false,twaiBusOffSnapshotValid:false,mcpBusOffSnapshotAgeMs:999999,canATxTraceCount:0,canBTxTraceCount:0},want:['—','—','—','—','—']},
  {name:'new-current-snapshot',stats:{...baseStats,canABusOffRecord:saved,canBBusOffRecord:saved,mcpBusOffSnapshotAgeMs:250,twaiBusOffSnapshotAgeMs:750,twaiBusOffSnapshotRxGapMs:20,canATxTraceCount:8,canATxTraceBusOffOrdinal:2,canBTxTraceCount:9,canBTxTraceBusOffOrdinal:3,twaiStateName:'BUS OFF',mcpBusOffCount:2,twaiBusOffCount:3,twaiStoppedCount:2,twaiRestartOkCount:4,canTaskHeartbeatTimeoutCountB:2},want:[
    'CURRENT BOOT · age 250 ms · SAVED','CURRENT BOOT · age 750 ms · SAVED','CURRENT BOOT · age 750 ms · SAVED · RX gap 20 ms',
    '8 frames · BUS OFF #2 · CURRENT BOOT · age 250 ms · SAVED','9 frames · BUS OFF #3 · CURRENT BOOT · age 750 ms · SAVED']},
];
const report = {status:'running',variants:{},rendering:[],pageErrors:[]};
async function readEvidence(page) {
  return page.evaluate(ids => ids.map(id => {
    const el=document.getElementById(id),row=el.parentElement.getBoundingClientRect();
    return {id,text:el.textContent,height:row.height,top:row.top};
  }),ids);
}
async function checkScenario(page,variant,item) {
  const result=await page.evaluate(async ({ids,stats}) => {
    const read=()=>ids.map(id=>{const el=$(id),r=el.parentElement.getBoundingClientRect();return {id,text:el.textContent,height:r.height,top:r.top}});
    await fetchSys(stats);const immediate=read();renderHeartbeatSnapshot();const timer=read();
    await fetchSys(stats);const repeated=read();renderHeartbeatSnapshot();const repeatedTimer=read();
    return {immediate,timer,repeated,repeatedTimer,state:$('sysTwaiState').textContent,mcpCount:$('sysMcpBusOff').textContent,busOffStopped:$('sysTwaiBusOffStopped').textContent,restart:$('sysTwaiRestart').textContent,heartbeatCounts:$('sysHeartbeatCounts').textContent};
  },{ids,stats:item.stats});
  for(const stage of ['immediate','timer','repeated','repeatedTimer']) {
    assert.deepEqual(result[stage].map(row=>row.text),item.want,variant+'/'+item.name+'/'+stage+': telemetry must immediately preserve boot/persistence evidence');
    assert.deepEqual(result[stage],result.immediate,variant+'/'+item.name+'/'+stage+': unchanged telemetry must retain row geometry');
  }
  assert.equal(result.state,item.stats.twaiStateName,'Real controller state must refresh');
  assert.equal(result.mcpCount,String(item.stats.mcpBusOffCount),'CAN A count must refresh');
  assert.equal(result.busOffStopped,item.stats.twaiBusOffCount+' / '+item.stats.twaiStoppedCount,'CAN B counts must refresh');
  assert.equal(result.restart,item.stats.twaiRestartOkCount+' / '+item.stats.twaiRestartFailCount,'Restart count must refresh');
  assert.equal(result.heartbeatCounts,'0 / '+item.stats.canTaskHeartbeatTimeoutCountB+' / 0','Heartbeat rows must still refresh');
  return {name:item.name,...result};
}
async function geometry(page) {
  return page.evaluate(() => {
    const issues=[];if(document.documentElement.scrollWidth>innerWidth+1)issues.push('document overflow');
    const panel=$('panelDiag');if(panel.scrollWidth>panel.clientWidth+1)issues.push('panel overflow');
    for(const row of panel.querySelectorAll('.r')) {
      const r=row.getBoundingClientRect();if(r.width===0||r.height===0)continue;
      const label=row.firstElementChild,value=row.lastElementChild,l=label.getBoundingClientRect(),v=value.getBoundingClientRect();
      if(r.left< -1||r.right>innerWidth+1)issues.push(value.id+': row outside viewport');
      if(v.width<=0||v.height<=0||v.bottom>r.bottom+1)issues.push(value.id+': clipped value');
      if(Math.min(l.right,v.right)>Math.max(l.left,v.left)+1&&Math.min(l.bottom,v.bottom)>Math.max(l.top,v.top)+1)issues.push(value.id+': label/value overlap');
    }
    return issues;
  });
}
async function validate(variant) {
  // Separate launches avoid macOS single-process Chromium context reuse crashes.
  const browser=await launchBrowser();let stats=baseStats,systemRequests=0;
  try {
    const page=await browser.newPage({viewport:{width:390,height:1100}});
    page.on('pageerror',error=>report.pageErrors.push({variant,error:error.message}));
    await page.route('**/*',route=>{
      const url=new URL(route.request().url());if(url.origin!=='http://diagnostics.test')return route.abort();
      const font=fonts[url.pathname];if(font)return route.fulfill({contentType:'font/woff2',body:font});
      if(!url.pathname.startsWith('/api/'))return route.fulfill({contentType:'text/html; charset=utf-8',body:variant==='source'?source:embedded});
      let data={};if(url.pathname==='/api/profile/status')data={setupMode:false,profile:1,topology:1,turn:0};
      else if(url.pathname==='/api/features/status')data={lab:true,s3xy:true};
      else if(url.pathname==='/api/system/stats'){data=stats;systemRequests++;}
      return route.fulfill({contentType:'application/json',body:JSON.stringify(data)});
    });
    await page.goto('http://diagnostics.test/',{waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.evaluate(()=>{universalBootReady=false;openPanel('panelDiag')});
    // Let the production panel transition's one deferred poll finish before isolated telemetry cases.
    await page.waitForTimeout(450);
    await page.evaluate(async()=>{await Promise.all([document.fonts.load('400 14px "Geist"'),document.fonts.load('400 12px "Geist Mono"')]);await document.fonts.ready});
    assert.deepEqual(await page.evaluate(()=>[...document.fonts].map(face=>({family:face.family,status:face.status})).sort((a,b)=>a.family.localeCompare(b.family))),[{family:'Geist',status:'loaded'},{family:'Geist Mono',status:'loaded'}],'Real firmware fonts must load');
    // Prime separate heartbeat rows so their first legitimate render cannot move later rows.
    await page.evaluate(async stats=>{await fetchSys(stats);renderHeartbeatSnapshot()},baseStats);
    const scenarios=[];for(const item of cases)scenarios.push(await checkScenario(page,variant,item));
    // A real recovery after a new snapshot must leave the evidence visible while live state changes.
    const recovered={...cases.at(-1),name:'recovered-running',stats:{...cases.at(-1).stats,twaiStateName:'RUNNING',twaiRestartOkCount:5}};
    scenarios.push(await checkScenario(page,variant,recovered));
    await page.evaluate(async stats=>{await fetchSys(stats);renderHeartbeatSnapshot()},baseStats);
    const beforeRequests=systemRequests;
    const timeline=await page.evaluate(async ({ids})=>{
      const start=performance.now(),samples=[];
      const read=()=>samples.push({ms:Math.round(performance.now()-start),rows:ids.map(id=>{const el=$(id);return {id,text:el.textContent,height:el.parentElement.getBoundingClientRect().height}})});
      const observer=new MutationObserver(read);ids.forEach(id=>observer.observe($(id),{childList:true,subtree:true,characterData:true}));
      read();const sampleTimer=setInterval(read,50);universalBootReady=true;forcePoll();
      await new Promise(resolve=>setTimeout(resolve,4300));universalBootReady=false;clearInterval(sampleTimer);observer.disconnect();read();return samples;
    },{ids});
    assert(systemRequests-beforeRequests>=3,'Continuous check must span at least three actual production system polls');
    assert(timeline.at(-1).ms>=4200,'Continuous check must include over four seconds of real timers');
    assert(timeline.length>=80,'Continuous check must sample between timer and poll ticks');
    for(const sample of timeline) {
      assert.deepEqual(sample.rows.map(row=>row.text),mixedWant,variant+'/poll at '+sample.ms+' ms: saved evidence must never revert to dash/legacy age');
      assert.deepEqual(sample.rows.map(row=>row.height),timeline[0].rows.map(row=>row.height),variant+'/poll: row heights must remain stable');
    }
    report.variants[variant]={scenarios,timeline,systemPolls:systemRequests-beforeRequests};
    for(const width of [320,390,430])for(const theme of ['light','dark']) {
      await page.setViewportSize({width,height:1100});await page.evaluate(theme=>window.t2Theme.setMode(theme),theme);await page.waitForTimeout(150);
      const issues=await geometry(page);assert.deepEqual(issues,[],variant+'/'+width+'/'+theme);
      const rows=await readEvidence(page);
      report.rendering.push({variant,width,theme,issues,rows});
      await page.evaluate(()=>{$('panelDiag').scrollTop=0});
      await page.screenshot({path:path.join(output,variant+'-'+width+'-'+theme+'-top.png')});
      await page.evaluate(()=>{$('sysTwaiState').scrollIntoView({block:'start'})});
      await page.screenshot({path:path.join(output,variant+'-'+width+'-'+theme+'-can-b.png')});
    }
    assert.deepEqual(report.pageErrors,[],'No browser runtime errors');
  } finally {await browser.close()}
}
(async()=>{
  fs.mkdirSync(output,{recursive:true});
  try {
    const selected=process.env.DASHBOARD_DIAGNOSTICS_VARIANT;
    assert(!selected||['source','embedded'].includes(selected),'Variant must be source or embedded');
    for(const variant of selected?[selected]:['source','embedded'])await validate(variant);
    report.status='passed';
    console.log('PASS Diagnostics persistence: '+Object.keys(report.variants).length+' HTML variants, 8 scenarios each, >=4.2 s actual polling, '+report.rendering.length+' responsive renders');
  } catch(error){report.status='failed';report.error=error.stack;throw error}
  finally{fs.writeFileSync(path.join(output,'validation.json'),JSON.stringify(report,null,2))}
})().catch(error=>{console.error(error);process.exit(1)});
