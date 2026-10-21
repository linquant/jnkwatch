#include "level.h"
#include <math.h>

void wf_level_compute(double ax, double ay, double az, wf_level_t *out)
{
    const double R2D = 57.29577951308232; /* 180 / pi */

    double mag = sqrt(ax * ax + ay * ay + az * az);
    if (mag < 1e-6) {
        /* Free-fall / bad sample: report flat, and never "level". */
        out->roll_deg = out->pitch_deg = out->tilt_deg = 0.0;
        out->bx = out->by = 0.0;
        out->level = false;
        return;
    }

    /* Tilt of each axis relative to the screen normal. */
    out->roll_deg  = atan2(ax, az) * R2D;
    out->pitch_deg = atan2(ay, az) * R2D;

    /* Total tilt of the screen from horizontal = angle of +Z from "up". */
    double c = az / mag;
    if (c > 1.0) c = 1.0; else if (c < -1.0) c = -1.0;
    out->tilt_deg = acos(c) * R2D;

    /* Ball-in-a-bowl: normalized in-plane gravity -> the low side. Since it is
     * the gravity vector normalized, bx^2 + by^2 <= 1 (stays in the unit disk). */
    out->bx = ax / mag;
    out->by = ay / mag;

    out->level = out->tilt_deg <= WF_LEVEL_TOL_DEG;
}
