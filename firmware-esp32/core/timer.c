#include "timer.h"
#include <stdio.h>

static int clamp_ms(int ms)
{
    if (ms < WF_TMR_MIN_MS) return WF_TMR_MIN_MS;
    if (ms > WF_TMR_MAX_MS) return WF_TMR_MAX_MS;
    return ms;
}

void wf_timer_init(wf_timer_t *t, int duration_ms)
{
    t->duration_ms = clamp_ms(duration_ms);
    t->remaining_ms = t->duration_ms;
    t->state = WF_TMR_IDLE;
}

void wf_timer_set_duration(wf_timer_t *t, int duration_ms)
{
    if (t->state != WF_TMR_IDLE) return;
    t->duration_ms = clamp_ms(duration_ms);
    t->remaining_ms = t->duration_ms;
}

void wf_timer_start(wf_timer_t *t)
{
    if (t->state == WF_TMR_IDLE || t->state == WF_TMR_PAUSED)
        t->state = WF_TMR_RUNNING;
}

void wf_timer_pause(wf_timer_t *t)
{
    if (t->state == WF_TMR_RUNNING) t->state = WF_TMR_PAUSED;
}

void wf_timer_toggle(wf_timer_t *t)
{
    if (t->state == WF_TMR_RUNNING) wf_timer_pause(t);
    else wf_timer_start(t);
}

void wf_timer_reset(wf_timer_t *t)
{
    t->remaining_ms = t->duration_ms;
    t->state = WF_TMR_IDLE;
}

void wf_timer_tick(wf_timer_t *t, int dt_ms)
{
    if (t->state != WF_TMR_RUNNING) return;
    t->remaining_ms -= dt_ms;
    if (t->remaining_ms <= 0) {
        t->remaining_ms = 0;
        t->state = WF_TMR_DONE;
    }
}

double wf_timer_remaining_fraction(const wf_timer_t *t)
{
    if (t->duration_ms <= 0) return 0.0;
    return (double)t->remaining_ms / t->duration_ms;
}

void wf_timer_format(const wf_timer_t *t, char *buf, size_t n)
{
    int sec = (t->remaining_ms + 999) / 1000; /* round up */
    int mm = sec / 60;
    int ss = sec % 60;
    if (mm > 99) mm = 99;
    snprintf(buf, n, "%02d:%02d", mm, ss);
}
