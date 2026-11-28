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

static void be_reset(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        be_recipe_init(&be_act[k], gmrt.ctl.active_config.seed, (uint8_t)k);
    memcpy(be_undo, be_act, sizeof be_undo);
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
        if (gmrt.ctl.stage_state == GM_STAGE_FREE)
            cue.q_gm = 0;                                 /* (the older stage is gone: nothing waits for a boundary) */
    }
    fm1_irq_on();
    return v;
}

/* Main loop, outside the IRQ: the native stage for the request. The base is the live pattern as the core sees it
 * (gmrt.u.base); once it is no longer needed the same bytes become the native stage (gmrt.u.n). A native edit meanwhile
 * bumps the controller's revision, gm_ctl_finish then drops this candidate. */
static gm_result be_build(const gm_request *r)
{
    be_patch p = be_pend;
    gm_config cfg = r->config;
    gm_cell merged[GM_STEPS];
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
    res = gm_runtime_base();
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
    }
    len = gmrt.u.base.timing[tr].length;
    swing = gmrt.u.base.timing[tr].swing;
    cfg.grid.division = gmrt.u.base.timing[tr].division;  /* (the micro span follows the grid the track really has) */
    for (s = 0; s < len; s++) {
        gm_cell b = gmrt.u.base.cell[tr][s], c = (layers & BE_RHY) ? gmrt.out.cell[tr][s] : b;
        if ((layers & BE_TIME) && (c.flags & GM_ON))
            c.micro = be_time_micro(&cfg, p.next.key[1], tr, s, swing);
        else
            c.micro = b.micro;                            /* (a step that does not play keeps its dormant timing) */
        res = be_merge_cell(b, c, layers, gm_is_locked(&gmrt.u.base, tr, s), &merged[s]);
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
    if (gmrt.ctl.stage_req.kind == GM_REQUEST_BE_LAYER && be_stage.request == gmrt.ctl.stage_req.revision &&
        be_stage.track < NTRK)
        be_act[be_stage.track] = be_stage.next;
}

/* UNDO / REDO (IRQ off): the recipes swap with the bytes, so a second one undoes the first. */
static void be_undo_hook(void)
{
    be_recipe tmp[NTRK];
    memcpy(tmp, be_act, sizeof tmp);
    memcpy(be_act, be_undo, sizeof tmp);
    memcpy(be_undo, tmp, sizeof tmp);
}
