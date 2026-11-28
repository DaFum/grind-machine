/* SPDX-License-Identifier: GPL-3.0-only */
/* BE01 T02: scoped (per track, per layer) mutation published on the real native runtime and sequencer. */
#define GRIND_NATIVE 1
#undef NDEBUG
#include <assert.h>
/* The window between "undo observed a valid snapshot" and "undo takes the guard": a test runs real code in it. */
static void (*race_hook)(void);
#define GM_UNDO_RACE_HOOK() do { if (race_hook) race_hook(); } while (0)
#include "../drum_host.h"

static int fails;
static int check(const char *what, int ok)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        fails++;
    return ok;
}

typedef struct {
    step_t st[NTRK][NSTEP];
    int16_t len[NTRK], div[NTRK], swing[NTRK], src[NTRK];
} snap_t;

static void take(snap_t *s)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        memcpy(s->st[k], trk[k].step, sizeof s->st[k]);
        s->len[k] = trk[k].p[P_SLEN];
        s->div[k] = trk[k].p[P_SDIV];
        s->swing[k] = trk[k].p[P_SSWING];
        s->src[k] = trk[k].p[P_SRC];
    }
}
static int track_same(const snap_t *s, uint32_t k)
{
    return !memcmp(s->st[k], trk[k].step, sizeof s->st[k]) && s->len[k] == trk[k].p[P_SLEN] &&
           s->div[k] == trk[k].p[P_SDIV] && s->swing[k] == trk[k].p[P_SSWING] && s->src[k] == trk[k].p[P_SRC];
}
static int others_same(const snap_t *s, uint32_t except)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        if (k != except && !track_same(s, k))
            return 0;
    return 1;
}

#define SEL 2u
/* a layer mutation from the live config (no panel in this harness) */
static gm_result submit_live(uint8_t track, uint8_t layers) { return be_submit_cfg(track, layers, &gmrt.ctl.active_config); }
static void fresh(void)
{
    gm_pattern p;
    host_init();
    song.g[G_BPM] = 120;
    gm_runtime_reset();
    assert(gm_factory_pattern(1, &p) == GM_OK);
    assert(gm_native_publish(&p) == GM_OK);
    memset(&trk[SEL].step[NSTEP - 1], 0xA7, sizeof(step_t));     /* a dormant native byte pattern past LEN (if LEN < 64) */
    gm_runtime_reset();
}
static void settle(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++)
        gm_runtime_poll();
}

static void rhythm_only(void)
{
    snap_t before;
    uint32_t s, ok = 1, changed = 0, len;
    be_recipe old;
    fresh();
    take(&before);
    len = (uint32_t)trk[SEL].p[P_SLEN];
    old = be_act[SEL];
    ok &= submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    ok &= others_same(&before, SEL);                              /* no other track moves */
    ok &= trk[SEL].p[P_SLEN] == before.len[SEL] && trk[SEL].p[P_SDIV] == before.div[SEL] &&
          trk[SEL].p[P_SSWING] == before.swing[SEL] && trk[SEL].p[P_SRC] == before.src[SEL];
    for (s = 0; s < len; s++) {
        const step_t *a = &before.st[SEL][s], *b = &trk[SEL].step[s];
        ok &= a->micro == b->micro;                               /* RHY alone never touches the timing */
        changed |= a->on != b->on || a->acc != b->acc || a->slide != b->slide || a->tie != b->tie || a->rat != b->rat ||
                   a->cond != b->cond || a->nt != b->nt;
    }
    ok &= changed != 0u;                                          /* the rhythm really changed */
    ok &= len == NSTEP || !memcmp(&before.st[SEL][len], &trk[SEL].step[len], (NSTEP - len) * sizeof(step_t));   /* dormant bytes */
    ok &= be_act[SEL].key[0] == be_key_next(old.key[0]) && be_act[SEL].key[1] == old.key[1] &&
          be_act[SEL].key[2] == old.key[2];
    check("RHY only: only the selected track's rhythm fields change, timing and dormant bytes stay, only its key advances", ok);
}

