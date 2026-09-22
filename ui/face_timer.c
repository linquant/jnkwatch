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

/* Control icons are drawn as OUTLINES (dark interior, ~3px accent stroke) to
 * light far fewer pixels than a filled glyph — better on AMOLED. */
#define BTN_FILL  0x0C1418   /* button/canvas interior (below the lit threshold) */
#define ICP 56               /* primary icon canvas (px)  */
#define ICS 40               /* side icon canvas (px)     */

typedef enum { IC_PLAY, IC_PAUSE, IC_RESET, IC_PLUS, IC_MINUS } icon_t;

static wf_timer_t tmr;

static lv_obj_t *glow[UI_GLOW_N];
static lv_obj_t *core;
static lv_obj_t *ring;
static lv_obj_t *btn_primary, *btn_minus, *btn_plus;
static lv_obj_t *ic_primary, *ic_minus, *ic_plus;   /* icon canvases */
static icon_t    t_primary, t_minus, t_plus;         /* current icon per button */
static uint8_t icbuf_p[LV_CANVAS_BUF_SIZE(ICP, ICP, 16, 1)] __attribute__((aligned(4)));
static uint8_t icbuf_m[LV_CANVAS_BUF_SIZE(ICS, ICS, 16, 1)] __attribute__((aligned(4)));
static uint8_t icbuf_l[LV_CANVAS_BUF_SIZE(ICS, ICS, 16, 1)] __attribute__((aligned(4)));

static void set_time_text(const char *s)
{
    for (int i = 0; i < UI_GLOW_N; i++) {
        lv_label_set_text(glow[i], s);
        lv_obj_align(glow[i], LV_ALIGN_CENTER, UI_GLOW_OFS[i][0], TIME_Y + UI_GLOW_OFS[i][1]);
    }
    lv_label_set_text(core, s);
    lv_obj_align(core, LV_ALIGN_CENTER, 0, TIME_Y);
}

/* Draw an outline icon centered in a w×w canvas. */
static void draw_icon(lv_obj_t *cv, int w, icon_t t, lv_color_t col)
{
    lv_canvas_fill_bg(cv, lv_color_hex(BTN_FILL), LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(cv, &layer);

    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.color = col; ld.width = 3; ld.opa = LV_OPA_COVER;
    ld.round_start = ld.round_end = 1;
    double W = w;
#define L(x1, y1, x2, y2) do { \
        ld.p1.x = (x1); ld.p1.y = (y1); ld.p2.x = (x2); ld.p2.y = (y2); \
        lv_draw_line(&layer, &ld); } while (0)

    switch (t) {
    case IC_PLAY: /* triangle outline pointing right */
        L(0.34 * W, 0.24 * W, 0.34 * W, 0.76 * W);
        L(0.34 * W, 0.24 * W, 0.74 * W, 0.50 * W);
        L(0.34 * W, 0.76 * W, 0.74 * W, 0.50 * W);
        break;
    case IC_PAUSE: { /* two hollow bars */
        lv_draw_rect_dsc_t rd;
        lv_draw_rect_dsc_init(&rd);
        rd.bg_opa = LV_OPA_TRANSP;
        rd.border_color = col; rd.border_width = 3; rd.radius = 2;
        lv_area_t b1 = { (int)(0.30 * W), (int)(0.26 * W), (int)(0.44 * W), (int)(0.74 * W) };
        lv_area_t b2 = { (int)(0.56 * W), (int)(0.26 * W), (int)(0.70 * W), (int)(0.74 * W) };
        lv_draw_rect(&layer, &rd, &b1);
        lv_draw_rect(&layer, &rd, &b2);
        break; }
    case IC_RESET: { /* circular arrow outline (~280°) + arrowhead */
        lv_draw_arc_dsc_t ad;
        lv_draw_arc_dsc_init(&ad);
        ad.color = col; ad.width = 3; ad.rounded = 1;
        ad.center.x = (int)(0.5 * W); ad.center.y = (int)(0.5 * W);
        ad.radius = (int)(0.26 * W);
        ad.start_angle = 40; ad.end_angle = 320;
        lv_draw_arc(&layer, &ad);
        double a = 320.0 * M_PI / 180.0;
        double ex = 0.5 * W + 0.26 * W * cos(a);
        double ey = 0.5 * W + 0.26 * W * sin(a);
        L(ex, ey, ex - 0.11 * W, ey - 0.02 * W);
        L(ex, ey, ex - 0.02 * W, ey - 0.12 * W);
        break; }
    case IC_PLUS:
        L(0.28 * W, 0.50 * W, 0.72 * W, 0.50 * W);
        L(0.50 * W, 0.28 * W, 0.50 * W, 0.72 * W);
        break;
    case IC_MINUS:
        L(0.28 * W, 0.50 * W, 0.72 * W, 0.50 * W);
        break;
    }
#undef L
    lv_canvas_finish_layer(cv, &layer);
}

