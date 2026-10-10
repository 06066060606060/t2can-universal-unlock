// Catches accepting failed-save error JSON as NAG config and retaining toggles
// after NVS errors; verifies the actual source and embedded dashboard handlers.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime'),root=path.resolve(__dirname,'..');
const savedNag={enabled:true,ignoreApState:false,pauseAtZeroSpeed:false,method:0,mode:0,targetId:0x370,apStateId:0x399,steeringId:0x129,hoRatePct:37,burstMs:1000,pauseMs:1500,torque:[{b2:8,b3:182}],torqueRightScrollEnabled:false,torqueRightScrollIntervalSeconds:60,torqueRightScrollPattern:0,tsl9RightPeriodicEnabled:false,tsl9RightPeriodicIntervalSeconds:60};
async function check(html,label){
 const browser=await launchBrowser();try{
  const page=await browser.newPage({viewport:{width:390,height:844}});let failRead=false;
  await page.addInitScript(()=>{window.setInterval=()=>0;window.confirm=()=>true;});
  await page.route('http://v3282.test/**',async route=>{
   const request=route.request(),url=new URL(request.url());
   if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
   if(request.method()==='POST')return route.fulfill({status:500,contentType:'application/json',body:'{"ok":false,"error":"Settings could not be saved"}'});
   if(failRead&&['/api/nag/config','/api/blinkA/stats','/api/summon/stats'].includes(url.pathname))return route.fulfill({status:503,contentType:'application/json',body:'{"error":"unavailable"}'});
   let body={};
   if(url.pathname==='/api/profile/status')body={setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true,advancedEapSupported:true,euUnlockSupported:true};
   else if(url.pathname==='/api/features/status')body={lab:false,s3xy:false,doorCancel:false,apDriveProfileSupported:true,apDriveProfileEnabled:false,apDriveProfileRegenRaw:10};
   else if(url.pathname==='/api/nag/config')body=savedNag;
   else if(url.pathname==='/api/blinkA/stats')body={enabled:false,delayMs:1000,noaStabilizationSeconds:5,cancelPauseSeconds:30};
   else if(url.pathname==='/api/summon/stats')body={tlssc:false,tlsscHighwayGateEnabled:false,tlsscBlockInNoa:false};
   await route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
  });
  await page.goto('http://v3282.test/',{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>universalBootReady);
  await page.evaluate(async()=>{await loadNag();await fetchBlink();await fetchSum();});
  const failures=[];
  for(const id of ['nagApply','nagReset']){
   await page.evaluate(async id=>{await loadNag();await document.getElementById(id).onclick();},id);
   const config=await page.evaluate(()=>nagCfg);
   if(config.enabled!==true||config.mode!==0||config.hoRatePct!==37||!Array.isArray(config.torque))failures.push(`${label} ${id}: failed-save error response replaced saved NAG configuration`);
  }
  for(const [id,call]of [['blinkToggle','toggleBlink'],['tlsscToggle','toggleTlssc']]){
   await page.evaluate(async ({id,call})=>{document.getElementById(id).checked=true;await window[call](true);await new Promise(resolve=>setTimeout(resolve,100));},{id,call});
   if(await page.locator('#'+id).isChecked())failures.push(`${label} ${id}: failed-save toggle was not restored from controller state`);
  }
  failRead=true;
  for(const [id,call]of [['blinkToggle','toggleBlink'],['tlsscToggle','toggleTlssc']]){
   await page.evaluate(async ({id,call})=>{document.getElementById(id).checked=true;await window[call](true);},{id,call});
   for(const control of [id,id+'2'])if(await page.locator('#'+control).isChecked())failures.push(`${label} ${control}: failed save + failed GET must retain the last confirmed toggle state`);
  }
  await page.evaluate(async()=>{await document.getElementById('nagReset').onclick();});
  assert.equal(await page.evaluate(()=>nagCfg.enabled),true,`${label}: failed NAG reset/read must retain confirmed configuration`);
  assert.deepEqual(failures,[],failures.join('\n'));
 }finally{await browser.close();}
}
(async()=>{
 if(process.env.V3282_DASH_SOURCE)return check(fs.readFileSync(process.env.V3282_DASH_SOURCE,'utf8'),'baseline');
 await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');
 const header=fs.readFileSync(path.join(root,'index_html.h'),'utf8');
 const bytes=[...header.matchAll(/0x([0-9a-fA-F]{2})/g)].map(m=>parseInt(m[1],16));
 await check(zlib.gunzipSync(Buffer.from(bytes)).toString(),'embedded');
})().then(()=>process.stdout.write('Settings NVS error recovery: '+(process.env.V3282_DASH_SOURCE?'selected source':'source + embedded')+' PASS\n'),error=>{process.stderr.write((error.stack||error)+'\n');process.exitCode=1;});
