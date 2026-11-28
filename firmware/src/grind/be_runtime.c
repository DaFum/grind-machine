/* SPDX-License-Identifier: GPL-3.0-only */
/* BRUTAL EVOLUTION on the native runtime: one track's named layers (RHY, TIME) mutated from a fixed base and published
 * through the same controller, stage and boundary as every other Grind request. Included at the end of runtime.c (it
 * needs gmrt), so it is one translation unit with the sequencer, and only under GRIND_NATIVE.
 *
 * What a request changes is named, not "the pattern": the stage is the live native bytes as they are (the dormant steps
 * past LEN, the other tracks, every padding bit), with only the selected track's rhythm fields (RHY) or micro (TIME)
 * written into its playable steps. Nothing is converted through the portable pattern and back. */

static be_recipe be_act[NTRK];        /* the recipes of the live pattern */
static be_recipe be_undo[NTRK];       /* the recipes that belong to gmrt.undo (swapped with it) */
static be_patch be_pend, be_stage;    /* the one request's scope (main loop writes both), and what its stage publishes */
static gm_config be_undo_cfg;         /* the generator config that belongs to gmrt.undo (swapped with it) */

static void be_reset(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        be_recipe_init(&be_act[k], gmrt.ctl.active_config.seed, (uint8_t)k);
    memcpy(be_undo, be_act, sizeof be_undo);
    be_undo_cfg = gmrt.ctl.active_config;
    memset(&be_pend, 0, sizeof be_pend);
    memset(&be_stage, 0, sizeof be_stage);
}

/* Main loop. The layers the track does not lock, from a fixed base: a candidate made from a changed pattern is dropped,
 * never rebased. GM_NO_CHANGE: every asked layer is locked. GM_UNSUPPORTED: only the timbre layer was asked (it is not
 * there yet), or a song runs. */
static gm_result be_submit(uint8_t track, uint8_t layers)
{
    gm_request r;
    gm_result v;
    if (track >= NTRK || (layers & ~BE_ALL))
        return GM_INVALID;
    layers &= (uint8_t)~be_act[track].locks;
    if (!layers)
        return GM_NO_CHANGE;
    layers &= (uint8_t)~BE_TMBR;
    if (!layers || chain.running)
        return GM_UNSUPPORTED;
    memset(&r, 0, sizeof r);
    if (!++gmrt.next_rev)
        ++gmrt.next_rev;                                  /* (0 is no request) */
    r.revision = gmrt.next_rev;
    r.at = GM_BOUNDARY_PATTERN;                           /* (while playing: the END of the longest pattern) */
    r.kind = GM_REQUEST_BE_LAYER;
    r.follow_live = 0;
    fm1_irq_off();
    r.base_revision = gmrt.ctl.native_revision;
    r.config = gmrt.ctl.active_config;                    /* (a commit leaves the legacy config as it is) */
    v = gm_ctl_submit(&gmrt.ctl, &r);
    if (v == GM_OK) {
        memset(&be_pend, 0, sizeof be_pend);
        be_pend.request = r.revision;
        be_pend.base_revision = r.base_revision;
        be_pend.track = track;
        be_pend.layers = layers;
        if (gmrt.ctl.stage_state == GM_STAGE_FREE) {
            cue.q_gm = 0;                                 /* (the older stage is gone: nothing waits for a boundary) */
            if (!cue.q_slot && !cue.q_var)
                cue.now = 0;                              /* (nor for the next step a replaced STEP request asked for) */
        }
    }
    fm1_irq_on();
    return v;
}

/* The portable base of ONE track: its own playable steps, timing and KEEP cells. The other tracks are inert filler (a
 * valid empty timing, no cells, no KEEP): their grids and KEEPs are none of this mutation's business and must neither
 * block it nor be validated by it. A track that plays another source (GRIDS) is not mutated. */
