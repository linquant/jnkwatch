#include "face_actions.h"
#include "theme.h"
#include "actions.h"
#include "display.h"   /* watch_send_action() */
#include <stdint.h>

/* One 2x3 page of shortcut buttons on the round screen. Columns straddle the
 * centre and rows stay in the circle's wide band so nothing hits the glass. */
#define BTN_SZ    92
#define FLASH_MS  180
#define COL_FILL  lv_color_hex(0x0C1418) /* rest button fill / flashed glyph  */
#define COL_CAP   lv_color_hex(0xB8C2C8) /* caption grey                      */

/* 2x3 grid on the 466 screen; rows spaced wider than a cell so a caption never
 * overlaps the button of the next row. */
static const int CELL_DX[2] = {-96, 96};
static const int CELL_DY[3] = {-140, 0, 140};

static wf_actions_t model;
static lv_obj_t *btn[WF_ACTIONS_PER_PAGE];
static lv_obj_t *ico[WF_ACTIONS_PER_PAGE];
static uint32_t  flash_until[WF_ACTIONS_PER_PAGE];
static bool      flashing[WF_ACTIONS_PER_PAGE];
static int       nbtn;

/* core is LVGL-free, so the icon enum maps to a glyph here. */
static const char *icon_symbol(wf_icon_t ic)
{
    switch (ic) {
        case WF_ICON_PHONE:  return LV_SYMBOL_GPS;
        case WF_ICON_CAMERA: return LV_SYMBOL_IMAGE;
        case WF_ICON_MUSIC:  return LV_SYMBOL_PLAY;
        case WF_ICON_HOME:   return LV_SYMBOL_HOME;
        case WF_ICON_BELL:   return LV_SYMBOL_BELL;
        case WF_ICON_VOLUME: return LV_SYMBOL_MUTE;
        case WF_ICON_POWER:  return LV_SYMBOL_POWER;
        case WF_ICON_GENERIC:return LV_SYMBOL_SETTINGS;
        default:             return "";
    }
}

static void set_flashed(int i, bool on)
{
    lv_color_t a = theme_accent();
    if (on) {
        lv_obj_set_style_bg_color(btn[i], a, 0);
        lv_obj_set_style_text_color(ico[i], COL_FILL, 0);
    } else {
        lv_obj_set_style_bg_color(btn[i], COL_FILL, 0);
        lv_obj_set_style_text_color(ico[i], a, 0);
    }
    flashing[i] = on;
}

static void on_action(lv_event_t *e)
{
    lv_obj_t *b = lv_event_get_current_target(e);
    int i = (int)(intptr_t)lv_obj_get_user_data(b);
    const wf_action_t *a = wf_actions_get(&model, i);
    if (!a || !a->enabled) return;

    watch_send_action(a->id);      /* the BLE signal (stubbed on PC) */
    flash_until[i] = lv_tick_get() + FLASH_MS;
    set_flashed(i, true);          /* immediate "sent" confirmation */
}

void face_actions_create(lv_obj_t *p)
{
    wf_actions_init(&model);
    lv_color_t a = theme_accent();

    int n = wf_actions_count(&model);
    if (n > WF_ACTIONS_PER_PAGE) n = WF_ACTIONS_PER_PAGE; /* one page for now */
    nbtn = n;

    for (int i = 0; i < n; i++) {
        const wf_action_t *act = wf_actions_get(&model, i);
        int col = i % 2, row = i / 2;

        /* Transparent cell: icon button stacked over its caption. */
        lv_obj_t *cell = lv_obj_create(p);
        lv_obj_remove_style_all(cell);
        lv_obj_set_size(cell, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_set_style_pad_row(cell, 6, 0);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(cell, LV_ALIGN_CENTER, CELL_DX[col], CELL_DY[row]);

        lv_obj_t *b = lv_button_create(cell);
        lv_obj_set_size(b, BTN_SZ, BTN_SZ);
        lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(b, COL_FILL, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(b, a, 0);
        lv_obj_set_style_border_width(b, 2, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_user_data(b, (void *)(intptr_t)i);
        lv_obj_add_event_cb(b, on_action, LV_EVENT_CLICKED, NULL);
        if (!act->enabled) lv_obj_add_state(b, LV_STATE_DISABLED);

        lv_obj_t *g = lv_label_create(b);
        lv_obj_set_style_text_font(g, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(g, a, 0);
        lv_label_set_text(g, icon_symbol(act->icon));
        lv_obj_center(g);

        lv_obj_t *cap = lv_label_create(cell);
        lv_obj_set_style_text_font(cap, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(cap, COL_CAP, 0);
        lv_label_set_text(cap, act->label);

        btn[i] = b;
        ico[i] = g;
        flashing[i] = false;
    }
}

void face_actions_tick(void)
{
    uint32_t now = lv_tick_get();
    for (int i = 0; i < nbtn; i++)
        if (flashing[i] && now >= flash_until[i])
            set_flashed(i, false); /* end the flash */
}

void face_actions_apply_accent(void)
{
    lv_color_t a = theme_accent();
    for (int i = 0; i < nbtn; i++) {
        lv_obj_set_style_border_color(btn[i], a, 0);
        if (!flashing[i]) lv_obj_set_style_text_color(ico[i], a, 0);
    }
}
