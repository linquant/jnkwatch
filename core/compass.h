#pragma once
/* Pure compass math. No LVGL / platform / sensor dependency. */

/* Wrap any angle into [0, 360). */
double wf_heading_normalize(double deg);

/* Shortest signed difference to rotate `from` onto `to`, in (-180, 180].
 * Handles the 359->0 wrap (used to smooth needle motion). */
double wf_angle_diff(double from, double to);

/* 8-wind cardinal name for a heading: "N","NE","E","SE","S","SW","W","NW". */
const char *wf_cardinal8(double deg);
