const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {launchBrowser,dashboardArtifacts,dashboardFonts}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');
const fonts=dashboardFonts(root);

// test_dashboard_v323_standalone_features.js: original scenarios and isolated page fixture.
async function checkDriverMonitoring(html,label){
  const browser=await launchBrowser();
  try{
    const page=await browser.newPage({viewport:{width:320,height:844}}),requests=[];
    let enabled=false;
    await page.route('http://v323.test/**',async route=>{
      const request=route.request(),url=new URL(request.url());
      requests.push(`${request.method()} ${url.pathname}${url.search}`);
      if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
      let body={};
      if(url.pathname==='/api/profile/status')body={setupMode:false,profile:1,topology:1,turn:1,nagSupported:false,nagTorqueSupported:false,nagTsl9Supported:false,advancedEapSupported:true,euUnlockSupported:true};
      else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false};
      else if(url.pathname==='/api/driver-monitoring/config'){
        if(request.method()==='POST'&&url.searchParams.has('enabled'))enabled=url.searchParams.get('enabled')==='1';
        body={enabled,supported:true};
      }
      await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
    });
    await page.goto('http://v323.test/',{waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.getByRole('button',{name:'Settings'}).click();
    assert.equal(await page.locator('#driverMonitoringRow').count(),1,'standalone Settings row missing');
    assert.equal(await page.locator('#panelNag #nagDmsToggle').count(),0,'Nag panel still owns Driver Monitoring');
    assert.equal(await page.locator('#driverMonitoringToggle').isDisabled(),false,'DMS must work when Nag is unsupported');
    assert.equal(await page.locator('#driverMonitoringToggle').isChecked(),false);
    await page.locator('#driverMonitoringRow label.toggle').click();
    await page.waitForFunction(()=>document.querySelector('#driverMonitoringToggle')?.checked===true);
    assert(requests.includes('POST /api/driver-monitoring/config?enabled=1'));
    assert.equal(requests.some(item=>item.startsWith('GET /api/nag/config')),false,'standalone DMS must not require Nag config');
    await page.reload({waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.getByRole('button',{name:'Settings'}).click();
    assert.equal(await page.locator('#driverMonitoringToggle').isChecked(),true,'saved standalone selection must reload');
    await page.evaluate(()=>{document.querySelector('#selectSheet')?.classList.remove('show');document.querySelector('#selectSheetBack')?.classList.remove('show');document.activeElement?.blur();});
    for(const width of[320,390,430])for(const theme of['light','dark']){
      await page.setViewportSize({width,height:844});
      await page.evaluate(t=>window.t2Theme.setMode(t),theme);
      assert.equal(await page.locator('main[data-page="settings"]').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}px overflow`);
      assert.equal(await page.locator('#driverMonitoringRow').evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true,`${label} ${width}/${theme} Driver Monitoring clipping`);
      const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-standalone-driver-monitoring-${width}-${theme}.png`),fullPage:false,animations:'disabled'});}
    }
  }finally{await browser.close();}
}


// test_dashboard_v326_isa_standalone.js: original scenarios and isolated page fixture.
async function checkIsaSuppression(html,label){
  const browser=await launchBrowser();
  try{
    const page=await browser.newPage({viewport:{width:320,height:844}}),requests=[];
    let enabled=false,failIsaRequests=0;
    await page.route('http://v326.test/**',async route=>{
      const request=route.request(),url=new URL(request.url());
      requests.push(`${request.method()} ${url.pathname}${url.search}`);
      if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
      let body={};
      if(url.pathname==='/api/profile/status')body={setupMode:false,profile:3,topology:3,turn:0,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:false,advancedEapSupported:false,euUnlockSupported:true};
      else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false};
      else if(url.pathname==='/api/driver-monitoring/config')body={enabled:false,supported:true,active:false};
      else if(url.pathname==='/api/isa-suppression/config'){
        if(failIsaRequests>0){failIsaRequests--;return route.fulfill({status:503,contentType:'application/json',body:'{"ok":false}'})}
        if(request.method()==='POST'&&url.searchParams.has('enabled'))enabled=url.searchParams.get('enabled')==='1';
        body={enabled,supported:true,active:enabled};
      }
      await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
    });
    await page.goto('http://v326.test/',{waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.getByRole('button',{name:'Settings'}).click();
    assert.equal(await page.locator('#isaSuppressionRow').count(),1,'standalone Settings row missing');
    assert.equal(await page.locator('#panelNag #tsl9IsaWrap').count(),0,'Nag panel still owns ISA Suppression');
    assert.equal(await page.locator('#isaSuppressionToggle').isDisabled(),false,'Party+Chassis ISA must work with TSL9 unavailable');
    assert.equal(await page.locator('#isaSuppressionToggle').isChecked(),false);
    await page.locator('#isaSuppressionRow label.toggle').click();
    await page.waitForFunction(()=>document.querySelector('#isaSuppressionToggle')?.checked===true);
    assert(requests.includes('POST /api/isa-suppression/config?enabled=1'));
    assert.equal(requests.some(item=>item.startsWith('POST /api/nag/')),false,'standalone ISA must not write Nag config');
    await page.reload({waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.getByRole('button',{name:'Settings'}).click();
    assert.equal(await page.locator('#isaSuppressionToggle').isChecked(),true,'saved standalone selection must reload');
    failIsaRequests=2;
    await page.locator('#isaSuppressionRow label.toggle').click();
    await page.waitForTimeout(100);
    assert.equal(await page.locator('#isaSuppressionToggle').isChecked(),true,'failed save and readback must restore confirmed selection');
    for(const width of[320,390,430])for(const theme of['light','dark']){
      await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
      assert.equal(await page.locator('main[data-page="settings"]').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} overflow`);
      assert.equal(await page.locator('#isaSuppressionRow').evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth;}),true,`${label} ${width}/${theme} ISA clipping`);
      const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-isa-suppression-${width}-${theme}.png`),fullPage:false,animations:'disabled'});}
    }
  }finally{await browser.close();}
}


