// Reintroducing a retired control, panel or poll fails the real dashboard check.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');const root=path.resolve(__dirname,'..');
async function check(html,label){const browser=await launchBrowser();try{
 const page=await browser.newPage({viewport:{width:320,height:844}}),errors=[],requests=[];page.on('pageerror',e=>errors.push(e.message));
 await page.route('http://v320.test/**',async route=>{const u=new URL(route.request().url());if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});requests.push(u.pathname);let body={};if(u.pathname==='/api/profile/status')body={setupMode:false,profile:3,topology:2,turn:1};if(u.pathname==='/api/features/status')body={lab:true};if(u.pathname==='/api/vision-control/stats'||u.pathname==='/api/vision-control/update')body={requestDisabled:false,bus:0,supported:true,bodySupported:true,busSupported:true,busSelectorVisible:true,labEnabled:true,apActive:true,apFresh:true,stockValid:true,stockBit:1,rxAgeMs:10,rxCount:2,gateOpen:false,txOk:0,txFail:0,state:'OFF'};await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});});
 await page.goto('http://v320.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.evaluate(()=>{universalBootReady=false;labEnabled=true;document.body.classList.remove('setup-active');showPage('settings');});
 assert.equal(await page.locator('[data-panel="panelLabVisionSpeed"], [data-panel="panelLabAdaptiveSpeed"]').count(),0,'retired 0x3F8 controls must no longer be available in LAB');
 assert.equal(await page.locator('#panelLabVisionSpeed, #panelLabAdaptiveSpeed, #visionSpeedDisableToggle, #adaptiveSpeedDisableToggle').count(),0,'retired panels and controls must be removed');
 assert.equal(await page.locator('[data-panel="panelVisionControl"]').isVisible(),true,'Visual Speed Control remains visible as a production setting');
 for(const width of [320,390,430])for(const theme of ['light','dark']){
  await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);await page.waitForTimeout(120);
  assert.equal(await page.locator('[data-panel="panelVisionControl"]').evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true,`${label} Settings menu ${width}/${theme} fits viewport`);
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true,'retired menu removal must not introduce horizontal overflow');
  const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-retired-speed-lab-${width}-${theme}.png`),fullPage:true,animations:'disabled'});}
 }
 await page.locator('[data-panel="panelVisionControl"]').click();await page.waitForFunction(()=>document.querySelector('#panelVisionControl.show'));await page.evaluate(()=>fetchVisionControlLab());assert.equal(await page.locator('#visionControlDisableToggle').isDisabled(),false);
 await page.evaluate(()=>{pollTick(true);openPanel('panelLabVisionSpeed');pollTick(true);openPanel('panelLabAdaptiveSpeed');pollTick(true);openPanel('panelVisionControl');pollTick(true);});await page.waitForTimeout(60);
 assert.equal(requests.filter(p=>p.startsWith('/api/lab/vision-speed/')||p.startsWith('/api/lab/adaptive-speed/')).length,0,'retired APIs are never polled');assert(requests.includes('/api/vision-control/stats'),'kept production feature continues polling');
 assert.deepEqual(errors,[]);
}finally{await browser.close();}}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.V320_DASHBOARD_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS retired 0x3F8 LAB controls/panels/polls removed; Visual Speed Control retained in Settings source/embedded'),e=>{console.error(e);process.exitCode=1;});