static void time_only(void)
{
    snap_t before;
    uint32_t s, ok = 1, moved = 0, len;
    fresh();
    take(&before);
    len = (uint32_t)trk[SEL].p[P_SLEN];
    ok &= submit_live(SEL, BE_TIME) == GM_OK;
    settle();
    ok &= others_same(&before, SEL);
    for (s = 0; s < len; s++) {
        const step_t *a = &before.st[SEL][s], *b = &trk[SEL].step[s];
        ok &= a->on == b->on && a->acc == b->acc && a->slide == b->slide && a->tie == b->tie && a->rat == b->rat &&
              a->cond == b->cond && a->nt == b->nt;
        if (!a->on)
            ok &= a->micro == b->micro;                           /* an OFF cell keeps its dormant timing */
        moved |= a->micro != b->micro;
    }
    ok &= moved != 0u;
    ok &= len == NSTEP || !memcmp(&before.st[SEL][len], &trk[SEL].step[len], (NSTEP - len) * sizeof(step_t));
    check("TIME only: only the micro of the selected track's hits changes", ok);
}

static void locks_and_keep(void)
{
    snap_t before;
    uint32_t ok = 1, s;
    fresh();
    take(&before);
    be_act[SEL].locks = BE_ALL;
    ok &= submit_live(SEL, BE_ALL) == GM_NO_CHANGE;                 /* every layer locked: nothing to do, nothing queued */
    settle();
    ok &= others_same(&before, NTRK) && track_same(&before, SEL) && !gmrt.ctl.has_pending;
    be_act[SEL].locks = BE_RHY;                                   /* RHY locked, TIME free */
    ok &= submit_live(SEL, BE_ALL) == GM_OK;
    settle();
    for (s = 0; s < (uint32_t)before.len[SEL]; s++) {
        const step_t *a = &before.st[SEL][s], *b = &trk[SEL].step[s];
        ok &= a->on == b->on && a->nt == b->nt && a->cond == b->cond;   /* the locked layer is not touched */
    }
    check("a locked layer is not mutated; all layers locked is NO_CHANGE with nothing pending", ok);

    fresh();
    take(&before);
    fm1_irq_off();
    gm_keep_set(SEL, 3, 1);
    fm1_irq_on();
    ok = submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    settle();
    ok &= !memcmp(&before.st[SEL][3], &trk[SEL].step[3], sizeof(step_t));
    check("a KEEP cell stays exactly as it was through RHY and TIME", ok);
}

static void refusals_and_stale(void)
{
    snap_t before;
    uint32_t ok = 1;
    uint32_t key0;
    fresh();
    take(&before);
    key0 = be_act[SEL].key[0];
    ok &= submit_live(NTRK, BE_RHY) == GM_INVALID && submit_live(SEL, 8u) == GM_INVALID;
    ok &= submit_live(SEL, BE_TMBR) == GM_UNSUPPORTED;              /* the timbre layer is not there yet */
    ok &= !gmrt.ctl.has_pending;
    check("an invalid track or mask, and the not-yet-available timbre layer, are refused and queue nothing", ok);

    ok = submit_live(SEL, BE_RHY) == GM_OK;
    gm_touch();                                                   /* a native edit between the request and its candidate */
    settle();
    ok &= track_same(&before, SEL) && others_same(&before, SEL) && be_act[SEL].key[0] == key0;
    check("a request whose base moved is dropped: no byte, no key changes", ok);

    fresh();
    take(&before);
    key0 = be_act[SEL].key[0];
    trk[SEL].p[P_SDIV] = 3;                                       /* the track is on 1/32 while the grid is 1/16 */
    take(&before);
    ok = submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    ok &= track_same(&before, SEL) && others_same(&before, SEL) && be_act[SEL].key[0] == key0 &&
          gm_runtime_status() == GM_UNSUPPORTED;
    check("RHY on a grid the rules cannot reproduce is refused, nothing converted", ok);
}

static void undo_restores(void)
{
    snap_t before;
    uint32_t ok = 1;
    be_recipe old;
    fresh();
    take(&before);
    old = be_act[SEL];
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    settle();
    ok &= !track_same(&before, SEL) && gm_runtime_commits() == 1u;
    ok &= gm_runtime_undo() == 1;
    ok &= track_same(&before, SEL) && others_same(&before, SEL);
    ok &= !memcmp(&be_act[SEL], &old, sizeof old);                /* the recipe comes back with the pattern */
    ok &= gm_runtime_undo() == 1 && !track_same(&before, SEL);   /* and REDO brings the mutation (and its keys) back */
    ok &= be_act[SEL].key[0] == be_key_next(old.key[0]);
    check("UNDO restores the bytes and the recipe together, REDO returns both", ok);
}

static void hook_edit(void) { gm_touch(); }               /* a native edit (what REC and every manual step write do) */
static void hook_replace(void)                                 /* a real replacement commit by another request */
{
    assert(submit_live(SEL, BE_TIME) == GM_OK);
    settle();
}

