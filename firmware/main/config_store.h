#pragma once
#include <stdbool.h>

/* Persistent watch configuration (NVS). Survives reboots so the watch keeps the
 * last phone-pushed settings even before the companion app reconnects. */
typedef struct {
    int  brightness;   /* 10..100                    */
    int  sleep_ms;     /* WF_SLEEP_5S / WF_SLEEP_10S  */
    bool wrist_wake;
    bool stay_awake_charging; /* keep screen on while on USB/mains */
    bool power_saver;  /* "watch only" ultra-low-power mode */
    int  accent;       /* 0..ACCENT_COUNT-1           */
} wf_stored_config_t;

/* Initialise NVS (also required by NimBLE bonding) and load the saved config,
 * falling back to the firmware defaults when nothing is stored. */
void config_store_init(void);

/* Copy out the current RAM mirror of the config. */
void config_store_get(wf_stored_config_t *out);

/* Update the settings and persist them to NVS. */
void config_store_set_settings(int brightness, int sleep_ms, bool wrist_wake,
                               bool stay_awake_charging, bool power_saver,
                               int accent);
