#include "steps.h"

void wf_steps_init(wf_steps_t *s)
{
    ped_reset(&s->ped);
    s->today = 0;
    s->yday = -1;
}

void wf_steps_restore(wf_steps_t *s, uint32_t today, int yday)
{
    ped_reset(&s->ped);
    s->today = today;
    s->yday = yday;
}

int wf_steps_update(wf_steps_t *s, double ax, double ay, double az,
                    uint32_t t_ms, int yday)
{
    if (s->yday < 0) {
        s->yday = yday;          /* first ever sample adopts the current day */
    } else if (yday != s->yday) {
        s->yday = yday;          /* midnight / year wrap -> new day */
        s->today = 0;
    }
    int n = ped_update(&s->ped, ax, ay, az, t_ms);
    s->today += (uint32_t)n;
    return n;
}

uint32_t wf_steps_today(const wf_steps_t *s) { return s->today; }

int wf_steps_yday(const wf_steps_t *s) { return s->yday; }
