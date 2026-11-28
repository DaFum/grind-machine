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
    gm_v[GU_STYLE] = 0;                                           /* (screen state outlives a test: start from style 1's panel) */
    gm_v[GU_AGGR] = 75;
    gm_v[GU_DENS] = 65;
    gm_v[GU_CHAOS] = 20;
    gm_v[GU_HUMAN] = 25;
    gm_v[GU_MUT] = 0;
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

static void mutation_works_straight_after_boot(void)
{
    gm_pattern p;
    uint32_t ok = 1, before;
    ui_host_init();
    memset(&gmrt, 0, sizeof gmrt);                                /* power-on: the runtime has never been reset */
    drum_boot_init();
    assert(gm_factory_pattern(1, &p) == GM_OK && gm_native_publish(&p) == GM_OK);
    before = track_sig(2);
    ok &= be_submit_cfg(2, BE_TIME, &gmrt.ctl.active_config) == GM_OK;                         /* the config MUT sends is a valid one, not all zeros */
    gm_runtime_poll();
    ok &= gm_runtime_commits() == 1u && track_sig(2) != before;
    check("MUT works right after power-on, before any other GRIND request", ok);
}

static void mutation_follows_the_panel_style(void)
{
    gm_config c;
    gm_pattern p;
    uint32_t ok = 1, before;
    ui_host_init();
    memset(&gmrt, 0, sizeof gmrt);
    drum_boot_init();                                             /* power-on: the controller holds style 1's config */
    gm_v[GU_STYLE] = (int16_t)GM_ALGORITHM_COUNT;                 /* the restored panel style: MATH, 7/4 over 28 steps */
    assert(gm_ui_style_config((uint32_t)gm_v[GU_STYLE], &c) == GM_OK && gm_generate(&c, NULL, &p) == GM_OK &&
           gm_native_publish(&p) == GM_OK);                       /* a restored project on that grid */
    song.sel = 0;                                                 /* the kick: MATH has no hi-hat to mutate */
    open_locks();
    frames(2);
    before = track_sig(0);
    turn(EN_K4, 1);
    frames(4);
    ok &= gm_runtime_commits() == 1u && track_sig(0) != before;   /* MUT uses the style the panel shows, not style 1 */
    check("MUT after a reboot with a non-default STYLE follows the panel's style", ok);
}

static void mutation_after_undone_style(void)
{
    uint32_t ok = 1, before, commits, len_a;
    fresh();
    song.sel = 0;
    gm_v[GU_STYLE] = 3;                                           /* a style of its own: its config is now the live pattern's */
    gm_ui_changed(GU_STYLE);
    frames(3);
    len_a = (uint32_t)trk[0].p[P_SLEN];
    gm_v[GU_STYLE] = (int16_t)GM_ALGORITHM_COUNT;                 /* MATH: 7/4 over 28 steps */
    gm_ui_changed(GU_STYLE);
    frames(3);
    ok &= gm_runtime_commits() == 2u && trk[0].p[P_SLEN] == 28;
    gm_v[GU_UNDO] = 1;
    gm_ui_changed(GU_UNDO);                                       /* the second style undone: the first one's pattern is live again */
    frames(2);
    ok &= (uint32_t)trk[0].p[P_SLEN] == len_a && gm_v[GU_STYLE] == 3;   /* the panel shows the style that is live */
    open_locks();
    frames(2);
    before = track_sig(0);
    commits = gm_runtime_commits();
    turn(EN_K4, 1);
    frames(4);
    ok &= gm_runtime_commits() == commits + 1u && track_sig(0) != before && (uint32_t)trk[0].p[P_SLEN] == len_a;
    check("MUT after an undone STYLE change mutates on the restored grid", ok);
}

static void undo_of_a_mutation_counter_alone(void)
{
    uint32_t ok = 1;
    fresh();
    gm_v[GU_MUT] = 3;
    gm_ui_changed(GU_MUT);                                        /* config A: mutation 3 */
    frames(3);
    gm_v[GU_MUT] = 5;
    gm_ui_changed(GU_MUT);                                        /* config B differs from A in the counter alone */
    frames(3);
    ok &= gmrt.ctl.active_config.mutation == 5u && gm_runtime_commits() == 2u;
    gm_v[GU_UNDO] = 1;
    gm_ui_changed(GU_UNDO);
    ok &= gm_v[GU_MUT] == 3;                                      /* the panel shows the counter of the live pattern */
    check("undoing a change of the MUT counter alone restores the counter on the panel", ok);
}

static void editor_undo_puts_a_cancelled_style_back(void)
{
    uint32_t ok = 1;
    fresh();
    song.sel = 0;
    gm_v[GU_STYLE] = 3;
    gm_ui_changed(GU_STYLE);                                      /* style A is live and known */
    frames(3);
    open_locks();
    frames(2);
    turn(EN_K4, 1);                                               /* a mutation under A: the undo has a snapshot of A's config */
    frames(4);
    ok &= gm_runtime_commits() == 2u && be_cfg_known;
    song.playing = 1;                                             /* B is turned while playing: it waits for the END */
    gm_v[GU_STYLE] = 5;
    gm_ui_changed(GU_STYLE);
    ok &= gm_v[GU_STYLE] == 5;
    gm_save_t = 0;
    ok &= gm_ui_undo() == 1;                                      /* what the panel's UNDO and the USB editor's UNDO both call */
    ok &= gm_v[GU_STYLE] == 3 && gm_save_t != 0;                  /* the style that never played is put back and remembered */
    song.playing = 0;
    check("UNDO puts a style turn that never played back on the panel (shared by the panel and the USB editor)", ok);
}

