# GRIND MACHINE BLAST IGNITION Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved BLAST IGNITION title and interruptible 400 ms display presentation with the existing FM-1 graphics, plus an opt-in preview using the same C renderer.

**Architecture:** Keep the protected boot sequence unchanged: the existing title hook draws SYSTEM WAKE, and the first eligible ordinary UI draw starts the moving presentation after the inherited startup delay. A portable integer sampler, a canvas renderer and a main-loop adapter own timing, pixels and lifecycle respectively. Native input continues normally; recovery and messages take display priority.

**Tech Stack:** Existing C99-compatible portable code and GNU C firmware amalgamation, `cv_*`/`md_text`, RGB565 LCD DMA, existing host fixtures, Emscripten, vanilla JavaScript and the existing simulator paint loop. No additional runtime dependency.

---

**Design:** [BLAST IGNITION specification](../specs/2026-11-27-grind-blast-ignition-design.md).

**Source inspected:** `DaFum/grind-machine@67be119a59b11d0ae154217d9248becefca59190`, 2026-11-27.

**Execution status:** Planning only. Code blocks below are proposed implementation contracts, not code already installed or tested in firmware. The accompanying SVG is a design storyboard.

## 1. Scope and repository facts

The user's approval covers this design and implementation plan. It does not need a second design approval. This document inherits the existing [Community quality policy](2026-11-27-grind-machine-implementation.md#community-quality-policy-user-approved-direction-revision-11), existing applicable test/build checks and actual native limits. The functional assertions below test the requested behavior; they do not create another release process or approval gate.

Read the pinned source links in specification section 8 before execution. Recheck the current branch because other GRIND work may change the UI or simulator. Preserve that work and integrate at function anchors, not historical line numbers.

Before any BA01 edits or commits, capture the chosen implementation base once with `GM_BOOT_BASE=$(git rev-parse HEAD)` and record its full SHA in the implementation evidence. The inspected source for this plan is `67be119a59b11d0ae154217d9248becefca59190`; a later implementation must record its actual starting commit. Restore `GM_BOOT_BASE` from that recorded SHA in a new execution session. Do not recompute it from a moving `HEAD` or `main` during BA06.

Key source constraints:

- `main.c` calls the title before timers/input/audio, then performs its existing 30 ms wait, possible calibration, 400 ms wait and black fill. Motion starts **after** that sequence in the regular UI. Do not replace, wrap or macro-substitute `fm1_delay_ms`, move protected code into an exempt function, render from an IRQ or widen the checker exclusions.
- `ui_draw()` has an `always_inline` declaration used by the existing comparison tooling. Keep it. SAFE START stays first.
- `cv_px` already holds 240×124 pixels. `cv_begin*()` synchronizes DMA but does not reset `cv_oy`. `lcd_blit()` does not screen-clip. Two 240×120 windows can paint the whole scene; ordinary updates use the two smaller specified windows.
- `project_boot()` already creates a 40-frame LOADED message. Preserve it with a typed deferral; do not identify messages by string matching or reset their lifetime.
- `sim_init()` does not call the title. Its time normally comes from audio samples. Keep that default and use an explicit-time preview instance.
- `tests/run_grind_tests.sh` accepts **one** suite name and uses explicit registration. Shell `GRIND_NATIVE=1` alone does not change its existing `run_native()` compiler command. The new dual-mode helper below supplies `-DGRIND_NATIVE` explicitly.

## 2. File ownership and dependency order

| File | Responsibility / change |
|---|---|
| New `firmware/src/grind/boot_anim.h`, `boot_anim.c` | Pure sample, phase boundaries, one-shot clock and unsigned elapsed-time handling |
| New `firmware/src/grind/boot_draw.c` | Fixed geometry, local RGB565 palette, full/dirty window rendering using existing canvas |
| New `firmware/src/grind/boot_ui.c` | Main-loop state, eligibility, typed notice deferral, display handoff; includes the other two C modules once |
| `firmware/src/ui.c` | Static adapter declarations; `ui_say()` notification hook |
| `firmware/src/ui_menu.c` | Conditional replacement inside `draw_boot_title()` |
| `firmware/src/ui_input.c` | Observe existing input snapshots; calibration-entry cancellation |
| `firmware/src/ui_draw.c` | Presentation service after SAFE START and before `ui.frame++` |
| `firmware/src/project.c` | Mark only the successful boot-autoload notice as deferrable |
| `firmware/src/enc.c`, `panel.c` | Non-consuming pending-step query; cancel when a normal mapped encoder read returns a step |
| `firmware/src/felucca.c` | Late guarded inclusion after editor/console support, before `main.c` |
| `tests/ui_host.h`, `tests/sim_host.h` | Mirror inclusion and pending-encoder adapters; host reset between independent tests |
| New `tests/grind/test_boot_anim.c`, `test_boot_draw.c`, `test_boot_ui.c` | Portable boundary tests, framebuffer/window checks and native UI behavior |
| `tests/run_grind_tests.sh` | Register focused suites and include them in `all` |
| `tests/boot_test.c`, `tests/editor_test.c`, `tests/editor_backup_test.c` | Extend real recovery/update/restore fixtures only where these hooks need coverage |
| `tests/sim_core.c`, `tests/sim_test.c`, `tests/sim_glue.mjs` | Shared scene preview export and native/WASM equivalence |
| `tools/build_sim.sh` | Explicit preview export and preview-controller test registration |
| `web/sim/engine.js`, `app.js`; new `web/sim/boot-preview.js`, `tests/sim_boot_preview.mjs` | Thin WASM wrapper and cancelable opt-in presentation controller |
| `tests/grind/browser_smoke.mjs` | Assert the actual build mode and rendered preview handoff using the existing browser fixture |
| `docs/manual/15-simulator.md`, `docs/grind/acceptance.md`, `CHANGELOG.md` | Describe implemented behavior and record actual evidence in BA06 |

Firmware include order is `ui.c → ui_draw.c → ui_menu.c → ui_input.c → project.c → editor.c → main.c`. Put guarded static declarations in `ui.c`, definitions in the late `boot_ui.c`. This permits reading `persist_note`, UI state and, in the OTA build, `ed.hush`. `boot_draw.c` must not depend on the `rg_*` helpers or future UI declarations.

