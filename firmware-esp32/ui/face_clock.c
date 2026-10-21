#include "face_clock.h"
#include "theme.h"
#include "geometry.h"
#include "fonts.h"      /* wf_digits_100 */
#include "ui.h"         /* ui_apply_accent() */
#include "face_notif.h" /* face_notif_unread() */
#include "watchface_logic.h"
#include "weather.h"    /* wf_weather_t + condition codes */
#include "display.h"    /* watch_battery_pct() */
#include <time.h>
#include <stdio.h>
#include <math.h>

/* --- Dual Display watchface (466px native) ---------------------------------
 * Casio-style split screen: big LCD time on the left (HH / MM), a hairline
 * divider, and a status column on the right (date, battery, weather, steps,
 * unread bell). The accent teinte tints the hour and the bell. No seconds: a
 * per-second redraw + panel flush costs battery for little value on a watch.
 * No backlight wash — the AMOLED background stays truly black. */

#define COL_STATUS  lv_color_hex(0xC7D2D8) /* bright status text            */
#define COL_ICON    lv_color_hex(0x9AA6AD) /* muted icon grey (cloud)       */
#define COL_DIVIDER lv_color_hex(0x2A3138) /* hairline between the columns   */
#define COL_CHARGE  lv_color_hex(0x34C759) /* charging (bolt + battery)      */

static lv_obj_t *hh, *mm;              /* left LCD time                      */
static lv_obj_t *date_lbl;             /* "18 SEP"                           */
static lv_obj_t *batt_ico, *batt_lbl;  /* battery icon + "82%"              */
static lv_obj_t *batt_bolt;            /* lightning bolt, shown while charging */
static lv_obj_t *bell;                 /* unread-notification indicator      */
static lv_obj_t *step_lbl;             /* today's step count                 */

/* Weather (pushed from the phone; the watch only displays it). */
#define WXW 46
#define WXH 42
static lv_obj_t *wx_canvas, *wx_temp, *wx_loc;
static wf_weather_t wx;
static uint8_t wx_buf[LV_CANVAS_BUF_SIZE(WXW, WXH, 16, 1)] __attribute__((aligned(4)));

static lv_obj_t *make_box(lv_obj_t *p, lv_flex_flow_t flow, int gap)
{
    lv_obj_t *b = lv_obj_create(p);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(b, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(b, flow);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_style_pad_gap(b, gap, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    return b;
}

static lv_obj_t *make_label(lv_obj_t *p, const lv_font_t *font, lv_color_t col)
{
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, col, 0);
    lv_label_set_text(l, "");
    return l;
}

/* --- Weather condition icon, drawn on a small canvas --------------------- */
static void wx_fill(lv_layer_t *layer, lv_color_t c, int x, int y, int w, int h, int radius)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = c; d.bg_opa = LV_OPA_COVER; d.radius = radius;
    lv_area_t a = { x, y, x + w, y + h };
    lv_draw_rect(layer, &d, &a);
}

static void wx_line(lv_layer_t *layer, lv_color_t c, int x1, int y1, int x2, int y2, int w)
{
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.color = c; ld.width = w; ld.opa = LV_OPA_COVER;
    ld.round_start = ld.round_end = 1;
    ld.p1.x = x1; ld.p1.y = y1; ld.p2.x = x2; ld.p2.y = y2;
    lv_draw_line(layer, &ld);
}

/* A grey cloud (base + two puffs), its top at yoff. */
static void wx_cloud(lv_layer_t *layer, int yoff)
{
    lv_color_t g = COL_ICON;
    wx_fill(layer, g, 6, yoff + 12, 34, 12, LV_RADIUS_CIRCLE);
    wx_fill(layer, g, 8, yoff + 4, 18, 18, LV_RADIUS_CIRCLE);
    wx_fill(layer, g, 22, yoff + 1, 16, 16, LV_RADIUS_CIRCLE);
}

static void wx_sun(lv_layer_t *layer, int cx, int cy, int r, int rayIn, int rayOut)
{
    lv_color_t s = COL_LOW; /* amber */
    wx_fill(layer, s, cx - r, cy - r, 2 * r, 2 * r, LV_RADIUS_CIRCLE);
    for (int i = 0; i < 8; i++) {
        double a = i * 3.14159265 / 4.0;
        wx_line(layer, s, cx + (int)(rayIn * cos(a)), cy + (int)(rayIn * sin(a)),
                cx + (int)(rayOut * cos(a)), cy + (int)(rayOut * sin(a)), 2);
    }
}

