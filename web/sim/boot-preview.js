// SPDX-License-Identifier: GPL-3.0-only
// BLAST IGNITION's scene preview for the page (?boot=1): presentation only. `load` makes a fresh, never-booted Fm1Sim
// (its sim_boot_preview export draws the shared C renderer into its own framebuffer); `emit` hands a framebuffer to the
// page's existing frame state. It sends no LED or audio command to a running simulator, and it is a scene preview, not
// a measurement of the device's boot timing.
export function createBootPreview(load, emit) {
  let generation = 0, sim = null, start = null, held = false, last = -1, reduced = false;
  return {
    async begin(reduce = false) {
      const own = ++generation;
      sim = null; start = null; held = false; last = -1; reduced = reduce;
      const loaded = await load();
      if (own !== generation) return;
      if (!loaded.bootPreview(reduced ? 399 : 0)) return;     // not in this build: the normal reel stays
      sim = loaded;
      emit(sim.fb());
      last = reduced ? 399 : 0;
      held = reduced;
    },
    cancel() {
      generation++;
      sim = null; start = null; held = false; last = -1;
    },
    tick(now) {                                    // true while the preview owns the screen
      if (!sim) return false;
      if (held) return true;
      if (start === null) start = now;
      const elapsed = Math.min(399, Math.max(0, Math.floor(now - start)));
      if (elapsed !== last) {
        sim.bootPreview(elapsed);
        emit(sim.fb());
        last = elapsed;
      }
      if (now - start >= 400) held = true;
      return true;
    },
    reduce() {
      reduced = true;
      if (sim) {
        sim.bootPreview(399); emit(sim.fb()); last = 399; held = true;
      }
    },
  };
}
