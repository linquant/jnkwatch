#include "face_timer.h"
#include "theme.h"
#include "fonts.h"     /* wf_digits_100 */
#include "timer.h"
#include "display.h"   /* watch_millis() */
#include <math.h>

#define TIME_Y (-22)
#define RING_MAX 1000
#define STEP_MS (60 * 1000)
#define DEFAULT_MS (5 * 60 * 1000)

static wf_timer_t tmr;

static lv_obj_t *glow[UI_GLOW_N];
static lv_obj_t *core;
static lv_obj_t *ring;
static lv_obj_t *lbl_primary, *lbl_minus, *lbl_plus;
static lv_obj_t *btn_primary, *btn_minus, *btn_plus;

static void set_time_text(const char *s)
{
    for (int i = 0; i < UI_GLOW_N; i++) {
        lv_label_set_text(glow[i], s);
        lv_obj_align(glow[i], LV_ALIGN_CENTER, UI_GLOW_OFS[i][0], TIME_Y + UI_GLOW_OFS[i][1]);
    }
    lv_label_set_text(core, s);
    lv_obj_align(core, LV_ALIGN_CENTER, 0, TIME_Y);
}

/* Update the button glyphs for the current state. */
static void refresh_controls(void)
{
    const char *prim =
        tmr.state == WF_TMR_RUNNING ? LV_SYMBOL_PAUSE :
        tmr.state == WF_TMR_DONE    ? LV_SYMBOL_REFRESH : LV_SYMBOL_PLAY;
    lv_label_set_text(lbl_primary, prim);

    if (tmr.state == WF_TMR_IDLE) {
        lv_label_set_text(lbl_minus, LV_SYMBOL_MINUS);
        lv_label_set_text(lbl_plus, LV_SYMBOL_PLUS);
    } else {
        lv_label_set_text(lbl_minus, LV_SYMBOL_REFRESH);
        lv_label_set_text(lbl_plus, LV_SYMBOL_REFRESH);
    }
}

static void on_primary(lv_event_t *e)
{
    (void)e;
    if (tmr.state == WF_TMR_DONE) wf_timer_reset(&tmr);
    else wf_timer_toggle(&tmr);
    refresh_controls();
}

static void on_minus(lv_event_t *e)
{
    (void)e;
    if (tmr.state == WF_TMR_IDLE) wf_timer_set_duration(&tmr, tmr.duration_ms - STEP_MS);
    else wf_timer_reset(&tmr);
    refresh_controls();
}

static void on_plus(lv_event_t *e)
{
    (void)e;
    if (tmr.state == WF_TMR_IDLE) wf_timer_set_duration(&tmr, tmr.duration_ms + STEP_MS);
    else wf_timer_reset(&tmr);
    refresh_controls();
}

static lv_obj_t *make_button(lv_obj_t *p, int dx, int dy, int size,
                             lv_color_t accent, lv_event_cb_t cb, lv_obj_t **out_lbl)
{
    lv_obj_t *b = lv_button_create(p);
    lv_obj_set_size(b, size, size);
    lv_obj_align(b, LV_ALIGN_CENTER, dx, dy);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0C1418), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, accent, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(l, accent, 0);
    lv_obj_center(l);
    *out_lbl = l;
    return b;
}

void face_timer_create(lv_obj_t *p)
{
    wf_timer_init(&tmr, DEFAULT_MS);

    /* Depleting progress ring. */
    ring = lv_arc_create(p);
    lv_obj_set_size(ring, 440, 440);
    lv_obj_center(ring);
    lv_arc_set_rotation(ring, 270);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_arc_set_range(ring, 0, RING_MAX);
    lv_arc_set_value(ring, RING_MAX);
    lv_obj_remove_style(ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(ring, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring, 9, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ring, COL_OK, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ring, true, LV_PART_INDICATOR);

    /* Title. */
    lv_obj_t *title = lv_label_create(p);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title, 6, 0);
    lv_label_set_text(title, "TIMER");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 78);

    /* MM:SS glow + core. */
    for (int i = 0; i < UI_GLOW_N; i++) {
        glow[i] = lv_label_create(p);
        lv_obj_set_style_text_font(glow[i], &wf_digits_100, 0);
        lv_obj_set_style_text_color(glow[i], COL_GLOW, 0);
        lv_obj_set_style_text_opa(glow[i], 24, 0);
        lv_label_set_text(glow[i], "");
    }
    core = lv_label_create(p);
    lv_obj_set_style_text_font(core, &wf_digits_100, 0);
    lv_obj_set_style_text_color(core, COL_CORE, 0);
    lv_label_set_text(core, "");

    /* Controls (all in the accent teinte; size sets the primary apart). */
    lv_color_t a = theme_accent();
    btn_minus   = make_button(p, -120, 116, 68, a, on_minus, &lbl_minus);
    btn_primary = make_button(p,    0, 116, 96, a, on_primary, &lbl_primary);
    btn_plus    = make_button(p,  120, 116, 68, a, on_plus, &lbl_plus);

    face_timer_apply_accent();  /* tint the digit glow too */
    refresh_controls();
    face_timer_tick();
}

void face_timer_apply_accent(void)
{
    lv_color_t a = theme_accent();
    for (int i = 0; i < UI_GLOW_N; i++)
        lv_obj_set_style_text_color(glow[i], a, 0);
    lv_obj_t *btns[3] = { btn_minus, btn_primary, btn_plus };
    lv_obj_t *lbls[3] = { lbl_minus, lbl_primary, lbl_plus };
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_border_color(btns[i], a, 0);
        lv_obj_set_style_text_color(lbls[i], a, 0);
    }
}

void face_timer_tick(void)
{
    /* Advance by real elapsed time (runs even when this face is hidden). */
    static uint32_t last = 0;
    static int started = 0;
    uint32_t now = watch_millis();
    uint32_t dt = started ? now - last : 0;
    last = now;
    started = 1;

    wf_tmr_state_t prev = tmr.state;
    wf_timer_tick(&tmr, (int)dt);
    if (tmr.state != prev) refresh_controls();

    char b[8];
    wf_timer_format(&tmr, b, sizeof(b));
    set_time_text(b);

    lv_arc_set_value(ring, (int)lround(wf_timer_remaining_fraction(&tmr) * RING_MAX));

    lv_color_t rc =
        tmr.state == WF_TMR_DONE   ? COL_CRIT :
        tmr.state == WF_TMR_PAUSED ? COL_LOW  : theme_accent();
    lv_obj_set_style_arc_color(ring, rc, LV_PART_INDICATOR);

    /* Blink the digits when finished. */
    lv_opa_t core_opa = LV_OPA_COVER;
    if (tmr.state == WF_TMR_DONE && (watch_millis() / 400) % 2)
        core_opa = LV_OPA_TRANSP;
    lv_obj_set_style_text_opa(core, core_opa, 0);
}
