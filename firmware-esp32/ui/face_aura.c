#include "face_aura.h"
#include "theme.h"
#include "geometry.h"
#include "fonts.h"     /* wf_digits_100 */
#include "activity.h"
#include "watchface_logic.h"
#include "display.h"   /* watch_read_accel() */
#include <time.h>
#include <math.h>
#include <stdio.h>

#define NP 60          /* particle count */
#define R 210.0        /* round boundary radius (~UI_SAFE_R) */
#define AURA_SHAKE_TH   0.28  /* |‖a‖-1g| above this = a shake (rest ~0.08) */
#define AURA_FLASH_FRAMES 26  /* explosion flash / re-arm duration (frames)  */
#define CXf 233.0
#define CYf 233.0
#define TIME_Y (-8)
#define NGLOW 8

typedef struct { double x, y, vx, vy; } particle_t;

static particle_t ps[NP];
static lv_obj_t *canvas;
static uint8_t cbuf[LV_CANVAS_BUF_SIZE(WATCH_SIZE, WATCH_SIZE, 16, 1)] WF_BIG_BSS __attribute__((aligned(4)));
static lv_obj_t *glow[NGLOW], *core, *date_lbl;
static wf_activity_t act;
static uint32_t rng = 2463534242u;

static uint32_t xrnd(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}
static double frnd(void) { return (xrnd() % 100000) / 100000.0; }

static void set_time_text(const char *s)
{
    for (int i = 0; i < NGLOW; i++) {
        lv_label_set_text(glow[i], s);
        lv_obj_align(glow[i], LV_ALIGN_CENTER, UI_GLOW_OFS[i][0], TIME_Y + UI_GLOW_OFS[i][1]);
    }
    lv_label_set_text(core, s);
    lv_obj_align(core, LV_ALIGN_CENTER, 0, TIME_Y);
}

