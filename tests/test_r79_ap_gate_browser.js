const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { launchBrowser } = require('./browser_test_runtime');
const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(process.env.DASHBOARD_HTML_PATH || path.join(root, 'dashboard_source.html'), 'utf8');
async function checkDashboard(html, variant) {
  const browser = await launchBrowser();
  try {
    const page = await browser.newPage({viewport:{width:390,height:844}});
    let state={enabled:false,mode:0,delaySeconds:2,allowManualDriving:false,apActive:false,gateAllowed:true,gateReason:'BYPASS',remainingMs:0};
    let fail=false,updates=0,hold=null,holdStats=false,statsRelease,statsStarted;const posted=[];
    await page.route('http://r79.test/**', async route => {
      const url=new URL(route.request().url());let body={};
      if(url.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
      if(url.pathname==='/api/features/status')body={lab:true};
      if(url.pathname==='/api/profile/status')body={setupMode:false,profile:1,topology:1,turn:1};
      if(url.pathname==='/api/r79/ap-control/stats'){body=state;if(holdStats){holdStats=false;await new Promise(resolve=>{statsRelease=resolve;statsStarted();});}}
      if(url.pathname==='/api/r79/ap-control/update'){
        updates++;posted.push(url.searchParams.toString());if(hold)await hold;
        if(fail)return route.fulfill({status:500,contentType:'application/json',body:'{"error":"save-failed"}'});
        state={...state,enabled:url.searchParams.get('enabled')==='1',mode:Number(url.searchParams.get('mode')),delaySeconds:Number(url.searchParams.get('delaySeconds')),allowManualDriving:url.searchParams.get('allowManualDriving')==='1'};body=state;
      }
      return route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
    });
    await page.goto('http://r79.test/');
    await page.waitForFunction(()=>universalBootReady);await page.evaluate(()=>{universalBootReady=false;});
    assert.equal(await page.locator('#r79ApControlToggle').count(),1,'LAB R79 AP master control must exist');
    assert.equal(await page.locator('#r79AllowManualDriving').count(),1,'manual-driving permission must be independently exposed in the existing R79 AP card');
    await page.evaluate(async()=>{labEnabled=true;document.body.classList.remove('setup-active');openPanel('panelLabR79ApControl');await fetchR79ApControl()});
    const toggle=page.locator('#r79ApControlToggle'),mode=page.locator('#r79ApControlMode'),delay=page.locator('#r79ApDelaySeconds');
    const manual=page.locator('#r79AllowManualDriving');
    assert.equal(await toggle.isChecked(),false);assert.equal(await page.locator('#r79ApControls').isVisible(),false);
    assert.equal(await manual.isChecked(),false,'default OFF retains manual suppression');assert.equal(await page.locator('label.toggle:has(#r79AllowManualDriving)').isVisible(),true,'manual toggle remains visible while master is OFF');assert.equal(await manual.isDisabled(),false);
    await page.locator('label.toggle:has(#r79AllowManualDriving)').click();await page.waitForFunction(()=>!r79ApSaving&&r79ApSaved.allowManualDriving);
    assert.equal(await toggle.isChecked(),false,'manual permission does not enable AP policy master');assert.equal(posted.at(-1),'enabled=0&mode=0&delaySeconds=2&allowManualDriving=1');
    await page.reload();await page.waitForFunction(()=>universalBootReady);await page.evaluate(async()=>{universalBootReady=false;labEnabled=true;document.body.classList.remove('setup-active');openPanel('panelLabR79ApControl');await fetchR79ApControl();});assert.equal(await manual.isChecked(),true,'independent permission persists across reload');assert.equal(await toggle.isChecked(),false);
    fail=true;await page.locator('label.toggle:has(#r79AllowManualDriving)').click();await page.waitForFunction(()=>!r79ApSaving);assert.equal(await manual.isChecked(),true,'failed save restores saved manual permission');fail=false;
    holdStats=true;const ready=new Promise(resolve=>{statsStarted=resolve;});await page.evaluate(()=>{window.__lateManual=fetchR79ApControl();});await ready;await page.locator('label.toggle:has(#r79AllowManualDriving)').click();await page.waitForFunction(()=>!r79ApSaving&&!r79ApSaved.allowManualDriving);statsRelease();await page.evaluate(()=>window.__lateManual);assert.equal(await manual.isChecked(),false,'late poll cannot restore obsolete permission');
    await page.locator('label.toggle:has(#r79ApControlToggle)').click();await page.waitForFunction(()=>!r79ApSaving&&r79ApSaved.enabled);
    assert.equal(await page.locator('#r79ApControls').isVisible(),true);assert.equal(await page.locator('#r79ApDelayWrap').isVisible(),false);
    assert.equal(await manual.isChecked(),false,'master update preserves independent manual permission');
    await mode.selectOption('1',{force:true});await page.waitForFunction(()=>!r79ApSaving&&r79ApSaved.mode===1);
    assert.equal(await delay.isVisible(),true);assert.equal(await delay.inputValue(),'2');
    await delay.fill('10');await delay.dispatchEvent('change');await page.waitForFunction(()=>!r79ApSaving&&r79ApSaved.delaySeconds===10);
    for(const value of ['1','11','2.5','']){const count=updates;await delay.fill(value);await delay.dispatchEvent('change');assert.equal(updates,count);assert.equal(await delay.inputValue(),'10');}
    fail=true;await delay.fill('4');await delay.dispatchEvent('change');await page.waitForFunction(()=>!r79ApSaving);assert.equal(await delay.inputValue(),'10');assert.match(await page.locator('#r79ApSaveState').textContent(),/Save failed/);fail=false;
    let release;hold=new Promise(resolve=>release=resolve);await delay.fill('6');await delay.dispatchEvent('change');await page.waitForFunction(()=>r79ApSaving);assert.equal(await toggle.isDisabled(),true);assert.equal(await mode.isDisabled(),true);assert.equal(await delay.isDisabled(),true);assert.equal(await manual.isDisabled(),true);release();hold=null;await page.waitForFunction(()=>!r79ApSaving&&r79ApSaved.delaySeconds===6);
    for(const reason of ['BYPASS','NON_AP','AP_BLOCKED','AP_WAIT','AP_READY','AP_UNKNOWN']){state={...state,gateReason:reason,remainingMs:1250};await page.evaluate(()=>fetchR79ApControl());assert.equal(await page.locator('#r79ApGateState').textContent(),reason);assert.equal(await page.locator('#r79ApRemaining').textContent(),reason==='AP_WAIT'?'2 s':'—');}
    for(const txState of ['AP BLOCKED','AP WAIT','AP UNKNOWN']){
      await page.evaluate(txState=>renderR79Summary({txState,apWaitRemainingMs:1250}),txState);
      assert.equal(await page.locator('#homeR79State').textContent(),txState==='AP WAIT'?'AP Wait · 2 s':txState);
    }
    await delay.blur();
    const dir=process.env.DASHBOARD_SCREENSHOT_DIR;
    if(dir)fs.mkdirSync(dir,{recursive:true});
    for(const width of [320,390,430])for(const theme of ['light','dark']){
      await page.setViewportSize({width,height:844});await page.evaluate(theme=>window.t2Theme.setMode(theme),theme);
      const layout=await delay.evaluate(input=>{const field=input.closest('.field'),label=field.querySelector('label'),a=input.getBoundingClientRect(),b=label.getBoundingClientRect(),f=field.getBoundingClientRect();return{right:a.right,left:a.left,labelRight:b.right,fieldRight:f.right,centerDelta:Math.abs((a.top+a.bottom-b.top-b.bottom)/2),align:getComputedStyle(input).textAlign,scroll:document.getElementById('panelLabR79ApControl').scrollWidth}});
      assert.equal(layout.scroll,width,`${width}/${theme} overflow`);assert.equal(layout.align,'right');assert(layout.left>layout.labelRight);assert(layout.right<=layout.fieldRight);assert(layout.centerDelta<2);
      if(dir)await page.screenshot({path:path.join(dir,`r79-ap-${variant}-${width}-${theme}.png`),fullPage:true,animations:'disabled'});
    }
    state={enabled:false,mode:0,delaySeconds:2,allowManualDriving:false,gateReason:'BYPASS'};await page.evaluate(()=>fetchR79ApControl());assert.equal(await toggle.isChecked(),false);assert.equal(await manual.isChecked(),false);assert.equal(await page.locator('label.toggle:has(#r79AllowManualDriving)').isVisible(),true);assert.equal(await delay.inputValue(),'2');assert.equal(await page.locator('#r79ApControls').isVisible(),false);
    await page.evaluate(()=>{labEnabled=false;renderR79ApControl(r79ApSaved)});assert.equal(await toggle.isDisabled(),true);assert.equal(await manual.isDisabled(),true);
  } finally { await browser.close(); }
}
async function main() {
  await checkDashboard(html, 'source');
  if (!process.env.DASHBOARD_HTML_PATH && process.env.R79_AP_SOURCE_ONLY !== '1') {
    const header=fs.readFileSync(path.join(root,'index_html.h'),'utf8');
    const data=header.match(/INDEX_HTML_GZ\[\][^=]*=\s*\{([\s\S]*?)\};/)[1];
    const embedded=require('node:zlib').gunzipSync(Buffer.from(data.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString();
    await checkDashboard(embedded, 'embedded');
  }
}
main().then(()=>console.log('R79 AP browser: PASS (defaults, visibility, bounds, saves, failure recovery, busy controls, 6 gate states, reset, LAB capability, 6 layouts)'),e=>{console.error(e);process.exitCode=1});
