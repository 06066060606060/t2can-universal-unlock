const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..'),screens='/private/tmp/t2can-v3251-layout';

async function check(html,label){
  const browser=await launchBrowser();
  try{
    const page=await browser.newPage({viewport:{width:320,height:844}}),errors=[];
    page.on('pageerror',e=>errors.push(e.message));
    await page.route('http://v3251.test/**',route=>{
      const u=new URL(route.request().url());
      if(u.pathname==='/')return route.fulfill({contentType:'text/html',body:html});
      const body=u.pathname==='/api/features/status'?{lab:true,s3xy:true}:u.pathname==='/api/profile/status'?{setupMode:false,profile:1,topology:1,turn:0}:{};
      return route.fulfill({contentType:'application/json',body:JSON.stringify(body)});
    });
    await page.goto('http://v3251.test/',{waitUntil:'domcontentloaded'});
    await page.waitForFunction(()=>universalBootReady);
    await page.evaluate(()=>{universalBootReady=false;labEnabled=true;s3xyEnabled=true;document.body.classList.remove('setup-active');
      // Make conditional diagnostic blocks inspectable without changing their markup.
      for(const row of document.querySelectorAll('.r:not(.rNote)')){
        for(let e=row.parentElement;e&&!e.matches('.panel,main');e=e.parentElement){
          e.classList.remove('tuUiHidden','hiddenByProfile');if(e.style.display==='none')e.style.display='';if(e.tagName==='DETAILS')e.open=true;
        }
        const value=row.lastElementChild;if(!value||row.children.length!==2)continue;
        value.textContent='4294967295 / 4294967295 / 4294967295';
      }
      const fixture={sysTwaiBusOffAge:'CURRENT BOOT · age 124.1 s · SAVED',sysCanBTxTrace:'4294967295 frames · BUS OFF #4294967295 · CURRENT BOOT · age 124.1 s · SAVED',sysCanATxTrace:'4294967295 frames · BUS OFF #4294967295 · PREVIOUS BOOT · event uptime 124.1 s · SAVED',sysFw2:'3.25.1'};
      for(const [id,text]of Object.entries(fixture))if($(id))$(id).textContent=text;
      for(const e of document.querySelectorAll('.r > :last-child'))if(/hash|raw|peerid/i.test(e.id))e.textContent='0123456789abcdef'.repeat(4);
      for(const e of document.querySelectorAll('.labValueV'))e.textContent=e.id==='researchCapState'?'RAW_AUTO_REARM_BLOCKED':'4294967295';
      for(const e of document.querySelectorAll('.detailValue'))e.textContent=e.id==='deviceConnectionValue'?'Waiting for secure connection':'Disable Visual Speed Control';
      for(const e of document.querySelectorAll('.listrow .value'))e.textContent=e.id==='sysFw'?'T2CAN Universal 3.25.1':e.id==='countryOverrideRowState'?'KOREA / KR7 · STOCK SYNCHRONIZED':'STOCK / INTERNATIONAL';
      for(const e of document.querySelectorAll('.r:not(.rNote) > *, .labValueV, .detailValue, .listrow .value'))e.dataset.layoutExpected=e.textContent;
    });
    const views=await page.evaluate(()=>[...document.querySelectorAll('.panel,main')].filter(e=>e.querySelector('.r, .labValueV, .detailValue, .listrow .value')).map(e=>({id:e.id,page:e.dataset.page,panel:e.classList.contains('panel')})));
    assert.ok(views.length>=10,'audit must cover dashboard panels, not diagnostics alone');
    let checked=0;
    fs.mkdirSync(screens,{recursive:true});
    for(const width of[320,390,430])for(const theme of['light','dark']){
      await page.setViewportSize({width,height:844});
      await page.evaluate(t=>window.t2Theme.setMode(t),theme);
      for(const view of views){
        await page.evaluate(v=>{if(v.panel)openPanel(v.id);else{tuHideAllPanels();showPage(v.page);}},view);
        await page.waitForTimeout(220);
        // Opening a panel refreshes live values; apply the stress fixture after that refresh.
        await page.evaluate(v=>{
          const container=v.panel?document.getElementById(v.id):document.querySelector('main[data-page="'+v.page+'"]');
          for(const e of container.querySelectorAll('[data-layout-expected]'))e.textContent=e.dataset.layoutExpected;
        },view);
        const result=await page.evaluate(v=>{
          const container=v.panel?document.getElementById(v.id):document.querySelector('main[data-page="'+v.page+'"]');
          const failures=[],rows=[...container.querySelectorAll('.r:not(.rNote)')].filter(e=>e.getBoundingClientRect().width>0);
          for(const row of rows){
            if(row.children.length!==2)continue;
            const a=row.firstElementChild,b=row.lastElementChild,r=row.getBoundingClientRect();
            for(const item of[a,b]){
              const range=document.createRange();range.selectNodeContents(item);
              const rects=[...range.getClientRects()].filter(x=>x.width>0&&x.height>0);
              const lines=new Set(rects.map(x=>Math.round(x.top*2)/2));
              const name=item.id||a.textContent;
              if(lines.size!==1)failures.push(name+': '+lines.size+' text lines');
              if(item.textContent!==item.dataset.layoutExpected)failures.push(name+': value text changed');
              const box=item.getBoundingClientRect(),style=getComputedStyle(item);
              if(box.left<r.left-1||box.right>r.right+1)failures.push(name+': outside row');
              if(style.textOverflow==='ellipsis')failures.push(name+': truncated by ellipsis');
              if(item.scrollWidth>item.clientWidth+1){
                item.scrollLeft=0;
                if(range.getBoundingClientRect().left<box.left-1)failures.push(name+': first text inaccessible at scroll start');
                if(!['auto','scroll'].includes(style.overflowX))failures.push(name+': long text inaccessible');
                item.scrollLeft=item.scrollWidth;
                if(range.getBoundingClientRect().right>box.right+1)failures.push(name+': final text inaccessible at scroll end');
                if(item.scrollLeft<item.scrollWidth-item.clientWidth-2)failures.push(name+': cannot scroll to final text');
                item.scrollLeft=0;
              }
            }
            // At least one ordinary short row must retain its compact inline layout.
            if(b.id==='sysFw2'&&Math.abs(a.getBoundingClientRect().top-b.getBoundingClientRect().top)>3)failures.push('short firmware row unnecessarily stacked');
          }
          for(const item of container.querySelectorAll('.labValueV, .detailValue, .listrow .value')){
            const box=item.getBoundingClientRect();if(!box.width)continue;
            const name=item.id||item.className,range=document.createRange();range.selectNodeContents(item);
            const lines=new Set([...range.getClientRects()].filter(x=>x.width&&x.height).map(x=>Math.round(x.top*2)/2));
            if(lines.size!==1)failures.push(name+': '+lines.size+' text lines');
            if(item.textContent!==item.dataset.layoutExpected)failures.push(name+': value text changed');
            if(box.left< -1||box.right>innerWidth+1)failures.push(name+': outside viewport');
            const style=getComputedStyle(item);
            if(style.textOverflow==='ellipsis')failures.push(name+': truncated by ellipsis');
            if(item.scrollWidth>item.clientWidth+1){
              item.scrollLeft=0;
              if(range.getBoundingClientRect().left<box.left-1)failures.push(name+': first text inaccessible at scroll start');
              if(!['auto','scroll'].includes(style.overflowX))failures.push(name+': long text inaccessible');
              item.scrollLeft=item.scrollWidth;
              if(range.getBoundingClientRect().right>box.right+1)failures.push(name+': final text inaccessible at scroll end');
              if(item.scrollLeft<item.scrollWidth-item.clientWidth-2)failures.push(name+': cannot scroll to final text');
              item.scrollLeft=0;
            }
          }
          if(container.scrollWidth>innerWidth+1||document.documentElement.scrollWidth>innerWidth+1)failures.push('page horizontal overflow');
          return{failures,rows:rows.length};
        },view);
        if(view.id==='panelDiag'){
          await page.evaluate(()=>{
            const fixture={sysReinit:'0 / fail 0',sysReason:'NONE · cmd 0',sysHeartbeatCause:'NONE',sysHeartbeatAge:'—',sysHeartbeatCounts:'0 / 0 / 0',sysTwaiState:'RUNNING',sysTwaiLastEvent:'TWAI_BUS_OFF · 123.3 s',sysTwaiBusOffStopped:'1 / 1',sysTwaiBusOffAge:'CURRENT BOOT · age 124.1 s · SAVED',sysTwaiRates:'4.4 / 904.6 / 0.1',sysTwaiRecoveryStart:'1 / 0',sysTwaiRestart:'1 / 0',sysTwaiRxGap:'0 ms / 1.76 s',sysTwaiErrCounters:'0 / 0',sysTwaiDriverErrors:'0 / 32 / 6634'};
            for(const [id,text]of Object.entries(fixture))$(id).textContent=text;
            const panel=$('panelDiag'),card=$('diagCanBCard');panel.scrollTop+=card.getBoundingClientRect().top-140;
          });
          await page.screenshot({path:path.join(screens,`${label}-${width}-${theme}.png`),fullPage:false});
        }
        assert.deepEqual(result.failures,[],`${label} ${view.id||view.page} ${width}/${theme}`);checked+=result.rows;
      }
    }
    assert.deepEqual(errors,[],'no page runtime errors');
    console.log(`${label}: ${views.length} views, ${checked} row observations, 320/390/430px light/dark`);
  }finally{await browser.close();}
}
(async()=>{
  await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');
  if(!process.env.V325_DASHBOARD_SOURCE_ONLY){
    const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
    await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');
  }
})().then(()=>console.log('PASS dashboard long labels/values remain single lines with complete accessible text'),e=>{console.error(e);process.exitCode=1;});
