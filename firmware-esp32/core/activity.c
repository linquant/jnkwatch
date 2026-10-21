#include "activity.h"
#include <math.h>

#define ALPHA 0.15 /* smoothing factor */

double wf_activity_instant(double ax, double ay, double az)
{
    double mag = sqrt(ax * ax + ay * ay + az * az);
    double e = fabs(mag - 1.0) * WF_ACT_GAIN;
    if (e < 0.0) e = 0.0;
    if (e > 1.0) e = 1.0;
    return e;
}

void wf_activity_init(wf_activity_t *a)
{
    a->energy = 0.0;
}

double wf_activity_update(wf_activity_t *a, double ax, double ay, double az)
{
    double inst = wf_activity_instant(ax, ay, az);
    a->energy += (inst - a->energy) * ALPHA;
    if (a->energy < 0.0) a->energy = 0.0;
    if (a->energy > 1.0) a->energy = 1.0;
    return a->energy;
}
