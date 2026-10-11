const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {launchBrowser,dashboardArtifacts,dashboardFonts}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');
const fonts=dashboardFonts(root);

// test_country_override_dashboard.js: original scenarios and isolated page fixture.
async function checkCountryModes(html, label) {
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
      else { stats = { ...stats, countryMode: Number(u.searchParams.get('countryMode')) }; body = {mapMode:0,...stats}; }
    } else if (u.pathname === '/api/country/stats') body = {mapMode:0,...stats};
    await route.fulfill({ status, contentType: 'application/json', body: JSON.stringify(body) });
  });
  try {
    await page.goto('http://country.test/', { waitUntil: 'domcontentloaded' });
    await page.evaluate(()=>fetchCountryOverrideLab());
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


// test_dashboard_v3150_country_lane_graph.js: original scenarios and isolated page fixture.
async function checkCountryMapAndLayout(html,label){const browser=await launchBrowser();try{
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
reject=true;await saveCountry('mapMode',1);assert.equal(await page.locator('#countryMapMode').inputValue(),'0');failRead=true;await saveCountry('countryMode',1);assert.equal(await page.locator('#countryOverrideMode').inputValue(),'3');failRead=false;reject=false;await page.evaluate(()=>fetchCountryOverrideLab());
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

(async()=>{
  const sourceOnly=Boolean(process.env.DASHBOARD_SOURCE_ONLY||process.env.COUNTRY_DASHBOARD_SOURCE_ONLY||process.env.V315_DASHBOARD_SOURCE_ONLY);
  for(const {html,label} of dashboardArtifacts(root,sourceOnly)){
    await checkCountryModes(html,label);
    await checkCountryMapAndLayout(html,label);
  }
})().then(()=>console.log('PASS country source/embedded suite (2 preserved scenario groups)'),e=>{console.error(e);process.exitCode=1;});
