/* Headless: render each face and report the black/lit pixel ratio inside the
 * round panel — a proxy for AMOLED power. With --check, fail (exit 1) if any
 * face's lit fraction exceeds its budget (regression guard against a bright
 * background creeping back). Needs no display server. */
#include "lvgl.h"
#include "display.h"
#include "ui.h"
#include "theme.h"
#include "pixelstats.h"
#include <stdio.h>
#include <string.h>

#define THRESH 12 /* max-channel value at/below which a pixel counts as black */

static const char *FACE_NAME[7] = {
    "actions", "notif", "clock", "aura", "timer", "compass", "settings"
};

/* Per-face lit-fraction budget (%). Calibrated from measured values + margin
 * (measured 2026-09-22: actions 25, notif 5.5, clock 6.3, aura 5.3, timer 22.7,
 * compass 7.6, settings 11.6). The guard exists to catch a big regression
 * (e.g. a bright background → lit ~100%), so budgets keep comfortable headroom;
 * aura is animated (moving particles), hence looser. */
static const double BUDGET[7] = {
    /*actions*/ 32.0, /*notif*/ 12.0, /*clock*/ 12.0, /*aura*/ 16.0,
    /*timer*/ 30.0, /*compass*/ 15.0, /*settings*/ 18.0
};

static void face_stats(int face, wf_pix_stats_t *s)
{
    ui_show_face(face);
    for (int i = 0; i < 4; i++) { ui_tick(); lv_timer_handler(); }
    lv_refr_now(NULL);
    wf_pix_stats_rgb565(watch_framebuffer(), WATCH_SIZE, WATCH_SIZE, 1, THRESH, s);
}

int main(int argc, char **argv)
{
    int check = (argc > 1 && strcmp(argv[1], "--check") == 0);

    theme_set_accent(ACCENT_CYAN); /* fixed teinte for a stable report */
    lv_init();
    lv_tick_set_cb(watch_millis);
    watch_display_init();
    ui_init();
    for (int i = 0; i < 3; i++) { ui_tick(); lv_timer_handler(); } /* settle */

    if (check) printf("AMOLED lit-pixel budget check (round panel, thresh=%d)\n", THRESH);
    else       printf("Face pixel usage (round panel, thresh=%d)\n", THRESH);

    int fail = 0;
    for (int f = 0; f < 7; f++) {
        wf_pix_stats_t s;
        face_stats(f, &s);
        double litpct = s.lit_ratio * 100.0;
        if (check) {
            int over = litpct > BUDGET[f];
            printf("  %-9s  lit %5.1f%%   budget %5.1f%%   %s\n",
                   FACE_NAME[f], litpct, BUDGET[f], over ? "FAIL" : "ok");
            if (over) fail = 1;
        } else {
            printf("  %-9s  black %5.1f%%   lit %5.1f%%   load %5.1f%%\n",
                   FACE_NAME[f], s.black_ratio * 100.0, litpct, s.avg_level * 100.0);
        }
    }

    if (check && fail) {
        fprintf(stderr, "amoled_budget: at least one face exceeds its lit budget\n");
        return 1;
    }
    return 0;
}
