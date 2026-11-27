/* SPDX-License-Identifier: GPL-3.0-only */
/* BA02: the BLAST IGNITION scene on the host framebuffer. Built with GRIND_NATIVE=0 (the existing title only) and =1. */
#undef NDEBUG
#include <assert.h>
#include "../ui_host.h"
#if GRIND_NATIVE
#include "boot_anim.c"
#include "boot_draw.c"
#endif

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
static uint16_t pal(uint32_t i)
{
    static const uint16_t p[5] = {RGB(8, 8, 9), RGB(49, 26, 29), RGB(188, 37, 45), RGB(244, 76, 81), RGB(246, 240, 239)};
    return p[i];
}
static void clear_fb(void) { memset(fb, 0, sizeof fb); }
static void full_at(uint32_t t)
{
    gm_boot_frame f = gm_boot_sample(t > 399u ? 399u : t);
    if (t == 0u && 0)
        f.phase = GM_BOOT_WAKE;
    gm_boot_draw_full(f);
}
static void wake_full(void)
{
    gm_boot_frame f = {GM_BOOT_WAKE, 0, GM_BOOT_NO_COLUMN, 0};
    gm_boot_draw_full(f);
}

static void probes(void)
{
    uint32_t ok = 1;
    int32_t w8 = md_text_w(&MD_F3, 8, "GRIND"), w4 = md_text_w(&MD_F3, 4, "MACHINE"), w2 = md_text_w(&MD_F3, 2, "FM-1 EDITION");
    clear_fb();
    wake_full();
    ok &= PX(0, 0) == 0x0841 && PX(8, 8) == 0x30C3 && PX(44, 44) == 0xB925;
    check("wake: the dark card, its frame and the red title", ok);
    clear_fb();
    full_at(0);
    check("0 ms: nothing revealed yet", PX(25, 138) == 0x30C3);
    clear_fb();
    full_at(60);
    check("60 ms: the first columns lit, the cursor hot", PX(25, 138) == 0xB925 && PX(121, 131) == 0xF26A);
    clear_fb();
    full_at(199);
    check("199 ms: the strike's line is red", PX(44, 64) == 0xB925);
    clear_fb();
    full_at(200);
    check("200 ms: the strike lines hand over, the title is paper", PX(44, 64) == 0xF79D && PX(121, 138) == 0xF26A && PX(121, 145) == 0x30C3);
    clear_fb();
    full_at(340);
    check("340 ms: the card holds", PX(66, 96) == 0xF79D && PX(24, 124) == 0xB925 && PX(121, 131) == 0x0841);
    check("text widths 152, 108 and 94", w8 == 152 && w4 == 108 && w2 == 94);
}

static void palette_only(void)
{
    uint32_t t, i, ok = 1, off = 0;
    for (t = 0; t < 400u; t += 20u) {
        clear_fb();
        full_at(t);
        for (i = 0; i < 240u * 240u; i++) {
            uint32_t k, hit = 0;
            for (k = 0; k < 5u; k++)
                hit |= fb[i] == pal(k);
            if (!hit)
                off++;
        }
    }
    ok &= off == 0;
    check("every pixel belongs to the five-entry local palette", ok);
}

static void incremental_equals_full(void)
{
    static const uint32_t seq[] = {0, 60, 119, 120, 199, 200, 279, 280, 399};
    static const uint32_t skip[][2] = {{0, 280}, {120, 399}};
    static uint16_t expected[240 * 240];
    uint32_t i, ok = 1;
    for (i = 1; i < sizeof seq / sizeof seq[0]; i++) {
        gm_boot_frame a = gm_boot_sample(seq[i - 1]), b = gm_boot_sample(seq[i]);
        clear_fb();
        gm_boot_draw_full(b);
        memcpy(expected, fb, sizeof fb);
        clear_fb();
        gm_boot_draw_full(a);
        gm_boot_draw_changed(a, b);
        ok &= memcmp(expected, fb, sizeof fb) == 0;
    }
    for (i = 0; i < 2u; i++) {
        gm_boot_frame a = gm_boot_sample(skip[i][0]), b = gm_boot_sample(skip[i][1]);
        clear_fb();
        gm_boot_draw_full(b);
        memcpy(expected, fb, sizeof fb);
        clear_fb();
        gm_boot_draw_full(a);
        gm_boot_draw_changed(a, b);
        ok &= memcmp(expected, fb, sizeof fb) == 0;
    }
    check("a dirty update equals a full redraw (steps and skipped samples)", ok);
    {
        gm_boot_frame a = gm_boot_sample(60);
        uint32_t px = host_blit_px, wins = host_wins;
        gm_boot_draw_changed(a, a);
        check("an identical sample sends nothing", host_blit_px == px && host_wins == wins);
        px = host_blit_px;
        gm_boot_draw_changed(gm_boot_sample(0), gm_boot_sample(30));
        check("a dirty update is a bounded window, not the screen", host_blit_px - px <= 192u * 70u && host_blit_px > px);
    }
}

static void theme_untouched(void)
{
    uint32_t th, ok = 1;
    for (th = 0; th < (uint32_t)NTHEMES; th++) {
        uint16_t bg, ink;
        uint32_t stored;
        theme_set(THEME_TAG | th);
        bg = C_BG;
        ink = C_INK;
        stored = settings.palette;
        full_at(100);
        ok &= C_BG == bg && C_INK == ink && settings.palette == stored;
    }
    theme_set(THEME_TAG | 0u);
    check("the stored theme and C_BG / C_INK are not changed by the scene", ok);
}
#endif

static void off_build_has_the_old_title(void)
{
    uint32_t ok = 1;
    ui_host_init();
    draw_boot_title();
    ok &= host_wins > 0;
    check("the existing title still draws", ok);
}

int main(void)
{
    off_build_has_the_old_title();
#if GRIND_NATIVE
    probes();
    palette_only();
    incremental_equals_full();
    theme_untouched();
#endif
    printf(fails ? "test_boot_draw: %d FAILED\n" : "test_boot_draw: all passed\n", fails);
    return fails != 0;
}
