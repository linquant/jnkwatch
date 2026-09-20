#pragma once
/* Pure settings state. No LVGL / platform dependency. */
#include <stdbool.h>

#define WF_BRIGHT_MIN 10   /* never let the screen go fully dark */
#define WF_BRIGHT_MAX 100

typedef struct {
    int brightness;   /* backlight %, clamped to [MIN,MAX] */
    bool wrist_wake;  /* lift-to-wake enabled */
} wf_settings_t;

void wf_settings_init(wf_settings_t *s);
/* Clamp + store brightness; returns the clamped value. */
int wf_settings_set_brightness(wf_settings_t *s, int pct);
void wf_settings_set_wrist_wake(wf_settings_t *s, bool on);
