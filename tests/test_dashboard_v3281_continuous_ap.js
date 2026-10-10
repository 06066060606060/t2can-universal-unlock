const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');const root=path.resolve(__dirname,'..');
// Catches confusing an idle episode with live RX readiness, retaining green after
// failed GET, losing cancellation evidence, and unreadable expanded mobile rows.
async function check(html,label){
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:320,height:844}});let failGet=false;
  const response={enabled:true,method:2,effectiveEnabled:true,supported:true,stalkSupported:false,scrollSingleSupported:true,scrollDoubleSupported:true,supportReason:'Match your vehicle setting.',state:'Idle',blockedReason:'Waiting for AP active',lastCancelReason:'Brake applied',signalReadiness:'Available',attempts:0,txOk:0,txFail:0};
  const signals=[['Ap','0x399','0','Chassis CAN B'],['Turn','0x3F5','0','Chassis CAN B'],['Brake','0x39D','1','Chassis CAN B'],['Gear','0x118','4','Chassis CAN B'],['Torque','0x370','0.00 Nm','Party CAN A'],['Control','0x3C2','Idle','Chassis CAN B']];
  for(const [key,id,value,bus]of signals)Object.assign(response,{['rx'+key+'Status']:'Fresh',['rx'+key+'Value']:value,['rx'+key+'Id']:id,['rx'+key+'Bus']:bus,['rx'+key+'AgeMs']:42});
  await page.route('http://v3281.test/**',async route=>{
   const url=new URL(route.request().url());if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});let body={};
   if(url.pathname==='/api/profile/status')body={setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true,advancedEapSupported:true,euUnlockSupported:true};
   else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false,apDriveProfileSupported:true};
   else if(url.pathname==='/api/continuous-ap/config'){if(failGet)return route.fulfill({status:503,contentType:'application/json',body:'{"error":"unavailable"}'});body=response;}
   await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
  });
  await page.goto('http://v3281.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);
  await page.getByRole('button',{name:'Settings'}).click();await page.locator('#continuousApSettingsRow').click();await page.waitForSelector('#panelContinuousAp.show');await page.evaluate(()=>fetchContinuousAp());
  const readiness=page.locator('#continuousApSignalReadiness'),details=page.locator('#continuousApSignalDetails');
  assert.equal(await readiness.count(),1,'Live signal readiness must be separate from episode Idle state');
  assert.equal(await readiness.textContent(),'Available');assert.equal(await readiness.evaluate(e=>e.classList.contains('good')),true,'fresh inactive AP / applied brake RX still means Available');
  assert.equal(await page.locator('#continuousApLastCancel').textContent(),'Brake applied','last cancellation evidence must remain visible');
  assert.equal(await details.evaluate(e=>e.tagName),'DETAILS');assert.equal(await details.evaluate(e=>e.open),false,'RX details start collapsed');
  await details.locator('summary').click();await page.evaluate(()=>fetchContinuousAp());assert.equal(await details.evaluate(e=>e.open),true,'polling must preserve expanded details');
  for(const [key,id,value,bus]of signals){const row=await page.locator('#continuousApRx'+key).textContent();for(const expected of['Fresh',id,value,bus,'42'])assert(row.includes(expected),`${label} ${key} missing ${expected}: ${row}`);}
  for(const status of['Missing','Stale','Invalid']){
   response.signalReadiness=status;response.rxApStatus=status;response.rxApAgeMs=status==='Missing'?4294967295:999;
   await page.evaluate(()=>fetchContinuousAp());assert.equal(await readiness.textContent(),status);assert.equal(await readiness.evaluate(e=>e.classList.contains('good')),false,`${status} must not show green`);
   assert((await page.locator('#continuousApRxAp').textContent()).includes(status));
   if(status==='Missing')assert(!(await page.locator('#continuousApRxAp').textContent()).includes('4294967295'),'unknown age sentinel must not look like real age');
  }
  response.signalReadiness='Available';response.rxApStatus='Fresh';response.rxApAgeMs=42;await page.evaluate(()=>fetchContinuousAp());
  failGet=true;await page.evaluate(()=>fetchContinuousAp());assert.equal(await readiness.evaluate(e=>e.classList.contains('good')),false,'failed GET must clear stale green');assert.notEqual(await readiness.textContent(),'Available','failed GET must not retain Available claim');
  failGet=false;await page.evaluate(()=>fetchContinuousAp());assert.equal(await readiness.textContent(),'Available');assert.equal(await readiness.evaluate(e=>e.classList.contains('good')),true);
  response.signalReadiness='Missing';response.rxControlStatus='Missing';response.rxControlAgeMs=4294967295;await page.evaluate(()=>fetchContinuousAp());assert.equal(await page.locator('#continuousApState').textContent(),'Idle');assert.equal(await readiness.evaluate(e=>e.classList.contains('good')),false,'Idle alone must not imply healthy RX');
  for(const width of[320,390,430])for(const theme of['light','dark']){
   await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
   assert.equal(await page.locator('#panelContinuousAp').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} panel overflow`);
   for(const [key]of signals)assert.equal(await page.locator('#continuousApRx'+key).evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&e.scrollWidth<=e.clientWidth+1}),true,`${label} ${width}/${theme} clipped ${key}`);
   const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-continuous-ap-rx-${width}-${theme}.png`),animations:'disabled'});await details.scrollIntoViewIfNeeded();await page.screenshot({path:path.join(dir,`${label}-continuous-ap-rx-details-${width}-${theme}.png`),animations:'disabled'});await page.evaluate(()=>window.scrollTo(0,0));}
  }
  await details.locator('summary').click();await page.evaluate(()=>fetchContinuousAp());assert.equal(await details.evaluate(e=>e.open),false,'polling must preserve collapsed details');
 }finally{await browser.close();}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.V328_DASHBOARD_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS Continuous AP live RX readiness, cancellation, GET failure and mobile details'),e=>{console.error(e);process.exitCode=1;});