static void draw_wx_icon(int cond)
{
    lv_canvas_fill_bg(wx_canvas, COL_BG, LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(wx_canvas, &layer);
    switch (cond) {
    case WF_WX_CLEAR:
        wx_sun(&layer, 23, 20, 9, 13, 18);
        break;
    case WF_WX_PARTLY:
        wx_sun(&layer, 15, 13, 6, 9, 13);
        wx_cloud(&layer, 9);
        break;
    case WF_WX_FOG:
        wx_cloud(&layer, 0);
        for (int i = 0; i < 3; i++) wx_line(&layer, COL_ICON, 8, 30 + i * 5, 38, 30 + i * 5, 2);
        break;
    case WF_WX_RAIN:
        wx_cloud(&layer, 0);
        for (int i = 0; i < 3; i++) wx_line(&layer, COL_GLOW, 13 + i * 10, 30, 10 + i * 10, 40, 2);
        break;
    case WF_WX_SNOW:
        wx_cloud(&layer, 0);
        for (int i = 0; i < 3; i++) wx_fill(&layer, COL_CORE, 11 + i * 11, 33, 4, 4, LV_RADIUS_CIRCLE);
        break;
    case WF_WX_STORM:
        wx_cloud(&layer, 0);
        wx_line(&layer, COL_LOW, 24, 27, 18, 36, 3);
        wx_line(&layer, COL_LOW, 18, 36, 26, 32, 3);
        wx_line(&layer, COL_LOW, 26, 32, 20, 41, 3);
        break;
    case WF_WX_CLOUDY:
    default:
        wx_cloud(&layer, 4);
        break;
    }
    lv_canvas_finish_layer(wx_canvas, &layer);
}

/* Render the current weather store into the row (default until first push). */
static void wx_render(void)
{
    if (wx.valid) {
        char t[8];
        snprintf(t, sizeof(t), "%d\xC2\xB0", wx.temp_c);
        lv_label_set_text(wx_temp, t);
        lv_label_set_text(wx_loc, wx.location);
        draw_wx_icon(wx.cond);
    } else {
        lv_label_set_text(wx_temp, "--\xC2\xB0");
        lv_label_set_text(wx_loc, "");
        draw_wx_icon(WF_WX_CLOUDY);
    }
}

void face_clock_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_text_color(hh, a, 0);
    lv_obj_set_style_text_color(bell, a, 0);
}

/* Tap the time to cycle teintes (sim/touch); the whole UI follows. */
static void on_cycle(lv_event_t *e)
{
    (void)e;
    theme_set_accent((accent_t)(theme_get_accent() + 1));
    ui_apply_accent();
}

