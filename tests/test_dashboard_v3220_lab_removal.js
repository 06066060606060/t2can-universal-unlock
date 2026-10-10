const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');const root=path.resolve(__dirname,'..');
async function check(html,label){const browser=await launchBrowser();try{
 const page=await browser.newPage({viewport:{width:390,height:844}}),errors=[],requests=[];page.on('pageerror',e=>errors.push(e.message));
 await page.route('http://lab-removal.test/**',async route=>{const u=new URL(route.request().url());requests.push(u.pathname);if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});let body={};
 if(u.pathname==='/api/features/status')body={lab:true};
 if(u.pathname==='/api/profile/status')body={profile:1,topology:1,turn:1,setupMode:false};
 return route.fulfill({contentType:'application/json',body:JSON.stringify(body)});});
 await page.goto('http://lab-removal.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);
 await page.evaluate(()=>{universalBootReady=false;labEnabled=true;document.body.classList.remove('setup-active');showPage('lab')});
 const lab=page.locator('.page[data-page="lab"]');
 for(const id of ['panelLabLaneGraph','panelLabParkedInjection','panelLabCanARx','panelLabUlcMonitor'])assert.equal(await page.locator('#'+id+', [data-panel="'+id+'"] ').count(),0,`removed panel/menu ${id}`);
 assert.equal(await page.locator('#labUlcBlind').count(),1,'blind spot injection remains available');
 assert.equal(await page.locator('.labToolsCard [data-panel="panelVisionControl"]').count(),0,'Visual Speed Control left LAB');
 assert.equal(await page.locator('main[data-page="settings"] [data-panel="panelVisionControl"]').count(),1,'Visual Speed Control is a production setting');
 for(const width of [320,390,430])for(const theme of ['light','dark']){
  await page.setViewportSize({width,height:844});await page.evaluate(t=>{window.t2Theme.setMode(t);showPage('lab')},theme);
  await lab.evaluate(async e=>{await Promise.allSettled(e.getAnimations().filter(a=>a.animationName?.startsWith("v38Panel")).map(a=>a.finished))});
  assert.equal(await lab.isVisible(),true);assert.equal(await lab.evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} page overflow`);
  const rows=await lab.locator('.labToolsCard .listrow').evaluateAll(es=>es.map(e=>{const r=e.getBoundingClientRect();return {left:r.left,right:r.right,top:r.top,bottom:r.bottom};}));
  for(let i=0;i<rows.length;i++){assert(rows[i].left>=0&&rows[i].right<=width,`${label} row clipping`);if(i)assert(rows[i].top>=rows[i-1].bottom-.5,`${label} overlapping rows`);}
  if(process.env.DASHBOARD_SCREENSHOT_DIR){fs.mkdirSync(process.env.DASHBOARD_SCREENSHOT_DIR,{recursive:true});await page.screenshot({path:path.join(process.env.DASHBOARD_SCREENSHOT_DIR,`${label}-lab-removal-${width}-${theme}.png`),fullPage:false,animations:'disabled'});}
 }
 assert(!requests.some(p=>/\/api\/lab\/(lane-graph|parked-injection|can-a-rx|ulc-monitor)\//.test(p)),'obsolete API requested');assert.deepEqual(errors,[]);
 }finally{await browser.close();}}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');})().then(()=>console.log('PASS source/embedded removed LAB menus, surviving controls, no obsolete requests and 12 responsive theme renders'),e=>{console.error(e);process.exitCode=1;});
