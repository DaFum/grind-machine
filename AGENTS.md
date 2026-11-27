# GRIND MACHINE: Agent Instructions

## Critical Commands
- Generated headers must exist first: `python tools/build.py --gen-only` (tests include `build/gen`).
- GRIND tests: `sh tests/run_grind_tests.sh <suite>` (suites: rng grid rules codec locks morph properties golden controller adapter cue ui persist menu editor hit_variation vary master_fx boot_anim boot_draw boot_ui boot_recovery boot_restore; no argument or `all` runs them; a new suite must also be listed under `all`). Set `CC` if `cc` is missing, e.g. `CC="python -m ziglang cc -Wno-date-time"`.
- Upstream firmware tests (they write screenshots and crash if the folders are missing: create the `build/ui_shots/*` directories named on the `mkdir -p` line of `tests/run_drum_tests.sh` first): for each of `tests/boot_test.c`, `tests/ui_test.c`, `tests/editor_pattern_test.c`, compile once with `-DGRIND_NATIVE=0` and once with `-DGRIND_NATIVE=1` using `$CC -std=gnu11 -O1 -Wno-unused-function -Wno-int-to-pointer-cast -Wno-macro-redefined -fno-sanitize=shift-base -UNDEBUG -DGRIND_NATIVE=<0|1> -Ibuild/gen -Ifirmware/src -Itests -Ifirmware/src/grind tests/<name>.c -o <out> -lm`, run each binary, and require exit code 0 for all six runs.
- `golden` fails on a Windows checkout with CRLF line endings. That is not a regression.

## Architecture Constraints
- The firmware is a textual-include unit: `firmware/src/felucca.c` includes the other `.c` files in order. A file compiled alone shows false "undeclared"/"unused" errors. Declare shared state in `firmware/src/core.h` (static, under `GRIND_NATIVE`).
- Everything GRIND is behind `GRIND_NATIVE` (default 0). With it off, output must stay bit-identical.
- Anything that writes a live step or timing parameter must call `gm_touch()` (`gm_touch_isr()` inside an IRQ-off region), or add `("<file>.c", "<function>"): "<why it is safe>"` to the `ALLOWED` dict in `tests/grind/test_edit_sites.py`. That audit fails otherwise.
- Frozen against upstream, compared by `tools/check_untouched.py`: `firmware/hal/`, `firmware/loader/`, `firmware/src/ota.c`, `usb.c`, `usb_app.c`, `firmware/crt0.S`, `firmware/app.ld` (byte for byte); `storage.c` except the `ST_MAGIC` line; `main.c` except `felucca_init()` and the boot title; the boot-guard tail of `core.h`. Do not edit them.
- A new `ui.c`/`ui_draw.c`/`ui_input.c` hook needs its definition in both host harnesses, `tests/ui_host.h` and `tests/sim_host.h`, or the simulator build fails to link. Anything GRIND-only that the OTA-disabled build still needs goes outside the `FELUCCA_OTA` block in `felucca.c`.
- The portable engine in `firmware/src/grind/` (core, rules, grid, locks, serialize) must not depend on firmware headers. Core style ids, saved bytes and golden patterns do not change.

## Testing
- New behaviour needs a test that fails first. A check that compares a value with itself passes for the wrong reason; record the value before the action and require it to change.
- With `GRIND_NATIVE=1` the start screen is BLAST IGNITION, so legacy HORTATOR title checks (`tests/ui_test.c`) run only with `GRIND_NATIVE=0`.
- In host UI tests a button acts in the frame that follows `press` (PLAY included); `ui_frame()` also renders audio, which consumes `transport_req`. Assert on a state that survives, such as `ui.home` or `song.playing`.
- Host tests that fake a running song (`chain.running = 1`) must not run frames while it is faked.

## Gotchas
- App RAM is at 95,160 of 98,304 B on the native build. Check CI's `RAM .data+.bss` line before adding static state. If a change would push it above 98,304 B, stop, report the measured total and ask the user; do not shrink the voice cap or pool floor. Features that need more than ~3 KB (ANCHOR, Tape) were deferred for that reason.
- The native firmware build needs the pinned JieLi toolchain and SDK (`BUILDING.md`; CI's native lane is the reference for the RAM figures). Host tests do not need them.
- The WASM build (`tools/build_sim.sh`, which then runs `sim_glue.mjs` in Node) needs Emscripten via Docker; `browser_smoke.mjs` also needs Chromium. Neither is available on a plain Windows setup, so they first run in CI. The simulator host test (`tests/sim_test.c`) builds locally with `-DSIGALRM=14 '-Dalarm(x)=0'` on Windows.
- `.vscode/`, `compile_commands.json` and `.clangd` are git-ignored; do not commit them.
- Before opening or merging a PR, check its base branch (`gh pr view N --json baseRefName`). A PR whose base is another feature branch merges into that branch, not main.
- Do not merge, release, or write to a connected FM-1 without the user's explicit instruction.
