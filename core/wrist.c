#include "wrist.h"
#include <math.h>

void wf_wrist_init(wf_wrist_t *w)
{
    w->raised = false;
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
    double tilt = wf_wrist_tilt_deg(ax, ay, az);
    if (w->raised) {
        if (tilt > WF_WRIST_DOWN_TH) w->raised = false; /* back to resting */
        return false;
    }
    if (tilt < WF_WRIST_UP_TH) {
        w->raised = true;
        return true; /* wake gesture completed */
    }
    return false;
}
