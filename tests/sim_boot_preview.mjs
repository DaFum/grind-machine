// SPDX-License-Identifier: GPL-3.0-only
// The ?boot=1 preview controller (web/sim/boot-preview.js) with a fake clock and a deferred loader.
import assert from 'node:assert/strict';
import { createBootPreview } from '../web/sim/boot-preview.js';

function fakeSim(supported = true) {
  const calls = [];
  return { calls, bootPreview(ms) { calls.push(ms); return supported; }, fb() { return calls.length; } };
}
function deferred() {
  let resolve;
  const promise = new Promise((r) => { resolve = r; });
  return { promise, resolve };
}
const harness = (sim) => {
  const emitted = [];
  const d = deferred();
  const c = createBootPreview(() => d.promise, (f) => emitted.push(f));
  return { c, emitted, d, sim };
};

// 0 -> 60 -> 400 holds READY (399)
{
  const sim = fakeSim();
  const { c, emitted, d } = harness(sim);
  const p = c.begin(false);
  d.resolve(sim);
  await p;
  assert.deepEqual(sim.calls, [0]);
  assert.equal(c.tick(1000), true);                 // start
  assert.equal(c.tick(1060), true);
  assert.equal(sim.calls.at(-1), 60);
  assert.equal(c.tick(1400), true);
  assert.equal(sim.calls.at(-1), 399);              // the hold
  const n = sim.calls.length;
  assert.equal(c.tick(2000), true);                 // held: nothing more is drawn
  assert.equal(sim.calls.length, n);
  assert.ok(emitted.length >= 3);
}
// reduced motion: READY once
{
  const sim = fakeSim();
  const { c, emitted, d } = harness(sim);
  const p = c.begin(true);
  d.resolve(sim);
  await p;
  assert.deepEqual(sim.calls, [399]);
  assert.equal(emitted.length, 1);
  c.tick(5000); c.tick(6000);
  assert.deepEqual(sim.calls, [399]);
}
// cancelled while the load is pending: no late frame
{
  const sim = fakeSim();
  const { c, emitted, d } = harness(sim);
  const p = c.begin(false);
  c.cancel();
  d.resolve(sim);
  await p;
  assert.equal(emitted.length, 0);
  assert.equal(sim.calls.length, 0);
  assert.equal(c.tick(100), false);
}
// unsupported export: false, the reel stays available
{
  const sim = fakeSim(false);
  const { c, emitted, d } = harness(sim);
  const p = c.begin(false);
  d.resolve(sim);
  await p;
  assert.equal(emitted.length, 0);
  assert.equal(c.tick(100), false);
}
// a cancelled controller never emits again
{
  const sim = fakeSim();
  const { c, emitted, d } = harness(sim);
  const p = c.begin(false);
  d.resolve(sim);
  await p;
  c.tick(0);
  c.cancel();
  const n = emitted.length;
  assert.equal(c.tick(100), false);
  c.reduce();
  assert.equal(emitted.length, n);
}
// reducing during playback ends on READY
{
  const sim = fakeSim();
  const { c, d } = harness(sim);
  const p = c.begin(false);
  d.resolve(sim);
  await p;
  c.tick(0); c.tick(100);
  c.reduce();
  assert.equal(sim.calls.at(-1), 399);
  const n = sim.calls.length;
  c.tick(300);
  assert.equal(sim.calls.length, n);
}
// reducing while the load is pending: the current flag is read after the await, so it ends on READY
{
  const sim = fakeSim();
  const { c, d } = harness(sim);
  const p = c.begin(false);
  c.reduce();                                       // nothing loaded yet
  d.resolve(sim);
  await p;
  assert.equal(sim.calls[0], 399);
  const n = sim.calls.length;
  c.tick(10); c.tick(500);
  assert.equal(sim.calls.length, n);
}
// the default URL creates no preview: an unused controller never owns the screen
{
  const c = createBootPreview(() => { throw new Error('must not load'); }, () => { throw new Error('must not emit'); });
  assert.equal(c.tick(0), false);
}
console.log('sim_boot_preview: all passed');
