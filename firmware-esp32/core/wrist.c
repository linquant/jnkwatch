#include "wrist.h"
#include <math.h>

/* Fixed-alpha smoothing (the detector is fed at a roughly steady poll rate and
 * has no timestamp). grav follows slow tilts but not the ~0.5..4 Hz step
 * oscillation; motion is a short memory of the dynamic acceleration so a single
 * lift transient decays quickly while sustained walking stays elevated. */
#define WF_WRIST_GRAV_A   0.05
#define WF_WRIST_MOTION_A 0.25

void wf_wrist_set_sensitivity(wf_wrist_t *w, int level)
{
    if (level < 0) level = 0;
    if (level >= WF_WRIST_SENS_COUNT) level = WF_WRIST_SENS_COUNT - 1;
    /* "up" threshold grows with sensitivity (earlier wake). Keep the 20deg
     * hysteresis gap so the mid-band stays stable at every level. Level 1
     * reproduces WF_WRIST_UP_TH / WF_WRIST_DOWN_TH. */
    static const double up[WF_WRIST_SENS_COUNT] = { 45.0, 55.0, 65.0 };
    /* Motion gate (g), issue #22: Basse (0.08) is below the pedometer step
     * threshold (0.14 g) so walking cannot wake the screen; Moyenne sits ~at it;
     * Haute is effectively open (reactive, tolerates motion). */
    static const double gate[WF_WRIST_SENS_COUNT] = { 0.08, 0.15, 1.00 };
    w->up_th       = up[level];
    w->down_th     = up[level] + 20.0;
    w->motion_gate = gate[level];
}

void wf_wrist_init(wf_wrist_t *w)
{
    w->raised = false;
    w->grav = 0.0;
    w->motion = 0.0;
    w->seeded = false;
    wf_wrist_set_sensitivity(w, WF_WRIST_SENS_DEFAULT);
}

double wf_wrist_tilt_deg(double ax, double ay, double az)
{
    double mag = sqrt(ax * ax + ay * ay + az * az);
    if (mag < 1e-6) return 90.0; /* free-fall / no data: treat as ambiguous */
    double c = az / mag;
    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;
    return acos(c) * 180.0 / 3.14159265358979323846;
}

bool wf_wrist_update(wf_wrist_t *w, double ax, double ay, double az)
{
    double mag = sqrt(ax * ax + ay * ay + az * az);
    if (!w->seeded) {
        w->grav = mag;       /* seed so dynamic accel starts near 0 */
        w->motion = 0.0;
        w->seeded = true;
    } else {
        w->grav += WF_WRIST_GRAV_A * (mag - w->grav);
        double ac = fabs(mag - w->grav);
        w->motion += WF_WRIST_MOTION_A * (ac - w->motion);
    }

    double tilt = wf_wrist_tilt_deg(ax, ay, az);
    if (w->raised) {
        if (tilt > w->down_th) w->raised = false; /* back to resting */
        return false;
    }
    /* Fire only on a settled raise: viewing tilt AND low enough motion. If the
     * tilt is reached while moving too much (e.g. walking), stay un-raised so a
     * later, calmer sample at the same tilt can still wake. */
    if (tilt < w->up_th && w->motion <= w->motion_gate) {
        w->raised = true;
        return true; /* wake gesture completed */
    }
    return false;
}
