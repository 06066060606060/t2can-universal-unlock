// Removing independent map saves, response epochs, LAB gating or rollback must fail.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');const root=path.resolve(__dirname,'..');
async function check(html,label){const browser=await launchBrowser();try{
const page=await browser.newPage({viewport:{width:320,height:844}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
// Layout bounds include animated transforms. Measure only after navigation settles.
const waitForNavigation=locator=>locator.evaluate(async e=>{await Promise.all(e.getAnimations().filter(a=>a.animationName?.startsWith('v38Panel')).map(a=>a.finished));});
let country={countrySupported:true,countryMode:0,mapMode:0,countryGateOpen:false,gateAllowed:false};
let reject=false,failRead=false,heldPath='',holdNext=false,release,started;const writes=[];
await page.route('http://v315.test/**',async route=>{const u=new URL(route.request().url());if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});let body={},status=200;
if(u.pathname==='/api/features/status')body={lab:true};
if(u.pathname==='/api/profile/status')body={setupMode:false,profile:3,topology:2,turn:1};
if(u.pathname==='/api/country/stats')body={...country};
if(u.pathname.endsWith('/update')&&u.pathname==='/api/country/update'){
writes.push([u.pathname,u.searchParams.toString()]);if(reject){status=503;body={error:'save-failed'};}else if(u.pathname==='/api/country/update'){for(const[k,v]of u.searchParams)country[k]=Number(v);body={...country};}}

if(failRead&&u.pathname.endsWith('/stats')){status=503;body={error:'unavailable'};}
if(holdNext&&u.pathname===heldPath){holdNext=false;await new Promise(resolve=>{release=resolve;started();});}
await route.fulfill({status,contentType:'application/json',body:JSON.stringify(body)});});
const setup=async()=>{universalBootReady=false;labEnabled=true;document.body.classList.remove('setup-active');openPanel('panelCountry');await fetchCountryOverrideLab();};
await page.goto('http://v315.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.evaluate(setup);
assert.equal(await page.locator('#countryMapMode').count(),1,'country and map must have independently saved controls');
assert.deepEqual(await page.locator('#countryMapMode option').evaluateAll(es=>es.map(e=>[e.value,e.textContent])),[['0','STOCK'],['1','US · 0'],['2','KOREA · 7']]);
const saveCountry=(key,value)=>page.evaluate(async({key,value})=>{$(key==='countryMode'?'countryOverrideMode':'countryMapMode').value=String(value);await $(key==='countryMode'?'countryOverrideMode':'countryMapMode').onchange();},{key,value});
await saveCountry('mapMode',2);await saveCountry('countryMode',3);assert.equal(await page.locator('#countryMapMode').inputValue(),'2','country change preserves saved map');assert.equal(await page.locator('#countryOverrideMode').inputValue(),'3');
await saveCountry('mapMode',0);assert.equal(await page.locator('#countryOverrideMode').inputValue(),'3','STOCK map preserves NZ country');
assert.equal(await page.locator('#countryOverrideRowState').textContent(),'NZ / STOCK');
assert.deepEqual(writes.slice(0,3),[['/api/country/update','mapMode=2'],['/api/country/update','countryMode=3'],['/api/country/update','mapMode=0']]);
await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.evaluate(setup);assert.equal(await page.locator('#countryOverrideMode').inputValue(),'3');assert.equal(await page.locator('#countryMapMode').inputValue(),'0');
reject=true;await saveCountry('mapMode',1);assert.equal(await page.locator('#countryMapMode').inputValue(),'0');failRead=true;await saveCountry('countryMode',1);assert.equal(await page.locator('#countryOverrideMode').inputValue(),'3');failRead=false;reject=false;
heldPath='/api/country/stats';holdNext=true;let ready=new Promise(resolve=>{started=resolve;});await page.evaluate(()=>{window.__lateCountry=fetchCountryOverrideLab();});await ready;await saveCountry('mapMode',2);release();await page.evaluate(()=>window.__lateCountry);assert.equal(await page.locator('#countryMapMode').inputValue(),'2','late poll must not undo map save');
await page.evaluate(()=>{labEnabled=false;return fetchCountryOverrideLab();});assert.equal(await page.locator('#countryMapMode').isDisabled(),false,'country/map remain available with LAB OFF');
for(const id of ['panelLabLaneGraph','panelLabParkedInjection','panelLabCanARx','panelLabUlcMonitor']) assert.equal(await page.locator('#'+id).count(),0,`removed LAB panel ${id}`);
await saveCountry('countryMode',0);await saveCountry('mapMode',1);assert.equal(await page.locator('#countryOverrideRowState').textContent(),'STOCK / US 0','map override must be visible when country remains stock');
for(const width of [320,390,430])for(const theme of ['light','dark'])for(const panel of ['panelCountry']){
await page.setViewportSize({width,height:844});await page.evaluate(({theme,panel})=>{window.t2Theme.setMode(theme);openPanel(panel);},{theme,panel});
assert.equal(await page.locator('#'+panel).isVisible(),true,`${label} ${panel} must be visible`);await waitForNavigation(page.locator('#'+panel));
assert.equal(await page.locator('#'+panel).evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${panel} ${width}/${theme} overflow`);
for(const id of ['countryOverrideMode','countryMapMode'])assert.equal(await page.locator('#'+id).evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true);
const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-${panel}-${width}-${theme}.png`),fullPage:true,animations:'disabled'});}}
// Settings starts hidden: exercise the known fractional frame on its first entry.
for(const width of [390,320,430])for(const theme of ['light','dark']){
 await page.setViewportSize({width,height:844});await page.evaluate(({width,theme})=>{window.t2Theme.setMode(theme);showPage('settings');
 // Reproduce the fractional bounds frame that exposed a missing navigation wait.
 if(width===390&&theme==='light'){const a=document.querySelector('.page[data-page="settings"]').getAnimations().find(a=>a.animationName==='v38PanelForward');a.pause();a.currentTime=5;a.play();}
 },{width,theme});
 const settings=page.locator('.page[data-page="settings"]');assert.equal(await settings.isVisible(),true);await waitForNavigation(settings);
 assert.equal(await settings.evaluate(e=>getComputedStyle(e).transform),'none',`${label} settings navigation must settle before measuring`);
 assert.equal(await page.locator('#countryOverrideSettingsRow').evaluate(e=>{const r=e.getBoundingClientRect(),v=e.querySelector('.rowRight').getBoundingClientRect(),title=e.querySelector('.listkey').getBoundingClientRect();const inline=title.right<=v.left&&title.top<v.bottom&&v.top<title.bottom,stacked=title.bottom<=v.top;return r.right<=innerWidth&&v.right<=r.right&&v.left>=r.left&&title.left>=r.left&&title.right<=r.right&&(inline||stacked);}),true,`${label} settings summary ${width}/${theme} fits`);
 if(process.env.DASHBOARD_SCREENSHOT_DIR)await page.screenshot({path:path.join(process.env.DASHBOARD_SCREENSHOT_DIR,`${label}-settings-${width}-${theme}.png`),fullPage:true,animations:'disabled'});
}
assert.deepEqual(errors,[]);
}finally{await browser.close();}}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.V315_DASHBOARD_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS independent Country/Map source/embedded persistence, rollback, late polls, LAB removal and responsive themes'),e=>{console.error(e);process.exitCode=1;});
