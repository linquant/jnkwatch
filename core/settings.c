#include "settings.h"

void wf_settings_init(wf_settings_t *s)
{
    s->brightness = 80;
    s->wrist_wake = true;
}

int wf_settings_set_brightness(wf_settings_t *s, int pct)
{
    if (pct < WF_BRIGHT_MIN) pct = WF_BRIGHT_MIN;
    if (pct > WF_BRIGHT_MAX) pct = WF_BRIGHT_MAX;
    s->brightness = pct;
    return pct;
}

void wf_settings_set_wrist_wake(wf_settings_t *s, bool on)
{
    s->wrist_wake = on;
}