static gm_result be_base_track(unsigned tr, const gm_config *cfg)
{
    uint32_t t, s, len = (uint32_t)clamp(trk[tr].p[P_SLEN], 1, NSTEP);
    uint8_t mus;
    gm_result res;
    if (trk[tr].p[P_SRC] != 0)
        return GM_UNSUPPORTED;
    res = gm_musical_division((uint8_t)trk[tr].p[P_SDIV], &mus);
    if (res != GM_OK)
        return res;
    memset(&gmrt.u.base, 0, sizeof gmrt.u.base);
    for (t = 0; t < NTRK; t++)
        gmrt.u.base.timing[t] = (gm_timing){16, cfg->grid.division, 0, 0};
    gmrt.u.base.timing[tr] = (gm_timing){(uint8_t)len, mus, (uint8_t)trk[tr].p[P_SSWING], 0};
    for (s = 0; s < len; s++) {
        gmrt.u.base.cell[tr][s] = gm_from_native(&trk[tr].step[s]);
        if (gm_keep_get(tr, s))
            gmrt.u.base.lock[(tr * NSTEP + s) >> 3] |= (uint8_t)(1u << ((tr * NSTEP + s) & 7u));
    }
    return GM_OK;
}

/* The steps of track tr that carry a parameter lock or a motion event: whole cells, protected as KEEP is. Read with the
 * IRQ off (the lock bank is written by the recorder). */
static uint64_t be_event_mask(unsigned tr, uint32_t len)
{
    uint64_t m = 0;
    uint32_t s;
    fm1_irq_off();
    for (s = 0; s < len; s++) {
        uint32_t i = lk_seek(&mo.b, tr, s);
        if (i < mo.b.count && (uint32_t)(mo.b.key[i] >> 7) == (tr << 6 | s))
            m |= UINT64_C(1) << s;
    }
    fm1_irq_on();
    return m;
}

/* Main loop, outside the IRQ: the native stage for the request. The base is the selected track as the core sees it
 * (gmrt.u.base); once it is no longer needed the same bytes become the native stage (gmrt.u.n). A native edit meanwhile
 * bumps the controller's revision, gm_ctl_finish then drops this candidate. */
static gm_result be_build(const gm_request *r)
{
    be_patch p = be_pend;
    gm_config cfg = r->config;
    gm_cell merged[GM_STEPS];
    gm_rule_masks rules;
    uint64_t paired = 0, events;
    uint32_t k, s, len, changed = 0;
    uint8_t swing;
    unsigned tr, layers;
    gm_result res;
    if (p.request != r->revision || p.track >= NTRK)
        return GM_STALE;
    tr = p.track;
    layers = p.layers & (BE_RHY | BE_TIME);
    p.next = be_act[tr];
    if (layers & BE_RHY)
        p.next.key[0] = be_key_next(p.next.key[0]);
    if (layers & BE_TIME)
        p.next.key[1] = be_key_next(p.next.key[1]);
    res = be_base_track(tr, &cfg);
    if (res != GM_OK)
        return res;
    if (layers & BE_RHY) {                                /* the rhythm: the generator, keyed by the RHY layer alone */
        cfg.mutation = p.next.key[0];
        res = gm_generate(&cfg, &gmrt.u.base, &gmrt.out);
        if (res != GM_OK)
            return res;
        if (gmrt.out.timing[tr].length != gmrt.u.base.timing[tr].length ||
            gmrt.out.timing[tr].division != gmrt.u.base.timing[tr].division)
            return GM_UNSUPPORTED;                        /* the rules would change this track's grid: not converted */
        if ((tr == GM_KICK || tr == GM_SNARE) && gm_rules_build(&cfg, &rules) == GM_OK)
            paired = rules.paired;                        /* (kick and snare share these hits: this track alone must not split them) */
    }
    len = gmrt.u.base.timing[tr].length;
    swing = gmrt.u.base.timing[tr].swing;
    events = be_event_mask(tr, len);
    cfg.grid.division = gmrt.u.base.timing[tr].division;  /* (the micro span follows the grid the track really has) */
    for (s = 0; s < len; s++) {
        gm_cell b = gmrt.u.base.cell[tr][s], c = (layers & BE_RHY) ? gmrt.out.cell[tr][s] : b;
        int prot = gm_is_locked(&gmrt.u.base, tr, s) || ((paired >> s) & 1u) || ((events >> s) & 1u);
        if ((layers & BE_TIME) && (c.flags & GM_ON))
            c.micro = be_time_micro(&cfg, p.next.key[1], tr, s, swing);
        else
            c.micro = b.micro;                            /* (a step that does not play keeps its dormant timing) */
        res = be_merge_cell(b, c, layers, prot, &merged[s]);
        if (res != GM_OK)
            return res;
        changed |= memcmp(&b, &merged[s], sizeof b) != 0;
    }
    if (!changed)
        return GM_NO_CHANGE;                              /* (a new key alone is not a change: it is not published) */
    for (k = 0; k < NTRK; k++) {                          /* the base is dead: the live bytes, exactly, become the stage */
        memcpy(gmrt.u.n.st[k], trk[k].step, sizeof trk[k].step);
        gmrt.u.n.len[k] = (uint8_t)trk[k].p[P_SLEN];
        gmrt.u.n.div[k] = (uint8_t)trk[k].p[P_SDIV];
        gmrt.u.n.swing[k] = (uint8_t)trk[k].p[P_SSWING];
        gmrt.u.n.src[k] = (uint8_t)trk[k].p[P_SRC];
    }
    for (s = 0; s < len; s++) {                           /* only the named fields of the playable steps */
        step_t *n = &gmrt.u.n.st[tr][s];
        gm_cell m = merged[s];
        if (layers & BE_RHY) {
            n->on = m.flags & 1u;
            n->acc = (m.flags >> 1) & 1u;
            n->slide = (m.flags >> 2) & 1u;
            n->tie = (m.flags >> 3) & 1u;
            n->rat = (m.flags >> GM_RAT_SHIFT) & 3u;
            n->cond = m.cond;
            n->nt = m.note;
        }
        if (layers & BE_TIME)
            n->micro = m.micro;
    }
    p.changed = 1;
    be_stage = p;
    return GM_OK;
}

