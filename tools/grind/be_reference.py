#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent vectors for the BRUTAL EVOLUTION layer tests (tests/grind/test_be_layers.c)."""

M = 0xFFFFFFFF


def mix(v):
    v = ((v ^ (v >> 16)) * 0x7FEB352D) & M
    v = ((v ^ (v >> 15)) * 0x846CA68B) & M
    return v ^ (v >> 16)


def rnd(seed, mut, eng, alg, t, s, d):
    x = mix(seed ^ 0x9E3779B9)
    for k in (mut, eng * 65536 + alg, t * 256 + s, d):
        x = mix(x ^ k)
    return x


def micro(seed, key, alg, bpm, div, hum, swing, t, s):
    span = 600 * bpm * div // 240000
    span = min(span, 12)
    span = span * hum // 100
    span = span * (250 - swing) // 250
    r = rnd(seed, key, 1, alg, t, s, 4)
    return r % (span + 1) if s == 0 else (r % (2 * span + 1)) - span


print("key_next", [hex(mix(k ^ 0x9E3779B9)) for k in (0, 1, M, 0x12345678)])
for (t, s) in [(2, 0), (2, 1), (2, 5), (2, 9), (5, 0), (5, 17)]:
    print(t, s, micro(0xC0FFEE, 0xA5A5A5A5, 5, 200, 16, 100, 0, t, s),
          micro(0xC0FFEE, 0xA5A5A5A5, 5, 200, 16, 100, 40, t, s))
for t in (0, 7):
    print("def", t, [hex(mix(0xC0FFEE ^ mix(((t + 1) << 8) | (15 + l)))) for l in range(3)])
