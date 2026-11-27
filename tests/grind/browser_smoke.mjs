// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { extname, resolve, sep } from 'node:path';

const { chromium } = await import('../../build/grind-browser/node_modules/playwright-core/index.mjs');
const preview = resolve('build/sim/preview');
const web = resolve('web');
const types = {
  '.css': 'text/css', '.html': 'text/html', '.js': 'text/javascript',
  '.json': 'application/json', '.mjs': 'text/javascript',
  '.wasm': 'application/wasm', '.woff': 'font/woff',
};
const server = createServer(async (request, response) => {
  const path = new URL(request.url, 'http://localhost').pathname;
  const fromWeb = path.startsWith('/editor-source/');
  const root = fromWeb ? web : preview;
  const file = resolve(root, fromWeb ? path.slice('/editor-source/'.length) : path.slice(1) || 'index.html');
  if (!file.startsWith(root + sep)) {
    response.writeHead(404).end();
    return;
  }
  let body;
  try {
    body = await readFile(file);
  } catch {
    response.writeHead(404).end();
    return;
  }
  response.writeHead(200, { 'Content-Type': types[extname(file)] ?? 'application/octet-stream' }).end(body);
});

server.listen(0, '127.0.0.1');
await once(server, 'listening');
const base = `http://127.0.0.1:${server.address().port}`;
let browser;
try {
  browser = await chromium.launch({ executablePath: process.env.GRIND_CHROMIUM });
  const page = await browser.newPage({ reducedMotion: 'reduce' });
  page.setDefaultTimeout(15_000);
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));

  const [wasm] = await Promise.all([
    page.waitForResponse(response => new URL(response.url()).pathname === '/fm1sim.wasm'),
    page.goto(base),
  ]);
  assert.equal(wasm.status(), 200);
  const power = page.getByRole('switch', { name: 'Power' });
  await power.click();
  await page.locator('#power-note').waitFor({ state: 'hidden' });
  assert.equal(await power.getAttribute('aria-checked'), 'true');
  assert.doesNotMatch(await page.locator('#note').textContent(), /firmware failed/i);

  // BLAST IGNITION's scene preview (?boot=1): the expected mode comes from GRIND_NATIVE (default 0)
  const native = (process.env.GRIND_NATIVE ?? '0') === '1';
  const supported = await page.evaluate(async () => {
    const { Fm1Sim } = await import('/engine.js');
    const sim = await Fm1Sim.create(await (await fetch('/fm1sim.wasm')).arrayBuffer());
    return typeof sim.x.sim_boot_preview === 'function' ? sim.bootPreview(399) : null;
  });
  assert.notEqual(supported, null, 'the served WASM must export sim_boot_preview in both modes');
  assert.equal(supported, native, 'sim_boot_preview supported only when GRIND_NATIVE=1');
  const lcdPixel = (p, x, y) => p.evaluate(([x0, y0]) => {
    const c = document.querySelector('canvas.lcd').getContext('2d').getImageData(x0, y0, 1, 1).data;
    return [c[0], c[1], c[2], c[3]];
  }, [x, y]);
  if (!native) {                                          // the unsupported build: ?boot=1 leaves the normal reel on screen
    const off = await browser.newPage({ reducedMotion: 'reduce' });
    off.setDefaultTimeout(15_000);
    off.on('pageerror', error => errors.push(error.message));
    await off.goto(`${base}/?boot=1`);
    await off.waitForFunction(() => {                     // the reel paints something (not the opaque black of no screen)
      const d = document.querySelector('canvas.lcd').getContext('2d').getImageData(0, 0, 240, 240).data;
      for (let i = 0; i < d.length; i += 4) if (d[i] || d[i + 1] || d[i + 2]) return true;
      return false;
    });
    await off.close();
  }
  if (native) {
    const boot = await browser.newPage({ reducedMotion: 'reduce' });
    boot.setDefaultTimeout(15_000);
    boot.on('pageerror', error => errors.push(error.message));
    await boot.route('**/reel.bin.gz*', route => route.fulfill({ status: 404 }));   // no reel: black is the baseline
    await boot.goto(`${base}/?boot=1`);
    await boot.waitForFunction(() => {                    // reduced motion: READY directly (the red rule at y 124)
      const c = document.querySelector('canvas.lcd').getContext('2d').getImageData(120, 124, 1, 1).data;
      return c[0] > 150 && c[1] < 80 && c[3] === 255;
    });
    await boot.keyboard.press('Escape');                   // handed back: no reel here, so opaque black, no lit LED
    await boot.waitForFunction(() => {
      const c = document.querySelector('canvas.lcd').getContext('2d').getImageData(120, 124, 1, 1).data;
      return c[0] < 40 && c[3] === 255;
    });
    assert.equal(await boot.locator('.lit').count(), 0, 'no LED stays lit after the hand-back');
    await boot.close();
    const plain = await browser.newPage({ reducedMotion: 'reduce' });      // the ordinary URL is unchanged
    await plain.goto(base);
    assert.notDeepEqual((await lcdPixel(plain, 120, 124)).slice(0, 3), [188, 37, 45]);
    await plain.close();
  }
  await page.goto(`${base}/editor-source/editor.html?mock=auto`);
  const projects = page.getByRole('tab', { name: 'Projects' });
  await projects.click();
  assert.equal(await projects.getAttribute('aria-selected'), 'true');
  assert.deepEqual(errors, []);
  console.log('PASS: Chromium simulator power-on and mock editor navigation');
} finally {
  await browser?.close();
  server.close();
}
