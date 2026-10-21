#pragma once
/* Pure weather store: the latest weather pushed from the phone (temperature,
 * condition, location name). NO LVGL/platform dependency, unit-tested (see
 * tests/test_weather.c). The BLE layer (or the PC mock) writes it via
 * wf_weather_set; the clock face only reads it. The watch never fetches weather
 * itself — the phone is the source. */
#include <stdbool.h>

#define WF_WX_LOC_LEN 24   /* location name, incl. NUL */

/* Condition codes — must match the phone's WMO->cond mapping in
 * docs/ble-protocol.md. */
typedef enum {
    WF_WX_CLEAR = 0,
    WF_WX_PARTLY,
    WF_WX_CLOUDY,
    WF_WX_FOG,
    WF_WX_RAIN,
    WF_WX_SNOW,
    WF_WX_STORM,
    WF_WX_COND_COUNT,
} wf_wx_cond_t;

typedef struct {
    int  temp_c;                  /* whole degrees Celsius            */
    int  cond;                    /* wf_wx_cond_t, clamped            */
    char location[WF_WX_LOC_LEN]; /* city name, truncated            */
    bool valid;                   /* false until the first push       */
} wf_weather_t;

/* Reset to "no data" (valid = false). */
void wf_weather_init(wf_weather_t *w);

/* Store a reading: cond is clamped to a valid code, location is truncated
 * (NULL -> empty). Marks the store valid. */
void wf_weather_set(wf_weather_t *w, int temp_c, int cond, const char *location);
