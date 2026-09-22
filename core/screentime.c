#include "screentime.h"
#include <stdio.h>

void wf_screentime_init(wf_screentime_t *s)
{
    s->on_ms = 0;
    s->total_ms = 0;
}

void wf_screentime_update(wf_screentime_t *s, unsigned dt_ms, int screen_on)
{
    s->total_ms += dt_ms;
    if (screen_on) s->on_ms += dt_ms;
}

int wf_screentime_on_pct(const wf_screentime_t *s)
{
    if (s->total_ms == 0) return 0;
    return (int)((s->on_ms * 100 + s->total_ms / 2) / s->total_ms);
}

void wf_screentime_format(char *buf, size_t n, unsigned long ms)
{
    unsigned long sec = ms / 1000;
    unsigned long h = sec / 3600;
    unsigned long m = (sec % 3600) / 60;
    unsigned long s = sec % 60;
    snprintf(buf, n, "%lu:%02lu:%02lu", h, m, s);
}
