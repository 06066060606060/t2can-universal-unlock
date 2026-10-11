const { chromium } = require('playwright');
const fs=require('node:fs'),path=require('node:path'),zlib=require('node:zlib');

const DARWIN_SANDBOX_ARGS = [
  '--single-process',
  '--no-zygote',
  '--disable-crash-reporter',
  '--disable-crashpad',
  '--disable-features=NetworkService,NetworkServiceInProcess',
];

function launchBrowser() {
  const options = { headless: true };
  if (process.platform === 'darwin') options.args = DARWIN_SANDBOX_ARGS;
  if (process.env.T2CAN_BROWSER_EXECUTABLE) {
    options.executablePath = process.env.T2CAN_BROWSER_EXECUTABLE;
  }
  return chromium.launch(options);
}

// Shared artifact fixtures: every suite executes the editable source and the
// exact gzip bytes served by the firmware, without importing historical tests.
function dashboardArtifacts(root, sourceOnly=false) {
  const artifacts=[{label:'source',html:fs.readFileSync(path.join(root,'dashboard_source.html'),'utf8')}];
  if(!sourceOnly){
    const header=fs.readFileSync(path.join(root,'index_html.h'),'utf8');
    const data=header.match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
    artifacts.push({label:'embedded',html:zlib.gunzipSync(Buffer.from(data.match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)))).toString()});
  }
  return artifacts;
}
function dashboardFonts(root) {
  const header=fs.readFileSync(path.join(root,'lab_fonts.h'),'utf8');
  const bytes=symbol=>Buffer.from(header.match(new RegExp(`${symbol}\\[\\][^{]*\\{([\\s\\S]*?)\\};`))[1].match(/0x[\da-f]+/gi).map(x=>parseInt(x,16)));
  return {'/fonts/geist-lab.woff2':bytes('LAB_GEIST_FONT'),'/fonts/geist-mono-lab.woff2':bytes('LAB_GEIST_MONO_FONT')};
}
module.exports = { launchBrowser, dashboardArtifacts, dashboardFonts };
