#pragma once
/* Pure motion-energy estimator from accelerometer samples. No platform dep.
 *
 * A static device reads ~1g, so |‖a‖ - 1| captures linear shaking/motion while
 * ignoring slow tilts (which keep ‖a‖≈1). The result is a smoothed 0..1 energy
 * that drives the "living" watchface (calm blue -> energetic magenta). */

#define WF_ACT_GAIN 2.5   /* maps |‖a‖-1| to 0..1 */

typedef struct {
    double energy; /* smoothed 0..1 */
} wf_activity_t;

void wf_activity_init(wf_activity_t *a);

/* Instantaneous normalized energy for one sample, 0..1. */
double wf_activity_instant(double ax, double ay, double az);

/* Feed a sample; returns the exponentially-smoothed energy, 0..1. */
double wf_activity_update(wf_activity_t *a, double ax, double ay, double az);
