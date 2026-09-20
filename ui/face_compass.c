#include "face_compass.h"
#include "theme.h"
#include "geometry.h"
#include "fonts.h"     /* wf_digits_100 */
#include "compass.h"
#include "display.h"   /* watch_heading_deg() */
#include <math.h>
#include <stdio.h>

#define CX 233.0
#define CY 233.0
#define D2R (3.14159265358979323846 / 180.0)
#define HDG_Y (-10)
#define NGLOW 8   /* fewer glow copies than the clock (updates every frame) */
#define NCARD 8   /* N E S W + NE SE SW NW */

static lv_obj_t *rose;
static uint8_t rose_buf[LV_CANVAS_BUF_SIZE(WATCH_SIZE, WATCH_SIZE, 16, 1)] __attribute__((aligned(4)));
static lv_obj_t *card_lbl[NCARD];
static lv_obj_t *hglow[NGLOW], *hcore, *dir_lbl;
static double disp_h;

/* Cardinal + intercardinal names and their bearings. */
static const char *CARD_NAME[NCARD] = {"N", "E", "S", "W", "NE", "SE", "SW", "NW"};
static const double CARD_ANG[NCARD] = {0, 90, 180, 270, 45, 135, 225, 315};

static void polar(double deg, double r, double *x, double *y)
{
    double a = deg * D2R; /* clockwise from 12 o'clock */
    *x = CX + r * sin(a);
    *y = CY - r * cos(a);
}

static void set_heading_text(const char *s)
{
    for (int i = 0; i < NGLOW; i++) {
        lv_label_set_text(hglow[i], s);
        lv_obj_align(hglow[i], LV_ALIGN_CENTER, UI_GLOW_OFS[i][0], HDG_Y + UI_GLOW_OFS[i][1]);
    }
    lv_label_set_text(hcore, s);
    lv_obj_align(hcore, LV_ALIGN_CENTER, 0, HDG_Y);
}

static void draw_rose(double heading)
{
    lv_color_t accent = theme_accent();
    lv_canvas_fill_bg(rose, COL_BG, LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(rose, &layer);

    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.round_start = ld.round_end = 1;

    for (int d = 0; d < 360; d += 5) {
        int major = (d % 90 == 0);
        int medium = (d % 45 == 0);
        double ang = d - heading;                 /* rose rotates with heading */
        double r_in = major ? 170 : medium ? 182 : 192;
        double x1, y1, x2, y2;
        polar(ang, r_in, &x1, &y1);
        polar(ang, 205, &x2, &y2);
        ld.p1.x = x1; ld.p1.y = y1;
        ld.p2.x = x2; ld.p2.y = y2;
        ld.width = major ? 4 : medium ? 3 : 2;
        ld.color = major ? accent : COL_LABEL;
        ld.opa = major ? LV_OPA_COVER : medium ? 180 : 120;
        lv_draw_line(&layer, &ld);
    }

    /* Fixed heading pointer at 12 o'clock (accent), marks where you face. */
    lv_draw_triangle_dsc_t td;
    lv_draw_triangle_dsc_init(&td);
    td.bg_color = accent;
    td.bg_opa = LV_OPA_COVER;
    td.p[0].x = CX;       td.p[0].y = 58;
    td.p[1].x = CX - 16;  td.p[1].y = 22;
    td.p[2].x = CX + 16;  td.p[2].y = 22;
    lv_draw_triangle(&layer, &td);

    lv_canvas_finish_layer(rose, &layer);
}

void face_compass_create(lv_obj_t *p)
{
    disp_h = 0;

    rose = lv_canvas_create(p);
    lv_canvas_set_buffer(rose, rose_buf, WATCH_SIZE, WATCH_SIZE, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(rose);

    for (int i = 0; i < NCARD; i++) {
        card_lbl[i] = lv_label_create(p);
        /* main cardinals big, intercardinals smaller/dim */
        lv_obj_set_style_text_font(card_lbl[i],
            i < 4 ? &lv_font_montserrat_40 : &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(card_lbl[i], COL_LABEL, 0);
        lv_label_set_text(card_lbl[i], CARD_NAME[i]);
    }

    for (int i = 0; i < NGLOW; i++) {
        hglow[i] = lv_label_create(p);
        lv_obj_set_style_text_font(hglow[i], &wf_digits_100, 0);
        lv_obj_set_style_text_opa(hglow[i], 24, 0);
        lv_label_set_text(hglow[i], "");
    }
    hcore = lv_label_create(p);
    lv_obj_set_style_text_font(hcore, &wf_digits_100, 0);
    lv_obj_set_style_text_color(hcore, COL_CORE, 0);
    lv_label_set_text(hcore, "");

    dir_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(dir_lbl, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_letter_space(dir_lbl, 3, 0);
    lv_label_set_text(dir_lbl, "");
    lv_obj_align(dir_lbl, LV_ALIGN_CENTER, 0, 86);

    face_compass_apply_accent();
    face_compass_tick();
}

void face_compass_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_text_color(card_lbl[0], a, 0); /* N marker */
    for (int i = 0; i < NGLOW; i++)
        lv_obj_set_style_text_color(hglow[i], a, 0);
    lv_obj_set_style_text_color(dir_lbl, a, 0);
}

void face_compass_tick(void)
{
    double target = wf_heading_normalize(watch_heading_deg());
    /* Smooth motion toward the target across the 359->0 seam. */
    disp_h = wf_heading_normalize(disp_h + wf_angle_diff(disp_h, target) * 0.3);

    draw_rose(disp_h);

    for (int i = 0; i < NCARD; i++) {
        double ang = CARD_ANG[i] - disp_h;
        double x, y;
        polar(ang, 152, &x, &y);
        lv_obj_align(card_lbl[i], LV_ALIGN_CENTER, (int)lround(x - CX), (int)lround(y - CY));
    }

    static int last = -999;
    int hd = (int)lround(disp_h) % 360;
    if (hd != last) {
        last = hd;
        char b[8];
        snprintf(b, sizeof(b), "%d°", hd); /* degree sign */
        set_heading_text(b);
        lv_label_set_text(dir_lbl, wf_cardinal8(disp_h));
        lv_obj_align(dir_lbl, LV_ALIGN_CENTER, 0, 86);
    }
}
