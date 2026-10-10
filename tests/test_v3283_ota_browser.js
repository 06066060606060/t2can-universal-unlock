// Actual dashboard handlers must reject malformed success, show controller
// failures and retain one active upload; long errors fit existing mobile styles.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');
const {launchBrowser}=require('./browser_test_runtime'),root=path.resolve(__dirname,'..');
async function check(html,label){
 const browser=await launchBrowser();try{
 let attempt=1,failBaseline=false;
 const page=await browser.newPage({viewport:{width:390,height:844}});
 await page.addInitScript(()=>{window.setInterval=()=>0;});
 await page.route('http://ota.test/**',route=>{const p=new URL(route.request().url()).pathname;if(p.startsWith('/fonts/'))return route.fulfill({contentType:'font/woff2',body:fs.readFileSync(path.join(root,p))});if(p==='/api/ota/status'&&failBaseline)return route.fulfill({status:503,body:'{}'});return route.fulfill({contentType:p==='/'?'text/html':'application/json',body:p==='/'?html:p==='/api/ota/status'?JSON.stringify({attempt,inProgress:false,ok:false,error:'Aborted',stage:'abort',updateError:12,receivedBytes:4099}):p==='/api/profile/status'?JSON.stringify({setupMode:false,profile:4,topology:2,turn:1,nagSupported:true,nagTorqueSupported:true,nagTsl9Supported:true}):'{}'});});
 await page.goto('http://ota.test/');await page.waitForFunction(()=>universalBootReady);
 await page.locator('#otaFile').setInputFiles({name:'fw.bin',mimeType:'application/octet-stream',buffer:Buffer.from([1,2,3])});
 await page.evaluate(()=>{waitForT2CanReconnect=()=>{};window.XMLHttpRequest=class {constructor(){this.upload={};window.otaTestXHR=this;}open(){}send(){window.otaTestSends=(window.otaTestSends||0)+1;window.otaTestSent=true;}};});
 for(const [status,body,want]of [[200,'garbage',/invalid|failed/i],[200,'{"ok":"true"}',/invalid|failed/i],[200,'{"ok":false,"error":"Not Enough Space","stage":"begin","updateError":4,"receivedBytes":123}',/Not Enough Space.*begin.*4.*123/],[500,'{"ok":false,"error":"Flash Write Failed","stage":"write","updateError":1,"receivedBytes":456}',/Flash Write Failed.*write.*1.*456/]]){
  await page.evaluate(async ({status,body})=>{await document.getElementById('otaUpload').onclick();otaTestXHR.status=status;otaTestXHR.responseText=body;otaTestXHR.onload();},{status,body});
  assert.equal(await page.evaluate(()=>otaUiPhase),'idle',label+' malformed/failed response must not reboot');
  assert.match(await page.locator('#otaMsg').textContent(),want,label+' diagnostic failure');
 }
 await page.evaluate(async()=>{await document.getElementById('otaUpload').onclick();});attempt++;
 await page.evaluate(async()=>{await otaTestXHR.onerror();});
 await page.waitForFunction(()=>document.getElementById('otaMsg').textContent.includes('4099'));
 assert.match(await page.locator('#otaMsg').textContent(),/Aborted.*abort.*12.*4099/);
 assert.equal(await page.evaluate(()=>otaUploading),false);
 // The previous attempt's stored error must not explain a POST that never started.
 for(const missingBaseline of [false,true]){
  failBaseline=missingBaseline;
  await page.evaluate(async()=>{await document.getElementById('otaUpload').onclick();await otaTestXHR.onerror();});
  const message=await page.locator('#otaMsg').textContent();
  assert.match(message,/Upload error/);assert(!message.includes('4099')&&!message.includes('Aborted'),label+' stale/missing baseline must keep generic failure');
 }
 failBaseline=false;
 // Render the longest production error with detail and the unchanged warning.
 await page.evaluate(async()=>{await document.getElementById('otaUpload').onclick();otaTestXHR.status=200;otaTestXHR.responseText=JSON.stringify({ok:false,error:'Upload interrupted (connection closed or receive timeout)',stage:'abort',updateError:12,receivedBytes:3145728});otaTestXHR.onload();document.body.classList.remove('setup-active');document.getElementById('profileRebootScreen').classList.remove('show');showPage('settings');openPanel('panelOta');});
 const screens='/private/tmp/t2can-v3283-ota-ui';fs.mkdirSync(screens,{recursive:true});
 for(const width of [320,390,430])for(const theme of ['light','dark']){
  await page.setViewportSize({width,height:844});
  await page.evaluate(theme=>{window.t2Theme.setMode(theme);openPanel('panelOta');},theme);await page.waitForTimeout(350);await page.evaluate(()=>document.fonts.ready);
  const layout=await page.evaluate(()=>{const msg=document.getElementById('otaMsg'),box=msg.getBoundingClientRect(),style=getComputedStyle(msg),panel=document.getElementById('panelOta');return {width:innerWidth,documentWidth:document.documentElement.scrollWidth,panelWidth:panel.clientWidth,panelScroll:panel.scrollWidth,msgWidth:msg.clientWidth,msgScroll:msg.scrollWidth,height:box.height,lineHeight:parseFloat(style.lineHeight),left:box.left,right:box.right,visible:box.width>0&&box.height>0&&style.visibility!=='hidden',message:msg.textContent};});
  assert(layout.visible&&layout.height>layout.lineHeight*2,label+' long error must visibly wrap '+JSON.stringify(layout));
  assert(layout.left>=0&&layout.right<=width,label+' error remains inside viewport');
  assert(layout.msgScroll<=layout.msgWidth&&layout.panelScroll<=layout.panelWidth&&layout.documentWidth<=width,label+' no horizontal overflow at '+width+' '+theme);
  assert.match(layout.message,/3145728 bytes.*CAN stays stopped/);
  await page.screenshot({path:path.join(screens,label+'-'+width+'-'+theme+'.png')});
 }
 await page.evaluate(async()=>{window.otaTestSends=0;const first=document.getElementById('otaUpload').onclick();document.getElementById('otaUpload').onclick();await first;});
 assert.equal(await page.evaluate(()=>otaTestSends),1,label+' duplicate active click must not start another upload');
 await page.evaluate(()=>{otaTestXHR.status=200;otaTestXHR.responseText='{"ok":true}';otaTestXHR.onload();});
 assert.equal(await page.evaluate(()=>otaUiPhase),'rebooting');assert.equal(await page.evaluate(()=>otaUploading),true);
 }finally{await browser.close();}
}
(async()=>{await check(fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8'),'source');if(!process.env.OTA_SOURCE_ONLY){const h=fs.readFileSync(path.join(root,'index_html.h'),'utf8').match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];await check(zlib.gunzipSync(Buffer.from(h.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString(),'embedded');}})().then(()=>console.log('OTA source/embedded malformed response, detailed errors, reconnect diagnosis, duplicate upload, success and 12 long-error mobile/theme renders PASS'),e=>{console.error(e);process.exitCode=1;});
