#pragma once
/* Pure pixel accounting for a rendered frame: how many pixels are black
 * (unlit) vs lit (coloured), plus an average light level. On an AMOLED the
 * unlit pixels draw ~0, so these ratios are a proxy for panel power draw and a
 * regression guard against a bright background creeping back in.
 * NO LVGL/platform dependency; unit-tested (tests/test_pixelstats.c). */
#include <stdint.h>

typedef struct {
    long   total;        /* pixels counted (inside the round mask if enabled) */
    long   black;        /* max(R,G,B) <= thresh  → considered unlit          */
    long   lit;          /* total - black         → coloured/lit              */
    double black_ratio;  /* black / total, 0..1                               */
    double lit_ratio;    /* lit / total, 0..1                                 */
    double avg_level;    /* mean of (R+G+B)/3 / 255 over total, 0..1          */
} wf_pix_stats_t;

/* Is this RGB888 pixel "lit"? (max channel strictly above the threshold) */
int wf_pix_is_lit(uint8_t r, uint8_t g, uint8_t b, uint8_t thresh);

/* Accumulate stats over an RGB565 buffer of w*h pixels. When round_mask is
 * non-zero, only pixels inside the inscribed circle (radius w/2, centred) are
 * counted — the real round-panel area. thresh is the max-channel value at or
 * below which a pixel counts as black. */
void wf_pix_stats_rgb565(const uint16_t *buf, int w, int h,
                         int round_mask, uint8_t thresh, wf_pix_stats_t *out);
