/* SPDX-License-Identifier: GPL-3.0-only */
/* BE01 T01: independent layer contracts. Expected values come from tools/grind/be_reference.py, not from the code. */
#include "common.h"
#include "be.h"
#include <string.h>

static void merge_fixture(void)
{
    gm_cell a = { GM_ON | GM_ACC | GM_TIE | (2u << GM_RAT_SHIFT), 7, 12, -25 };
    gm_cell b = { GM_ON | GM_SLIDE, 3, -5, 9 }, out;
    assert(be_merge_cell(a, b, BE_RHY, 0, &out) == GM_OK);
    assert(out.flags == b.flags && out.cond == 3 && out.note == -5 && out.micro == -25);
    assert(be_merge_cell(a, b, BE_TIME, 0, &out) == GM_OK);
    assert(out.flags == a.flags && out.cond == 7 && out.note == 12 && out.micro == 9);
    assert(be_merge_cell(a, b, BE_RHY | BE_TIME, 0, &out) == GM_OK);
    assert(!memcmp(&b, &out, sizeof b));
    assert(be_merge_cell(a, b, BE_ALL, 1, &out) == GM_OK);          /* a protected cell is exactly the base */
    assert(!memcmp(&a, &out, sizeof a));
    assert(be_merge_cell(a, b, 0, 0, &out) == GM_OK);               /* no layer: the base */
    assert(!memcmp(&a, &out, sizeof a));
    assert(be_merge_cell(a, b, BE_TMBR, 0, &out) == GM_OK);         /* TIMBRE alone never touches cell bytes */
    assert(!memcmp(&a, &out, sizeof a));

    gm_cell off = { 0, 9, 4, -3 };                                  /* an OFF cell with dormant attributes */
    assert(be_merge_cell(off, b, BE_ALL, 1, &out) == GM_OK);
    assert(!memcmp(&off, &out, sizeof off));
}

static void invalid_inputs(void)
{
    gm_cell a = { GM_ON, 0, 0, 0 }, good = { GM_ON, 3, 1, 2 };
    gm_cell sentinel = { 0xEE, 0xEE, 0x7E, 0x7E }, out = sentinel;
    gm_cell bad_flags = { 64u, 0, 0, 0 }, bad_cond = { GM_ON, 59u, 0, 0 };
    gm_cell bad_note_lo = { GM_ON, 0, -61, 0 }, bad_note_hi = { GM_ON, 0, 68, 0 };
    gm_cell bad_micro_lo = { GM_ON, 0, 0, -51 }, bad_micro_hi = { GM_ON, 0, 0, 51 };
    const gm_cell bad[] = { bad_flags, bad_cond, bad_note_lo, bad_note_hi, bad_micro_lo, bad_micro_hi };
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        assert(be_merge_cell(a, bad[i], BE_ALL, 0, &out) == GM_INVALID);
        assert(!memcmp(&out, &sentinel, sizeof out));               /* a refusal never writes */
        assert(be_merge_cell(bad[i], a, BE_ALL, 0, &out) == GM_INVALID);
        assert(!memcmp(&out, &sentinel, sizeof out));
    }
    assert(be_merge_cell(a, good, 8u, 0, &out) == GM_INVALID);      /* a mask outside BE_ALL */
    assert(be_merge_cell(a, good, 0x80u, 1, &out) == GM_INVALID);
    assert(!memcmp(&out, &sentinel, sizeof out));
    assert(be_merge_cell(a, good, BE_ALL, 0, NULL) == GM_INVALID);
}

static void keys(void)
{
    /* independent vectors: tools/grind/be_reference.py */
    assert(be_key_next(0u) == UINT32_C(0x01fce552));
    assert(be_key_next(1u) == UINT32_C(0xa4f7896c));
    assert(be_key_next(UINT32_C(0xFFFFFFFF)) == UINT32_C(0x35e1e3a1));
    assert(be_key_next(UINT32_C(0x12345678)) == UINT32_C(0x8ff7a29e));
    be_recipe r0, r7;
    be_recipe_init(&r0, UINT32_C(0xC0FFEE), 0);
    be_recipe_init(&r7, UINT32_C(0xC0FFEE), 7);
    assert(r0.key[0] == UINT32_C(0x0f0727ce) && r0.key[1] == UINT32_C(0xa15031ff) && r0.key[2] == UINT32_C(0x2550df90));
    assert(r7.key[0] == UINT32_C(0x501ea5ed) && r7.key[1] == UINT32_C(0x5b74c1c8) && r7.key[2] == UINT32_C(0xde8080e7));
    /* the three layers and eight tracks never share one effective key */
    uint32_t all[GM_TRACKS * 3u];
    for (unsigned t = 0; t < GM_TRACKS; ++t) {
        be_recipe r;
        be_recipe_init(&r, UINT32_C(0xC0FFEE), (uint8_t)t);
        for (unsigned l = 0; l < 3u; ++l)
            all[t * 3u + l] = r.key[l];
    }
    for (unsigned i = 0; i < GM_TRACKS * 3u; ++i)
        for (unsigned j = i + 1u; j < GM_TRACKS * 3u; ++j)
            assert(all[i] != all[j]);
    for (unsigned i = 0; i < 64u; ++i)                              /* advancing never sticks, a zero key is a normal key */
        assert(be_key_next((uint32_t)i) != (uint32_t)i);
}

