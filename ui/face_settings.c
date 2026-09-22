#include "face_settings.h"
#include "ui.h"          /* ui_apply_brightness(), ui_apply_accent() */
#include "theme.h"
#include "settings.h"
#include "display.h"     /* watch_set_wrist_wake() */
#include <stdint.h>
#include <stdio.h>

static wf_settings_t cfg;
static lv_obj_t *bright_lbl, *slider, *sw;
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
    char b[24];
    snprintf(b, sizeof(b), "LUMINOSITE  %d%%", cfg.brightness);
    lv_label_set_text(bright_lbl, b);
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

bool face_settings_wrist_wake(void)
{
    return cfg.wrist_wake;
}

int face_settings_sleep_ms(void)
{
    return cfg.sleep_ms;
}

void face_settings_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_bg_color(slider, a, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, a, LV_PART_INDICATOR | LV_STATE_CHECKED);
    /* Ring the currently-selected teinte swatch. */
    accent_t cur = theme_get_accent();
    for (int i = 0; i < ACCENT_COUNT; i++) {
        bool selected = (i == (int)cur);
        lv_obj_set_style_border_color(dot[i], lv_color_white(), 0);
        lv_obj_set_style_border_width(dot[i], selected ? 4 : 0, 0);
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

void face_settings_create(lv_obj_t *p)
{
    wf_settings_init(&cfg);

    /* Centered flex column holding every setting. */
    lv_obj_t *col = lv_obj_create(p);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, 360, LV_SIZE_CONTENT);
    lv_obj_center(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 16, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(col);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title, 6, 0);
    lv_label_set_text(title, "SETTINGS");

    /* Brightness */
    bright_lbl = lv_label_create(col);
    lv_obj_set_style_text_font(bright_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(bright_lbl, COL_CORE, 0);

    slider = lv_slider_create(col);
    lv_obj_set_size(slider, 280, 16);
    lv_slider_set_range(slider, WF_BRIGHT_MIN, WF_BRIGHT_MAX);
    lv_slider_set_value(slider, cfg.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, theme_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, COL_CORE, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, on_brightness, LV_EVENT_VALUE_CHANGED, NULL);

    /* Wrist wake row */
    lv_obj_t *wrow = row(col, 280, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_t *wl = lv_label_create(wrow);
    lv_obj_set_style_text_font(wl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(wl, COL_CORE, 0);
    lv_label_set_text(wl, "WRIST WAKE");

    sw = lv_switch_create(wrow);
    lv_obj_set_size(sw, 90, 46);
    lv_obj_set_style_bg_color(sw, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, theme_accent(), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, COL_CORE, LV_PART_KNOB);
    if (cfg.wrist_wake) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, on_wrist, LV_EVENT_VALUE_CHANGED, NULL);

    /* Screen-sleep timeout: 5s / 10s segmented control. */
    lv_obj_t *slrow = row(col, 280, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_t *sll = lv_label_create(slrow);
    lv_obj_set_style_text_font(sll, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(sll, COL_CORE, 0);
    lv_label_set_text(sll, "VEILLE");
    lv_obj_t *seg = row(slrow, 134, LV_FLEX_ALIGN_END, 10);
    static const char *SLEEP_TXT[2] = { "5s", "10s" };
    for (int i = 0; i < 2; i++) {
        sleep_btn[i] = lv_button_create(seg);
        lv_obj_set_size(sleep_btn[i], 62, 44);
        lv_obj_set_style_radius(sleep_btn[i], 10, 0);
        lv_obj_set_style_bg_opa(sleep_btn[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sleep_btn[i], 2, 0);
        lv_obj_set_style_shadow_width(sleep_btn[i], 0, 0);
        lv_obj_set_user_data(sleep_btn[i], (void *)(intptr_t)i);
        lv_obj_add_event_cb(sleep_btn[i], on_sleep, LV_EVENT_CLICKED, NULL);
        sleep_lbl[i] = lv_label_create(sleep_btn[i]);
        lv_obj_set_style_text_font(sleep_lbl[i], &lv_font_montserrat_24, 0);
        lv_label_set_text(sleep_lbl[i], SLEEP_TXT[i]);
        lv_obj_center(sleep_lbl[i]);
    }

    /* Teinte picker: one tappable swatch per accent. */
    lv_obj_t *trow = row(col, 300, LV_FLEX_ALIGN_CENTER, 12);
    for (int i = 0; i < ACCENT_COUNT; i++) {
        dot[i] = lv_obj_create(trow);
        lv_obj_remove_style_all(dot[i]);
        lv_obj_set_size(dot[i], 34, 34);
        lv_obj_set_style_radius(dot[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot[i], theme_accent_color((accent_t)i), 0);
        lv_obj_set_style_bg_opa(dot[i], LV_OPA_COVER, 0);
        lv_obj_set_user_data(dot[i], (void *)(intptr_t)i);
        lv_obj_add_flag(dot[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(dot[i], on_dot, LV_EVENT_CLICKED, NULL);
    }

    update_bright_label();
    face_settings_apply_accent();
    ui_apply_brightness(cfg.brightness);
    watch_set_wrist_wake(cfg.wrist_wake);
}
