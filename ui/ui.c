#include "ui.h"
#include "theme.h"
#include "geometry.h"
#include "face_clock.h"
#include "face_actions.h"
#include "face_notif.h"
#include "face_aura.h"
#include "face_timer.h"
#include "face_settings.h"
#include "face_debug.h"
#include "display.h"   /* watch_set_backlight(), watch_read_accel(), watch_millis() */
#include "wrist.h"
#include "screentime.h"
#include "lvgl.h"

/* Faces live in a horizontal tileview: swipe left/right to switch. */
static lv_obj_t *tv;
static lv_obj_t *tile_notif; /* tracked so we can clear unread when shown */
static lv_obj_t *tile_aura;  /* tracked so its costly canvas only animates when shown */
static lv_obj_t *dim_overlay;
static lv_obj_t *sleep_overlay;
static wf_wrist_t wrist;
static wf_screentime_t screentime; /* time the screen has been lit vs uptime */
static bool s_asleep; /* current screen-sleep state (see power_tick) */

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
    /* Only auto-sleep when there is a registered input device that can wake us
     * (touch on hardware, the SDL pointer on PC). During device bring-up before
     * touch/IMU are wired there is no wake source, and staying awake beats a
     * screen that blacks out forever. Wrist-wake is not counted here: it is on
     * by default but the accelerometer HAL is still stubbed, so it cannot yet
     * generate a real raise. */
    bool can_wake = lv_indev_get_next(NULL) != NULL;
    /* Charging normally keeps the screen on, but the user can disable that
     * ("Garder allumé sur secteur" off) so the watch also sleeps while plugged
     * in — handy on a bench/USB with no battery. */
    bool keep_on_power = watch_is_charging() && face_settings_stay_awake_charging();
    bool asleep = can_wake && !keep_on_power &&
        lv_display_get_inactive_time(disp) > (uint32_t)face_settings_sleep_ms();
    lv_obj_set_style_bg_opa(sleep_overlay, asleep ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

    /* On the sleep<->wake edge, drive the real panel power (AMOLED off/on).
     * Only on transition, so we never spam DCS 0x28/0x29 every frame. The device
     * HAL decides light-sleep (BLE kept) vs, in the "watch only" power-saver
     * mode, screen-off + deep-sleep with a wrist-raise (IMU) wake. Waking always
     * shows the normal clock. */
    if (asleep != s_asleep) {
        s_asleep = asleep;
        watch_display_sleep(asleep);
    }

    /* Accumulate screen-on time (screen counts as "on" when not asleep). */
    static uint32_t st_last = 0;
    static int st_started = 0;
    uint32_t now = watch_millis();
    unsigned dt = st_started ? (unsigned)(now - st_last) : 0;
    st_last = now;
    st_started = 1;
    wf_screentime_update(&screentime, dt, !asleep);
}

bool ui_is_asleep(void) { return s_asleep; }