static void undo_is_atomic(void)
{
    snap_t a, b;
    uint32_t ok = 1;
    fresh();
    take(&a);
    ok &= submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    take(&b);
    ok &= !track_same(&a, SEL);
    gm_touch();                                                   /* a manual edit after the publish: nothing to undo */
    ok &= gm_runtime_undo() == 0 && track_same(&b, SEL);
    check("an edit after a publish ends the undo: it refuses and changes nothing", ok);

    fresh();
    take(&a);
    ok = submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    take(&b);
    race_hook = hook_edit;                                        /* the edit lands between the observation and the swap */
    ok &= gm_runtime_undo() == 0;
    race_hook = NULL;
    ok &= track_same(&b, SEL) && others_same(&b, SEL);
    check("an edit between the undo's check and its swap stops the swap (the recorded hit stands)", ok);

    fresh();
    take(&a);
    ok = submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    take(&b);
    race_hook = hook_replace;                                     /* another publish replaces the snapshot meanwhile */
    ok &= gm_runtime_undo() == 0;
    race_hook = NULL;
    {
        snap_t c;
        take(&c);
        ok &= !track_same(&b, SEL);                               /* B's own commit is live, untouched by the old undo */
        ok &= gm_runtime_undo() == 1 && track_same(&b, SEL);      /* a fresh undo restores B's predecessor */
    }
    check("a snapshot replaced between check and swap is refused; a fresh undo restores the replacement's predecessor", ok);

    fresh();
    take(&a);
    ok = submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    fm1_irq_off();
    gm_keep_set(SEL, 4, 1);                                       /* a KEEP is not a pattern byte: undo stays */
    fm1_irq_on();
    ok &= gm_runtime_undo() == 1 && track_same(&a, SEL);
    check("a KEEP change does not end the undo", ok);
}

static snap_t hooked;
static void hook_rec(void)                                      /* the real recorder: a hit written into the playing step */
{
    (void)rec_hit(&trk[SEL], 100);
    take(&hooked);
}
static void recorded_hit_survives(void)
{
    uint32_t ok = 1;
    fresh();
    ok &= submit_live(SEL, BE_RHY) == GM_OK;
    settle();
    race_hook = hook_rec;
    ok &= gm_runtime_undo() == 0;                                 /* the recording took the snapshot: no swap */
    race_hook = NULL;
    ok &= gmrt.undo_ok == 0 && track_same(&hooked, SEL) && others_same(&hooked, SEL);
    check("a hit recorded by the real rec_hit between the undo's check and its swap stands", ok);
}

static void queued_mutation_honours_new_locks(void)
{
    uint32_t ok = 1;
    fresh();
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    song.playing = 1;                                             /* playing: the candidate waits for the END */
    gm_runtime_poll();
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    be_lock_set(SEL + 1u, BE_RHY, 1);                             /* another track's lock does not touch it */
    be_lock_set(SEL, BE_TMBR, 1);                                 /* neither does a layer it does not carry */
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    be_lock_set(SEL, BE_RHY, 1);                                  /* a layer it would change: it must not publish */
    ok &= gmrt.ctl.stage_state != GM_STAGE_READY && cue.q_gm == 0;
    song.playing = 0;
    check("locking a layer a queued mutation carries drops it; other locks leave it", ok);
}

static void queued_mutation_honours_new_events(void)
{
    uint32_t ok = 1;
    fresh();
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    song.playing = 1;
    gm_runtime_poll();
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    lock_set(SEL + 1u, 3, P_E1, 20);                              /* another track's new lock leaves it queued */
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    lock_set(SEL, 3, P_E1, 20);                                   /* this track gains a locked step: the candidate is stale */
    ok &= gmrt.ctl.stage_state != GM_STAGE_READY && cue.q_gm == 0;
    song.playing = 0;
    check("a parameter lock added while a mutation waits for the END drops it (other tracks' locks do not)", ok);
}

static void editing_an_existing_event_keeps_the_mutation(void)
{
    uint32_t ok = 1;
    fresh();
    lock_set(SEL, 3, P_E1, 20);                                   /* step 3 already carries an event */
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    song.playing = 1;
    gm_runtime_poll();
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    lock_set(SEL, 3, P_E1, 40);                                   /* a new value on a step the candidate keeps whole */
    lock_set(SEL, 3, P_E2, 40);                                   /* another knob of the same step */
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    lock_set(SEL, 5, P_E1, 20);                                   /* the first event of another step: the candidate is stale */
    ok &= gmrt.ctl.stage_state != GM_STAGE_READY && cue.q_gm == 0;
    song.playing = 0;
    check("editing a step that already had an event keeps a queued mutation; the first event of another step drops it", ok);
}

