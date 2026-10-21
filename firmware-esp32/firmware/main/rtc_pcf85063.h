#pragma once
#include <stdbool.h>
#include <time.h>

/* PCF85063 RTC on the shared I2C bus (bus must already be installed, e.g. by
 * touch_ft3168_init). Times are kept in UTC in the chip; the caller applies the
 * timezone. */

/* Read the RTC. Returns false if the oscillator-stop flag is set (time never
 * set / power was lost) — in that case *out is not meaningful. */
bool rtc_pcf85063_get(struct tm *out);

/* Set the RTC (clears the oscillator-stop flag). */
void rtc_pcf85063_set(const struct tm *t);
