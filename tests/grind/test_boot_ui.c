/* SPDX-License-Identifier: GPL-3.0-only */
/* BA03: the one-shot BLAST IGNITION presentation inside the real UI loop: eligibility, input priority, cleanup. */
#undef NDEBUG
#include <assert.h>
#include "../ui_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

#if GRIND_NATIVE
#define PX(x, y) fb[(y) * 240u + (x)]

/* main.c's order: the title, (the inherited waits), the black fill, then the ordinary UI loop */
static void model_boot(int armed)
{
    ui_host_init();
    gm_boot_ui_reset();
    fm1_ms = 900u;
    if (armed)
        draw_boot_title();
    else
        gm_boot_ui.clock.state = GM_BOOT_DONE;
    lcd_fill(0, 0, 240, 240, C_BG);
}
static void at(uint32_t ms)
{
    fm1_ms = ms;
    ui_frame();
}

static void natural_run(void)
{
    static uint16_t base[240 * 240];
    uint32_t ok = 1, frame0;
    model_boot(0);
    at(1000u);
    at(1400u);
    at(1500u);
    memcpy(base, fb, sizeof fb);                      /* the unarmed baseline: what the native UI draws */
    model_boot(1);
    ok &= gm_boot_ui.clock.state == GM_BOOT_PENDING;
    frame0 = ui.frame;
    at(1000u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_ACTIVE && PX(25, 138) == 0x30C3 && ui.frame == frame0;   /* the scene, not the UI */
    at(1120u);
    ok &= PX(44, 64) != 0 && gm_boot_ui.previous.phase == GM_BOOT_ATTACK;
    at(1280u);
    ok &= gm_boot_ui.previous.phase == GM_BOOT_READY;
    at(1400u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE;
    at(1500u);
    ok &= memcmp(base, fb, sizeof fb) == 0;           /* completion restores the full native framebuffer */
    at(1600u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE && ui.frame > frame0;   /* and it cannot restart */
    check("a natural run: WAKE, the scene on the first ordinary draw, then the native UI exactly as unarmed", ok);
}

static void input_cancels_and_counts_once(void)
{
    uint32_t ok = 1, fills, before, shown_base, shown_armed;
    model_boot(0);
    before = ui.home;
    press(B_EDIT);
    at(1000u);
    release_all();
    at(1010u);
    shown_base = ui.home;                              /* EDIT leaves HOME: observable, once */
    ok &= shown_base != before;
    model_boot(1);
    fills = host_fill_px + host_blit_px;
    press(B_EDIT);
    at(1000u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE;      /* the first input ends it ... */
    release_all();
    at(1010u);
    shown_armed = ui.home;
    ok &= shown_armed == shown_base;                   /* ... and acts exactly once, as without the presentation */
    ok &= host_blit_px + host_fill_px >= fills;
    check("the first button press cancels the presentation and plays as without it", ok);

    model_boot(0);
    turn(EN_K1, 1);
    at(1000u);
    {
        int16_t base_g[G_COUNT];
        memcpy(base_g, song.g, sizeof base_g);
        model_boot(1);
        turn(EN_K1, 1);
        at(1000u);
        ok = gm_boot_ui.clock.state == GM_BOOT_DONE && memcmp(base_g, song.g, sizeof base_g) == 0;
    }
    check("an encoder step cancels it and is applied once, as without it", ok);
}

static void boot_loaded_note_keeps_it(void)
{
    uint32_t ok = 1;
    model_boot(1);                                     /* project.c: the power-on load says LOADED, then defers it */
    ui_say("LOADED", "");
    gm_boot_ui_defer_loaded();
    at(1000u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_ACTIVE && PX(25, 138) == 0x30C3;
    model_boot(1);                                     /* any other message (no deferral) still cancels */
    ui_say("SOMETHING", "");
    at(1000u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE;
    check("the boot's own LOADED note keeps the presentation; another message cancels it", ok);
}

static void pending_cancel_adds_no_pixels(void)
{
    uint32_t ok = 1, px, wins;
    model_boot(1);
    px = host_fill_px + host_blit_px;
    wins = host_wins;
    gm_boot_ui_cancel();
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE && px == host_fill_px + host_blit_px && wins == host_wins;
    check("cancelling PENDING adds no LCD traffic (main.c already cleared the title)", ok);
}

static void skipped_frames_and_wrap(void)
{
    uint32_t ok = 1, fills;
    model_boot(1);
    at(1000u);
    fills = host_fill_px;
    at(6000u);                                        /* the loop stalled past the deadline */
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE && host_fill_px > fills && ui.force != 2;
    check("a skipped update past the deadline ends it with one cleanup", ok);

    ok = 1;
    model_boot(1);
    at(0xFFFFFF00u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_ACTIVE;
    at(0xFFFFFF00u + 399u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_ACTIVE && gm_boot_ui.previous.phase == GM_BOOT_READY;
    at(0xFFFFFF00u + 400u);
    ok &= gm_boot_ui.clock.state == GM_BOOT_DONE;
    check("a millisecond counter wrap does not change the 400 ms", ok);
}

static void keeps_native_state(void)
{
    uint32_t ok = 1, page, home, theme;
    model_boot(0);
    at(1000u);
    page = ui.page;
    home = ui.home;
    theme = settings.palette;
    model_boot(1);
    at(1000u);
    at(1100u);
    at(1500u);
    ok &= ui.page == page && ui.home == home && settings.palette == theme;
    check("page, HOME and theme are what they were", ok);
}
#endif

static void off_build(void)
{
    uint32_t ok = 1, fills;
    ui_host_init();
    draw_boot_title();                                 /* the existing HORTATOR title */
    fills = host_wins;
    ok &= fills > 0;
    check("GRIND_NATIVE=0: the existing title and nothing else", ok);
}

int main(void)
{
    off_build();
#if GRIND_NATIVE
    natural_run();
    input_cancels_and_counts_once();
    boot_loaded_note_keeps_it();
    pending_cancel_adds_no_pixels();
    skipped_frames_and_wrap();
    keeps_native_state();
#endif
    printf(fails ? "test_boot_ui: %d FAILED\n" : "test_boot_ui: all passed\n", fails);
    return fails != 0;
}
