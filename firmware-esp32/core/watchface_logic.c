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

/* Typical LiPo open-circuit curve (mV -> %), descending. */
static const struct { int mv, pct; } k_lipo[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3920, 70}, {3870, 60}, {3820, 50},
    {3790, 40},  {3770, 30}, {3740, 20}, {3680, 10}, {3450, 5},  {3300, 0},
};

int wf_battery_pct_from_mv(int mv)
{
    const int n = (int)(sizeof(k_lipo) / sizeof(k_lipo[0]));
    if (mv >= k_lipo[0].mv) return 100;
    if (mv <= k_lipo[n - 1].mv) return 0;
    for (int i = 1; i < n; i++) {
        if (mv >= k_lipo[i].mv) {
            int hi_mv = k_lipo[i - 1].mv, lo_mv = k_lipo[i].mv;
            int hi_p = k_lipo[i - 1].pct, lo_p = k_lipo[i].pct;
            /* Rounded linear interpolation inside the segment. */
            int span = hi_mv - lo_mv;
            return lo_p + ((mv - lo_mv) * (hi_p - lo_p) + span / 2) / span;
        }
    }
    return 0;
}

int wf_battery_bars(int pct)
{
    if (pct <= 10) return 0;
    if (pct <= 35) return 1;
    if (pct <= 60) return 2;
    if (pct <= 85) return 3;
    return 4;
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

void wf_format_steps(char *buf, size_t n, unsigned steps)
{
    if (n == 0) return;
    char d[12];
    int ndig = snprintf(d, sizeof d, "%u", steps);
    if (ndig < 0) ndig = 0;
    size_t o = 0;
    for (int i = 0; i < ndig; i++) {
        /* Space before every group of 3 digits counted from the right. */
        if (i > 0 && (ndig - i) % 3 == 0 && o < n - 1)
            buf[o++] = ' ';
        if (o < n - 1)
            buf[o++] = d[i];
    }
    buf[o] = '\0';
}
