const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');

async function check(html,label){
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

(async()=>{
  await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');
  if(!process.env.V326_DASHBOARD_SOURCE_ONLY){
    const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
    await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');
  }
})().then(()=>console.log('PASS standalone ISA Suppression on Party+Chassis with TSL9 unavailable'),e=>{console.error(e);process.exitCode=1;});
