/* SPDX-License-Identifier: GPL-3.0-only */
/* BRUTAL EVOLUTION: portable per-track recipes and layer rules. No target headers, no state. */
#ifndef GRIND_BE_H
#define GRIND_BE_H

#include "grind.h"

/* The three independent layers of one track's mutation. */
enum { BE_RHY = 1u, BE_TIME = 2u, BE_TMBR = 4u, BE_ALL = 7u };
enum { BE_ASCEND, BE_DESCEND, BE_ALTERNATE, BE_CHAOTIC, BE_MOTIF, BE_PULSE, BE_CURVE_N };
enum { BE_INDEX, BE_MDEC, BE_FBK, BE_TARGET_N };
enum { BE_OFF, BE_METAL, BE_GROWL, BE_CHAOS, BE_NOISE, BE_MODE_N };
/* Random domains of the BE layers: 8..14 stay reserved for the DNA engine. */
enum { BE_DOMAIN_RHY = 15, BE_DOMAIN_TIME = 16, BE_DOMAIN_TMBR = 17 };
enum { BE_FLAG_TIMBRE = 1u, BE_FLAG_MASK = 1u, BE_SPAN_MAX = 64u, BE_AMOUNT_MAX = 100u, BE_BRUTAL_MAX = 127u };

/* One track's recipe. key[] are effective 32-bit keys in the order RHY, TIME, TMBR, never the UI request id.
 * locks has the BE_* bits that mutation must not change. */
typedef struct {
    uint32_t key[3];
    uint8_t curve, amount, span, target;
    uint8_t mode, brutal, locks, flags;
} be_recipe;
typedef char be_recipe_is_20[sizeof(be_recipe) == 20 ? 1 : -1];

/* What one accepted mutation publishes for one track. */
typedef struct {
    uint32_t request, base_revision, context;
    be_recipe next;
    uint8_t track, layers, changed, reserved;
} be_patch;
typedef char be_patch_is_36[sizeof(be_patch) == 36 ? 1 : -1];

uint32_t be_key_next(uint32_t key);
/* The neutral recipe of a track: keys derived from the seed, every layer free, no timbre contribution. */
void be_recipe_init(be_recipe *r, uint32_t seed, uint8_t track);
int be_recipe_valid(const be_recipe *r);

/* Merge a candidate cell into the base for the named layers. RHY takes flags, cond and note, TIME takes micro,
 * TMBR never touches cell bytes. A protected cell stays exactly the base. *out is written only on GM_OK. */
gm_result be_merge_cell(gm_cell base, gm_cell candidate, unsigned layers, int protected_cell, gm_cell *out);
/* The micro-timing the TIME layer picks for one step, from its own key (cfg->mutation is not read). */
int8_t be_time_micro(const gm_config *cfg, uint32_t key, unsigned track, unsigned step, uint8_t swing);

#endif
