/* SPDX-License-Identifier: GPL-3.0-only */
#include "be.h"

gm_result be_merge_cell(gm_cell base, gm_cell candidate, unsigned layers, int protected_cell, gm_cell *out)
{
    if (!out || (layers & ~BE_ALL) || !gm_cell_valid(base) || !gm_cell_valid(candidate))
        return GM_INVALID;
    gm_cell next = base;
    if (!protected_cell) {
        if (layers & BE_RHY) {
            next.flags = candidate.flags;
            next.cond = candidate.cond;
            next.note = candidate.note;
        }
        if (layers & BE_TIME)
            next.micro = candidate.micro;
    }
    *out = next;
    return GM_OK;
}

/* The generator's own micro-timing rule (core.c), driven by the TIME key instead of the shared mutation counter. */
int8_t be_time_micro(const gm_config *cfg, uint32_t key, unsigned track, unsigned step, uint8_t swing)
{
    gm_config keyed = *cfg;
    keyed.mutation = key;
    unsigned span = 600u * cfg->bpm * cfg->grid.division / 240000u;
    if (span > 12u)
        span = 12u;
    span = span * cfg->humanize / 100u;
    span = span * (250u - swing) / 250u;
    uint32_t word = gm_random(&keyed, (uint8_t)track, (uint8_t)step, GM_DOMAIN_MICRO);
    if (step == 0u)
        return (int8_t)(word % (span + 1u));
    return (int8_t)((int)(word % (2u * span + 1u)) - (int)span);
}