All native hooks and state are under `#if GRIND_NATIVE`. The portable sampler can also compile standalone. Neither `main.c`, `gfx.c`, `lcd.c`, HAL, loader, persistence layouts, audio/IRQ code, nor the frozen checks need modification.

## 3. BA01 — Define and test the deterministic sample and clock

**Files:** create `boot_anim.h`, `boot_anim.c`, `tests/grind/test_boot_anim.c`; modify `tests/run_grind_tests.sh`.

- [ ] Add the header contract and a failing test executable with the independent expected table below. Register the suite before invoking it. Keep SPDX GPL-3.0-only notices and existing authorship where applicable.

```c
/* firmware/src/grind/boot_anim.h */
#ifndef GM_BOOT_ANIM_H
#define GM_BOOT_ANIM_H
#include <stdint.h>
#define GM_BOOT_DURATION_MS 400u
#define GM_BOOT_NO_COLUMN 16u
enum gm_boot_phase {
    GM_BOOT_WAKE, GM_BOOT_IGNITION, GM_BOOT_ATTACK,
    GM_BOOT_READY, GM_BOOT_FINISHED
};
enum gm_boot_state { GM_BOOT_OFF, GM_BOOT_PENDING, GM_BOOT_ACTIVE, GM_BOOT_DONE };
typedef struct {
    uint8_t phase, revealed, cursor, impact;
} gm_boot_frame;
typedef struct {
    uint32_t started_ms;
    uint8_t state;
} gm_boot_clock;
gm_boot_frame gm_boot_sample(uint32_t elapsed_ms);
uint32_t gm_boot_key(gm_boot_frame f);
void gm_boot_arm(gm_boot_clock *c);
void gm_boot_cancel(gm_boot_clock *c);
int gm_boot_step(gm_boot_clock *c, uint32_t now_ms, int blocked, gm_boot_frame *f);
#endif
```

Independent expected sample rows are `{time, phase, revealed, cursor, impact}`:

```c
static const uint32_t cases[][5] = {
    {0,   GM_BOOT_IGNITION, 0,  0,  0},
    {60,  GM_BOOT_IGNITION, 8,  8,  0},
    {119, GM_BOOT_IGNITION, 15, 15, 0},
    {120, GM_BOOT_ATTACK,   16, 0,  1},
    {199, GM_BOOT_ATTACK,   16, 7,  1},
    {200, GM_BOOT_ATTACK,   16, 8,  2},
    {279, GM_BOOT_ATTACK,   16, 15, 2},
    {280, GM_BOOT_READY,    16, 16, 0},
    {399, GM_BOOT_READY,    16, 16, 0},
    {400, GM_BOOT_FINISHED, 16, 16, 0},
    {UINT32_MAX, GM_BOOT_FINISHED, 16, 16, 0}
};
```

For each row, assert all four fields, not a `memcmp` over unspecified padding. Also sample these rows in reverse order and after repeated equal calls: output must remain identical. Test OFF never starts, PENDING starts at its first eligible call, blocked PENDING never starts, cancellation is terminal, rearming DONE does nothing, and a skipped update past the deadline returns inactive.

```c
gm_boot_clock c = {0};
gm_boot_frame f;
gm_boot_arm(&c);
assert(gm_boot_step(&c, UINT32_MAX - 100u, 0, &f));
assert(gm_boot_step(&c, 298u, 0, &f)); /* elapsed 399 across wrap */
assert(f.phase == GM_BOOT_READY);
assert(!gm_boot_step(&c, 299u, 0, &f)); /* elapsed 400 */
assert(c.state == GM_BOOT_DONE);
gm_boot_arm(&c);
assert(!gm_boot_step(&c, 300u, 0, &f));
```

- [ ] Add `boot_anim) run_test boot_anim "$CORE/boot_anim.c" ;;` to the dispatch and `run_test boot_anim "$CORE/boot_anim.c"` to `all`. The test includes `"boot_anim.h"`; existing `-I"$CORE"` is sufficient. Run `sh tests/run_grind_tests.sh boot_anim`. Expect a missing implementation first, then a failing boundary assertion if only a stub is present; retain the actual red output.
- [ ] Implement the complete portable behavior:

```c
/* firmware/src/grind/boot_anim.c */
#include "boot_anim.h"
gm_boot_frame gm_boot_sample(uint32_t t)
{
    gm_boot_frame f = {GM_BOOT_FINISHED, 16, GM_BOOT_NO_COLUMN, 0};
    if (t < 120u) {
        f.phase = GM_BOOT_IGNITION;
        f.revealed = (uint8_t)(16u * t / 120u);
        f.cursor = f.revealed;
    } else if (t < 280u) {
        f.phase = GM_BOOT_ATTACK;
        f.cursor = (uint8_t)(16u * (t - 120u) / 160u);
        f.impact = (uint8_t)(t < 200u ? 1u : 2u);
    } else if (t < GM_BOOT_DURATION_MS) {
        f.phase = GM_BOOT_READY;
    }
    return f;
}
uint32_t gm_boot_key(gm_boot_frame f)
{
    return ((uint32_t)f.phase << 16) | ((uint32_t)f.revealed << 8)
         | ((uint32_t)f.cursor << 2) | f.impact;
}
void gm_boot_arm(gm_boot_clock *c)
{
    if (c->state == GM_BOOT_OFF)
        c->state = GM_BOOT_PENDING;
}
void gm_boot_cancel(gm_boot_clock *c)
{
    if (c->state == GM_BOOT_PENDING || c->state == GM_BOOT_ACTIVE)
        c->state = GM_BOOT_DONE;
}
int gm_boot_step(gm_boot_clock *c, uint32_t now, int blocked, gm_boot_frame *f)
{
    if (blocked)
        gm_boot_cancel(c);
    if (c->state == GM_BOOT_PENDING) {
        c->started_ms = now;
        c->state = GM_BOOT_ACTIVE;
    }
    if (c->state != GM_BOOT_ACTIVE)
        return 0;
    *f = gm_boot_sample((uint32_t)(now - c->started_ms));
    if (f->phase == GM_BOOT_FINISHED) {
        gm_boot_cancel(c);
        return 0;
    }
    return 1;
}
```

