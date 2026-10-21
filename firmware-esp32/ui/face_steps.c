#include "face_steps.h"
#include "theme.h"
#include "ui.h"              /* ui_steps_today() */
#include "watchface_logic.h" /* wf_format_steps() */
#include <stdint.h>

/* Daily goal driving the progress ring. */
#define STEP_GOAL 10000

static lv_obj_t *arc, *num_lbl;

void face_steps_create(lv_obj_t *p)
{
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    /* Progress ring: a 270° arc, open at the bottom. Non-interactive (no knob,
     * no click) so it never swallows the swipe that leaves the app. */
    arc = lv_arc_create(p);
    lv_obj_set_size(arc, 380, 380);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, STEP_GOAL);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, theme_accent(), LV_PART_INDICATOR);

    lv_obj_t *title = lv_label_create(p);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title, 6, 0);
    lv_label_set_text(title, "PAS");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -72);

    num_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(num_lbl, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(num_lbl, COL_CORE, 0);
    lv_label_set_text(num_lbl, "0");
    lv_obj_align(num_lbl, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t *goal = lv_label_create(p);
    lv_obj_set_style_text_font(goal, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(goal, COL_LABEL, 0);
    lv_label_set_text(goal, "/ 10 000");
    lv_obj_align(goal, LV_ALIGN_CENTER, 0, 64);

    face_steps_tick();
}

void face_steps_tick(void)
{
    static uint32_t last = UINT32_MAX;
    uint32_t s = ui_steps_today();
    if (s == last) return;
    last = s;

    char buf[16];
    wf_format_steps(buf, sizeof buf, s);
    lv_label_set_text(num_lbl, buf);

    lv_arc_set_value(arc, (int)(s > STEP_GOAL ? STEP_GOAL : s));
    lv_obj_set_style_arc_color(arc, s >= STEP_GOAL ? COL_OK : theme_accent(),
                               LV_PART_INDICATOR);
}

void face_steps_apply_accent(void)
{
    if (!arc) return;
    /* Keep the "goal reached" green; otherwise follow the accent. */
    if (ui_steps_today() < STEP_GOAL)
        lv_obj_set_style_arc_color(arc, theme_accent(), LV_PART_INDICATOR);
}
