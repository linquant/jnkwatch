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
#define CXf 233.0
#define CYf 233.0
#define TIME_Y (-8)
#define NGLOW 8

typedef struct { double x, y, vx, vy; } particle_t;

static particle_t ps[NP];
static lv_obj_t *canvas;
static uint8_t cbuf[LV_CANVAS_BUF_SIZE(WATCH_SIZE, WATCH_SIZE, 16, 1)] __attribute__((aligned(4)));
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
    double e = wf_activity_update(&act, ax, ay, az);

    /* Stays in the current teinte: calm = pure accent, energetic = accent
     * brightened toward white so motion reads as heat without leaving the hue. */
    lv_color_t col = lv_color_mix(lv_color_white(), theme_accent(), (uint8_t)(e * 150));
    double speed = 0.4 + e * 3.5;
    int psize = (int)(4 + e * 7);
    lv_opa_t popa = (lv_opa_t)(70 + e * 185);

    lv_canvas_fill_bg(canvas, COL_BG, LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = col;
    d.bg_opa = popa;
    d.radius = LV_RADIUS_CIRCLE;

    for (int i = 0; i < NP; i++) {
        ps[i].x += ps[i].vx * speed;
        ps[i].y += ps[i].vy * speed;
        double dx = ps[i].x - CXf, dy = ps[i].y - CYf;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > R) {                     /* bounce off the round edge */
            double nx = dx / dist, ny = dy / dist;
            double dot = ps[i].vx * nx + ps[i].vy * ny;
            ps[i].vx -= 2 * dot * nx;
            ps[i].vy -= 2 * dot * ny;
            ps[i].x = CXf + nx * R;
            ps[i].y = CYf + ny * R;
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