The multiplication occurs only in the bounded phase branches; `UINT32_MAX` is not multiplied. The wake still is a distinct constant `{GM_BOOT_WAKE, 0, GM_BOOT_NO_COLUMN, 0}` and does not use a pre-initialization time source.

- [ ] Run the same focused suite green and, where configured, its existing sanitizer/WASM modes:

```sh
sh tests/run_grind_tests.sh boot_anim
GRIND_SAN_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all' sh tests/run_grind_tests.sh boot_anim
GRIND_WASM=1 CC=emcc sh tests/run_grind_tests.sh boot_anim
git diff --check
git add -- firmware/src/grind/boot_anim.h firmware/src/grind/boot_anim.c tests/grind/test_boot_anim.c tests/run_grind_tests.sh
git commit -m "feat(grind): define deterministic boot animation phases"
```

Expected: every boundary and lifecycle assertion passes. An unavailable compiler is recorded for that command; it is not a passing run.

## 4. BA02 — Render the approved pixel scene with existing graphics

**Files:** create `firmware/src/grind/boot_draw.c`, `tests/grind/test_boot_draw.c`; modify `tests/ui_host.h` for optional window assertions and `tests/run_grind_tests.sh` for a dual-mode fixture helper. The fixture can initially include `boot_anim.c` and `boot_draw.c` directly after `ui_host.h`; BA03 replaces those two includes with the shared adapter inclusion.

- [ ] Add focused pixel tests before the renderer. Build the fixture with `GRIND_NATIVE=0` and `=1`, using distinct output files. The off build checks the existing title; the on build checks the proposed scene. Do not hardcode `#define GRIND_NATIVE 1` in the new fixture.

Use this helper in the existing runner, adding each named suite to its dispatch and `all` when its file exists:

```sh
run_boot_native() {
    suite=$1
    [ "$WASM" = 1 ] && { echo "$suite: native fixture is host-only"; return 0; }
    "${PYTHON:-python3}" tools/build.py --gen-only > /dev/null
    for native in 0 1; do
        $CC -std=gnu11 -O1 -g -Wall -Wno-unused-function -Wno-int-to-pointer-cast \
            -Wno-macro-redefined -Wno-date-time \
            ${GRIND_SAN_FLAGS:+${GRIND_SAN_FLAGS} -fno-sanitize=shift-base} \
            -UNDEBUG -DGRIND_NATIVE="$native" -DGRIND_BOOT_LCD_ASSERT=1 \
            -Ibuild/gen -Ifirmware/src -Itests -I"$CORE" \
            "tests/grind/test_$suite.c" -o "$OUT/test_${suite}_$native" -lm
        "$OUT/test_${suite}_$native"
    done
}
# case entry: boot_draw) run_boot_native boot_draw ;;
# all entry:  run_boot_native boot_draw
```

In the fixture, include `<assert.h>` before `ui_host.h`. For `GRIND_BOOT_LCD_ASSERT`, assert in the host `lcd_blit` stub before copying: `w && h`, `x < 240`, `y < 240`, `w <= 240-x`, `h <= 240-y` and `w*h <= 240*124`. Keep these assertions local to the new fixture flag so existing unrelated test expectations are not silently rewritten. The host stub's clipping would otherwise hide invalid native windows.

Independent pixel probes use native-order RGB565 in `fb`:

| Sample | Pixel | Expected |
|---|---|---|
| WAKE | `(0,0)`, `(8,8)`, `(44,44)` | `0x0841`, `0x30C3`, `0xB925` |
| 0 ms | `(25,138)` | `0x30C3` (nothing revealed) |
| 60 ms | `(25,138)`, `(121,131)` | `0xB925`, `0xF26A` |
| 199 ms | `(44,64)` | `0xB925` (local strike) |
| 200 ms | `(44,64)`, `(121,138)`, `(121,145)` | `0xF79D`, `0xF26A`, `0x30C3` |
| 340 ms | `(66,96)`, `(24,124)`, `(121,131)` | `0xF79D`, `0xB925`, `0x0841` |

Assert text widths `152`, `108`, `94` at scales 8/4/2. Assert all colors belong to the five-entry local palette. Check the stored theme and `C_BG/C_INK` before/after for every existing theme.

- [ ] Run `sh tests/run_grind_tests.sh boot_draw`, retain red output, then implement the scene below. All functions are static except the portable API. Include `boot_anim.h` and preserve the source font attribution.

