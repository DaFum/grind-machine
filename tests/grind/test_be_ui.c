/* SPDX-License-Identifier: GPL-3.0-only */
/* BE01 T04: the LOCKS page through the real UI (panel buttons, encoders, frames): per-track layer locks and MUT. */
#define GRIND_NATIVE 1
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

static void tap(uint32_t b)
{
    press(b);
    ui_frame();
    release_all();
    ui_frame();
}
static void frames(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        ui_frame();
}
static void fresh(void)
{
    gm_pattern p;
    ui_host_init();
    gm_runtime_reset();
    song.g[G_BPM] = 240;
    assert(gm_factory_pattern(1, &p) == GM_OK && gm_native_publish(&p) == GM_OK);
    gm_runtime_reset();
}
static int on_locks(void)
{
    return cur_page()->scope == SC_GRIND && cur_page()->id[0] == GU_LRHY;
}
static void open_locks(void)
{
    uint32_t i;
    for (i = 0; i < 12u && !on_locks(); i++)
        tap(B_ARP);
}
static uint32_t track_sig(uint32_t k)
{
    uint32_t h = 2166136261u, s;
    const uint8_t *b = (const uint8_t *)trk[k].step;
    for (s = 0; s < sizeof trk[k].step; s++)
        h = (h ^ b[s]) * 16777619u;
    return h;
}

static void navigation(void)
{
    static const uint32_t OLD[5] = { GU_STYLE, GU_HUMAN, GU_KEEP, GU_SINK, GU_BROWSE };
    uint32_t ok = 1, i;
    fresh();
    for (i = 0; i < 12u && !(cur_page()->scope == SC_GRIND); i++)
        tap(B_ARP);
    for (i = 0; i < 5u; i++) {                                    /* the old pages, in the old order */
        ok &= cur_page()->scope == SC_GRIND && cur_page()->id[0] == OLD[i];
        tap(B_ARP);
    }
    ok &= on_locks() && cur_page()->id[1] == GU_LTIME && cur_page()->id[2] == GU_LTMBR && cur_page()->id[3] == GU_LMUT;
    check("ui: the LOCKS page (RHY TIME TMBR MUT) comes after the five old GRIND pages, which keep their order", ok);
}

static void locks_are_per_track(void)
{
    uint32_t ok = 1;
    fresh();
    song.sel = 2;
    open_locks();
    frames(2);
    ok &= on_locks() && gm_v[GU_LRHY] == 0;
    turn(EN_K1, 1);
    frames(2);
    ok &= be_act[2].locks == BE_RHY && be_act[3].locks == 0 && be_act[0].locks == 0 && gm_v[GU_LRHY] == 1;
    song.sel = 3;
    frames(2);
    ok &= gm_v[GU_LRHY] == 0 && be_act[3].locks == 0;             /* the page shows the selected track's own locks */
    turn(EN_K2, 1);
    frames(2);
    ok &= be_act[3].locks == BE_TIME && be_act[2].locks == BE_RHY;
    song.sel = 2;
    frames(2);
    ok &= gm_v[GU_LRHY] == 1 && gm_v[GU_LTIME] == 0;
    turn(EN_K1, -1);
    frames(2);
    ok &= be_act[2].locks == 0;
    check("ui: a layer lock belongs to the selected track; the page follows the selection", ok);
}

static void mut_runs_the_free_layers(void)
{
    uint32_t ok = 1, before, others[NTRK], k, commits;
    fresh();
    song.sel = 2;
    open_locks();
    frames(2);
    for (k = 0; k < NTRK; k++)
        others[k] = track_sig(k);
    before = track_sig(2);
    commits = gm_runtime_commits();
    turn(EN_K4, 1);                                               /* MUT: right detent only */
    frames(4);
    ok &= gm_runtime_commits() == commits + 1u && track_sig(2) != before && gm_v[GU_LMUT] == 0;
    for (k = 0; k < NTRK; k++)
        if (k != 2u)
            ok &= track_sig(k) == others[k];
    ok &= str_eq(ui.msg, "MUTATED");
    commits = gm_runtime_commits();
    before = track_sig(2);
    turn(EN_K4, -1);                                              /* the left detent is no action */
    frames(4);
    ok &= gm_runtime_commits() == commits && track_sig(2) == before;
    check("ui: MUT mutates only the selected track's free layers (right detent); the left detent does nothing", ok);
}

static void all_locked_and_unavailable_timbre(void)
{
    uint32_t ok = 1, before, commits;
    fresh();
    song.sel = 2;
    open_locks();
    frames(2);
    turn(EN_K1, 1);
    turn(EN_K2, 1);
    turn(EN_K3, 1);
    frames(2);
    ok &= be_act[2].locks == BE_ALL;
    before = track_sig(2);
    commits = gm_runtime_commits();
    turn(EN_K4, 1);
    frames(4);
    ok &= str_eq(ui.msg, "LOCKED") && !gmrt.ctl.has_pending && gm_runtime_commits() == commits && track_sig(2) == before;
    check("ui: all layers locked says LOCKED and queues nothing", ok);

    be_act[2].locks = BE_RHY | BE_TIME;                           /* only the timbre layer is free: it is not there yet */
    frames(2);
    turn(EN_K4, 1);
    frames(4);
    ok = !str_eq(ui.msg, "MUTATED") && !gmrt.ctl.has_pending && gm_runtime_commits() == commits && track_sig(2) == before;
    check("ui: only the (not yet available) timbre layer free: no request, no false success", ok);
}

static void held_step_never_runs_mut(void)
{
    uint32_t ok = 1, commits, before;
    fresh();
    song.sel = 2;
    open_locks();
    frames(2);
    commits = gm_runtime_commits();
    before = track_sig(2);
    ui.env = 1;                                                   /* STEPS: the white keys are the selected track's steps */
    keys(1u << STEP_KEY[2]);                                      /* a step is held: the knobs belong to its locks */
    frames(1);
    turn(EN_K4, 1);
    for (uint32_t i = 0; i < STEP_HOLD_MS + 60u; i++)             /* a long hold (on a GRIND page: KEEP), never a tap that toggles the step */
        ui_frame();
    keys(0);
    frames(2);
    ok &= gm_runtime_commits() == commits && !gmrt.ctl.has_pending && track_sig(2) == before && gm_v[GU_LMUT] == 0;
    check("ui: a turn with a step held never invokes MUT", ok);
}

int main(void)
{
    navigation();
    locks_are_per_track();
    mut_runs_the_free_layers();
    all_locked_and_unavailable_timbre();
    held_step_never_runs_mut();
    printf(fails ? "test_be_ui: %d FAILED\n" : "test_be_ui: all passed\n", fails);
    return fails != 0;
}
