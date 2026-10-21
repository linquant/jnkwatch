#include "face_level.h"
#include "theme.h"
#include "geometry.h"
#include "level.h"       /* wf_level_compute (pure, unit-tested) */
#include "display.h"     /* watch_read_accel */
#include <math.h>
#include <stdio.h>

/* --- Bubble level -----------------------------------------------------------
 * The accelerometer gives the gravity vector; core/level.c turns it into a ball
 * offset in the unit disk (rolls to the low side) plus a total tilt angle. Here
 * we just place a ball inside a ring and colour it green when level. A small EMA
 * steadies the jitter. Physically it is the accelerometer, not the gyro, that
 * yields absolute tilt. */

#define RING_R    182   /* outer ring radius (px)            */
#define TRAVEL_R  150   /* max ball travel from the centre   */
#define BALL_D    56    /* ball diameter                     */
#define TARGET_D  76    /* centre "level" target ring        */

#define COL_LEVEL lv_color_hex(0x27E36B) /* green when flat */

static lv_obj_t *ring, *target, *ball, *angle_lbl, *title_lbl;
static double sx, sy;   /* EMA-smoothed ball offset */

void face_level_create(lv_obj_t *p)
{
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    ring = lv_obj_create(p);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, RING_R * 2, RING_R * 2);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, COL_TRACK, 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    target = lv_obj_create(p);
    lv_obj_remove_style_all(target);
    lv_obj_set_size(target, TARGET_D, TARGET_D);
    lv_obj_center(target);
    lv_obj_set_style_radius(target, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(target, 2, 0);
    lv_obj_set_style_border_color(target, COL_LABEL, 0);
    lv_obj_remove_flag(target, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ball = lv_obj_create(p);
    lv_obj_remove_style_all(ball);
    lv_obj_set_size(ball, BALL_D, BALL_D);
    lv_obj_center(ball);
    lv_obj_set_style_radius(ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ball, theme_accent(), 0);
    lv_obj_set_style_bg_opa(ball, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ball, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    title_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_lbl, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title_lbl, 4, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 60);
    lv_label_set_text(title_lbl, "NIVEAU");

    angle_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(angle_lbl, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(angle_lbl, COL_CORE, 0);
    lv_obj_align(angle_lbl, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_label_set_text(angle_lbl, "0.0\xC2\xB0");

    sx = sy = 0.0;
    face_level_tick();
}

void face_level_apply_accent(void)
{
    if (ball) lv_obj_set_style_bg_color(ball, theme_accent(), 0);
}

void face_level_tick(void)
{
    if (!ball) return;

    double ax, ay, az;
    watch_read_accel(&ax, &ay, &az);
    wf_level_t l;
    wf_level_compute(ax, ay, az, &l);

    /* Steady the ball (the raw accel is jittery). */
    const double a = 0.25;
    sx += a * (l.bx - sx);
    sy += a * (l.by - sy);

    /* Keep the ball inside the travel disk. */
    double mx = sx, my = sy;
    double m2 = mx * mx + my * my;
    if (m2 > 1.0) { double k = 1.0 / sqrt(m2); mx *= k; my *= k; }
    lv_obj_align(ball, LV_ALIGN_CENTER, (int)(mx * TRAVEL_R), (int)(my * TRAVEL_R));

    lv_color_t c = l.level ? COL_LEVEL : theme_accent();
    lv_obj_set_style_bg_color(ball, c, 0);
    lv_obj_set_style_border_color(target, l.level ? COL_LEVEL : COL_LABEL, 0);

    char b[16];
    snprintf(b, sizeof(b), "%.1f\xC2\xB0", l.tilt_deg);
    lv_label_set_text(angle_lbl, b);
    lv_obj_set_style_text_color(angle_lbl, l.level ? COL_LEVEL : COL_CORE, 0);
}
