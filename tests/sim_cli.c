/* SPDX-License-Identifier: GPL-3.0-only
 * Drum machine fork: 2026 DEADACTIVE */
/* The simulator core as a line-driven process (tools/mcp/grind_mcp.py drives it): one command a line on stdin,
 * one JSON object a line on stdout. The same C the browser runs as wasm and tests/sim_test.c runs headless, with no
 * hardware and no device: nothing here can reach an FM-1. Commands:
 *   init <0|1>               power-on (1: fresh flash)
 *   run <frames>             advance the audio clock (the UI frames due run inside)
 *   btn <name|n> <0|1>       a panel button down / up        key <n> <0|1>     a note key (0..26)
 *   enc <name|n> <steps>     turn an encoder (+ clockwise)   master <0..1023>
 *   tap <name|n>             press, run 2048 frames, release, run 2048 frames
 *   state                    page, mode, selection, transport, message, the controller's commit count
 *   audio <frames>           advance and report the peak and RMS of what the firmware played
 *   shot <path>              the screen as a binary PPM (240x240)
 *   leds                     button and key LED masks */
#include "sim_core.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char *const btn_names[] = {"FX", "SCL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ",
                                        "PLAY", "REC", "OCTDN", "OCTUP"};
static const char *const enc_names[] = {"SELECT", "ALGO", "PRESET", "K1", "K2", "K3", "K4"};

static int lookup(const char *s, const char *const *names, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        if (!strcmp(s, names[i]))
            return (int)i;
    if (s[0] >= '0' && s[0] <= '9') {
        i = (uint32_t)strtoul(s, NULL, 10);
        return i < n ? (int)i : -1;
    }
    return -1;
}

static void run(uint32_t frames)
{
    while (frames) {
        uint32_t n = frames < SIM_MAX_FRAMES ? frames : SIM_MAX_FRAMES;
        sim_render(n);
        frames -= n;
    }
}

static void json_str(const char *s)
{
    putchar('"');
    for (; s && *s; s++) {
        if (*s == '"' || *s == '\\')
            printf("\\%c", *s);
        else if ((unsigned char)*s < 32u)
            printf("\\u%04x", (unsigned)(unsigned char)*s);
        else
            putchar(*s);
    }
    putchar('"');
}

static void ok(void) { puts("{\"ok\":true}"); }
static void err(const char *m) { printf("{\"ok\":false,\"error\":"); json_str(m); puts("}"); }

static void state(void)
{
    printf("{\"ok\":true,\"page\":%u,\"home\":%u,\"hmode\":%u,\"track\":%u,\"playing\":%u,\"ui_frames\":%u,\"msg\":",
           (unsigned)ui.page, (unsigned)ui.home, (unsigned)ui.hmode, (unsigned)song.sel, (unsigned)sim_playing(),
           (unsigned)felucca_dbg.ui_frames);
    json_str(ui.msg_t ? ui.msg : "");
#if GRIND_NATIVE
    printf(",\"grind\":true,\"commits\":%u,\"status\":%d", (unsigned)gm_runtime_commits(), (int)gm_runtime_status());
#else
    printf(",\"grind\":false");
#endif
    puts("}");
}

int main(void)
{
    char line[512];
    sim_init(1);
    run(4096);
    while (fflush(stdout), fgets(line, sizeof line, stdin)) {   /* (msvcrt ignores line buffering: flush before each wait) */
        char cmd[32] = "", a[300] = "", b[64] = "";
        int n;
        if (!strncmp(line, "shot ", 5)) {                      /* the rest of the line is the path: it may hold spaces */
            int cut = !strchr(line, '\n') && strlen(line) == sizeof line - 1u;   /* (fgets stopped before the end) */
            char *p = line + 5;
            size_t len = strlen(p);
            while (len && (p[len - 1] == '\n' || p[len - 1] == '\r' || p[len - 1] == ' '))
                p[--len] = 0;
            while (*p == ' ')
                p++, len--;
            if (!len) { err("shot needs a path"); continue; }
            if (cut || len >= sizeof a) { err("path too long"); continue; }
            memcpy(a, p, len + 1u);
            n = 2;
            strcpy(cmd, "shot");
        } else
            n = sscanf(line, "%31s %299s %63s", cmd, a, b);
        if (n < 1)
            continue;
        if (!strcmp(cmd, "init")) {
            sim_init(n > 1 ? atoi(a) : 1);
            run(4096);
            ok();
        } else if (!strcmp(cmd, "run") && n > 1) {
            run((uint32_t)strtoul(a, NULL, 10));
            ok();
        } else if (!strcmp(cmd, "btn") && n > 2) {
            int l = lookup(a, btn_names, NB);
            if (l < 0) { err("unknown button"); continue; }
            sim_btn((uint32_t)l, atoi(b));
            ok();
        } else if (!strcmp(cmd, "tap") && n > 1) {
            int l = lookup(a, btn_names, NB);
            if (l < 0) { err("unknown button"); continue; }
            sim_btn((uint32_t)l, 1);
            run(2048);
            sim_btn((uint32_t)l, 0);
            run(2048);
            ok();
        } else if (!strcmp(cmd, "key") && n > 2) {
            int k = atoi(a);
            if (k < 0 || k > 26) { err("key out of range 0..26"); continue; }
            sim_key((uint32_t)k, atoi(b));
            ok();
        } else if (!strcmp(cmd, "enc") && n > 2) {
            int e = lookup(a, enc_names, NE);
            if (e < 0) { err("unknown encoder"); continue; }
            sim_enc((uint32_t)e, atoi(b));
            run(2048);
            ok();
        } else if (!strcmp(cmd, "master") && n > 1) {
            sim_master((uint32_t)strtoul(a, NULL, 10));
            ok();
        } else if (!strcmp(cmd, "state")) {
            state();
        } else if (!strcmp(cmd, "audio") && n > 1) {
            uint32_t frames = (uint32_t)strtoul(a, NULL, 10), total = 0;
            double sq = 0;
            float pk = 0;
            while (frames) {
                uint32_t m = frames < SIM_MAX_FRAMES ? frames : SIM_MAX_FRAMES, i;
                const float *p;
                sim_render(m);
                p = sim_audio();
                for (i = 0; i < 2u * m; i++) {
                    float v = fabsf(p[i]);
                    pk = v > pk ? v : pk;
                    sq += (double)p[i] * p[i];
                }
                total += 2u * m;
                frames -= m;
            }
            printf("{\"ok\":true,\"peak\":%.6f,\"rms\":%.6f}\n", pk, total ? sqrt(sq / total) : 0.0);
        } else if (!strcmp(cmd, "shot") && n > 1) {
            FILE *f = fopen(a, "wb");
            const uint16_t *fbp = sim_fb();
            uint32_t i;
            int good;
            if (!f) { err("cannot write the file"); continue; }
            good = fprintf(f, "P6\n240 240\n255\n") > 0;
            for (i = 0; good && i < 240u * 240u; i++) {
                uint16_t c = fbp[i];
                uint8_t px[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63u) << 2), (uint8_t)((c & 31u) << 3)};
                good = fwrite(px, 1, 3, f) == 3u;
            }
            good = fclose(f) == 0 && good;                    /* (closed whatever happened; success only if every write was) */
            if (good)
                ok();
            else
                err("writing the screenshot failed");
        } else if (!strcmp(cmd, "leds")) {
            printf("{\"ok\":true,\"buttons\":%u,\"keys\":%u}\n", (unsigned)sim_leds(), (unsigned)sim_key_leds());
        } else if (!strcmp(cmd, "quit")) {
            break;
        } else {
            err("unknown command");
        }
    }
    return 0;
}