```c
static const uint16_t GM_BOOT_BG = RGB(8, 8, 9);
static const uint16_t GM_BOOT_DIM = RGB(49, 26, 29);
static const uint16_t GM_BOOT_RED = RGB(188, 37, 45);
static const uint16_t GM_BOOT_HOT = RGB(244, 76, 81);
static const uint16_t GM_BOOT_PAPER = RGB(246, 240, 239);
static const uint16_t GM_BOOT_PATTERN[8] = {
    0x5555u, 0xAAAAu, 0x5555u, 0x8888u,
    0x0800u, 0x2000u, 0x8000u, 0x8001u
};
static void gm_boot_scene(gm_boot_frame f)
{
    uint32_t r, c;
    int wake = f.phase == GM_BOOT_WAKE;
    uint16_t title = f.phase >= GM_BOOT_ATTACK ? GM_BOOT_PAPER : GM_BOOT_RED;
    uint16_t sub = wake ? GM_BOOT_DIM : f.phase >= GM_BOOT_READY ? GM_BOOT_PAPER : GM_BOOT_HOT;
    cv_frame(8, 8, 224, 224, GM_BOOT_DIM);
    md_text((240 - md_text_w(&MD_F3, 8, "GRIND")) / 2, 44, &MD_F3, 8, "GRIND", title);
    md_text((240 - md_text_w(&MD_F3, 4, "MACHINE")) / 2, 96, &MD_F3, 4, "MACHINE", sub);
    md_text((240 - md_text_w(&MD_F3, 2, "FM-1 EDITION")) / 2, 210, &MD_F3, 2,
            "FM-1 EDITION", wake ? GM_BOOT_DIM : GM_BOOT_PAPER);
    cv_rect(24, 124, 192, 2, f.phase >= GM_BOOT_READY ? GM_BOOT_RED : GM_BOOT_DIM);
    for (r = 0; r < 8u; r++) {
        for (c = 0; c < 16u; c++) {
            uint16_t color = GM_BOOT_DIM;
            if (!wake && c < f.revealed && ((GM_BOOT_PATTERN[r] >> c) & 1u))
                color = f.phase == GM_BOOT_ATTACK && c == f.cursor ? GM_BOOT_HOT : GM_BOOT_RED;
            cv_rect(25 + 12 * (int32_t)c, 138 + 7 * (int32_t)r, 10, 4, color);
        }
    }
    if (f.cursor < GM_BOOT_NO_COLUMN)
        cv_rect(25 + 12 * (int32_t)f.cursor, 131, 10, 2, GM_BOOT_HOT);
    if (f.impact) {
        uint16_t color = f.impact == 1u ? GM_BOOT_HOT : GM_BOOT_DIM;
        cv_line(22, 46, 36, 52, color); cv_line(20, 68, 34, 68, color);
        cv_line(22, 90, 36, 84, color); cv_line(204, 52, 218, 46, color);
        cv_line(206, 68, 220, 68, color); cv_line(204, 84, 218, 90, color);
        if (f.impact == 1u)
            cv_rect(44, 64, 152, 2, GM_BOOT_RED);
    }
}
static void gm_boot_window(gm_boot_frame f, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    /* Only the fixed, bounded windows below reach this private helper. */
    cv_oy = 0;
    cv_begin_win(x, y, w, h, 0, GM_BOOT_BG);
    gm_boot_scene(f);
    cv_blit(0, 0);
    cv_oy = 0;
}
static void gm_boot_draw_full(gm_boot_frame f)
{
    gm_boot_window(f, 0, 0, 240, 120);
    gm_boot_window(f, 0, 120, 240, 120);
}
static void gm_boot_draw_changed(gm_boot_frame old, gm_boot_frame f)
{
    if (gm_boot_key(old) == gm_boot_key(f))
        return;
    if (old.phase != f.phase || old.impact != f.impact)
        gm_boot_window(f, 20, 36, 201, 84);
    if (old.phase != f.phase || old.revealed != f.revealed || old.cursor != f.cursor)
        gm_boot_window(f, 24, 124, 192, 70);
}
```

`gm_boot_draw_changed` is for active scenes only: WAKE → first active frame always uses `gm_boot_draw_full`, including the footer change. Only render the FINISHED sample as a READY still in the preview by clamping to 399 ms before sampling. No other caller passes FINISHED to the scene renderer.

- [ ] Add the incremental/full equivalence fixture using a test-only `uint16_t expected[240*240]`: render each current sample fully into `fb`, copy expected pixels, render the previous sample fully, apply `gm_boot_draw_changed`, and compare all pixels. Cover 0→60→119→120→199→200→279→280→399 and skips 0→280, 120→399. Verify an identical sample adds zero to `host_blit_px` and `host_wins`; dirty updates use their defined windows rather than a full screen. These are structural assertions, not a new traffic budget.
- [ ] Run `sh tests/run_grind_tests.sh boot_draw` and the existing graphics/UI coverage through `sh tests/run_drum_tests.sh` once after integration. Keep the legacy `tests/ui_test.c::test_boot_title` and theme-sheet expectations for the off build; do not delete them. Confirm DMA ownership by inspecting that every write to the shared canvas comes through `cv_begin*()` before a blit. Host framebuffer equality cannot prove asynchronous DMA timing.
- [ ] Commit the focused files with `git commit -m "feat(grind): draw BLAST IGNITION with the existing canvas"` after `git diff --check` and a path-limited `git add` for this task.

## 5. BA03 — Integrate the one-shot presentation and normal input

**Files:** create `firmware/src/grind/boot_ui.c`; modify `ui.c`, `ui_menu.c`, `ui_input.c`, `ui_draw.c`, `enc.c`, `panel.c`, `felucca.c`, `tests/ui_host.h`, `tests/sim_host.h`; create `tests/grind/test_boot_ui.c`; extend the runner.

- [ ] Add the lifecycle fixture and register `boot_ui) run_boot_native boot_ui ;;` plus `run_boot_native boot_ui` in `all`. The on fixture explicitly calls the title; the normal simulator remains unarmed because it never calls that hook. Record red with `sh tests/run_grind_tests.sh boot_ui`.
- [ ] In `ui.c`, before `ui_say()`, declare the following under `#if GRIND_NATIVE`:

```c
static void gm_boot_ui_reset(void); /* host new-boot fixtures only; native BSS supplies reset */
static void gm_boot_ui_title(void);
static void gm_boot_ui_cancel(void);
static void gm_boot_ui_notice_changed(void);
static void gm_boot_ui_defer_loaded(void);
static void gm_boot_ui_input(uint32_t pressed, uint32_t notes);
static int gm_boot_ui_draw(uint32_t now_ms);
```

- [ ] Define the adapter late. Its dependencies are the portable/renderer modules above and actual existing UI/editor fields:

