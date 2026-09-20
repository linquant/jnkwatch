#include "watchface_logic.h"
#include <math.h>
#include <stdio.h>

#define WF_PI 3.14159265358979323846

double wf_second_fraction(int sec)
{
    return sec / 60.0;
}

double wf_second_fraction_ms(int sec, int ms)
{
    return (sec + ms / 1000.0) / 60.0;
}

wf_batt_level_t wf_battery_level(int pct)
{
    if (pct <= 5) return WF_BATT_CRIT;
    if (pct <= 20) return WF_BATT_LOW;
    return WF_BATT_OK;
}

double wf_minute_fraction(int min, int sec)
{
    return (min + sec / 60.0) / 60.0;
}

double wf_hour_fraction(int hour, int min)
{
    return ((hour % 12) + min / 60.0) / 12.0;
}

wf_hand_t wf_hand_geometry(double cx, double cy, double f, double len, double tail)
{
    double a = 2.0 * WF_PI * f;
    double sx = sin(a);
    double cy_up = -cos(a); /* screen y grows downward, so 12 o'clock is -cos */

    wf_hand_t h;
    h.x1 = cx + len * sx;
    h.y1 = cy + len * cy_up;
    h.x0 = cx - tail * sx;
    h.y0 = cy - tail * cy_up;
    return h;
}

void wf_format_time(char *buf, size_t n, int hour, int min)
{
    snprintf(buf, n, "%02d:%02d", hour, min);
}

void wf_format_date(char *buf, size_t n, int mday, int mon0)
{
    static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    const char *m = (mon0 >= 0 && mon0 < 12) ? months[mon0] : "???";
    snprintf(buf, n, "%02d %s", mday, m);
}

unsigned wf_seven_seg_mask(int digit)
{
    static const unsigned seg[10] = {
        WF_SEG_A|WF_SEG_B|WF_SEG_C|WF_SEG_D|WF_SEG_E|WF_SEG_F,         /* 0 */
        WF_SEG_B|WF_SEG_C,                                            /* 1 */
        WF_SEG_A|WF_SEG_B|WF_SEG_G|WF_SEG_E|WF_SEG_D,                 /* 2 */
        WF_SEG_A|WF_SEG_B|WF_SEG_G|WF_SEG_C|WF_SEG_D,                 /* 3 */
        WF_SEG_F|WF_SEG_G|WF_SEG_B|WF_SEG_C,                          /* 4 */
        WF_SEG_A|WF_SEG_F|WF_SEG_G|WF_SEG_C|WF_SEG_D,                 /* 5 */
        WF_SEG_A|WF_SEG_F|WF_SEG_G|WF_SEG_E|WF_SEG_C|WF_SEG_D,        /* 6 */
        WF_SEG_A|WF_SEG_B|WF_SEG_C,                                   /* 7 */
        WF_SEG_A|WF_SEG_B|WF_SEG_C|WF_SEG_D|WF_SEG_E|WF_SEG_F|WF_SEG_G,/* 8 */
        WF_SEG_A|WF_SEG_B|WF_SEG_C|WF_SEG_D|WF_SEG_F|WF_SEG_G,        /* 9 */
    };
    return (digit >= 0 && digit <= 9) ? seg[digit] : 0u;
}

const char *wf_weekday_abbrev(int wday)
{
    static const char *days[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    return (wday >= 0 && wday < 7) ? days[wday] : "???";
}

void wf_format_month_day(char *buf, size_t n, int mon0, int mday)
{
    snprintf(buf, n, "%d-%d", mon0 + 1, mday);
}
