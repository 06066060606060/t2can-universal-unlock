const assert = require('node:assert/strict');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { launchBrowser } = require('./browser_test_runtime');

const root = path.resolve(__dirname, '..');
const preview = process.env.DASHBOARD_HTML_PATH
  || path.join(root, 'dashboard_source.html');

async function main() {
  const browser = await launchBrowser();
  try {
    const page = await browser.newPage({ viewport: { width: 320, height: 844 } });
    // Always exercise the current source instead of a copied historical preview.
    await page.addInitScript({ path: path.join(root, 'tools/v38_preview_mock.js') });
    for (const width of [320, 390, 430]) {
      await page.setViewportSize({ width, height: 844 });
      const errors = [];
      page.on('pageerror', error => errors.push(error.message));
      await page.goto(pathToFileURL(preview).href, { waitUntil: 'domcontentloaded' });
      await page.locator('.navbtn[data-page="settings"]').click();
      await page.locator('#nagSettingsRow').click();
      await page.waitForFunction(() => document.querySelector('#modeH')?.classList.contains('primary'));

      const layout = await page.evaluate(() => {
        const names = ['panelNag', 'modeA', 'modeB', 'modeC', 'modeH', 'nagIgnoreApStateWrap', 'nagModeSummary', 'nagModeSummaryText'];
        const boxes = Object.fromEntries(names.map(id => {
          const el = document.getElementById(id);
          const r = el.getBoundingClientRect();
          return [id, { x: r.x, right: r.right, width: r.width, height: r.height }];
        }));
        return { boxes, font: getComputedStyle(document.querySelector('#modeH .nagModeHLabel')).fontSize,
          scrollWidth: document.getElementById('panelNag').scrollWidth };
      });
      const b = layout.boxes;
      assert.equal(layout.scrollWidth, width, `panel overflows at ${width}px`);
      assert(b.modeA.right < b.modeB.x && b.modeB.right < b.modeC.x);
      assert(b.nagIgnoreApStateWrap.right <= width);
      assert(b.nagModeSummaryText.right <= b.nagModeSummary.right);
      assert.equal(layout.font, '14px');
      assert.deepEqual(errors, []);
      if (width === 390 && process.env.DASHBOARD_SCREENSHOT_DIR) {
        await page.screenshot({ path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, 'nag.png'), fullPage: true });
      }
    }
    await page.close();
  } finally {
    await browser.close();
  }
}

main().then(() => process.stdout.write('NAG layout: PASS\n'), error => {
  process.stderr.write(`${error.stack || error}\n`);
  process.exitCode = 1;
});