static void neutral_recipe(void)
{
    be_recipe r;
    be_recipe_init(&r, 1u, 3);
    assert(r.locks == 0 && r.amount == 0 && r.span == 16 && r.target == BE_INDEX);
    assert(r.curve == BE_ASCEND && r.mode == BE_OFF && r.brutal == 0 && r.flags == 0);
    assert(be_recipe_valid(&r));
    be_recipe c = r;
    c.locks = 8u;                       assert(!be_recipe_valid(&c));
    c = r; c.flags = 2u;                assert(!be_recipe_valid(&c));
    c = r; c.curve = BE_CURVE_N;        assert(!be_recipe_valid(&c));
    c = r; c.target = BE_TARGET_N;      assert(!be_recipe_valid(&c));
    c = r; c.mode = BE_MODE_N;          assert(!be_recipe_valid(&c));
    c = r; c.amount = 101;              assert(!be_recipe_valid(&c));
    c = r; c.span = 0;                  assert(!be_recipe_valid(&c));
    c = r; c.span = 65;                 assert(!be_recipe_valid(&c));
    c = r; c.brutal = 128;              assert(!be_recipe_valid(&c));
    c = r; c.locks = BE_ALL; c.amount = 100; c.span = 64; c.brutal = 127; c.flags = 1u;
    assert(be_recipe_valid(&c));
}

static void time_micro(void)
{
    gm_config cfg = {0};
    cfg.seed = UINT32_C(0xC0FFEE);
    cfg.bpm = 200;
    cfg.engine_version = GM_ENGINE_VERSION;
    cfg.algorithm = 5;
    cfg.humanize = 100;
    cfg.grid.division = 16;
    const uint32_t key = UINT32_C(0xA5A5A5A5);
    /* swing 0 / swing 40, independent vectors: tools/grind/be_reference.py */
    assert(be_time_micro(&cfg, key, 2, 0, 0) == 0 && be_time_micro(&cfg, key, 2, 0, 40) == 3);
    assert(be_time_micro(&cfg, key, 2, 1, 0) == -3 && be_time_micro(&cfg, key, 2, 1, 40) == -3);
    assert(be_time_micro(&cfg, key, 2, 5, 0) == 0 && be_time_micro(&cfg, key, 2, 5, 40) == 0);
    assert(be_time_micro(&cfg, key, 2, 9, 0) == -6 && be_time_micro(&cfg, key, 2, 9, 40) == 6);
    assert(be_time_micro(&cfg, key, 5, 0, 0) == 1 && be_time_micro(&cfg, key, 5, 0, 40) == 1);
    assert(be_time_micro(&cfg, key, 5, 17, 0) == 1 && be_time_micro(&cfg, key, 5, 17, 40) == -3);
    for (unsigned s = 0; s < GM_STEPS; ++s) {                       /* step 0 is never negative, the range holds */
        int8_t m = be_time_micro(&cfg, key, 2, s, 0);
        assert(m >= -8 && m <= 8 && (s != 0u || m >= 0));
    }
    /* the TIME key alone decides: the config's own mutation counter must not matter */
    gm_config other = cfg;
    other.mutation = 99u;
    for (unsigned s = 0; s < GM_STEPS; ++s)
        assert(be_time_micro(&cfg, key, 2, s, 0) == be_time_micro(&other, key, 2, s, 0));
    assert(cfg.mutation == 0u && cfg.seed == UINT32_C(0xC0FFEE));   /* the caller's config is untouched */
}

int main(void)
{
    merge_fixture();
    invalid_inputs();
    keys();
    neutral_recipe();
    time_micro();
    return 0;
}
