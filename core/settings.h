#pragma once
/* Pure settings state. No LVGL / platform dependency. */
#include <stdbool.h>

#define WF_BRIGHT_MIN 10   /* never let the screen go fully dark */
#define WF_BRIGHT_MAX 100

/* Screen-sleep timeout choices (ms). */
#define WF_SLEEP_5S  5000
#define WF_SLEEP_10S 10000

typedef struct {
    int brightness;           /* backlight %, clamped to [MIN,MAX] */
    bool wrist_wake;          /* lift-to-wake enabled */
    int wrist_sens;           /* lift-to-wake sensitivity level (0..2, see wrist.h) */
    int sleep_ms;             /* screen-sleep timeout, one of WF_SLEEP_5S/10S */
    bool stay_awake_charging; /* keep the screen on while on USB/mains power */
    bool power_saver;         /* "watch only" ultra-low-power mode: on sleep show
                                 an always-on clock and (on device) deep-sleep */
} wf_settings_t;

void wf_settings_init(wf_settings_t *s);
/* Clamp + store brightness; returns the clamped value. */
int wf_settings_set_brightness(wf_settings_t *s, int pct);
void wf_settings_set_wrist_wake(wf_settings_t *s, bool on);
/* Clamp + store the lift-to-wake sensitivity level; returns the clamped value. */
int wf_settings_set_wrist_sens(wf_settings_t *s, int level);
/* Snap to the nearest allowed sleep timeout (5s or 10s); returns it. */
int wf_settings_set_sleep_ms(wf_settings_t *s, int ms);
void wf_settings_set_stay_awake_charging(wf_settings_t *s, bool on);
void wf_settings_set_power_saver(wf_settings_t *s, bool on);
