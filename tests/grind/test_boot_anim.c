/* SPDX-License-Identifier: GPL-3.0-only */
/* BA01: the deterministic BLAST IGNITION sample and its one-shot clock. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "boot_anim.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

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
#define NCASE (sizeof cases / sizeof cases[0])

static void rows(void)
{
    unsigned i;
    for (i = 0; i < NCASE; i++) {
        gm_boot_frame f = gm_boot_sample(cases[i][0]), g = gm_boot_sample(cases[i][0]);
        CHECK(f.phase == cases[i][1] && f.revealed == cases[i][2] && f.cursor == cases[i][3] && f.impact == cases[i][4]);
        CHECK(gm_boot_key(f) == gm_boot_key(g));                  /* repeated equal calls */
    }
    for (i = NCASE; i-- > 0;) {                                    /* reverse order: no hidden state */
        gm_boot_frame f = gm_boot_sample(cases[i][0]);
        CHECK(f.phase == cases[i][1] && f.revealed == cases[i][2] && f.cursor == cases[i][3] && f.impact == cases[i][4]);
    }
}

static void lifecycle(void)
{
    gm_boot_clock c = {0};
    gm_boot_frame f;
    CHECK(!gm_boot_step(&c, 10u, 0, &f) && c.state == GM_BOOT_OFF);          /* OFF never starts */
    gm_boot_arm(&c);
    CHECK(c.state == GM_BOOT_PENDING);
    CHECK(gm_boot_step(&c, 1000u, 0, &f) && c.state == GM_BOOT_ACTIVE && f.phase == GM_BOOT_IGNITION && f.revealed == 0);
    CHECK(gm_boot_step(&c, 1060u, 0, &f) && f.revealed == 8);
    gm_boot_cancel(&c);
    CHECK(c.state == GM_BOOT_DONE && !gm_boot_step(&c, 1061u, 0, &f));       /* cancellation is terminal */
    gm_boot_arm(&c);
    CHECK(c.state == GM_BOOT_DONE);                                          /* rearming DONE does nothing */

    c.state = GM_BOOT_OFF;
    gm_boot_arm(&c);
    CHECK(!gm_boot_step(&c, 5u, 1, &f) && c.state == GM_BOOT_DONE);          /* blocked PENDING never starts */

    c.state = GM_BOOT_OFF;
    gm_boot_arm(&c);
    CHECK(gm_boot_step(&c, 100u, 0, &f));
    CHECK(!gm_boot_step(&c, 100u + 5000u, 0, &f) && c.state == GM_BOOT_DONE); /* a skipped update past the deadline */
}

static void wrap(void)
{
    gm_boot_clock c = {0};
    gm_boot_frame f;
    gm_boot_arm(&c);
    CHECK(gm_boot_step(&c, UINT32_MAX - 100u, 0, &f));
    CHECK(gm_boot_step(&c, 298u, 0, &f));                                    /* elapsed 399 across the wrap */
    CHECK(f.phase == GM_BOOT_READY);
    CHECK(!gm_boot_step(&c, 299u, 0, &f));                                   /* elapsed 400 */
    CHECK(c.state == GM_BOOT_DONE);
    gm_boot_arm(&c);
    CHECK(!gm_boot_step(&c, 300u, 0, &f));
}

int main(void)
{
    rows();
    lifecycle();
    wrap();
    printf(fails ? "test_boot_anim: %d FAILED\n" : "test_boot_anim: all passed\n", fails);
    return fails != 0;
}
