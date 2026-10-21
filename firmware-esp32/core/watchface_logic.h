#pragma once
/* Pure watchface logic: geometry + formatting, with NO LVGL / platform
 * dependency. Everything here is unit-tested (see tests/). The LVGL layer in
 * ui/ only calls into these functions and turns the results into widgets. */
#include <stddef.h>

/* Endpoints of a clock hand, in pixel coordinates. */
typedef struct {
    double x0, y0; /* tail (behind the center hub) */
    double x1, y1; /* tip  (outer end) */
} wf_hand_t;

/* Fraction of a full turn (0..1), measured clockwise from 12 o'clock. */
double wf_second_fraction(int sec);              /* sec in [0,59]        */
double wf_second_fraction_ms(int sec, int ms);   /* smooth, sub-second   */
double wf_minute_fraction(int min, int sec);     /* smooth with seconds  */
double wf_hour_fraction(int hour, int min);      /* hour any int; %12    */

/* Geometry of a hand centered at (cx,cy): tip is `len` px out at fraction f,
 * tail is `tail` px on the opposite side. f is clockwise from 12 o'clock. */
wf_hand_t wf_hand_geometry(double cx, double cy, double f, double len, double tail);

/* "HH:MM" (24h, zero-padded). Writes at most n bytes incl. NUL. */
void wf_format_time(char *buf, size_t n, int hour, int min);

/* "DD MON" e.g. "14 SEP". mon0 is 0-based (0=Jan). */
void wf_format_date(char *buf, size_t n, int mday, int mon0);

/* --- Casio-style digital watchface helpers --- */

/* Seven-segment layout, one bit per segment:
 *      AAA
 *     F   B
 *     F   B
 *      GGG
 *     E   C
 *     E   C
 *      DDD                                                              */
enum {
    WF_SEG_A = 1u << 0,
    WF_SEG_B = 1u << 1,
    WF_SEG_C = 1u << 2,
    WF_SEG_D = 1u << 3,
    WF_SEG_E = 1u << 4,
    WF_SEG_F = 1u << 5,
    WF_SEG_G = 1u << 6,
};

/* Bitmask of lit segments for a decimal digit 0..9; 0 for anything else. */
unsigned wf_seven_seg_mask(int digit);

/* Uppercase 3-letter weekday, wday 0=Sunday..6=Saturday; "???" if invalid. */
const char *wf_weekday_abbrev(int wday);

/* "M-D" with no leading zeros, e.g. "9-14". mon0 is 0-based (0=Jan). */
void wf_format_month_day(char *buf, size_t n, int mon0, int mday);

/* Step count grouped by thousands with a space, e.g. 6240 -> "6 240". Writes at
 * most n bytes incl. NUL. */
void wf_format_steps(char *buf, size_t n, unsigned steps);

/* Battery charge level bucket (drives the battery ring colour). */
typedef enum {
    WF_BATT_OK = 0,   /* > 20%        */
    WF_BATT_LOW = 1,  /* 6..20%       */
    WF_BATT_CRIT = 2, /* <= 5%        */
} wf_batt_level_t;

wf_batt_level_t wf_battery_level(int pct);

/* Single-cell Li-ion state of charge from its (lightly loaded) voltage in mV:
 * piecewise-linear over a typical LiPo discharge curve, clamped to 0..100
 * (<= 3300 mV -> 0, >= 4200 mV -> 100). */
int wf_battery_pct_from_mv(int mv);

/* Battery icon fill, 0 (empty) .. 4 (full), for a charge in % (clamped). */
int wf_battery_bars(int pct);