/* The boundary (ISR, or the main loop with the IRQ off), after the swap: what UNDO brings back is the recipes the
 * pattern had, and the new ones belong to the new bytes. */
static void be_commit_hook(void)
{
    memcpy(be_undo, be_act, sizeof be_undo);
    be_undo_cfg = gmrt.ctl.active_config;                 /* (the commit that follows replaces it) */
    if (gmrt.ctl.stage_req.kind == GM_REQUEST_BE_LAYER && be_stage.request == gmrt.ctl.stage_req.revision &&
        be_stage.track < NTRK) {
        uint8_t locks = be_act[be_stage.track].locks;     /* (a lock set while the candidate was made is the player's, not the patch's) */
        be_act[be_stage.track] = be_stage.next;
        be_act[be_stage.track].locks = locks;
    }
}

/* UNDO / REDO (IRQ off): the recipes swap with the bytes, so a second one undoes the first. */
static void be_undo_hook(void)
{
    be_recipe tmp[NTRK];
    gm_config cfg = gmrt.ctl.active_config;
    uint32_t k;
    gmrt.ctl.active_config = be_undo_cfg;                 /* the generator config goes with the pattern it made */
    be_undo_cfg = cfg;
    memcpy(tmp, be_act, sizeof tmp);
    memcpy(be_act, be_undo, sizeof tmp);
    memcpy(be_undo, tmp, sizeof tmp);
    for (k = 0; k < NTRK; k++) {                          /* the locks are the player's setting, not part of the pattern's history */
        be_act[k].locks = tmp[k].locks;
        be_undo[k].locks = tmp[k].locks;
    }
}

/* A layer lock of one track (main loop). The IRQ is off: the commit hook rewrites the recipes at the boundary. */
static void be_lock_set(uint8_t track, uint8_t bit, int on)
{
    if (track >= NTRK)
        return;
    fm1_irq_off();
    if (on) {
        be_act[track].locks |= bit;
        /* a mutation that carries this layer and is not live yet (pending, being made, or waiting for the END) must
         * not publish it any more: it is dropped, the player asks again */
        if (be_pend.track == track && (be_pend.layers & bit) &&
            ((gmrt.ctl.has_pending && gmrt.ctl.pending.kind == GM_REQUEST_BE_LAYER && gmrt.ctl.pending.revision == be_pend.request) ||
             ((gmrt.ctl.stage_state == GM_STAGE_WRITING || gmrt.ctl.stage_state == GM_STAGE_READY) &&
              gmrt.ctl.stage_req.kind == GM_REQUEST_BE_LAYER && gmrt.ctl.stage_req.revision == be_pend.request)))
            gm_runtime_stop();
    } else {
        be_act[track].locks &= (uint8_t)~bit;
    }
    fm1_irq_on();
}
