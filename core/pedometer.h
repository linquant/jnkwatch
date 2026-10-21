#pragma once
#include <stdint.h>

/* Pure software pedometer: counts discrete steps from accelerometer magnitude.
 * No platform/LVGL dependency (only libm sqrt); unit-tested in tests/.
 *
 * Accelerometer only — no gyro. The gyro is the big IMU consumer, so a step
 * counter that never needs it can run while the gyro stays off.
 *
 * Method: take the sample magnitude ‖a‖ (≈1g at rest), track gravity with a
 * slow low-pass, and look at the dynamic component ac = ‖a‖ - gravity. Each
 * walking step produces one ac peak; we count a step on the rising crossing of
 * a high threshold, with hysteresis (a lower release threshold) and a time
 * refractory so a single peak — or sensor noise — is not counted twice.
 *
 * It is rate-adaptive: ped_update() takes the sample timestamp, so a varying
 * poll rate (e.g. 30 Hz screen-on vs a slower cadence screen-off) does not need
 * a fixed dt. Reliable detection still wants roughly >=20 Hz sampling; below
 * that, fast steps start to be missed (Nyquist). */

typedef struct {
    double   grav;     /* gravity baseline, low-pass of ‖a‖ (g)        */
    int      above;    /* 1 while ac is above the high threshold       */
    int      started;  /* 0 until the first sample seeds the filter    */
    uint32_t prev_ms;  /* timestamp of the previous sample (ms)        */
    uint32_t last_ms;  /* timestamp of the last accepted step (ms)     */
    uint32_t count;    /* steps since the last reset                   */
} ped_t;

/* Reset to zero steps and an unseeded filter. */
void ped_reset(ped_t *p);

/* Feed one accelerometer sample (g), captured at t_ms (monotonic, ms).
 * Returns the number of NEW steps detected on this sample (0 or 1). */
int ped_update(ped_t *p, double ax, double ay, double az, uint32_t t_ms);

/* Total steps counted since the last reset. */
uint32_t ped_count(const ped_t *p);