static void commit_records_the_config_that_made_it(void)
{
    gm_config c;
    uint32_t ok = 1;
    fresh();
    assert(gm_factory_config(5, &c) == GM_OK);
    ok &= !be_cfg_known && be_submit_cfg(SEL, BE_RHY | BE_TIME, &c) == GM_OK;   /* no known config: the panel's one generates */
    settle();
    ok &= gm_runtime_commits() == 1u && !memcmp(&gmrt.ctl.active_config, &c, sizeof c) && be_cfg_known;
    ok &= gm_runtime_undo() == 1 && !be_cfg_known;                /* the undo brings back "unknown" with the bytes */
    ok &= gm_runtime_undo() == 1 && be_cfg_known && !memcmp(&gmrt.ctl.active_config, &c, sizeof c);
    check("a BE commit records the config that generated it; UNDO and REDO swap that knowledge with the bytes", ok);

    fresh();
    ok = 1;
    assert(gm_factory_config(5, &c) == GM_OK);
    ok &= gm_runtime_submit(&gmrt.ctl.active_config, GM_BOUNDARY_PATTERN, GM_REQUEST_MORPH) == GM_OK;
    settle();
    ok &= be_cfg_known;                                           /* a legacy commit makes it known too */
    ok &= be_submit_cfg(SEL, BE_TIME, &c) == GM_OK;               /* a panel style that differs: the live config wins */
    settle();
    ok &= gm_runtime_commits() == 2u && memcmp(&gmrt.ctl.active_config, &c, sizeof c) != 0;
    check("with a known live config a different panel config does not generate or replace it", ok);
}

static void layer_locks_survive_a_context_change(void)
{
    uint32_t ok = 1;
    fresh();
    be_lock_set(SEL, BE_RHY, 1);
    be_lock_set(SEL + 1u, BE_TIME, 1);
    fm1_irq_off();
    gm_context_isr();                                             /* a project load or a cue's pattern switch */
    fm1_irq_on();
    ok &= be_act[SEL].locks == BE_RHY && be_act[SEL + 1u].locks == BE_TIME && !be_cfg_known;
    ok &= gm_runtime_undo() == 0 && be_act[SEL].locks == BE_RHY;  /* (the undo is gone with the pattern) */
    check("layer locks survive a project or pattern switch, the recipes and the known config do not", ok);
}

static void dropped_candidate_says_not_applied(void)
{
    uint32_t ok = 1;
    fresh();
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    song.playing = 1;
    gm_runtime_poll();
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY;
    be_lock_set(SEL, BE_TIME, 1);
    ok &= gmrt.ctl.stage_state != GM_STAGE_READY && gm_runtime_status() == GM_STALE;
    gm_runtime_ack_status();
    ok &= submit_live(SEL, BE_RHY) == GM_OK;                      /* a request that is replaced on purpose says nothing */
    ok &= submit_live(SEL, BE_RHY) == GM_OK && gm_runtime_status() == GM_OK;
    song.playing = 0;
    check("a candidate dropped by a lock or event says NOT APPLIED (stale); a superseded one does not", ok);
}

static void song_start_between_submit_and_commit(void)
{
    uint32_t ok = 1, before;
    fresh();
    before = gm_runtime_commits();
    ok &= submit_live(SEL, BE_TIME) == GM_OK;
    song.playing = 1;                                             /* PLAY lands before the main loop made the candidate */
    gm_runtime_poll();
    ok &= gm_runtime_commits() == before && cue.q_gm == 1;        /* it waits for the END instead of cutting in */
    song.playing = 0;
    check("a song that starts between submit and commit makes the mutation wait for the END", ok);
}

static void tempo_follows_the_panel(void)
{
    gm_config c;
    uint32_t ok = 1;
    fresh();
    ok &= gm_runtime_submit(&gmrt.ctl.active_config, GM_BOUNDARY_PATTERN, GM_REQUEST_MORPH) == GM_OK;
    settle();
    ok &= be_cfg_known;
    c = gmrt.ctl.active_config;
    c.bpm = (uint16_t)(c.bpm == 240u ? 100u : 240u);              /* the tempo changed without a generation */
    ok &= be_submit_cfg(SEL, BE_TIME, &c) == GM_OK;
    ok &= gmrt.ctl.pending.config.bpm == c.bpm;                   /* the request scales the micro timing with it */
    settle();
    ok &= gmrt.ctl.active_config.bpm == c.bpm;
    check("a layer mutation uses the panel's current tempo, the known config's style and controls", ok);
}

