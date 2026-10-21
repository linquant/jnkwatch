#include "pixelstats.h"

/* RGB565 → 8-bit channels (same scaling as shot/main_shot.c). */
static void decode565(uint16_t p, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
    *g = (uint8_t)(((p >> 5) & 0x3F) * 255 / 63);
    *b = (uint8_t)((p & 0x1F) * 255 / 31);
}

int wf_pix_is_lit(uint8_t r, uint8_t g, uint8_t b, uint8_t thresh)
{
    uint8_t m = r > g ? r : g;
    if (b > m) m = b;
    return m > thresh;
}

void wf_pix_stats_rgb565(const uint16_t *buf, int w, int h,
                         int round_mask, uint8_t thresh, wf_pix_stats_t *out)
{
    long total = 0, black = 0;
    double sum_level = 0.0;
    int cx = w / 2, cy = h / 2;
    long r2 = (long)cx * cx; /* circle uses w/2 as radius, like the shot mask */

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (round_mask) {
                long dx = x - cx, dy = y - cy;
                if (dx * dx + dy * dy > r2) continue; /* outside the round panel */
            }
            uint8_t r, g, b;
            decode565(buf[(long)y * w + x], &r, &g, &b);
            total++;
            if (!wf_pix_is_lit(r, g, b, thresh)) black++;
            sum_level += ((double)r + g + b) / 3.0 / 255.0;
        }
    }

    out->total = total;
    out->black = black;
    out->lit = total - black;
    out->black_ratio = total ? (double)black / total : 0.0;
    out->lit_ratio = total ? (double)out->lit / total : 0.0;
    out->avg_level = total ? sum_level / total : 0.0;
}
