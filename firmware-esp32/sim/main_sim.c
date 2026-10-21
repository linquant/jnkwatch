/* Interactive target: shows the watchface in an SDL2 window (round-masked)
 * and forwards mouse clicks as touch input. This is the one YOU run. */
#include "lvgl.h"
#include "display.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#define SCALE 1 /* 466x466 already fills a desktop window */

static int32_t g_mouse_x, g_mouse_y;
static bool g_mouse_down;

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = g_mouse_x;
    data->point.y = g_mouse_y;
    data->state = g_mouse_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

int main(void)
{
    lv_init();
    lv_tick_set_cb(watch_millis);
    watch_display_init();
    ui_init();

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("ESP32-S3 Watch (AMOLED 466x466)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WATCH_SIZE * SCALE, WATCH_SIZE * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, WATCH_SIZE, WATCH_SIZE);

    /* Round mask overlay (opaque black corners with a transparent circle). */
    SDL_Texture *mask = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, WATCH_SIZE, WATCH_SIZE);
    SDL_SetTextureBlendMode(mask, SDL_BLENDMODE_BLEND);
    {
        uint32_t *px = SDL_malloc(WATCH_SIZE * WATCH_SIZE * 4);
        int R = WATCH_SIZE / 2;
        for (int y = 0; y < WATCH_SIZE; y++)
            for (int x = 0; x < WATCH_SIZE; x++) {
                int dx = x - R, dy = y - R;
                px[y * WATCH_SIZE + x] =
                    (dx * dx + dy * dy > R * R) ? 0x000000FFu : 0x00000000u;
            }
        SDL_UpdateTexture(mask, NULL, px, WATCH_SIZE * 4);
        SDL_free(px);
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_MOUSEMOTION) {
                g_mouse_x = e.motion.x / SCALE;
                g_mouse_y = e.motion.y / SCALE;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) g_mouse_down = true;
            else if (e.type == SDL_MOUSEBUTTONUP) g_mouse_down = false;
        }

        ui_tick();
        lv_timer_handler();
        lv_refr_now(NULL);

        SDL_UpdateTexture(tex, NULL, watch_framebuffer(), WATCH_SIZE * 2);
        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderCopy(ren, mask, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_Delay(16); /* ~60 fps */
    }

    SDL_DestroyTexture(mask);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
