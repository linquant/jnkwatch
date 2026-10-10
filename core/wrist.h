#pragma once
/* Pure lift-to-wake detector from accelerometer samples. No platform dep.
 *
 * Model: tilt = angle between the screen normal (+Z, out of the display) and
 * "up" (opposite gravity). Screen flat facing up -> 0deg; watch vertical
 * (arm hanging) -> ~90deg; screen facing down -> 180deg. A viewing posture is
 * a small-ish tilt; resting is a large tilt. Hysteresis avoids flapping. */
#include <stdbool.h>

/* Default thresholds (= "Moyenne" sensitivity). */
#define WF_WRIST_UP_TH   55.0 /* tilt below this = viewing  */
#define WF_WRIST_DOWN_TH 75.0 /* tilt above this = resting  */

/* Sensitivity levels: 0 Basse, 1 Moyenne (default), 2 Haute. Higher = wakes at
 * a larger tilt, so a smaller raise triggers it (but more false wakes). */
#define WF_WRIST_SENS_COUNT   3
#define WF_WRIST_SENS_DEFAULT 1

typedef struct {
    bool   raised;
    double up_th;   /* tilt below this = viewing (fires a wake)  */
    double down_th; /* tilt above this = resting (re-arms)       */
} wf_wrist_t;

/* Resets state and applies the default (Moyenne) sensitivity. */
void wf_wrist_init(wf_wrist_t *w);

/* Set the detection thresholds from a sensitivity level (clamped to
 * [0, WF_WRIST_SENS_COUNT-1]). Keeps the current raised/resting state. */
void wf_wrist_set_sensitivity(wf_wrist_t *w, int level);

/* Tilt of the screen normal from vertical, in degrees [0,180]. ax/ay/az in g. */
double wf_wrist_tilt_deg(double ax, double ay, double az);

/* Feed one accelerometer sample. Returns true exactly once on the
 * resting -> viewing transition (i.e. a wake gesture completed). */
bool wf_wrist_update(wf_wrist_t *w, double ax, double ay, double az);
