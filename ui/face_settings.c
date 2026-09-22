#include "face_settings.h"
#include "ui.h"          /* ui_apply_brightness(), ui_apply_accent() */
#include "theme.h"
#include "settings.h"
#include "display.h"     /* watch_set_wrist_wake() */
#include <stdint.h>
#include <stdio.h>

/* Settings is split into four vertical sub-screens (swipe up/down):
 *   0 LUMINOSITE   1 VEILLE   2 COULEURS   3 REVEIL POIGNET                */
#define NSUB 4

static wf_settings_t cfg;
static lv_obj_t *bright_val, *slider, *sw, *color_name;
static lv_obj_t *dot[ACCENT_COUNT];   /* teinte picker swatches */
static lv_obj_t *sleep_btn[2], *sleep_lbl[2];
static const int SLEEP_VAL[2] = { WF_SLEEP_5S, WF_SLEEP_10S };

static void update_sleep_sel(void)
{
    lv_color_t a = theme_accent();
    for (int i = 0; i < 2; i++) {
        int sel = (cfg.sleep_ms == SLEEP_VAL[i]);
        lv_obj_set_style_bg_color(sleep_btn[i], sel ? a : lv_color_hex(0x0C1418), 0);
        lv_obj_set_style_border_color(sleep_btn[i], a, 0);
        lv_obj_set_style_text_color(sleep_lbl[i], sel ? lv_color_hex(0x05070A) : a, 0);
    }
}

static void update_bright_label(void)
{
    char b[8];
    snprintf(b, sizeof(b), "%d%%", cfg.brightness);
    lv_label_set_text(bright_val, b);
}

static void on_brightness(lv_event_t *e)
{
    (void)e;
    wf_settings_set_brightness(&cfg, lv_slider_get_value(slider));
    update_bright_label();
    ui_apply_brightness(cfg.brightness);
}

static void on_wrist(lv_event_t *e)
{
    (void)e;
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    wf_settings_set_wrist_wake(&cfg, on);
    watch_set_wrist_wake(on);
}

static void on_dot(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    theme_set_accent((accent_t)i);
    ui_apply_accent();   /* re-tints every face, including these swatches */
}

static void on_sleep(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    wf_settings_set_sleep_ms(&cfg, SLEEP_VAL[i]);
    update_sleep_sel();
}

bool face_settings_wrist_wake(void) { return cfg.wrist_wake; }
int  face_settings_sleep_ms(void)   { return cfg.sleep_ms; }

void face_settings_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_bg_color(slider, a, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, a, LV_PART_INDICATOR | LV_STATE_CHECKED);
    /* Ring the currently-selected teinte swatch and name it. */
    accent_t cur = theme_get_accent();
    for (int i = 0; i < ACCENT_COUNT; i++) {
        bool selected = (i == (int)cur);
        lv_obj_set_style_border_color(dot[i], lv_color_white(), 0);
        lv_obj_set_style_border_width(dot[i], selected ? 4 : 0, 0);
    }
    if (color_name) {
        lv_label_set_text(color_name, theme_accent_name(cur));
        lv_obj_set_style_text_color(color_name, a, 0);
    }
    update_sleep_sel();
}

