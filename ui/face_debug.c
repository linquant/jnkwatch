#include "face_debug.h"
#include "theme.h"
#include "ui.h"          /* ui_screen_on_ms(), ui_uptime_ms(), ui_screen_on_pct() */
#include "display.h"     /* watch_battery_pct() */
#include "screentime.h"  /* wf_screentime_format() */
#include "face_notif.h"  /* face_notif_unread() */
#include <stdio.h>

#define COL_LBL lv_color_hex(0x6B7780) /* dim caption */
#define COL_VAL lv_color_hex(0xC7D2D8) /* bright value */

static lv_obj_t *v_screen;  /* big screen-on time */
static lv_obj_t *v_uptime, *v_ratio, *v_batt, *v_unread;

static lv_obj_t *mklabel(lv_obj_t *p, const lv_font_t *f, lv_color_t c)
{
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_text(l, "");
    return l;
}

void face_debug_apply_accent(void)
{
    lv_obj_set_style_text_color(v_screen, theme_accent(), 0);
}

void face_debug_create(lv_obj_t *p)
{
    lv_obj_t *col = lv_obj_create(p);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, 340, LV_SIZE_CONTENT);
    lv_obj_center(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 10, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = mklabel(col, &lv_font_montserrat_24, COL_LABEL);
    lv_obj_set_style_text_letter_space(title, 6, 0);
    lv_label_set_text(title, "DEBUG");

    lv_obj_t *cap = mklabel(col, &lv_font_montserrat_20, COL_LBL);
    lv_obj_set_style_text_letter_space(cap, 2, 0);
    lv_label_set_text(cap, "ECRAN ALLUME");

    v_screen = mklabel(col, &lv_font_montserrat_40, theme_accent()); /* big time */

    v_uptime = mklabel(col, &lv_font_montserrat_24, COL_VAL);
    v_ratio  = mklabel(col, &lv_font_montserrat_24, COL_VAL);
    v_batt   = mklabel(col, &lv_font_montserrat_24, COL_VAL);
    v_unread = mklabel(col, &lv_font_montserrat_24, COL_VAL);

    face_debug_tick();
}

void face_debug_tick(void)
{
    static unsigned long last_sec = (unsigned long)-1;
    unsigned long on = ui_screen_on_ms();
    if (on / 1000 == last_sec) return; /* update once per second */
    last_sec = on / 1000;

    char t[16], line[28];
    wf_screentime_format(t, sizeof(t), on);
    lv_label_set_text(v_screen, t);

    wf_screentime_format(t, sizeof(t), ui_uptime_ms());
    snprintf(line, sizeof(line), "Uptime  %s", t);
    lv_label_set_text(v_uptime, line);

    snprintf(line, sizeof(line), "Ratio ON  %d%%", ui_screen_on_pct());
    lv_label_set_text(v_ratio, line);

    snprintf(line, sizeof(line), "Batterie  %d%%", watch_battery_pct());
    lv_label_set_text(v_batt, line);

    snprintf(line, sizeof(line), "Non lues  %d", face_notif_unread());
    lv_label_set_text(v_unread, line);
}
