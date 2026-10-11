const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {launchBrowser,dashboardArtifacts}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');

// test_dashboard_v328_continuous_ap.js: original scenarios and isolated page fixture.
async function checkContinuousApSettings(html,label){
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:320,height:844}}),requests=[];
  let enabled=false,method=0,profile=4,topology=2,failGet=false,failPost=false,lostReply=false;
  await page.route('http://v328.test/**',async route=>{
   const request=route.request(),url=new URL(request.url());requests.push(`${request.method()} ${url.pathname}${url.search}`);
   if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
   let body={};
   if(url.pathname==='/api/profile/status')body={setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true,advancedEapSupported:true,euUnlockSupported:true};
   else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false,apDriveProfileSupported:true};
   else if(url.pathname==='/api/continuous-ap/config'){
    if(request.method()==='GET'&&failGet||request.method()==='POST'&&failPost)return route.fulfill({status:503,contentType:'application/json',body:'{"error":"unavailable"}'});
    if(request.method()==='POST'){enabled=url.searchParams.get('enabled')==='1';method=Number(url.searchParams.get('method'));if(lostReply){lostReply=false;return route.abort('failed');}}
    const supported=profile!==1&&topology===2;body={enabled,method,effectiveEnabled:enabled&&supported,supported,stalkSupported:supported&&profile===3,scrollSingleSupported:supported&&profile===4,scrollDoubleSupported:supported&&profile===4,supportReason:supported?'Match your vehicle setting.':'Unavailable for this vehicle connection.',state:'Idle',blockedReason:'Waiting for AP active',txOk:0,txFail:0};
   }
   await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
  });
  await page.goto('http://v328.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);
  await page.getByRole('button',{name:'Settings'}).click();
  assert.equal(await page.locator('#continuousApSettingsRow').count(),1,'Continuous AP Settings row missing');
  await page.locator('#continuousApSettingsRow').click();await page.waitForSelector('#panelContinuousAp.show');
  const toggle=page.locator('#continuousApToggle'),select=page.locator('#continuousApMethod');
  assert.equal(await toggle.isChecked(),false);assert.equal(await select.inputValue(),'0');assert.equal(await toggle.isDisabled(),true,'ON must require a method');
  assert.equal(await page.locator('#continuousApMethod option[value="1"]').isDisabled(),true,'unsupported stalk must be disabled');
  assert.equal(await select.evaluate(e=>getComputedStyle(e.closest('.labControlRow')).opacity),'1','supported selector must appear enabled despite disabled unsupported option');
  await select.click();assert.equal(await page.locator('#selectSheetList').getByRole('option',{name:'Stalk Down',exact:true}).count(),0);await page.locator('#selectSheetList').getByRole('option',{name:'Scroll Double Click',exact:true}).click();await page.waitForFunction(()=>continuousApControl.saved?.method===3&&!continuousApControl.saving);assert.equal(await toggle.isDisabled(),false);
  await page.locator('#panelContinuousAp label.toggle').click();await page.waitForFunction(()=>continuousApControl.saved?.enabled===true&&!continuousApControl.saving);
  assert(requests.includes('POST /api/continuous-ap/config?enabled=1&method=3'));
  await select.selectOption('2');await page.waitForFunction(()=>continuousApControl.saved?.method===2&&!continuousApControl.saving);assert.equal(enabled,true,'enabled method change should remain ON');
  failPost=true;await select.selectOption('3');await page.waitForFunction(()=>!continuousApControl.saving);assert.equal(await select.inputValue(),'2','failed save must reconcile saved method');failPost=false;
  lostReply=true;let posts=requests.filter(x=>x.startsWith('POST /api/continuous-ap')).length;await select.selectOption('3');await page.waitForFunction(()=>continuousApControl.saved?.method===3&&!continuousApControl.saving);assert.equal(requests.filter(x=>x.startsWith('POST /api/continuous-ap')).length,posts+1,'lost response must not repeat POST');
  failGet=true;failPost=true;await select.selectOption('2');await page.waitForFunction(()=>!continuousApControl.saving&&continuousApControl.uncertain);assert.equal(await select.isDisabled(),true,'unresolved save must lock controls');assert.equal(await toggle.isDisabled(),true);
  failGet=false;failPost=false;await page.evaluate(()=>fetchContinuousAp());assert.equal(await select.inputValue(),'3');assert.equal(await select.isDisabled(),false);
  await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.getByRole('button',{name:'Settings'}).click();await page.locator('#continuousApSettingsRow').click();assert.equal(await toggle.isChecked(),true);assert.equal(await select.inputValue(),'3');
  for(const width of[320,390,430])for(const theme of['light','dark']){
   await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
   assert.equal(await page.locator('#panelContinuousAp').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} overflow`);
   assert.equal(await select.evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true,`${label} ${width}/${theme} clipped selector`);
   const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-continuous-ap-${width}-${theme}.png`),animations:'disabled'});if(width===320){await page.evaluate(()=>{openPanel('panelApDriveProfile');document.querySelector('#apDriveRegenSelect').disabled=false;document.querySelector('#apDriveProfileToggle').disabled=false});await page.screenshot({path:path.join(dir,`${label}-ap-drive-reference-${width}-${theme}.png`),animations:'disabled'});await page.evaluate(()=>openPanel('panelContinuousAp'));}}
  }
  topology=3;await page.evaluate(()=>fetchContinuousAp());assert.equal(await select.isDisabled(),true);assert.equal(await toggle.isDisabled(),false,'unsupported saved ON must still allow OFF');await page.locator('#panelContinuousAp label.toggle').click();await page.waitForFunction(()=>continuousApControl.saved?.enabled===false&&!continuousApControl.saving);assert.equal(await toggle.isDisabled(),true);
  failGet=true;await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.getByRole('button',{name:'Settings'}).click();await page.locator('#continuousApSettingsRow').click();assert.equal(await select.isDisabled(),true);assert.equal(await toggle.isDisabled(),true);
 }finally{await browser.close();}
}

