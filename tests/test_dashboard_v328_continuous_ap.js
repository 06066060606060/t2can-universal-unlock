const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');const root=path.resolve(__dirname,'..');
async function check(html,label){
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:320,height:844}}),requests=[];
  let enabled=false,method=0,profile=4,failGet=false,failPost=false,lostReply=false;
  await page.route('http://v328.test/**',async route=>{
   const request=route.request(),url=new URL(request.url());requests.push(`${request.method()} ${url.pathname}${url.search}`);
   if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
   let body={};
   if(url.pathname==='/api/profile/status')body={setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true,advancedEapSupported:true,euUnlockSupported:true};
   else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false,apDriveProfileSupported:true};
   else if(url.pathname==='/api/continuous-ap/config'){
    if(request.method()==='GET'&&failGet||request.method()==='POST'&&failPost)return route.fulfill({status:503,contentType:'application/json',body:'{"error":"unavailable"}'});
    if(request.method()==='POST'){enabled=url.searchParams.get('enabled')==='1';method=Number(url.searchParams.get('method'));if(lostReply){lostReply=false;return route.abort('failed');}}
    const supported=profile!==1;body={enabled,method,effectiveEnabled:enabled&&supported,supported,stalkSupported:profile===3,scrollSingleSupported:profile===4,scrollDoubleSupported:profile===4,supportReason:supported?'Match your vehicle setting.':'Unavailable for this vehicle connection.',state:'Idle',blockedReason:'Waiting for AP active',txOk:0,txFail:0};
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
  await select.click();assert.equal(await page.locator('#selectSheetList').getByRole('option',{name:'Stalk Down',exact:true}).count(),0);await page.locator('#selectSheetList').getByRole('option',{name:'Scroll Double Click',exact:true}).click();await page.waitForFunction(()=>continuousApSaved?.method===3&&!continuousApSaving);assert.equal(await toggle.isDisabled(),false);
  await page.locator('#panelContinuousAp label.toggle').click();await page.waitForFunction(()=>continuousApSaved?.enabled===true&&!continuousApSaving);
  assert(requests.includes('POST /api/continuous-ap/config?enabled=1&method=3'));
  await select.selectOption('2');await page.waitForFunction(()=>continuousApSaved?.method===2&&!continuousApSaving);assert.equal(enabled,true,'enabled method change should remain ON');
  failPost=true;await select.selectOption('3');await page.waitForFunction(()=>!continuousApSaving);assert.equal(await select.inputValue(),'2','failed save must reconcile saved method');failPost=false;
  lostReply=true;let posts=requests.filter(x=>x.startsWith('POST /api/continuous-ap')).length;await select.selectOption('3');await page.waitForFunction(()=>continuousApSaved?.method===3&&!continuousApSaving);assert.equal(requests.filter(x=>x.startsWith('POST /api/continuous-ap')).length,posts+1,'lost response must not repeat POST');
  failGet=true;failPost=true;await select.selectOption('2');await page.waitForFunction(()=>!continuousApSaving&&continuousApUncertain);assert.equal(await select.isDisabled(),true,'unresolved save must lock controls');assert.equal(await toggle.isDisabled(),true);
  failGet=false;failPost=false;await page.evaluate(()=>fetchContinuousAp());assert.equal(await select.inputValue(),'3');assert.equal(await select.isDisabled(),false);
  await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.getByRole('button',{name:'Settings'}).click();await page.locator('#continuousApSettingsRow').click();assert.equal(await toggle.isChecked(),true);assert.equal(await select.inputValue(),'3');
  for(const width of[320,390,430])for(const theme of['light','dark']){
   await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
   assert.equal(await page.locator('#panelContinuousAp').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} overflow`);
   assert.equal(await select.evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true,`${label} ${width}/${theme} clipped selector`);
   const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-continuous-ap-${width}-${theme}.png`),animations:'disabled'});if(width===320){await page.evaluate(()=>{openPanel('panelApDriveProfile');document.querySelector('#apDriveRegenSelect').disabled=false;document.querySelector('#apDriveProfileToggle').disabled=false});await page.screenshot({path:path.join(dir,`${label}-ap-drive-reference-${width}-${theme}.png`),animations:'disabled'});await page.evaluate(()=>openPanel('panelContinuousAp'));}}
  }
  profile=1;await page.evaluate(()=>fetchContinuousAp());assert.equal(await select.isDisabled(),true);assert.equal(await toggle.isDisabled(),false,'unsupported saved ON must still allow OFF');await page.locator('#panelContinuousAp label.toggle').click();await page.waitForFunction(()=>continuousApSaved?.enabled===false&&!continuousApSaving);assert.equal(await toggle.isDisabled(),true);
  failGet=true;await page.reload({waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);await page.getByRole('button',{name:'Settings'}).click();await page.locator('#continuousApSettingsRow').click();assert.equal(await select.isDisabled(),true);assert.equal(await toggle.isDisabled(),true);
 }finally{await browser.close();}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.V328_DASHBOARD_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS Continuous AP settings, durable reconciliation and mobile themes'),e=>{console.error(e);process.exitCode=1;});
