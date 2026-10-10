#include "pedometer.h"
#include <math.h>

/* Gravity low-pass time constant (ms). The baseline must track slow tilts (so
 * ac stays centred on 0) but not the ~0.5..4 Hz step oscillation itself, so it
 * is kept well slower than a step period. */
#define GRAV_TAU_MS 600.0

/* Dynamic-component thresholds (g). A step must push ac above TH_HI; ac must
 * then fall back below TH_LO before another step can arm (hysteresis). TH_HI is
 * set above typical wrist jitter / typing so resting does not accrue steps. */
#define TH_HI 0.14
#define TH_LO 0.04

/* Minimum interval between two counted steps (ms): caps cadence at ~5 steps/s
 * (fast running) and rejects a double count on a single multi-sample peak. */
#define STEP_MIN_MS 200

void ped_reset(ped_t *p)
{
    p->grav = 0.0;
    p->above = 0;
    p->started = 0;
    p->prev_ms = 0;
    p->last_ms = 0;
    p->count = 0;
}

int ped_update(ped_t *p, double ax, double ay, double az, uint32_t t_ms)
{
    double mag = sqrt(ax * ax + ay * ay + az * az);

    if (!p->started) {
        /* Seed the baseline with the first sample so ac starts near 0, and
         * backdate last_ms so the very first real peak can count immediately. */
        p->grav = mag;
        p->prev_ms = t_ms;
        p->last_ms = t_ms - STEP_MIN_MS;
        p->started = 1;
        return 0;
    }

    uint32_t dt = t_ms - p->prev_ms; /* monotonic ms; wrap-safe unsigned diff */
    p->prev_ms = t_ms;

    /* dt-aware one-pole low-pass: alpha = dt / (tau + dt). dt==0 -> no change. */
    double alpha = (double)dt / (GRAV_TAU_MS + (double)dt);
    p->grav += alpha * (mag - p->grav);

    double ac = mag - p->grav;

    if (!p->above) {
        if (ac > TH_HI) {
            p->above = 1;
            if ((uint32_t)(t_ms - p->last_ms) >= STEP_MIN_MS) {
                p->count++;
                p->last_ms = t_ms;
                return 1;
            }
        }
    } else if (ac < TH_LO) {
        p->above = 0;
    }
    return 0;
}

uint32_t ped_count(const ped_t *p)
{
    return p->count;
}
