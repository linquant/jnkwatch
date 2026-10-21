#pragma once
/* Pure bubble-level (spirit level) math from an accelerometer sample. No
 * platform/LVGL dependency, so it is unit-tested on the PC.
 *
 * A spirit level measures tilt against GRAVITY, so it uses the accelerometer
 * (which reads the gravity vector at rest), NOT the gyroscope (which only gives
 * angular rate and cannot yield an absolute orientation). Convention matches the
 * rest of the firmware: screen flat and facing up -> a ~= (0, 0, +1) g.
 *
 * The "ball" follows the ball-in-a-bowl metaphor: it rolls to the LOW side
 * (toward the in-plane gravity), and sits dead-center when the surface is flat. */
#include <stdbool.h>

#define WF_LEVEL_TOL_DEG 2.0  /* total tilt at/under which we report "level" */

typedef struct {
    double roll_deg;   /* tilt about Y (left/right) = atan2(ax, az), degrees   */
    double pitch_deg;  /* tilt about X (front/back) = atan2(ay, az), degrees   */
    double tilt_deg;   /* total tilt of the screen from horizontal, [0,180]    */
    double bx, by;     /* ball offset in the unit disk [-1,1]; flat -> (0,0)   */
    bool   level;      /* true when tilt_deg <= WF_LEVEL_TOL_DEG               */
} wf_level_t;

/* Fill *out from one accelerometer sample (in g). Robust to a zero/tiny vector
 * (reports flat and not level rather than dividing by zero). */
void wf_level_compute(double ax, double ay, double az, wf_level_t *out);