static void mutation_publishes_at_the_pattern_end(void)
{
    uint32_t ok = 1, before, commits, k, len = 0, longest = 0;
    fresh();
    for (k = 0; k < NTRK; k++)
        if ((uint32_t)trk[k].p[P_SLEN] > len) {
            len = (uint32_t)trk[k].p[P_SLEN];
            longest = k;
        }
    before = gm_runtime_commits();
    ok &= submit_live(SEL, BE_TIME) == GM_OK;
    song.playing = 1;
    gm_runtime_poll();
    ok &= gmrt.ctl.stage_state == GM_STAGE_READY && cue.q_gm == 1;
    for (k = 0; k < NTRK; k++) {
        trk[k].seq_pos = 0;
        trk[k].seq_idx = 0;
        trk[k].seq_rem = 0;
        trk[k].seq_cnt = 0;
    }
    cue_tick(64, 1u << 20, 0);                                    /* mid-pattern: the boundary is not here */
    ok &= gm_runtime_commits() == before && cue.q_gm == 1;
    trk[longest].seq_idx = len - 1u;                              /* the last step of the longest pattern */
    commits = gm_runtime_commits();
    cue_tick(64, 1u << 20, 0);
    ok &= gm_runtime_commits() == commits + 1u && cue.q_gm == 0;  /* published at the END, once */
    song.playing = 0;
    (void)before;
    check("a queued mutation publishes at the END of the longest pattern, not before", ok);
}

static void undo_restores_the_generator_config(void)
{
    gm_config c0, c1;
    uint32_t ok = 1;
    fresh();
    c0 = gmrt.ctl.active_config;
    assert(gm_factory_config(5, &c1) == GM_OK);
    ok &= gm_runtime_submit(&c1, GM_BOUNDARY_PATTERN, GM_REQUEST_STYLE) == GM_OK;
    settle();
    ok &= memcmp(&gmrt.ctl.active_config, &c0, sizeof c0) != 0;   /* the style change moved the config */
    ok &= gm_runtime_undo() == 1 && !memcmp(&gmrt.ctl.active_config, &c0, sizeof c0);
    ok &= gm_runtime_undo() == 1 && !memcmp(&gmrt.ctl.active_config, &c1, sizeof c1);   /* REDO brings it back too */
    check("UNDO and REDO swap the generator config together with the pattern", ok);
}

static void parameter_locks_protect_cells(void)
{
    snap_t before;
    uint32_t ok = 1, s, len, moved = 0;
    fresh();
    take(&before);
    len = (uint32_t)trk[SEL].p[P_SLEN];
    for (s = 0; s < len; s++)
        lock_set(SEL, s, P_E1, 20);                               /* every playable step carries a parameter lock */
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    settle();
    ok &= track_same(&before, SEL) && gm_runtime_status() == GM_NO_CHANGE;   /* nothing may change under a lock */
    check("a step with a parameter lock is protected as a whole", ok);

    fresh();
    take(&before);
    lock_set(SEL, 3, P_E1, 20);
    ok = submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    settle();
    ok &= !memcmp(&before.st[SEL][3], &trk[SEL].step[3], sizeof(step_t));
    for (s = 0; s < len; s++)
        moved |= memcmp(&before.st[SEL][s], &trk[SEL].step[s], sizeof(step_t)) != 0;
    ok &= moved != 0u;                                            /* the other steps do change */
    check("only the locked step is protected: the rest of the track still mutates", ok);
}

static void paired_hits_stay_paired(void)
{
    snap_t before;
    gm_config h;
    gm_rule_masks m;
    uint32_t ok = 1, s, len;
    fresh();
    assert(gm_factory_config(2, &h) == GM_OK);                    /* HAMMER: kick and snare share their optional hits */
    ok &= gm_runtime_submit(&h, GM_BOUNDARY_PATTERN, GM_REQUEST_STYLE) == GM_OK;
    settle();
    ok &= gm_rules_build(&gmrt.ctl.active_config, &m) == GM_OK && m.paired != 0u;
    take(&before);
    len = (uint32_t)trk[GM_KICK].p[P_SLEN];
    ok &= submit_live(GM_KICK, BE_RHY) == GM_OK;
    settle();
    for (s = 0; s < len; s++)
        if ((m.paired >> s) & 1u)
            ok &= !memcmp(&before.st[GM_KICK][s], &trk[GM_KICK].step[s], sizeof(step_t));   /* a pair is not split */
    check("RHY on the kick leaves the steps it shares with the snare as they are", ok);
}

