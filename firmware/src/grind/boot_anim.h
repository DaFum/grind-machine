/* SPDX-License-Identifier: GPL-3.0-only */
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
