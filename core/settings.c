#include "settings.h"
#include "wrist.h"   /* WF_WRIST_SENS_COUNT / WF_WRIST_SENS_DEFAULT */

void wf_settings_init(wf_settings_t *s)
{
    s->brightness = 80;
    s->wrist_wake = true;
    s->wrist_sens = WF_WRIST_SENS_DEFAULT;
    s->sleep_ms = WF_SLEEP_10S;
    s->stay_awake_charging = true;  /* on by default: don't sleep while plugged in */
    s->power_saver = false;         /* off by default: normal veille, BLE stays up */
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

int wf_settings_set_wrist_sens(wf_settings_t *s, int level)
{
    if (level < 0) level = 0;
    if (level >= WF_WRIST_SENS_COUNT) level = WF_WRIST_SENS_COUNT - 1;
    s->wrist_sens = level;
    return level;
}

void wf_settings_set_stay_awake_charging(wf_settings_t *s, bool on)
{
    s->stay_awake_charging = on;
}

void wf_settings_set_power_saver(wf_settings_t *s, bool on)
{
    s->power_saver = on;
}

int wf_settings_set_sleep_ms(wf_settings_t *s, int ms)
{
    /* midpoint between the two allowed values */
    s->sleep_ms = (ms < (WF_SLEEP_5S + WF_SLEEP_10S) / 2) ? WF_SLEEP_5S : WF_SLEEP_10S;
    return s->sleep_ms;
}
