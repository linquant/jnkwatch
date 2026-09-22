#pragma once
/* Pure accounting of how long the screen has been ON vs total uptime — a proxy
 * for AMOLED energy use. NO LVGL/platform dependency; unit-tested. The UI feeds
 * it a per-frame delta and whether the display is currently lit. */
#include <stddef.h>

typedef struct {
    unsigned long on_ms;    /* accumulated time the screen was ON  */
    unsigned long total_ms; /* accumulated total time (uptime)     */
} wf_screentime_t;

void wf_screentime_init(wf_screentime_t *s);

/* Add dt_ms to total; also to on_ms when screen_on is non-zero. */
void wf_screentime_update(wf_screentime_t *s, unsigned dt_ms, int screen_on);

/* On-time as a percentage of total (0..100), 0 if no time elapsed. */
int wf_screentime_on_pct(const wf_screentime_t *s);

/* Format a millisecond duration as "H:MM:SS" (hours unbounded). */
void wf_screentime_format(char *buf, size_t n, unsigned long ms);
