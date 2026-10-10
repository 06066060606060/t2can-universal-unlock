// Removing profile gating, persistence rollback, or response epochs must fail these UI checks.
const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');
async function check(html,label){
 const browser=await launchBrowser();
 try{
  const page=await browser.newPage({viewport:{width:320,height:844}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  let saved=false,profile=3,topology=2,reject=false,failRead=false,lateRead=null,readStarted=null,holdNext=false;
  const posts=[];
  const stats=()=>({mode:1,modeName:'Mode 1',bit18Mode:0,mode1TxWaitMode:1,mode1ReinjectEnabled:true,mode1DelayMs:150,mode2ReinjectEnabled:false,mode2DelayMs:150,fixedPolicy:true,hw3Enabled:saved,hw3Supported:[3,5].includes(profile)&&[2,3].includes(topology),hw3Active:saved&&[3,5].includes(profile)&&[2,3].includes(topology),bit47ModeName:saved&&[3,5].includes(profile)&&[2,3].includes(topology)?'STOCK':'FORCE_1',stockBit47Valid:true,stockBit47:0,bit47Valid:true,bit47:saved&&[3,5].includes(profile)&&[2,3].includes(topology)?0:1});
  await page.route('http://r79.test/**',async route=>{
   const u=new URL(route.request().url());if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
   let body={},status=200;
   if(u.pathname==='/api/r79/stats'){
    body=stats();if(failRead){status=503;body={error:'unavailable'};}
    if(holdNext){holdNext=false;await new Promise(resolve=>{lateRead=resolve;readStarted();});}
   }
   if(u.pathname==='/api/r79/update'){
    posts.push(u.searchParams.toString());
    if(reject){status=503;body={error:'hw3-save-failed'};}else{saved=u.searchParams.get('hw3Enabled')==='1';body=stats();}
   }
   await route.fulfill({status,contentType:'application/json',body:JSON.stringify(body)});
  });
  const setup=async({p,t})=>{universalBootReady=false;window.__t2ProfileId=p;window.__t2TopologyId=t;document.body.classList.remove('setup-active');openPanel('panelR79');await fetchR79();};
  await page.goto('http://r79.test/',{waitUntil:'domcontentloaded'});await page.evaluate(setup,{p:3,t:2});
  assert.equal(await page.locator('#r79Hw3Toggle').count(),1,'R79 panel must offer the saved HW3 opt-in');
  assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),false,'missing saved option defaults OFF');
  assert.equal(await page.locator('#r79Hw3Wrap').isVisible(),true);
  assert.match(await page.locator('#r79Bit47Value').textContent(),/FORCE.?1.*0.*1/);
  const set=enabled=>page.evaluate(async value=>{$('r79Hw3Toggle').checked=value;await updateR79Hw3();},enabled);
  await set(true);assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),true);
  assert.equal(posts.at(-1),'hw3Enabled=1','option changes must be a standalone POST');
  assert.match(await page.locator('#r79Bit47Value').textContent(),/STOCK.*0.*0/);
  await page.reload({waitUntil:'domcontentloaded'});await page.evaluate(setup,{p:3,t:2});
  assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),true,'saved ON survives reload');
  reject=true;await set(false);assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),true,'failed save restores saved ON');
  failRead=true;await set(false);assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),true,'failed save plus failed read retains last confirmed setting');failRead=false;reject=false;
  await set(false);assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),false);
  await page.reload({waitUntil:'domcontentloaded'});await page.evaluate(setup,{p:3,t:2});assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),false,'saved OFF survives reload');
  // Capture OFF before saving ON, release that old GET only after save completes.
  const started=new Promise(resolve=>{readStarted=resolve;});holdNext=true;
  await page.evaluate(()=>{window.__hw3LateFetch=fetchR79();});await started;
  await set(true);lateRead();await page.evaluate(()=>window.__hw3LateFetch);
  assert.equal(await page.locator('#r79Hw3Toggle').isChecked(),true,'late polling response cannot overwrite confirmed ON');
  for(const [p,t,want] of [[3,2,true],[3,3,true],[5,2,true],[5,3,true],[1,1,false],[2,2,false],[2,3,false],[4,2,false],[4,3,false],[0,0,false],[3,0,false]]){
   profile=p;topology=t;await page.evaluate(setup,{p,t});
   assert.equal(await page.locator('#r79Hw3Wrap').isVisible(),want,`${label} profile ${p}/${t} visibility`);
   assert.equal(await page.locator('#r79Hw3Toggle').isDisabled(),!want,`${label} unsupported setting disabled`);
   assert.equal(await page.locator('#r79Bit47Value').isVisible(),want,`${label} unsupported HW3 diagnostics hidden`);
  }
  profile=3;topology=2;await page.evaluate(setup,{p:3,t:2});
  const unknown=stats();unknown.stockBit47Valid=false;unknown.bit47Valid=false;await page.evaluate(r=>fetchR79(r),unknown);assert.match(await page.locator('#r79Bit47Value').textContent(),/—.*—/);
  await page.evaluate(()=>fetchR79());
  for(const width of [320,390,430])for(const theme of ['light','dark']){
   await page.setViewportSize({width,height:844});await page.evaluate(t=>window.t2Theme.setMode(t),theme);
   assert.equal(await page.locator('#panelR79').evaluate(e=>e.scrollWidth<=innerWidth),true,`${label} ${width}/${theme} panel overflow`);
   const fit=await page.locator('#r79Hw3Wrap').evaluate(e=>{const r=e.getBoundingClientRect(),a=e.querySelector('.toggle').getBoundingClientRect(),b=e.querySelector('.listkey').getBoundingClientRect();return r.left>=0&&r.right<=innerWidth&&b.right<=a.left;});assert.equal(fit,true,`${label} ${width}/${theme} toggle fits`);
   if(process.env.DASHBOARD_SCREENSHOT_DIR){fs.mkdirSync(process.env.DASHBOARD_SCREENSHOT_DIR,{recursive:true});await page.screenshot({path:path.join(process.env.DASHBOARD_SCREENSHOT_DIR,`${label}-r79-hw3-${width}-${theme}.png`),fullPage:true,animations:'disabled'});}
  }
  assert.deepEqual(errors,[]);
 }finally{await browser.close();}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.R79_HW3_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('PASS R79 HW3 source/embedded persistence, rollback, late polling, profile gating and 320/390/430px light/dark'),e=>{console.error(e);process.exitCode=1;});