```c
/* firmware/src/grind/boot_ui.c: include once, after UI/project/editor definitions */
#include "boot_anim.c"
#include "boot_draw.c"
static struct {
    gm_boot_clock clock;
    gm_boot_frame previous;
    uint8_t has_pixels, previous_valid, deferred_loaded; /* has_pixels: active UI-loop scene only */
} gm_boot_ui;
static void gm_boot_ui_reset(void)
{
    memset(&gm_boot_ui, 0, sizeof gm_boot_ui);
}
static void gm_boot_ui_cancel(void)
{
    gm_boot_cancel(&gm_boot_ui.clock);
    if (gm_boot_ui.clock.state == GM_BOOT_DONE && gm_boot_ui.has_pixels)
        ui.force = 1;
}
static void gm_boot_ui_notice_changed(void)
{
    gm_boot_ui.deferred_loaded = 0;
    if (gm_boot_ui.clock.state == GM_BOOT_ACTIVE)
        gm_boot_ui_cancel();
}
static void gm_boot_ui_defer_loaded(void)
{
    if (gm_boot_ui.clock.state == GM_BOOT_PENDING && ui.msg_t)
        gm_boot_ui.deferred_loaded = 1;
}
static int gm_boot_ui_blocked(void)
{
    if (safe_start || persist_note || ui.confirm || ui.menu || ui.mute || ui.tools ||
        ui.layer || ui.pg_open || ui.cue_map || name_on() || pp_on() || pl_on() ||
        transport_busy() || UI_RESTORING() || (ui.msg_t && !gm_boot_ui.deferred_loaded))
        return 1;
#if FELUCCA_OTA
    if (ed.hush || usb.ota_req || usb.uboot_req)
        return 1;
#endif
    return 0;
}
static void gm_boot_ui_title(void)
{
    const gm_boot_frame wake = {GM_BOOT_WAKE, 0, GM_BOOT_NO_COLUMN, 0};
    if (gm_boot_ui.clock.state != GM_BOOT_OFF)
        return;
    gm_boot_arm(&gm_boot_ui.clock);
    gm_boot_draw_full(wake);
    gm_boot_ui.has_pixels = 0; /* main.c clears this early WAKE before any ordinary UI call */
    gm_boot_ui.previous_valid = 0;
}
static void gm_boot_ui_input(uint32_t pressed, uint32_t notes)
{
    uint32_t e;
    if (gm_boot_ui.clock.state != GM_BOOT_PENDING && gm_boot_ui.clock.state != GM_BOOT_ACTIVE)
        return;
    if (pressed || notes || fm1_in.buttons || fm1_in.notes || gm_boot_ui_blocked())
        gm_boot_ui_cancel();
    for (e = 0; e < 7u; e++)
        if (enc_pending(e))
            gm_boot_ui_cancel();
}
static int gm_boot_ui_draw(uint32_t now_ms)
{
    gm_boot_frame f;
    if (gm_boot_step(&gm_boot_ui.clock, now_ms, gm_boot_ui_blocked(), &f)) {
        if (!gm_boot_ui.previous_valid)
            gm_boot_draw_full(f);
        else
            gm_boot_draw_changed(gm_boot_ui.previous, f);
        gm_boot_ui.previous = f;
        gm_boot_ui.previous_valid = 1;
        gm_boot_ui.has_pixels = 1;
        return 1;
    }
    if (gm_boot_ui.has_pixels && gm_boot_ui.clock.state == GM_BOOT_DONE) {
        lcd_sync();
        lcd_fill(0, 0, 240, 240, C_BG);
        ui.force = 1;
        gm_boot_ui.has_pixels = 0;
        gm_boot_ui.previous_valid = 0;
    }
    return 0;
}
```

Call the blocker only while pending/active or about to hand off; a later implementation may short-circuit terminal states to avoid unnecessary reads. It remains read-only. A running UI remains responsive because the adapter never waits for elapsed time, consumes an event, changes a project, or invokes audio/USB services itself.

The early title and active scene have different cleanup owners: the unchanged `main.c` clears SYSTEM WAKE; the adapter clears only pixels it subsequently painted during ACTIVE. Cancelling or blocking PENDING must not add another full-screen fill before the first native dialog or input result. ACTIVE completion/cancellation still synchronizes and clears once before the native redraw.

- [ ] Add these guarded hooks at their exact function anchors:

| Function | Code insertion |
|---|---|
| `draw_boot_title()` | `#if GRIND_NATIVE` / `gm_boot_ui_title();` / `#else` / entire existing title body / `#endif` |
| `ui_say()` | Call `gm_boot_ui_notice_changed();` before overwriting the existing message |
| `ui_input()` | Call `gm_boot_ui_input(pressed, notes);` after `now = fm1_ticks();`, before `fam = cur_fam();` |
| `ui_draw()` | After the SAFE-START block, before `ui.frame++`: `if (gm_boot_ui_draw(fm1_ms)) return;` |
| `panel_setup()` | Call `gm_boot_ui_cancel();` before any drawing or waiting |

Each insertion above is surrounded by `#if GRIND_NATIVE` / `#endif`. Do not change the native `pressed`/`notes` variables, held-button bits or the normal handler order.

- [ ] Add the non-consuming encoder helper next to `enc_take`, with matching host adapters:

```c
/* enc.c, under #if GRIND_NATIVE */
static int enc_pending(uint32_t e) { return enc_st.total[e] != enc_seen[e]; }
/* tests/ui_host.h, under #if GRIND_NATIVE */
static int enc_pending(uint32_t e) { return host_enc[e % 7u] != 0; }
/* tests/sim_host.h, under #if GRIND_NATIVE */
static int enc_pending(uint32_t e) { return sim_enc_acc[e % 7u] != 0; }
```

Also guard-declare `static void gm_boot_ui_cancel(void);` immediately before `panel_enc()` in `panel.c`, and replace its single return with this equivalent consumption plus notification:

```c
int32_t steps = enc_take(panel.enc[role]) * panel.dir[role];
#if GRIND_NATIVE
if (steps)
    gm_boot_ui_cancel();
#endif
return steps;
```

This catches a step arriving after the initial peek but consumed by a later native handler in the same UI pass. It does not read or drain the encoder twice. Preserve `host_enc_late` semantics; the peek must not advance that injected-late-step fixture.

- [ ] Add `#if GRIND_NATIVE` / `#include "grind/boot_ui.c"` / `#endif` in `felucca.c` after the optional editor/console/BT-API definitions and before the `main.c` include. Add the corresponding relative include after project inclusion in `tests/ui_host.h` and `tests/sim_host.h`. In `ui_host.h`, allow `GM_BOOT_UI_DEFER_INCLUDE` to defer this include for the editor fixture in BA04. Add a guarded `gm_boot_ui_reset()` call in `ui_host_init()` so independent tests cannot inherit terminal state. Remove BA02's temporary direct includes from the renderer fixture.
- [ ] Drive `fm1_ms` explicitly in the new fixtures and reset it for each modeled boot. Existing `ui_frame()`/`render_mix()` do not advance this clock; `host_ticks` is separate. For example, after the first draw at `fm1_ms = 1000u`, set it to `1120u`, `1280u` and `1400u` before the next modeled draws. Use explicit wrapped values for the wraparound case. Model the existing `main.c` black fill after the title/calibration sequence and before the first ordinary UI call. Do not change global host-clock behavior to make these tests pass.
- [ ] Run focused fixtures green. Check a complete natural run, cancellation before the first active draw, skipped frames, and a millisecond wrap. Compare input outcomes with an otherwise identical unarmed baseline: `transport_req`/PLAY, selected track after one note, parameter delta for one encoder step, HOME/REC tap and hold effects. Run the same event sequence twice only in independently reset fixtures, never compare two samples of the animation as the sole oracle. Verify first input is processed once and not postponed to release; verify normal `ui_leds()` is still called.
- [ ] Confirm completion/cancellation restores the full native framebuffer, leaves `ui.page`, `ui.home`, selected project, theme and pattern unchanged, and cannot restart on another `ui.force`. Measure the adapter separately from the following native draw: cancelling/blocking PENDING after the modeled main clear adds zero LCD fills/blits, while cancellation after an ACTIVE frame performs one cleanup and the next native redraw matches the unarmed baseline. Include failed autoload and input before the first active draw. Run `sh tests/run_grind_tests.sh boot_ui`, `sh tests/run_grind_tests.sh boot_draw`, and the relevant existing UI/encoder tests through the existing host runner. Commit `feat(grind): integrate interruptible boot presentation` with only BA03 paths.