static void mutation_clears_a_stale_result(void)
{
    uint32_t ok = 1, commits;
    fresh();
    song.sel = 0;
    open_locks();
    frames(2);
    gm_last_result = GM_STALE;                                    /* an earlier request was dropped: the USB editor still says NOT APPLIED */
    commits = gm_runtime_commits();
    turn(EN_K4, 1);
    frames(4);
    ok &= gm_runtime_commits() == commits + 1u && gm_last_result == GM_OK;
    check("a MUT that goes out clears a stale NOT APPLIED for the USB editor", ok);
}

static void undo_to_an_unknown_config_keeps_the_panel(void)
{
    gm_pattern p;
    gm_config c;
    uint32_t ok = 1, before;
    ui_host_init();
    memset(&gmrt, 0, sizeof gmrt);
    drum_boot_init();                                             /* power-on: the controller holds style 1's config, which made nothing */
    gm_v[GU_STYLE] = (int16_t)GM_ALGORITHM_COUNT;                 /* the restored panel style (MATH) and a project on its grid */
    assert(gm_ui_style_config((uint32_t)gm_v[GU_STYLE], &c) == GM_OK && gm_generate(&c, NULL, &p) == GM_OK &&
           gm_native_publish(&p) == GM_OK);
    gm_v[GU_AGGR] = 80;
    gm_ui_changed(GU_AGGR);                                       /* a MORPH commits: the config is known from here */
    frames(3);
    ok &= gm_runtime_commits() == 1u;
    gm_save_t = 0;
    gm_v[GU_UNDO] = 1;
    gm_ui_changed(GU_UNDO);                                       /* back to the loaded pattern, whose config nobody knows */
    ok &= gm_v[GU_STYLE] == (int16_t)GM_ALGORITHM_COUNT && gm_save_t == 0;   /* the panel and the saved style stay */
    song.sel = 0;
    open_locks();
    frames(2);
    before = track_sig(0);
    turn(EN_K4, 1);
    frames(4);
    ok &= gm_runtime_commits() == 2u && track_sig(0) != before;   /* MUT still follows the panel: the loaded 28-step grid */
    check("UNDO to a pattern of unknown origin leaves the panel and the saved style alone; MUT follows the panel", ok);
}

static void live_config_beats_an_uncommitted_panel(void)
{
    uint32_t ok = 1, before, commits;
    fresh();
    song.sel = 0;
    gm_v[GU_STYLE] = 3;
    gm_ui_changed(GU_STYLE);
    frames(3);
    gm_v[GU_STYLE] = (int16_t)GM_ALGORITHM_COUNT;                 /* a panel style that was never applied (a rejected turn) */
    open_locks();
    frames(2);
    before = track_sig(0);
    commits = gm_runtime_commits();
    turn(EN_K4, 1);
    frames(4);
    ok &= gm_runtime_commits() == commits + 1u && track_sig(0) != before && trk[0].p[P_SLEN] != 28;   /* the live pattern's style made it */
    ok &= gmrt.ctl.active_config.grid.num != 7u;
    check("MUT uses the live pattern's config, not an uncommitted panel style", ok);
}

static void undo_restores_counter_and_saves_style(void)
{
    uint32_t ok = 1;
    fresh();
    gm_v[GU_MUT] = 3;
    gm_ui_changed(GU_MUT);                                        /* the generator config now carries mutation 3 */
    frames(3);
    ok &= gmrt.ctl.active_config.mutation == 3u;
    gm_v[GU_STYLE] = 4;
    gm_ui_changed(GU_STYLE);                                      /* a style change resets the counter */
    frames(3);
    ok &= gm_v[GU_MUT] == 0;
    gm_save_t = 0;
    gm_v[GU_UNDO] = 1;
    gm_ui_changed(GU_UNDO);
    ok &= gm_v[GU_STYLE] == 0 && gm_v[GU_MUT] == 3;               /* the undone panel: style and counter of the live pattern */
    ok &= gm_save_t != 0;                                         /* and the style that is live again gets written */
    check("undoing a style change restores the panel's MUT counter and schedules the style save", ok);
}

int main(void)
{
    mutation_follows_the_panel_style();
    mutation_after_undone_style();
    undo_restores_counter_and_saves_style();
    undo_of_a_mutation_counter_alone();
    editor_undo_puts_a_cancelled_style_back();
    mutation_clears_a_stale_result();
    undo_to_an_unknown_config_keeps_the_panel();
    live_config_beats_an_uncommitted_panel();
    mutation_works_straight_after_boot();
    navigation();
    locks_are_per_track();
    mut_runs_the_free_layers();
    all_locked_and_unavailable_timbre();
    held_step_never_runs_mut();
    printf(fails ? "test_be_ui: %d FAILED\n" : "test_be_ui: all passed\n", fails);
    return fails != 0;
}
