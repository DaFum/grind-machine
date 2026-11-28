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
    ok &= be_submit(SEL, BE_RHY) == GM_OK;
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
    ok &= be_submit(SEL, BE_TIME) == GM_OK;
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
    ok &= be_submit(SEL, BE_ALL) == GM_NO_CHANGE;                 /* every layer locked: nothing to do, nothing queued */
    settle();
    ok &= others_same(&before, NTRK) && track_same(&before, SEL) && !gmrt.ctl.has_pending;
    be_act[SEL].locks = BE_RHY;                                   /* RHY locked, TIME free */
    ok &= be_submit(SEL, BE_ALL) == GM_OK;
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
    ok = be_submit(SEL, BE_RHY | BE_TIME) == GM_OK;
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
    ok &= be_submit(NTRK, BE_RHY) == GM_INVALID && be_submit(SEL, 8u) == GM_INVALID;
    ok &= be_submit(SEL, BE_TMBR) == GM_UNSUPPORTED;              /* the timbre layer is not there yet */
    ok &= !gmrt.ctl.has_pending;
    check("an invalid track or mask, and the not-yet-available timbre layer, are refused and queue nothing", ok);

    ok = be_submit(SEL, BE_RHY) == GM_OK;
    gm_touch();                                                   /* a native edit between the request and its candidate */
    settle();
    ok &= track_same(&before, SEL) && others_same(&before, SEL) && be_act[SEL].key[0] == key0;
    check("a request whose base moved is dropped: no byte, no key changes", ok);

    fresh();
    take(&before);
    key0 = be_act[SEL].key[0];
    trk[SEL].p[P_SDIV] = 3;                                       /* the track is on 1/32 while the grid is 1/16 */
    take(&before);
    ok = be_submit(SEL, BE_RHY) == GM_OK;
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
    ok &= be_submit(SEL, BE_RHY | BE_TIME) == GM_OK;
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
    assert(be_submit(SEL, BE_TIME) == GM_OK);
    settle();
}

static void undo_is_atomic(void)
{
    snap_t a, b;
    uint32_t ok = 1;
    fresh();
    take(&a);
    ok &= be_submit(SEL, BE_RHY) == GM_OK;
    settle();
    take(&b);
    ok &= !track_same(&a, SEL);
    gm_touch();                                                   /* a manual edit after the publish: nothing to undo */
    ok &= gm_runtime_undo() == 0 && track_same(&b, SEL);
    check("an edit after a publish ends the undo: it refuses and changes nothing", ok);

    fresh();
    take(&a);
    ok = be_submit(SEL, BE_RHY) == GM_OK;
    settle();
    take(&b);
    race_hook = hook_edit;                                        /* the edit lands between the observation and the swap */
    ok &= gm_runtime_undo() == 0;
    race_hook = NULL;
    ok &= track_same(&b, SEL) && others_same(&b, SEL);
    check("an edit between the undo's check and its swap stops the swap (the recorded hit stands)", ok);

    fresh();
    take(&a);
    ok = be_submit(SEL, BE_RHY) == GM_OK;
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
    ok = be_submit(SEL, BE_RHY) == GM_OK;
    settle();
    fm1_irq_off();
    gm_keep_set(SEL, 4, 1);                                       /* a KEEP is not a pattern byte: undo stays */
    fm1_irq_on();
    ok &= gm_runtime_undo() == 1 && track_same(&a, SEL);
    check("a KEEP change does not end the undo", ok);
}

int main(void)
{
    rhythm_only();
    time_only();
    locks_and_keep();
    refusals_and_stale();
    undo_restores();
    undo_is_atomic();
    printf(fails ? "test_be_runtime: %d FAILED\n" : "test_be_runtime: all passed\n", fails);
    return fails != 0;
}