unsigned long ui_screen_on_ms(void) { return screentime.on_ms; }
unsigned long ui_uptime_ms(void)    { return screentime.total_ms; }
int           ui_screen_on_pct(void) { return wf_screentime_on_pct(&screentime); }

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

    /* Horizontal strip: actions | notif | clock | timer | settings | debug.
     * The two TIME watchfaces (clock + aura) share col 2 (swipe up/down), and
     * SETTINGS is a 5-row column (col 4): brightness / veille / couleurs /
     * poignet / energie, swipe up/down; swipe LEFT/RIGHT moves along the strip.
     * Clock is the boot face. */
    lv_obj_t *tile_actions  = lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT);
    tile_notif              = lv_tileview_add_tile(tv, 1, 0, LV_DIR_HOR);
    lv_obj_t *tile_clock    = lv_tileview_add_tile(tv, 2, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    tile_aura               = lv_tileview_add_tile(tv, 2, 1, LV_DIR_TOP);
    lv_obj_t *tile_timer    = lv_tileview_add_tile(tv, 3, 0, LV_DIR_HOR);
    lv_obj_t *tile_set_b    = lv_tileview_add_tile(tv, 4, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *tile_set_v    = lv_tileview_add_tile(tv, 4, 1, LV_DIR_VER);
    lv_obj_t *tile_set_c    = lv_tileview_add_tile(tv, 4, 2, LV_DIR_VER);
    lv_obj_t *tile_set_w    = lv_tileview_add_tile(tv, 4, 3, LV_DIR_VER);
    lv_obj_t *tile_set_e    = lv_tileview_add_tile(tv, 4, 4, LV_DIR_TOP);
    lv_obj_t *tile_debug    = lv_tileview_add_tile(tv, 5, 0, LV_DIR_LEFT);

    face_actions_create(tile_actions);
    face_notif_create(tile_notif);
    face_clock_create(tile_clock);
    face_aura_create(tile_aura);
    face_timer_create(tile_timer);
    face_settings_create(tile_set_b, tile_set_v, tile_set_c, tile_set_w, tile_set_e);
    face_debug_create(tile_debug);

    wf_screentime_init(&screentime);

    /* Land on the clock (col 2, row 0). */
    lv_tileview_set_tile_by_index(tv, 2, 0, LV_ANIM_OFF);

    wf_wrist_init(&wrist);
}

/* Apply configuration pushed from the phone (BLE), drained off the write side. */
static void config_tick(void)
{
    wf_config_msg_t m;
    while (watch_poll_config(&m)) {
        switch (m.kind) {
        case WF_CFG_SETTINGS:
            face_settings_apply_config(m.brightness, m.sleep_ms, m.wrist_wake,
                                       m.stay_awake_charging, m.power_saver);
            theme_set_accent((accent_t)m.accent);
            ui_apply_accent();
            break;
        case WF_CFG_ACTION_CLEAR:
            face_actions_apply_clear();
            break;
        case WF_CFG_ACTION_SET:
            face_actions_apply_set(m.index, m.action_id, m.label,
                                   m.icon, m.enabled);
            break;
        case WF_CFG_NOTIF_CLEAR:
            face_notif_clear();
            break;
        case WF_CFG_WEATHER:
            face_clock_set_weather(m.temp_c, m.cond, m.location);
            break;
        }
    }
}

void ui_tick(void)
{
    config_tick();
    /* Every face ticks each frame so the timer keeps running in the
     * background regardless of the shown face. */
    face_clock_tick();
    face_actions_tick();
    face_notif_tick();
    /* The aura repaints a full-screen canvas each tick — only pay that cost
     * while it is actually on screen, or it throttles the whole UI. We test the
     * tile's real coordinates rather than lv_tileview_get_tile_act(), which is
     * not updated reliably by touch-driven scrolling on device. */
    if (tile_aura) {
        lv_area_t a;
        lv_obj_get_coords(tile_aura, &a);
        if (a.x1 < UI_W && a.x2 >= 0 && a.y1 < UI_H && a.y2 >= 0)
            face_aura_tick();
    }
    face_timer_tick();
    face_debug_tick();
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
        {4, 0}, /* 5 settings      */
        {5, 0}, /* 6 debug         */
    };
    if (!tv || idx < 0 || idx >= (int)(sizeof(POS) / sizeof(POS[0]))) return;
    lv_tileview_set_tile_by_index(tv, POS[idx].col, POS[idx].row, LV_ANIM_OFF);
}

void ui_goto_tile(int col, int row)
{
    if (tv) lv_tileview_set_tile_by_index(tv, col, row, LV_ANIM_OFF);
}

void ui_apply_accent(void)
{
    /* Push the current teinte to every face so they stay harmonised. Aura
     * recolours itself in its tick; these hold the statically-coloured widgets. */
    face_clock_apply_accent();
    face_actions_apply_accent();
    face_notif_apply_accent();
    face_timer_apply_accent();
    face_settings_apply_accent();
    face_debug_apply_accent();
}
