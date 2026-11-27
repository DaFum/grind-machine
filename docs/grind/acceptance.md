# GRIND acceptance evidence

What was run for BLAST IGNITION (the 400 ms interruptible startup presentation, `GRIND_NATIVE=1` only), with the result
actually observed. Nothing here is a device measurement unless it says so. Source: branch `feat/blast-ignition`, base
`8942e7f` (the commit before BA01).

## Host tests (Windows, `python -m ziglang cc`, Python 3.12, Node 25)

| Command | Result |
|---|---|
| `sh tests/run_grind_tests.sh boot_anim` | pass (portable sampler and clock, ASan/UBSan flags where the runner sets them) |
| `sh tests/run_grind_tests.sh boot_draw` | pass, `GRIND_NATIVE` 0 and 1 (pixel probes, palette only, incremental equals full) |
| `sh tests/run_grind_tests.sh boot_ui` | pass, 0 and 1 (natural run equals an unarmed baseline, input, encoder, wrap) |
| `sh tests/run_grind_tests.sh boot_recovery` | pass (`tests/boot_test.c` with `GRIND_NATIVE=1`: remembered, failed and absent project, SAFE START, persist note, new message, input) |
| `sh tests/run_grind_tests.sh boot_restore` | pass (`tests/editor_backup_test.c`: RESTORING cancels, no restart) |
| `sh tests/run_grind_tests.sh editor` | pass (update traffic through the real `ed.hush` and `usb.ota_req`) |
| the other GRIND suites, `tests/grind/test_edit_sites.py` | pass |
| upstream `boot_test`, `ui_test`, `editor_pattern_test`, each with `GRIND_NATIVE` 0 and 1 | pass (6 of 6) |
| `tests/sim_test.c` with `GRIND_NATIVE` 0 and 1 (simulator host core) | pass |
| `node tests/sim_boot_preview.mjs` | pass |

`golden` fails on a Windows checkout with CRLF line endings; that is not a regression (see `AGENTS.md`).

## Not run here

- WASM build, `tests/sim_glue.mjs` and `tests/grind/browser_smoke.mjs`: they need Emscripten (Docker) and Chromium, which this
  machine does not have. The code is written and syntax-checked; the CI WASM lane is the first real run.
- The native firmware link and its RAM report: needs the pinned JieLi toolchain; CI's native lane is the reference. The
  change adds about 16 bytes of `.bss` (the adapter state); no animation framebuffer exists.
- Hardware (title, first PLAY or note, SAFE START, calibration return, update): not tested on a device. These fixtures model
  the boot and do not execute `fm1_cstart()`. Calibration cancelling (`panel_setup`) is a one-line call, not exercised by a
  host test (it blocks on hardware input).

## Protected paths

`git diff 8942e7f HEAD`, the same with `--cached`, and the working-tree diff, over `main.c`, `firmware/hal`,
`firmware/loader`, `ota.c`, `usb.c`, `usb_app.c`, `crt0.S`, `app.ld`, `storage.c`, `core.h`, `tools/check_untouched.py` and
`tools/frozen_base.txt`: empty. `tools/check_untouched.py --available-reference` fails on this Windows checkout, and it
fails the same way on an untouched checkout of the base (line endings); it runs on Linux in CI.