void face_aura_create(lv_obj_t *p)
{
    wf_activity_init(&act);

    canvas = lv_canvas_create(p);
    lv_canvas_set_buffer(canvas, cbuf, WATCH_SIZE, WATCH_SIZE, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(canvas);

    for (int i = 0; i < NP; i++) {
        double ang = frnd() * 6.2831853, r = frnd() * (R - 10);
        ps[i].x = CXf + r * cos(ang);
        ps[i].y = CYf + r * sin(ang);
        double vang = frnd() * 6.2831853, sp = 0.4 + frnd() * 0.6;
        ps[i].vx = sp * cos(vang);
        ps[i].vy = sp * sin(vang);
    }

    for (int i = 0; i < NGLOW; i++) {
        glow[i] = lv_label_create(p);
        lv_obj_set_style_text_font(glow[i], &wf_digits_100, 0);
        lv_obj_set_style_text_opa(glow[i], 40, 0);
        lv_label_set_text(glow[i], "");
    }
    core = lv_label_create(p);
    lv_obj_set_style_text_font(core, &wf_digits_100, 0);
    lv_obj_set_style_text_color(core, COL_CORE, 0);
    lv_label_set_text(core, "");

    date_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_letter_space(date_lbl, 2, 0);
    lv_label_set_text(date_lbl, "");
    lv_obj_align(date_lbl, LV_ALIGN_CENTER, 0, 86);

    face_aura_tick();
}

void face_aura_tick(void)
{
    double ax, ay, az;
    watch_read_accel(&ax, &ay, &az);
    double mag = sqrt(ax * ax + ay * ay + az * az);
    double dev = fabs(mag - 1.0);          /* dynamic motion energy, in g */
    double e = wf_activity_update(&act, ax, ay, az);

    /* Shake -> explosion. Resting noise is ~0.08g; a real shake exceeds ~0.5g.
     * `flash` counts down after a burst so a long shake fires bursts spaced out
     * (and drives the white flash + particle enlargement while it decays). */
    static int flash = 0;
    if (flash > 0) flash--;
    int boom = (dev > AURA_SHAKE_TH && flash == 0);
    if (boom) flash = AURA_FLASH_FRAMES;
    double burst = flash / (double)AURA_FLASH_FRAMES;   /* 1 -> 0 after a boom */

    /* In-plane gravity pulls the particles "downhill" so they pool into a block
     * on one side; -(in-plane a) is the downhill direction. During/just after an
     * explosion gravity is suppressed (gacc) so the scattered particles float
     * freely, then it fades back in to re-accumulate them. */
    double gx = -ax, gy = -ay;
    const double GACC = 0.8;    /* gravity strength (px/frame^2 per g) */
    const double DAMP = 0.92;   /* velocity damping (friction)         */
    double gacc = GACC * (1.0 - burst);
    double jitter = 0.25 + e * 2.0;

    /* Calm = pure accent; motion and the explosion flash brighten toward white. */
    int mix = (int)(e * 120 + burst * 200);
    if (mix > 255) mix = 255;
    lv_color_t col = lv_color_mix(lv_color_white(), theme_accent(), (uint8_t)mix);
    int psize = (int)(4 + e * 3 + burst * 5);
    int opv = (int)(90 + e * 100 + burst * 65);
    lv_opa_t popa = (lv_opa_t)(opv > 255 ? 255 : opv);

    lv_canvas_fill_bg(canvas, COL_BG, LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = col;
    d.bg_opa = popa;
    d.radius = LV_RADIUS_CIRCLE;

    for (int i = 0; i < NP; i++) {
        if (boom) {                         /* fling in a random direction */
            double ang = frnd() * 6.2831853;
            double spd = 12.0 + frnd() * 10.0;
            ps[i].vx = spd * cos(ang);
            ps[i].vy = spd * sin(ang);
        } else {                            /* settle under gravity + shimmer */
            ps[i].vx = ps[i].vx * DAMP + gx * gacc + (frnd() - 0.5) * jitter;
            ps[i].vy = ps[i].vy * DAMP + gy * gacc + (frnd() - 0.5) * jitter;
        }
        ps[i].x += ps[i].vx;
        ps[i].y += ps[i].vy;

        double dx = ps[i].x - CXf, dy = ps[i].y - CYf;
        double dist = sqrt(dx * dx + dy * dy);
        double lim = R - psize;
        if (dist > lim) {                   /* bounce off the round edge (lossy) */
            double nx = dx / dist, ny = dy / dist;
            double dot = ps[i].vx * nx + ps[i].vy * ny;
            ps[i].vx -= 1.6 * dot * nx;
            ps[i].vy -= 1.6 * dot * ny;
            ps[i].x = CXf + nx * lim;
            ps[i].y = CYf + ny * lim;
        }
        int x = (int)ps[i].x, y = (int)ps[i].y;
        lv_area_t a = { x, y, x + psize, y + psize };
        lv_draw_rect(&layer, &d, &a);
    }
    lv_canvas_finish_layer(canvas, &layer);

    /* Time: glow + date tinted by energy, bright white core. */
    for (int i = 0; i < NGLOW; i++)
        lv_obj_set_style_text_color(glow[i], col, 0);
    lv_obj_set_style_text_color(date_lbl, col, 0);

    static int last_min = -1;
    struct tm lt;
    watch_now(&lt);
    if (lt.tm_min != last_min) {
        last_min = lt.tm_min;
        char t[8], d[12], buf[20];
        wf_format_time(t, sizeof(t), lt.tm_hour, lt.tm_min);
        set_time_text(t);
        wf_format_date(d, sizeof(d), lt.tm_mday, lt.tm_mon);
        snprintf(buf, sizeof(buf), "%s  %s", wf_weekday_abbrev(lt.tm_wday), d);
        lv_label_set_text(date_lbl, buf);
        lv_obj_align(date_lbl, LV_ALIGN_CENTER, 0, 86);
    }
}