// test_dashboard_v3281_continuous_ap.js: original scenarios and isolated page fixture.
async function checkContinuousApReadiness(html,label){
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:320,height:844}});let failGet=false;
  const response={enabled:true,method:2,effectiveEnabled:true,supported:true,stalkSupported:false,scrollSingleSupported:true,scrollDoubleSupported:true,supportReason:'Match your vehicle setting.',state:'Idle',blockedReason:'Waiting for AP active',lastCancelReason:'Brake applied',signalReadiness:'Available',attempts:0,txOk:0,txFail:0};
  const signals=[['Ap','0x399','0','Chassis CAN B'],['Turn','0x3F5','0','Chassis CAN B'],['Brake','0x39D','1','Chassis CAN B'],['Gear','0x118','4','Chassis CAN B'],['Torque','0x372','0.00 Nm','Body CAN A'],['Control','0x3C2','Idle','Body CAN A']];
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
  // Polling renders the selected brake source without assuming a fixed bus or ID.
  for(const [id,bus,status] of [['0x145','Body CAN A','Fresh'],['0x145','Chassis CAN B','Fresh'],['0x145','Party CAN A','Invalid'],['0x39D','Chassis CAN B','Fresh'],['0x145','CAN A / CAN B','Missing']]){
   Object.assign(response,{rxBrakeId:id,rxBrakeBus:bus,rxBrakeStatus:status,rxBrakeAgeMs:status==='Missing'?4294967295:42});
   await page.evaluate(()=>fetchContinuousAp());
   const text=await page.locator('#continuousApRxBrake').textContent();
   for(const expected of[id,bus,status])assert(text.includes(expected),`${label} brake source missing ${expected}: ${text}`);
  }
  Object.assign(response,{rxBrakeId:'0x39D',rxBrakeBus:'Chassis CAN B',rxBrakeStatus:'Fresh',rxBrakeAgeMs:42});
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

(async()=>{
  const sourceOnly=Boolean(process.env.DASHBOARD_SOURCE_ONLY||process.env.V328_DASHBOARD_SOURCE_ONLY);
  for(const {html,label} of dashboardArtifacts(root,sourceOnly)){
    await checkContinuousApSettings(html,label);
    await checkContinuousApReadiness(html,label);
  }
})().then(()=>console.log('PASS continuous ap source/embedded suite (2 preserved scenario groups)'),e=>{console.error(e);process.exitCode=1;});
