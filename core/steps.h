#pragma once
#include <stdint.h>
#include "pedometer.h"

/* Daily step tracker: wraps the pure pedometer with a "today" total that resets
 * at local midnight. No platform dependency — the UI feeds it accel samples and
 * the current local day-of-year each tick; persistence (load at boot, save) is
 * done by the platform layer via wf_steps_restore()/wf_steps_today() and the
 * watch_steps_load()/watch_steps_save() HAL hooks. Unit-tested in tests/. */

typedef struct {
    ped_t    ped;    /* step detector                                   */
    uint32_t today;  /* steps counted for the current day               */
    int      yday;   /* day-of-year (tm_yday) `today` belongs to; -1 unset */
} wf_steps_t;

/* Fresh tracker: zero steps, day unset (adopted from the first sample). */
void wf_steps_init(wf_steps_t *s);

/* Restore a persisted total for a given day-of-year (from NVS at boot). If the
 * current day later differs, the next update() rolls it over to 0. */
void wf_steps_restore(wf_steps_t *s, uint32_t today, int yday);

/* Feed one accel sample (g) at t_ms, with yday = current local tm_yday. Resets
 * the daily total on a day change (midnight / year wrap). Returns steps added
 * on this call (0 or 1). */
int wf_steps_update(wf_steps_t *s, double ax, double ay, double az,
                    uint32_t t_ms, int yday);

/* Steps counted for the current day. */
uint32_t wf_steps_today(const wf_steps_t *s);

/* Day-of-year the current total belongs to (-1 if still unset). */
int wf_steps_yday(const wf_steps_t *s);
