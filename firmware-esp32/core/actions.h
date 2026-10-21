#pragma once
/* Pure logic for the customizable action-button set. NO LVGL/platform deps,
 * fully unit-tested (see tests/test_actions.c). Mirrors core/notify.c (fixed
 * array + snprintf-truncated strings) and core/settings.c (init + validating
 * mutators). The UI reads it; a future BLE layer customizes it by calling the
 * same clear/add/set mutators the seed uses. */
#include <stddef.h>
#include <stdbool.h>

#define WF_ACTION_MAX        8   /* store capacity (room for the phone to grow) */
#define WF_ACTION_LABEL_LEN  12  /* short button caption, incl. NUL            */
#define WF_ACTIONS_PER_PAGE  6   /* 2x3 grid shown per round screen            */

/* Icon selector kept LVGL-free; the UI maps it to an LV_SYMBOL_* glyph. */
typedef enum {
    WF_ICON_NONE = 0,
    WF_ICON_PHONE,   /* find phone   */
    WF_ICON_CAMERA,  /* camera       */
    WF_ICON_MUSIC,   /* media        */
    WF_ICON_HOME,    /* home/lights  */
    WF_ICON_BELL,    /* ping/alert   */
    WF_ICON_VOLUME,  /* mute/ring    */
    WF_ICON_POWER,   /* power/torch  */
    WF_ICON_GENERIC, /* generic action placeholder */
    WF_ICON_COUNT,
} wf_icon_t;

typedef struct {
    unsigned  id;                        /* stable action id sent over BLE     */
    char      label[WF_ACTION_LABEL_LEN];/* snprintf-truncated, NULL -> ""     */
    wf_icon_t icon;                      /* clamped to [0,WF_ICON_COUNT)       */
    bool      enabled;                   /* inert/greyed when false            */
} wf_action_t;

typedef struct {
    wf_action_t items[WF_ACTION_MAX];
    int         count;                   /* 0..WF_ACTION_MAX                   */
} wf_actions_t;

/* Reset and seed the default action set. */
void wf_actions_init(wf_actions_t *s);

/* Number of configured actions. */
int wf_actions_count(const wf_actions_t *s);

/* Get by slot index (0-based). NULL if out of range. */
const wf_action_t *wf_actions_get(const wf_actions_t *s, int i);

/* Append an action. Returns the new slot index, or -1 if full. label is
 * truncated (NULL -> ""), icon clamped. */
int wf_actions_add(wf_actions_t *s, unsigned id, const char *label,
                   wf_icon_t icon, bool enabled);

/* Replace slot i in place. Returns i, or -1 if out of range. Same validation
 * as add. */
int wf_actions_set(wf_actions_t *s, int i, unsigned id, const char *label,
                   wf_icon_t icon, bool enabled);

/* Toggle a slot's enabled flag. Out-of-range index is a no-op. */
void wf_actions_set_enabled(wf_actions_t *s, int i, bool on);

/* Drop everything (count = 0), e.g. before a BLE full resync. */
void wf_actions_clear(wf_actions_t *s);

/* Number of pages of WF_ACTIONS_PER_PAGE items; always >= 1. */
int wf_actions_page_count(const wf_actions_t *s);
