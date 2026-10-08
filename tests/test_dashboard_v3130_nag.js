const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');
async function check(html,label){
 const browser=await launchBrowser();
 try{
 const page=await browser.newPage({viewport:{width:320,height:844}});const errors=[];page.on('pageerror',e=>errors.push(e.message));
 let saved={enabled:false,ignoreApState:false,method:0,mode:7,torque:[],targetId:1160,apStateId:921,steeringId:880},reject=false;
 await page.route('http://nag.test/**',async route=>{
 const u=new URL(route.request().url());if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
 let body={},status=200;
 if(u.pathname==='/api/nag/config')body=saved;
 if(u.pathname==='/api/nag/update'){
  if(reject){status=500;body={error:'save_failed'};}else{for(const [k,v] of u.searchParams)saved[k]=['method','mode'].includes(k)?Number(v):v==='1';body=saved;}
 }
 if(u.pathname==='/api/nag/mode'){saved.mode=Number(u.searchParams.get('m'));body=saved;}
 await route.fulfill({status,contentType:'application/json',body:JSON.stringify(body)});
 });
 await page.goto('http://nag.test/',{waitUntil:'domcontentloaded'});
 const setup=async()=>{nagCfg=await (await fetch('/api/nag/config')).json();window.__t2NagSupported=true;window.__t2NagTorqueSupported=true;window.__t2NagTsl9Supported=true;document.body.classList.remove('setup-active');openPanel('panelNag');renderNagCfg();};
 await page.evaluate(setup);
 assert.equal(await page.locator('#nagIgnoreApStateToggle').isChecked(),false);
 await page.evaluate(()=>toggleNag(true,'ignoreApState'));assert.equal(saved.ignoreApState,true);
 await page.reload({waitUntil:'domcontentloaded'});await page.evaluate(setup);
 assert.equal(await page.locator('#nagIgnoreApStateToggle').isChecked(),true,'persisted value restores on reload');
 reject=true;await page.evaluate(()=>{document.querySelector('#nagIgnoreApStateToggle').checked=false;return toggleNag(false,'ignoreApState');});
 assert.equal(await page.locator('#nagIgnoreApStateToggle').isChecked(),true,'failed save rolls UI back');reject=false;
 await page.evaluate(()=>updateNagValue('method',1));assert.equal(await page.locator('#nagIgnoreApStateWrap').isVisible(),false);assert.equal(saved.ignoreApState,true);
 await page.evaluate(()=>updateNagValue('method',0));assert.equal(await page.locator('#nagIgnoreApStateToggle').isChecked(),true);
 for(const mode of [0,1,3,7]){await page.evaluate(m=>setMode(m),mode);assert.equal(saved.ignoreApState,true);}
 assert.equal(await page.locator('[id^="modeHRev"]').count(),0);
 const statusCases=await page.evaluate(async()=>{
  const stats={apActive:false,dasStateValid:false,dasState:0,dasAgeMs:999999,torque:0,humanPhase:'WAIT'};
  const result=[];
  for(const c of [{method:0,enabled:true,ignoreApState:true},{method:1,enabled:true,ignoreApState:true},{method:0,enabled:false,ignoreApState:true},{method:0,enabled:true,ignoreApState:false},{method:0,enabled:true,ignoreApState:true,stoppedGate:true},{method:0,enabled:true,ignoreApState:true,stoppedGate:true,stopCarrierActive:true}]){
   Object.assign(nagCfg,c);const s={...stats,stoppedGate:!!c.stoppedGate,stopCarrierActive:!!c.stopCarrierActive};
   await fetchNag(s);const detail={label:$('nagState').textContent,panel:$('nagPanelState').textContent,ap:$('nagDiagAp').textContent};
   renderHomeFast({nag:s});result.push({...detail,home:$('nagState').textContent});
  }
  return result;
 });
 assert.deepEqual(statusCases.map(s=>s.label),['BYPASS','STANDBY','OFF','STANDBY','PAUSED','CARRIER']);
 assert.deepEqual(statusCases.map(s=>s.home),['BYPASS','STANDBY','OFF','STANDBY','PAUSED','CARRIER']);
 assert.match(statusCases[0].panel,/AP bypass/);
 assert.equal(statusCases[1].panel,'TSL9 standby');
 assert.equal(statusCases[2].panel,'Disabled');
 assert.equal(statusCases[3].panel,'Standby');
 assert.match(statusCases[4].panel,/Paused/);
 assert(statusCases.every(s=>s.ap.startsWith('INVALID')), 'bypass must preserve actual AP diagnostic validity');

 await page.evaluate(()=>{renderNagHumanLab({peakMinNm:1.8,peakMaxNm:2.6});});
 assert.equal(await page.locator('#humanV1PeakMinNm').inputValue(),'1.80');
 assert.equal(await page.locator('#humanV4CarrierMinNm').inputValue(),'0.10');
 await page.evaluate(async()=>{Object.assign(nagCfg,{enabled:true,method:0,mode:7,ignoreApState:true});renderNagCfg();await fetchNag({apActive:false,dasStateValid:false,torque:0,humanPhase:'WAIT'});openPanel('panelNag');});
 for(const width of [320,390,430])for(const theme of ['light','dark']){
  await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
  assert.equal(await page.locator('#panelNag').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width} ${theme} overflow`);
  const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.waitForTimeout(250);await page.screenshot({path:path.join(dir,`${label}-nag-${width}-${theme}.png`),fullPage:true,animations:'disabled'});}
 }
 assert.deepEqual(errors,[]);
 }finally{await browser.close();}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.NAG_DASHBOARD_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS v3.26.3 NAG source/embedded persistence, rollback, methods and responsive themes'),e=>{console.error(e);process.exitCode=1;});
