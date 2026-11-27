/* SPDX-License-Identifier: GPL-3.0-only */
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
