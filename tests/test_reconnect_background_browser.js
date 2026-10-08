const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {launchBrowser}=require('./browser_test_runtime');
const root=path.resolve(__dirname,'..');
async function check(html,variant){
 const browser=await launchBrowser();
 try{
  const page=await browser.newPage({viewport:{width:390,height:844}});
  await page.addInitScript(()=>{window.setInterval=()=>0});
  await page.route('http://reconnect.test/**',route=>route.fulfill({contentType:route.request().url()==='http://reconnect.test/'?'text/html':'application/json',body:route.request().url()==='http://reconnect.test/'?html:'{}'}));
  await page.goto('http://reconnect.test/');
  for(const width of [320,390,430])for(const theme of ['light','dark'])for(const ap of [3,5]){
   await page.setViewportSize({width,height:844});
   await page.evaluate(({theme,ap})=>{document.body.classList.remove('setup-active');document.getElementById('profileRebootScreen').classList.remove('show');window.t2Theme.setMode(theme);renderApPresentation(ap,true)}, {theme,ap});
   await page.waitForTimeout(650);
   const original=await page.evaluate(()=>getComputedStyle(document.body).backgroundColor);
   await page.evaluate(()=>showReconnectScreen('OTA Complete · Rebooting','Firmware update succeeded. Waiting for T-2CAN Wi-Fi to return.\nThe dashboard will reopen at HOME automatically.'));
   const colors=await page.evaluate(()=>{const overlay=document.getElementById('profileRebootScreen'),box=overlay.getBoundingClientRect(),probe=document.createElement('span');probe.style.backgroundColor=document.querySelector('meta[name="theme-color"]').content;document.body.append(probe);const meta=getComputedStyle(probe).backgroundColor;probe.remove();return {overlay:getComputedStyle(overlay).backgroundColor,body:getComputedStyle(document.body).backgroundColor,root:getComputedStyle(document.documentElement).backgroundColor,meta,width:box.width,scroll:document.documentElement.scrollWidth,title:document.getElementById('profileRebootTitle').getBoundingClientRect().width}});
   assert.equal(colors.body,colors.overlay,`${variant} ${theme} AP${ap}: safe-area body must match reboot overlay`);
   assert.equal(colors.root,colors.overlay);assert.equal(colors.meta,colors.overlay);assert.equal(colors.width,width);assert.equal(colors.scroll,width);assert(colors.title<=width);
   // Polling and phone appearance changes must not restore AP color underneath the overlay.
   await page.evaluate(ap=>renderApPresentation(ap===3?5:3,true),ap);
   assert.equal(await page.evaluate(()=>getComputedStyle(document.body).backgroundColor),colors.overlay);
   if(process.env.DASHBOARD_SCREENSHOT_DIR&&ap===3){fs.mkdirSync(process.env.DASHBOARD_SCREENSHOT_DIR,{recursive:true});await page.screenshot({path:path.join(process.env.DASHBOARD_SCREENSHOT_DIR,`reconnect-${variant}-${width}-${theme}.png`),fullPage:true,animations:'disabled'})}
   await page.evaluate(ap=>{document.getElementById('profileRebootScreen').classList.remove('show');renderApPresentation(ap,true)},ap);
   await page.waitForFunction(expected=>getComputedStyle(document.body).backgroundColor===expected,original);
   assert.equal(await page.evaluate(()=>getComputedStyle(document.body).backgroundColor),original,'hiding overlay restores normal AP theme');
  }
  await page.locator('#otaFile').setInputFiles({name:'test.bin',mimeType:'application/octet-stream',buffer:Buffer.from([1,2,3])});
  for(const failure of ['load','error','abort']){
   await page.evaluate(failure=>{const Native=window.XMLHttpRequest;window.XMLHttpRequest=class{constructor(){this.upload={};this.status=500;this.responseText='{}'}open(){}send(){this['on'+failure]()}};document.getElementById('otaUpload').onclick();window.XMLHttpRequest=Native},failure);
   assert.match(await page.locator('#otaMsg').textContent(),/If OTA started, CAN stays stopped\. Reboot T-2CAN before use\./);
  }
 }finally{await browser.close()}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(process.env.RECONNECT_SOURCE_ONLY!=='1'){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8'),data=h.match(/INDEX_HTML_GZ\[\][^=]*=\s*\{([\s\S]*?)\};/)[1];await check(require('node:zlib').gunzipSync(Buffer.from(data.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded')}console.log('Reconnect background PASS: source/embedded, 320/390/430, light/dark, Autosteer/NOA, polling and dismissal')})().catch(e=>{console.error(e);process.exitCode=1});
