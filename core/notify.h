#pragma once
/* Pure notification logic: a small ring buffer of recent phone notifications
 * plus text/time helpers. NO LVGL or platform dependency, fully unit-tested
 * (see tests/test_notify.c). The BLE layer (or the PC mock) pushes into the
 * store; the UI only reads it. */
#include <stddef.h>

#define WF_NOTIF_MAX        8   /* how many recent notifications we keep     */
#define WF_NOTIF_APP_LEN    18  /* source app name, incl. NUL                */
#define WF_NOTIF_TITLE_LEN  40  /* title / sender, incl. NUL                 */
#define WF_NOTIF_BODY_LEN   128 /* message preview, incl. NUL                */

typedef struct {
    char app[WF_NOTIF_APP_LEN];    /* e.g. "Messages", "WhatsApp"            */
    char title[WF_NOTIF_TITLE_LEN];/* e.g. sender or subject                 */
    char body[WF_NOTIF_BODY_LEN];  /* message text (may be truncated)        */
    long ts;                       /* epoch seconds when received            */
    unsigned id;                   /* unique, increasing per push            */
} wf_notif_t;

typedef struct {
    wf_notif_t items[WF_NOTIF_MAX]; /* newest at items[head]                 */
    int  count;                     /* stored count, 0..WF_NOTIF_MAX         */
    int  head;                      /* index of the newest item              */
    unsigned next_id;               /* id handed to the next push            */
} wf_notif_store_t;

/* Reset to empty. */
void wf_notif_init(wf_notif_store_t *s);

/* Add a notification as the newest one; the oldest is dropped when full.
 * Fields are copied and safely truncated to their buffer sizes. NULL fields
 * become empty strings. Returns the id assigned to the new notification. */
unsigned wf_notif_push(wf_notif_store_t *s, const char *app,
                       const char *title, const char *body, long ts);

/* Number of stored notifications. */
int wf_notif_count(const wf_notif_store_t *s);

/* Get by recency: i=0 is the newest, i=1 the next, ... Returns NULL if out of
 * range (including when the store is empty). */
const wf_notif_t *wf_notif_get(const wf_notif_store_t *s, int i);

/* Remove the notification at recency index i (0=newest), closing the gap so
 * the one that followed takes its place. Out-of-range i is a no-op. Returns
 * the new count. */
int wf_notif_remove(wf_notif_store_t *s, int i);

/* Drop everything (e.g. "clear all"). Ids keep increasing across clears. */
void wf_notif_clear(wf_notif_store_t *s);

/* Compact relative age of a notification for the round screen, from
 * (now - ts): "now" (<60s), "Nm" (<60min), "Nh" (<24h), else "Nj" (days).
 * Negative/zero ages read "now". Writes at most n bytes incl. NUL. */
void wf_notif_reltime(char *buf, size_t n, long now, long ts);
