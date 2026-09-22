#include "face_clock.h"
#include "theme.h"
#include "geometry.h"
#include "fonts.h"      /* wf_digits_100 */
#include "ui.h"         /* ui_apply_accent() */
#include "face_notif.h" /* face_notif_unread() */
#include "watchface_logic.h"
#include "display.h"    /* watch_battery_pct() */
#include <time.h>
#include <stdio.h>

/* --- Dual Display watchface (466px native) ---------------------------------
 * Casio-style split screen: big LCD time on the left (HH / MM / :SS), a
 * hairline divider, and a status column on the right (date, battery, weather,
 * steps, unread bell). The accent teinte tints the hour, seconds, bell and a
 * faint backlight wash. */

#define COL_STATUS  lv_color_hex(0xC7D2D8) /* bright status text            */
#define COL_ICON    lv_color_hex(0x9AA6AD) /* muted icon grey (cloud)       */
#define COL_DIVIDER lv_color_hex(0x2A3138) /* hairline between the columns   */

static lv_obj_t *wash;                 /* accent backlight tint (per teinte) */
static lv_obj_t *hh, *mm, *ss;         /* left LCD time                      */
static lv_obj_t *date_lbl;             /* "18 SEP"                           */
static lv_obj_t *batt_ico, *batt_lbl;  /* battery icon + "82%"              */
static lv_obj_t *bell;                 /* unread-notification indicator      */

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

/* A small grey cloud glyph (two puffs on a rounded base), 466-scaled. */
static void make_cloud(lv_obj_t *p)
{
    lv_obj_t *c = lv_obj_create(p);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, 42, 27);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *base = lv_obj_create(c);
    lv_obj_remove_style_all(base);
    lv_obj_set_size(base, 38, 14);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(base, COL_ICON, 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_align(base, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *p1 = lv_obj_create(c);
    lv_obj_remove_style_all(p1);
    lv_obj_set_size(p1, 23, 23);
    lv_obj_set_style_radius(p1, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(p1, COL_ICON, 0);
    lv_obj_set_style_bg_opa(p1, LV_OPA_COVER, 0);
    lv_obj_align(p1, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    lv_obj_t *p2 = lv_obj_create(c);
    lv_obj_remove_style_all(p2);
    lv_obj_set_size(p2, 19, 19);
    lv_obj_set_style_radius(p2, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(p2, COL_ICON, 0);
    lv_obj_set_style_bg_opa(p2, LV_OPA_COVER, 0);
    lv_obj_align(p2, LV_ALIGN_TOP_MID, 4, 0);
}

void face_clock_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_bg_color(wash, a, 0);
    lv_obj_set_style_text_color(hh, a, 0);
    lv_obj_set_style_text_color(ss, a, 0);
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
    /* Faint accent backlight wash + subtle inner bezel ring. */
    wash = lv_obj_create(p);
    lv_obj_remove_style_all(wash);
    lv_obj_set_size(wash, 410, 410);
    lv_obj_center(wash);
    lv_obj_set_style_radius(wash, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(wash, 26, 0);
    lv_obj_remove_flag(wash, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

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
    ss = make_label(left, &lv_font_montserrat_40, theme_accent());
    lv_obj_set_style_pad_top(ss, 8, 0);
    lv_obj_add_flag(left, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(left, on_cycle, LV_EVENT_CLICKED, NULL);

    /* Right column: status stack. */
    lv_obj_t *right = make_box(p, LV_FLEX_FLOW_COLUMN, 18);
    lv_obj_align(right, LV_ALIGN_LEFT_MID, 250, 0);

    date_lbl = make_label(right, &lv_font_montserrat_34, COL_STATUS);
    lv_obj_set_style_text_letter_space(date_lbl, 1, 0);

    lv_obj_t *batt_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    batt_ico = make_label(batt_row, &lv_font_montserrat_28, COL_STATUS);
    lv_label_set_text(batt_ico, LV_SYMBOL_BATTERY_3);
    batt_lbl = make_label(batt_row, &lv_font_montserrat_28, COL_STATUS);

    lv_obj_t *wx_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    make_cloud(wx_row);
    lv_obj_t *wx_lbl = make_label(wx_row, &lv_font_montserrat_28, COL_STATUS);
    lv_label_set_text(wx_lbl, "14\xC2\xB0"); /* placeholder: no weather source yet */

    lv_obj_t *step_row = make_box(right, LV_FLEX_FLOW_ROW, 10);
    lv_obj_t *step_ico = make_label(step_row, &lv_font_montserrat_28, COL_ICON);
    lv_label_set_text(step_ico, LV_SYMBOL_LOOP);
    lv_obj_t *step_lbl = make_label(step_row, &lv_font_montserrat_28, COL_STATUS);
    lv_label_set_text(step_lbl, "6 240"); /* placeholder: no step counter yet */

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

    static int last_sec = -1;
    if (lt.tm_sec != last_sec) {
        last_sec = lt.tm_sec;
        char s[8];
        snprintf(s, sizeof(s), ":%02d", lt.tm_sec);
        lv_label_set_text(ss, s);
    }

    static int last_pct = -1;
    int pct = watch_battery_pct();
    if (pct != last_pct) {
        last_pct = pct;
        char b[8];
        snprintf(b, sizeof(b), "%d%%", pct);
        lv_label_set_text(batt_lbl, b);
        lv_color_t bc;
        switch (wf_battery_level(pct)) {
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
}
