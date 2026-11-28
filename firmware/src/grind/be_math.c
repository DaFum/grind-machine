/* SPDX-License-Identifier: GPL-3.0-only */
#include "be.h"

uint32_t be_key_next(uint32_t key)
{
    return gm_mix32(key ^ UINT32_C(0x9e3779b9));
}

void be_recipe_init(be_recipe *r, uint32_t seed, uint8_t track)
{
    for (unsigned l = 0; l < 3u; ++l)
        r->key[l] = gm_mix32(seed ^ gm_mix32((((uint32_t)track + 1u) << 8) | (BE_DOMAIN_RHY + l)));
    r->curve = BE_ASCEND;
    r->amount = 0;
    r->span = 16;
    r->target = BE_INDEX;
    r->mode = BE_OFF;
    r->brutal = 0;
    r->locks = 0;
    r->flags = 0;
}

int be_recipe_valid(const be_recipe *r)
{
    return r && r->curve < BE_CURVE_N && r->target < BE_TARGET_N && r->mode < BE_MODE_N &&
           r->amount <= BE_AMOUNT_MAX && r->span >= 1u && r->span <= BE_SPAN_MAX &&
           r->brutal <= BE_BRUTAL_MAX && (r->locks & ~BE_ALL) == 0u && (r->flags & ~BE_FLAG_MASK) == 0u;
}