static void paired_hits_keep_presence_but_retime(void)
{
    snap_t before;
    gm_config h;
    gm_rule_masks m;
    uint32_t ok = 1, s, len, moved = 0, on_paired = 0;
    fresh();
    assert(gm_factory_config(2, &h) == GM_OK);
    ok &= gm_runtime_submit(&h, GM_BOUNDARY_PATTERN, GM_REQUEST_STYLE) == GM_OK;
    settle();
    ok &= gm_rules_build(&gmrt.ctl.active_config, &m) == GM_OK && m.paired != 0u;
    take(&before);
    len = (uint32_t)trk[GM_KICK].p[P_SLEN];
    ok &= submit_live(GM_KICK, BE_RHY | BE_TIME) == GM_OK;          /* both layers free: the LOCKS page's MUT */
    settle();
    for (s = 0; s < len; s++)
        if ((m.paired >> s) & 1u) {
            const step_t *a = &before.st[GM_KICK][s], *b = &trk[GM_KICK].step[s];
            ok &= a->on == b->on && a->acc == b->acc && a->slide == b->slide && a->tie == b->tie && a->rat == b->rat &&
                  a->cond == b->cond && a->nt == b->nt;           /* the pair is not split: presence and attributes stay */
            if (a->on) {
                on_paired++;
                moved |= a->micro != b->micro;                    /* but its timing is the TIME layer's to move */
            }
        }
    ok &= on_paired > 0u && moved != 0u;
    check("RHY and TIME on the kick: paired hits keep their presence, their micro-timing still mutates", ok);
}

static void unrelated_tracks_do_not_block(void)
{
    snap_t before;
    uint32_t ok = 1;
    fresh();
    trk[5].p[P_SDIV] = 4;                                         /* another track on a grid Grind cannot export */
    trk[5].p[P_SLEN] = 7;
    fm1_irq_off();
    gm_keep_set(5, 2, 1);                                         /* and a KEEP there that the generator would refuse */
    fm1_irq_on();
    take(&before);
    ok &= submit_live(SEL, BE_RHY | BE_TIME) == GM_OK;
    settle();
    ok &= !track_same(&before, SEL) && others_same(&before, SEL) && gm_runtime_commits() == 1u;
    check("another track's grid or KEEP does not block a mutation of the selected track", ok);
}

static void superseded_step_request_clears_now(void)
{
    gm_config c;
    uint32_t ok = 1;
    fresh();
    song.playing = 1;
    assert(gm_factory_config(5, &c) == GM_OK);
    ok &= gm_runtime_submit(&c, GM_BOUNDARY_STEP, GM_REQUEST_STYLE) == GM_OK;
    gm_runtime_poll();
    ok &= cue.now == 1 && cue.q_gm == 1;                          /* a STEP request waits for the next step */
    ok &= submit_live(SEL, BE_TIME) == GM_OK;                       /* ... and a BE request (at the END) replaces it */
    ok &= cue.now == 0 && cue.q_gm == 0;
    song.playing = 0;
    check("a BE request that replaces a ready STEP request does not inherit its next-step boundary", ok);
}

int main(void)
{
    recorded_hit_survives();
    queued_mutation_honours_new_locks();
    queued_mutation_honours_new_events();
    editing_an_existing_event_keeps_the_mutation();
    commit_records_the_config_that_made_it();
    layer_locks_survive_a_context_change();
    dropped_candidate_says_not_applied();
    song_start_between_submit_and_commit();
    tempo_follows_the_panel();
    mutation_publishes_at_the_pattern_end();
    undo_restores_the_generator_config();
    parameter_locks_protect_cells();
    paired_hits_stay_paired();
    paired_hits_keep_presence_but_retime();
    unrelated_tracks_do_not_block();
    superseded_step_request_clears_now();
    rhythm_only();
    time_only();
    locks_and_keep();
    refusals_and_stale();
    undo_restores();
    undo_is_atomic();
    printf(fails ? "test_be_runtime: %d FAILED\n" : "test_be_runtime: all passed\n", fails);
    return fails != 0;
}
