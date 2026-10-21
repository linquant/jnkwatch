#include "face_probes.h"
#include "theme.h"
#include "probes.h"         /* wf_probes_t + store logic */
#include "display.h"        /* watch_millis() */
#include <stdint.h>

/* The pushed values + reception timestamps (portable, unit-tested logic). The
 * face owns the store and only renders it. */
static wf_probes_t store;

/* Rows are created lazily — only when a slot first receives data — so an idle
 * watch allocates nothing here (pre-building all WF_PROBE_MAX rows at boot used
 * enough of LVGL's memory pool to starve other faces' draws). Each row: name on
 * top, value + freshness below. */
static lv_obj_t *list;                       /* scrolling container (the tile) */
static lv_obj_t *row[WF_PROBE_MAX];          /* NULL until the slot is filled  */
static lv_obj_t *name_lbl[WF_PROBE_MAX];
static lv_obj_t *val_lbl[WF_PROBE_MAX];
static lv_obj_t *age_lbl[WF_PROBE_MAX];
static lv_obj_t *empty_lbl;

static void ensure_row(int i)
{
    if (row[i] || !list) return;

    lv_obj_t *r = lv_obj_create(list);
    lv_obj_remove_style_all(r);
    lv_obj_set_width(r, lv_pct(100));
    lv_obj_set_height(r, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(r, 2, 0);
    lv_obj_set_style_pad_ver(r, 8, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);

    name_lbl[i] = lv_label_create(r);
    lv_obj_set_style_text_font(name_lbl[i], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(name_lbl[i], theme_accent(), 0);
    lv_obj_set_style_text_letter_space(name_lbl[i], 2, 0);
    lv_label_set_text(name_lbl[i], "");

    val_lbl[i] = lv_label_create(r);
    lv_obj_set_style_text_font(val_lbl[i], &lv_font_montserrat_34, 0);
    lv_obj_set_style_text_color(val_lbl[i], COL_CORE, 0);
    lv_label_set_text(val_lbl[i], "");

    age_lbl[i] = lv_label_create(r);
    lv_obj_set_style_text_font(age_lbl[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(age_lbl[i], COL_LABEL, 0);
    lv_label_set_text(age_lbl[i], "");

    row[i] = r;
}

void face_probes_create(lv_obj_t *p)
{
    wf_probes_init(&store);
    for (int i = 0; i < WF_PROBE_MAX; i++) row[i] = NULL;
    list = p;

    /* Vertically scrolling column (lists of up to WF_PROBE_MAX probes can run
     * past the panel height). Scrollbar hidden to keep the black panel clean. */
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(p, 4, 0);
    lv_obj_set_style_pad_hor(p, 60, 0);   /* clear of the round panel edges */
    lv_obj_set_style_pad_ver(p, 70, 0);
    lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(p, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(p);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COL_LABEL, 0);
    lv_obj_set_style_text_letter_space(title, 6, 0);
    lv_label_set_text(title, "SONDES");

    empty_lbl = lv_label_create(p);
    lv_obj_set_style_text_font(empty_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(empty_lbl, COL_LABEL, 0);
    lv_label_set_text(empty_lbl, "Aucune donnee");
}

/* Paint one slot's three labels from the store (row must already exist). */
static void render_slot(int i, uint32_t now)
{
    if (!row[i]) return;
    const wf_probe_slot_t *s = &store.slot[i];
    lv_label_set_text(name_lbl[i], s->name);
    lv_label_set_text(val_lbl[i], s->value[0] ? s->value : "--");
    char age[32];
    wf_probes_format_age(age, sizeof age, s->updated_ms, now);
    lv_label_set_text(age_lbl[i], age);
}

void face_probes_set(int slot, const char *name, const char *value)
{
    uint32_t now = watch_millis();
    wf_probes_set(&store, slot, name, value, now);
    if (slot < 0 || slot >= WF_PROBE_MAX || !store.slot[slot].used) return;
    ensure_row(slot);
    render_slot(slot, now);
    if (empty_lbl)
        lv_obj_add_flag(empty_lbl, LV_OBJ_FLAG_HIDDEN);   /* we have data now */
}

void face_probes_tick(void)
{
    /* Only the freshness strings drift between pushes; refresh them about once a
     * second so "il y a N min" stays truthful without churning every frame. */
    static uint32_t last = 0;
    uint32_t now = watch_millis();
    if (now - last < 1000) return;
    last = now;

    int n = 0;
    for (int i = 0; i < WF_PROBE_MAX; i++) {
        if (!store.slot[i].used || !row[i]) continue;
        n++;
        char age[32];
        wf_probes_format_age(age, sizeof age, store.slot[i].updated_ms, now);
        lv_label_set_text(age_lbl[i], age);
    }
    if (empty_lbl) {
        if (n == 0) lv_obj_remove_flag(empty_lbl, LV_OBJ_FLAG_HIDDEN);
        else        lv_obj_add_flag(empty_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

void face_probes_apply_accent(void)
{
    for (int i = 0; i < WF_PROBE_MAX; i++)
        if (row[i] && name_lbl[i])
            lv_obj_set_style_text_color(name_lbl[i], theme_accent(), 0);
}