## 6. BA04 — Preserve boot notices, calibration and update priority

**Files:** `firmware/src/project.c`, `tests/grind/test_boot_ui.c`, `tests/boot_test.c`, `tests/editor_test.c`, `tests/editor_backup_test.c`, `tests/ui_host.h`, `tests/run_grind_tests.sh`.

- [ ] Add a red test for a successful saved-project boot: title arms PENDING; `project_boot()` leaves LOADED with `ui.msg_t == 40`; first eligible draw starts ACTIVE; further active draws retain 40; the first normal draw after completion begins the original countdown. Set `fm1_ms` explicitly before those draws (start, +120, +280, +400); repeating `ui_frame()` alone does not advance it. The existing message is not recreated. Add a second test in which a new `ui_say()` during ACTIVE clears the exception, cancels, and renders the new message with its original lifetime.
- [ ] Mark the typed success only inside the existing successful `project_boot()` branch, after setting `song.g[G_SLOT]`:

```c
#if GRIND_NATIVE
gm_boot_ui_defer_loaded();
#endif
```

Do not change `proj_load()`, the failed-load dialog, `persist_note_show()` or flash data. A message created while PENDING is evaluated at start; cancelling all PENDING messages inside `ui_say()` would make the typed LOADED hook too late.

- [ ] Extend the existing real boot fixture under `GRIND_NATIVE` to reset the new adapter, invoke the existing title after settings initialization, and model the unchanged main black fill before ordinary UI service. Reuse its flash setup and boot cases; add assertions to the cases below. These host fixtures model boot behavior and do not execute native `fm1_cstart()` or prove physical recovery.

| Fixture setup | Required observation |
|---|---|
| No remembered project, no errors | Presentation runs once; native boot state unchanged |
| Valid remembered project | Existing successful LOADED deferred, then shown with full lifetime |
| Failed remembered project | `ui.confirm == 5`; normal failure dialog, no active presentation or extra adapter clear |
| SEQ held at power-on | SAFE START, stopped state and existing recovery screen; no animation |
| Any `persist_note` loss/migration/clamp flag | No active presentation; existing note order/text unchanged |
| Enter calibration from PENDING or ACTIVE | Terminal cancellation before the first calibration paint; success/cancel/timeout never resumes animation |
| Normal input during ACTIVE | Original action once; no new hold suppression, LED pattern or audio trigger from the renderer |

Add this helper to compile existing fixtures without renaming their source files:

```sh
run_boot_existing() {
    suite=$1
    boot_source=$2
    [ "$WASM" = 1 ] && { echo "$suite: native fixture is host-only"; return 0; }
    "${PYTHON:-python3}" tools/build.py --gen-only > /dev/null
    $CC -std=gnu11 -O1 -g -Wall -Wno-unused-function -Wno-int-to-pointer-cast \
        -Wno-macro-redefined -Wno-date-time \
        ${GRIND_SAN_FLAGS:+${GRIND_SAN_FLAGS} -fno-sanitize=shift-base} \
        -UNDEBUG -DGRIND_NATIVE=1 -Ibuild/gen -Ifirmware/src -Itests -I"$CORE" \
        "$boot_source" -o "$OUT/test_$suite" -lm
    "$OUT/test_$suite"
}
# case entries:
# boot_recovery) run_boot_existing boot_recovery tests/boot_test.c ;;
# boot_restore) run_boot_existing boot_restore tests/editor_backup_test.c ;;
# all entries:
# run_boot_existing boot_recovery tests/boot_test.c
# run_boot_existing boot_restore tests/editor_backup_test.c
```

Keep the original off-mode boot run in `tests/run_drum_tests.sh`. These focused cases use the same optional sanitizer policy as the other native fixtures; no test is relabeled as a target-device result.

- [ ] In `tests/editor_test.c`, define `GM_BOOT_UI_DEFER_INCLUDE` before `ui_host.h` when GRIND is enabled. Include `grind/boot_ui.c` only after its real `editor.c` include and restored `FELUCCA_OTA=1`. This makes the test exercise the actual `ed.hush` branch, rather than the OTA-disabled host stub. Add a test using its existing request/frame helpers: arm title, set `usb.ota_req`, run `ed_service()`, clear the request as `main.c` does, call `ui_input()/ui_draw()`, and assert terminal cancellation despite `usb.ota_req == 0`. Repeat with ACTIVE and with recognized update traffic; clearing hush afterward must not restart it.
- [ ] Extend `tests/editor_backup_test.c` while its real `ED_RESTORE_LOCK` and `ed_restoring()` are active: arm title, enter its existing restore fixture, call UI, assert no active presentation and normal restore behavior. This fixture uses OTA-disabled `ui_host.h` and a real backup module declared later; its existing forward declaration supplies `ed_restoring()`. It needs no fake `ed.hush` or extra update state. Register a host-only `boot_restore` case compiling that existing file with `-DGRIND_NATIVE=1`, and add it to `all`.
- [ ] Run each affected command separately:

```sh
sh tests/run_grind_tests.sh boot_ui
sh tests/run_grind_tests.sh boot_recovery
sh tests/run_grind_tests.sh editor
sh tests/run_grind_tests.sh boot_restore
sh tests/run_drum_tests.sh
```

Expected: functional assertions pass in the affected fixtures. Existing optional host measurements or unavailable historical references retain their documented status; if they stop the broad runner before functional fixtures finish, execute those functional fixture commands independently rather than reporting an incomplete run as passed.

