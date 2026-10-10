#include "face_apps.h"
#include "theme.h"
#include "ui.h"          /* ui_goto_tile */
#include <stdint.h>

/* Launcher menu. The apps live on the same column as this tile (col 6): the
 * level is the sub-tile directly below (row 1), the pedometer is row 2. Tapping
 * a name jumps there; swiping up from the app returns here. (Sondes is a
 * top-level watchface left of the clock, not an app here.) */

#define APPS_COL 6
#define LEVEL_ROW 1
#define STEPS_ROW 2

static lv_obj_t *btn_level, *btn_steps;

static void on_level(lv_event_t *e)
{
    (void)e;
    ui_goto_tile(APPS_COL, LEVEL_ROW);
}

static void on_steps(lv_event_t *e)
{
    (void)e;
    ui_goto_tile(APPS_COL, STEPS_ROW);
}

static lv_obj_t *app_button(lv_obj_t *parent, const char *txt, bool enabled)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_size(b, 300, 76);
    lv_obj_set_style_radius(b, 18, 0);
    /* Outline only (no fill) — keeps the AMOLED background truly black. */
    lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_border_color(b, enabled ? theme_accent() : COL_TRACK, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);

    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(l, enabled ? COL_CORE : COL_LABEL, 0);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    return b;
}

void face_apps_create(lv_obj_t *p)
{
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *col = lv_obj_create(p);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, 360, LV_SIZE_CONTENT);
    lv_obj_center(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 24, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(col);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title, 4, 0);
    lv_label_set_text(title, "APPLICATIONS");

    btn_level = app_button(col, "Niveau a bulle", true);
    lv_obj_add_event_cb(btn_level, on_level, LV_EVENT_CLICKED, NULL);

    btn_steps = app_button(col, "Podometre", true);
    lv_obj_add_event_cb(btn_steps, on_steps, LV_EVENT_CLICKED, NULL);
}

void face_apps_apply_accent(void)
{
    if (btn_level)
        lv_obj_set_style_border_color(btn_level, theme_accent(), 0);
    if (btn_steps)
        lv_obj_set_style_border_color(btn_steps, theme_accent(), 0);
}
