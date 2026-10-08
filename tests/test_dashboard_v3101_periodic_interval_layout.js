const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { launchBrowser } = require('./browser_test_runtime');

const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(
  process.env.DASHBOARD_HTML_PATH || path.join(root, 'dashboard_source.html'),
  'utf8',
);
async function main(html) {
  const staticHtml = html.replace(/<script\b[^>]*>[\s\S]*?<\/script>/gi, '');
  const browser = await launchBrowser();
  try {
    const page = await browser.newPage();
    for (const theme of ['light', 'dark']) for (const width of [320, 390, 430]) {
      await page.setViewportSize({ width, height: 844 });
      await page.setContent(staticHtml, { waitUntil: 'domcontentloaded' });
      await page.evaluate(theme => document.documentElement.dataset.theme = theme, theme);

      const layout = await page.evaluate(() => {
        document.querySelector('.setupShell')?.remove();
        document.querySelector('.rebootScreen')?.remove();
        const panel = document.getElementById('panelNag');
        const wrap = document.getElementById('tsl9RightPeriodicWrap');
        panel.classList.add('show');
        wrap.classList.remove('tuUiHidden');
        document.getElementById('torqueRightScrollWrap').style.display = 'none';

        const input = document.getElementById('tsl9RightPeriodicInterval');
        const field = input.closest('.field');
        const card = input.closest('.card');
        const inputRect = input.getBoundingClientRect();
        const fieldRect = field?.getBoundingClientRect();
        const cardRect = card.getBoundingClientRect();
        const style = field ? getComputedStyle(field) : null;

        return {
          hasFieldCard: Boolean(field),
          inputWidth: inputRect.width,
          inputRight: inputRect.right,
          fieldRight: fieldRect?.right || 0,
          textAlign: getComputedStyle(input).textAlign,
          fieldWidth: fieldRect?.width || 0,
          cardWidth: cardRect.width,
          borderRadius: style?.borderRadius || '',
          panelScrollWidth: panel.scrollWidth,
        };
      });

      assert.equal(layout.hasFieldCard, true, `Periodic Interval must use a field card at ${width}px`);
      assert(layout.fieldWidth >= layout.cardWidth * 0.85, `field card is not full width at ${width}px`);
      assert.equal(layout.textAlign, 'right');
      assert(layout.inputWidth >= 60, 'value must remain easy to edit');
      assert(layout.fieldRight - layout.inputRight <= 20, 'value must align with the right inset');
      assert.equal(layout.borderRadius, '16px');
      assert.equal(layout.panelScrollWidth, width, `panel overflows at ${width}px`);
      if (width === 390 && process.env.DASHBOARD_SCREENSHOT_DIR) {
        fs.mkdirSync(process.env.DASHBOARD_SCREENSHOT_DIR, { recursive: true });
        await page.evaluate(() => document.getAnimations().forEach(animation => animation.finish()));
        await page.screenshot({
          path: path.join(process.env.DASHBOARD_SCREENSHOT_DIR, `periodic-interval-${theme}.png`),
          fullPage: true,
        });
      }
    }
    await page.close();
  } finally {
    await browser.close();
  }
}

(async () => {
  await main(html);
  const header = fs.readFileSync(path.join(root, 'index_html.h'), 'utf8');
  const data = header.match(/INDEX_HTML_GZ\[\][^{]*\{([\s\S]*?)\};/)[1];
  const embedded = require('node:zlib').gunzipSync(Buffer.from(data.match(/0x[\da-f]+/gi).map(x => parseInt(x, 16)))).toString();
  await main(embedded);
})().then(() => process.stdout.write('Periodic Interval source/embedded layout: PASS\n'), error => {
  process.stderr.write(`${error.stack || error}\n`);
  process.exitCode = 1;
});
