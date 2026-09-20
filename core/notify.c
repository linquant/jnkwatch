#include "notify.h"
#include <string.h>
#include <stdio.h>

static void copy_field(char *dst, size_t cap, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    /* snprintf always NUL-terminates and truncates to cap-1 chars. */
    snprintf(dst, cap, "%s", src);
}

void wf_notif_init(wf_notif_store_t *s)
{
    s->count = 0;
    s->head = 0;
    s->next_id = 1;
}

unsigned wf_notif_push(wf_notif_store_t *s, const char *app,
                       const char *title, const char *body, long ts)
{
    /* head walks backwards so items[head] is always the newest; the physical
     * slot of the (now overwritten) oldest is reused when the buffer is full. */
    s->head = (s->head - 1 + WF_NOTIF_MAX) % WF_NOTIF_MAX;
    wf_notif_t *n = &s->items[s->head];

    copy_field(n->app, sizeof(n->app), app);
    copy_field(n->title, sizeof(n->title), title);
    copy_field(n->body, sizeof(n->body), body);
    n->ts = ts;
    n->id = s->next_id++;

    if (s->count < WF_NOTIF_MAX) s->count++;
    return n->id;
}

int wf_notif_count(const wf_notif_store_t *s)
{
    return s->count;
}

const wf_notif_t *wf_notif_get(const wf_notif_store_t *s, int i)
{
    if (i < 0 || i >= s->count) return NULL;
    int idx = (s->head + i) % WF_NOTIF_MAX;
    return &s->items[idx];
}

int wf_notif_remove(wf_notif_store_t *s, int i)
{
    if (i < 0 || i >= s->count) return s->count;
    /* Shift the newer-to-older run down by one over the removed slot; the
     * newest stays physically at head (head only moves on push). */
    for (int j = i; j < s->count - 1; j++) {
        int a = (s->head + j) % WF_NOTIF_MAX;
        int b = (s->head + j + 1) % WF_NOTIF_MAX;
        s->items[a] = s->items[b];
    }
    s->count--;
    return s->count;
}

void wf_notif_clear(wf_notif_store_t *s)
{
    s->count = 0;
    s->head = 0;
    /* next_id intentionally preserved so ids stay unique across clears. */
}

void wf_notif_reltime(char *buf, size_t n, long now, long ts)
{
    long d = now - ts;
    if (d < 60)          snprintf(buf, n, "now");
    else if (d < 3600)   snprintf(buf, n, "%ldm", d / 60);
    else if (d < 86400)  snprintf(buf, n, "%ldh", d / 3600);
    else                 snprintf(buf, n, "%ldj", d / 86400);
}
