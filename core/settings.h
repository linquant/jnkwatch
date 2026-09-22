#pragma once
/* Pure settings state. No LVGL / platform dependency. */
#include <stdbool.h>

#define WF_BRIGHT_MIN 10   /* never let the screen go fully dark */
#define WF_BRIGHT_MAX 100

/* Screen-sleep timeout choices (ms). */
#define WF_SLEEP_5S  5000
#define WF_SLEEP_10S 10000

typedef struct {
    int brightness;   /* backlight %, clamped to [MIN,MAX] */
    bool wrist_wake;  /* lift-to-wake enabled */
    int sleep_ms;     /* screen-sleep timeout, one of WF_SLEEP_5S/10S */
} wf_settings_t;

void wf_settings_init(wf_settings_t *s);
/* Clamp + store brightness; returns the clamped value. */
int wf_settings_set_brightness(wf_settings_t *s, int pct);
void wf_settings_set_wrist_wake(wf_settings_t *s, bool on);
/* Snap to the nearest allowed sleep timeout (5s or 10s); returns it. */
int wf_settings_set_sleep_ms(wf_settings_t *s, int ms);
