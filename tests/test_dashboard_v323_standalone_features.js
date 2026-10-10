const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');

async function check(html,label){
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

(async()=>{
  await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');
  if(!process.env.V323_DASHBOARD_SOURCE_ONLY){
    const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
    await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');
  }
})().then(()=>console.log('PASS standalone Driver Monitoring settings with Nag unavailable'),e=>{console.error(e);process.exitCode=1;});
