const { chromium } = require('playwright');

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

module.exports = { launchBrowser };