static void show_btn(lv_obj_t *b, int on)
{
    if (on) lv_obj_remove_flag(b, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
}

/* Choose icons + which side buttons are visible for the current state.
 *   IDLE           : [-] [play] [+]
 *   RUNNING/PAUSED : [ ] [pause/play] [reset]   (minus zone goes black)
 *   DONE           : [ ] [reset] [ ]            (only the centre reset)
 */
static void refresh_controls(void)
{
    lv_color_t a = theme_accent();
    t_primary = tmr.state == WF_TMR_RUNNING ? IC_PAUSE :
                tmr.state == WF_TMR_DONE    ? IC_RESET : IC_PLAY;
    draw_icon(ic_primary, ICP, t_primary, a);

    if (tmr.state == WF_TMR_IDLE) {
        show_btn(btn_minus, 1); show_btn(btn_plus, 1);
        t_minus = IC_MINUS; t_plus = IC_PLUS;
        draw_icon(ic_minus, ICS, t_minus, a);
        draw_icon(ic_plus,  ICS, t_plus,  a);
    } else if (tmr.state == WF_TMR_DONE) {
        show_btn(btn_minus, 0); show_btn(btn_plus, 0);
    } else { /* RUNNING or PAUSED: a single reset on the plus, minus blanked */
        show_btn(btn_minus, 0);
        show_btn(btn_plus, 1);
        t_plus = IC_RESET;
        draw_icon(ic_plus, ICS, t_plus, a);
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

static lv_obj_t *make_button(lv_obj_t *p, int dx, int dy, int size, int icsz,
                             uint8_t *icbuf, lv_color_t accent, lv_event_cb_t cb,
                             lv_obj_t **out_icon)
{
    lv_obj_t *b = lv_button_create(p);
    lv_obj_set_size(b, size, size);
    lv_obj_align(b, LV_ALIGN_CENTER, dx, dy);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(BTN_FILL), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, accent, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cv = lv_canvas_create(b);
    lv_canvas_set_buffer(cv, icbuf, icsz, icsz, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(cv);
    lv_obj_remove_flag(cv, LV_OBJ_FLAG_CLICKABLE); /* clicks go to the button */
    *out_icon = cv;
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
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
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

    /* Controls with outline icons (size sets the primary apart). */
    lv_color_t a = theme_accent();
    btn_minus   = make_button(p, -120, 116, 68, ICS, icbuf_m, a, on_minus, &ic_minus);
    btn_primary = make_button(p,    0, 116, 96, ICP, icbuf_p, a, on_primary, &ic_primary);
    btn_plus    = make_button(p,  120, 116, 68, ICS, icbuf_l, a, on_plus, &ic_plus);

    face_timer_apply_accent();  /* tint the digit glow */
    refresh_controls();         /* draw the icons      */
    face_timer_tick();
}

void face_timer_apply_accent(void)
{
    lv_color_t a = theme_accent();
    for (int i = 0; i < UI_GLOW_N; i++)
        lv_obj_set_style_text_color(glow[i], a, 0);
    lv_obj_set_style_border_color(btn_minus, a, 0);
    lv_obj_set_style_border_color(btn_primary, a, 0);
    lv_obj_set_style_border_color(btn_plus, a, 0);
    /* redraw the outline icons in the new teinte */
    if (ic_primary) {
        draw_icon(ic_primary, ICP, t_primary, a);
        draw_icon(ic_minus,   ICS, t_minus,   a);
        draw_icon(ic_plus,    ICS, t_plus,    a);
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