void face_clock_create(lv_obj_t *p)
{
    /* Subtle inner bezel ring (no backlight wash — keep the AMOLED truly black). */
    lv_obj_t *ring = lv_obj_create(p);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, 434, 434);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ring, 1, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(0x141A20), 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_60, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *div = lv_obj_create(p);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 2, 230);
    lv_obj_center(div);
    lv_obj_set_style_bg_color(div, COL_DIVIDER, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);

    /* Left column: big LCD time. */
    lv_obj_t *left = make_box(p, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_align(left, LV_ALIGN_LEFT_MID, 52, 0);
    hh = make_label(left, &wf_digits_100, theme_accent());
    mm = make_label(left, &wf_digits_100, COL_CORE);
    lv_obj_add_flag(left, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(left, on_cycle, LV_EVENT_CLICKED, NULL);

    /* Right column: status stack. */
    lv_obj_t *right = make_box(p, LV_FLEX_FLOW_COLUMN, 18);
    lv_obj_align(right, LV_ALIGN_LEFT_MID, 250, 0);

    date_lbl = make_label(right, &lv_font_montserrat_34, COL_STATUS);
    lv_obj_set_style_text_letter_space(date_lbl, 1, 0);

    lv_obj_t *batt_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    batt_bolt = make_label(batt_row, &lv_font_montserrat_28, COL_CHARGE);
    lv_label_set_text(batt_bolt, LV_SYMBOL_CHARGE);
    lv_obj_add_flag(batt_bolt, LV_OBJ_FLAG_HIDDEN);
    batt_ico = make_label(batt_row, &lv_font_montserrat_28, COL_STATUS);
    lv_label_set_text(batt_ico, LV_SYMBOL_BATTERY_3);
    batt_lbl = make_label(batt_row, &lv_font_montserrat_28, COL_STATUS);

    lv_obj_t *wx_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    wx_canvas = lv_canvas_create(wx_row);
    lv_canvas_set_buffer(wx_canvas, wx_buf, WXW, WXH, LV_COLOR_FORMAT_RGB565);
    lv_obj_remove_flag(wx_canvas, LV_OBJ_FLAG_CLICKABLE);
    wx_temp = make_label(wx_row, &lv_font_montserrat_28, COL_STATUS);
    wx_loc = make_label(right, &lv_font_montserrat_20, COL_ICON); /* small location */
    wf_weather_init(&wx);
    wx_render();

    lv_obj_t *step_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    lv_obj_t *step_ico = make_label(step_row, &lv_font_montserrat_28, COL_ICON);
    lv_label_set_text(step_ico, LV_SYMBOL_LOOP);
    step_lbl = make_label(step_row, &lv_font_montserrat_28, COL_STATUS);
    lv_label_set_text(step_lbl, "0"); /* filled from ui_steps_today() each tick */

    /* Unread-notification bell; hidden when nothing unread (flex reclaims). */
    bell = make_label(right, &lv_font_montserrat_28, theme_accent());
    lv_obj_set_style_text_letter_space(bell, 1, 0);
    lv_obj_add_flag(bell, LV_OBJ_FLAG_HIDDEN);

    face_clock_apply_accent();
    face_clock_tick();
}

void face_clock_tick(void)
{
    struct tm lt;
    watch_now(&lt);

    static int last_min = -1;
    if (lt.tm_min != last_min) {
        last_min = lt.tm_min;
        char h[4], m[4], d[12];
        snprintf(h, sizeof(h), "%02d", lt.tm_hour);
        snprintf(m, sizeof(m), "%02d", lt.tm_min);
        lv_label_set_text(hh, h);
        lv_label_set_text(mm, m);
        wf_format_date(d, sizeof(d), lt.tm_mday, lt.tm_mon);
        lv_label_set_text(date_lbl, d);
    }

    static int last_pct = -2, last_chg = -1;
    int pct = watch_battery_pct();          /* -1 = unknown */
    int chg = watch_is_charging() ? 1 : 0;
    if (pct != last_pct || chg != last_chg) {
        last_pct = pct;
        last_chg = chg;
        static const char *const icons[] = {
            LV_SYMBOL_BATTERY_EMPTY, LV_SYMBOL_BATTERY_1, LV_SYMBOL_BATTERY_2,
            LV_SYMBOL_BATTERY_3, LV_SYMBOL_BATTERY_FULL,
        };
        lv_label_set_text(batt_ico, icons[wf_battery_bars(pct < 0 ? 100 : pct)]);
        if (pct >= 0) {
            char b[12];
            snprintf(b, sizeof(b), "%d%%", pct);
            lv_label_set_text(batt_lbl, b);
            lv_obj_remove_flag(batt_lbl, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(batt_lbl, LV_OBJ_FLAG_HIDDEN);   /* level unknown */
        }
        if (chg) lv_obj_remove_flag(batt_bolt, LV_OBJ_FLAG_HIDDEN);
        else     lv_obj_add_flag(batt_bolt, LV_OBJ_FLAG_HIDDEN);

        lv_color_t bc;
        if (chg) bc = COL_CHARGE;
        else switch (wf_battery_level(pct)) {
            case WF_BATT_CRIT: bc = COL_CRIT; break;
            case WF_BATT_LOW:  bc = COL_LOW;  break;
            default:           bc = COL_STATUS; break;
        }
        lv_obj_set_style_text_color(batt_ico, bc, 0);
        lv_obj_set_style_text_color(batt_lbl, bc, 0);
    }

    static int last_unread = -1;
    int unread = face_notif_unread();
    if (unread != last_unread) {
        last_unread = unread;
        if (unread <= 0) {
            lv_obj_add_flag(bell, LV_OBJ_FLAG_HIDDEN);
        } else {
            char nb[16];
            if (unread > 1) snprintf(nb, sizeof(nb), LV_SYMBOL_BELL " %d", unread);
            else            snprintf(nb, sizeof(nb), LV_SYMBOL_BELL);
            lv_label_set_text(bell, nb);
            lv_obj_remove_flag(bell, LV_OBJ_FLAG_HIDDEN);
        }
    }

    static uint32_t last_steps = UINT32_MAX;
    uint32_t steps = ui_steps_today();
    if (steps != last_steps) {
        last_steps = steps;
        char s[16];
        wf_format_steps(s, sizeof(s), steps);
        lv_label_set_text(step_lbl, s);
    }
}

void face_clock_set_weather(int temp_c, int cond, const char *location)
{
    wf_weather_set(&wx, temp_c, cond, location);
    wx_render();
}
