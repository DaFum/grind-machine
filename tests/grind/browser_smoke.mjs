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
  let path = new URL(request.url, 'http://localhost').pathname;
  if (path.endsWith('/')) path += 'index.html';
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
  const native = (process.env.GRIND_NATIVE ?? '0') === '1';
  const buildLabel = native ? 'GRIND build' : 'Base drum-machine build';
  assert.equal(await page.title(), 'GRIND MACHINE | FM-1 Web Simulator');
  await page.waitForFunction(label => document.querySelector('#sim-build').textContent === label, buildLabel);
  assert.equal(await page.getByRole('heading', { name: 'Generate a GRIND pattern' }).isVisible(), native);
  const installer = await page.request.get(`${base}/webapp/installer/index.html`);
  assert.ok(installer.ok(), 'the preview includes an installer');
  const installerHtml = await installer.text();
  const editorHref = installerHtml.match(/<a href="([^"]+)">Open the USB web editor<\/a>/)?.[1];
  let editorUrl = `${base}/editor-source/editor.html`;
  if (installerHtml.includes('preview-has-no-package.fwsc')) {
    assert.equal(editorHref, 'https://dafum.github.io/grind-machine/webapp/editor/');
  } else {
    assert.equal(editorHref, '../editor/');
    editorUrl = new URL(editorHref, installer.url()).href;
    assert.ok((await page.request.get(editorUrl)).ok(), 'the packaged preview serves its bundled editor');
  }
  const power = page.getByRole('switch', { name: 'Power' });
  assert.equal(await page.locator('#device').getByRole('slider').count(), 8);
  assert.ok(await page.getByRole('button', { name: 'Reset to demos' }).isEnabled());
  await power.click();
  await page.locator('#power-note').waitFor({ state: 'hidden' });
  assert.equal(await power.getAttribute('aria-checked'), 'true');
  assert.doesNotMatch(await page.locator('#note').textContent(), /firmware failed/i);

  // BLAST IGNITION's scene preview (?boot=1): the expected mode comes from GRIND_NATIVE (default 0)
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
  const assertReadOnly = async previewPage => {
    assert.ok(await previewPage.getByRole('switch', { name: 'Power' }).isDisabled());
    assert.ok(await previewPage.getByRole('button', { name: 'Reset to demos' }).isDisabled());
    const panel = previewPage.locator('#device');
    assert.equal(await panel.locator('.enc:visible').count(), 8, 'the preview renders all eight knobs');
    assert.equal(await panel.getByRole('slider').count(), 0, 'the preview exposes no interactive sliders');
    const focused = await panel.locator('.enc').evaluateAll(knobs => knobs.some(knob => {
      knob.focus();
      return knob === document.activeElement;
    }));
    assert.equal(focused, false, 'the preview knobs cannot receive focus');
  };
  if (!native) {                                          // the unsupported build: ?boot=1 leaves the normal reel on screen
    const off = await browser.newPage({ reducedMotion: 'reduce' });
    off.setDefaultTimeout(15_000);
    off.on('pageerror', error => errors.push(error.message));
    await off.goto(`${base}/?boot=1`);
    await off.waitForFunction(() => document.documentElement.dataset.bootPreview === 'settled');   // the preview's load is over
    await assertReadOnly(off);
    const owners = new Set(), frames = new Set();         // who painted the LCD, and the reel's images
    for (let i = 0; i < 40 && !(owners.has('reel') && frames.size >= 2); i++) {
      const [owner, hash] = await off.evaluate(() => {
        const d = document.querySelector('canvas.lcd').getContext('2d').getImageData(0, 0, 240, 240).data;
        let h = 2166136261;
        for (let k = 0; k < d.length; k += 4) h = Math.imul(h ^ (d[k] | d[k + 1] << 8 | d[k + 2] << 16), 16777619) >>> 0;
        return [document.documentElement.dataset.lcdOwner ?? '', h];
      });
      owners.add(owner);
      if (owner === 'reel') frames.add(hash);              // only frames the reel painted count as moving
      await off.waitForTimeout(200);
    }
    const first = frames.size;                            // the observation continues: still the reel, still moving
    for (let i = 0; i < 20 && frames.size < first + 2; i++) {
      const [owner, hash] = await off.evaluate(() => {
        const d = document.querySelector('canvas.lcd').getContext('2d').getImageData(0, 0, 240, 240).data;
        let h = 2166136261;
        for (let k = 0; k < d.length; k += 4) h = Math.imul(h ^ (d[k] | d[k + 1] << 8 | d[k + 2] << 16), 16777619) >>> 0;
        return [document.documentElement.dataset.lcdOwner ?? '', h];
      });
      owners.add(owner);
      if (owner === 'reel') frames.add(hash);
      await off.waitForTimeout(200);
    }
    assert.ok(frames.size >= first + 1, 'the demo reel must still be changing the LCD after its first images');
    assert.ok(!owners.has('boot'), '?boot=1 on a build without the preview must never paint a preview frame');
    assert.ok(owners.has('reel'), '?boot=1 on a build without the preview must leave the demo reel on screen');
    assert.ok(frames.size >= 2, 'the demo reel must keep playing (the LCD image changes), not freeze after one frame');
    await off.close();
  }
  if (native) {
    // No reel in the popup, so Escape should restore a black screen.
    await page.context().route('**/reel.bin.gz*', route => route.fulfill({ status: 404 }));
    const originalUrl = page.url();
    const [boot] = await Promise.all([
      page.waitForEvent('popup'),
      page.getByRole('link', { name: /Preview BLAST IGNITION/ }).click(),
    ]);
    boot.setDefaultTimeout(15_000);
    boot.on('pageerror', error => errors.push(error.message));
    await boot.waitForURL(`${base}/?boot=1#play`);
    assert.equal(page.url(), originalUrl);
    assert.equal(await power.getAttribute('aria-checked'), 'true', 'the original simulator stays powered on');
    await boot.waitForFunction(() => document.documentElement.dataset.bootPreview === 'settled');
    await assertReadOnly(boot);
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
    await assertReadOnly(boot);
    await boot.close();
    const plain = await browser.newPage({ reducedMotion: 'reduce' });      // the ordinary URL is unchanged
    await plain.goto(base);
    await plain.waitForFunction(() => document.documentElement.dataset.lcdOwner === 'reel');   // the reel (not the opaque black start) has painted
    assert.notDeepEqual((await lcdPixel(plain, 120, 124)).slice(0, 3), [188, 37, 45]);
    await plain.close();
  }
  await page.goto(`${editorUrl}?mock=auto`);
  const projects = page.getByRole('tab', { name: 'Projects' });
  await projects.click();
  assert.equal(await projects.getAttribute('aria-selected'), 'true');
  assert.deepEqual(errors, []);
  console.log('PASS: Chromium simulator power-on and mock editor navigation');
} finally {
  await browser?.close();
  server.close();
}
