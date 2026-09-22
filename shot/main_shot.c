/* Headless target: render one frame of the watchface and write it to a PNG.
 * Needs no display server and no SDL, so it runs anywhere (CI, this session). */
#include "lvgl.h"
#include "display.h"
#include "ui.h"
#include "theme.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static void rgb565_to_rgb888(const uint16_t *src, uint8_t *dst, int mask_round)
{
    for (int y = 0; y < WATCH_SIZE; y++) {
        for (int x = 0; x < WATCH_SIZE; x++) {
            int i = y * WATCH_SIZE + x;
            uint16_t p = src[i];
            uint8_t r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
            uint8_t g = (uint8_t)(((p >> 5) & 0x3F) * 255 / 63);
            uint8_t b = (uint8_t)((p & 0x1F) * 255 / 31);
            if (mask_round) {
                int dx = x - WATCH_SIZE / 2, dy = y - WATCH_SIZE / 2;
                if (dx * dx + dy * dy > (WATCH_SIZE / 2) * (WATCH_SIZE / 2))
                    r = g = b = 20; /* dim grey outside the round glass */
            }
            dst[i * 3 + 0] = r;
            dst[i * 3 + 1] = g;
            dst[i * 3 + 2] = b;
        }
    }
}

int main(int argc, char **argv)
{
    const char *out = (argc > 1) ? argv[1] : "frame.png";

    /* argv[2] is a face index, or "col,row" to preview a raw tileview cell. */
    int face = 0, col = -1, row = 0;
    if (argc > 2) {
        if (strchr(argv[2], ',')) sscanf(argv[2], "%d,%d", &col, &row);
        else face = atoi(argv[2]);
    }
    if (argc > 3) theme_set_accent((accent_t)atoi(argv[3])); /* teinte 0..5 */

    lv_init();
    lv_tick_set_cb(watch_millis);
    watch_display_init();
    ui_init();
    if (col >= 0) ui_goto_tile(col, row);
    else ui_show_face(face);

    /* Let a few frames settle, then force a full refresh into the framebuffer. */
    for (int i = 0; i < 3; i++) {
        ui_tick();
        lv_timer_handler();
    }
    lv_refr_now(NULL);

    static uint8_t rgb[WATCH_SIZE * WATCH_SIZE * 3];
    rgb565_to_rgb888(watch_framebuffer(), rgb, 1);

    if (!stbi_write_png(out, WATCH_SIZE, WATCH_SIZE, 3, rgb, WATCH_SIZE * 3)) {
        fprintf(stderr, "failed to write %s\n", out);
        return 1;
    }
    printf("wrote %s (%dx%d)\n", out, WATCH_SIZE, WATCH_SIZE);
    return 0;
}
