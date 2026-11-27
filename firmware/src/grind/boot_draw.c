/* SPDX-License-Identifier: GPL-3.0-only */
/* BLAST IGNITION scene (docs/superpowers/specs/2026-11-27-grind-blast-ignition-design.md): the title card and an eight-track,
 * sixteen-step motif drawn with the existing canvas (cv_*), the MD_F3 font and a local five-colour palette. Included once
 * from boot_ui.c (firmware) or from a test fixture after the graphics; nothing here touches the stored theme. */
#include "boot_anim.h"
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