// test_dashboard_settings_transactions.js: original scenarios and isolated page fixture.
async function checkSteadyPolling(html,label){
 const code=html.slice(html.indexOf('function createSettingsController('),html.indexOf('\nfunction enabledSettingsResponse'));
 for(const interval of[800,1500])for(const latencyTicks of[2,4]){
  let tick=0;const requests=[];
  const get=()=>new Promise(resolve=>requests.push({tick,resolve,done:false}));
  const factory=new Function('jget','featurePostJson',code+';return createSettingsController;')(get,async()=>({enabled:false,supported:true}));
  const state=factory({url:'/config',validate:x=>x,render:()=>{},status:()=>{}});
  for(tick=0;tick<20;tick++){
   state.refresh();
   for(const request of requests)if(!request.done&&request.tick<=tick-latencyTicks){request.done=true;request.resolve({enabled:false,supported:true});}
   await new Promise(resolve=>setImmediate(resolve));
  }
  assert(requests.some(r=>r.done),'probe must complete successful GETs');
  assert(state.saved!==null&&!state.uncertain,`${label}: successful GETs delayed ${interval*latencyTicks}ms must publish during sustained ${interval}ms polling`);
  assert(requests.length<=Math.ceil(20/(latencyTicks+1)),`${label}: polling coalesces an in-flight GET`);
 }
}
async function checkSettingsTransactions(html,label){
 await checkSteadyPolling(html,label);
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:320,height:844}});
  let failPost=false,failGet=false;
  await page.route('http://settings.test/**',async route=>{
   const r=route.request(),u=new URL(r.url());
   if(fonts[u.pathname])return route.fulfill({contentType:'font/woff2',body:fonts[u.pathname]});
   if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
   let body={};
   if(u.pathname==='/api/profile/status')body={setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true,advancedEapSupported:true,euUnlockSupported:true};
   if(u.pathname==='/api/features/status')body={lab:false,s3xy:false};
   if(u.pathname==='/api/driver-monitoring/config'){
    if(r.method()==='POST'&&failPost||r.method()==='GET'&&failGet)return route.fulfill({status:503,contentType:'application/json',body:'{"error":"unavailable"}'});
    body={enabled:false,supported:true};
   }
   return route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
  });
  await page.goto('http://settings.test/',{waitUntil:'domcontentloaded'});await page.waitForFunction(()=>universalBootReady);
  failPost=true;failGet=true;
  await page.evaluate(async()=>{document.querySelector('#driverMonitoringToggle').checked=true;await updateDriverMonitoringControl();});
  assert.equal(await page.locator('#driverMonitoringToggle').isChecked(),false,`${label}: rejected DMS save plus failed reconciliation must preserve confirmed OFF`);
  assert.equal(await page.locator('#driverMonitoringToggle').isDisabled(),true,`${label}: unresolved DMS result must lock writes`);
  const cases=await page.evaluate(async()=>{
   universalBootReady=false;
   const check=(value,message)=>{if(!value)throw new Error(message)},deferred=()=>{let resolve,reject;const promise=new Promise((a,b)=>{resolve=a;reject=b});return {promise,resolve,reject}};
   const features=[
    {name:'DMS',state:driverMonitoringControl,fetch:fetchDriverMonitoringControl,update:()=>updateDriverMonitoringControl(),id:'driverMonitoringToggle',status:'driverMonitoringSaveState',value:s=>s.enabled,response:(enabled=false,supported=true)=>({enabled,supported}),choose:v=>$('driverMonitoringToggle').checked=v},
    {name:'ISA',state:isaSuppressionControl,fetch:fetchIsaSuppressionControl,update:()=>updateIsaSuppressionControl(),id:'isaSuppressionToggle',status:'isaSuppressionSaveState',value:s=>s.enabled,response:(enabled=false,supported=true)=>({enabled,supported}),choose:v=>$('isaSuppressionToggle').checked=v},
    {name:'Country',state:countryOverrideControl,fetch:fetchCountryOverrideLab,update:()=>updateCountryOverrideMode(),id:'countryOverrideMode',status:'countryOverrideSaveState',value:s=>s.countryMode!==0,response:(enabled=false,supported=true)=>({countryMode:enabled?2:0,mapMode:0,countrySupported:supported}),choose:v=>$('countryOverrideMode').value=v?'2':'0'},
    {name:'Continuous AP',state:continuousApControl,fetch:fetchContinuousAp,update:()=>updateContinuousAp('enabled'),id:'continuousApToggle',status:'continuousApSaveState',value:s=>s.enabled,response:(enabled=false,supported=true)=>({enabled,method:2,supported,scrollSingleSupported:supported,scrollDoubleSupported:supported,stalkSupported:false,effectiveEnabled:enabled&&supported,signalReadiness:'Available'}),choose:v=>$('continuousApToggle').checked=v}
   ];
   let count=0;const fail=async()=>{throw new Error('network failed')};
   for(const f of features){
    let posts=0;
    const seed=async(enabled=false,supported=true)=>{jget=async()=>f.response(enabled,supported);await f.fetch();check(!f.state.uncertain,f.name+' seed');posts=0};
    await seed();featurePostJson=async()=>{posts++;throw new Error('write failed')};jget=fail;f.choose(true);await f.update();
    check(!f.value(f.state.saved)&&f.state.uncertain&&!f.state.saving,f.name+' preserves confirmed state on double failure');
    check($(f.id).disabled&&/unknown|unavailable/i.test($(f.status).textContent),f.name+' uncertainty visible and locked');
    f.choose(true);await f.update();check(posts===1,f.name+' unknown result forbids another write');++count;
    jget=async()=>f.response();await f.fetch();check(!f.state.uncertain&&!$(f.id).disabled,f.name+' confirmed GET recovers');++count;
    featurePostJson=async()=>{posts++;throw new Error('response lost after commit')};jget=async()=>f.response(true);f.choose(true);await f.update();
    check(f.value(f.state.saved)&&!f.state.uncertain&&posts===2,f.name+' lost POST reply reconciles without retransmit');++count;
    await seed();const oldPoll=deferred();jget=()=>oldPoll.promise;const pending=f.fetch();featurePostJson=async()=>{posts++;return f.response(true)};f.choose(true);await f.update();oldPoll.resolve(f.response());await pending;
    check(f.value(f.state.saved)&&!f.state.uncertain,f.name+' stale successful poll cannot overwrite save');++count;
    await seed();const oldError=deferred();jget=()=>oldError.promise;const errorPending=f.fetch();f.choose(true);await f.update();oldError.reject(new Error('old GET failed'));await errorPending;
    check(f.value(f.state.saved)&&!f.state.uncertain,f.name+' stale failed poll cannot invalidate save');++count;
    await seed();const shared=deferred();let reads=0;jget=()=>{reads++;return shared.promise};const sharedFirst=f.fetch(),sharedSecond=f.fetch();shared.resolve(f.response(true));await Promise.all([sharedFirst,sharedSecond]);
    check(reads===1&&f.value(f.state.saved)&&!f.state.uncertain,f.name+' concurrent polls share one GET');++count;
    await seed();const invalidated=deferred();jget=()=>invalidated.promise;const invalidatedPending=f.fetch();featurePostJson=async()=>f.response(true);f.choose(true);await f.update();jget=async()=>f.response();await f.fetch();invalidated.resolve(f.response(true));await invalidatedPending;
    check(!f.value(f.state.saved)&&!f.state.uncertain,f.name+' post-save refresh bypasses invalidated pending GET');++count;
    await seed();const write=deferred();featurePostJson=()=>{posts++;return write.promise};f.choose(true);const saving=f.update();f.choose(true);await f.update();check(posts===1&&f.state.saving,f.name+' duplicate click posts once');write.resolve(f.response(true));await saving;check(!f.state.saving&&f.value(f.state.saved),f.name+' first write completes');++count;
    await seed();featurePostJson=async()=>{posts++;return {}};jget=async()=>({});f.choose(true);await f.update();check(!f.value(f.state.saved)&&f.state.uncertain&&$(f.id).disabled,f.name+' malformed POST and GET cannot publish');++count;
    await seed();jget=fail;await f.fetch();f.choose(true);await f.update();check(f.state.uncertain&&posts===0&&$(f.id).disabled,f.name+' failed fresh poll locks writes');++count;
    await seed(false,false);featurePostJson=async()=>{posts++;return f.response(true)};f.choose(true);await f.update();check(posts===0&&$(f.id).disabled,f.name+' unsupported cannot enable');++count;
   }
   // Continuous AP uniquely permits an already-enabled unsupported method to turn OFF.
   jget=async()=>({enabled:true,method:2,supported:false,scrollSingleSupported:false});await fetchContinuousAp();
   check(!$('continuousApToggle').disabled,'unsupported stored Continuous AP ON can turn OFF');
   let offPosts=0;featurePostJson=async url=>{offPosts++;check(url.includes('enabled=0'),'Continuous AP OFF request');return {enabled:false,method:2,supported:false}};
   $('continuousApToggle').checked=false;await updateContinuousAp('enabled');check(offPosts===1&&$('continuousApToggle').disabled,'unsupported Continuous AP OFF stays gated');++count;
   // Leave DMS/ISA in the longest uncertain state for actual mobile layout checks.
   for(const f of features.slice(0,2)){jget=async()=>f.response();await f.fetch();featurePostJson=fail;jget=fail;f.choose(true);await f.update();}
   document.body.classList.remove('setup-active');showPage('settings');
   let recoveryPosts=0;featurePostJson=async()=>{recoveryPosts++;throw new Error('unexpected write')};
   jget=async url=>url==='/api/driver-monitoring/config'||url==='/api/isa-suppression/config'?{enabled:false,supported:true}:{};
   pollLast.driverMonitoringRecovery=0;pollLast.isaSuppressionRecovery=0;pollTick(false);
   await new Promise(resolve=>setTimeout(resolve,0));
   check(!driverMonitoringControl.uncertain&&!isaSuppressionControl.uncertain&&recoveryPosts===0,'Settings polling recovers uncertain toggles using GET only');++count;
   for(const f of features.slice(0,2)){featurePostJson=fail;jget=fail;f.choose(true);await f.update();}
   return count;
  });
  assert.equal(cases,46,`${label}: complete controller/feature matrix`);
  for(const width of[320,390,430])for(const theme of['light','dark']){
   await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
   await page.evaluate(()=>document.fonts.ready);
   for(const id of['driverMonitoringRow','isaSuppressionRow']){
    await page.locator('#'+id).scrollIntoViewIfNeeded();
    assert.equal(await page.locator('#'+id).evaluate(e=>{const r=e.getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&e.scrollWidth<=e.clientWidth;}),true,`${label}: ${id} ${width}/${theme} overflow`);
    assert.equal(await page.locator('#'+id+' .listdesc').evaluateAll(es=>es.every(e=>e.scrollWidth<=e.clientWidth)),true,`${label}: status text wraps`);
    const dir=process.env.DASHBOARD_SCREENSHOT_DIR;if(dir){fs.mkdirSync(dir,{recursive:true});await page.screenshot({path:path.join(dir,`${label}-${id}-uncertain-${width}-${theme}.png`),animations:'disabled'});}
   }
  }
 }finally{await browser.close();}
}

(async()=>{
  const sourceOnly=Boolean(process.env.DASHBOARD_SOURCE_ONLY||process.env.V323_DASHBOARD_SOURCE_ONLY||process.env.V326_DASHBOARD_SOURCE_ONLY||process.env.SETTINGS_DASHBOARD_SOURCE_ONLY);
  for(const {html,label} of dashboardArtifacts(root,sourceOnly)){
    await checkDriverMonitoring(html,label);
    await checkIsaSuppression(html,label);
    await checkSettingsTransactions(html,label);
  }
})().then(()=>console.log('PASS settings controls source/embedded suite (3 preserved scenario groups)'),e=>{console.error(e);process.exitCode=1;});
