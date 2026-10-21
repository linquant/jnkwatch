#pragma once
/* Pure countdown-timer state machine. No LVGL / platform dependency. */
#include <stddef.h>

typedef enum {
    WF_TMR_IDLE,    /* stopped at the configured duration        */
    WF_TMR_RUNNING, /* counting down                             */
    WF_TMR_PAUSED,  /* held part-way                             */
    WF_TMR_DONE,    /* reached 00:00                             */
} wf_tmr_state_t;

typedef struct {
    wf_tmr_state_t state;
    int duration_ms;  /* configured total */
    int remaining_ms; /* counts down to 0 */
} wf_timer_t;

#define WF_TMR_MIN_MS (60 * 1000)       /* 1 minute  */
#define WF_TMR_MAX_MS (99 * 60 * 1000)  /* 99 minutes */

void wf_timer_init(wf_timer_t *t, int duration_ms);
/* Change the total; only honored while IDLE. Clamps to [MIN,MAX]. */
void wf_timer_set_duration(wf_timer_t *t, int duration_ms);
void wf_timer_start(wf_timer_t *t);   /* IDLE/PAUSED -> RUNNING */
void wf_timer_pause(wf_timer_t *t);   /* RUNNING -> PAUSED */
/* RUNNING -> pause; IDLE/PAUSED -> start; DONE -> no-op. */
void wf_timer_toggle(wf_timer_t *t);
void wf_timer_reset(wf_timer_t *t);   /* -> IDLE, remaining = duration */
void wf_timer_tick(wf_timer_t *t, int dt_ms); /* advance while RUNNING */

/* Remaining fraction 1..0 (full -> empty). 0 if duration is 0. */
double wf_timer_remaining_fraction(const wf_timer_t *t);
/* "MM:SS" of the remaining time, rounded up so it starts at the full value. */
void wf_timer_format(const wf_timer_t *t, char *buf, size_t n);
