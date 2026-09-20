#pragma once
/* Pure lift-to-wake detector from accelerometer samples. No platform dep.
 *
 * Model: tilt = angle between the screen normal (+Z, out of the display) and
 * "up" (opposite gravity). Screen flat facing up -> 0deg; watch vertical
 * (arm hanging) -> ~90deg; screen facing down -> 180deg. A viewing posture is
 * a small-ish tilt; resting is a large tilt. Hysteresis avoids flapping. */
#include <stdbool.h>

#define WF_WRIST_UP_TH   55.0 /* tilt below this = viewing  */
#define WF_WRIST_DOWN_TH 75.0 /* tilt above this = resting  */

typedef struct {
    bool raised;
} wf_wrist_t;

void wf_wrist_init(wf_wrist_t *w);

/* Tilt of the screen normal from vertical, in degrees [0,180]. ax/ay/az in g. */
double wf_wrist_tilt_deg(double ax, double ay, double az);

/* Feed one accelerometer sample. Returns true exactly once on the
 * resting -> viewing transition (i.e. a wake gesture completed). */
bool wf_wrist_update(wf_wrist_t *w, double ax, double ay, double az);
