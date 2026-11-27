/* SPDX-License-Identifier: GPL-3.0-only */
/* BLAST IGNITION in the UI loop (docs/superpowers/specs/2026-11-27-grind-blast-ignition-design.md): the title hook draws
 * SYSTEM WAKE (main.c clears it, unchanged); the first eligible ordinary ui_draw() starts the moving 400 ms presentation;
 * any input, dialog, message, recovery or update path cancels it and the native UI continues at once. Included once, after
 * the UI, project and editor definitions (felucca.c; tests/ui_host.h, tests/sim_host.h). GRIND_NATIVE only. */
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
