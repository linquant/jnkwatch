#include "ui.h"
#include "theme.h"
#include "geometry.h"
#include "face_clock.h"
#include "face_actions.h"
#include "face_notif.h"
#include "face_aura.h"
#include "face_timer.h"
#include "face_compass.h"
#include "face_settings.h"
#include "display.h"   /* watch_set_backlight(), watch_read_accel() */
#include "wrist.h"
#include "lvgl.h"

#define SLEEP_MS 6000  /* screen sleeps after this much inactivity */

/* Faces live in a horizontal tileview: swipe left/right to switch. */
static lv_obj_t *tv;
static lv_obj_t *tile_notif; /* tracked so we can clear unread when shown */
static lv_obj_t *dim_overlay;
static lv_obj_t *sleep_overlay;
static wf_wrist_t wrist;

/* Sleep the screen after inactivity; wake on user input or a wrist raise. */
static void power_tick(void)
{
    lv_display_t *disp = lv_display_get_default();

    if (face_settings_wrist_wake()) {
        double ax, ay, az;
        watch_read_accel(&ax, &ay, &az);
        if (wf_wrist_update(&wrist, ax, ay, az))
            lv_display_trigger_activity(disp); /* raise counts as activity */
    }

    if (!sleep_overlay) {
        sleep_overlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(sleep_overlay);
        lv_obj_set_size(sleep_overlay, UI_W, UI_H);
        lv_obj_center(sleep_overlay);
        lv_obj_set_style_bg_color(sleep_overlay, lv_color_black(), 0);
        lv_obj_remove_flag(sleep_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
    bool asleep = lv_display_get_inactive_time(disp) > SLEEP_MS;
    lv_obj_set_style_bg_opa(sleep_overlay, asleep ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

void ui_apply_brightness(int pct)
{
    watch_set_backlight(pct); /* real hardware effect */

    /* Simulator feedback: a black overlay whose opacity grows as brightness
     * drops. On hardware the LEDC PWM does the dimming and this stays clear. */
    if (!dim_overlay) {
        dim_overlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(dim_overlay);
        lv_obj_set_size(dim_overlay, UI_W, UI_H);
        lv_obj_center(dim_overlay);
        lv_obj_set_style_bg_color(dim_overlay, lv_color_black(), 0);
        lv_obj_remove_flag(dim_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
    int op = (100 - pct) * 19 / 10; /* 0 (full) .. ~171 (dim) */
    lv_obj_set_style_bg_opa(dim_overlay, (lv_opa_t)op, 0);
}

void ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    tv = lv_tileview_create(scr);
    lv_obj_set_size(tv, UI_W, UI_H);
    lv_obj_center(tv);
    lv_obj_set_scrollbar_mode(tv, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(tv, COL_BG, 0);
    lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);

    /* Horizontal strip: actions | notif | clock | timer | compass | settings.
     * The two TIME watchfaces (clock + aura) share ONE horizontal slot (col 2):
     * clock on top, aura stacked below it — swipe UP/DOWN to switch between the
     * two, swipe LEFT/RIGHT to move along the strip. Clock is the boot face. */
    lv_obj_t *tile_actions  = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
    tile_notif              = lv_tileview_add_tile(tv, 1, 0, LV_DIR_HOR);
    lv_obj_t *tile_clock    = lv_tileview_add_tile(tv, 2, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *tile_aura     = lv_tileview_add_tile(tv, 2, 1, LV_DIR_TOP);
    lv_obj_t *tile_timer    = lv_tileview_add_tile(tv, 3, 0, LV_DIR_HOR);
    lv_obj_t *tile_compass  = lv_tileview_add_tile(tv, 4, 0, LV_DIR_HOR);
    lv_obj_t *tile_settings = lv_tileview_add_tile(tv, 5, 0, LV_DIR_LEFT);

    face_actions_create(tile_actions);
    face_notif_create(tile_notif);
    face_clock_create(tile_clock);
    face_aura_create(tile_aura);
    face_timer_create(tile_timer);
    face_compass_create(tile_compass);
    face_settings_create(tile_settings);

    /* Land on the clock (col 2, row 0). */
    lv_tileview_set_tile_by_index(tv, 2, 0, LV_ANIM_OFF);

    wf_wrist_init(&wrist);
}

void ui_tick(void)
{
    /* Every face ticks each frame so the timer keeps running in the
     * background and the compass stays live regardless of the shown face. */
    face_clock_tick();
    face_actions_tick();
    face_notif_tick();
    face_aura_tick();
    face_timer_tick();
    face_compass_tick();
    power_tick();

    /* Seeing the notifications face clears the clock's unread bell. */
    if (tv && lv_tileview_get_tile_act(tv) == tile_notif)
        face_notif_mark_read();
}

void ui_show_face(int idx)
{
    /* Logical face index -> tile (col,row). Aura shares the clock's column on
     * a second row, so it isn't addressable by column alone. */
    static const struct { uint8_t col, row; } POS[] = {
        {0, 0}, /* 0 actions      */
        {1, 0}, /* 1 notifications */
        {2, 0}, /* 2 clock         */
        {2, 1}, /* 3 aura          */
        {3, 0}, /* 4 timer         */
        {4, 0}, /* 5 compass       */
        {5, 0}, /* 6 settings      */
    };
    if (!tv || idx < 0 || idx >= (int)(sizeof(POS) / sizeof(POS[0]))) return;
    lv_tileview_set_tile_by_index(tv, POS[idx].col, POS[idx].row, LV_ANIM_OFF);
}

void ui_apply_accent(void)
{
    /* Push the current teinte to every face so they stay harmonised. Aura and
     * the compass rose recolour themselves in their tick; these hold the
     * statically-coloured widgets. */
    face_clock_apply_accent();
    face_actions_apply_accent();
    face_notif_apply_accent();
    face_timer_apply_accent();
    face_compass_apply_accent();
    face_settings_apply_accent();
}
