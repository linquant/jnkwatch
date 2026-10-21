#include "face_notif.h"
#include "theme.h"
#include "notify.h"
#include "display.h"   /* watch_poll_notification() */
#include <time.h>
#include <stdio.h>

/* Round-screen layout budget: a narrow, vertically-centred card so all text
 * stays in the circle's widest band and never reaches the curved glass. */
#define CARD_W    320
#define TEXT_W    300

#define COL_TITLE lv_color_hex(0xEAF1F4) /* bright title            */
#define COL_BODY  lv_color_hex(0x93A0A8) /* muted message preview   */
#define COL_TIME  lv_color_hex(0x63707A) /* dim timestamp / footer  */
#define COL_LINE  lv_color_hex(0x232B31) /* header hairline         */

static wf_notif_store_t store;

static lv_obj_t *card;                 /* holds the latest notification    */
static lv_obj_t *dot, *app_lbl, *time_lbl;
static lv_obj_t *title_lbl, *body_lbl, *foot_lbl;
static lv_obj_t *empty;                /* "no notifications" state         */

static unsigned newest_id;   /* id of the newest stored, to spot arrivals */
static int browse_idx;       /* which notification is on screen (0=newest) */
static int unread;           /* new arrivals not yet seen (clock bell)     */
static uint32_t last_refresh;

int face_notif_unread(void)
{
    return unread;
}

void face_notif_mark_read(void)
{
    unread = 0;
}

static void render_current(void);
static void on_next(lv_event_t *e);

void face_notif_clear(void)
{
    wf_notif_clear(&store);
    newest_id = 0;
    browse_idx = 0;
    unread = 0;
    render_current();
}

static lv_obj_t *make_row(lv_obj_t *p, int w, lv_flex_align_t main, int gap)
{
    lv_obj_t *r = lv_obj_create(p);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, w ? w : LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_style_pad_gap(r, gap, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    return r;
}

static lv_obj_t *make_label(lv_obj_t *p, const lv_font_t *f, lv_color_t c)
{
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_text(l, "");
    return l;
}

void face_notif_apply_accent(void)
{
    lv_color_t a = theme_accent();
    lv_obj_set_style_bg_color(dot, a, 0);
    lv_obj_set_style_text_color(app_lbl, a, 0);
}

void face_notif_create(lv_obj_t *p)
{
    wf_notif_init(&store);
    newest_id = 0;
    browse_idx = 0;
    unread = 0;
    last_refresh = 0;

    /* --- Card with the latest notification ------------------------------- */
    card = lv_obj_create(p);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, CARD_W, LV_SIZE_CONTENT);
    lv_obj_center(card);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, on_next, LV_EVENT_CLICKED, NULL);

    /* Header: accent dot + app on the left, relative time on the right. */
    lv_obj_t *head = make_row(card, 300, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_t *left = make_row(head, 0, LV_FLEX_ALIGN_START, 12);
    dot = lv_obj_create(left);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    app_lbl = make_label(left, &lv_font_montserrat_28, theme_accent());
    lv_obj_set_style_text_letter_space(app_lbl, 1, 0);
    time_lbl = make_label(head, &lv_font_montserrat_24, COL_TIME);

    /* Hairline under the header. */
    lv_obj_t *line = lv_obj_create(card);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, 260, 1);
    lv_obj_set_style_bg_color(line, COL_LINE, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    /* Title: sender / subject, wraps to at most a couple of lines. */
    title_lbl = make_label(card, &lv_font_montserrat_40, COL_TITLE);
    lv_obj_set_width(title_lbl, TEXT_W);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_max_height(title_lbl, 100, 0); /* ~2 lines then ellipsis */

    /* Body preview: fixed ~5-line box, ellipsis on overflow. */
    body_lbl = make_label(card, &lv_font_montserrat_28, COL_BODY);
    lv_obj_set_size(body_lbl, TEXT_W, 160);
    lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(body_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* Footer: how many are stacked behind this one. */
    foot_lbl = make_label(card, &lv_font_montserrat_24, COL_TIME);
    lv_obj_set_style_text_letter_space(foot_lbl, 1, 0);

    /* --- Empty state ----------------------------------------------------- */
    empty = lv_obj_create(p);
    lv_obj_remove_style_all(empty);
    lv_obj_set_size(empty, CARD_W, LV_SIZE_CONTENT);
    lv_obj_center(empty);
    lv_obj_set_flex_flow(empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(empty, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(empty, 16, 0);
    lv_obj_remove_flag(empty, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *bell = make_label(empty, &lv_font_montserrat_48, COL_TIME);
    lv_label_set_text(bell, LV_SYMBOL_BELL);
    lv_obj_t *none = make_label(empty, &lv_font_montserrat_28, COL_TIME);
    lv_label_set_text(none, "Aucune notification");

    face_notif_apply_accent();
    face_notif_tick();
}

static void render_current(void)
{
    int c = wf_notif_count(&store);
    if (c == 0) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(empty, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (browse_idx >= c) browse_idx = 0; /* clamp after drops/clears */

    const wf_notif_t *n = wf_notif_get(&store, browse_idx);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(empty, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(app_lbl, n->app);
    lv_label_set_text(title_lbl, n->title);
    lv_label_set_text(body_lbl, n->body);

    char t[16];
    wf_notif_reltime(t, sizeof(t), (long)watch_time(), n->ts);
    lv_label_set_text(time_lbl, t);

    /* Footer: position when several are stacked (tap to see the next). */
    char f[32];
    if (c > 1) snprintf(f, sizeof(f), "%d / %d", browse_idx + 1, c);
    else       snprintf(f, sizeof(f), "a jour");
    lv_label_set_text(foot_lbl, f);
}

/* Tap the card to dismiss the shown notification (it's been read) and reveal
 * the next one; when the last is dismissed the empty state shows. */
static void on_next(lv_event_t *e)
{
    (void)e;
    if (wf_notif_count(&store) == 0) return;
    int left = wf_notif_remove(&store, browse_idx);
    if (browse_idx >= left) browse_idx = 0; /* fell off the end → newest */
    render_current();
}

void face_notif_tick(void)
{
    /* Drain the (mock) BLE source into the store, counting new arrivals. */
    wf_notif_t in;
    while (watch_poll_notification(&in)) {
        wf_notif_push(&store, in.app, in.title, in.body, in.ts);
        if (unread < wf_notif_count(&store)) unread++;
    }

    const wf_notif_t *n = wf_notif_get(&store, 0);
    unsigned top = n ? n->id : 0;
    uint32_t now = lv_tick_get();

    if (top > newest_id) {
        /* A genuinely new notification arrived (ids only grow): jump to it.
         * A shrinking top id means we just dismissed one — don't treat that
         * as an arrival. */
        newest_id = top;
        browse_idx = 0;
        last_refresh = now;
        render_current();
    } else if ((now - last_refresh) > 5000) {
        /* Periodic refresh so the relative time on the viewed card ticks. */
        last_refresh = now;
        render_current();
    }
}
