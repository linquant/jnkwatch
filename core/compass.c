#include "compass.h"
#include <math.h>

double wf_heading_normalize(double deg)
{
    double r = fmod(deg, 360.0);
    if (r < 0) r += 360.0;
    return r;
}

double wf_angle_diff(double from, double to)
{
    double d = wf_heading_normalize(to - from);
    if (d > 180.0) d -= 360.0;
    return d;
}

const char *wf_cardinal8(double deg)
{
    static const char *names[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int idx = (int)floor((wf_heading_normalize(deg) + 22.5) / 45.0) % 8;
    return names[idx];
}