- [ ] Inspect the protected-path diff and commit `test(grind): preserve boot notices and recovery priority`. Keep the actual functional implementation change confined to the typed success hook; fix any demonstrated integration defect in its owning BA03 helper, not by weakening an old assertion or adding recovery exceptions.

## 7. BA05 — Add the opt-in shared-C simulator preview

**Files:** `tests/sim_core.c`, `tests/sim_test.c`, `tests/sim_glue.mjs`, `tests/grind/browser_smoke.mjs`, `tools/build_sim.sh`, `web/sim/engine.js`, `web/sim/app.js`; create `web/sim/boot-preview.js`, `tests/sim_boot_preview.mjs`.

- [ ] Write C and JS tests before the export. A fresh isolated simulator instance may render the scene without `sim_init()`. Snapshot flash dirty bits, transport, sample count and native boot state; preview rendering must leave them unchanged. Compare fixed-time preview pixels to the shared renderer at 0/60/120/200/340/399. The default `sim_init()` tests must remain responsive as before.
- [ ] Add this export after the simulator's normal declarations, and append `_sim_boot_preview` to `tools/build_sim.sh`'s `EXPORTS` list in both build modes:

```c
int sim_boot_preview(uint32_t elapsed_ms)
{
#if GRIND_NATIVE
    gm_boot_frame f = gm_boot_sample(elapsed_ms < GM_BOOT_DURATION_MS ? elapsed_ms : 399u);
    gm_boot_draw_full(f);
    return 1;
#else
    (void)elapsed_ms;
    return 0;
#endif
}
```

This is a scene-only export. It deliberately does not arm the UI adapter, install a demo, advance firmware time, call `sim_frame()` or render audio. Full repaint is acceptable for this isolated preview framebuffer; it does not change native dirty-window behavior. `engine.js` adds `bootPreview(ms) { return this.x.sim_boot_preview(ms) !== 0; }`. Pass integer nonnegative milliseconds clamped to 400 from the page.

- [ ] Add a small preview controller with explicit cancellation and a generation token. It uses the **existing** `paint(now)` loop; no second perpetual animation loop is added. The complete controller contract is:

```js
// web/sim/boot-preview.js
export function createBootPreview(load, emit) {
  let generation = 0, sim = null, start = null, held = false, last = -1, reduced = false;
  return {
    async begin(reduce = false) {
      const own = ++generation;
      sim = null; start = null; held = false; last = -1; reduced = reduce;
      const loaded = await load();
      if (own !== generation) return;
      if (!loaded.bootPreview(reduced ? 399 : 0)) return;
      sim = loaded;
      emit(sim.fb());
      last = reduced ? 399 : 0;
      held = reduced;
    },
    cancel() {
      generation++;
      sim = null; start = null; held = false; last = -1;
    },
    tick(now) {
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
    }
  };
}
```

The controller owns presentation only. The injected `load` creates a fresh `Fm1Sim` from the already fetched `wasmBytes` and never calls `.boot()`. `emit` sets the existing `frame` to `{fb, leds: 0, keyLeds: 0}` while the physical device is only a pre-power preview. It does not send LED or audio commands to a running simulator.

- [ ] In `app.js`, import `Fm1Sim` and `createBootPreview`; construct the controller after the existing frame state is available. Start it only when `new URLSearchParams(location.search).get('boot') === '1'`, after the existing `still` media query has been initialized. Catch a preview-load rejection and leave the normal reel available. At the start of `paint(now)`, use `if (!bootPreview.tick(now)) reelTick(now);` before normal frame painting. Listen for a change to reduced motion and call `.reduce()` when enabled. Turning it off does not replay the introduction automatically.
- [ ] Add one app-level cancellation helper called at the beginning of `powerOn()`, before its first `await`, and on Escape. Call `bootPreview.cancel()` to invalidate pending loads. Track the owner of queued frames and of the last painted LCD image (`boot`, `reel`, or `live`); discard only a queued `boot` frame. If the LCD still belongs to `boot`, hand it back by the next paint: use a queued normal frame first, otherwise refresh the available pre-power reel, otherwise fill the existing canvas black and clear its visual LED classes, matching the existing no-reel/off baseline. Dropping a queued frame alone does not erase an already painted canvas. Once a live frame owns the LCD, Escape must neither blank it nor discard a queued Worklet frame. Cancelling a pending load that never painted must leave the current normal screen alone. Do not allocate another framebuffer for this handoff.
- [ ] Make a requested reel refresh work even when its target equals `reelPos`: retain the last reel LED/key-LED metadata and repaint the existing `reelFb` without replaying `reel.apply()` or encoder `spin()` side effects. A later target still follows the existing incremental path. Do not reset `reelPos` to force reconstruction, since this would replay historical encoder deltas. Consume the refresh request once; the ordinary URL and subsequent normal paint loop retain their existing behavior.
- [ ] Add `tests/sim_boot_preview.mjs` using `node:assert/strict` with a fake clock and deferred loader. Test these exact sequences: 0→60→400 holds READY; reduced mode draws READY once; power-on/Escape cancellation while load is pending prevents a late frame; unsupported export returns false and leaves the reel available; canceled controller never emits again; reducing during playback or loading ends on READY; default URL creates no preview instance. The loading-pending `.reduce()` test catches stale use of the initial function argument: read the current `reduced` variable after `await`, as the controller does.
- [ ] Register `node tests/sim_boot_preview.mjs` alongside the existing `node tests/sim_font.mjs` in `tools/build_sim.sh`, before its `--host-only` exit. Extend the real WASM `tests/sim_glue.mjs` to verify the C export in both configurations; fake-controller tests do not establish C rendering correctness. Extend `tests/grind/browser_smoke.mjs` using its current Playwright setup. Read the expected build mode from `GRIND_NATIVE` (default `0`), assert that the served WASM exports `sim_boot_preview` in both modes, and check its supported/unsupported return on an isolated instance against that mode before selecting cases. Missing or unsupported functionality must fail the GRIND-on run, not silently skip it. The off run covers fallback/default behavior. The on run covers visible `?boot=1` playback, reduced-motion READY, reel priority, Escape with the reel response deliberately failed, cancellation at an unchanged reel target without extra encoder spins, power-on during load/motion, and Escape after a live frame. For the no-reel regression, request READY through reduced motion, make `reel.bin.gz` return 404, confirm the scene, then assert opaque-black canvas pixels and cleared visual LED classes after Escape by the next paint. Assert rendered canvas/LED behavior as well as cancellation of future emissions.
- [ ] Run:

