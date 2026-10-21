#include "probes.h"
#include <stdio.h>
#include <string.h>

static void copy_field(char *dst, size_t cap, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    /* snprintf always NUL-terminates and truncates to cap-1 chars. */
    snprintf(dst, cap, "%s", src);
}

void wf_probes_init(wf_probes_t *p)
{
    for (int i = 0; i < WF_PROBE_MAX; i++) {
        p->slot[i].name[0] = '\0';
        p->slot[i].value[0] = '\0';
        p->slot[i].updated_ms = 0;
        p->slot[i].used = false;
    }
}

void wf_probes_set(wf_probes_t *p, int slot, const char *name,
                   const char *value, unsigned long now_ms)
{
    if (slot < 0 || slot >= WF_PROBE_MAX) return;
    if (!name || name[0] == '\0') return;
    wf_probe_slot_t *s = &p->slot[slot];
    copy_field(s->name, sizeof(s->name), name);
    copy_field(s->value, sizeof(s->value), value);
    s->updated_ms = now_ms;
    s->used = true;
}

void wf_probes_clear(wf_probes_t *p, int slot)
{
    if (slot < 0 || slot >= WF_PROBE_MAX) return;
    p->slot[slot].name[0] = '\0';
    p->slot[slot].value[0] = '\0';
    p->slot[slot].updated_ms = 0;
    p->slot[slot].used = false;
}

int wf_probes_count(const wf_probes_t *p)
{
    int n = 0;
    for (int i = 0; i < WF_PROBE_MAX; i++)
        if (p->slot[i].used) n++;
    return n;
}

void wf_probes_format_age(char *buf, size_t cap,
                          unsigned long updated_ms, unsigned long now_ms)
{
    /* Guard against a clock that ran backwards (e.g. RTC resync) -> treat as 0. */
    unsigned long delta = (now_ms >= updated_ms) ? (now_ms - updated_ms) : 0UL;
    unsigned long sec = delta / 1000UL;

    if (sec < 10UL)
        snprintf(buf, cap, "a l'instant");
    else if (sec < 60UL)
        snprintf(buf, cap, "il y a %lu s", sec);
    else if (sec < 3600UL)
        snprintf(buf, cap, "il y a %lu min", sec / 60UL);
    else if (sec < 86400UL)
        snprintf(buf, cap, "il y a %lu h", sec / 3600UL);
    else
        snprintf(buf, cap, "il y a %lu j", sec / 86400UL);
}