/* A transparent flex row of a given width and main-axis alignment. */
static lv_obj_t *row(lv_obj_t *p, int w, lv_flex_align_t main, int gap)
{
    lv_obj_t *r = lv_obj_create(p);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, w, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_style_pad_gap(r, gap, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    return r;
}

/* Centered flex column that holds one sub-screen's content. */
static lv_obj_t *sub_col(lv_obj_t *p)
{
    lv_obj_t *col = lv_obj_create(p);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, 360, LV_SIZE_CONTENT);
    lv_obj_center(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 22, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    return col;
}

static void sub_title(lv_obj_t *col, const char *txt)
{
    lv_obj_t *t = lv_label_create(col);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(t, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(t, 6, 0);
    lv_label_set_text(t, txt);
}

static lv_obj_t *caption(lv_obj_t *col, const char *txt)
{
    lv_obj_t *c = lv_label_create(col);
    lv_obj_set_style_text_font(c, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(c, lv_color_hex(0x6B7780), 0);
    lv_obj_set_style_text_align(c, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(c, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(c, 300);
    lv_label_set_text(c, txt);
    return c;
}

/* Four small dots on the right edge marking which sub-screen is shown. */
static void page_dots(lv_obj_t *p, int active)
{
    lv_obj_t *ind = lv_obj_create(p);
    lv_obj_remove_style_all(ind);
    lv_obj_set_size(ind, 14, LV_SIZE_CONTENT);
    lv_obj_align(ind, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_flex_flow(ind, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ind, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ind, 9, 0);
    lv_obj_remove_flag(ind, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < NSUB; i++) {
        lv_obj_t *dt = lv_obj_create(ind);
        lv_obj_remove_style_all(dt);
        int on = (i == active);
        lv_obj_set_size(dt, on ? 9 : 6, on ? 9 : 6);
        lv_obj_set_style_radius(dt, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dt, on ? COL_CORE : lv_color_hex(0x2A363C), 0);
        lv_obj_set_style_bg_opa(dt, LV_OPA_COVER, 0);
    }
}

void face_settings_create(lv_obj_t *p_bright, lv_obj_t *p_sleep,
                          lv_obj_t *p_color, lv_obj_t *p_wrist)
{
    wf_settings_init(&cfg);
    lv_obj_t *parents[NSUB] = { p_bright, p_sleep, p_color, p_wrist };
    for (int s = 0; s < NSUB; s++) page_dots(parents[s], s);

    /* --- 0 · LUMINOSITE --------------------------------------------------- */
    lv_obj_t *cb = sub_col(p_bright);
    sub_title(cb, "LUMINOSITE");
    bright_val = lv_label_create(cb);
    lv_obj_set_style_text_font(bright_val, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(bright_val, COL_CORE, 0);
    slider = lv_slider_create(cb);
    lv_obj_set_size(slider, 300, 18);
    lv_slider_set_range(slider, WF_BRIGHT_MIN, WF_BRIGHT_MAX);
    lv_slider_set_value(slider, cfg.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, theme_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, COL_CORE, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, on_brightness, LV_EVENT_VALUE_CHANGED, NULL);

    /* --- 1 · VEILLE ------------------------------------------------------- */
    lv_obj_t *cv = sub_col(p_sleep);
    sub_title(cv, "VEILLE");
    caption(cv, "Extinction de l'ecran");
    lv_obj_t *seg = row(cv, 240, LV_FLEX_ALIGN_CENTER, 24);
    static const char *SLEEP_TXT[2] = { "5s", "10s" };
    for (int i = 0; i < 2; i++) {
        sleep_btn[i] = lv_button_create(seg);
        lv_obj_set_size(sleep_btn[i], 100, 68);
        lv_obj_set_style_radius(sleep_btn[i], 14, 0);
        lv_obj_set_style_bg_opa(sleep_btn[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sleep_btn[i], 2, 0);
        lv_obj_set_style_shadow_width(sleep_btn[i], 0, 0);
        lv_obj_set_user_data(sleep_btn[i], (void *)(intptr_t)i);
        lv_obj_add_event_cb(sleep_btn[i], on_sleep, LV_EVENT_CLICKED, NULL);
        sleep_lbl[i] = lv_label_create(sleep_btn[i]);
        lv_obj_set_style_text_font(sleep_lbl[i], &lv_font_montserrat_28, 0);
        lv_label_set_text(sleep_lbl[i], SLEEP_TXT[i]);
        lv_obj_center(sleep_lbl[i]);
    }

    /* --- 2 · COULEURS ----------------------------------------------------- */
    lv_obj_t *cc = sub_col(p_color);
    sub_title(cc, "COULEURS");
    color_name = lv_label_create(cc);
    lv_obj_set_style_text_font(color_name, &lv_font_montserrat_28, 0);
    lv_obj_t *swrow = row(cc, 340, LV_FLEX_ALIGN_CENTER, 14);
    for (int i = 0; i < ACCENT_COUNT; i++) {
        dot[i] = lv_obj_create(swrow);
        lv_obj_remove_style_all(dot[i]);
        lv_obj_set_size(dot[i], 42, 42);
        lv_obj_set_style_radius(dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot[i], theme_accent_color((accent_t)i), 0);
        lv_obj_set_style_bg_opa(dot[i], LV_OPA_COVER, 0);
        lv_obj_set_user_data(dot[i], (void *)(intptr_t)i);
        lv_obj_add_flag(dot[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(dot[i], on_dot, LV_EVENT_CLICKED, NULL);
    }

    /* --- 3 · REVEIL POIGNET ---------------------------------------------- */
    lv_obj_t *cw = sub_col(p_wrist);
    sub_title(cw, "REVEIL POIGNET");
    sw = lv_switch_create(cw);
    lv_obj_set_size(sw, 110, 54);
    lv_obj_set_style_bg_color(sw, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, theme_accent(), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, COL_CORE, LV_PART_KNOB);
    if (cfg.wrist_wake) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, on_wrist, LV_EVENT_VALUE_CHANGED, NULL);
    caption(cw, "Lever le poignet pour allumer l'ecran");

    update_bright_label();
    face_settings_apply_accent();
    ui_apply_brightness(cfg.brightness);
    watch_set_wrist_wake(cfg.wrist_wake);
}