```sh
node tests/sim_boot_preview.mjs
GRIND_NATIVE=0 bash tools/build_sim.sh --host-only
GRIND_NATIVE=1 bash tools/build_sim.sh --host-only
GRIND_NATIVE=0 bash tools/build_sim.sh --preview
GRIND_NATIVE=0 node tests/grind/browser_smoke.mjs
GRIND_NATIVE=1 bash tools/build_sim.sh --preview
GRIND_NATIVE=1 node tests/grind/browser_smoke.mjs
```

The host-only script still requires Node for its existing font checks. The preview builds require the repository's configured Emscripten/Docker environment. Run each browser smoke immediately after its matching preview build because both modes write `build/sim/preview`; use the existing Chromium/Playwright environment with `GRIND_CHROMIUM` set as in the current WASM lane. The default lane alone exercises an off build and cannot establish enabled-preview coverage. Inspect `?boot=1` and the ordinary URL in the browser, and label the result as a scene preview, not a measurement of actual boot timing. Commit `feat(sim): preview BLAST IGNITION through the shared C renderer`.

## 8. BA06 — Integrate existing checks and document actual results

**Files:** existing `docs/grind/acceptance.md`, `docs/manual/15-simulator.md`, `CHANGELOG.md`; only implementation/test files requiring a demonstrated correction from BA01–BA05. No new workflow or release checker.

- [ ] Run `sh tests/run_grind_tests.sh all` after every new suite is registered. Ensure the portable `boot_anim` suite runs in the existing WASM-core lane as part of `all`; host-only fixtures explicitly say so. Preserve the existing off-build title tests, graphics/encoder/UI/boot/update/restore checks and their assertions.
- [ ] Use the existing CI entry points in their configured environments, retaining their actual exit status and evidence paths:

```sh
bash tools/grind/run_ci.sh host-core
bash tools/grind/run_ci.sh host-upstream
bash tools/grind/run_ci.sh wasm
GRIND_NATIVE=1 bash tools/grind/run_ci.sh native
```

These commands are the repository's current lanes, not a new requirement to run unavailable toolchains locally. The native lane needs its pinned vendor archive and SDK supplied as already documented in `.github/workflows/grind.yml`. The normal WASM/web lane may use the default build, so retain BA05's explicit GRIND-on preview evidence as well. Use existing resource reports for actual image/RAM/pool changes; preserve the inherited pool floor. Do not reuse historical image sizes as measurements of this implementation.

- [ ] On a full checkout with the approved Hortator ancestor available, run `python3 tools/check_untouched.py --available-reference`. Inspect the same protected paths, including the checker and its pin, against the full implementation base recorded before BA01. All three comparisons below must be empty; checking only an unqualified `git diff` would miss committed and staged changes. The separate index comparison also detects a staged edit hidden by a restored working copy.

```sh
: "${GM_BOOT_BASE:?Use the full pre-implementation SHA recorded in section 1}"
git diff --exit-code "$GM_BOOT_BASE" HEAD -- \
    firmware/src/main.c firmware/hal firmware/loader \
    tools/check_untouched.py tools/frozen_base.txt
git diff --cached --exit-code "$GM_BOOT_BASE" -- \
    firmware/src/main.c firmware/hal firmware/loader \
    tools/check_untouched.py tools/frozen_base.txt
git diff --exit-code "$GM_BOOT_BASE" -- \
    firmware/src/main.c firmware/hal firmware/loader \
    tools/check_untouched.py tools/frozen_base.txt
```

This corrects the existing scope inspection; it adds no workflow or approval criterion. Keep historical comparison and optional performance statuses governed by the existing policy; never manufacture a missing tag, disable an old check or convert unavailable measurements to PASS.
- [ ] Verify native-off behavior via the old title fixture and both simulator build modes; native-on via the new host cases and actual target link. Preserve the `ui_draw` attribute, native USB identity and package names. A new allocation must fit the existing target check, not a newly invented animation percentage or byte reserve.
- [ ] During the project's already authorized device validation, observe the title, unoperated sequence, first PLAY/note/encoder action, SAFE START and calibration return; verify ordinary update/recovery through the existing procedure. Record tested hardware, commit and observed behavior only after those actions occur. This plan does not flash a device or make missing device evidence a prerequisite for merging these planning documents.
- [ ] Update the manual with the real timing distinction (static title during initialization, 400 ms interruptible presentation afterward), pass-through input, LOADED deferral and `?boot=1` scene preview. Record source commit, completed tests, skipped/unavailable instrumentation and actual native resources in the existing evidence documents. A statement such as “native animation tested on hardware” requires a real device result.
- [ ] Review exact diff scope, run `git diff --check`, commit `docs(grind): document BLAST IGNITION behavior and evidence`, and open the implementation PR with test results. This does not authorize merging, flashing or publishing a release.

## 9. Traceability and handoff

| Specification | Task |
|---|---|
| Timing clarification; protected existing boot | BA01, BA03, BA06 |
| Pixel geometry, palette, masks, typography | BA02 |
| State transitions and immediate normal input | BA01, BA03 |
| LOADED lifetime and error/calibration/update precedence | BA04 |
| Canvas/DMA ownership, dirty-window equivalence | BA02, BA06 |
| Native-off compatibility and no persistent settings | BA03, BA06 |
| Same-C preview, default simulator unchanged, reduced motion | BA05 |
| Existing community checks and honest evidence | BA06 |

BA01 is independent portable work. BA02 follows its sample contract. BA03 integrates their implementation; BA04 completes the real boot/update behavior before the feature is considered integrated. BA05 can proceed after the renderer contract stabilizes, with one integrator for shared include/test files. BA06 records actual integrated results. None of these tasks depends on merging the separate DNA ENGINE or community expansion PR.

The planning PR contains this plan, its specification, storyboard and the storyboard's entry in the existing asset-license inventory. It changes no executable firmware, CI configuration or release criterion. Implementation remains a subsequent code change following these six tasks.
