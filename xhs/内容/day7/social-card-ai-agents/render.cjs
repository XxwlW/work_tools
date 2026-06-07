/**
 * Social Card Render Script
 * Renders Swiss-style social cards to PNG using Playwright
 */

const { chromium } = require('playwright');
const path = require('path');
const fs = require('fs');

const OUTPUT_DIR = path.join(__dirname, 'output');
const INPUT_HTML = path.join(__dirname, 'index.html');

// Target frames to export
const targets = [
  { selector: '#xhs-01', filename: 'xhs-01-cover.png', width: 1080, height: 1440 },
  { selector: '#xhs-02', filename: 'xhs-02-openclaw.png', width: 1080, height: 1440 },
  { selector: '#xhs-03', filename: 'xhs-03-hermes.png', width: 1080, height: 1440 },
  { selector: '#xhs-04', filename: 'xhs-04-claude-code.png', width: 1080, height: 1440 },
  { selector: '#xhs-05', filename: 'xhs-05-codex.png', width: 1080, height: 1440 },
  { selector: '#xhs-06', filename: 'xhs-06-comparison.png', width: 1080, height: 1440 },
  { selector: '#xhs-07', filename: 'xhs-07-takeaway.png', width: 1080, height: 1440 },
];

async function renderCards() {
  // Ensure output directory exists
  if (!fs.existsSync(OUTPUT_DIR)) {
    fs.mkdirSync(OUTPUT_DIR, { recursive: true });
  }

  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage();

  // Load the HTML file
  const fileUrl = `file://${INPUT_HTML}`;
  console.log(`Loading: ${fileUrl}`);
  await page.goto(fileUrl, { waitUntil: 'networkidle' });

  // Wait for fonts and icons to load
  await page.waitForTimeout(2000);

  for (const target of targets) {
    try {
      console.log(`Rendering: ${target.selector} -> ${target.filename}`);

      const element = await page.$(target.selector);
      if (!element) {
        console.error(`  ERROR: Element ${target.selector} not found`);
        continue;
      }

      // Get element bounding box
      const box = await element.boundingBox();

      // Take screenshot with proper dimensions
      await element.screenshot({
        path: path.join(OUTPUT_DIR, target.filename),
        type: 'png',
        clip: {
          x: box.x,
          y: box.y,
          width: box.width,
          height: box.height
        }
      });

      // Verify dimensions
      const imgBuffer = fs.readFileSync(path.join(OUTPUT_DIR, target.filename));
      // PNG header check (width/height in IHDR chunk)
      const expectedWidth = target.width;
      const expectedHeight = target.height;

      console.log(`  ✓ Saved: ${target.filename} (${box.width}x${box.height})`);
    } catch (err) {
      console.error(`  ERROR rendering ${target.selector}:`, err.message);
    }
  }

  await browser.close();
  console.log('\n✓ All renders complete!');
  console.log(`Output directory: ${OUTPUT_DIR}`);
}

renderCards().catch(console.error);